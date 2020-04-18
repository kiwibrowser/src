android_sdk_repository(
    name = "androidsdk"
)

http_archive(
    name = "com_google_protobuf",
    sha256 = "cef7f1b5a7c5fba672bec2a319246e8feba471f04dcebfe362d55930ee7c1c30",
    strip_prefix = "protobuf-3.5.0",
    urls = ["https://github.com/google/protobuf/archive/v3.5.0.zip"],
)

http_archive(
    name = "com_google_protobuf_javalite",
    sha256 = "d8a2fed3708781196f92e1e7e7e713cf66804bd2944894401057214aff4f468e",
    strip_prefix = "protobuf-5e8916e881c573c5d83980197a6f783c132d4276",
    urls = ["https://github.com/google/protobuf/archive/5e8916e881c573c5d83980197a6f783c132d4276.zip"],
)

http_archive(
    name = "bazel_android",
    strip_prefix = "bazel_android-a4f491fdaea9efdbfc32f867dc7f5ff1ba548ceb",
    urls = ["https://github.com/dkelmer/bazel_android/archive/a4f491fdaea9efdbfc32f867dc7f5ff1ba548ceb.zip"],
)
load("@bazel_android//:setup_robolectric.bzl", "setup_robolectric")
setup_robolectric()

maven_jar(
    name = "com_google_code_findbugs_jsr305",
    artifact = "com.google.code.findbugs:jsr305:3.0.2",
)

maven_jar(
    name = "com_google_truth_truth",
    artifact = "com.google.truth:truth:0.39",
)

maven_jar(
    name = "com_google_truth_liteproto_extension",
    artifact = "com.google.truth.extensions:truth-liteproto-extension:0.39"
)

maven_jar(
    name = "org_mockito_mockito_all",
    artifact = "org.mockito:mockito-all:1.9.5",
)
