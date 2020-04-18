/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif


#include "gtest/gtest.h"
#include "native_client/src/trusted/validator_arm/actual_vs_baseline.h"
#include "native_client/src/trusted/validator_arm/arm_helpers.h"
#include "native_client/src/trusted/validator_arm/gen/arm32_decode_named_bases.h"

using nacl_arm_dec::Instruction;
using nacl_arm_dec::ClassDecoder;
using nacl_arm_dec::Register;
using nacl_arm_dec::RegisterList;

namespace nacl_arm_test {

// The following classes are derived class decoder testers that
// add row pattern constraints and decoder restrictions to each tester.
// This is done so that it can be used to make sure that the
// corresponding pattern is not tested for cases that would be excluded
//  due to row checks, or restrictions specified by the row restrictions.


// op(25)=0 & op1(24:20)=0xx1x & op2(7:4)=1011
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0,
//       defs: {},
//       pattern: cccc0000xx1xxxxxxxxxxxxx1xx1xxxx,
//       rule: extra_load_store_instructions_unpriviledged,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~0xx1x
  if ((inst.Bits() & 0x01200000)  !=
          0x00200000) return false;
  // op2(7:4)=~1011
  if ((inst.Bits() & 0x000000F0)  !=
          0x000000B0) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25)=0 & op1(24:20)=0xx1x & op2(7:4)=11x1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0,
//       defs: {},
//       pattern: cccc0000xx1xxxxxxxxxxxxx1xx1xxxx,
//       rule: extra_load_store_instructions_unpriviledged,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~0xx1x
  if ((inst.Bits() & 0x01200000)  !=
          0x00200000) return false;
  // op2(7:4)=~11x1
  if ((inst.Bits() & 0x000000D0)  !=
          0x000000D0) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25)=1 & op1(24:20)=10000
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1,
//       baseline: MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if S
//            else None},
//       dynamic_code_replace_immediates: {imm4, imm12},
//       fields: [S(20), imm4(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm4: imm4(19:16),
//       pattern: cccc00110000iiiiddddiiiiiiiiiiii,
//       rule: MOVW,
//       safety: [Rd(15:12)=1111 => UNPREDICTABLE],
//       uses: {}}
class MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25)=~1
  if ((inst.Bits() & 0x02000000)  !=
          0x02000000) return false;
  // op1(24:20)=~10000
  if ((inst.Bits() & 0x01F00000)  !=
          0x01000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25)=1 & op1(24:20)=10100
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1,
//       baseline: MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if S
//            else None},
//       dynamic_code_replace_immediates: {imm4, imm12},
//       fields: [S(20), imm4(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm4: imm4(19:16),
//       pattern: cccc00110100iiiiddddiiiiiiiiiiii,
//       rule: MOVT,
//       safety: [Rd(15:12)=1111 => UNPREDICTABLE],
//       uses: {}}
class MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25)=~1
  if ((inst.Bits() & 0x02000000)  !=
          0x02000000) return false;
  // op1(24:20)=~10100
  if ((inst.Bits() & 0x01F00000)  !=
          0x01400000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// op(25)=0 & op1(24:20)=0xx1x & op2(7:4)=1011
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0,
//       defs: {},
//       pattern: cccc0000xx1xxxxxxxxxxxxx1xx1xxxx,
//       rule: extra_load_store_instructions_unpriviledged,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0Tester_Case0
    : public extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0TesterCase0 {
 public:
  extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0Tester_Case0()
    : extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0TesterCase0(
      state_.extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0_extra_load_store_instructions_unpriviledged_instance_)
  {}
};

// op(25)=0 & op1(24:20)=0xx1x & op2(7:4)=11x1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0,
//       defs: {},
//       pattern: cccc0000xx1xxxxxxxxxxxxx1xx1xxxx,
//       rule: extra_load_store_instructions_unpriviledged,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0Tester_Case1
    : public extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0TesterCase1 {
 public:
  extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0Tester_Case1()
    : extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0TesterCase1(
      state_.extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0_extra_load_store_instructions_unpriviledged_instance_)
  {}
};

// op(25)=1 & op1(24:20)=10000
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1,
//       baseline: MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if S
//            else None},
//       dynamic_code_replace_immediates: {imm4, imm12},
//       fields: [S(20), imm4(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm4: imm4(19:16),
//       pattern: cccc00110000iiiiddddiiiiiiiiiiii,
//       rule: MOVW,
//       safety: [Rd(15:12)=1111 => UNPREDICTABLE],
//       uses: {}}
class MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0Tester_Case2
    : public MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0TesterCase2 {
 public:
  MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0Tester_Case2()
    : MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0TesterCase2(
      state_.MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0_MOVW_instance_)
  {}
};

// op(25)=1 & op1(24:20)=10100
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1,
//       baseline: MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if S
//            else None},
//       dynamic_code_replace_immediates: {imm4, imm12},
//       fields: [S(20), imm4(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm4: imm4(19:16),
//       pattern: cccc00110100iiiiddddiiiiiiiiiiii,
//       rule: MOVT,
//       safety: [Rd(15:12)=1111 => UNPREDICTABLE],
//       uses: {}}
class MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0Tester_Case3
    : public MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0TesterCase3 {
 public:
  MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0Tester_Case3()
    : MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0TesterCase3(
      state_.MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0_MOVT_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op(25)=0 & op1(24:20)=0xx1x & op2(7:4)=1011
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0,
//       defs: {},
//       pattern: cccc0000xx1xxxxxxxxxxxxx1xx1xxxx,
//       rule: extra_load_store_instructions_unpriviledged,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0Tester_Case0_TestCase0) {
  extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0Tester_Case0 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_extra_load_store_instructions_unpriviledged actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000xx1xxxxxxxxxxxxx1xx1xxxx");
}

// op(25)=0 & op1(24:20)=0xx1x & op2(7:4)=11x1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0,
//       defs: {},
//       pattern: cccc0000xx1xxxxxxxxxxxxx1xx1xxxx,
//       rule: extra_load_store_instructions_unpriviledged,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0Tester_Case1_TestCase1) {
  extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0Tester_Case1 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_extra_load_store_instructions_unpriviledged actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000xx1xxxxxxxxxxxxx1xx1xxxx");
}

// op(25)=1 & op1(24:20)=10000
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1,
//       baseline: MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if S
//            else None},
//       dynamic_code_replace_immediates: {imm4, imm12},
//       fields: [S(20), imm4(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm4: imm4(19:16),
//       pattern: cccc00110000iiiiddddiiiiiiiiiiii,
//       rule: MOVW,
//       safety: [Rd(15:12)=1111 => UNPREDICTABLE],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0Tester_Case2_TestCase2) {
  MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0Tester_Case2 baseline_tester;
  NamedActual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1_MOVW actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00110000iiiiddddiiiiiiiiiiii");
}

// op(25)=1 & op1(24:20)=10100
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1,
//       baseline: MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if S
//            else None},
//       dynamic_code_replace_immediates: {imm4, imm12},
//       fields: [S(20), imm4(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm4: imm4(19:16),
//       pattern: cccc00110100iiiiddddiiiiiiiiiiii,
//       rule: MOVT,
//       safety: [Rd(15:12)=1111 => UNPREDICTABLE],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0Tester_Case3_TestCase3) {
  MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0Tester_Case3 baseline_tester;
  NamedActual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1_MOVT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00110100iiiiddddiiiiiiiiiiii");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
