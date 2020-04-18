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

// Assembler tests for instructions in the "Debug" section of the
// SPIR-V spec.

#include "UnitSPIRV.h"

#include <string>

#include "gmock/gmock.h"
#include "TestFixture.h"

namespace {

using spvtest::MakeInstruction;
using spvtest::MakeVector;
using spvtest::TextToBinaryTest;
using ::testing::Eq;

// Test OpSource

// A single test case for OpSource
struct LanguageCase {
  uint32_t get_language_value() const {
    return static_cast<uint32_t>(language_value);
  }
  const char* language_name;
  SpvSourceLanguage language_value;
  uint32_t version;
};

// clang-format off
// The list of OpSource cases to use.
const LanguageCase kLanguageCases[] = {
#define CASE(NAME, VERSION) \
  { #NAME, SpvSourceLanguage##NAME, VERSION }
  CASE(Unknown, 0),
  CASE(Unknown, 999),
  CASE(ESSL, 310),
  CASE(GLSL, 450),
  CASE(OpenCL_C, 120),
  CASE(OpenCL_C, 200),
  CASE(OpenCL_C, 210),
  CASE(OpenCL_CPP, 210),
#undef CASE
};
// clang-format on

using OpSourceTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<LanguageCase>>;

TEST_P(OpSourceTest, AnyLanguage) {
  const std::string input = std::string("OpSource ") +
                            GetParam().language_name + " " +
                            std::to_string(GetParam().version);
  EXPECT_THAT(CompiledInstructions(input),
              Eq(MakeInstruction(SpvOpSource, {GetParam().get_language_value(),
                                               GetParam().version})));
}

INSTANTIATE_TEST_CASE_P(TextToBinaryTestDebug, OpSourceTest,
                        ::testing::ValuesIn(kLanguageCases),);

TEST_F(OpSourceTest, WrongLanguage) {
  EXPECT_THAT(CompileFailure("OpSource xxyyzz 12345"),
              Eq("Invalid source language 'xxyyzz'."));
}

TEST_F(TextToBinaryTest, OpSourceAcceptsOptionalFileId) {
  // In the grammar, the file id is an OperandOptionalId.
  const std::string input = "OpSource GLSL 450 %file_id";
  EXPECT_THAT(
      CompiledInstructions(input),
      Eq(MakeInstruction(SpvOpSource, {SpvSourceLanguageGLSL, 450, 1})));
}

TEST_F(TextToBinaryTest, OpSourceAcceptsOptionalSourceText) {
  std::string fake_source = "To be or not to be";
  const std::string input =
      "OpSource GLSL 450 %file_id \"" + fake_source + "\"";
  EXPECT_THAT(CompiledInstructions(input),
              Eq(MakeInstruction(SpvOpSource, {SpvSourceLanguageGLSL, 450, 1},
                                 MakeVector(fake_source))));
}

// Test OpSourceContinued

using OpSourceContinuedTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<const char*>>;

TEST_P(OpSourceContinuedTest, AnyExtension) {
  // TODO(dneto): utf-8, quoting, escaping
  const std::string input =
      std::string("OpSourceContinued \"") + GetParam() + "\"";
  EXPECT_THAT(
      CompiledInstructions(input),
      Eq(MakeInstruction(SpvOpSourceContinued, MakeVector(GetParam()))));
}

// TODO(dneto): utf-8, quoting, escaping
INSTANTIATE_TEST_CASE_P(TextToBinaryTestDebug, OpSourceContinuedTest,
                        ::testing::ValuesIn(std::vector<const char*>{
                            "", "foo bar this and that"}),);

// Test OpSourceExtension

using OpSourceExtensionTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<const char*>>;

TEST_P(OpSourceExtensionTest, AnyExtension) {
  // TODO(dneto): utf-8, quoting, escaping
  const std::string input =
      std::string("OpSourceExtension \"") + GetParam() + "\"";
  EXPECT_THAT(
      CompiledInstructions(input),
      Eq(MakeInstruction(SpvOpSourceExtension, MakeVector(GetParam()))));
}

// TODO(dneto): utf-8, quoting, escaping
INSTANTIATE_TEST_CASE_P(TextToBinaryTestDebug, OpSourceExtensionTest,
                        ::testing::ValuesIn(std::vector<const char*>{
                            "", "foo bar this and that"}),);

TEST_F(TextToBinaryTest, OpLine) {
  EXPECT_THAT(CompiledInstructions("OpLine %srcfile 42 99"),
              Eq(MakeInstruction(SpvOpLine, {1, 42, 99})));
}

TEST_F(TextToBinaryTest, OpNoLine) {
  EXPECT_THAT(CompiledInstructions("OpNoLine"),
              Eq(MakeInstruction(SpvOpNoLine, {})));
}

using OpStringTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<const char*>>;

TEST_P(OpStringTest, AnyString) {
  // TODO(dneto): utf-8, quoting, escaping
  const std::string input =
      std::string("%result = OpString \"") + GetParam() + "\"";
  EXPECT_THAT(CompiledInstructions(input),
              Eq(MakeInstruction(SpvOpString, {1}, MakeVector(GetParam()))));
}

// TODO(dneto): utf-8, quoting, escaping
INSTANTIATE_TEST_CASE_P(TextToBinaryTestDebug, OpStringTest,
                        ::testing::ValuesIn(std::vector<const char*>{
                            "", "foo bar this and that"}),);

using OpNameTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<const char*>>;

TEST_P(OpNameTest, AnyString) {
  // TODO(dneto): utf-8, quoting, escaping
  const std::string input =
      std::string("OpName %target \"") + GetParam() + "\"";
  EXPECT_THAT(CompiledInstructions(input),
              Eq(MakeInstruction(SpvOpName, {1}, MakeVector(GetParam()))));
}

// TODO(dneto): utf-8, quoting, escaping
INSTANTIATE_TEST_CASE_P(TextToBinaryTestDebug, OpNameTest,
                        ::testing::ValuesIn(std::vector<const char*>{
                            "", "foo bar this and that"}),);

using OpMemberNameTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<const char*>>;

TEST_P(OpMemberNameTest, AnyString) {
  // TODO(dneto): utf-8, quoting, escaping
  const std::string input =
      std::string("OpMemberName %type 42 \"") + GetParam() + "\"";
  EXPECT_THAT(
      CompiledInstructions(input),
      Eq(MakeInstruction(SpvOpMemberName, {1, 42}, MakeVector(GetParam()))));
}

// TODO(dneto): utf-8, quoting, escaping
INSTANTIATE_TEST_CASE_P(TextToBinaryTestDebug, OpMemberNameTest,
                        ::testing::ValuesIn(std::vector<const char*>{
                            "", "foo bar this and that"}),);

// TODO(dneto): Parse failures?

}  // anonymous namespace
