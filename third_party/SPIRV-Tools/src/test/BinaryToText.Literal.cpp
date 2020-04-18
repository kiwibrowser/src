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

#include "gmock/gmock.h"
#include "TestFixture.h"

using ::testing::Eq;

namespace {

using RoundTripLiteralsTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<std::string>>;

TEST_P(RoundTripLiteralsTest, Sample) {
  EXPECT_THAT(EncodeAndDecodeSuccessfully(GetParam()), Eq(GetParam()));
}

// clang-format off
INSTANTIATE_TEST_CASE_P(
    StringLiterals, RoundTripLiteralsTest,
    ::testing::ValuesIn(std::vector<std::string>{
        "OpName %1 \"\"\n",           // empty
        "OpName %1 \"foo\"\n",        // normal
        "OpName %1 \"foo bar\"\n",    // string with spaces
        "OpName %1 \"foo\tbar\"\n",   // string with tab
        "OpName %1 \"\tfoo\"\n",      // starts with tab
        "OpName %1 \" foo\"\n",       // starts with space
        "OpName %1 \"foo \"\n",       // ends with space
        "OpName %1 \"foo\t\"\n",       // ends with tab
        "OpName %1 \"foo\nbar\"\n",               // contains newline
        "OpName %1 \"\nfoo\nbar\"\n",             // starts with newline
        "OpName %1 \"\n\n\nfoo\nbar\"\n",         // multiple newlines
        "OpName %1 \"\\\"foo\nbar\\\"\"\n",       // escaped quote
        "OpName %1 \"\\\\foo\nbar\\\\\"\n",       // escaped backslash
        "OpName %1 \"\xE4\xBA\xB2\"\n",             // UTF-8
    }),);
// clang-format on

using RoundTripSpecialCaseLiteralsTest = spvtest::TextToBinaryTestBase<
    ::testing::TestWithParam<std::pair<std::string, std::string>>>;

// Test case where the generated disassembly is not the same as the
// assembly passed in.
TEST_P(RoundTripSpecialCaseLiteralsTest, Sample) {
  EXPECT_THAT(EncodeAndDecodeSuccessfully(std::get<0>(GetParam())),
              Eq(std::get<1>(GetParam())));
}

// clang-format off
INSTANTIATE_TEST_CASE_P(
    StringLiterals, RoundTripSpecialCaseLiteralsTest,
    ::testing::ValuesIn(std::vector<std::pair<std::string, std::string>>{
      {"OpName %1 \"\\foo\"\n", "OpName %1 \"foo\"\n"}, // Escape f
      {"OpName %1 \"\\\nfoo\"\n", "OpName %1 \"\nfoo\"\n"}, // Escape newline
      {"OpName %1 \"\\\xE4\xBA\xB2\"\n", "OpName %1 \"\xE4\xBA\xB2\"\n"}, // Escape utf-8
    }),);
// clang-format on

}  // anonymous namespace
