#pragma once
#include <opencv2/opencv.hpp>
#include <string>

struct HandGesture {
    bool  detected;
    int   x, y;
    bool  pinching;
    float scrollDelta;
};

class HandTracker {
public:
    HandTracker(const std::string& model_path);
    ~HandTracker();
    bool getPointerCoordinates(const cv::Mat& frame, int& x, int& y);
    HandGesture getGesture(const cv::Mat& frame);
private:
    void* landmarker_;
};
