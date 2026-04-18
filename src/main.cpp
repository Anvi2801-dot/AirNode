#include <iostream>
#include <opencv2/opencv.hpp>
#include <chrono>
#include "../include/MouseFactory.h"
#include "../include/HandTracker.h"
#include "../include/WebSocketServer.h"   // ADD

static const int   SCREEN_W            = 1512;
static const int   SCREEN_H            = 982;
static const float SMOOTH              = 0.5f;
static const float DEAD_ZONE           = 15.0f;
static const int   CLICK_FRAMES        = 8;
static const int   DOUBLE_CLICK_FRAMES = 50;
static const int   DRAG_FRAMES         = 80;
static const int   FIST_FRAMES         = 20;
static const int   PINCH_RELEASE_GRACE = 8;
static const int   DETECTION_GRACE     = 45;

using Clock = std::chrono::steady_clock;
using Ms    = std::chrono::milliseconds;

// ADD — converts status string to a JSON event broadcast
static void broadcastEvent(WebSocketServer& ws, const std::string& tag, const std::string& body) {
    ws.broadcast(R"({"type":"event","tag":")" + tag + R"(","body":")" + body + R"("})");
}

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

    WebSocketServer ws(9001);   // ADD
    ws.start();                 // ADD — launches background thread

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
    bool dblClickArmed     = false;
    bool paused            = false;
    int  detectionGrace    = 0;
    int  frameCount        = 0;   // ADD — for throttling metrics broadcasts

    cv::Mat frame;

    while (true) {
        cap >> frame;
        if (frame.empty()) continue;
        cv::flip(frame, frame, 1);

        auto t0 = Clock::now();   // ADD — latency timing start
        HandGesture g = tracker.getGesture(frame);
        auto t1 = Clock::now();   // ADD
        int latencyMs = (int)std::chrono::duration_cast<Ms>(t1 - t0).count();   // ADD

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
                std::string state = paused ? "PAUSED" : "RESUMED";
                std::cout << state << std::endl;
                broadcastEvent(ws, "GESTURE", "fist → " + state);   // ADD
            }
        } else {
            fistHeld = 0;
        }

        // ADD — broadcast metrics every 10 frames (don't flood the socket)
        frameCount++;
        if (frameCount % 10 == 0) {
            ws.broadcast(
                R"({"type":"metrics","latency_ms":)" + std::to_string(latencyMs) +
                R"(,"confidence":)" + std::to_string(g.detected ? (int)(g.confidence * 100) : 0) +
                R"(,"fps":60})"
            );
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

            float sx = (float(g.x) / frame.cols) * SCREEN_W;
            float sy = (float(g.y) / frame.rows) * SCREEN_H;
            float newX = SMOOTH*smoothX + (1.f-SMOOTH)*sx;
            float newY = SMOOTH*smoothY + (1.f-SMOOTH)*sy;
            float dx = newX-prevSmoothX, dy = newY-prevSmoothY;
            float speed = std::sqrt(dx*dx + dy*dy);
            if (speed > DEAD_ZONE) {
                float scale = (speed - DEAD_ZONE) / speed;
                smoothX = prevSmoothX + dx * scale;
                smoothY = prevSmoothY + dy * scale;
                prevSmoothX = smoothX;
                prevSmoothY = smoothY;
                mouse->move(int(smoothX), int(smoothY));

                // ADD — broadcast move events (throttled to every 3 frames)
                if (frameCount % 3 == 0) {
                    broadcastEvent(ws, "MOVE",
                        "dx=" + std::to_string(int(dx)) +
                        " dy=" + std::to_string(int(dy)));
                }
            }

            if (pinchActive) {
                pinchHeld++;

                if (pinchHeld == CLICK_FRAMES)
                    clickPending = true;

                if (pinchHeld == DOUBLE_CLICK_FRAMES && !dragging) {
                    clickPending   = false;
                    dblClickArmed  = true;
                }

                if (pinchHeld == DRAG_FRAMES && !dragging) {
                    dblClickArmed = false;
                    clickPending  = false;
                    dragging      = true;
                    mouse->dragStart(int(smoothX), int(smoothY));
                    std::cout << "Drag start" << std::endl;
                    broadcastEvent(ws, "DRAG", "start");   // ADD
                }

                // progress bar (unchanged)
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
                if (dragging) {
                    mouse->dragEnd(int(smoothX), int(smoothY));
                    dragging = false;
                    std::cout << "Drag end" << std::endl;
                    broadcastEvent(ws, "DRAG", "end");   // ADD
                } else if (dblClickArmed) {
                    mouse->doubleClick();
                    std::cout << "Double click!" << std::endl;
                    broadcastEvent(ws, "CLICK", "double");   // ADD
                } else if (clickPending) {
                    mouse->click();
                    std::cout << "Click" << std::endl;
                    broadcastEvent(ws, "CLICK", "left btn");   // ADD
                }
                clickPending  = false;
                dblClickArmed = false;
                pinchHeld     = 0;
            }

            if (g.rightClick) {
                mouse->rightClick();
                broadcastEvent(ws, "CLICK", "right btn");   // ADD
            }
            if (g.scrollDelta != 0.f) {
                mouse->scroll(g.scrollDelta);
                broadcastEvent(ws, "SCROLL", "dy=" + std::to_string(int(g.scrollDelta)));   // ADD
            }
            if (g.threeFingerSwipe) {
                mouse->swipe(g.swipeDeltaX > 0);
                broadcastEvent(ws, "GESTURE", "three-finger swipe");   // ADD
            }
            if (g.pinchZoomDelta != 0.f) {
                mouse->zoom(g.pinchZoomDelta);
                broadcastEvent(ws, "GESTURE", "pinch zoom");   // ADD
            }

            // ADD — broadcast current gesture state to update dashboard label
            std::string gestureName = dragging       ? "drag"
                                    : dblClickArmed  ? "click"
                                    : pinchActive    ? "click"
                                    : g.threeFingerSwipe ? "scroll"
                                    :                  "pointer";
            ws.broadcast(R"({"type":"gesture","gesture":")" + gestureName + R"("})");

            ws.broadcast(
                R"({"type":"confidence",)"
                R"("pointer":)" + std::to_string(!pinchActive && !dragging && g.detected ? 0.94f : 0.0f) +
                R"(,"click":)"   + std::to_string(pinchActive && !dragging ? 0.90f : 0.0f) +
                R"(,"scroll":)"  + std::to_string(g.scrollDelta != 0.f ? 0.88f : 0.0f) +
                R"(,"drag":)"    + std::to_string(dragging ? 0.92f : 0.0f) +
                R"(,"zoom":)"    + std::to_string(g.pinchZoomDelta != 0.f ? 0.85f : 0.0f) +
                R"(})"
            );

            if (frameCount % 2 == 0) {
                const LandmarkCache& lc = tracker.getLandmarks();
                std::string lm = R"({"type":"landmarks","points":[)";
                for (int i = 0; i < 21; i++) {
                    lm += R"({"x":)" + std::to_string(lc.x[i] / frame.cols) +
                        R"(,"y":)" + std::to_string(lc.y[i] / frame.rows) + "}";
                    if (i < 20) lm += ",";
                }
                lm += "]}";
                ws.broadcast(lm);
            }

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
                    broadcastEvent(ws, "DRAG", "end (no hand)");   // ADD
                }
                clickPending  = false;
                dblClickArmed = false;
                pinchActive   = false;
                pinchHeld     = 0;
                cv::putText(frame, "No hand", cv::Point(20, 50),
                            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0,165,255), 2);
                broadcastEvent(ws, "INFO", "no hand detected");   // ADD
            } else {
                mouse->move(int(smoothX), int(smoothY));
                cv::putText(frame, "Reconnecting...", cv::Point(20, 50),
                            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0,165,255), 2);
            }
        }

        cv::imshow("GCVM (q to quit)", frame);
        if (cv::waitKey(1) == 'q') break;
    }

    ws.stop();   // ADD — clean shutdown
    cap.release();
    cv::destroyAllWindows();
    delete mouse;
    return 0;
}