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

// Assembler tests for instructions in the "Barrier Instructions" section
// of the SPIR-V spec.

#include "UnitSPIRV.h"

#include "TestFixture.h"
#include "gmock/gmock.h"

namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::_;
using spvtest::MakeInstruction;
using spvtest::TextToBinaryTest;

// Test OpMemoryBarrier

using OpMemoryBarrier = spvtest::TextToBinaryTest;

TEST_F(OpMemoryBarrier, Good) {
  const std::string input = "OpMemoryBarrier %1 %2\n";
  EXPECT_THAT(CompiledInstructions(input),
              Eq(MakeInstruction(SpvOpMemoryBarrier, {1, 2})));
  EXPECT_THAT(EncodeAndDecodeSuccessfully(input), Eq(input));
}

TEST_F(OpMemoryBarrier, BadMissingScopeId) {
  const std::string input = "OpMemoryBarrier\n";
  EXPECT_THAT(CompileFailure(input),
              Eq("Expected operand, found end of stream."));
}

TEST_F(OpMemoryBarrier, BadInvalidScopeId) {
  const std::string input = "OpMemoryBarrier 99\n";
  EXPECT_THAT(CompileFailure(input), Eq("Expected id to start with %."));
}

TEST_F(OpMemoryBarrier, BadMissingMemorySemanticsId) {
  const std::string input = "OpMemoryBarrier %scope\n";
  EXPECT_THAT(CompileFailure(input),
              Eq("Expected operand, found end of stream."));
}

TEST_F(OpMemoryBarrier, BadInvalidMemorySemanticsId) {
  const std::string input = "OpMemoryBarrier %scope 14\n";
  EXPECT_THAT(CompileFailure(input), Eq("Expected id to start with %."));
}

// TODO(dneto): OpControlBarrier
// TODO(dneto): OpGroupAsyncCopy
// TODO(dneto): OpGroupWaitEvents
// TODO(dneto): OpGroupAll
// TODO(dneto): OpGroupAny
// TODO(dneto): OpGroupBroadcast
// TODO(dneto): OpGroupIAdd
// TODO(dneto): OpGroupFAdd
// TODO(dneto): OpGroupFMin
// TODO(dneto): OpGroupUMin
// TODO(dneto): OpGroupSMin
// TODO(dneto): OpGroupFMax
// TODO(dneto): OpGroupUMax
// TODO(dneto): OpGroupSMax

using NamedMemoryBarrierTest = spvtest::TextToBinaryTest;

TEST_F(NamedMemoryBarrierTest, OpcodeUnrecognizedInV10) {
  EXPECT_THAT(CompileFailure("OpMemoryNamedBarrier %bar %scope %semantics",
                             SPV_ENV_UNIVERSAL_1_0),
              Eq("Invalid Opcode name 'OpMemoryNamedBarrier'"));
}

TEST_F(NamedMemoryBarrierTest, ArgumentCount) {
  EXPECT_THAT(CompileFailure("OpMemoryNamedBarrier", SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected operand, found end of stream."));
  EXPECT_THAT(
      CompileFailure("OpMemoryNamedBarrier %bar", SPV_ENV_UNIVERSAL_1_1),
      Eq("Expected operand, found end of stream."));
  EXPECT_THAT(
      CompileFailure("OpMemoryNamedBarrier %bar %scope", SPV_ENV_UNIVERSAL_1_1),
      Eq("Expected operand, found end of stream."));
  EXPECT_THAT(
      CompiledInstructions("OpMemoryNamedBarrier %bar %scope %semantics",
                           SPV_ENV_UNIVERSAL_1_1),
      ElementsAre(spvOpcodeMake(4, SpvOpMemoryNamedBarrier), _, _, _));
  EXPECT_THAT(
      CompileFailure("OpMemoryNamedBarrier %bar %scope %semantics %extra",
                     SPV_ENV_UNIVERSAL_1_1),
      Eq("Expected '=', found end of stream."));
}

TEST_F(NamedMemoryBarrierTest, ArgumentTypes) {
  EXPECT_THAT(CompileFailure("OpMemoryNamedBarrier 123 %scope %semantics",
                             SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected id to start with %."));
  EXPECT_THAT(CompileFailure("OpMemoryNamedBarrier %bar %scope \"semantics\"",
                             SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected id to start with %."));
}

using TypeNamedBarrierTest = spvtest::TextToBinaryTest;

TEST_F(TypeNamedBarrierTest, OpcodeUnrecognizedInV10) {
  EXPECT_THAT(CompileFailure("%t = OpTypeNamedBarrier", SPV_ENV_UNIVERSAL_1_0),
              Eq("Invalid Opcode name 'OpTypeNamedBarrier'"));
}

TEST_F(TypeNamedBarrierTest, ArgumentCount) {
  EXPECT_THAT(CompileFailure("OpTypeNamedBarrier", SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected <result-id> at the beginning of an instruction, "
                 "found 'OpTypeNamedBarrier'."));
  EXPECT_THAT(
      CompiledInstructions("%t = OpTypeNamedBarrier", SPV_ENV_UNIVERSAL_1_1),
      ElementsAre(spvOpcodeMake(2, SpvOpTypeNamedBarrier), _));
  EXPECT_THAT(
      CompileFailure("%t = OpTypeNamedBarrier 1 2 3", SPV_ENV_UNIVERSAL_1_1),
      Eq("Expected <opcode> or <result-id> at the beginning of an instruction, "
         "found '1'."));
}

using NamedBarrierInitializeTest = spvtest::TextToBinaryTest;

TEST_F(NamedBarrierInitializeTest, OpcodeUnrecognizedInV10) {
  EXPECT_THAT(CompileFailure("%bar = OpNamedBarrierInitialize %type %count",
                             SPV_ENV_UNIVERSAL_1_0),
              Eq("Invalid Opcode name 'OpNamedBarrierInitialize'"));
}

TEST_F(NamedBarrierInitializeTest, ArgumentCount) {
  EXPECT_THAT(
      CompileFailure("%bar = OpNamedBarrierInitialize", SPV_ENV_UNIVERSAL_1_1),
      Eq("Expected operand, found end of stream."));
  EXPECT_THAT(CompileFailure("%bar = OpNamedBarrierInitialize %ype",
                             SPV_ENV_UNIVERSAL_1_1),
              Eq("Expected operand, found end of stream."));
  EXPECT_THAT(
      CompiledInstructions("%bar = OpNamedBarrierInitialize %type %count",
                           SPV_ENV_UNIVERSAL_1_1),
      ElementsAre(spvOpcodeMake(4, SpvOpNamedBarrierInitialize), _, _, _));
  EXPECT_THAT(
      CompileFailure("%bar = OpNamedBarrierInitialize %type %count \"extra\"",
                     SPV_ENV_UNIVERSAL_1_1),
      Eq("Expected <opcode> or <result-id> at the beginning of an instruction, "
         "found '\"extra\"'."));
}

}  // anonymous namespace
