// Copyright 2017 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cmath>

#include "examples/libfuzzer/libfuzzer_example.pb.h"
#include "src/libfuzzer/libfuzzer_macro.h"

protobuf_mutator::protobuf::LogSilencer log_silincer;

DEFINE_BINARY_PROTO_FUZZER(const libfuzzer_example::Msg& message) {
  // Emulate a bug.
  if (message.optional_string() == "FooBar" &&
      message.optional_uint64() > 100 &&
      !std::isnan(message.optional_float()) &&
      std::fabs(message.optional_float()) > 1000 &&
      std::fabs(message.optional_float()) < 1E10) {
    abort();
  }
}
