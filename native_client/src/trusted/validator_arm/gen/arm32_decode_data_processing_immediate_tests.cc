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


// op(24:20)=10001 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1,
//       baseline: TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm32: ARMExpandImm_C(imm12),
//       pattern: cccc00110001nnnn0000iiiiiiiiiiii,
//       rule: TST_immediate,
//       sets_Z_if_clear_bits: Rn  ==
//               RegIndex(test_register()) &&
//            (imm32 &&
//            clears_mask())  ==
//               clears_mask(),
//       uses: {Rn}}
class TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~10001
  if ((inst.Bits() & 0x01F00000)  !=
          0x01100000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=10011 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//       baseline: TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16)],
//       pattern: cccc00110011nnnn0000iiiiiiiiiiii,
//       rule: TEQ_immediate,
//       uses: {Rn}}
class TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~10011
  if ((inst.Bits() & 0x01F00000)  !=
          0x01300000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=10101 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//       baseline: CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16)],
//       pattern: cccc00110101nnnn0000iiiiiiiiiiii,
//       rule: CMP_immediate,
//       uses: {Rn}}
class CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~10101
  if ((inst.Bits() & 0x01F00000)  !=
          0x01500000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=10111 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//       baseline: CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16)],
//       pattern: cccc00110111nnnn0000iiiiiiiiiiii,
//       rule: CMN_immediate,
//       uses: {Rn}}
class CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~10111
  if ((inst.Bits() & 0x01F00000)  !=
          0x01700000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=0000x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010000snnnnddddiiiiiiiiiiii,
//       rule: AND_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~0000x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=0001x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010001snnnnddddiiiiiiiiiiii,
//       rule: EOR_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~0001x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00200000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=0010x & Rn(19:16)=~1111
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1,
//       baseline: SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010010snnnnddddiiiiiiiiiiii,
//       rule: SUB_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         (Rn(19:16)=1111 &&
//            S(20)=0) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~0010x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00400000) return false;
  // Rn(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=0010x & Rn(19:16)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1,
//       baseline: ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12)],
//       pattern: cccc001001001111ddddiiiiiiiiiiii,
//       rule: ADR_A2,
//       safety: [Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       uses: {Pc}}
class ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~0010x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00400000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=0011x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010011snnnnddddiiiiiiiiiiii,
//       rule: RSB_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~0011x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00600000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=0100x & Rn(19:16)=~1111
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1,
//       baseline: ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010100snnnnddddiiiiiiiiiiii,
//       rule: ADD_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         (Rn(19:16)=1111 &&
//            S(20)=0) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~0100x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00800000) return false;
  // Rn(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=0100x & Rn(19:16)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1,
//       baseline: ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12)],
//       pattern: cccc001010001111ddddiiiiiiiiiiii,
//       rule: ADR_A1,
//       safety: [Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       uses: {Pc}}
class ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~0100x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00800000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=0101x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010101snnnnddddiiiiiiiiiiii,
//       rule: ADC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~0101x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00A00000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=0110x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010110snnnnddddiiiiiiiiiiii,
//       rule: SBC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~0110x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00C00000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=0111x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010111snnnnddddiiiiiiiiiiii,
//       rule: RSC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~0111x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00E00000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=1100x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1,
//       baseline: ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       dynamic_code_replace_immediates: {imm12},
//       fields: [S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       pattern: cccc0011100snnnnddddiiiiiiiiiiii,
//       rule: ORR_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~1100x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01800000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=1101x & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1,
//       baseline: MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       dynamic_code_replace_immediates: {imm12},
//       fields: [S(20), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       pattern: cccc0011101s0000ddddiiiiiiiiiiii,
//       rule: MOV_immediate_A1,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {}}
class MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~1101x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01A00000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=1110x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1,
//       baseline: BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0,
//       clears_bits: (imm32 &&
//            clears_mask())  ==
//               clears_mask(),
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm32: ARMExpandImm(imm12),
//       pattern: cccc0011110snnnnddddiiiiiiiiiiii,
//       rule: BIC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0TesterCase16
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~1110x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01C00000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(24:20)=1111x & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1,
//       baseline: MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       dynamic_code_replace_immediates: {imm12},
//       fields: [S(20), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       pattern: cccc0011111s0000ddddiiiiiiiiiiii,
//       rule: MVN_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {}}
class MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0TesterCase17
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(24:20)=~1111x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01E00000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x000F0000)  !=
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

// op(24:20)=10001 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1,
//       baseline: TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm32: ARMExpandImm_C(imm12),
//       pattern: cccc00110001nnnn0000iiiiiiiiiiii,
//       rule: TST_immediate,
//       sets_Z_if_clear_bits: Rn  ==
//               RegIndex(test_register()) &&
//            (imm32 &&
//            clears_mask())  ==
//               clears_mask(),
//       uses: {Rn}}
class TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0Tester_Case0
    : public TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0TesterCase0 {
 public:
  TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0Tester_Case0()
    : TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0TesterCase0(
      state_.TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0_TST_immediate_instance_)
  {}
};

// op(24:20)=10011 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//       baseline: TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16)],
//       pattern: cccc00110011nnnn0000iiiiiiiiiiii,
//       rule: TEQ_immediate,
//       uses: {Rn}}
class TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0Tester_Case1
    : public TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0TesterCase1 {
 public:
  TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0Tester_Case1()
    : TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0TesterCase1(
      state_.TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0_TEQ_immediate_instance_)
  {}
};

// op(24:20)=10101 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//       baseline: CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16)],
//       pattern: cccc00110101nnnn0000iiiiiiiiiiii,
//       rule: CMP_immediate,
//       uses: {Rn}}
class CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0Tester_Case2
    : public CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0TesterCase2 {
 public:
  CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0Tester_Case2()
    : CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0TesterCase2(
      state_.CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0_CMP_immediate_instance_)
  {}
};

// op(24:20)=10111 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//       baseline: CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16)],
//       pattern: cccc00110111nnnn0000iiiiiiiiiiii,
//       rule: CMN_immediate,
//       uses: {Rn}}
class CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0Tester_Case3
    : public CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0TesterCase3 {
 public:
  CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0Tester_Case3()
    : CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0TesterCase3(
      state_.CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0_CMN_immediate_instance_)
  {}
};

// op(24:20)=0000x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010000snnnnddddiiiiiiiiiiii,
//       rule: AND_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0Tester_Case4
    : public AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0TesterCase4 {
 public:
  AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0Tester_Case4()
    : AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0TesterCase4(
      state_.AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0_AND_immediate_instance_)
  {}
};

// op(24:20)=0001x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010001snnnnddddiiiiiiiiiiii,
//       rule: EOR_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0Tester_Case5
    : public EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0TesterCase5 {
 public:
  EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0Tester_Case5()
    : EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0TesterCase5(
      state_.EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0_EOR_immediate_instance_)
  {}
};

// op(24:20)=0010x & Rn(19:16)=~1111
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1,
//       baseline: SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010010snnnnddddiiiiiiiiiiii,
//       rule: SUB_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         (Rn(19:16)=1111 &&
//            S(20)=0) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0Tester_Case6
    : public SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0TesterCase6 {
 public:
  SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0Tester_Case6()
    : SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0TesterCase6(
      state_.SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0_SUB_immediate_instance_)
  {}
};

// op(24:20)=0010x & Rn(19:16)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1,
//       baseline: ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12)],
//       pattern: cccc001001001111ddddiiiiiiiiiiii,
//       rule: ADR_A2,
//       safety: [Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       uses: {Pc}}
class ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0Tester_Case7
    : public ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0TesterCase7 {
 public:
  ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0Tester_Case7()
    : ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0TesterCase7(
      state_.ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0_ADR_A2_instance_)
  {}
};

// op(24:20)=0011x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010011snnnnddddiiiiiiiiiiii,
//       rule: RSB_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0Tester_Case8
    : public RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0TesterCase8 {
 public:
  RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0Tester_Case8()
    : RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0TesterCase8(
      state_.RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0_RSB_immediate_instance_)
  {}
};

// op(24:20)=0100x & Rn(19:16)=~1111
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1,
//       baseline: ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010100snnnnddddiiiiiiiiiiii,
//       rule: ADD_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         (Rn(19:16)=1111 &&
//            S(20)=0) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0Tester_Case9
    : public ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0TesterCase9 {
 public:
  ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0Tester_Case9()
    : ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0TesterCase9(
      state_.ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0_ADD_immediate_instance_)
  {}
};

// op(24:20)=0100x & Rn(19:16)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1,
//       baseline: ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12)],
//       pattern: cccc001010001111ddddiiiiiiiiiiii,
//       rule: ADR_A1,
//       safety: [Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       uses: {Pc}}
class ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0Tester_Case10
    : public ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0TesterCase10 {
 public:
  ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0Tester_Case10()
    : ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0TesterCase10(
      state_.ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0_ADR_A1_instance_)
  {}
};

// op(24:20)=0101x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010101snnnnddddiiiiiiiiiiii,
//       rule: ADC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0Tester_Case11
    : public ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0TesterCase11 {
 public:
  ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0Tester_Case11()
    : ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0TesterCase11(
      state_.ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0_ADC_immediate_instance_)
  {}
};

// op(24:20)=0110x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010110snnnnddddiiiiiiiiiiii,
//       rule: SBC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0Tester_Case12
    : public SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0TesterCase12 {
 public:
  SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0Tester_Case12()
    : SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0TesterCase12(
      state_.SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0_SBC_immediate_instance_)
  {}
};

// op(24:20)=0111x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010111snnnnddddiiiiiiiiiiii,
//       rule: RSC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0Tester_Case13
    : public RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0TesterCase13 {
 public:
  RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0Tester_Case13()
    : RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0TesterCase13(
      state_.RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0_RSC_immediate_instance_)
  {}
};

// op(24:20)=1100x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1,
//       baseline: ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       dynamic_code_replace_immediates: {imm12},
//       fields: [S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       pattern: cccc0011100snnnnddddiiiiiiiiiiii,
//       rule: ORR_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0Tester_Case14
    : public ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0TesterCase14 {
 public:
  ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0Tester_Case14()
    : ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0TesterCase14(
      state_.ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0_ORR_immediate_instance_)
  {}
};

// op(24:20)=1101x & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1,
//       baseline: MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       dynamic_code_replace_immediates: {imm12},
//       fields: [S(20), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       pattern: cccc0011101s0000ddddiiiiiiiiiiii,
//       rule: MOV_immediate_A1,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {}}
class MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0Tester_Case15
    : public MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0TesterCase15 {
 public:
  MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0Tester_Case15()
    : MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0TesterCase15(
      state_.MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0_MOV_immediate_A1_instance_)
  {}
};

// op(24:20)=1110x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1,
//       baseline: BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0,
//       clears_bits: (imm32 &&
//            clears_mask())  ==
//               clears_mask(),
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm32: ARMExpandImm(imm12),
//       pattern: cccc0011110snnnnddddiiiiiiiiiiii,
//       rule: BIC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
class BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0Tester_Case16
    : public BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0TesterCase16 {
 public:
  BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0Tester_Case16()
    : BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0TesterCase16(
      state_.BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0_BIC_immediate_instance_)
  {}
};

// op(24:20)=1111x & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1,
//       baseline: MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       dynamic_code_replace_immediates: {imm12},
//       fields: [S(20), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       pattern: cccc0011111s0000ddddiiiiiiiiiiii,
//       rule: MVN_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {}}
class MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0Tester_Case17
    : public MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0TesterCase17 {
 public:
  MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0Tester_Case17()
    : MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0TesterCase17(
      state_.MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0_MVN_immediate_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op(24:20)=10001 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1,
//       baseline: TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm32: ARMExpandImm_C(imm12),
//       pattern: cccc00110001nnnn0000iiiiiiiiiiii,
//       rule: TST_immediate,
//       sets_Z_if_clear_bits: Rn  ==
//               RegIndex(test_register()) &&
//            (imm32 &&
//            clears_mask())  ==
//               clears_mask(),
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0Tester_Case0_TestCase0) {
  TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0Tester_Case0 baseline_tester;
  NamedActual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1_TST_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00110001nnnn0000iiiiiiiiiiii");
}

// op(24:20)=10011 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//       baseline: TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16)],
//       pattern: cccc00110011nnnn0000iiiiiiiiiiii,
//       rule: TEQ_immediate,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0Tester_Case1_TestCase1) {
  TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0Tester_Case1 baseline_tester;
  NamedActual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1_TEQ_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00110011nnnn0000iiiiiiiiiiii");
}

// op(24:20)=10101 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//       baseline: CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16)],
//       pattern: cccc00110101nnnn0000iiiiiiiiiiii,
//       rule: CMP_immediate,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0Tester_Case2_TestCase2) {
  CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0Tester_Case2 baseline_tester;
  NamedActual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1_CMP_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00110101nnnn0000iiiiiiiiiiii");
}

// op(24:20)=10111 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Rn: Rn(19:16),
//       actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//       baseline: CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16)],
//       pattern: cccc00110111nnnn0000iiiiiiiiiiii,
//       rule: CMN_immediate,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0Tester_Case3_TestCase3) {
  CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0Tester_Case3 baseline_tester;
  NamedActual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1_CMN_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00110111nnnn0000iiiiiiiiiiii");
}

// op(24:20)=0000x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010000snnnnddddiiiiiiiiiiii,
//       rule: AND_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0Tester_Case4_TestCase4) {
  AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0Tester_Case4 baseline_tester;
  NamedActual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_AND_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0010000snnnnddddiiiiiiiiiiii");
}

