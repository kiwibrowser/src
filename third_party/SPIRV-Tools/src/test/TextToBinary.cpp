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

#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

#include "gmock/gmock.h"

#include "TestFixture.h"
#include "UnitSPIRV.h"
#include "source/spirv_constant.h"
#include "source/util/bitutils.h"
#include "source/util/hex_float.h"

namespace {

using libspirv::AssemblyContext;
using libspirv::AssemblyGrammar;
using spvtest::AutoText;
using spvtest::Concatenate;
using spvtest::MakeInstruction;
using spvtest::ScopedContext;
using spvtest::TextToBinaryTest;
using testing::Eq;
using testing::IsNull;
using testing::NotNull;

// An mask parsing test case.
struct MaskCase {
  spv_operand_type_t which_enum;
  uint32_t expected_value;
  const char* expression;
};

using GoodMaskParseTest = ::testing::TestWithParam<MaskCase>;

TEST_P(GoodMaskParseTest, GoodMaskExpressions) {
  spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_0);

  uint32_t value;
  EXPECT_EQ(SPV_SUCCESS,
            AssemblyGrammar(context).parseMaskOperand(
                GetParam().which_enum, GetParam().expression, &value));
  EXPECT_EQ(GetParam().expected_value, value);

  spvContextDestroy(context);
}

INSTANTIATE_TEST_CASE_P(
    ParseMask, GoodMaskParseTest,
    ::testing::ValuesIn(std::vector<MaskCase>{
        {SPV_OPERAND_TYPE_FP_FAST_MATH_MODE, 0, "None"},
        {SPV_OPERAND_TYPE_FP_FAST_MATH_MODE, 1, "NotNaN"},
        {SPV_OPERAND_TYPE_FP_FAST_MATH_MODE, 2, "NotInf"},
        {SPV_OPERAND_TYPE_FP_FAST_MATH_MODE, 3, "NotNaN|NotInf"},
        // Mask experssions are symmetric.
        {SPV_OPERAND_TYPE_FP_FAST_MATH_MODE, 3, "NotInf|NotNaN"},
        // Repeating a value has no effect.
        {SPV_OPERAND_TYPE_FP_FAST_MATH_MODE, 3, "NotInf|NotNaN|NotInf"},
        // Using 3 operands still works.
        {SPV_OPERAND_TYPE_FP_FAST_MATH_MODE, 0x13, "NotInf|NotNaN|Fast"},
        {SPV_OPERAND_TYPE_SELECTION_CONTROL, 0, "None"},
        {SPV_OPERAND_TYPE_SELECTION_CONTROL, 1, "Flatten"},
        {SPV_OPERAND_TYPE_SELECTION_CONTROL, 2, "DontFlatten"},
        // Weirdly, you can specify to flatten and don't flatten a selection.
        {SPV_OPERAND_TYPE_SELECTION_CONTROL, 3, "Flatten|DontFlatten"},
        {SPV_OPERAND_TYPE_LOOP_CONTROL, 0, "None"},
        {SPV_OPERAND_TYPE_LOOP_CONTROL, 1, "Unroll"},
        {SPV_OPERAND_TYPE_LOOP_CONTROL, 2, "DontUnroll"},
        // Weirdly, you can specify to unroll and don't unroll a loop.
        {SPV_OPERAND_TYPE_LOOP_CONTROL, 3, "Unroll|DontUnroll"},
        {SPV_OPERAND_TYPE_FUNCTION_CONTROL, 0, "None"},
        {SPV_OPERAND_TYPE_FUNCTION_CONTROL, 1, "Inline"},
        {SPV_OPERAND_TYPE_FUNCTION_CONTROL, 2, "DontInline"},
        {SPV_OPERAND_TYPE_FUNCTION_CONTROL, 4, "Pure"},
        {SPV_OPERAND_TYPE_FUNCTION_CONTROL, 8, "Const"},
        {SPV_OPERAND_TYPE_FUNCTION_CONTROL, 0xd, "Inline|Const|Pure"},
    }), );

using BadFPFastMathMaskParseTest = ::testing::TestWithParam<const char*>;

