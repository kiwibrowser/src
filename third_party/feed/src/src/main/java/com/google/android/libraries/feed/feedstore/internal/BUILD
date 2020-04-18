package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

java_library(
    name = "internal",
    srcs = glob(["*.java"]),
    deps = [
        "//src/main/java/com/google/android/libraries/feed/api/common",
        "//src/main/java/com/google/android/libraries/feed/api/store",
        "//src/main/java/com/google/android/libraries/feed/common/concurrent",
        "//src/main/java/com/google/android/libraries/feed/common/functional",
        "//src/main/java/com/google/android/libraries/feed/common/time",
        "//src/main/java/com/google/android/libraries/feed/host/storage",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "@com_google_code_findbugs_jsr305//jar",
        "@com_google_protobuf_javalite//:protobuf_java_lite",
    ],
)
