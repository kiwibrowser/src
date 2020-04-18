package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

package_group(
    name = "feedstore_pkgs",
    packages = [
        "//src/main/java/com/google/android/libraries/feed/feedstore/...",
        "//src/test/java/com/google/android/libraries/feed/feedstore/...",
    ],
)

java_library(
    name = "feedstore",
    srcs = glob(["*.java"]),
    deps = [
        "//src/main/java/com/google/android/libraries/feed/api/common",
        "//src/main/java/com/google/android/libraries/feed/api/store",
        "//src/main/java/com/google/android/libraries/feed/common",
        "//src/main/java/com/google/android/libraries/feed/common/concurrent",
        "//src/main/java/com/google/android/libraries/feed/common/functional",
        "//src/main/java/com/google/android/libraries/feed/common/logging",
        "//src/main/java/com/google/android/libraries/feed/common/protoextensions",
        "//src/main/java/com/google/android/libraries/feed/common/time",
        "//src/main/java/com/google/android/libraries/feed/feedstore/internal",
        "//src/main/java/com/google/android/libraries/feed/host/storage",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "//src/main/proto/search/now/proto/wire/feed:feed_java_proto_lite",
        "@com_google_protobuf_javalite//:protobuf_java_lite",
    ],
)
