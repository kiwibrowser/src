package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

proto_library(
    name = "client_feed_proto",
    srcs = glob(["*.proto"]),
    deps = [
        "//src/main/proto/search/now/proto/ui/stream:stream_proto",
        "//src/main/proto/search/now/proto/wire/feed:feed_proto",
    ],
)

java_proto_library(
    name = "client_feed_java_proto",
    deps = [":client_feed_proto"],
)

java_lite_proto_library(
    name = "client_feed_java_proto_lite",
    deps = [":client_feed_proto"],
)
