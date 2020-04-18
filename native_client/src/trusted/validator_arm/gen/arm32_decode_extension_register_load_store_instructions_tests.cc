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


// opcode(24:20)=01x00 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~01x00
  if ((inst.Bits() & 0x01B00000)  !=
          0x00800000) return false;
  // S(8)=~0
  if ((inst.Bits() & 0x00000100)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=01x00 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~01x00
  if ((inst.Bits() & 0x01B00000)  !=
          0x00800000) return false;
  // S(8)=~1
  if ((inst.Bits() & 0x00000100)  !=
          0x00000100) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=01x01 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~01x01
  if ((inst.Bits() & 0x01B00000)  !=
          0x00900000) return false;
  // S(8)=~0
  if ((inst.Bits() & 0x00000100)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=01x01 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~01x01
  if ((inst.Bits() & 0x01B00000)  !=
          0x00900000) return false;
  // S(8)=~1
  if ((inst.Bits() & 0x00000100)  !=
          0x00000100) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=01x10 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~01x10
  if ((inst.Bits() & 0x01B00000)  !=
          0x00A00000) return false;
  // S(8)=~0
  if ((inst.Bits() & 0x00000100)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=01x10 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~01x10
  if ((inst.Bits() & 0x01B00000)  !=
          0x00A00000) return false;
  // S(8)=~1
  if ((inst.Bits() & 0x00000100)  !=
          0x00000100) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=01x11 & Rn(19:16)=~1101 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~01x11
  if ((inst.Bits() & 0x01B00000)  !=
          0x00B00000) return false;
  // Rn(19:16)=1101
  if ((inst.Bits() & 0x000F0000)  ==
          0x000D0000) return false;
  // S(8)=~0
  if ((inst.Bits() & 0x00000100)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=01x11 & Rn(19:16)=~1101 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~01x11
  if ((inst.Bits() & 0x01B00000)  !=
          0x00B00000) return false;
  // Rn(19:16)=1101
  if ((inst.Bits() & 0x000F0000)  ==
          0x000D0000) return false;
  // S(8)=~1
  if ((inst.Bits() & 0x00000100)  !=
          0x00000100) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=01x11 & Rn(19:16)=1101 & S(8)=0
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPOP_cccc11001d111101dddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11001d111101dddd1010iiiiiiii,
//       regs: imm8,
//       rule: VPOP,
//       safety: [regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
class VPOP_cccc11001d111101dddd1010iiiiiiii_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  VPOP_cccc11001d111101dddd1010iiiiiiii_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPOP_cccc11001d111101dddd1010iiiiiiii_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~01x11
  if ((inst.Bits() & 0x01B00000)  !=
          0x00B00000) return false;
  // Rn(19:16)=~1101
  if ((inst.Bits() & 0x000F0000)  !=
          0x000D0000) return false;
  // S(8)=~0
  if ((inst.Bits() & 0x00000100)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=01x11 & Rn(19:16)=1101 & S(8)=1
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPOP_cccc11001d111101dddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11001d111101dddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VPOP,
//       safety: [regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
class VPOP_cccc11001d111101dddd1011iiiiiiii_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  VPOP_cccc11001d111101dddd1011iiiiiiii_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPOP_cccc11001d111101dddd1011iiiiiiii_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~01x11
  if ((inst.Bits() & 0x01B00000)  !=
          0x00B00000) return false;
  // Rn(19:16)=~1101
  if ((inst.Bits() & 0x000F0000)  !=
          0x000D0000) return false;
  // S(8)=~1
  if ((inst.Bits() & 0x00000100)  !=
          0x00000100) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=10x10 & Rn(19:16)=~1101 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~10x10
  if ((inst.Bits() & 0x01B00000)  !=
          0x01200000) return false;
  // Rn(19:16)=1101
  if ((inst.Bits() & 0x000F0000)  ==
          0x000D0000) return false;
  // S(8)=~0
  if ((inst.Bits() & 0x00000100)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=10x10 & Rn(19:16)=~1101 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~10x10
  if ((inst.Bits() & 0x01B00000)  !=
          0x01200000) return false;
  // Rn(19:16)=1101
  if ((inst.Bits() & 0x000F0000)  ==
          0x000D0000) return false;
  // S(8)=~1
  if ((inst.Bits() & 0x00000100)  !=
          0x00000100) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=10x10 & Rn(19:16)=1101 & S(8)=0
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11010d101101dddd1010iiiiiiii,
//       regs: imm8,
//       rule: VPUSH,
//       safety: [regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
class VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~10x10
  if ((inst.Bits() & 0x01B00000)  !=
          0x01200000) return false;
  // Rn(19:16)=~1101
  if ((inst.Bits() & 0x000F0000)  !=
          0x000D0000) return false;
  // S(8)=~0
  if ((inst.Bits() & 0x00000100)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=10x10 & Rn(19:16)=1101 & S(8)=1
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11010d101101dddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VPUSH,
//       safety: [regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
class VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~10x10
  if ((inst.Bits() & 0x01B00000)  !=
          0x01200000) return false;
  // Rn(19:16)=~1101
  if ((inst.Bits() & 0x000F0000)  !=
          0x000D0000) return false;
  // S(8)=~1
  if ((inst.Bits() & 0x00000100)  !=
          0x00000100) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=10x11 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~10x11
  if ((inst.Bits() & 0x01B00000)  !=
          0x01300000) return false;
  // S(8)=~0
  if ((inst.Bits() & 0x00000100)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=10x11 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~10x11
  if ((inst.Bits() & 0x01B00000)  !=
          0x01300000) return false;
  // S(8)=~1
  if ((inst.Bits() & 0x00000100)  !=
          0x00000100) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=1xx00 & S(8)=0
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       n: Rn,
//       pattern: cccc1101ud00nnnndddd1010iiiiiiii,
//       rule: VSTR,
//       safety: [n  ==
//               Pc => FORBIDDEN_OPERANDS],
//       uses: {Rn},
//       violations: [implied by 'base']}
class VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0TesterCase16
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~1xx00
  if ((inst.Bits() & 0x01300000)  !=
          0x01000000) return false;
  // S(8)=~0
  if ((inst.Bits() & 0x00000100)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=1xx00 & S(8)=1
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       n: Rn,
//       pattern: cccc1101ud00nnnndddd1011iiiiiiii,
//       rule: VSTR,
//       safety: [n  ==
//               Pc => FORBIDDEN_OPERANDS],
//       uses: {Rn},
//       violations: [implied by 'base']}
class VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0TesterCase17
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~1xx00
  if ((inst.Bits() & 0x01300000)  !=
          0x01000000) return false;
  // S(8)=~1
  if ((inst.Bits() & 0x00000100)  !=
          0x00000100) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=1xx01 & S(8)=0
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc1101ud01nnnndddd1010iiiiiiii,
//       rule: VLDR,
//       uses: {Rn},
//       violations: [implied by 'base']}
class VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0TesterCase18
    : public Arm32DecoderTester {
 public:
  VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0TesterCase18(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0TesterCase18
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~1xx01
  if ((inst.Bits() & 0x01300000)  !=
          0x01100000) return false;
  // S(8)=~0
  if ((inst.Bits() & 0x00000100)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opcode(24:20)=1xx01 & S(8)=1
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc1101ud01nnnndddd1011iiiiiiii,
//       rule: VLDR,
//       uses: {Rn},
//       violations: [implied by 'base']}
class VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0TesterCase19
    : public Arm32DecoderTester {
 public:
  VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0TesterCase19(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0TesterCase19
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opcode(24:20)=~1xx01
  if ((inst.Bits() & 0x01300000)  !=
          0x01100000) return false;
  // S(8)=~1
  if ((inst.Bits() & 0x00000100)  !=
          0x00000100) return false;

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

// opcode(24:20)=01x00 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case0
    : public VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase0 {
 public:
  VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case0()
    : VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase0(
      state_.VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0_VSTM_instance_)
  {}
};

// opcode(24:20)=01x00 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case1
    : public VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase1 {
 public:
  VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case1()
    : VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase1(
      state_.VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0_VSTM_instance_)
  {}
};

// opcode(24:20)=01x01 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case2
    : public VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase2 {
 public:
  VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case2()
    : VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase2(
      state_.VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0_VLDM_instance_)
  {}
};

// opcode(24:20)=01x01 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case3
    : public VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase3 {
 public:
  VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case3()
    : VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase3(
      state_.VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0_VLDM_instance_)
  {}
};

// opcode(24:20)=01x10 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case4
    : public VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase4 {
 public:
  VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case4()
    : VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase4(
      state_.VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0_VSTM_instance_)
  {}
};

// opcode(24:20)=01x10 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case5
    : public VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase5 {
 public:
  VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case5()
    : VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase5(
      state_.VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0_VSTM_instance_)
  {}
};

// opcode(24:20)=01x11 & Rn(19:16)=~1101 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case6
    : public VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase6 {
 public:
  VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case6()
    : VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase6(
      state_.VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0_VLDM_instance_)
  {}
};

// opcode(24:20)=01x11 & Rn(19:16)=~1101 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case7
    : public VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase7 {
 public:
  VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case7()
    : VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase7(
      state_.VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0_VLDM_instance_)
  {}
};

// opcode(24:20)=01x11 & Rn(19:16)=1101 & S(8)=0
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPOP_cccc11001d111101dddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11001d111101dddd1010iiiiiiii,
//       regs: imm8,
//       rule: VPOP,
//       safety: [regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
class VPOP_cccc11001d111101dddd1010iiiiiiii_case_0Tester_Case8
    : public VPOP_cccc11001d111101dddd1010iiiiiiii_case_0TesterCase8 {
 public:
  VPOP_cccc11001d111101dddd1010iiiiiiii_case_0Tester_Case8()
    : VPOP_cccc11001d111101dddd1010iiiiiiii_case_0TesterCase8(
      state_.VPOP_cccc11001d111101dddd1010iiiiiiii_case_0_VPOP_instance_)
  {}
};

// opcode(24:20)=01x11 & Rn(19:16)=1101 & S(8)=1
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPOP_cccc11001d111101dddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11001d111101dddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VPOP,
//       safety: [regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
class VPOP_cccc11001d111101dddd1011iiiiiiii_case_0Tester_Case9
    : public VPOP_cccc11001d111101dddd1011iiiiiiii_case_0TesterCase9 {
 public:
  VPOP_cccc11001d111101dddd1011iiiiiiii_case_0Tester_Case9()
    : VPOP_cccc11001d111101dddd1011iiiiiiii_case_0TesterCase9(
      state_.VPOP_cccc11001d111101dddd1011iiiiiiii_case_0_VPOP_instance_)
  {}
};

// opcode(24:20)=10x10 & Rn(19:16)=~1101 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case10
    : public VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase10 {
 public:
  VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case10()
    : VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0TesterCase10(
      state_.VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0_VSTM_instance_)
  {}
};

// opcode(24:20)=10x10 & Rn(19:16)=~1101 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case11
    : public VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase11 {
 public:
  VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case11()
    : VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0TesterCase11(
      state_.VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0_VSTM_instance_)
  {}
};

// opcode(24:20)=10x10 & Rn(19:16)=1101 & S(8)=0
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11010d101101dddd1010iiiiiiii,
//       regs: imm8,
//       rule: VPUSH,
//       safety: [regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
class VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0Tester_Case12
    : public VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0TesterCase12 {
 public:
  VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0Tester_Case12()
    : VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0TesterCase12(
      state_.VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0_VPUSH_instance_)
  {}
};

// opcode(24:20)=10x10 & Rn(19:16)=1101 & S(8)=1
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11010d101101dddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VPUSH,
//       safety: [regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
class VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0Tester_Case13
    : public VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0TesterCase13 {
 public:
  VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0Tester_Case13()
    : VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0TesterCase13(
      state_.VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0_VPUSH_instance_)
  {}
};

// opcode(24:20)=10x11 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case14
    : public VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase14 {
 public:
  VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case14()
    : VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0TesterCase14(
      state_.VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0_VLDM_instance_)
  {}
};

// opcode(24:20)=10x11 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case15
    : public VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase15 {
 public:
  VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case15()
    : VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0TesterCase15(
      state_.VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0_VLDM_instance_)
  {}
};

// opcode(24:20)=1xx00 & S(8)=0
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       n: Rn,
//       pattern: cccc1101ud00nnnndddd1010iiiiiiii,
//       rule: VSTR,
//       safety: [n  ==
//               Pc => FORBIDDEN_OPERANDS],
//       uses: {Rn},
//       violations: [implied by 'base']}
class VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0Tester_Case16
    : public VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0TesterCase16 {
 public:
  VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0Tester_Case16()
    : VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0TesterCase16(
      state_.VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0_VSTR_instance_)
  {}
};

// opcode(24:20)=1xx00 & S(8)=1
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       n: Rn,
//       pattern: cccc1101ud00nnnndddd1011iiiiiiii,
//       rule: VSTR,
//       safety: [n  ==
//               Pc => FORBIDDEN_OPERANDS],
//       uses: {Rn},
//       violations: [implied by 'base']}
class VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0Tester_Case17
    : public VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0TesterCase17 {
 public:
  VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0Tester_Case17()
    : VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0TesterCase17(
      state_.VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0_VSTR_instance_)
  {}
};

// opcode(24:20)=1xx01 & S(8)=0
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc1101ud01nnnndddd1010iiiiiiii,
//       rule: VLDR,
//       uses: {Rn},
//       violations: [implied by 'base']}
class VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0Tester_Case18
    : public VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0TesterCase18 {
 public:
  VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0Tester_Case18()
    : VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0TesterCase18(
      state_.VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0_VLDR_instance_)
  {}
};

// opcode(24:20)=1xx01 & S(8)=1
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc1101ud01nnnndddd1011iiiiiiii,
//       rule: VLDR,
//       uses: {Rn},
//       violations: [implied by 'base']}
class VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0Tester_Case19
    : public VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0TesterCase19 {
 public:
  VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0Tester_Case19()
    : VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0TesterCase19(
      state_.VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0_VLDR_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// opcode(24:20)=01x00 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case0_TestCase0) {
  VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case0 baseline_tester;
  NamedActual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1_VSTM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw0nnnndddd1010iiiiiiii");
}

// opcode(24:20)=01x00 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case1_TestCase1) {
  VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case1 baseline_tester;
  NamedActual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1_VSTM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw0nnnndddd1011iiiiiiii");
}

// opcode(24:20)=01x01 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case2_TestCase2) {
  VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case2 baseline_tester;
  NamedActual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1_VLDM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw1nnnndddd1010iiiiiiii");
}

// opcode(24:20)=01x01 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case3_TestCase3) {
  VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case3 baseline_tester;
  NamedActual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1_VLDM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw1nnnndddd1011iiiiiiii");
}

// opcode(24:20)=01x10 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case4_TestCase4) {
  VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case4 baseline_tester;
  NamedActual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1_VSTM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw0nnnndddd1010iiiiiiii");
}

// opcode(24:20)=01x10 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case5_TestCase5) {
  VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case5 baseline_tester;
  NamedActual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1_VSTM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw0nnnndddd1011iiiiiiii");
}

// opcode(24:20)=01x11 & Rn(19:16)=~1101 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case6_TestCase6) {
  VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case6 baseline_tester;
  NamedActual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1_VLDM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw1nnnndddd1010iiiiiiii");
}

// opcode(24:20)=01x11 & Rn(19:16)=~1101 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case7_TestCase7) {
  VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case7 baseline_tester;
  NamedActual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1_VLDM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw1nnnndddd1011iiiiiiii");
}

// opcode(24:20)=01x11 & Rn(19:16)=1101 & S(8)=0
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPOP_cccc11001d111101dddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11001d111101dddd1010iiiiiiii,
//       regs: imm8,
//       rule: VPOP,
//       safety: [regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       VPOP_cccc11001d111101dddd1010iiiiiiii_case_0Tester_Case8_TestCase8) {
  VPOP_cccc11001d111101dddd1010iiiiiiii_case_0Tester_Case8 baseline_tester;
  NamedActual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1_VPOP actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11001d111101dddd1010iiiiiiii");
}

// opcode(24:20)=01x11 & Rn(19:16)=1101 & S(8)=1
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPOP_cccc11001d111101dddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11001d111101dddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VPOP,
//       safety: [regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       VPOP_cccc11001d111101dddd1011iiiiiiii_case_0Tester_Case9_TestCase9) {
  VPOP_cccc11001d111101dddd1011iiiiiiii_case_0Tester_Case9 baseline_tester;
  NamedActual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1_VPOP actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11001d111101dddd1011iiiiiiii");
}

// opcode(24:20)=10x10 & Rn(19:16)=~1101 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case10_TestCase10) {
  VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0Tester_Case10 baseline_tester;
  NamedActual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1_VSTM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw0nnnndddd1010iiiiiiii");
}

// opcode(24:20)=10x10 & Rn(19:16)=~1101 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       n: Rn,
//       pattern: cccc110pudw0nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VSTM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=1 &&
//            U(23)=0 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         Rn  ==
//               Pc => FORBIDDEN_OPERANDS,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case11_TestCase11) {
  VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0Tester_Case11 baseline_tester;
  NamedActual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1_VSTM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw0nnnndddd1011iiiiiiii");
}

// opcode(24:20)=10x10 & Rn(19:16)=1101 & S(8)=0
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11010d101101dddd1010iiiiiiii,
//       regs: imm8,
//       rule: VPUSH,
//       safety: [regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0Tester_Case12_TestCase12) {
  VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0Tester_Case12 baseline_tester;
  NamedActual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1_VPUSH actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11010d101101dddd1010iiiiiiii");
}

// opcode(24:20)=10x10 & Rn(19:16)=1101 & S(8)=1
//    = {D: D(22),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1,
//       base: Sp,
//       baseline: VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Sp},
//       fields: [D(22), Vd(15:12), imm8(7:0)],
//       imm8: imm8(7:0),
//       pattern: cccc11010d101101dddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VPUSH,
//       safety: [regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: true,
//       true: true,
//       uses: {Sp},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0Tester_Case13_TestCase13) {
  VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0Tester_Case13 baseline_tester;
  NamedActual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1_VPUSH actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11010d101101dddd1011iiiiiiii");
}

// opcode(24:20)=10x11 & S(8)=0
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0,
//       d: Vd:D,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1010iiiiiiii,
//       regs: imm8,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case14_TestCase14) {
  VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0Tester_Case14 baseline_tester;
  NamedActual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1_VLDM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw1nnnndddd1010iiiiiiii");
}

// opcode(24:20)=10x11 & S(8)=1
//    = {D: D(22),
//       None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Sp: 13,
//       U: U(23),
//       Vd: Vd(15:12),
//       W: W(21),
//       actual: Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0,
//       d: D:Vd,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [P(24),
//         U(23),
//         D(22),
//         W(21),
//         Rn(19:16),
//         Vd(15:12),
//         imm8(7:0)],
//       imm8: imm8(7:0),
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc110pudw1nnnndddd1011iiiiiiii,
//       regs: imm8 / 2,
//       rule: VLDM,
//       safety: [P(24)=0 &&
//            U(23)=0 &&
//            W(21)=0 => DECODER_ERROR,
//         P(24)=1 &&
//            W(21)=0 => DECODER_ERROR,
//         P  ==
//               U &&
//            W(21)=1 => UNDEFINED,
//         n  ==
//               Pc &&
//            wback => UNPREDICTABLE,
//         P(24)=0 &&
//            U(23)=1 &&
//            W(21)=1 &&
//            Rn  ==
//               Sp => DECODER_ERROR,
//         regs  ==
//               0 ||
//            regs  >
//               16 ||
//            d + regs  >
//               32 => UNPREDICTABLE,
//         VFPSmallRegisterBank() &&
//            d + regs  >
//               16 => UNPREDICTABLE,
//         imm8(0)  ==
//               1 => DEPRECATED],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case15_TestCase15) {
  VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0Tester_Case15 baseline_tester;
  NamedActual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1_VLDM actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc110pudw1nnnndddd1011iiiiiiii");
}

