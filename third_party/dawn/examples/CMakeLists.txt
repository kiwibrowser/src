# Copyright 2020 The Dawn Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_library(dawn_sample_utils STATIC ${DAWN_DUMMY_FILE})
target_sources(dawn_sample_utils PRIVATE
    "SampleUtils.cpp"
    "SampleUtils.h"
)
target_link_libraries(dawn_sample_utils PUBLIC
    dawn_internal_config
    dawncpp
    dawn_proc
    dawn_common
    dawn_native
    dawn_wire
    dawn_utils
    glfw
)

add_executable(CppHelloTriangle "CppHelloTriangle.cpp")
target_link_libraries(CppHelloTriangle dawn_sample_utils)

add_executable(CHelloTriangle "CHelloTriangle.cpp")
target_link_libraries(CHelloTriangle dawn_sample_utils)

add_executable(ComputeBoids "ComputeBoids.cpp")
target_link_libraries(ComputeBoids dawn_sample_utils glm)

add_executable(Animometer "Animometer.cpp")
target_link_libraries(Animometer dawn_sample_utils)

add_executable(CubeReflection "CubeReflection.cpp")
target_link_libraries(CubeReflection dawn_sample_utils glm)