TEST_P(BadFPFastMathMaskParseTest, BadMaskExpressions) {
  spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_0);

  uint32_t value;
  EXPECT_NE(SPV_SUCCESS,
            AssemblyGrammar(context).parseMaskOperand(
                SPV_OPERAND_TYPE_FP_FAST_MATH_MODE, GetParam(), &value));

  spvContextDestroy(context);
}

INSTANTIATE_TEST_CASE_P(ParseMask, BadFPFastMathMaskParseTest,
                        ::testing::ValuesIn(std::vector<const char*>{
                            nullptr, "", "NotValidEnum", "|", "NotInf|",
                            "|NotInf", "NotInf||NotNaN",
                            "Unroll"  // A good word, but for the wrong enum
                        }), );

TEST_F(TextToBinaryTest, InvalidText) {
  ASSERT_EQ(SPV_ERROR_INVALID_TEXT,
            spvTextToBinary(ScopedContext().context, nullptr, 0, &binary,
                            &diagnostic));
  EXPECT_NE(nullptr, diagnostic);
  EXPECT_THAT(diagnostic->error, Eq(std::string("Missing assembly text.")));
}

TEST_F(TextToBinaryTest, InvalidPointer) {
  SetText(
      "OpEntryPoint Kernel 0 \"\"\nOpExecutionMode 0 LocalSizeHint 1 1 1\n");
  ASSERT_EQ(SPV_ERROR_INVALID_POINTER,
            spvTextToBinary(ScopedContext().context, text.str, text.length,
                            nullptr, &diagnostic));
}

TEST_F(TextToBinaryTest, InvalidDiagnostic) {
  SetText(
      "OpEntryPoint Kernel 0 \"\"\nOpExecutionMode 0 LocalSizeHint 1 1 1\n");
  ASSERT_EQ(SPV_ERROR_INVALID_DIAGNOSTIC,
            spvTextToBinary(ScopedContext().context, text.str, text.length,
                            &binary, nullptr));
}

TEST_F(TextToBinaryTest, InvalidPrefix) {
  EXPECT_EQ(
      "Expected <opcode> or <result-id> at the beginning of an instruction, "
      "found 'Invalid'.",
      CompileFailure("Invalid"));
}

TEST_F(TextToBinaryTest, EmptyAssemblyString) {
  // An empty assembly module is valid!
  // It should produce a valid module with zero instructions.
  EXPECT_THAT(CompiledInstructions(""), Eq(std::vector<uint32_t>{}));
}

TEST_F(TextToBinaryTest, StringSpace) {
  const std::string code = ("OpSourceExtension \"string with spaces\"\n");
  EXPECT_EQ(code, EncodeAndDecodeSuccessfully(code));
}

TEST_F(TextToBinaryTest, UnknownBeginningOfInstruction) {
  EXPECT_EQ(
      "Expected <opcode> or <result-id> at the beginning of an instruction, "
      "found 'Google'.",
      CompileFailure(
          "\nOpSource OpenCL_C 12\nOpMemoryModel Physical64 OpenCL\nGoogle\n"));
  EXPECT_EQ(4u, diagnostic->position.line + 1);
  EXPECT_EQ(1u, diagnostic->position.column + 1);
}

TEST_F(TextToBinaryTest, NoEqualSign) {
  EXPECT_EQ("Expected '=', found end of stream.",
            CompileFailure("\nOpSource OpenCL_C 12\n"
                           "OpMemoryModel Physical64 OpenCL\n%2\n"));
  EXPECT_EQ(5u, diagnostic->position.line + 1);
  EXPECT_EQ(1u, diagnostic->position.column + 1);
}

TEST_F(TextToBinaryTest, NoOpCode) {
  EXPECT_EQ("Expected opcode, found end of stream.",
            CompileFailure("\nOpSource OpenCL_C 12\n"
                           "OpMemoryModel Physical64 OpenCL\n%2 =\n"));
  EXPECT_EQ(5u, diagnostic->position.line + 1);
  EXPECT_EQ(1u, diagnostic->position.column + 1);
}

