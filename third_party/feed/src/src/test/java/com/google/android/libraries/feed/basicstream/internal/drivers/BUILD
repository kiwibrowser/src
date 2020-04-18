licenses(["notice"])  # Apache 2

android_local_test(
    name = "CardDriverTest",
    size = "small",
    timeout = "moderate",
    srcs = ["CardDriverTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/drivers",
        "//src/main/java/com/google/android/libraries/feed/testing/modelprovider:fake_model_cursor",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/stream:stream_java_proto_lite",
        "@bazel_android//:robolectric",
        "@com_google_guava_guava//jar",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)

android_local_test(
    name = "ClusterDriverTest",
    size = "small",
    timeout = "moderate",
    srcs = ["ClusterDriverTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/drivers",
        "//src/main/java/com/google/android/libraries/feed/testing/modelprovider:fake_model_cursor",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/stream:stream_java_proto_lite",
        "@bazel_android//:robolectric",
        "@com_google_guava_guava//jar",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)

android_local_test(
    name = "ContentDriverTest",
    size = "small",
    timeout = "moderate",
    srcs = ["ContentDriverTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/drivers",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/viewholders",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/piet:piet_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/stream:stream_java_proto_lite",
        "//src/main/proto/search/now/proto/wire/feed:feed_java_proto_lite",
        "@bazel_android//:robolectric",
        "@com_google_guava_guava//jar",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)

android_local_test(
    name = "ContinuationDriverTest",
    size = "small",
    timeout = "moderate",
    srcs = ["ContinuationDriverTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/drivers",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/viewholders",
        "//src/main/java/com/google/android/libraries/feed/host/config",
        "@bazel_android//:robolectric",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)

android_local_test(
    name = "StreamDriverTest",
    size = "small",
    timeout = "moderate",
    srcs = ["StreamDriverTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/common",
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/drivers",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/drivers/testing:fake_feature_driver",
        "//src/main/java/com/google/android/libraries/feed/host/config",
        "//src/main/java/com/google/android/libraries/feed/testing/modelprovider:fake_model_child",
        "//src/main/java/com/google/android/libraries/feed/testing/modelprovider:fake_model_cursor",
        "//src/main/java/com/google/android/libraries/feed/testing/modelprovider:fake_model_feature",
        "//src/main/java/com/google/android/libraries/feed/testing/modelprovider:fake_model_token",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/stream:stream_java_proto_lite",
        "@bazel_android//:robolectric",
        "@com_google_code_findbugs_jsr305//jar",
        "@com_google_guava_guava//jar",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)
