package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

android_library(
    name = "fake_feature_driver",
    testonly = True,
    srcs = ["FakeFeatureDriver.java"],
    deps = [
        ":fake_leaf_feature_driver",
        "//src/main/java/com/google/android/libraries/feed/api/modelprovider",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/drivers",
        "//src/main/java/com/google/android/libraries/feed/testing/modelprovider:fake_model_feature",
        "@com_google_code_findbugs_jsr305//jar",
    ],
)

android_library(
    name = "fake_leaf_feature_driver",
    testonly = True,
    srcs = ["FakeLeafFeatureDriver.java"],
    deps = [
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/drivers",
        "//src/main/java/com/google/android/libraries/feed/basicstream/internal/viewholders",
    ],
)