TEST_F(TextToBinaryTest, WrongOpCode) {
  EXPECT_EQ("Invalid Opcode prefix 'Wahahaha'.",
            CompileFailure("\nOpSource OpenCL_C 12\n"
                           "OpMemoryModel Physical64 OpenCL\n%2 = Wahahaha\n"));
  EXPECT_EQ(4u, diagnostic->position.line + 1);
  EXPECT_EQ(6u, diagnostic->position.column + 1);
}

TEST_F(TextToBinaryTest, CRLF) {
  const std::string input =
      "%i32 = OpTypeInt 32 1\r\n%c = OpConstant %i32 123\r\n";
  EXPECT_THAT(CompiledInstructions(input),
              Eq(Concatenate({MakeInstruction(SpvOpTypeInt, {1, 32, 1}),
                              MakeInstruction(SpvOpConstant, {1, 2, 123})})));
}

using TextToBinaryFloatValueTest = spvtest::TextToBinaryTestBase<
    ::testing::TestWithParam<std::pair<std::string, uint32_t>>>;

TEST_P(TextToBinaryFloatValueTest, Samples) {
  const std::string input =
      "%1 = OpTypeFloat 32\n%2 = OpConstant %1 " + GetParam().first;
  EXPECT_THAT(CompiledInstructions(input),
              Eq(Concatenate({MakeInstruction(SpvOpTypeFloat, {1, 32}),
                              MakeInstruction(SpvOpConstant,
                                              {1, 2, GetParam().second})})));
}

INSTANTIATE_TEST_CASE_P(
    FloatValues, TextToBinaryFloatValueTest,
    ::testing::ValuesIn(std::vector<std::pair<std::string, uint32_t>>{
        {"0.0", 0x00000000},          // +0
        {"!0x00000001", 0x00000001},  // +denorm
        {"!0x00800000", 0x00800000},  // +norm
        {"1.5", 0x3fc00000},
        {"!0x7f800000", 0x7f800000},  // +inf
        {"!0x7f800001", 0x7f800001},  // NaN

        {"-0.0", 0x80000000},         // -0
        {"!0x80000001", 0x80000001},  // -denorm
        {"!0x80800000", 0x80800000},  // -norm
        {"-2.5", 0xc0200000},
        {"!0xff800000", 0xff800000},  // -inf
        {"!0xff800001", 0xff800001},  // NaN
    }), );

using TextToBinaryHalfValueTest = spvtest::TextToBinaryTestBase<
    ::testing::TestWithParam<std::pair<std::string, uint32_t>>>;

TEST_P(TextToBinaryHalfValueTest, Samples) {
  const std::string input =
      "%1 = OpTypeFloat 16\n%2 = OpConstant %1 " + GetParam().first;
  EXPECT_THAT(CompiledInstructions(input),
              Eq(Concatenate({MakeInstruction(SpvOpTypeFloat, {1, 16}),
                              MakeInstruction(SpvOpConstant,
                                              {1, 2, GetParam().second})})));
}

INSTANTIATE_TEST_CASE_P(
    HalfValues, TextToBinaryHalfValueTest,
    ::testing::ValuesIn(std::vector<std::pair<std::string, uint32_t>>{
        {"0.0", 0x00000000},
        {"1.0", 0x00003c00},
        {"1.000844", 0x00003c00},  // Truncate to 1.0
        {"1.000977", 0x00003c01},  // Don't have to truncate
        {"1.001465", 0x00003c01},  // Truncate to 1.0000977
        {"1.5", 0x00003e00},
        {"-1.0", 0x0000bc00},
        {"2.0", 0x00004000},
        {"-2.0", 0x0000c000},
        {"0x1p1", 0x00004000},
        {"-0x1p1", 0x0000c000},
        {"0x1.8p1", 0x00004200},
        {"0x1.8p4", 0x00004e00},
        {"0x1.801p4", 0x00004e00},
        {"0x1.804p4", 0x00004e01},
    }), );

