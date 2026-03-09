workspace(name = "gcvm_project")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# 1. FAKE REPOS - Neutralizing GPU/Remote checks for Mac
new_local_repository(name = "local_config_cuda", path = ".", build_file_content = "exports_files(['cuda/build_defs.bzl'])")
new_local_repository(name = "local_config_rocm", path = ".", build_file_content = "exports_files(['rocm/build_defs.bzl'])")
new_local_repository(name = "local_config_tensorrt", path = ".", build_file_content = "exports_files(['build_defs.bzl'])")
new_local_repository(name = "local_config_remote_execution", path = ".", build_file_content = "exports_files(['remote_execution.bzl'])")

# 2. CORE DEPENDENCIES
http_archive(
    name = "bazel_skylib",
    sha256 = "cd55a062e763b9349921f0f5db8c3933288dc8ba4f76dd9416aac68acee3cb94",
    urls = ["https://github.com/bazelbuild/bazel-skylib/releases/download/1.5.0/bazel-skylib-1.5.0.tar.gz"],
)

http_archive(
    name = "build_bazel_rules_android",
    urls = ["https://github.com/bazelbuild/rules_android/archive/v0.1.1.tar.gz"],
    strip_prefix = "rules_android-0.1.1",
    sha256 = "6461c1c5744442b394f46645957d6bd3420eb1b421908fe63caa03091b1b3655",
)

http_archive(
    name = "org_tensorflow",
    sha256 = "e58c939079588623e6fa1d054aec2f90f95018266e0a970fd353a5244f5173dc",
    strip_prefix = "tensorflow-2.13.0",
    urls = ["https://github.com/tensorflow/tensorflow/archive/v2.13.0.tar.gz"],
)

http_archive(
    name = "mediapipe",
    urls = ["https://github.com/google/mediapipe/archive/refs/tags/v0.10.14.tar.gz"],
    strip_prefix = "mediapipe-0.10.14",
    sha256 = "9d46fa5363f5c4e11c3d1faec71b0746f15c5aab7b5460d0e5655d7af93c6957",
)

# 3. OpenCV for macOS - point to your Homebrew prefix
new_local_repository(
    name = "macos_opencv",
    build_file = "@mediapipe//third_party:opencv_macos.BUILD",
    path = "/opt/homebrew/opt/opencv",  # Intel Mac: /usr/local/opt/opencv
)

# 4. MEDIAPIPE TRANSITIVE DEPENDENCIES

http_archive(
    name = "com_google_absl",
    sha256 = "338420448b140f0dfd1a1ea3c3ce71b3bc172071f24f4d9a57d59b45037da440",
    strip_prefix = "abseil-cpp-20230125.3",
    urls = ["https://github.com/abseil/abseil-cpp/archive/refs/tags/20230125.3.tar.gz"],
)

http_archive(
    name = "com_google_protobuf",
    sha256 = "e5de8fb9a939c27c8843e1f35fedc30b3c64f22e2b1cc6e7c9826fa7bd73e2cf",
    strip_prefix = "protobuf-3.21.12",
    urls = ["https://github.com/protocolbuffers/protobuf/archive/v3.21.12.tar.gz"],
)

http_archive(
    name = "rules_cc",
    sha256 = "2037875b9a4456dce4a79d112a8ae885bbc4aad968e6587dca6e64f3a0900cdf",
    strip_prefix = "rules_cc-0.0.9",
    urls = ["https://github.com/bazelbuild/rules_cc/archive/refs/tags/0.0.9.tar.gz"],
)

http_archive(
    name = "com_github_google_flatbuffers",
    sha256 = "8aff985da30aaab37edf8e5b02fda33ed4cbdd962699a8e2af98fdef306f4e4d",
    strip_prefix = "flatbuffers-23.5.26",
    urls = ["https://github.com/google/flatbuffers/archive/refs/tags/v23.5.26.tar.gz"],
)

http_archive(
    name = "org_tensorflow_lite",
    sha256 = "e58c939079588623e6fa1d054aec2f90f95018266e0a970fd353a5244f5173dc",
    strip_prefix = "tensorflow-2.13.0",
    urls = ["https://github.com/tensorflow/tensorflow/archive/v2.13.0.tar.gz"],
)