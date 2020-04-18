licenses(["notice"])  # Apache 2

android_local_test(
    name = "HttpRequestTest",
    size = "small",
    timeout = "moderate",
    srcs = ["HttpRequestTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/common/testing",
        "//src/main/java/com/google/android/libraries/feed/host/network",
        "@bazel_android//:robolectric",
        "@com_google_truth_truth//jar",
    ],
)

android_local_test(
    name = "HttpHeaderTest",
    size = "small",
    timeout = "moderate",
    srcs = ["HttpHeaderTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/common/testing",
        "//src/main/java/com/google/android/libraries/feed/host/network",
        "@bazel_android//:robolectric",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)