TEST(AssemblyContextParseNarrowSignedIntegers, Sample) {
  AssemblyContext context(AutoText(""), nullptr);
  const spv_result_t ec = SPV_FAILED_MATCH;
  int16_t i16;

  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("", ec, &i16, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("0=", ec, &i16, ""));

  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0", ec, &i16, ""));
  EXPECT_EQ(0, i16);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("32767", ec, &i16, ""));
  EXPECT_EQ(32767, i16);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-32768", ec, &i16, ""));
  EXPECT_EQ(-32768, i16);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-0", ec, &i16, ""));
  EXPECT_EQ(0, i16);

  // These are out of range, so they should return an error.
  // The error code depends on whether this is an optional value.
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("32768", ec, &i16, ""));
  EXPECT_EQ(SPV_ERROR_INVALID_TEXT,
            context.parseNumber("65535", SPV_ERROR_INVALID_TEXT, &i16, ""));

  // Check hex parsing.
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0x7fff", ec, &i16, ""));
  EXPECT_EQ(32767, i16);
  // This is out of range.
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("0xffff", ec, &i16, ""));
}

TEST(AssemblyContextParseNarrowUnsignedIntegers, Sample) {
  AssemblyContext context(AutoText(""), nullptr);
  const spv_result_t ec = SPV_FAILED_MATCH;
  uint16_t u16;

  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("", ec, &u16, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("0=", ec, &u16, ""));

  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0", ec, &u16, ""));
  EXPECT_EQ(0, u16);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("65535", ec, &u16, ""));
  EXPECT_EQ(65535, u16);
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("65536", ec, &u16, ""));

  // We don't care about -0 since it's rejected at a higher level.
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("-1", ec, &u16, ""));
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0xffff", ec, &u16, ""));
  EXPECT_EQ(0xffff, u16);
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("0x10000", ec, &u16, ""));
}

TEST(AssemblyContextParseSignedIntegers, Sample) {
  AssemblyContext context(AutoText(""), nullptr);
  const spv_result_t ec = SPV_FAILED_MATCH;
  int32_t i32;

  // Invalid parse.
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("", ec, &i32, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("0=", ec, &i32, ""));

  // Decimal values.
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0", ec, &i32, ""));
  EXPECT_EQ(0, i32);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("2147483647", ec, &i32, ""));
  EXPECT_EQ(std::numeric_limits<int32_t>::max(), i32);
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("2147483648", ec, &i32, ""));
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-0", ec, &i32, ""));
  EXPECT_EQ(0, i32);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-1", ec, &i32, ""));
  EXPECT_EQ(-1, i32);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-2147483648", ec, &i32, ""));
  EXPECT_EQ(std::numeric_limits<int32_t>::min(), i32);

  // Hex values.
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0x7fffffff", ec, &i32, ""));
  EXPECT_EQ(std::numeric_limits<int32_t>::max(), i32);
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("0x80000000", ec, &i32, ""));
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-0x000", ec, &i32, ""));
  EXPECT_EQ(0, i32);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-0x001", ec, &i32, ""));
  EXPECT_EQ(-1, i32);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-0x80000000", ec, &i32, ""));
  EXPECT_EQ(std::numeric_limits<int32_t>::min(), i32);
}

TEST(AssemblyContextParseUnsignedIntegers, Sample) {
  AssemblyContext context(AutoText(""), nullptr);
  const spv_result_t ec = SPV_FAILED_MATCH;
  uint32_t u32;

  // Invalid parse.
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("", ec, &u32, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("0=", ec, &u32, ""));

  // Valid values.
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0", ec, &u32, ""));
  EXPECT_EQ(0u, u32);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("4294967295", ec, &u32, ""));
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), u32);
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("4294967296", ec, &u32, ""));

  // Hex values.
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0xffffffff", ec, &u32, ""));
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), u32);

  // We don't care about -0 since it's rejected at a higher level.
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("-1", ec, &u32, ""));
}

