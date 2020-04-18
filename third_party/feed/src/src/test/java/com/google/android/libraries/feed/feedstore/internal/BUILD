licenses(["notice"])  # Apache 2

android_local_test(
    name = "ActionGcTest",
    size = "small",
    timeout = "moderate",
    srcs = ["ActionGcTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/store",
        "//src/main/java/com/google/android/libraries/feed/common/concurrent",
        "//src/main/java/com/google/android/libraries/feed/common/functional",
        "//src/main/java/com/google/android/libraries/feed/common/time",
        "//src/main/java/com/google/android/libraries/feed/feedstore/internal",
        "//src/main/java/com/google/android/libraries/feed/host/storage",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "@bazel_android//:robolectric",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)
