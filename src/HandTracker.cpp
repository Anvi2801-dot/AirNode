#include "HandTracker.h"
#include "../include/mediapipe_c.h"
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <cstring>
#include <cmath>

// Landmark indices
static const int WRIST       = 0;
static const int THUMB_TIP   = 4;
static const int INDEX_MCP   = 5;
static const int INDEX_TIP   = 8;
static const int MIDDLE_MCP  = 9;
static const int MIDDLE_TIP  = 12;
static const int RING_TIP    = 16;
static const int PINKY_TIP   = 20;
static const int THUMB_MCP   = 2;

// Connections for skeleton drawing (pairs of landmark indices)
static const int CONNECTIONS[][2] = {
    {0,1},{1,2},{2,3},{3,4},       // thumb
    {0,5},{5,6},{6,7},{7,8},       // index
    {0,9},{9,10},{10,11},{11,12},  // middle
    {0,13},{13,14},{14,15},{15,16},// ring
    {0,17},{17,18},{18,19},{19,20},// pinky
    {5,9},{9,13},{13,17}           // palm
};
static const int NUM_CONNECTIONS = 21;

// Thresholds
static const float PINCH_THRESHOLD   = 0.05f;
static const float RCLICK_THRESHOLD  = 0.07f;
static const float SCROLL_THRESHOLD  = 0.05f;
static const float FIST_THRESHOLD    = 0.12f; // fingertip to palm dist
static const float SWIPE_SPEED       = 0.018f; // normalized px/frame
static const int   SWIPE_MIN_FRAMES  = 5;

static float dist(const MpNormalizedLandmark& a, const MpNormalizedLandmark& b) {
    float dx = a.x-b.x, dy = a.y-b.y;
    return std::sqrt(dx*dx+dy*dy);
}

