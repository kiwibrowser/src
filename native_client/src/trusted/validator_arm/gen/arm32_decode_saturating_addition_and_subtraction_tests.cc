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


// op(22:21)=00 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QADD_cccc00010000nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010000nnnndddd00000101mmmm,
//       rule: QADD,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QADD_cccc00010000nnnndddd00000101mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  QADD_cccc00010000nnnndddd00000101mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool QADD_cccc00010000nnnndddd00000101mmmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22:21)=~00
  if ((inst.Bits() & 0x00600000)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22:21)=01 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSUB_cccc00010010nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010010nnnndddd00000101mmmm,
//       rule: QSUB,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QSUB_cccc00010010nnnndddd00000101mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  QSUB_cccc00010010nnnndddd00000101mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool QSUB_cccc00010010nnnndddd00000101mmmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22:21)=~01
  if ((inst.Bits() & 0x00600000)  !=
          0x00200000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22:21)=10 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QDADD_cccc00010100nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010100nnnndddd00000101mmmm,
//       rule: QDADD,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QDADD_cccc00010100nnnndddd00000101mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  QDADD_cccc00010100nnnndddd00000101mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool QDADD_cccc00010100nnnndddd00000101mmmm_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22:21)=~10
  if ((inst.Bits() & 0x00600000)  !=
          0x00400000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22:21)=11 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QDSUB_cccc00010110nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010110nnnndddd00000101mmmm,
//       rule: QDSUB,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QDSUB_cccc00010110nnnndddd00000101mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  QDSUB_cccc00010110nnnndddd00000101mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool QDSUB_cccc00010110nnnndddd00000101mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22:21)=~11
  if ((inst.Bits() & 0x00600000)  !=
          0x00600000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000000) return false;

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

// op(22:21)=00 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QADD_cccc00010000nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010000nnnndddd00000101mmmm,
//       rule: QADD,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QADD_cccc00010000nnnndddd00000101mmmm_case_0Tester_Case0
    : public QADD_cccc00010000nnnndddd00000101mmmm_case_0TesterCase0 {
 public:
  QADD_cccc00010000nnnndddd00000101mmmm_case_0Tester_Case0()
    : QADD_cccc00010000nnnndddd00000101mmmm_case_0TesterCase0(
      state_.QADD_cccc00010000nnnndddd00000101mmmm_case_0_QADD_instance_)
  {}
};

// op(22:21)=01 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSUB_cccc00010010nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010010nnnndddd00000101mmmm,
//       rule: QSUB,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QSUB_cccc00010010nnnndddd00000101mmmm_case_0Tester_Case1
    : public QSUB_cccc00010010nnnndddd00000101mmmm_case_0TesterCase1 {
 public:
  QSUB_cccc00010010nnnndddd00000101mmmm_case_0Tester_Case1()
    : QSUB_cccc00010010nnnndddd00000101mmmm_case_0TesterCase1(
      state_.QSUB_cccc00010010nnnndddd00000101mmmm_case_0_QSUB_instance_)
  {}
};

// op(22:21)=10 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QDADD_cccc00010100nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010100nnnndddd00000101mmmm,
//       rule: QDADD,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QDADD_cccc00010100nnnndddd00000101mmmm_case_0Tester_Case2
    : public QDADD_cccc00010100nnnndddd00000101mmmm_case_0TesterCase2 {
 public:
  QDADD_cccc00010100nnnndddd00000101mmmm_case_0Tester_Case2()
    : QDADD_cccc00010100nnnndddd00000101mmmm_case_0TesterCase2(
      state_.QDADD_cccc00010100nnnndddd00000101mmmm_case_0_QDADD_instance_)
  {}
};

// op(22:21)=11 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QDSUB_cccc00010110nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010110nnnndddd00000101mmmm,
//       rule: QDSUB,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QDSUB_cccc00010110nnnndddd00000101mmmm_case_0Tester_Case3
    : public QDSUB_cccc00010110nnnndddd00000101mmmm_case_0TesterCase3 {
 public:
  QDSUB_cccc00010110nnnndddd00000101mmmm_case_0Tester_Case3()
    : QDSUB_cccc00010110nnnndddd00000101mmmm_case_0TesterCase3(
      state_.QDSUB_cccc00010110nnnndddd00000101mmmm_case_0_QDSUB_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op(22:21)=00 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QADD_cccc00010000nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010000nnnndddd00000101mmmm,
//       rule: QADD,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       QADD_cccc00010000nnnndddd00000101mmmm_case_0Tester_Case0_TestCase0) {
  QADD_cccc00010000nnnndddd00000101mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_QADD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010000nnnndddd00000101mmmm");
}

// op(22:21)=01 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSUB_cccc00010010nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010010nnnndddd00000101mmmm,
//       rule: QSUB,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       QSUB_cccc00010010nnnndddd00000101mmmm_case_0Tester_Case1_TestCase1) {
  QSUB_cccc00010010nnnndddd00000101mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_QSUB actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010010nnnndddd00000101mmmm");
}

// op(22:21)=10 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QDADD_cccc00010100nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010100nnnndddd00000101mmmm,
//       rule: QDADD,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       QDADD_cccc00010100nnnndddd00000101mmmm_case_0Tester_Case2_TestCase2) {
  QDADD_cccc00010100nnnndddd00000101mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_QDADD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010100nnnndddd00000101mmmm");
}

// op(22:21)=11 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QDSUB_cccc00010110nnnndddd00000101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc00010110nnnndddd00000101mmmm,
//       rule: QDSUB,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       QDSUB_cccc00010110nnnndddd00000101mmmm_case_0Tester_Case3_TestCase3) {
  QDSUB_cccc00010110nnnndddd00000101mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_QDSUB actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010110nnnndddd00000101mmmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
