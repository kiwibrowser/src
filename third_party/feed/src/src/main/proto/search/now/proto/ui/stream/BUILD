package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

proto_library(
    name = "stream_proto",
    srcs = ["stream_structure.proto"],
    deps = [
        "//src/main/proto/search/now/proto/ui/piet:piet_proto",
        "//src/main/proto/search/now/proto/wire/feed:feed_proto",
    ],
)

java_proto_library(
    name = "stream_java_proto",
    deps = [":stream_proto"],
)

java_lite_proto_library(
    name = "stream_java_proto_lite",
    deps = [":stream_proto"],
)

proto_library(
    name = "stream_swipe_extension",
    srcs = ["stream_swipe_extension.proto"],
    deps = [
        ":stream_proto",
        "//src/main/proto/search/now/proto/ui/action:feed_action_payload_proto",
    ],
)

java_proto_library(
    name = "stream_swipe_extension_java_proto",
    deps = [":stream_swipe_extension"],
)

java_lite_proto_library(
    name = "stream_swipe_extension_java_proto_lite",
    deps = [":stream_swipe_extension"],
)
