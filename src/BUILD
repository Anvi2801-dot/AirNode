load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "gcvm_app",
    srcs = [
        "main.cpp",
        "HandTracker.cpp",
        "MacMouse.cpp",
        "HandTracker.h",
    ],
    deps = [
        "//include:headers",
        "@mediapipe//mediapipe/tasks/cc/vision/hand_landmarker:hand_landmarker",  # cc not cpp
        "@mediapipe//mediapipe/framework/port:opencv_core",
        "@mediapipe//mediapipe/framework/port:opencv_highgui",
        "@mediapipe//mediapipe/framework/port:opencv_videoio",
    ],
    linkopts = ["-framework ApplicationServices"],
)