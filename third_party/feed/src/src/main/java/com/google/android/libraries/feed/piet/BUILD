package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

package_group(
    name = "framework",
    packages = ["//src/main/java/com/google/android/libraries/feed/piet/..."],
)

package_group(
    name = "framework_and_users",
    includes = [
        ":framework",
    ],
    packages = ["//..."],
)

android_library(
    name = "piet",
    srcs = glob(["*.java"]),
    manifest = "AndroidManifest.xml",
    resource_files = glob(["res/**/*"]),
    deps = [
        "//src/main/java/com/google/android/libraries/feed/common",
        "//src/main/java/com/google/android/libraries/feed/common/functional",
        "//src/main/java/com/google/android/libraries/feed/common/logging",
        "//src/main/java/com/google/android/libraries/feed/host/config",
        "//src/main/java/com/google/android/libraries/feed/piet/host",
        "//src/main/java/com/google/android/libraries/feed/piet/ui",
        "//src/main/proto/search/now/proto/ui/piet:piet_java_proto_lite",
        "//src/main/proto/search/now/proto/wire/feed:feed_java_proto_lite",
        "@androidsdk//com.android.support:support-annotations-25.3.1",
        "@androidsdk//com.android.support:support-compat-25.3.1",
        "@com_google_code_findbugs_jsr305//jar",
    ],
)
