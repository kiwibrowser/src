// Copyright (c) 2016 Google
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

// Assembler tests for instructions in the "Barrier Instructions" section
// of the SPIR-V spec.

#include "UnitSPIRV.h"

#include "TestFixture.h"
#include "gmock/gmock.h"

namespace {

using ::spvtest::MakeInstruction;
using ::testing::Eq;
using std::vector;

using OpGetKernelLocalSizeForSubgroupCountTest = spvtest::TextToBinaryTest;

TEST_F(OpGetKernelLocalSizeForSubgroupCountTest, OpcodeUnrecognizedInV10) {
  EXPECT_THAT(
      CompileFailure("%res = OpGetKernelLocalSizeForSubgroupCount %type "
                     "%sgcount %invoke %param %param_size %param_align",
                     SPV_ENV_UNIVERSAL_1_0),
      Eq("Invalid Opcode name 'OpGetKernelLocalSizeForSubgroupCount'"));
}

TEST_F(OpGetKernelLocalSizeForSubgroupCountTest, ArgumentCount) {
  EXPECT_THAT(CompileFailure("OpGetKernelLocalSizeForSubgroupCount",
                             SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected <result-id> at the beginning of an instruction, "
                 "found 'OpGetKernelLocalSizeForSubgroupCount'."));
  EXPECT_THAT(CompileFailure("%res = OpGetKernelLocalSizeForSubgroupCount",
                             SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected operand, found end of stream."));
  EXPECT_THAT(
      CompileFailure("%1 = OpGetKernelLocalSizeForSubgroupCount %2 %3 %4 %5 %6",
                     SPV_ENV_UNIVERSAL_1_1),
      Eq("Expected operand, found end of stream."));
  EXPECT_THAT(
      CompiledInstructions("%res = OpGetKernelLocalSizeForSubgroupCount %type "
                           "%sgcount %invoke %param %param_size %param_align",
                           SPV_ENV_UNIVERSAL_1_1),
      Eq(MakeInstruction(SpvOpGetKernelLocalSizeForSubgroupCount,
                         {1, 2, 3, 4, 5, 6, 7})));
  EXPECT_THAT(
      CompileFailure("%res = OpGetKernelLocalSizeForSubgroupCount %type "
                     "%sgcount %invoke %param %param_size %param_align %extra",
                     SPV_ENV_UNIVERSAL_1_1),
      Eq("Expected '=', found end of stream."));
}

TEST_F(OpGetKernelLocalSizeForSubgroupCountTest, ArgumentTypes) {
  EXPECT_THAT(CompileFailure(
                  "%1 = OpGetKernelLocalSizeForSubgroupCount 2 %3 %4 %5 %6 %7",
                  SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected id to start with %."));
  EXPECT_THAT(
      CompileFailure(
          "%1 = OpGetKernelLocalSizeForSubgroupCount %2 %3 %4 %5 %6 \"abc\"",
          SPV_ENV_UNIVERSAL_1_1),
      Eq("Expected id to start with %."));
}

using OpGetKernelMaxNumSubgroupsTest = spvtest::TextToBinaryTest;

TEST_F(OpGetKernelMaxNumSubgroupsTest, OpcodeUnrecognizedInV10) {
  EXPECT_THAT(CompileFailure("%res = OpGetKernelLocalSizeForSubgroupCount "
                             "%type %invoke %param %param_size %param_align",
                             SPV_ENV_UNIVERSAL_1_0),
              Eq("Invalid Opcode name 'OpGetKernelLocalSizeForSubgroupCount'"));
}

TEST_F(OpGetKernelMaxNumSubgroupsTest, ArgumentCount) {
  EXPECT_THAT(
      CompileFailure("OpGetKernelMaxNumSubgroups", SPV_ENV_UNIVERSAL_1_1),
      Eq("Expected <result-id> at the beginning of an instruction, found "
         "'OpGetKernelMaxNumSubgroups'."));
  EXPECT_THAT(CompileFailure("%res = OpGetKernelMaxNumSubgroups",
                             SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected operand, found end of stream."));
  EXPECT_THAT(CompileFailure("%1 = OpGetKernelMaxNumSubgroups %2 %3 %4 %5",
                             SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected operand, found end of stream."));
  EXPECT_THAT(
      CompiledInstructions("%res = OpGetKernelMaxNumSubgroups %type "
                           "%invoke %param %param_size %param_align",
                           SPV_ENV_UNIVERSAL_1_1),
      Eq(MakeInstruction(SpvOpGetKernelMaxNumSubgroups, {1, 2, 3, 4, 5, 6})));
  EXPECT_THAT(CompileFailure("%res = OpGetKernelMaxNumSubgroups %type %invoke "
                             "%param %param_size %param_align %extra",
                             SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected '=', found end of stream."));
}

TEST_F(OpGetKernelMaxNumSubgroupsTest, ArgumentTypes) {
  EXPECT_THAT(CompileFailure("%1 = OpGetKernelMaxNumSubgroups 2 %3 %4 %5 %6",
                             SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected id to start with %."));
  EXPECT_THAT(
      CompileFailure("%1 = OpGetKernelMaxNumSubgroups %2 %3 %4 %5 \"abc\"",
                     SPV_ENV_UNIVERSAL_1_1),
      Eq("Expected id to start with %."));
}

}  // anonymous namespace
