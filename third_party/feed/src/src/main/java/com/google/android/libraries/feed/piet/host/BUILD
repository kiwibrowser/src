package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

android_library(
    name = "host",
    srcs = glob(["*.java"]),
    deps = [
        "//src/main/java/com/google/android/libraries/feed/common/functional",
        "//src/main/proto/search/now/proto/ui/piet:piet_java_proto_lite",
        "@com_google_code_findbugs_jsr305//jar",
    ],
)
