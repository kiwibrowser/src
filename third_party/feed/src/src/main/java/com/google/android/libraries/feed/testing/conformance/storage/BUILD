package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

java_library(
    name = "storage",
    testonly = 1,
    srcs = glob(["*.java"]),
    deps = [
        "//src/main/java/com/google/android/libraries/feed/common",
        "//src/main/java/com/google/android/libraries/feed/common/functional",
        "//src/main/java/com/google/android/libraries/feed/common/testing",
        "//src/main/java/com/google/android/libraries/feed/host/storage",
        "@com_google_truth_truth//jar",
        "@junit_junit//jar",
    ],
)