// opcode(24:20)=1xx00 & S(8)=0
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       n: Rn,
//       pattern: cccc1101ud00nnnndddd1010iiiiiiii,
//       rule: VSTR,
//       safety: [n  ==
//               Pc => FORBIDDEN_OPERANDS],
//       uses: {Rn},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0Tester_Case16_TestCase16) {
  VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0Tester_Case16 baseline_tester;
  NamedActual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1_VSTR actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc1101ud00nnnndddd1010iiiiiiii");
}

// opcode(24:20)=1xx00 & S(8)=1
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       n: Rn,
//       pattern: cccc1101ud00nnnndddd1011iiiiiiii,
//       rule: VSTR,
//       safety: [n  ==
//               Pc => FORBIDDEN_OPERANDS],
//       uses: {Rn},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0Tester_Case17_TestCase17) {
  VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0Tester_Case17 baseline_tester;
  NamedActual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1_VSTR actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc1101ud00nnnndddd1011iiiiiiii");
}

// opcode(24:20)=1xx01 & S(8)=0
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc1101ud01nnnndddd1010iiiiiiii,
//       rule: VLDR,
//       uses: {Rn},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0Tester_Case18_TestCase18) {
  VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0Tester_Case18 baseline_tester;
  NamedActual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1_VLDR actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc1101ud01nnnndddd1010iiiiiiii");
}

// opcode(24:20)=1xx01 & S(8)=1
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1,
//       base: Rn,
//       baseline: VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: cccc1101ud01nnnndddd1011iiiiiiii,
//       rule: VLDR,
//       uses: {Rn},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0Tester_Case19_TestCase19) {
  VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0Tester_Case19 baseline_tester;
  NamedActual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1_VLDR actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc1101ud01nnnndddd1011iiiiiiii");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
