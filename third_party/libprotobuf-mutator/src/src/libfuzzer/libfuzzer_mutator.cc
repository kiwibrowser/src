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

#include "src/libfuzzer/libfuzzer_mutator.h"

#include <string.h>
#include <cassert>
#include <memory>
#include <string>

#include "port/protobuf.h"
#include "src/mutator.h"

extern "C" size_t LLVMFuzzerMutate(uint8_t*, size_t, size_t)
    __attribute__((weak));

namespace protobuf_mutator {
namespace libfuzzer {

namespace {

template <class T>
T MutateValue(T v) {
  size_t size =
      LLVMFuzzerMutate(reinterpret_cast<uint8_t*>(&v), sizeof(v), sizeof(v));
  memset(reinterpret_cast<uint8_t*>(&v) + size, 0, sizeof(v) - size);
  return v;
}

}  // namespace

int32_t Mutator::MutateInt32(int32_t value) { return MutateValue(value); }

int64_t Mutator::MutateInt64(int64_t value) { return MutateValue(value); }

uint32_t Mutator::MutateUInt32(uint32_t value) { return MutateValue(value); }

uint64_t Mutator::MutateUInt64(uint64_t value) { return MutateValue(value); }

float Mutator::MutateFloat(float value) { return MutateValue(value); }

double Mutator::MutateDouble(double value) { return MutateValue(value); }

std::string Mutator::MutateString(const std::string& value,
                                  size_t size_increase_hint) {
  // Randomly return empty strings as LLVMFuzzerMutate does not produce them.
  if (!std::uniform_int_distribution<uint8_t>(0, 20)(*random())) return {};
  std::string result = value;
  result.resize(value.size() + size_increase_hint);
  if (result.empty()) result.push_back(0);
  result.resize(LLVMFuzzerMutate(reinterpret_cast<uint8_t*>(&result[0]),
                                 value.size(), result.size()));
  return result;
}

}  // namespace libfuzzer
}  // namespace protobuf_mutator
