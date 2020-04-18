package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

java_library(
    name = "store",
    srcs = glob(["*.java"]),
    deps = [
        "//src/main/java/com/google/android/libraries/feed/api/common",
        "//src/main/java/com/google/android/libraries/feed/common",
        "//src/main/java/com/google/android/libraries/feed/common/functional",
        "//src/main/java/com/google/android/libraries/feed/host/storage",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "@androidsdk//com.android.support:support-annotations-25.3.1",
        "@com_google_protobuf_javalite//:protobuf_java_lite",
    ],
)