// op(24:20)=0001x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010001snnnnddddiiiiiiiiiiii,
//       rule: EOR_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0Tester_Case5_TestCase5) {
  EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0Tester_Case5 baseline_tester;
  NamedActual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_EOR_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0010001snnnnddddiiiiiiiiiiii");
}

// op(24:20)=0010x & Rn(19:16)=~1111
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1,
//       baseline: SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010010snnnnddddiiiiiiiiiiii,
//       rule: SUB_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         (Rn(19:16)=1111 &&
//            S(20)=0) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0Tester_Case6_TestCase6) {
  SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0Tester_Case6 baseline_tester;
  NamedActual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1_SUB_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0010010snnnnddddiiiiiiiiiiii");
}

// op(24:20)=0010x & Rn(19:16)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1,
//       baseline: ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12)],
//       pattern: cccc001001001111ddddiiiiiiiiiiii,
//       rule: ADR_A2,
//       safety: [Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       uses: {Pc}}
TEST_F(Arm32DecoderStateTests,
       ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0Tester_Case7_TestCase7) {
  ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0Tester_Case7 baseline_tester;
  NamedActual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1_ADR_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc001001001111ddddiiiiiiiiiiii");
}

// op(24:20)=0011x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010011snnnnddddiiiiiiiiiiii,
//       rule: RSB_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0Tester_Case8_TestCase8) {
  RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0Tester_Case8 baseline_tester;
  NamedActual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_RSB_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0010011snnnnddddiiiiiiiiiiii");
}

