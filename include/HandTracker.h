#pragma once
#include <opencv2/opencv.hpp>
#include <string>

struct HandGesture {
    bool  detected;
    int   x, y;
    bool  pinching;
    bool  rightClick;
    bool  fist;
    bool  threeFingerSwipe;
    float swipeDeltaX;
    float pinchZoomDelta;
    float scrollDelta;
    float confidence;
};

struct LandmarkCache {
    float x[21], y[21];
    bool  valid     = false;
    int   handCount = 0;
    float hand1x[21], hand1y[21];
};

class HandTracker {
public:
    HandTracker(const std::string& model_path);
    ~HandTracker();
    bool                 getPointerCoordinates(const cv::Mat& frame, int& x, int& y);
    HandGesture          getGesture(const cv::Mat& frame);
    void                 drawSkeleton(cv::Mat& frame, const HandGesture& g);
    const LandmarkCache& getLandmarks() const { return cache_; }

private:
    void*         landmarker_;
    LandmarkCache cache_;
    float         prevWristX_  = -1.f;
    int           swipeFrames_ = 0;
};