licenses(["notice"])  # Apache 2

android_local_test(
    name = "FeedActionParserTest",
    size = "small",
    timeout = "moderate",
    srcs = ["FeedActionParserTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/protocoladapter",
        "//src/main/java/com/google/android/libraries/feed/common",
        "//src/main/java/com/google/android/libraries/feed/feedactionparser",
        "//src/main/java/com/google/android/libraries/feed/host/action",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/action:feed_action_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/action:feed_action_payload_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/action:piet_extensions_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/piet:piet_java_proto_lite",
        "//src/main/proto/search/now/proto/wire/feed:feed_java_proto_lite",
        "@bazel_android//:robolectric",
        "@com_google_code_findbugs_jsr305//jar",
        "@com_google_guava_guava//jar",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)
