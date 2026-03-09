#include "HandTracker.h"
#include "../include/mediapipe_c.h"
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <cstring>

HandTracker::HandTracker(const std::string& model_path) : landmarker_(nullptr) {
    MpHandLandmarkerOptions opts;
    std::memset(&opts, 0, sizeof(opts));
    opts.base_options.model_asset_path        = model_path.c_str();
    opts.base_options.model_asset_buffer      = nullptr;
    opts.base_options.model_asset_buffer_count = 0;
    opts.running_mode                         = 1; // IMAGE
    opts.num_hands                            = 1;
    opts.min_hand_detection_confidence        = 0.5f;
    opts.min_hand_presence_confidence         = 0.5f;
    opts.min_tracking_confidence              = 0.5f;
    opts.result_callback                      = nullptr;

    char* err = nullptr;
    int status = MpHandLandmarkerCreate(&opts, &landmarker_, &err);
    if (status != 0 || !landmarker_) {
        std::cerr << "HandLandmarker create failed (status=" << status << "): "
                  << (err ? err : "unknown") << std::endl;
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

bool HandTracker::getPointerCoordinates(const cv::Mat& frame, int& x, int& y) {
    if (!landmarker_) return false;

    // Convert BGR -> SRGB (RGB), ensure contiguous
    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    if (!rgb.isContinuous()) rgb = rgb.clone();

    // Create MediaPipe image from raw pixels
    void* mp_image = nullptr;
    char* err = nullptr;
    int status = MpImageCreateFromUint8Data(
        MP_IMAGE_FORMAT_SRGB,
        rgb.cols, rgb.rows,
        rgb.data,
        rgb.cols * rgb.rows * 3,
        &mp_image, &err);

    if (status != 0 || !mp_image) {
        std::cerr << "MpImageCreate failed: " << (err ? err : "unknown") << std::endl;
        if (err) MpErrorFree(err);
        return false;
    }

    MpHandLandmarkerResult result;
    std::memset(&result, 0, sizeof(result));

    err = nullptr;
    status = MpHandLandmarkerDetectImage(landmarker_, mp_image, nullptr, &result, &err);
    MpImageFree(mp_image);

    if (status != 0) {
        std::cerr << "DetectImage failed: " << (err ? err : "unknown") << std::endl;
        if (err) MpErrorFree(err);
        return false;
    }

    if (result.hand_landmarks_count == 0) {
        MpHandLandmarkerCloseResult(&result);
        return false;
    }

    // Landmark 8 = index finger tip (normalized 0..1)
    const MpNormalizedLandmark& tip = result.hand_landmarks[0].landmarks[8];
    x = static_cast<int>(tip.x * frame.cols);
    y = static_cast<int>(tip.y * frame.rows);

    MpHandLandmarkerCloseResult(&result);
    return true;
}