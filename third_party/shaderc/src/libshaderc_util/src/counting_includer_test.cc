// Copyright 2015 The Shaderc Authors. All rights reserved.
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

#include "libshaderc_util/counting_includer.h"

#include <gmock/gmock.h>
#include <thread>

namespace {

const auto kRelative = glslang::TShader::Includer::EIncludeRelative;
const auto kStandard = glslang::TShader::Includer::EIncludeStandard;

// A trivial implementation of CountingIncluder's virtual methods, so tests can
// instantiate.
class ConcreteCountingIncluder : public shaderc_util::CountingIncluder {
 public:
  virtual glslang::TShader::Includer::IncludeResult* include_delegate(
      const char* requested, glslang::TShader::Includer::IncludeType,
      const char* requestor,
      size_t) override {
    const char kError[] = "Unexpected #include";
    return new glslang::TShader::Includer::IncludeResult{
        "", kError, strlen(kError), nullptr};
  }
  virtual void release_delegate(
      glslang::TShader::Includer::IncludeResult* include_result) override {
    delete include_result;
  }
};

TEST(CountingIncluderTest, InitialCount) {
  EXPECT_EQ(0, ConcreteCountingIncluder().num_include_directives());
}

TEST(CountingIncluderTest, OneInclude) {
  ConcreteCountingIncluder includer;
  includer.include("random file name", kRelative, "from me", 0);
  EXPECT_EQ(1, includer.num_include_directives());
}

TEST(CountingIncluderTest, TwoIncludesAnyIncludeType) {
  ConcreteCountingIncluder includer;
  includer.include("name1", kRelative, "from me", 0);
  includer.include("name2", kStandard, "me", 0);
  EXPECT_EQ(2, includer.num_include_directives());
}

TEST(CountingIncluderTest, ManyIncludes) {
  ConcreteCountingIncluder includer;
  for (int i = 0; i < 100; ++i) {
    includer.include("filename", kRelative, "from me", i);
  }
  EXPECT_EQ(100, includer.num_include_directives());
}

#ifndef SHADERC_DISABLE_THREADED_TESTS
TEST(CountingIncluderTest, ThreadedIncludes) {
  ConcreteCountingIncluder includer;
  std::thread t1(
      [&includer]() { includer.include("name1", kRelative, "me", 0); });
  std::thread t2(
      [&includer]() { includer.include("name2", kRelative, "me", 1); });
  std::thread t3(
      [&includer]() { includer.include("name3", kRelative, "me", 2); });
  t1.join();
  t2.join();
  t3.join();
  EXPECT_EQ(3, includer.num_include_directives());
}
#endif // SHADERC_DISABLE_THREADED_TESTS

}  // anonymous namespace