// op(24:20)=0100x & Rn(19:16)=~1111
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1,
//       baseline: ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010100snnnnddddiiiiiiiiiiii,
//       rule: ADD_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         (Rn(19:16)=1111 &&
//            S(20)=0) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0Tester_Case9_TestCase9) {
  ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0Tester_Case9 baseline_tester;
  NamedActual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1_ADD_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0010100snnnnddddiiiiiiiiiiii");
}

// op(24:20)=0100x & Rn(19:16)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1,
//       baseline: ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12)],
//       pattern: cccc001010001111ddddiiiiiiiiiiii,
//       rule: ADR_A1,
//       safety: [Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       uses: {Pc}}
TEST_F(Arm32DecoderStateTests,
       ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0Tester_Case10_TestCase10) {
  ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0Tester_Case10 baseline_tester;
  NamedActual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1_ADR_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc001010001111ddddiiiiiiiiiiii");
}

// op(24:20)=0101x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010101snnnnddddiiiiiiiiiiii,
//       rule: ADC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0Tester_Case11_TestCase11) {
  ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0Tester_Case11 baseline_tester;
  NamedActual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_ADC_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0010101snnnnddddiiiiiiiiiiii");
}

// op(24:20)=0110x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010110snnnnddddiiiiiiiiiiii,
//       rule: SBC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0Tester_Case12_TestCase12) {
  SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0Tester_Case12 baseline_tester;
  NamedActual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_SBC_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0010110snnnnddddiiiiiiiiiiii");
}

