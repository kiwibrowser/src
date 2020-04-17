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

licenses(["notice"])  # BSD, Mesa, MIT, SGI, Khronos

# The source version of GLEW 2.1.0 is a custom, reduced-size version that only
# contains the extensions we need (otherwise, GLEW takes up more than a
# megabyte in source code).  How to rebuild it:
#   1. Get a copy of GLEW source
#   2. Copy custom.txt into auto/ directory.
#   3. Run `make custom` in auto/ directory.
#   4. Copy the resulting files.
cc_library(
    name = "glew",
    srcs = ["src/glew.c"],
    hdrs = glob(["include/GL/*.h"]),
    includes = ["include"],
    linkopts = select({
        "//buildenv:linux": ["-lGL"],
        "//buildenv:osx": ["-framework OpenGL"],
    }),
    visibility = ["//visibility:public"],
)
