#include <iostream>
#include <opencv2/opencv.hpp>
#include <chrono>
#include "../include/MouseFactory.h"
#include "../include/HandTracker.h"

static const int   SCREEN_W        = 1512;
static const int   SCREEN_H        = 982;
static const float SMOOTH          = 0.25f;
static const float DEAD_ZONE       = 8.0f;
static const int   CLICK_FRAMES    = 6;
static const int   DRAG_FRAMES     = 20;
static const int   DOUBLE_CLICK_MS = 4000;

using Clock = std::chrono::steady_clock;
using Ms    = std::chrono::milliseconds;

int main() {
    IMouseController* mouse = MouseFactory::createMouse();
    HandTracker tracker("hand_landmarker.task");

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) { std::cerr << "Camera failed!" << std::endl; return -1; }
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);

    float smoothX = SCREEN_W / 2.0f, smoothY = SCREEN_H / 2.0f;
    float prevSmoothX = smoothX, prevSmoothY = smoothY;

    int  pinchHeld    = 0;
    bool dragging     = false;
    bool clickPending = false;
    int  clickCount   = 0;
    auto lastClickTime = Clock::now() - Ms(2000);

    cv::Mat frame;

    while (true) {
        cap >> frame;
        if (frame.empty()) continue;
        cv::flip(frame, frame, 1);

        HandGesture g = tracker.getGesture(frame);

        if (g.detected) {
            float sx = (float(g.x) / frame.cols) * SCREEN_W;
            float sy = (float(g.y) / frame.rows) * SCREEN_H;
            float newX = SMOOTH * smoothX + (1.0f - SMOOTH) * sx;
            float newY = SMOOTH * smoothY + (1.0f - SMOOTH) * sy;
            float dx = newX - prevSmoothX, dy = newY - prevSmoothY;
            if (std::abs(dx) > DEAD_ZONE || std::abs(dy) > DEAD_ZONE) {
                smoothX = newX; smoothY = newY;
                prevSmoothX = smoothX; prevSmoothY = smoothY;
                mouse->move(int(smoothX), int(smoothY));
            }

            if (g.pinching) {
                pinchHeld++;
                if (pinchHeld == CLICK_FRAMES)
                    clickPending = true;
                if (pinchHeld == DRAG_FRAMES && !dragging) {
                    clickPending = false;
                    dragging = true;
                    mouse->dragStart(int(smoothX), int(smoothY));
                    clickCount = 0;
                    std::cout << "Drag start" << std::endl;
                }
            } else {
                if (dragging) {
                    mouse->dragEnd(int(smoothX), int(smoothY));
                    lastClickTime = Clock::now() - Ms(5000);
                    dragging = false;
                    std::cout << "Drag end" << std::endl;
                } else if (clickPending) {
                    auto now = Clock::now();
                    int ms = std::chrono::duration_cast<Ms>(now - lastClickTime).count();
                    std::cout << "Release | ms since last: " << ms
                              << " | clickCount: " << clickCount << std::endl;

                    if (ms < DOUBLE_CLICK_MS && clickCount == 1) {
                        mouse->doubleClick();
                        clickCount = 0;
                        std::cout << ">>> DOUBLE CLICK" << std::endl;
                    } else {
                        mouse->click();
                        clickCount = 1;
                        std::cout << ">>> Single click" << std::endl;
                    }
                    lastClickTime = now;
                }
                clickPending = false;
                pinchHeld    = 0;
            }

            if (g.rightClick) mouse->rightClick();
            if (g.scrollDelta != 0.0f) mouse->scroll(g.scrollDelta);

            cv::Scalar dotColor = dragging   ? cv::Scalar(255,165,0)
                                : g.pinching ? cv::Scalar(0,0,255)
                                :              cv::Scalar(0,255,0);
            cv::circle(frame, cv::Point(g.x, g.y), 20, dotColor, -1);
            cv::circle(frame, cv::Point(g.x, g.y), 25, cv::Scalar(255,255,255), 2);
            cv::putText(frame, dragging ? "DRAG" : g.pinching ? "PINCH" : "Move",
                        cv::Point(g.x+30, g.y), cv::FONT_HERSHEY_SIMPLEX, 1.0, dotColor, 2);
        } else {
            if (dragging) {
                mouse->dragEnd(int(smoothX), int(smoothY));
                lastClickTime = Clock::now() - Ms(5000);
                dragging = false; pinchHeld = 0;
            }
            clickPending = false;
            cv::putText(frame, "No hand", cv::Point(30,50),
                        cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0,165,255), 2);
        }

        cv::imshow("GCVM (q to quit)", frame);
        if (cv::waitKey(1) == 'q') break;
    }

    cap.release();
    cv::destroyAllWindows();
    delete mouse;
    return 0;
}