// op(24:20)=0111x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//       baseline: RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12)],
//       pattern: cccc0010111snnnnddddiiiiiiiiiiii,
//       rule: RSC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0Tester_Case13_TestCase13) {
  RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0Tester_Case13 baseline_tester;
  NamedActual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_RSC_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0010111snnnnddddiiiiiiiiiiii");
}

// op(24:20)=1100x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1,
//       baseline: ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       dynamic_code_replace_immediates: {imm12},
//       fields: [S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       pattern: cccc0011100snnnnddddiiiiiiiiiiii,
//       rule: ORR_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0Tester_Case14_TestCase14) {
  ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0Tester_Case14 baseline_tester;
  NamedActual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1_ORR_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0011100snnnnddddiiiiiiiiiiii");
}

// op(24:20)=1101x & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1,
//       baseline: MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       dynamic_code_replace_immediates: {imm12},
//       fields: [S(20), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       pattern: cccc0011101s0000ddddiiiiiiiiiiii,
//       rule: MOV_immediate_A1,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0Tester_Case15_TestCase15) {
  MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0Tester_Case15 baseline_tester;
  NamedActual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1_MOV_immediate_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0011101s0000ddddiiiiiiiiiiii");
}

// op(24:20)=1110x
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       Rn: Rn(19:16),
//       S: S(20),
//       actual: Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1,
//       baseline: BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0,
//       clears_bits: (imm32 &&
//            clears_mask())  ==
//               clears_mask(),
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       imm32: ARMExpandImm(imm12),
//       pattern: cccc0011110snnnnddddiiiiiiiiiiii,
//       rule: BIC_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0Tester_Case16_TestCase16) {
  BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0Tester_Case16 baseline_tester;
  NamedActual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1_BIC_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0011110snnnnddddiiiiiiiiiiii");
}

// op(24:20)=1111x & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Rd: Rd(15:12),
//       S: S(20),
//       actual: Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1,
//       baseline: MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       dynamic_code_replace_immediates: {imm12},
//       fields: [S(20), Rd(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       pattern: cccc0011111s0000ddddiiiiiiiiiiii,
//       rule: MVN_immediate,
//       safety: [(Rd(15:12)=1111 &&
//            S(20)=1) => DECODER_ERROR,
//         Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//       setflags: S(20)=1,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0Tester_Case17_TestCase17) {
  MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0Tester_Case17 baseline_tester;
  NamedActual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1_MVN_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0011111s0000ddddiiiiiiiiiiii");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
