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


// op1(27:20)=11000100
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 111111000100ssssttttiiiiiiiiiiii,
//       rule: MCRR2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(27:20)=~11000100
  if ((inst.Bits() & 0x0FF00000)  !=
          0x0C400000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(27:20)=11000101
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 111111000101ssssttttiiiiiiiiiiii,
//       rule: MRRC2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(27:20)=~11000101
  if ((inst.Bits() & 0x0FF00000)  !=
          0x0C500000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(27:20)=100xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000101000000000
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: RFE_1111100pu0w1nnnn0000101000000000_case_0,
//       defs: {},
//       pattern: 1111100pu0w1nnnn0000101000000000,
//       rule: RFE,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class RFE_1111100pu0w1nnnn0000101000000000_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  RFE_1111100pu0w1nnnn0000101000000000_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool RFE_1111100pu0w1nnnn0000101000000000_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(27:20)=~100xx0x1
  if ((inst.Bits() & 0x0E500000)  !=
          0x08100000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000101000000000
  if ((inst.Bits() & 0x0000FFFF)  !=
          0x00000A00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(27:20)=100xx1x0 & $pattern(31:0)=xxxxxxxxxxxx110100000101000xxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SRS_1111100pu1w0110100000101000iiiii_case_0,
//       defs: {},
//       pattern: 1111100pu1w0110100000101000iiiii,
//       rule: SRS,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class SRS_1111100pu1w0110100000101000iiiii_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  SRS_1111100pu1w0110100000101000iiiii_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SRS_1111100pu1w0110100000101000iiiii_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(27:20)=~100xx1x0
  if ((inst.Bits() & 0x0E500000)  !=
          0x08400000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx110100000101000xxxxx
  if ((inst.Bits() & 0x000FFFE0)  !=
          0x000D0500) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(27:20)=1110xxx0 & op(4)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0,
//       defs: {},
//       pattern: 11111110iii0iiiittttiiiiiii1iiii,
//       rule: MCR2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(27:20)=~1110xxx0
  if ((inst.Bits() & 0x0F100000)  !=
          0x0E000000) return false;
  // op(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(27:20)=1110xxx1 & op(4)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0,
//       defs: {},
//       pattern: 11111110iii1iiiittttiiiiiii1iiii,
//       rule: MRC2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(27:20)=~1110xxx1
  if ((inst.Bits() & 0x0F100000)  !=
          0x0E100000) return false;
  // op(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(27:20)=110xxxx0 & op1_repeated(27:20)=~11000x00
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111110pudw0nnnniiiiiiiiiiiiiiii,
//       rule: STC2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(27:20)=~110xxxx0
  if ((inst.Bits() & 0x0E100000)  !=
          0x0C000000) return false;
  // op1_repeated(27:20)=11000x00
  if ((inst.Bits() & 0x0FB00000)  ==
          0x0C000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(27:20)=110xxxx1 & Rn(19:16)=~1111 & op1_repeated(27:20)=~11000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111110pudw1nnnniiiiiiiiiiiiiiii,
//       rule: LDC2_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(27:20)=~110xxxx1
  if ((inst.Bits() & 0x0E100000)  !=
          0x0C100000) return false;
  // Rn(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // op1_repeated(27:20)=11000x01
  if ((inst.Bits() & 0x0FB00000)  ==
          0x0C100000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(27:20)=110xxxx1 & Rn(19:16)=1111 & op1_repeated(27:20)=~11000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111110pudw11111iiiiiiiiiiiiiiii,
//       rule: LDC2_literal,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(27:20)=~110xxxx1
  if ((inst.Bits() & 0x0E100000)  !=
          0x0C100000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // op1_repeated(27:20)=11000x01
  if ((inst.Bits() & 0x0FB00000)  ==
          0x0C100000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(27:20)=1110xxxx & op(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0,
//       defs: {},
//       pattern: 11111110iiiiiiiiiiiiiiiiiii0iiii,
//       rule: CDP2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(27:20)=~1110xxxx
  if ((inst.Bits() & 0x0F000000)  !=
          0x0E000000) return false;
  // op(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(27:20)=101xxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111101hiiiiiiiiiiiiiiiiiiiiiiii,
//       rule: BLX_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(27:20)=~101xxxxx
  if ((inst.Bits() & 0x0E000000)  !=
          0x0A000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// op1(27:20)=11000100
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 111111000100ssssttttiiiiiiiiiiii,
//       rule: MCRR2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0Tester_Case0
    : public MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0TesterCase0 {
 public:
  MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0Tester_Case0()
    : MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0TesterCase0(
      state_.MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0_MCRR2_instance_)
  {}
};

// op1(27:20)=11000101
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 111111000101ssssttttiiiiiiiiiiii,
//       rule: MRRC2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0Tester_Case1
    : public MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0TesterCase1 {
 public:
  MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0Tester_Case1()
    : MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0TesterCase1(
      state_.MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0_MRRC2_instance_)
  {}
};

// op1(27:20)=100xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000101000000000
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: RFE_1111100pu0w1nnnn0000101000000000_case_0,
//       defs: {},
//       pattern: 1111100pu0w1nnnn0000101000000000,
//       rule: RFE,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class RFE_1111100pu0w1nnnn0000101000000000_case_0Tester_Case2
    : public RFE_1111100pu0w1nnnn0000101000000000_case_0TesterCase2 {
 public:
  RFE_1111100pu0w1nnnn0000101000000000_case_0Tester_Case2()
    : RFE_1111100pu0w1nnnn0000101000000000_case_0TesterCase2(
      state_.RFE_1111100pu0w1nnnn0000101000000000_case_0_RFE_instance_)
  {}
};

// op1(27:20)=100xx1x0 & $pattern(31:0)=xxxxxxxxxxxx110100000101000xxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SRS_1111100pu1w0110100000101000iiiii_case_0,
//       defs: {},
//       pattern: 1111100pu1w0110100000101000iiiii,
//       rule: SRS,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class SRS_1111100pu1w0110100000101000iiiii_case_0Tester_Case3
    : public SRS_1111100pu1w0110100000101000iiiii_case_0TesterCase3 {
 public:
  SRS_1111100pu1w0110100000101000iiiii_case_0Tester_Case3()
    : SRS_1111100pu1w0110100000101000iiiii_case_0TesterCase3(
      state_.SRS_1111100pu1w0110100000101000iiiii_case_0_SRS_instance_)
  {}
};

// op1(27:20)=1110xxx0 & op(4)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0,
//       defs: {},
//       pattern: 11111110iii0iiiittttiiiiiii1iiii,
//       rule: MCR2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0Tester_Case4
    : public MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0TesterCase4 {
 public:
  MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0Tester_Case4()
    : MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0TesterCase4(
      state_.MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0_MCR2_instance_)
  {}
};

// op1(27:20)=1110xxx1 & op(4)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0,
//       defs: {},
//       pattern: 11111110iii1iiiittttiiiiiii1iiii,
//       rule: MRC2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0Tester_Case5
    : public MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0TesterCase5 {
 public:
  MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0Tester_Case5()
    : MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0TesterCase5(
      state_.MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0_MRC2_instance_)
  {}
};

// op1(27:20)=110xxxx0 & op1_repeated(27:20)=~11000x00
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111110pudw0nnnniiiiiiiiiiiiiiii,
//       rule: STC2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0Tester_Case6
    : public STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0TesterCase6 {
 public:
  STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0Tester_Case6()
    : STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0TesterCase6(
      state_.STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0_STC2_instance_)
  {}
};

// op1(27:20)=110xxxx1 & Rn(19:16)=~1111 & op1_repeated(27:20)=~11000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111110pudw1nnnniiiiiiiiiiiiiiii,
//       rule: LDC2_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0Tester_Case7
    : public LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0TesterCase7 {
 public:
  LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0Tester_Case7()
    : LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0TesterCase7(
      state_.LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0_LDC2_immediate_instance_)
  {}
};

// op1(27:20)=110xxxx1 & Rn(19:16)=1111 & op1_repeated(27:20)=~11000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111110pudw11111iiiiiiiiiiiiiiii,
//       rule: LDC2_literal,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0Tester_Case8
    : public LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0TesterCase8 {
 public:
  LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0Tester_Case8()
    : LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0TesterCase8(
      state_.LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0_LDC2_literal_instance_)
  {}
};

// op1(27:20)=1110xxxx & op(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0,
//       defs: {},
//       pattern: 11111110iiiiiiiiiiiiiiiiiii0iiii,
//       rule: CDP2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0Tester_Case9
    : public CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0TesterCase9 {
 public:
  CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0Tester_Case9()
    : CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0TesterCase9(
      state_.CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0_CDP2_instance_)
  {}
};

// op1(27:20)=101xxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111101hiiiiiiiiiiiiiiiiiiiiiiii,
//       rule: BLX_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case10
    : public BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase10 {
 public:
  BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case10()
    : BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase10(
      state_.BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0_BLX_immediate_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op1(27:20)=11000100
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 111111000100ssssttttiiiiiiiiiiii,
//       rule: MCRR2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0Tester_Case0_TestCase0) {
  MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0Tester_Case0 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_MCRR2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111111000100ssssttttiiiiiiiiiiii");
}

// op1(27:20)=11000101
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 111111000101ssssttttiiiiiiiiiiii,
//       rule: MRRC2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0Tester_Case1_TestCase1) {
  MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0Tester_Case1 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_MRRC2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111111000101ssssttttiiiiiiiiiiii");
}

// op1(27:20)=100xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000101000000000
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: RFE_1111100pu0w1nnnn0000101000000000_case_0,
//       defs: {},
//       pattern: 1111100pu0w1nnnn0000101000000000,
//       rule: RFE,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       RFE_1111100pu0w1nnnn0000101000000000_case_0Tester_Case2_TestCase2) {
  RFE_1111100pu0w1nnnn0000101000000000_case_0Tester_Case2 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_RFE actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111100pu0w1nnnn0000101000000000");
}

// op1(27:20)=100xx1x0 & $pattern(31:0)=xxxxxxxxxxxx110100000101000xxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SRS_1111100pu1w0110100000101000iiiii_case_0,
//       defs: {},
//       pattern: 1111100pu1w0110100000101000iiiii,
//       rule: SRS,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       SRS_1111100pu1w0110100000101000iiiii_case_0Tester_Case3_TestCase3) {
  SRS_1111100pu1w0110100000101000iiiii_case_0Tester_Case3 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_SRS actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111100pu1w0110100000101000iiiii");
}

// op1(27:20)=1110xxx0 & op(4)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0,
//       defs: {},
//       pattern: 11111110iii0iiiittttiiiiiii1iiii,
//       rule: MCR2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0Tester_Case4_TestCase4) {
  MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0Tester_Case4 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_MCR2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11111110iii0iiiittttiiiiiii1iiii");
}

// op1(27:20)=1110xxx1 & op(4)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0,
//       defs: {},
//       pattern: 11111110iii1iiiittttiiiiiii1iiii,
//       rule: MRC2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0Tester_Case5_TestCase5) {
  MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0Tester_Case5 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_MRC2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11111110iii1iiiittttiiiiiii1iiii");
}

// op1(27:20)=110xxxx0 & op1_repeated(27:20)=~11000x00
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111110pudw0nnnniiiiiiiiiiiiiiii,
//       rule: STC2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0Tester_Case6_TestCase6) {
  STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0Tester_Case6 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_STC2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111110pudw0nnnniiiiiiiiiiiiiiii");
}

// op1(27:20)=110xxxx1 & Rn(19:16)=~1111 & op1_repeated(27:20)=~11000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111110pudw1nnnniiiiiiiiiiiiiiii,
//       rule: LDC2_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0Tester_Case7_TestCase7) {
  LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0Tester_Case7 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_LDC2_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111110pudw1nnnniiiiiiiiiiiiiiii");
}

// op1(27:20)=110xxxx1 & Rn(19:16)=1111 & op1_repeated(27:20)=~11000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111110pudw11111iiiiiiiiiiiiiiii,
//       rule: LDC2_literal,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0Tester_Case8_TestCase8) {
  LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0Tester_Case8 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_LDC2_literal actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111110pudw11111iiiiiiiiiiiiiiii");
}

// op1(27:20)=1110xxxx & op(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0,
//       defs: {},
//       pattern: 11111110iiiiiiiiiiiiiiiiiii0iiii,
//       rule: CDP2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0Tester_Case9_TestCase9) {
  CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0Tester_Case9 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_CDP2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11111110iiiiiiiiiiiiiiiiiii0iiii");
}

// op1(27:20)=101xxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: 1111101hiiiiiiiiiiiiiiiiiiiiiiii,
//       rule: BLX_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case10_TestCase10) {
  BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case10 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_BLX_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111101hiiiiiiiiiiiiiiiiiiiiiiii");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
