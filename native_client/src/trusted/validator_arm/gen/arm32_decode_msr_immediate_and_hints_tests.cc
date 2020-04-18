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


// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000000 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_NOP_cccc0011001000001111000000000000_case_1,
//       baseline: NOP_cccc0011001000001111000000000000_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000000,
//       rule: NOP,
//       uses: {}}
class NOP_cccc0011001000001111000000000000_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  NOP_cccc0011001000001111000000000000_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool NOP_cccc0011001000001111000000000000_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22)=~0
  if ((inst.Bits() & 0x00400000)  !=
          0x00000000) return false;
  // op1(19:16)=~0000
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;
  // op2(7:0)=~00000000
  if ((inst.Bits() & 0x000000FF)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx11110000xxxxxxxx
  if ((inst.Bits() & 0x0000FF00)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000001 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_NOP_cccc0011001000001111000000000000_case_1,
//       baseline: YIELD_cccc0011001000001111000000000001_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000001,
//       rule: YIELD,
//       uses: {}}
class YIELD_cccc0011001000001111000000000001_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  YIELD_cccc0011001000001111000000000001_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool YIELD_cccc0011001000001111000000000001_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22)=~0
  if ((inst.Bits() & 0x00400000)  !=
          0x00000000) return false;
  // op1(19:16)=~0000
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;
  // op2(7:0)=~00000001
  if ((inst.Bits() & 0x000000FF)  !=
          0x00000001) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx11110000xxxxxxxx
  if ((inst.Bits() & 0x0000FF00)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000010 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: WFE_cccc0011001000001111000000000010_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000010,
//       rule: WFE,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class WFE_cccc0011001000001111000000000010_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  WFE_cccc0011001000001111000000000010_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool WFE_cccc0011001000001111000000000010_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22)=~0
  if ((inst.Bits() & 0x00400000)  !=
          0x00000000) return false;
  // op1(19:16)=~0000
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;
  // op2(7:0)=~00000010
  if ((inst.Bits() & 0x000000FF)  !=
          0x00000002) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx11110000xxxxxxxx
  if ((inst.Bits() & 0x0000FF00)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000011 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: WFI_cccc0011001000001111000000000011_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000011,
//       rule: WFI,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class WFI_cccc0011001000001111000000000011_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  WFI_cccc0011001000001111000000000011_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool WFI_cccc0011001000001111000000000011_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22)=~0
  if ((inst.Bits() & 0x00400000)  !=
          0x00000000) return false;
  // op1(19:16)=~0000
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;
  // op2(7:0)=~00000011
  if ((inst.Bits() & 0x000000FF)  !=
          0x00000003) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx11110000xxxxxxxx
  if ((inst.Bits() & 0x0000FF00)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000100 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SEV_cccc0011001000001111000000000100_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000100,
//       rule: SEV,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class SEV_cccc0011001000001111000000000100_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  SEV_cccc0011001000001111000000000100_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SEV_cccc0011001000001111000000000100_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22)=~0
  if ((inst.Bits() & 0x00400000)  !=
          0x00000000) return false;
  // op1(19:16)=~0000
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;
  // op2(7:0)=~00000100
  if ((inst.Bits() & 0x000000FF)  !=
          0x00000004) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx11110000xxxxxxxx
  if ((inst.Bits() & 0x0000FF00)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=1111xxxx & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: DBG_cccc001100100000111100001111iiii_case_0,
//       defs: {},
//       pattern: cccc001100100000111100001111iiii,
//       rule: DBG,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class DBG_cccc001100100000111100001111iiii_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  DBG_cccc001100100000111100001111iiii_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool DBG_cccc001100100000111100001111iiii_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22)=~0
  if ((inst.Bits() & 0x00400000)  !=
          0x00000000) return false;
  // op1(19:16)=~0000
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;
  // op2(7:0)=~1111xxxx
  if ((inst.Bits() & 0x000000F0)  !=
          0x000000F0) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx11110000xxxxxxxx
  if ((inst.Bits() & 0x0000FF00)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22)=0 & op1(19:16)=0100 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       actual: Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0,
//       defs: {NZCV
//            if write_nzcvq
//            else None},
//       fields: [mask(19:18)],
//       mask: mask(19:18),
//       pattern: cccc00110010mm001111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [mask(19:18)=00 => DECODER_ERROR],
//       uses: {},
//       write_nzcvq: mask(1)=1}
class MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22)=~0
  if ((inst.Bits() & 0x00400000)  !=
          0x00000000) return false;
  // op1(19:16)=~0100
  if ((inst.Bits() & 0x000F0000)  !=
          0x00040000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22)=0 & op1(19:16)=1x00 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       actual: Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0,
//       defs: {NZCV
//            if write_nzcvq
//            else None},
//       fields: [mask(19:18)],
//       mask: mask(19:18),
//       pattern: cccc00110010mm001111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [mask(19:18)=00 => DECODER_ERROR],
//       uses: {},
//       write_nzcvq: mask(1)=1}
class MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22)=~0
  if ((inst.Bits() & 0x00400000)  !=
          0x00000000) return false;
  // op1(19:16)=~1x00
  if ((inst.Bits() & 0x000B0000)  !=
          0x00080000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22)=0 & op1(19:16)=xx01 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22)=~0
  if ((inst.Bits() & 0x00400000)  !=
          0x00000000) return false;
  // op1(19:16)=~xx01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22)=0 & op1(19:16)=xx1x & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22)=~0
  if ((inst.Bits() & 0x00400000)  !=
          0x00000000) return false;
  // op1(19:16)=~xx1x
  if ((inst.Bits() & 0x00020000)  !=
          0x00020000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(22)=1 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(22)=~1
  if ((inst.Bits() & 0x00400000)  !=
          0x00400000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

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

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000000 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_NOP_cccc0011001000001111000000000000_case_1,
//       baseline: NOP_cccc0011001000001111000000000000_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000000,
//       rule: NOP,
//       uses: {}}
class NOP_cccc0011001000001111000000000000_case_0Tester_Case0
    : public NOP_cccc0011001000001111000000000000_case_0TesterCase0 {
 public:
  NOP_cccc0011001000001111000000000000_case_0Tester_Case0()
    : NOP_cccc0011001000001111000000000000_case_0TesterCase0(
      state_.NOP_cccc0011001000001111000000000000_case_0_NOP_instance_)
  {}
};

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000001 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_NOP_cccc0011001000001111000000000000_case_1,
//       baseline: YIELD_cccc0011001000001111000000000001_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000001,
//       rule: YIELD,
//       uses: {}}
class YIELD_cccc0011001000001111000000000001_case_0Tester_Case1
    : public YIELD_cccc0011001000001111000000000001_case_0TesterCase1 {
 public:
  YIELD_cccc0011001000001111000000000001_case_0Tester_Case1()
    : YIELD_cccc0011001000001111000000000001_case_0TesterCase1(
      state_.YIELD_cccc0011001000001111000000000001_case_0_YIELD_instance_)
  {}
};

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000010 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: WFE_cccc0011001000001111000000000010_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000010,
//       rule: WFE,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class WFE_cccc0011001000001111000000000010_case_0Tester_Case2
    : public WFE_cccc0011001000001111000000000010_case_0TesterCase2 {
 public:
  WFE_cccc0011001000001111000000000010_case_0Tester_Case2()
    : WFE_cccc0011001000001111000000000010_case_0TesterCase2(
      state_.WFE_cccc0011001000001111000000000010_case_0_WFE_instance_)
  {}
};

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000011 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: WFI_cccc0011001000001111000000000011_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000011,
//       rule: WFI,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class WFI_cccc0011001000001111000000000011_case_0Tester_Case3
    : public WFI_cccc0011001000001111000000000011_case_0TesterCase3 {
 public:
  WFI_cccc0011001000001111000000000011_case_0Tester_Case3()
    : WFI_cccc0011001000001111000000000011_case_0TesterCase3(
      state_.WFI_cccc0011001000001111000000000011_case_0_WFI_instance_)
  {}
};

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000100 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SEV_cccc0011001000001111000000000100_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000100,
//       rule: SEV,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class SEV_cccc0011001000001111000000000100_case_0Tester_Case4
    : public SEV_cccc0011001000001111000000000100_case_0TesterCase4 {
 public:
  SEV_cccc0011001000001111000000000100_case_0Tester_Case4()
    : SEV_cccc0011001000001111000000000100_case_0TesterCase4(
      state_.SEV_cccc0011001000001111000000000100_case_0_SEV_instance_)
  {}
};

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=1111xxxx & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: DBG_cccc001100100000111100001111iiii_case_0,
//       defs: {},
//       pattern: cccc001100100000111100001111iiii,
//       rule: DBG,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class DBG_cccc001100100000111100001111iiii_case_0Tester_Case5
    : public DBG_cccc001100100000111100001111iiii_case_0TesterCase5 {
 public:
  DBG_cccc001100100000111100001111iiii_case_0Tester_Case5()
    : DBG_cccc001100100000111100001111iiii_case_0TesterCase5(
      state_.DBG_cccc001100100000111100001111iiii_case_0_DBG_instance_)
  {}
};

// op(22)=0 & op1(19:16)=0100 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       actual: Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0,
//       defs: {NZCV
//            if write_nzcvq
//            else None},
//       fields: [mask(19:18)],
//       mask: mask(19:18),
//       pattern: cccc00110010mm001111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [mask(19:18)=00 => DECODER_ERROR],
//       uses: {},
//       write_nzcvq: mask(1)=1}
class MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0Tester_Case6
    : public MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0TesterCase6 {
 public:
  MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0Tester_Case6()
    : MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0TesterCase6(
      state_.MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0_MSR_immediate_instance_)
  {}
};

// op(22)=0 & op1(19:16)=1x00 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       actual: Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0,
//       defs: {NZCV
//            if write_nzcvq
//            else None},
//       fields: [mask(19:18)],
//       mask: mask(19:18),
//       pattern: cccc00110010mm001111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [mask(19:18)=00 => DECODER_ERROR],
//       uses: {},
//       write_nzcvq: mask(1)=1}
class MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0Tester_Case7
    : public MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0TesterCase7 {
 public:
  MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0Tester_Case7()
    : MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0TesterCase7(
      state_.MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0_MSR_immediate_instance_)
  {}
};

// op(22)=0 & op1(19:16)=xx01 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case8
    : public MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase8 {
 public:
  MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case8()
    : MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase8(
      state_.MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0_MSR_immediate_instance_)
  {}
};

// op(22)=0 & op1(19:16)=xx1x & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case9
    : public MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase9 {
 public:
  MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case9()
    : MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase9(
      state_.MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0_MSR_immediate_instance_)
  {}
};

// op(22)=1 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case10
    : public MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase10 {
 public:
  MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case10()
    : MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0TesterCase10(
      state_.MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0_MSR_immediate_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000000 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_NOP_cccc0011001000001111000000000000_case_1,
//       baseline: NOP_cccc0011001000001111000000000000_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000000,
//       rule: NOP,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       NOP_cccc0011001000001111000000000000_case_0Tester_Case0_TestCase0) {
  NOP_cccc0011001000001111000000000000_case_0Tester_Case0 baseline_tester;
  NamedActual_NOP_cccc0011001000001111000000000000_case_1_NOP actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0011001000001111000000000000");
}

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000001 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_NOP_cccc0011001000001111000000000000_case_1,
//       baseline: YIELD_cccc0011001000001111000000000001_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000001,
//       rule: YIELD,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       YIELD_cccc0011001000001111000000000001_case_0Tester_Case1_TestCase1) {
  YIELD_cccc0011001000001111000000000001_case_0Tester_Case1 baseline_tester;
  NamedActual_NOP_cccc0011001000001111000000000000_case_1_YIELD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0011001000001111000000000001");
}

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000010 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: WFE_cccc0011001000001111000000000010_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000010,
//       rule: WFE,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       WFE_cccc0011001000001111000000000010_case_0Tester_Case2_TestCase2) {
  WFE_cccc0011001000001111000000000010_case_0Tester_Case2 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_WFE actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0011001000001111000000000010");
}

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000011 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: WFI_cccc0011001000001111000000000011_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000011,
//       rule: WFI,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       WFI_cccc0011001000001111000000000011_case_0Tester_Case3_TestCase3) {
  WFI_cccc0011001000001111000000000011_case_0Tester_Case3 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_WFI actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0011001000001111000000000011");
}

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=00000100 & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SEV_cccc0011001000001111000000000100_case_0,
//       defs: {},
//       pattern: cccc0011001000001111000000000100,
//       rule: SEV,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       SEV_cccc0011001000001111000000000100_case_0Tester_Case4_TestCase4) {
  SEV_cccc0011001000001111000000000100_case_0Tester_Case4 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_SEV actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0011001000001111000000000100");
}

// op(22)=0 & op1(19:16)=0000 & op2(7:0)=1111xxxx & $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: DBG_cccc001100100000111100001111iiii_case_0,
//       defs: {},
//       pattern: cccc001100100000111100001111iiii,
//       rule: DBG,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       DBG_cccc001100100000111100001111iiii_case_0Tester_Case5_TestCase5) {
  DBG_cccc001100100000111100001111iiii_case_0Tester_Case5 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_DBG actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc001100100000111100001111iiii");
}

