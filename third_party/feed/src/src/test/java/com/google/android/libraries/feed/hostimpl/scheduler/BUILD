licenses(["notice"])  # Apache 2

android_local_test(
    name = "FeedSchedulerApiImplTest",
    size = "small",
    timeout = "moderate",
    srcs = ["FeedSchedulerApiImplTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/common",
        "//src/main/java/com/google/android/libraries/feed/hostimpl/scheduler",
        "//src/main/java/com/google/android/libraries/feed/testing/conformance/scheduler",
        "@bazel_android//:robolectric",
        "@org_mockito_mockito_all//jar",
    ],
)
