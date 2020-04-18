package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

android_library(
    name = "protoextensions",
    srcs = glob(["*.java"]),
    deps = [
        "//src/main/java/com/google/android/libraries/feed/host/proto",
        "//src/main/proto/search/now/proto/ui/action:feed_action_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/action:piet_extensions_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/stream:stream_java_proto_lite",
        "//src/main/proto/search/now/proto/wire/feed:feed_java_proto_lite",
        "@com_google_protobuf_javalite//:protobuf_java_lite",
    ],
)
