package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

package_group(
    name = "feedsessionmanager_pkgs",
    packages = [
        "//src/main/java/com/google/android/libraries/feed/feedsessionmanager/...",
        "//src/test/java/com/google/android/libraries/feed/feedsessionmanager/...",
    ],
)

android_library(
    name = "feedsessionmanager",
    srcs = glob(["*.java"]),
    deps = [
        "//src/main/java/com/google/android/libraries/feed/api/common",
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "//src/main/java/com/google/android/libraries/feed/api/protocoladapter",
        "//src/main/java/com/google/android/libraries/feed/api/requestmanager",
        "//src/main/java/com/google/android/libraries/feed/api/sessionmanager",
        "//src/main/java/com/google/android/libraries/feed/api/store",
        "//src/main/java/com/google/android/libraries/feed/common",
        "//src/main/java/com/google/android/libraries/feed/common/functional",
        "//src/main/java/com/google/android/libraries/feed/common/logging",
        "//src/main/java/com/google/android/libraries/feed/common/time",
        "//src/main/java/com/google/android/libraries/feed/feedsessionmanager/internal",
        "//src/main/java/com/google/android/libraries/feed/host/config",
        "//src/main/java/com/google/android/libraries/feed/host/scheduler",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "//src/main/proto/search/now/proto/wire/feed:feed_java_proto_lite",
        "@androidsdk//com.android.support:support-annotations-25.3.1",
        "@com_google_code_findbugs_jsr305//jar",
    ],
)
