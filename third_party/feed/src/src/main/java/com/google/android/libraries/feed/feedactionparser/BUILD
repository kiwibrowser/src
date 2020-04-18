package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

package_group(
    name = "feedactionparser_pkgs",
    packages = [
        "//src/main/java/com/google/android/libraries/feed/feedactionparser/...",
        "//src/test/java/com/google/android/libraries/feed/feedactionparser/...",
    ],
)

android_library(
    name = "feedactionparser",
    srcs = glob(["*.java"]),
    deps = [
        "//src/main/java/com/google/android/libraries/feed/api/actionparser",
        "//src/main/java/com/google/android/libraries/feed/api/protocoladapter",
        "//src/main/java/com/google/android/libraries/feed/common",
        "//src/main/java/com/google/android/libraries/feed/common/logging",
        "//src/main/java/com/google/android/libraries/feed/feedactionparser/internal",
        "//src/main/java/com/google/android/libraries/feed/host/action",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/action:feed_action_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/action:feed_action_payload_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/piet:piet_java_proto_lite",
        "@com_google_code_findbugs_jsr305//jar",
    ],
)
