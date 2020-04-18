package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

java_library(
    name = "testing",
    testonly = 1,
    srcs = glob(["*.java"]),
    deps = [
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "//src/main/proto/search/now/proto/wire/feed:feed_java_proto_lite",
    ],
)
