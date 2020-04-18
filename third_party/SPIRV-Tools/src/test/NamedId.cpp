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

#include <vector>

#include "TestFixture.h"
#include "UnitSPIRV.h"

namespace {

using NamedIdTest = spvtest::TextToBinaryTest;

TEST_F(NamedIdTest, Default) {
  const std::string input = R"(
          OpCapability Shader
          OpMemoryModel Logical Simple
          OpEntryPoint Vertex %main "foo"
  %void = OpTypeVoid
%fnMain = OpTypeFunction %void
  %main = OpFunction %void None %fnMain
%lbMain = OpLabel
          OpReturn
          OpFunctionEnd)";
  const std::string output =
      "OpCapability Shader\n"
      "OpMemoryModel Logical Simple\n"
      "OpEntryPoint Vertex %1 \"foo\"\n"
      "%2 = OpTypeVoid\n"
      "%3 = OpTypeFunction %2\n"
      "%1 = OpFunction %2 None %3\n"
      "%4 = OpLabel\n"
      "OpReturn\n"
      "OpFunctionEnd\n";
  EXPECT_EQ(output, EncodeAndDecodeSuccessfully(input));
}

struct IdCheckCase {
  std::string id;
  bool valid;
};

using IdValidityTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<IdCheckCase>>;

TEST_P(IdValidityTest, IdTypes) {
  const std::string input = GetParam().id + " = OpTypeVoid";
  SetText(input);
  if (GetParam().valid) {
    CompileSuccessfully(input);
  } else {
    CompileFailure(input);
  }
}

INSTANTIATE_TEST_CASE_P(
    ValidAndInvalidIds, IdValidityTest,
    ::testing::ValuesIn(std::vector<IdCheckCase>({{"%1", true},
                                                  {"%2abc", true},
                                                  {"%3Def", true},
                                                  {"%4GHI", true},
                                                  {"%5_j_k", true},
                                                  {"%6J_M", true},
                                                  {"%n", true},
                                                  {"%O", true},
                                                  {"%p7", true},
                                                  {"%Q8", true},
                                                  {"%R_S", true},
                                                  {"%T_10_U", true},
                                                  {"%V_11", true},
                                                  {"%W_X_13", true},
                                                  {"%_A", true},
                                                  {"%_", true},
                                                  {"%__", true},
                                                  {"%A_", true},
                                                  {"%_A_", true},

                                                  {"%@", false},
                                                  {"%!", false},
                                                  {"%ABC!", false},
                                                  {"%__A__@", false},
                                                  {"%%", false},
                                                  {"%-", false},
                                                  {"%foo_@_bar", false},
                                                  {"%", false},

                                                  {"5", false},
                                                  {"32", false},
                                                  {"foo", false},
                                                  {"a%bar", false}})),);

}  // anonymous namespace
