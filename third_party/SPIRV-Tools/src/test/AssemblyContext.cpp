// Copyright (c) 2015-2016 The Khronos Group Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

#include "UnitSPIRV.h"

#include <vector>
#include <gmock/gmock.h>

#include "source/instruction.h"

using libspirv::AssemblyContext;
using spvtest::AutoText;
using spvtest::Concatenate;
using ::testing::Eq;

namespace {

struct EncodeStringCase {
  std::string str;
  std::vector<uint32_t> initial_contents;
};

using EncodeStringTest = ::testing::TestWithParam<EncodeStringCase>;

TEST_P(EncodeStringTest, Sample) {
  AssemblyContext context(AutoText(""), nullptr);
  spv_instruction_t inst;
  inst.words = GetParam().initial_contents;
  ASSERT_EQ(SPV_SUCCESS,
            context.binaryEncodeString(GetParam().str.c_str(), &inst));
  // We already trust MakeVector
  EXPECT_THAT(inst.words,
              Eq(Concatenate({GetParam().initial_contents,
                              spvtest::MakeVector(GetParam().str)})));
}

// clang-format off
INSTANTIATE_TEST_CASE_P(
    BinaryEncodeString, EncodeStringTest,
    ::testing::ValuesIn(std::vector<EncodeStringCase>{
      // Use cases that exercise at least one to two words,
      // and both empty and non-empty initial contents.
      {"", {}},
      {"", {1,2,3}},
      {"a", {}},
      {"a", {4}},
      {"ab", {4}},
      {"abc", {}},
      {"abc", {18}},
      {"abcd", {}},
      {"abcd", {22}},
      {"abcde", {4}},
      {"abcdef", {}},
      {"abcdef", {99,42}},
      {"abcdefg", {}},
      {"abcdefg", {101}},
      {"abcdefgh", {}},
      {"abcdefgh", {102, 103, 104}},
      // A very long string, encoded after an initial word.
      // SPIR-V limits strings to 65535 characters.
      {std::string(65535, 'a'), {1}},
    }),);
// clang-format on

}  // anonymous namespace
