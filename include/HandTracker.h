#pragma once
#include <opencv2/opencv.hpp>
#include <string>

struct HandGesture {
    bool  detected;
    int   x, y;
    bool  pinching;       // thumb + index → left click / drag
    bool  rightClick;     // thumb + middle on second hand → right click
    bool  fist;           // all fingers closed → pause tracking
    bool  threeFingerSwipe; // index+middle+ring extended, moving left/right
    float swipeDeltaX;    // normalized swipe velocity
    float pinchZoomDelta; // two-hand pinch distance change → zoom
    float scrollDelta;    // two-hand vertical → scroll
};

class HandTracker {
public:
    HandTracker(const std::string& model_path);
    ~HandTracker();
    bool        getPointerCoordinates(const cv::Mat& frame, int& x, int& y);
    HandGesture getGesture(const cv::Mat& frame);
    void        drawSkeleton(cv::Mat& frame, const HandGesture& g);

private:
    void*  landmarker_;
    // Store last landmarks for skeleton drawing and gesture history
    struct LandmarkCache {
        float x[21], y[21];
        bool  valid = false;
        int   handCount = 0;
        float hand1x[21], hand1y[21];
    } cache_;

    // Swipe tracking
    float prevWristX_   = -1.f;
    int   swipeFrames_  = 0;
};