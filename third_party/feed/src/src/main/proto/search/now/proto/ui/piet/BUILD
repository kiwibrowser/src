package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2

exports_files(["OWNERS"])

PIET_ERRORS_SRCS = ["errors.proto"]

proto_library(
    name = "piet_proto",
    srcs = glob(
        ["*.proto"],
        exclude = PIET_ERRORS_SRCS,
    ),
)

proto_library(
    name = "piet_errors_proto",
    srcs = PIET_ERRORS_SRCS,
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

java_proto_library(
    name = "piet_java_proto",
    deps = [":piet_proto"],
)

java_proto_library(
    name = "piet_errors_java_proto",
    deps = [":piet_errors_proto"],
)

java_lite_proto_library(
    name = "piet_java_proto_lite",
    deps = [":piet_proto"],
)
