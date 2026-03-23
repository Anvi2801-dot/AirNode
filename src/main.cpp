#include <iostream>
#include <opencv2/opencv.hpp>
#include <chrono>
#include "../include/MouseFactory.h"
#include "../include/HandTracker.h"

static const int   SCREEN_W            = 1512;
static const int   SCREEN_H            = 982;
static const float SMOOTH              = 0.5f;
static const float DEAD_ZONE           = 15.0f;
static const int   CLICK_FRAMES        = 8;
static const int   DOUBLE_CLICK_FRAMES = 50;  // ~1.5s hold arms double click
static const int   DRAG_FRAMES         = 80;  // ~2.5s hold starts drag
static const int   FIST_FRAMES         = 20;
static const int   PINCH_RELEASE_GRACE = 8;
static const int   DETECTION_GRACE     = 45; // frames before "no hand" is accepted;

using Clock = std::chrono::steady_clock;
using Ms    = std::chrono::milliseconds;

void drawLegend(cv::Mat& frame) {
    int ly = frame.rows - 280;
    cv::rectangle(frame, cv::Point(10, ly-15), cv::Point(500, frame.rows-10), cv::Scalar(0,0,0), -1);
    cv::putText(frame, "GESTURES:",                       cv::Point(25, ly+10),  cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(255,255,255), 2);
    cv::putText(frame, "Pinch          =  Click",         cv::Point(25, ly+40),  cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0,255,0),     1);
    cv::putText(frame, "Hold 1.5s+rel  =  Double Click",  cv::Point(25, ly+70),  cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0,200,255),   1);
    cv::putText(frame, "Hold 2.5s      =  Drag",          cv::Point(25, ly+100), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255,165,0),   1);
    cv::putText(frame, "Fist (hold)    =  Pause/Resume",  cv::Point(25, ly+130), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0,0,255),     1);
    cv::putText(frame, "2 Hands Vert   =  Scroll",        cv::Point(25, ly+160), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255,255,0),   1);
    cv::putText(frame, "3 Fingers      =  Mission Ctrl",  cv::Point(25, ly+190), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(200,0,255),   1);
    cv::putText(frame, "2H Pinch Zoom  =  Cmd +/-",       cv::Point(25, ly+220), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255,100,100), 1);
    cv::putText(frame, "2nd Hand       =  Right Click",   cv::Point(25, ly+250), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(100,255,100), 1);
}

