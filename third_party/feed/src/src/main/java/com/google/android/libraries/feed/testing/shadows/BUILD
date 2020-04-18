package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

package_group(
    name = "test_framework",
    packages = [
        "//src/test/java/com/google/android/libraries/feed/...",
    ],
)

android_library(
    name = "shadows",
    testonly = True,
    srcs = glob(["*.java"]),
    deps = [
        "@androidsdk//com.android.support:recyclerview-v7-25.3.1",
        "@bazel_android//:robolectric",
    ],
)
