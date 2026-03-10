#include "HandTracker.h"
#include "../include/mediapipe_c.h"
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <cstring>
#include <cmath>

static const int   IDX_THUMB_TIP  = 4;
static const int   IDX_INDEX_TIP  = 8;
static const int   IDX_MIDDLE_TIP = 12;
static const int   IDX_WRIST      = 0;
static const float PINCH_THRESHOLD       = 0.07f;
static const float RIGHT_CLICK_THRESHOLD = 0.07f;
static const float SCROLL_THRESHOLD      = 0.05f;

HandTracker::HandTracker(const std::string& model_path) : landmarker_(nullptr) {
    MpHandLandmarkerOptions opts;
    std::memset(&opts, 0, sizeof(opts));
    opts.base_options.model_asset_path         = model_path.c_str();
    opts.base_options.model_asset_buffer       = nullptr;
    opts.base_options.model_asset_buffer_count = 0;
    opts.running_mode                          = 1;
    opts.num_hands                             = 2;
    opts.min_hand_detection_confidence         = 0.3f;
    opts.min_hand_presence_confidence          = 0.3f;
    opts.min_tracking_confidence               = 0.3f;
    opts.result_callback                       = nullptr;

    char* err = nullptr;
    int status = MpHandLandmarkerCreate(&opts, &landmarker_, &err);
    if (status != 0 || !landmarker_) {
        std::cerr << "HandLandmarker create failed: " << (err ? err : "unknown") << std::endl;
        if (err) MpErrorFree(err);
    } else {
        std::cout << "HandLandmarker created OK." << std::endl;
    }
}

HandTracker::~HandTracker() {
    if (landmarker_) {
        char* err = nullptr;
        MpHandLandmarkerClose(landmarker_, &err);
        if (err) MpErrorFree(err);
    }
}

static float dist(const MpNormalizedLandmark& a, const MpNormalizedLandmark& b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx*dx + dy*dy);
}

HandGesture HandTracker::getGesture(const cv::Mat& frame) {
    HandGesture g{};
    if (!landmarker_) return g;

    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    if (!rgb.isContinuous()) rgb = rgb.clone();

    void* mp_image = nullptr;
    char* err = nullptr;
    int status = MpImageCreateFromUint8Data(
        MP_IMAGE_FORMAT_SRGB, rgb.cols, rgb.rows, rgb.data,
        rgb.cols * rgb.rows * 3, &mp_image, &err);
    if (status != 0 || !mp_image) { if (err) MpErrorFree(err); return g; }

    MpHandLandmarkerResult result;
    std::memset(&result, 0, sizeof(result));
    err = nullptr;
    status = MpHandLandmarkerDetectImage(landmarker_, mp_image, nullptr, &result, &err);
    MpImageFree(mp_image);

    if (status != 0 || result.hand_landmarks_count == 0) {
        if (err) MpErrorFree(err);
        MpHandLandmarkerCloseResult(&result);
        return g;
    }

    // Primary hand: pointer + left pinch
    const MpNormalizedLandmarks& hand0 = result.hand_landmarks[0];
    const MpNormalizedLandmark& indexTip  = hand0.landmarks[IDX_INDEX_TIP];
    const MpNormalizedLandmark& thumbTip  = hand0.landmarks[IDX_THUMB_TIP];

    g.detected  = true;
    g.x         = static_cast<int>(indexTip.x * frame.cols);
    g.y         = static_cast<int>(indexTip.y * frame.rows);
    g.pinching  = (dist(thumbTip, indexTip) < PINCH_THRESHOLD);

    if (result.hand_landmarks_count >= 2) {
        const MpNormalizedLandmarks& hand1 = result.hand_landmarks[1];

        // Right click: thumb + middle pinch on second hand
        const MpNormalizedLandmark& t1 = hand1.landmarks[IDX_THUMB_TIP];
        const MpNormalizedLandmark& m1 = hand1.landmarks[IDX_MIDDLE_TIP];
        g.rightClick = (dist(t1, m1) < RIGHT_CLICK_THRESHOLD);

        // Scroll: vertical wrist delta between hands
        float y0 = hand0.landmarks[IDX_WRIST].y;
        float y1 = hand1.landmarks[IDX_WRIST].y;
        float delta = y1 - y0;
        if (std::abs(delta) > SCROLL_THRESHOLD)
            g.scrollDelta = -delta * 10.0f;
    }

    MpHandLandmarkerCloseResult(&result);
    return g;
}

bool HandTracker::getPointerCoordinates(const cv::Mat& frame, int& x, int& y) {
    HandGesture g = getGesture(frame);
    if (!g.detected) return false;
    x = g.x; y = g.y;
    return true;
}