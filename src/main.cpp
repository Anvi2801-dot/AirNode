#include <iostream>
#include <opencv2/opencv.hpp>
#include "../include/MouseFactory.h"
#include "../include/HandTracker.h"

static const int SCREEN_W = 1512;
static const int SCREEN_H = 982;
static const float SMOOTH = 0.3f;

int main() {
    IMouseController* mouse = MouseFactory::createMouse();
    HandTracker tracker("hand_landmarker.task");

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) { std::cerr << "Camera failed!" << std::endl; return -1; }
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);

    float smoothX = SCREEN_W / 2.0f, smoothY = SCREEN_H / 2.0f;
    cv::Mat frame;

    while (true) {
        cap >> frame;
        if (frame.empty()) continue;
        cv::flip(frame, frame, 1);

        int rawX = 0, rawY = 0;
        if (tracker.getPointerCoordinates(frame, rawX, rawY)) {
            float sx = (float(rawX) / frame.cols) * SCREEN_W;
            float sy = (float(rawY) / frame.rows) * SCREEN_H;
            smoothX = SMOOTH * smoothX + (1.0f - SMOOTH) * sx;
            smoothY = SMOOTH * smoothY + (1.0f - SMOOTH) * sy;
            mouse->move(int(smoothX), int(smoothY));
            cv::circle(frame, cv::Point(rawX, rawY), 10, cv::Scalar(0,255,0), -1);
        }

        cv::imshow("GCVM (q to quit)", frame);
        if (cv::waitKey(1) == 'q') break;
    }

    cap.release();
    cv::destroyAllWindows();
    delete mouse;
    return 0;
}
