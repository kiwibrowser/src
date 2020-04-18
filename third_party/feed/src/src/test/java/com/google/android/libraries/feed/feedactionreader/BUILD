licenses(["notice"])  # Apache 2

android_local_test(
    name = "FeedActionReaderTest",
    size = "small",
    timeout = "moderate",
    srcs = ["FeedActionReaderTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/actionmanager",
        "//src/main/java/com/google/android/libraries/feed/api/common",
        "//src/main/java/com/google/android/libraries/feed/api/common/testing",
        "//src/main/java/com/google/android/libraries/feed/api/protocoladapter",
        "//src/main/java/com/google/android/libraries/feed/api/store",
        "//src/main/java/com/google/android/libraries/feed/common",
        "//src/main/java/com/google/android/libraries/feed/common/testing",
        "//src/main/java/com/google/android/libraries/feed/common/time",
        "//src/main/java/com/google/android/libraries/feed/feedactionreader",
        "//src/main/java/com/google/android/libraries/feed/host/config",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "//src/main/proto/search/now/proto/wire/feed:feed_java_proto_lite",
        "@bazel_android//:robolectric",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)
