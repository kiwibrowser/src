# Copyright 2017 Google Inc. All rights reserved.
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

add_subdirectory(libfuzzer)

add_library(protobuf-mutator
            binary_format.cc
            mutator.cc
            text_format.cc
            utf8_fix.cc)
target_link_libraries(protobuf-mutator
                      ${PROTOBUF_LIBRARIES})
set_property(TARGET protobuf-mutator
             PROPERTY COMPILE_FLAGS "${NO_FUZZING_FLAGS}")

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
                      mutator_test_proto2.proto
                      mutator_test_proto3.proto)

add_executable(mutator_test
               mutator_test.cc
               utf8_fix_test.cc
               weighted_reservoir_sampler_test.cc
               ${PROTO_SRCS})
target_link_libraries(mutator_test
                      protobuf-mutator
                      ${ZLIB_LIBRARIES}
                      ${GTEST_BOTH_LIBRARIES}
                      ${CMAKE_THREAD_LIBS_INIT})

ProcessorCount(CPU_COUNT)
math(EXPR TEST_SHARDS_COUNT 2*${CPU_COUNT})
math(EXPR TEST_SHARDS_MAX ${TEST_SHARDS_COUNT}-1)
foreach(SHARD RANGE ${TEST_SHARDS_MAX})
  add_test(test.protobuf_mutator_test_${SHARD} mutator_test --gtest_color=yes AUTO)
  set_property(
      TEST test.protobuf_mutator_test_${SHARD}
      APPEND PROPERTY ENVIRONMENT
      GTEST_SHARD_INDEX=${SHARD}
      GTEST_TOTAL_SHARDS=${TEST_SHARDS_COUNT})
endforeach(SHARD)

add_dependencies(check mutator_test)
