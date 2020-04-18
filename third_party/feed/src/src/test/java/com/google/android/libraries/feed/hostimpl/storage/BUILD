licenses(["notice"])  # Apache 2

android_local_test(
    name = "InMemoryJournalStorageTest",
    size = "small",
    timeout = "moderate",
    srcs = ["InMemoryJournalStorageTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/common",
        "//src/main/java/com/google/android/libraries/feed/hostimpl/storage",
        "//src/main/java/com/google/android/libraries/feed/testing/conformance/storage",
        "@bazel_android//:robolectric",
        "@org_mockito_mockito_all//jar",
    ],
)

android_local_test(
    name = "InMemoryContentStorageTest",
    size = "small",
    timeout = "moderate",
    srcs = ["InMemoryContentStorageTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/common",
        "//src/main/java/com/google/android/libraries/feed/common/testing",
        "//src/main/java/com/google/android/libraries/feed/host/storage",
        "//src/main/java/com/google/android/libraries/feed/hostimpl/storage",
        "//src/main/java/com/google/android/libraries/feed/testing/conformance/storage",
        "@bazel_android//:robolectric",
        "@com_google_code_findbugs_jsr305//jar",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)

android_local_test(
    name = "PersistentContentStorageTest",
    size = "small",
    timeout = "moderate",
    srcs = ["PersistentContentStorageTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/common",
        "//src/main/java/com/google/android/libraries/feed/common",
        "//src/main/java/com/google/android/libraries/feed/common/testing",
        "//src/main/java/com/google/android/libraries/feed/host/storage",
        "//src/main/java/com/google/android/libraries/feed/hostimpl/storage",
        "//src/main/java/com/google/android/libraries/feed/testing/conformance/storage",
        "@bazel_android//:robolectric",
        "@com_google_code_findbugs_jsr305//jar",
        "@com_google_guava_guava//jar",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)

android_local_test(
    name = "PersistentJournalStorageTest",
    size = "small",
    timeout = "moderate",
    srcs = ["PersistentJournalStorageTest.java"],
    manifest = "AndroidManifest.xml",
    deps = [
        "@com_google_protobuf_javalite//:protobuf_java_lite",
        "//src/main/java/com/google/android/libraries/feed/api/common",
        "//src/main/java/com/google/android/libraries/feed/common",
        "//src/main/java/com/google/android/libraries/feed/common/testing",
        "//src/main/java/com/google/android/libraries/feed/host/storage",
        "//src/main/java/com/google/android/libraries/feed/hostimpl/storage",
        "//src/main/java/com/google/android/libraries/feed/testing/conformance/storage",
        "@bazel_android//:robolectric",
        "@com_google_code_findbugs_jsr305//jar",
        "@com_google_guava_guava//jar",
        "@com_google_truth_truth//jar",
        "@org_mockito_mockito_all//jar",
    ],
)
