package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

android_library(
    name = "internal",
    srcs = glob(["*.java"]),
    deps = [
        "//src/main/java/com/google/android/libraries/feed/api/actionmanager",
        "//src/main/java/com/google/android/libraries/feed/api/actionparser",
        "//src/main/java/com/google/android/libraries/feed/api/stream",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/drivers",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/viewholders",
        "//src/main/java/com/google/android/libraries/feed/common/logging",
        "//src/main/java/com/google/android/libraries/feed/host/action",
        "//src/main/java/com/google/android/libraries/feed/host/stream",
        "//src/main/java/com/google/android/libraries/feed/piet",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "//src/main/proto/search/now/proto/ui/action:feed_action_java_proto_lite",
        "@androidsdk//com.android.support:recyclerview-v7-25.3.1",
        "@androidsdk//com.android.support:support-annotations-25.3.1",
        "@com_google_code_findbugs_jsr305//jar",
    ],
)
