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


// coproc(11:8)=~101x & op1(25:20)=000100
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MCRR_cccc11000100ttttttttccccoooommmm_case_0,
//       defs: {},
//       pattern: cccc11000100ttttttttccccoooommmm,
//       rule: MCRR,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MCRR_cccc11000100ttttttttccccoooommmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  MCRR_cccc11000100ttttttttccccoooommmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MCRR_cccc11000100ttttttttccccoooommmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // coproc(11:8)=101x
  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00) return false;
  // op1(25:20)=~000100
  if ((inst.Bits() & 0x03F00000)  !=
          0x00400000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// coproc(11:8)=~101x & op1(25:20)=000101
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRRC_cccc11000101ttttttttccccoooommmm_case_0,
//       defs: {},
//       pattern: cccc11000101ttttttttccccoooommmm,
//       rule: MRRC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MRRC_cccc11000101ttttttttccccoooommmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  MRRC_cccc11000101ttttttttccccoooommmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MRRC_cccc11000101ttttttttccccoooommmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // coproc(11:8)=101x
  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00) return false;
  // op1(25:20)=~000101
  if ((inst.Bits() & 0x03F00000)  !=
          0x00500000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// coproc(11:8)=~101x & op1(25:20)=10xxx0 & op(4)=1
//    = {actual: Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1,
//       baseline: MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0,
//       defs: {},
//       diagnostics: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//        error('Consider using DSB (defined in ARMv7) for memory barrier')],
//       pattern: cccc1110ooo0nnnnttttccccooo1mmmm,
//       rule: MCR,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {},
//       violations: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//        error('Consider using DSB (defined in ARMv7) for memory barrier')]}
class MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // coproc(11:8)=101x
  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00) return false;
  // op1(25:20)=~10xxx0
  if ((inst.Bits() & 0x03100000)  !=
          0x02000000) return false;
  // op(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// coproc(11:8)=~101x & op1(25:20)=10xxx1 & op(4)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0,
