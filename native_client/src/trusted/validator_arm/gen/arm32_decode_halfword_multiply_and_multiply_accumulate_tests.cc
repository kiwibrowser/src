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


// op1(22:21)=00
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//       baseline: SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010000ddddaaaammmm1xx0nnnn,
//       rule: SMLABB_SMLABT_SMLATB_SMLATT,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:21)=~00
  if ((inst.Bits() & 0x00600000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:21)=01 & op(5)=0
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//       baseline: SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010010ddddaaaammmm1x00nnnn,
//       rule: SMLAWB_SMLAWT,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:21)=~01
  if ((inst.Bits() & 0x00600000)  !=
          0x00200000) return false;
  // op(5)=~0
  if ((inst.Bits() & 0x00000020)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:21)=01 & op(5)=1 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//       baseline: SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010010dddd0000mmmm1x10nnnn,
//       rule: SMULWB_SMULWT,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:21)=~01
  if ((inst.Bits() & 0x00600000)  !=
          0x00200000) return false;
  // op(5)=~1
  if ((inst.Bits() & 0x00000020)  !=
          0x00000020) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:21)=10
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1,
//       baseline: SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0,
//       defs: {RdLo, RdHi},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010100hhhhllllmmmm1xx0nnnn,
//       rule: SMLALBB_SMLALBT_SMLALTB_SMLALTT,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdLo, RdHi, Rn, Rm}}
class SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:21)=~10
  if ((inst.Bits() & 0x00600000)  !=
          0x00400000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:21)=11 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//       baseline: SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010110dddd0000mmmm1xx0nnnn,
//       rule: SMULBB_SMULBT_SMULTB_SMULTT,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:21)=~11
  if ((inst.Bits() & 0x00600000)  !=
          0x00600000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
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

// op1(22:21)=00
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//       baseline: SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010000ddddaaaammmm1xx0nnnn,
//       rule: SMLABB_SMLABT_SMLATB_SMLATT,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0Tester_Case0
    : public SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0TesterCase0 {
 public:
  SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0Tester_Case0()
    : SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0TesterCase0(
      state_.SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0_SMLABB_SMLABT_SMLATB_SMLATT_instance_)
  {}
};

// op1(22:21)=01 & op(5)=0
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//       baseline: SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010010ddddaaaammmm1x00nnnn,
//       rule: SMLAWB_SMLAWT,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0Tester_Case1
    : public SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0TesterCase1 {
 public:
  SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0Tester_Case1()
    : SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0TesterCase1(
      state_.SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0_SMLAWB_SMLAWT_instance_)
  {}
};

// op1(22:21)=01 & op(5)=1 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//       baseline: SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010010dddd0000mmmm1x10nnnn,
//       rule: SMULWB_SMULWT,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0Tester_Case2
    : public SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0TesterCase2 {
 public:
  SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0Tester_Case2()
    : SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0TesterCase2(
      state_.SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0_SMULWB_SMULWT_instance_)
  {}
};

// op1(22:21)=10
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1,
//       baseline: SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0,
//       defs: {RdLo, RdHi},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010100hhhhllllmmmm1xx0nnnn,
//       rule: SMLALBB_SMLALBT_SMLALTB_SMLALTT,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdLo, RdHi, Rn, Rm}}
class SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0Tester_Case3
    : public SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0TesterCase3 {
 public:
  SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0Tester_Case3()
    : SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0TesterCase3(
      state_.SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0_SMLALBB_SMLALBT_SMLALTB_SMLALTT_instance_)
  {}
};

// op1(22:21)=11 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//       baseline: SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010110dddd0000mmmm1xx0nnnn,
//       rule: SMULBB_SMULBT_SMULTB_SMULTT,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0Tester_Case4
    : public SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0TesterCase4 {
 public:
  SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0Tester_Case4()
    : SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0TesterCase4(
      state_.SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0_SMULBB_SMULBT_SMULTB_SMULTT_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op1(22:21)=00
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//       baseline: SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010000ddddaaaammmm1xx0nnnn,
//       rule: SMLABB_SMLABT_SMLATB_SMLATT,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
TEST_F(Arm32DecoderStateTests,
       SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0Tester_Case0_TestCase0) {
  SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0Tester_Case0 baseline_tester;
  NamedActual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1_SMLABB_SMLABT_SMLATB_SMLATT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010000ddddaaaammmm1xx0nnnn");
}

// op1(22:21)=01 & op(5)=0
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//       baseline: SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010010ddddaaaammmm1x00nnnn,
//       rule: SMLAWB_SMLAWT,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
TEST_F(Arm32DecoderStateTests,
       SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0Tester_Case1_TestCase1) {
  SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0Tester_Case1 baseline_tester;
  NamedActual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1_SMLAWB_SMLAWT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010010ddddaaaammmm1x00nnnn");
}

// op1(22:21)=01 & op(5)=1 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//       baseline: SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010010dddd0000mmmm1x10nnnn,
//       rule: SMULWB_SMULWT,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0Tester_Case2_TestCase2) {
  SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0Tester_Case2 baseline_tester;
  NamedActual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1_SMULWB_SMULWT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010010dddd0000mmmm1x10nnnn");
}

// op1(22:21)=10
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1,
//       baseline: SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0,
//       defs: {RdLo, RdHi},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010100hhhhllllmmmm1xx0nnnn,
//       rule: SMLALBB_SMLALBT_SMLALTB_SMLALTT,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdLo, RdHi, Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0Tester_Case3_TestCase3) {
  SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0Tester_Case3 baseline_tester;
  NamedActual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1_SMLALBB_SMLALBT_SMLALTB_SMLALTT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010100hhhhllllmmmm1xx0nnnn");
}

// op1(22:21)=11 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//       baseline: SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc00010110dddd0000mmmm1xx0nnnn,
//       rule: SMULBB_SMULBT_SMULTB_SMULTT,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0Tester_Case4_TestCase4) {
  SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0Tester_Case4 baseline_tester;
  NamedActual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1_SMULBB_SMULBT_SMULTB_SMULTT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010110dddd0000mmmm1xx0nnnn");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