TEST(AssemblyContextParseWideSignedIntegers, Sample) {
  AssemblyContext context(AutoText(""), nullptr);
  const spv_result_t ec = SPV_FAILED_MATCH;
  int64_t i64;
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("", ec, &i64, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("0=", ec, &i64, ""));
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0", ec, &i64, ""));
  EXPECT_EQ(0, i64);
  EXPECT_EQ(SPV_SUCCESS,
            context.parseNumber("0x7fffffffffffffff", ec, &i64, ""));
  EXPECT_EQ(0x7fffffffffffffff, i64);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-0", ec, &i64, ""));
  EXPECT_EQ(0, i64);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-1", ec, &i64, ""));
  EXPECT_EQ(-1, i64);
}

TEST(AssemblyContextParseWideUnsignedIntegers, Sample) {
  AssemblyContext context(AutoText(""), nullptr);
  const spv_result_t ec = SPV_FAILED_MATCH;
  uint64_t u64;
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("", ec, &u64, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("0=", ec, &u64, ""));
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0", ec, &u64, ""));
  EXPECT_EQ(0u, u64);
  EXPECT_EQ(SPV_SUCCESS,
            context.parseNumber("0xffffffffffffffff", ec, &u64, ""));
  EXPECT_EQ(0xffffffffffffffffULL, u64);

  // We don't care about -0 since it's rejected at a higher level.
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("-1", ec, &u64, ""));
}

TEST(AssemblyContextParseFloat, Sample) {
  AssemblyContext context(AutoText(""), nullptr);
  const spv_result_t ec = SPV_FAILED_MATCH;
  float f;

  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("", ec, &f, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("0=", ec, &f, ""));

  // These values are exactly representatble.
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0", ec, &f, ""));
  EXPECT_EQ(0.0f, f);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("42", ec, &f, ""));
  EXPECT_EQ(42.0f, f);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("2.5", ec, &f, ""));
  EXPECT_EQ(2.5f, f);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-32.5", ec, &f, ""));
  EXPECT_EQ(-32.5f, f);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("1e38", ec, &f, ""));
  EXPECT_EQ(1e38f, f);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-1e38", ec, &f, ""));
  EXPECT_EQ(-1e38f, f);
}

TEST(AssemblyContextParseFloat, Overflow) {
  // The assembler parses using HexFloat<FloatProxy<float>>.  Make
  // sure that succeeds for in-range values, and fails for out of
  // range values.  When it does overflow, the value is set to the
  // nearest finite value, matching C++11 behavior for operator>>
  // on floating point.
  AssemblyContext context(AutoText(""), nullptr);
  const spv_result_t ec = SPV_FAILED_MATCH;
  spvutils::HexFloat<spvutils::FloatProxy<float>> f(0.0f);

  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("1e38", ec, &f, ""));
  EXPECT_EQ(1e38f, f.value().getAsFloat());
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-1e38", ec, &f, ""));
  EXPECT_EQ(-1e38f, f.value().getAsFloat());
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("1e40", ec, &f, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("-1e40", ec, &f, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("1e400", ec, &f, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("-1e400", ec, &f, ""));
}

TEST(AssemblyContextParseDouble, Sample) {
  AssemblyContext context(AutoText(""), nullptr);
  const spv_result_t ec = SPV_FAILED_MATCH;
  double f;

  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("", ec, &f, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("0=", ec, &f, ""));

  // These values are exactly representatble.
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("0", ec, &f, ""));
  EXPECT_EQ(0.0, f);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("42", ec, &f, ""));
  EXPECT_EQ(42.0, f);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("2.5", ec, &f, ""));
  EXPECT_EQ(2.5, f);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-32.5", ec, &f, ""));
  EXPECT_EQ(-32.5, f);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("1e38", ec, &f, ""));
  EXPECT_EQ(1e38, f);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-1e38", ec, &f, ""));
  EXPECT_EQ(-1e38, f);
  // These are out of range for 32-bit float, but in range for 64-bit float.
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("1e40", ec, &f, ""));
  EXPECT_EQ(1e40, f);
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-1e40", ec, &f, ""));
  EXPECT_EQ(-1e40, f);
}