//       defs: {},
//       pattern: cccc1110ooo1nnnnttttccccooo1mmmm,
//       rule: MRC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // coproc(11:8)=101x
  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00) return false;
  // op1(25:20)=~10xxx1
  if ((inst.Bits() & 0x03100000)  !=
          0x02100000) return false;
  // op(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// coproc(11:8)=~101x & op1(25:20)=0xxxx0 & op1_repeated(25:20)=~000x00
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0,
//       defs: {},
//       pattern: cccc110pudw0nnnnddddcccciiiiiiii,
//       rule: STC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // coproc(11:8)=101x
  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00) return false;
  // op1(25:20)=~0xxxx0
  if ((inst.Bits() & 0x02100000)  !=
          0x00000000) return false;
  // op1_repeated(25:20)=000x00
  if ((inst.Bits() & 0x03B00000)  ==
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// coproc(11:8)=~101x & op1(25:20)=0xxxx1 & Rn(19:16)=~1111 & op1_repeated(25:20)=~000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0,
//       defs: {},
//       pattern: cccc110pudw1nnnnddddcccciiiiiiii,
//       rule: LDC_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // coproc(11:8)=101x
  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00) return false;
  // op1(25:20)=~0xxxx1
  if ((inst.Bits() & 0x02100000)  !=
          0x00100000) return false;
  // Rn(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // op1_repeated(25:20)=000x01
  if ((inst.Bits() & 0x03B00000)  ==
          0x00100000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// coproc(11:8)=~101x & op1(25:20)=0xxxx1 & Rn(19:16)=1111 & op1_repeated(25:20)=~000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0,
//       defs: {},
//       pattern: cccc110pudw11111ddddcccciiiiiiii,
//       rule: LDC_literal,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // coproc(11:8)=101x
  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00) return false;
  // op1(25:20)=~0xxxx1
  if ((inst.Bits() & 0x02100000)  !=
          0x00100000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // op1_repeated(25:20)=000x01
  if ((inst.Bits() & 0x03B00000)  ==
          0x00100000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// coproc(11:8)=~101x & op1(25:20)=10xxxx & op(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0,
//       defs: {},
//       pattern: cccc1110oooonnnnddddccccooo0mmmm,
//       rule: CDP,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // coproc(11:8)=101x
  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00) return false;
  // op1(25:20)=~10xxxx
  if ((inst.Bits() & 0x03000000)  !=
          0x02000000) return false;
  // op(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(25:20)=00000x
//    = {actual: Actual_Unnamed_case_1,
//       baseline: Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0,
//       defs: {},
//       pattern: cccc1100000xnnnnxxxxccccxxxoxxxx,
//       safety: [true => UNDEFINED],
//       true: true,
//       uses: {}}
class Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(25:20)=~00000x
  if ((inst.Bits() & 0x03E00000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(25:20)=11xxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc1111iiiiiiiiiiiiiiiiiiiiiiii,
//       rule: SVC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(25:20)=~11xxxx
  if ((inst.Bits() & 0x03000000)  !=
          0x03000000) return false;

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

// coproc(11:8)=~101x & op1(25:20)=000100
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MCRR_cccc11000100ttttttttccccoooommmm_case_0,
//       defs: {},
//       pattern: cccc11000100ttttttttccccoooommmm,
//       rule: MCRR,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MCRR_cccc11000100ttttttttccccoooommmm_case_0Tester_Case0
    : public MCRR_cccc11000100ttttttttccccoooommmm_case_0TesterCase0 {
 public:
  MCRR_cccc11000100ttttttttccccoooommmm_case_0Tester_Case0()
    : MCRR_cccc11000100ttttttttccccoooommmm_case_0TesterCase0(
      state_.MCRR_cccc11000100ttttttttccccoooommmm_case_0_MCRR_instance_)
  {}
};

// coproc(11:8)=~101x & op1(25:20)=000101
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRRC_cccc11000101ttttttttccccoooommmm_case_0,
//       defs: {},
//       pattern: cccc11000101ttttttttccccoooommmm,
//       rule: MRRC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MRRC_cccc11000101ttttttttccccoooommmm_case_0Tester_Case1
    : public MRRC_cccc11000101ttttttttccccoooommmm_case_0TesterCase1 {
 public:
  MRRC_cccc11000101ttttttttccccoooommmm_case_0Tester_Case1()
    : MRRC_cccc11000101ttttttttccccoooommmm_case_0TesterCase1(
      state_.MRRC_cccc11000101ttttttttccccoooommmm_case_0_MRRC_instance_)
  {}
};

// coproc(11:8)=~101x & op1(25:20)=10xxx0 & op(4)=1
//    = {actual: Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1,
//       baseline: MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0,
//       defs: {},
//       diagnostics: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//        error('Consider using DSB (defined in ARMv7) for memory barrier')],
//       pattern: cccc1110ooo0nnnnttttccccooo1mmmm,
//       rule: MCR,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {},
//       violations: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//        error('Consider using DSB (defined in ARMv7) for memory barrier')]}
class MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0Tester_Case2
    : public MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0TesterCase2 {
 public:
  MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0Tester_Case2()
    : MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0TesterCase2(
      state_.MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0_MCR_instance_)
  {}
};

// coproc(11:8)=~101x & op1(25:20)=10xxx1 & op(4)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0,
//       defs: {},
//       pattern: cccc1110ooo1nnnnttttccccooo1mmmm,
//       rule: MRC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0Tester_Case3
    : public MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0TesterCase3 {
 public:
  MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0Tester_Case3()
    : MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0TesterCase3(
      state_.MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0_MRC_instance_)
  {}
};

// coproc(11:8)=~101x & op1(25:20)=0xxxx0 & op1_repeated(25:20)=~000x00
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0,
//       defs: {},
//       pattern: cccc110pudw0nnnnddddcccciiiiiiii,
//       rule: STC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0Tester_Case4
    : public STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0TesterCase4 {
 public:
  STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0Tester_Case4()
    : STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0TesterCase4(
      state_.STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0_STC_instance_)
  {}
};

// coproc(11:8)=~101x & op1(25:20)=0xxxx1 & Rn(19:16)=~1111 & op1_repeated(25:20)=~000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0,
//       defs: {},
//       pattern: cccc110pudw1nnnnddddcccciiiiiiii,
//       rule: LDC_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0Tester_Case5
    : public LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0TesterCase5 {
 public:
  LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0Tester_Case5()
    : LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0TesterCase5(
      state_.LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0_LDC_immediate_instance_)
  {}
};

// coproc(11:8)=~101x & op1(25:20)=0xxxx1 & Rn(19:16)=1111 & op1_repeated(25:20)=~000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0,
//       defs: {},
//       pattern: cccc110pudw11111ddddcccciiiiiiii,
//       rule: LDC_literal,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0Tester_Case6
    : public LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0TesterCase6 {
 public:
  LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0Tester_Case6()
    : LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0TesterCase6(
      state_.LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0_LDC_literal_instance_)
  {}
};

// coproc(11:8)=~101x & op1(25:20)=10xxxx & op(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0,
//       defs: {},
//       pattern: cccc1110oooonnnnddddccccooo0mmmm,
//       rule: CDP,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0Tester_Case7
    : public CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0TesterCase7 {
 public:
  CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0Tester_Case7()
    : CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0TesterCase7(
      state_.CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0_CDP_instance_)
  {}
};

// op1(25:20)=00000x
//    = {actual: Actual_Unnamed_case_1,
//       baseline: Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0,
//       defs: {},
//       pattern: cccc1100000xnnnnxxxxccccxxxoxxxx,
//       safety: [true => UNDEFINED],
//       true: true,
//       uses: {}}
class Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0Tester_Case8
    : public Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0TesterCase8 {
 public:
  Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0Tester_Case8()
    : Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0TesterCase8(
      state_.Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0_None_instance_)
  {}
};

// op1(25:20)=11xxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc1111iiiiiiiiiiiiiiiiiiiiiiii,
//       rule: SVC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case9
    : public SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase9 {
 public:
  SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case9()
    : SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase9(
      state_.SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0_SVC_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// coproc(11:8)=~101x & op1(25:20)=000100
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MCRR_cccc11000100ttttttttccccoooommmm_case_0,
//       defs: {},
//       pattern: cccc11000100ttttttttccccoooommmm,
//       rule: MCRR,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MCRR_cccc11000100ttttttttccccoooommmm_case_0Tester_Case0_TestCase0) {
  MCRR_cccc11000100ttttttttccccoooommmm_case_0Tester_Case0 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_MCRR actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11000100ttttttttccccoooommmm");
}

// coproc(11:8)=~101x & op1(25:20)=000101
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRRC_cccc11000101ttttttttccccoooommmm_case_0,
//       defs: {},
//       pattern: cccc11000101ttttttttccccoooommmm,
//       rule: MRRC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MRRC_cccc11000101ttttttttccccoooommmm_case_0Tester_Case1_TestCase1) {
  MRRC_cccc11000101ttttttttccccoooommmm_case_0Tester_Case1 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_MRRC actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11000101ttttttttccccoooommmm");
}

// coproc(11:8)=~101x & op1(25:20)=10xxx0 & op(4)=1
//    = {actual: Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1,
//       baseline: MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0,
//       defs: {},
//       diagnostics: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//        error('Consider using DSB (defined in ARMv7) for memory barrier')],
//       pattern: cccc1110ooo0nnnnttttccccooo1mmmm,
//       rule: MCR,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {},
//       violations: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//        error('Consider using DSB (defined in ARMv7) for memory barrier')]}
TEST_F(Arm32DecoderStateTests,
       MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0Tester_Case2_TestCase2) {
  MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1_MCR actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc1110ooo0nnnnttttccccooo1mmmm");
}

// coproc(11:8)=~101x & op1(25:20)=10xxx1 & op(4)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0,
//       defs: {},
//       pattern: cccc1110ooo1nnnnttttccccooo1mmmm,
//       rule: MRC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0Tester_Case3_TestCase3) {
  MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_MRC actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc1110ooo1nnnnttttccccooo1mmmm");
}

// coproc(11:8)=~101x & op1(25:20)=0xxxx0 & op1_repeated(25:20)=~000x00
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0,
//       defs: {},
//       pattern: cccc110pudw0nnnnddddcccciiiiiiii,
//       rule: STC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0Tester_Case4_TestCase4) {
  STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0Tester_Case4 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_STC actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw0nnnnddddcccciiiiiiii");
}

// coproc(11:8)=~101x & op1(25:20)=0xxxx1 & Rn(19:16)=~1111 & op1_repeated(25:20)=~000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0,
//       defs: {},
//       pattern: cccc110pudw1nnnnddddcccciiiiiiii,
//       rule: LDC_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0Tester_Case5_TestCase5) {
  LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0Tester_Case5 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_LDC_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw1nnnnddddcccciiiiiiii");
}

