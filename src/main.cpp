#include <iostream>
#include <opencv2/opencv.hpp>
#include "../include/MouseFactory.h"
#include "../include/HandTracker.h"

static const int   SCREEN_W       = 1512;
static const int   SCREEN_H       = 982;
static const float SMOOTH         = 0.3f;
static const int   CLICK_FRAMES   = 8;
static const int   CLICK_COOLDOWN = 20;

int main() {
    IMouseController* mouse = MouseFactory::createMouse();
    HandTracker tracker("hand_landmarker.task");

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) { std::cerr << "Camera failed!" << std::endl; return -1; }
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);

    float smoothX = SCREEN_W / 2.0f, smoothY = SCREEN_H / 2.0f;
    int pinchHeld = 0, clickCooldown = 0;
    cv::Mat frame;

    while (true) {
        cap >> frame;
        if (frame.empty()) continue;
        cv::flip(frame, frame, 1);

        HandGesture g = tracker.getGesture(frame);

        if (g.detected) {
            float sx = (float(g.x) / frame.cols) * SCREEN_W;
            float sy = (float(g.y) / frame.rows) * SCREEN_H;
            smoothX = SMOOTH * smoothX + (1.0f - SMOOTH) * sx;
            smoothY = SMOOTH * smoothY + (1.0f - SMOOTH) * sy;
            mouse->move(int(smoothX), int(smoothY));

            if (g.pinching) {
                pinchHeld++;
                if (pinchHeld == CLICK_FRAMES && clickCooldown == 0) {
                    mouse->click();
                    clickCooldown = CLICK_COOLDOWN;
                    std::cout << "Click!" << std::endl;
                }
            } else {
                pinchHeld = 0;
            }

            if (g.scrollDelta != 0.0f)
                mouse->scroll(g.scrollDelta);

            cv::Scalar dotColor = g.pinching ? cv::Scalar(0,0,255) : cv::Scalar(0,255,0);
            cv::circle(frame, cv::Point(g.x, g.y), 20, dotColor, -1);
            cv::circle(frame, cv::Point(g.x, g.y), 25, cv::Scalar(255,255,255), 2);
            cv::putText(frame, g.pinching ? "CLICK" : "Move",
                        cv::Point(g.x+30, g.y), cv::FONT_HERSHEY_SIMPLEX, 1.0, dotColor, 2);
        } else {
            cv::putText(frame, "No hand", cv::Point(30,50),
                        cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0,165,255), 2);
        }

        if (clickCooldown > 0) clickCooldown--;

        cv::imshow("GCVM (q to quit)", frame);
        if (cv::waitKey(1) == 'q') break;
    }

    cap.release();
    cv::destroyAllWindows();
    delete mouse;
    return 0;
}