TEST(AssemblyContextParseDouble, Overflow) {
  // The assembler parses using HexFloat<FloatProxy<double>>.  Make
  // sure that succeeds for in-range values, and fails for out of
  // range values.  When it does overflow, the value is set to the
  // nearest finite value, matching C++11 behavior for operator>>
  // on floating point.
  AssemblyContext context(AutoText(""), nullptr);
  const spv_result_t ec = SPV_FAILED_MATCH;
  spvutils::HexFloat<spvutils::FloatProxy<double>> f(0.0);

  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("1e38", ec, &f, ""));
  EXPECT_EQ(1e38, f.value().getAsFloat());
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-1e38", ec, &f, ""));
  EXPECT_EQ(-1e38, f.value().getAsFloat());
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("1e40", ec, &f, ""));
  EXPECT_EQ(1e40, f.value().getAsFloat());
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-1e40", ec, &f, ""));
  EXPECT_EQ(-1e40, f.value().getAsFloat());
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("1e400", ec, &f, ""));
  EXPECT_EQ(SPV_FAILED_MATCH, context.parseNumber("-1e400", ec, &f, ""));
}

TEST(AssemblyContextParseFloat16, Overflow) {
  // The assembler parses using HexFloat<FloatProxy<Float16>>.  Make
  // sure that succeeds for in-range values, and fails for out of
  // range values.  When it does overflow, the value is set to the
  // nearest finite value, matching C++11 behavior for operator>>
  // on floating point.
  AssemblyContext context(AutoText(""), nullptr);
  const spv_result_t ec = SPV_FAILED_MATCH;
  spvutils::HexFloat<spvutils::FloatProxy<spvutils::Float16>> f(0);

  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("-0.0", ec, &f, ""));
  EXPECT_EQ(uint16_t{0x8000}, f.value().getAsFloat().get_value());
  EXPECT_EQ(SPV_SUCCESS, context.parseNumber("1.0", ec, &f, ""));
  EXPECT_EQ(uint16_t{0x3c00}, f.value().getAsFloat().get_value());

  // Overflows 16-bit but not 32-bit
  EXPECT_EQ(ec, context.parseNumber("1e38", ec, &f, ""));
  EXPECT_EQ(ec, context.parseNumber("-1e38", ec, &f, ""));

  // Overflows 32-bit but not 64-bit
  EXPECT_EQ(ec, context.parseNumber("1e40", ec, &f, ""));
  EXPECT_EQ(ec, context.parseNumber("-1e40", ec, &f, ""));

  // Overflows 64-bit
  EXPECT_EQ(ec, context.parseNumber("1e400", ec, &f, ""));
  EXPECT_EQ(ec, context.parseNumber("-1e400", ec, &f, ""));
}

TEST(AssemblyContextParseMessages, Errors) {
  spv_diagnostic diag = nullptr;
  const spv_result_t ec = SPV_FAILED_MATCH;
  AssemblyContext context(AutoText(""), &diag);
  int16_t i16;

  // No message is generated for a failure to parse an optional value.
  EXPECT_EQ(SPV_FAILED_MATCH,
            context.parseNumber("abc", ec, &i16, "bad narrow int: "));
  EXPECT_EQ(nullptr, diag);

  // For a required value, use the message fragment.
  EXPECT_EQ(SPV_ERROR_INVALID_TEXT,
            context.parseNumber("abc", SPV_ERROR_INVALID_TEXT, &i16,
                                "bad narrow int: "));
  ASSERT_NE(nullptr, diag);
  EXPECT_EQ("bad narrow int: abc", std::string(diag->error));
  // Don't leak.
  spvDiagnosticDestroy(diag);
}

TEST(CreateContext, InvalidEnvironment) {
  spv_target_env env;
  std::memset(&env, 99, sizeof(env));
  EXPECT_THAT(spvContextCreate(env), IsNull());
}

TEST(CreateContext, UniversalEnvironment) {
  auto c = spvContextCreate(SPV_ENV_UNIVERSAL_1_0);
  EXPECT_THAT(c, NotNull());
  spvContextDestroy(c);
}

TEST(CreateContext, VulkanEnvironment) {
  auto c = spvContextCreate(SPV_ENV_VULKAN_1_0);
  EXPECT_THAT(c, NotNull());
  spvContextDestroy(c);
}

}  // anonymous namespace
