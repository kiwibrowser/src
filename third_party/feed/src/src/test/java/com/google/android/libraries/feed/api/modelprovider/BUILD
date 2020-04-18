licenses(["notice"])  # Apache 2

android_local_test(
    name = "TokenCompletedTest",
    size = "small",
    timeout = "moderate",
    srcs = ["TokenCompletedTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/common",
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "@bazel_android//:robolectric",
        "@com_google_code_findbugs_jsr305//jar",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)
