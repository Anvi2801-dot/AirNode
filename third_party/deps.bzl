load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _non_module_deps_impl(ctx):
    # Add MediaPipe (not in BCR) - using v0.10.14
    http_archive(
        name = "mediapipe",
        urls = ["https://github.com/google/mediapipe/archive/refs/tags/v0.10.14.tar.gz"],
        strip_prefix = "mediapipe-0.10.14",
    )

non_module_deps = module_extension(
    implementation = _non_module_deps_impl,
)

