#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── Image format ─────────────────────────────────────────────────────────────
typedef enum {
    MP_IMAGE_FORMAT_UNKNOWN = 0,
    MP_IMAGE_FORMAT_SRGB    = 1,
    MP_IMAGE_FORMAT_SRGBA   = 2,
    MP_IMAGE_FORMAT_GRAY8   = 3,
} MpImageFormat;

// ── Image API ────────────────────────────────────────────────────────────────
int  MpImageCreateFromUint8Data(int format, int width, int height,
                                 const uint8_t* data, int size,
                                 void** out_image, char** error_msg);
void MpImageFree(void* image);

// ── BaseOptions ──────────────────────────────────────────────────────────────
typedef struct {
    const char*  model_asset_buffer;
    unsigned int model_asset_buffer_count;
    const char*  model_asset_path;
} MpBaseOptions;

// ── HandLandmarkerOptions ────────────────────────────────────────────────────
typedef struct {
    MpBaseOptions base_options;
    int           running_mode;   // 0=IMAGE, 1=VIDEO, 2=LIVE_STREAM
    int           num_hands;
    float         min_hand_detection_confidence;
    float         min_hand_presence_confidence;
    float         min_tracking_confidence;
    void*         result_callback; // NULL for IMAGE mode
} MpHandLandmarkerOptions;

// ── Result structs ───────────────────────────────────────────────────────────
typedef struct {
    int         index;
    float       score;
    const char* category_name;
    const char* display_name;
} MpCategory;

typedef struct {
    MpCategory* categories;
    uint32_t    categories_count;
} MpCategories;

typedef struct {
    float       x, y, z;
    bool        has_visibility;
    float       visibility;
    bool        has_presence;
    float       presence;
    const char* name;
} MpNormalizedLandmark;

typedef struct {
    MpNormalizedLandmark* landmarks;
    uint32_t              landmarks_count;
} MpNormalizedLandmarks;

typedef struct {
    float       x, y, z;
    bool        has_visibility;
    float       visibility;
    bool        has_presence;
    float       presence;
    const char* name;
} MpLandmark;

typedef struct {
    MpLandmark* landmarks;
    uint32_t    landmarks_count;
} MpLandmarks;

typedef struct {
    MpCategories*          handedness;
    uint32_t               handedness_count;
    MpNormalizedLandmarks* hand_landmarks;
    uint32_t               hand_landmarks_count;
    MpLandmarks*           hand_world_landmarks;
    uint32_t               hand_world_landmarks_count;
} MpHandLandmarkerResult;

// ── HandLandmarker API ───────────────────────────────────────────────────────
// All status-returning functions: 0 = OK. Free errors with MpErrorFree.

int  MpHandLandmarkerCreate(const MpHandLandmarkerOptions* options,
                             void** out_handle,
                             char** error_msg);

int  MpHandLandmarkerDetectImage(void* handle,
                                  void* image,
                                  const void* processing_options, // pass NULL
                                  MpHandLandmarkerResult* out_result,
                                  char** error_msg);

void MpHandLandmarkerCloseResult(MpHandLandmarkerResult* result);

int  MpHandLandmarkerClose(void* handle, char** error_msg);

void MpErrorFree(void* error_msg);

#ifdef __cplusplus
}
#endif