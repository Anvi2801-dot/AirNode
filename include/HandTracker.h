#pragma once
#include <opencv2/opencv.hpp>
#include <string>

class HandTracker {
public:
    HandTracker(const std::string& model_path);
    ~HandTracker();
    bool getPointerCoordinates(const cv::Mat& frame, int& x, int& y);

private:
    void* landmarker_;  // opaque handle to MpHandLandmarker
};