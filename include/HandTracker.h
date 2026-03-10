#pragma once
#include <opencv2/opencv.hpp>
#include <string>

struct HandGesture {
    bool  detected;
    int   x, y;
    bool  pinching;     // index + thumb pinch → left click / drag
    bool  rightClick;   // middle + thumb pinch on second hand → right click
    float scrollDelta;  // two-hand vertical → scroll
};

class HandTracker {
public:
    HandTracker(const std::string& model_path);
    ~HandTracker();
    bool        getPointerCoordinates(const cv::Mat& frame, int& x, int& y);
    HandGesture getGesture(const cv::Mat& frame);
private:
    void* landmarker_;
};