int main() {
    IMouseController* mouse = MouseFactory::createMouse();
    HandTracker tracker("hand_landmarker.task");

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) { std::cerr << "Camera failed!" << std::endl; return -1; }
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);

    float smoothX = SCREEN_W/2.f, smoothY = SCREEN_H/2.f;
    float prevSmoothX = smoothX, prevSmoothY = smoothY;

    int  pinchHeld         = 0;
    int  pinchReleaseGrace = 0;
    bool pinchActive       = false;
    int  fistHeld          = 0;
    bool dragging          = false;
    bool clickPending      = false;
    bool dblClickArmed     = false; // armed at 1.5s, fires on release
    bool paused            = false;

    int detectionGrace = 0;

    cv::Mat frame;

    while (true) {
        cap >> frame;
        if (frame.empty()) continue;
        cv::flip(frame, frame, 1);

        HandGesture g = tracker.getGesture(frame);
        tracker.drawSkeleton(frame, g);

        // ── Stable pinch with grace period ───────────────────────────────────
        if (g.pinching) {
            pinchReleaseGrace = 0;
            pinchActive = true;
        } else {
            if (pinchActive) {
                pinchReleaseGrace++;
                if (pinchReleaseGrace >= PINCH_RELEASE_GRACE)
                    pinchActive = false;
            }
        }

        // ── Fist = toggle pause ───────────────────────────────────────────────
        if (g.detected && g.fist) {
            fistHeld++;
            if (fistHeld == FIST_FRAMES) {
                paused = !paused;
                fistHeld = 0;
                std::cout << (paused ? "PAUSED" : "RESUMED") << std::endl;
            }
        } else {
            fistHeld = 0;
        }

        drawLegend(frame);

        if (paused) {
            cv::putText(frame, "PAUSED - hold fist to resume",
                        cv::Point(30, 60), cv::FONT_HERSHEY_SIMPLEX,
                        1.0, cv::Scalar(0,0,255), 2);
            cv::imshow("GCVM (q to quit)", frame);
            if (cv::waitKey(1) == 'q') break;
            continue;
        }

        if (g.detected && !g.fist) {
            detectionGrace = 0;
            // ── Smooth + velocity-based dead zone ─────────────────────────────
            float sx = (float(g.x) / frame.cols) * SCREEN_W;
            float sy = (float(g.y) / frame.rows) * SCREEN_H;
            float newX = SMOOTH*smoothX + (1.f-SMOOTH)*sx;
            float newY = SMOOTH*smoothY + (1.f-SMOOTH)*sy;
            float dx = newX-prevSmoothX, dy = newY-prevSmoothY;
            float speed = std::sqrt(dx*dx + dy*dy);
            if (speed > DEAD_ZONE) {
                // Scale movement — slow at edges of dead zone, full speed beyond
                float scale = (speed - DEAD_ZONE) / speed;
                smoothX = prevSmoothX + dx * scale;
                smoothY = prevSmoothY + dy * scale;
                prevSmoothX = smoothX;
                prevSmoothY = smoothY;
                mouse->move(int(smoothX), int(smoothY));
            }

            // ── Pinch state machine ───────────────────────────────────────────
            if (pinchActive) {
                pinchHeld++;

                // Arm single click
                if (pinchHeld == CLICK_FRAMES)
                    clickPending = true;

                // Arm double click (fires on release, NOT here)
                if (pinchHeld == DOUBLE_CLICK_FRAMES && !dragging) {
                    clickPending   = false;
                    dblClickArmed  = true;
                }

                // Drag — cancels double click, starts drag immediately
                if (pinchHeld == DRAG_FRAMES && !dragging) {
                    dblClickArmed = false;
                    clickPending  = false;
                    dragging      = true;
                    mouse->dragStart(int(smoothX), int(smoothY));
                    std::cout << "Drag start" << std::endl;
                }

                // Progress bar
                if (pinchHeld > CLICK_FRAMES && pinchHeld < DOUBLE_CLICK_FRAMES) {
                    float p = float(pinchHeld - CLICK_FRAMES) / (DOUBLE_CLICK_FRAMES - CLICK_FRAMES);
                    cv::rectangle(frame, cv::Point(g.x-50, g.y-35), cv::Point(g.x-50+int(p*100), g.y-25), cv::Scalar(0,200,255), -1);
                    cv::rectangle(frame, cv::Point(g.x-50, g.y-35), cv::Point(g.x+50, g.y-25), cv::Scalar(255,255,255), 1);
                    cv::putText(frame, "Hold for dbl click", cv::Point(g.x-50, g.y-40), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0,200,255), 1);
                } else if (pinchHeld >= DOUBLE_CLICK_FRAMES && pinchHeld < DRAG_FRAMES) {
                    float p = float(pinchHeld - DOUBLE_CLICK_FRAMES) / (DRAG_FRAMES - DOUBLE_CLICK_FRAMES);
                    cv::rectangle(frame, cv::Point(g.x-50, g.y-35), cv::Point(g.x-50+int(p*100), g.y-25), cv::Scalar(255,165,0), -1);
                    cv::rectangle(frame, cv::Point(g.x-50, g.y-35), cv::Point(g.x+50, g.y-25), cv::Scalar(255,255,255), 1);
                    cv::putText(frame, "Release=dbl  Hold=drag", cv::Point(g.x-50, g.y-40), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,165,0), 1);
                }

            } else {
                // ── Pinch released ────────────────────────────────────────────
                if (dragging) {
                    mouse->dragEnd(int(smoothX), int(smoothY));
                    dragging = false;
                    std::cout << "Drag end" << std::endl;
                } else if (dblClickArmed) {
                    // Fire double click on release
                    mouse->doubleClick();
                    std::cout << "Double click!" << std::endl;
                } else if (clickPending) {
                    mouse->click();
                    std::cout << "Click" << std::endl;
                }
                clickPending  = false;
                dblClickArmed = false;
                pinchHeld     = 0;
            }

            if (g.rightClick)            mouse->rightClick();
            if (g.scrollDelta != 0.f)    mouse->scroll(g.scrollDelta);
            if (g.threeFingerSwipe)      mouse->swipe(g.swipeDeltaX > 0);
            if (g.pinchZoomDelta != 0.f) mouse->zoom(g.pinchZoomDelta);

            cv::Scalar dotColor = dragging       ? cv::Scalar(255,165,0)
                                : dblClickArmed  ? cv::Scalar(0,200,255)
                                : pinchActive    ? cv::Scalar(0,100,255)
                                :                  cv::Scalar(0,255,0);
            cv::circle(frame, cv::Point(g.x, g.y), 14, dotColor, -1, cv::LINE_AA);
            cv::circle(frame, cv::Point(g.x, g.y), 18, cv::Scalar(255,255,255), 2, cv::LINE_AA);

            std::string status = dragging      ? "DRAGGING"
                               : dblClickArmed ? "ARMED: release=dbl, hold=drag"
                               : pinchActive   ? "PINCH"
                               : g.threeFingerSwipe ? "SWIPE"
                               :                 "Move";
            cv::putText(frame, status, cv::Point(20, 50),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, dotColor, 2, cv::LINE_AA);

        } else if (!g.detected) {
            detectionGrace++;
            if (detectionGrace >= 45) {
                if (dragging) {
                    mouse->dragEnd(int(smoothX), int(smoothY));
                    dragging = false; pinchHeld = 0;
                }
                clickPending  = false;
                dblClickArmed = false;
                pinchActive   = false;
                pinchHeld     = 0;
                cv::putText(frame, "No hand", cv::Point(20, 50),
                            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0,165,255), 2);
            } else {
                // Keep cursor alive at last position during brief loss
                mouse->move(int(smoothX), int(smoothY));
                cv::putText(frame, "Reconnecting...", cv::Point(20, 50),
                            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0,165,255), 2);
            }
        }

        cv::imshow("GCVM (q to quit)", frame);
        if (cv::waitKey(1) == 'q') break;
    }

    cap.release();
    cv::destroyAllWindows();
    delete mouse;
    return 0;
}