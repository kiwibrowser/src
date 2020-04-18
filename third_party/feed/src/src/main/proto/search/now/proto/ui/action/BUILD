package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

exports_files(["LICENSE"])

proto_library(
    name = "piet_extensions_proto",
    srcs = ["piet_extensions.proto"],
    deps = [
        ":feed_action_payload_proto",
        "//src/main/proto/search/now/proto/ui/piet:piet_proto",
    ],
)

java_proto_library(
    name = "piet_extensions_java_proto",
    deps = [":piet_extensions_proto"],
)

java_lite_proto_library(
    name = "piet_extensions_java_proto_lite",
    deps = [":piet_extensions_proto"],
)

proto_library(
    name = "feed_action_proto",
    srcs = ["feed_action.proto"],
    deps = [
        ":feed_action_payload_proto",
        "//src/main/proto/search/now/proto/wire/feed:feed_proto",
    ],
)

java_proto_library(
    name = "feed_action_java_proto",
    deps = [":feed_action_proto"],
)

java_lite_proto_library(
    name = "feed_action_java_proto_lite",
    deps = [":feed_action_proto"],
)

proto_library(
    name = "feed_action_payload_proto",
    srcs = ["feed_action_payload.proto"],
)

java_proto_library(
    name = "feed_action_payload_java_proto",
    deps = [":feed_action_payload_proto"],
)

java_lite_proto_library(
    name = "feed_action_payload_java_proto_lite",
    deps = [":feed_action_payload_proto"],
)
