package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

android_library(
    name = "internal",
    srcs = glob(["*.java"]),
    deps = [
        "//src/main/java/com/google/android/libraries/feed/common/logging",
        "//src/main/proto/search/now/proto/ui/action:feed_action_payload_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/action:piet_extensions_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/piet:piet_java_proto_lite",
        "@com_google_code_findbugs_jsr305//jar",
    ],
)