HandTracker::HandTracker(const std::string& model_path) : landmarker_(nullptr) {
    MpHandLandmarkerOptions opts;
    std::memset(&opts, 0, sizeof(opts));
    opts.base_options.model_asset_path         = model_path.c_str();
    opts.base_options.model_asset_buffer       = nullptr;
    opts.base_options.model_asset_buffer_count = 0;
    opts.running_mode                          = 1;
    opts.num_hands                             = 2;
    opts.min_hand_detection_confidence         = 0.1f;
    opts.min_hand_presence_confidence          = 0.1f;
    opts.min_tracking_confidence               = 0.1f;
    opts.result_callback                       = nullptr;

    char* err = nullptr;
    int status = MpHandLandmarkerCreate(&opts, &landmarker_, &err);
    if (status != 0 || !landmarker_) {
        std::cerr << "HandLandmarker create failed: " << (err?err:"unknown") << std::endl;
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
        rgb.cols*rgb.rows*3, &mp_image, &err);
    if (status != 0 || !mp_image) { if (err) MpErrorFree(err); return g; }

    MpHandLandmarkerResult result;
    std::memset(&result, 0, sizeof(result));
    err = nullptr;
    status = MpHandLandmarkerDetectImage(landmarker_, mp_image, nullptr, &result, &err);
    MpImageFree(mp_image);

    if (status != 0 || result.hand_landmarks_count == 0) {
        if (err) MpErrorFree(err);
        MpHandLandmarkerCloseResult(&result);
        cache_.valid = false;
        prevWristX_ = -1.f;
        return g;
    }

    const MpNormalizedLandmarks& hand0 = result.hand_landmarks[0];

    // Cache landmarks for skeleton drawing
    cache_.valid     = true;
    cache_.handCount = result.hand_landmarks_count;
    for (int i = 0; i < 21; i++) {
        cache_.x[i] = hand0.landmarks[i].x * frame.cols;
        cache_.y[i] = hand0.landmarks[i].y * frame.rows;
    }
    if (result.hand_landmarks_count >= 2) {
        const MpNormalizedLandmarks& hand1 = result.hand_landmarks[1];
        for (int i = 0; i < 21; i++) {
            cache_.hand1x[i] = hand1.landmarks[i].x * frame.cols;
            cache_.hand1y[i] = hand1.landmarks[i].y * frame.rows;
        }
    }

    // ── Pointer ──────────────────────────────────────────────────────────────
    g.detected = true;
    g.x = static_cast<int>(hand0.landmarks[INDEX_TIP].x * frame.cols);
    g.y = static_cast<int>(hand0.landmarks[INDEX_TIP].y * frame.rows);

    // ── Pinch (left click) ────────────────────────────────────────────────────
    g.pinching = (dist(hand0.landmarks[THUMB_TIP], hand0.landmarks[INDEX_TIP]) < PINCH_THRESHOLD);

    // ── Fist detection ────────────────────────────────────────────────────────
    // All 4 fingertips close to their MCP (knuckle)
    float idxDist  = dist(hand0.landmarks[INDEX_TIP],  hand0.landmarks[INDEX_MCP]);
    float midDist  = dist(hand0.landmarks[MIDDLE_TIP], hand0.landmarks[MIDDLE_MCP]);
    float rngDist  = dist(hand0.landmarks[RING_TIP],   hand0.landmarks[MIDDLE_MCP]);
    float pnkDist  = dist(hand0.landmarks[PINKY_TIP],  hand0.landmarks[MIDDLE_MCP]);
    g.fist = (idxDist < FIST_THRESHOLD && midDist < FIST_THRESHOLD &&
              rngDist < FIST_THRESHOLD && pnkDist < FIST_THRESHOLD);

    // ── Three-finger swipe ────────────────────────────────────────────────────
    // Index, middle, ring extended (tips above MCPs in Y), thumb and pinky relaxed
    bool indexUp  = hand0.landmarks[INDEX_TIP].y  < hand0.landmarks[INDEX_MCP].y;
    bool middleUp = hand0.landmarks[MIDDLE_TIP].y < hand0.landmarks[MIDDLE_MCP].y;
    bool ringUp   = hand0.landmarks[RING_TIP].y   < hand0.landmarks[MIDDLE_MCP].y;
    bool pinkyDown= hand0.landmarks[PINKY_TIP].y  > hand0.landmarks[MIDDLE_MCP].y;
    bool threeUp  = indexUp && middleUp && ringUp && pinkyDown;

    float wristX = hand0.landmarks[WRIST].x;
    if (threeUp && prevWristX_ >= 0.f) {
        float delta = wristX - prevWristX_;
        if (std::abs(delta) > SWIPE_SPEED) {
            swipeFrames_++;
            if (swipeFrames_ >= SWIPE_MIN_FRAMES) {
                g.threeFingerSwipe = true;
                g.swipeDeltaX      = delta;
                swipeFrames_       = 0;
            }
        } else {
            swipeFrames_ = 0;
        }
    } else {
        swipeFrames_ = 0;
    }
    prevWristX_ = threeUp ? wristX : -1.f;

    // ── Two-hand gestures ─────────────────────────────────────────────────────
    if (result.hand_landmarks_count >= 2) {
        const MpNormalizedLandmarks& hand1 = result.hand_landmarks[1];

        // Right click: thumb + middle pinch on second hand
        g.rightClick = (dist(hand1.landmarks[THUMB_TIP], hand1.landmarks[MIDDLE_TIP]) < RCLICK_THRESHOLD);

        // Scroll: vertical wrist delta
        float y0 = hand0.landmarks[WRIST].y;
        float y1 = hand1.landmarks[WRIST].y;
        float dy = y1 - y0;
        if (std::abs(dy) > SCROLL_THRESHOLD)
            g.scrollDelta = -dy * 10.0f;

        // Pinch zoom: distance between index tips of both hands
        static float prevPinchDist = -1.f;
        float pinchDist = dist(hand0.landmarks[INDEX_TIP], hand1.landmarks[INDEX_TIP]);
        if (prevPinchDist >= 0.f) {
            float delta = pinchDist - prevPinchDist;
            if (std::abs(delta) > 0.02f)
                g.pinchZoomDelta = delta;
        }
        prevPinchDist = pinchDist;
    }

    MpHandLandmarkerCloseResult(&result);
    return g;
}

void HandTracker::drawSkeleton(cv::Mat& frame, const HandGesture& g) {
    if (!cache_.valid) return;

    auto drawHand = [&](float* xs, float* ys, cv::Scalar boneColor) {
        // Draw bones
        for (int i = 0; i < NUM_CONNECTIONS; i++) {
            int a = CONNECTIONS[i][0], b = CONNECTIONS[i][1];
            cv::line(frame,
                cv::Point(int(xs[a]), int(ys[a])),
                cv::Point(int(xs[b]), int(ys[b])),
                boneColor, 2, cv::LINE_AA);
        }
        // Draw joints
        for (int i = 0; i < 21; i++) {
            cv::Scalar jointColor = (i == INDEX_TIP) ? cv::Scalar(0,255,0)
                                  : (i == THUMB_TIP)  ? cv::Scalar(0,165,255)
                                  :                     cv::Scalar(255,255,255);
            cv::circle(frame, cv::Point(int(xs[i]), int(ys[i])), 5, jointColor, -1, cv::LINE_AA);
        }
    };

    cv::Scalar hand0Color = g.fist ? cv::Scalar(0,0,255) : cv::Scalar(200,200,200);
    drawHand(cache_.x, cache_.y, hand0Color);

    if (cache_.handCount >= 2)
        drawHand(cache_.hand1x, cache_.hand1y, cv::Scalar(180,180,255));
}

bool HandTracker::getPointerCoordinates(const cv::Mat& frame, int& x, int& y) {
    HandGesture g = getGesture(frame);
    if (!g.detected) return false;
    x = g.x; y = g.y;
    return true;
}