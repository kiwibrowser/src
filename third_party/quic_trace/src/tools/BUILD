# Copyright 2018 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

licenses(["notice"])  # Apache 2.0

cc_binary(
    name = "quic_trace_to_time_sequence_gnuplot",
    srcs = ["quic_trace_to_time_sequence_gnuplot.cc"],
    deps = [
        "//lib:quic_trace_cc_proto",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
    ],
)

cc_binary(
    name = "transform_quic_trace",
    srcs = ["transform_quic_trace.cc"],
    deps = [
        "//lib:quic_trace_cc_proto",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf",
    ],
)

sh_binary(
    name = "time_sequence_gnuplot",
    srcs = ["time_sequence_gnuplot.sh"],
    data = [":quic_trace_to_time_sequence_gnuplot"],
)
