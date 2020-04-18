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

// Assembler tests for instructions in the "Function" section of the
// SPIR-V spec.

#include "UnitSPIRV.h"

#include "gmock/gmock.h"
#include "TestFixture.h"

namespace {

using spvtest::EnumCase;
using spvtest::MakeInstruction;
using spvtest::TextToBinaryTest;
using ::testing::Eq;

// Test OpFunction

using OpFunctionControlTest = spvtest::TextToBinaryTestBase<
    ::testing::TestWithParam<EnumCase<SpvFunctionControlMask>>>;

TEST_P(OpFunctionControlTest, AnySingleFunctionControlMask) {
  const std::string input = "%result_id = OpFunction %result_type " +
                            GetParam().name() + " %function_type ";
  EXPECT_THAT(
      CompiledInstructions(input),
      Eq(MakeInstruction(SpvOpFunction, {1, 2, GetParam().value(), 3})));
}

// clang-format off
#define CASE(VALUE,NAME) { SpvFunctionControl##VALUE, NAME }
INSTANTIATE_TEST_CASE_P(TextToBinaryFunctionTest, OpFunctionControlTest,
                        ::testing::ValuesIn(std::vector<EnumCase<SpvFunctionControlMask>>{
                            CASE(MaskNone, "None"),
                            CASE(InlineMask, "Inline"),
                            CASE(DontInlineMask, "DontInline"),
                            CASE(PureMask, "Pure"),
                            CASE(ConstMask, "Const"),
                        }),);
#undef CASE
// clang-format on

TEST_F(OpFunctionControlTest, CombinedFunctionControlMask) {
  // Sample a single combination.  This ensures we've integrated
  // the instruction parsing logic with spvTextParseMask.
  const std::string input =
      "%result_id = OpFunction %result_type Inline|Pure|Const %function_type";
  const uint32_t expected_mask = SpvFunctionControlInlineMask |
                                 SpvFunctionControlPureMask |
                                 SpvFunctionControlConstMask;
  EXPECT_THAT(CompiledInstructions(input),
              Eq(MakeInstruction(SpvOpFunction, {1, 2, expected_mask, 3})));
}

TEST_F(OpFunctionControlTest, WrongFunctionControl) {
  EXPECT_THAT(CompileFailure("%r = OpFunction %t Inline|Unroll %ft"),
              Eq("Invalid function control operand 'Inline|Unroll'."));
}

// TODO(dneto): OpFunctionParameter
// TODO(dneto): OpFunctionEnd
// TODO(dneto): OpFunctionCall

}  // anonymous namespace
