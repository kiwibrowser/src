package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

java_library(
    name = "fake_model_child",
    testonly = True,
    srcs = ["FakeModelChild.java"],
    deps = [
        ":fake_model_token",
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "//src/main/java/com/google/android/libraries/feed/common",
        "@com_google_code_findbugs_jsr305//jar",
    ],
)

java_library(
    name = "fake_model_cursor",
    testonly = True,
    srcs = ["FakeModelCursor.java"],
    deps = [
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "@com_google_code_findbugs_jsr305//jar",
    ],
)

java_library(
    name = "fake_model_feature",
    testonly = True,
    srcs = ["FakeModelFeature.java"],
    deps = [
        ":fake_model_cursor",
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
        "@com_google_code_findbugs_jsr305//jar",
    ],
)

java_library(
    name = "fake_model_token",
    testonly = True,
    srcs = ["FakeModelToken.java"],
    deps = [
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "//src/main/proto/com/google/android/libraries/feed/api/proto:client_feed_java_proto_lite",
    ],
)