// op(22)=0 & op1(19:16)=0100 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       actual: Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0,
//       defs: {NZCV
//            if write_nzcvq
//            else None},
//       fields: [mask(19:18)],
//       mask: mask(19:18),
//       pattern: cccc00110010mm001111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [mask(19:18)=00 => DECODER_ERROR],
//       uses: {},
//       write_nzcvq: mask(1)=1}
TEST_F(Arm32DecoderStateTests,
       MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0Tester_Case6_TestCase6) {
  MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0Tester_Case6 baseline_tester;
  NamedActual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1_MSR_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00110010mm001111iiiiiiiiiiii");
}

// op(22)=0 & op1(19:16)=1x00 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       actual: Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0,
//       defs: {NZCV
//            if write_nzcvq
//            else None},
//       fields: [mask(19:18)],
//       mask: mask(19:18),
//       pattern: cccc00110010mm001111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [mask(19:18)=00 => DECODER_ERROR],
//       uses: {},
//       write_nzcvq: mask(1)=1}
TEST_F(Arm32DecoderStateTests,
       MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0Tester_Case7_TestCase7) {
  MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0Tester_Case7 baseline_tester;
  NamedActual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1_MSR_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00110010mm001111iiiiiiiiiiii");
}

// op(22)=0 & op1(19:16)=xx01 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case8_TestCase8) {
  MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case8 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_MSR_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00110r10mmmm1111iiiiiiiiiiii");
}

// op(22)=0 & op1(19:16)=xx1x & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case9_TestCase9) {
  MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case9 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_MSR_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00110r10mmmm1111iiiiiiiiiiii");
}

// op(22)=1 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//       rule: MSR_immediate,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case10_TestCase10) {
  MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0Tester_Case10 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_MSR_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00110r10mmmm1111iiiiiiiiiiii");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