// coproc(11:8)=~101x & op1(25:20)=0xxxx1 & Rn(19:16)=1111 & op1_repeated(25:20)=~000x01
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0,
//       defs: {},
//       pattern: cccc110pudw11111ddddcccciiiiiiii,
//       rule: LDC_literal,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0Tester_Case6_TestCase6) {
  LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0Tester_Case6 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_LDC_literal actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw11111ddddcccciiiiiiii");
}

// coproc(11:8)=~101x & op1(25:20)=10xxxx & op(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0,
//       defs: {},
//       pattern: cccc1110oooonnnnddddccccooo0mmmm,
//       rule: CDP,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0Tester_Case7_TestCase7) {
  CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_CDP actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc1110oooonnnnddddccccooo0mmmm");
}

// op1(25:20)=00000x
//    = {actual: Actual_Unnamed_case_1,
//       baseline: Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0,
//       defs: {},
//       pattern: cccc1100000xnnnnxxxxccccxxxoxxxx,
//       safety: [true => UNDEFINED],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0Tester_Case8_TestCase8) {
  Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0Tester_Case8 baseline_tester;
  NamedActual_Unnamed_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc1100000xnnnnxxxxccccxxxoxxxx");
}

// op1(25:20)=11xxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc1111iiiiiiiiiiiiiiiiiiiiiiii,
//       rule: SVC,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case9_TestCase9) {
  SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case9 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_SVC actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc1111iiiiiiiiiiiiiiiiiiiiiiii");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
