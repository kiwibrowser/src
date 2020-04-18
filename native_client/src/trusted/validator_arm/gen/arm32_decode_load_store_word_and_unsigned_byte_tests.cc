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


// A(25)=0 & op1(24:20)=0x010
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u010nnnnttttiiiiiiiiiiii,
//       rule: STRT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~0x010
  if ((inst.Bits() & 0x01700000)  !=
          0x00200000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=0 & op1(24:20)=0x011
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u011nnnnttttiiiiiiiiiiii,
//       rule: LDRT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~0x011
  if ((inst.Bits() & 0x01700000)  !=
          0x00300000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=0 & op1(24:20)=0x110
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u110nnnnttttiiiiiiiiiiii,
//       rule: STRBT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~0x110
  if ((inst.Bits() & 0x01700000)  !=
          0x00600000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=0 & op1(24:20)=0x111
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u111nnnnttttiiiiiiiiiiii,
//       rule: LDRBT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~0x111
  if ((inst.Bits() & 0x01700000)  !=
          0x00700000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=0 & op1(24:20)=xx0x0 & op1_repeated(24:20)=~0x010
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc010pu0w0nnnnttttiiiiiiiiiiii,
//       rule: STR_immediate,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~xx0x0
  if ((inst.Bits() & 0x00500000)  !=
          0x00000000) return false;
  // op1_repeated(24:20)=0x010
  if ((inst.Bits() & 0x01700000)  ==
          0x00200000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=0 & op1(24:20)=xx0x1 & Rn(19:16)=~1111 & op1_repeated(24:20)=~0x011
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Tp: 9,
//       U: U(23),
//       W: W(21),
//       actual: Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1,
//       add: U(23)=1,
//       base: Rn,
//       baseline: LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), U(23), W(21), Rn(19:16), Rt(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       index: P(24)=1,
//       is_load_tp: Rn  ==
//               Tp &&
//            index &&
//            not wback &&
//            add &&
//            imm12 in {0, 4},
//       pattern: cccc010pu0w1nnnnttttiiiiiiiiiiii,
//       rule: LDR_immediate,
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         wback &&
//            Rn  ==
//               Rt => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~xx0x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00100000) return false;
  // Rn(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // op1_repeated(24:20)=0x011
  if ((inst.Bits() & 0x01700000)  ==
          0x00300000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=0 & op1(24:20)=xx0x1 & Rn(19:16)=1111 & op1_repeated(24:20)=~0x011 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {Pc: 15,
//       Rt: Rt(15:12),
//       actual: Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1,
//       base: Pc,
//       baseline: LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0,
//       defs: {Rt},
//       fields: [Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0101u0011111ttttiiiiiiiiiiii,
//       rule: LDR_literal,
//       safety: [Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~xx0x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00100000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // op1_repeated(24:20)=0x011
  if ((inst.Bits() & 0x01700000)  ==
          0x00300000) return false;
  // $pattern(31:0)=~xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x01200000)  !=
          0x01000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=0 & op1(24:20)=xx1x0 & op1_repeated(24:20)=~0x110
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc010pu1w0nnnnttttiiiiiiiiiiii,
//       rule: STRB_immediate,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~xx1x0
  if ((inst.Bits() & 0x00500000)  !=
          0x00400000) return false;
  // op1_repeated(24:20)=0x110
  if ((inst.Bits() & 0x01700000)  ==
          0x00600000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=0 & op1(24:20)=xx1x1 & Rn(19:16)=~1111 & op1_repeated(24:20)=~0x111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc010pu1w1nnnnttttiiiiiiiiiiii,
//       rule: LDRB_immediate,
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            Rn  ==
//               Rt => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~xx1x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00500000) return false;
  // Rn(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // op1_repeated(24:20)=0x111
  if ((inst.Bits() & 0x01700000)  ==
          0x00700000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=0 & op1(24:20)=xx1x1 & Rn(19:16)=1111 & op1_repeated(24:20)=~0x111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {Pc: 15,
//       Rt: Rt(15:12),
//       actual: Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1,
//       base: Pc,
//       baseline: LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0,
//       defs: {Rt},
//       fields: [Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0101u1011111ttttiiiiiiiiiiii,
//       rule: LDRB_literal,
//       safety: [Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~0
  if ((inst.Bits() & 0x02000000)  !=
          0x00000000) return false;
  // op1(24:20)=~xx1x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00500000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // op1_repeated(24:20)=0x111
  if ((inst.Bits() & 0x01700000)  ==
          0x00700000) return false;
  // $pattern(31:0)=~xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x01200000)  !=
          0x01000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=1 & op1(24:20)=0x010 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u010nnnnttttiiiiitt0mmmm,
//       rule: STRT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~1
  if ((inst.Bits() & 0x02000000)  !=
          0x02000000) return false;
  // op1(24:20)=~0x010
  if ((inst.Bits() & 0x01700000)  !=
          0x00200000) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=1 & op1(24:20)=0x011 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u011nnnnttttiiiiitt0mmmm,
//       rule: LDRT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~1
  if ((inst.Bits() & 0x02000000)  !=
          0x02000000) return false;
  // op1(24:20)=~0x011
  if ((inst.Bits() & 0x01700000)  !=
          0x00300000) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=1 & op1(24:20)=0x110 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u110nnnnttttiiiiitt0mmmm,
//       rule: STRBT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~1
  if ((inst.Bits() & 0x02000000)  !=
          0x02000000) return false;
  // op1(24:20)=~0x110
  if ((inst.Bits() & 0x01700000)  !=
          0x00600000) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=1 & op1(24:20)=0x111 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u111nnnnttttiiiiitt0mmmm,
//       rule: LDRBT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~1
  if ((inst.Bits() & 0x02000000)  !=
          0x02000000) return false;
  // op1(24:20)=~0x111
  if ((inst.Bits() & 0x01700000)  !=
          0x00700000) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=1 & op1(24:20)=xx0x0 & B(4)=0 & op1_repeated(24:20)=~0x010
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pd0w0nnnnttttiiiiitt0mmmm,
//       rule: STR_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rm  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rm, Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~1
  if ((inst.Bits() & 0x02000000)  !=
          0x02000000) return false;
  // op1(24:20)=~xx0x0
  if ((inst.Bits() & 0x00500000)  !=
          0x00000000) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // op1_repeated(24:20)=0x010
  if ((inst.Bits() & 0x01700000)  ==
          0x00200000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=1 & op1(24:20)=xx0x1 & B(4)=0 & op1_repeated(24:20)=~0x011
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pu0w1nnnnttttiiiiitt0mmmm,
//       rule: LDR_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rm  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       uses: {Rm, Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~1
  if ((inst.Bits() & 0x02000000)  !=
          0x02000000) return false;
  // op1(24:20)=~xx0x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00100000) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // op1_repeated(24:20)=0x011
  if ((inst.Bits() & 0x01700000)  ==
          0x00300000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=1 & op1(24:20)=xx1x0 & B(4)=0 & op1_repeated(24:20)=~0x110
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pu1w0nnnnttttiiiiitt0mmmm,
//       rule: STRB_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Pc in {Rm, Rt} => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rm, Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0TesterCase16
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~1
  if ((inst.Bits() & 0x02000000)  !=
          0x02000000) return false;
  // op1(24:20)=~xx1x0
  if ((inst.Bits() & 0x00500000)  !=
          0x00400000) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // op1_repeated(24:20)=0x110
  if ((inst.Bits() & 0x01700000)  ==
          0x00600000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(25)=1 & op1(24:20)=xx1x1 & B(4)=0 & op1_repeated(24:20)=~0x111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pu1w1nnnnttttiiiiitt0mmmm,
//       rule: LDRB_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Pc in {Rt, Rm} => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rm, Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0TesterCase17
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(25)=~1
  if ((inst.Bits() & 0x02000000)  !=
          0x02000000) return false;
  // op1(24:20)=~xx1x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00500000) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // op1_repeated(24:20)=0x111
  if ((inst.Bits() & 0x01700000)  ==
          0x00700000) return false;

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

// A(25)=0 & op1(24:20)=0x010
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u010nnnnttttiiiiiiiiiiii,
//       rule: STRT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0Tester_Case0
    : public STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0TesterCase0 {
 public:
  STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0Tester_Case0()
    : STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0TesterCase0(
      state_.STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0_STRT_A1_instance_)
  {}
};

// A(25)=0 & op1(24:20)=0x011
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u011nnnnttttiiiiiiiiiiii,
//       rule: LDRT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0Tester_Case1
    : public LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0TesterCase1 {
 public:
  LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0Tester_Case1()
    : LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0TesterCase1(
      state_.LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0_LDRT_A1_instance_)
  {}
};

// A(25)=0 & op1(24:20)=0x110
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u110nnnnttttiiiiiiiiiiii,
//       rule: STRBT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0Tester_Case2
    : public STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0TesterCase2 {
 public:
  STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0Tester_Case2()
    : STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0TesterCase2(
      state_.STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0_STRBT_A1_instance_)
  {}
};

// A(25)=0 & op1(24:20)=0x111
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u111nnnnttttiiiiiiiiiiii,
//       rule: LDRBT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0Tester_Case3
    : public LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0TesterCase3 {
 public:
  LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0Tester_Case3()
    : LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0TesterCase3(
      state_.LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0_LDRBT_A1_instance_)
  {}
};

// A(25)=0 & op1(24:20)=xx0x0 & op1_repeated(24:20)=~0x010
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc010pu0w0nnnnttttiiiiiiiiiiii,
//       rule: STR_immediate,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0Tester_Case4
    : public STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0TesterCase4 {
 public:
  STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0Tester_Case4()
    : STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0TesterCase4(
      state_.STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0_STR_immediate_instance_)
  {}
};

// A(25)=0 & op1(24:20)=xx0x1 & Rn(19:16)=~1111 & op1_repeated(24:20)=~0x011
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Tp: 9,
//       U: U(23),
//       W: W(21),
//       actual: Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1,
//       add: U(23)=1,
//       base: Rn,
//       baseline: LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), U(23), W(21), Rn(19:16), Rt(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       index: P(24)=1,
//       is_load_tp: Rn  ==
//               Tp &&
//            index &&
//            not wback &&
//            add &&
//            imm12 in {0, 4},
//       pattern: cccc010pu0w1nnnnttttiiiiiiiiiiii,
//       rule: LDR_immediate,
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         wback &&
//            Rn  ==
//               Rt => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0Tester_Case5
    : public LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0TesterCase5 {
 public:
  LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0Tester_Case5()
    : LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0TesterCase5(
      state_.LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0_LDR_immediate_instance_)
  {}
};

// A(25)=0 & op1(24:20)=xx0x1 & Rn(19:16)=1111 & op1_repeated(24:20)=~0x011 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {Pc: 15,
//       Rt: Rt(15:12),
//       actual: Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1,
//       base: Pc,
//       baseline: LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0,
//       defs: {Rt},
//       fields: [Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0101u0011111ttttiiiiiiiiiiii,
//       rule: LDR_literal,
//       safety: [Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0Tester_Case6
    : public LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0TesterCase6 {
 public:
  LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0Tester_Case6()
    : LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0TesterCase6(
      state_.LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0_LDR_literal_instance_)
  {}
};

// A(25)=0 & op1(24:20)=xx1x0 & op1_repeated(24:20)=~0x110
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc010pu1w0nnnnttttiiiiiiiiiiii,
//       rule: STRB_immediate,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0Tester_Case7
    : public STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0TesterCase7 {
 public:
  STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0Tester_Case7()
    : STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0TesterCase7(
      state_.STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0_STRB_immediate_instance_)
  {}
};

// A(25)=0 & op1(24:20)=xx1x1 & Rn(19:16)=~1111 & op1_repeated(24:20)=~0x111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc010pu1w1nnnnttttiiiiiiiiiiii,
//       rule: LDRB_immediate,
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            Rn  ==
//               Rt => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0Tester_Case8
    : public LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0TesterCase8 {
 public:
  LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0Tester_Case8()
    : LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0TesterCase8(
      state_.LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0_LDRB_immediate_instance_)
  {}
};

// A(25)=0 & op1(24:20)=xx1x1 & Rn(19:16)=1111 & op1_repeated(24:20)=~0x111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {Pc: 15,
//       Rt: Rt(15:12),
//       actual: Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1,
//       base: Pc,
//       baseline: LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0,
//       defs: {Rt},
//       fields: [Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0101u1011111ttttiiiiiiiiiiii,
//       rule: LDRB_literal,
//       safety: [Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0Tester_Case9
    : public LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0TesterCase9 {
 public:
  LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0Tester_Case9()
    : LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0TesterCase9(
      state_.LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0_LDRB_literal_instance_)
  {}
};

// A(25)=1 & op1(24:20)=0x010 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u010nnnnttttiiiiitt0mmmm,
//       rule: STRT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0Tester_Case10
    : public STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0TesterCase10 {
 public:
  STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0Tester_Case10()
    : STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0TesterCase10(
      state_.STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0_STRT_A2_instance_)
  {}
};

// A(25)=1 & op1(24:20)=0x011 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u011nnnnttttiiiiitt0mmmm,
//       rule: LDRT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0Tester_Case11
    : public LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0TesterCase11 {
 public:
  LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0Tester_Case11()
    : LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0TesterCase11(
      state_.LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0_LDRT_A2_instance_)
  {}
};

// A(25)=1 & op1(24:20)=0x110 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u110nnnnttttiiiiitt0mmmm,
//       rule: STRBT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0Tester_Case12
    : public STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0TesterCase12 {
 public:
  STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0Tester_Case12()
    : STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0TesterCase12(
      state_.STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0_STRBT_A2_instance_)
  {}
};

// A(25)=1 & op1(24:20)=0x111 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u111nnnnttttiiiiitt0mmmm,
//       rule: LDRBT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0Tester_Case13
    : public LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0TesterCase13 {
 public:
  LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0Tester_Case13()
    : LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0TesterCase13(
      state_.LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0_LDRBT_A2_instance_)
  {}
};

// A(25)=1 & op1(24:20)=xx0x0 & B(4)=0 & op1_repeated(24:20)=~0x010
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pd0w0nnnnttttiiiiitt0mmmm,
//       rule: STR_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rm  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rm, Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0Tester_Case14
    : public STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0TesterCase14 {
 public:
  STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0Tester_Case14()
    : STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0TesterCase14(
      state_.STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0_STR_register_instance_)
  {}
};

// A(25)=1 & op1(24:20)=xx0x1 & B(4)=0 & op1_repeated(24:20)=~0x011
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pu0w1nnnnttttiiiiitt0mmmm,
//       rule: LDR_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rm  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       uses: {Rm, Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0Tester_Case15
    : public LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0TesterCase15 {
 public:
  LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0Tester_Case15()
    : LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0TesterCase15(
      state_.LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0_LDR_register_instance_)
  {}
};

// A(25)=1 & op1(24:20)=xx1x0 & B(4)=0 & op1_repeated(24:20)=~0x110
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pu1w0nnnnttttiiiiitt0mmmm,
//       rule: STRB_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Pc in {Rm, Rt} => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rm, Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0Tester_Case16
    : public STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0TesterCase16 {
 public:
  STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0Tester_Case16()
    : STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0TesterCase16(
      state_.STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0_STRB_register_instance_)
  {}
};

// A(25)=1 & op1(24:20)=xx1x1 & B(4)=0 & op1_repeated(24:20)=~0x111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pu1w1nnnnttttiiiiitt0mmmm,
//       rule: LDRB_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Pc in {Rt, Rm} => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rm, Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
class LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0Tester_Case17
    : public LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0TesterCase17 {
 public:
  LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0Tester_Case17()
    : LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0TesterCase17(
      state_.LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0_LDRB_register_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// A(25)=0 & op1(24:20)=0x010
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u010nnnnttttiiiiiiiiiiii,
//       rule: STRT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0Tester_Case0_TestCase0) {
  STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0Tester_Case0 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_STRT_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0100u010nnnnttttiiiiiiiiiiii");
}

// A(25)=0 & op1(24:20)=0x011
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u011nnnnttttiiiiiiiiiiii,
//       rule: LDRT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0Tester_Case1_TestCase1) {
  LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0Tester_Case1 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_LDRT_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0100u011nnnnttttiiiiiiiiiiii");
}

// A(25)=0 & op1(24:20)=0x110
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u110nnnnttttiiiiiiiiiiii,
//       rule: STRBT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0Tester_Case2_TestCase2) {
  STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0Tester_Case2 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_STRBT_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0100u110nnnnttttiiiiiiiiiiii");
}

// A(25)=0 & op1(24:20)=0x111
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0,
//       defs: {},
//       pattern: cccc0100u111nnnnttttiiiiiiiiiiii,
//       rule: LDRBT_A1,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0Tester_Case3_TestCase3) {
  LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0Tester_Case3 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_LDRBT_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0100u111nnnnttttiiiiiiiiiiii");
}

// A(25)=0 & op1(24:20)=xx0x0 & op1_repeated(24:20)=~0x010
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc010pu0w0nnnnttttiiiiiiiiiiii,
//       rule: STR_immediate,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
TEST_F(Arm32DecoderStateTests,
       STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0Tester_Case4_TestCase4) {
  STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0Tester_Case4 baseline_tester;
  NamedActual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1_STR_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc010pu0w0nnnnttttiiiiiiiiiiii");
}

// A(25)=0 & op1(24:20)=xx0x1 & Rn(19:16)=~1111 & op1_repeated(24:20)=~0x011
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Tp: 9,
//       U: U(23),
//       W: W(21),
//       actual: Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1,
//       add: U(23)=1,
//       base: Rn,
//       baseline: LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), U(23), W(21), Rn(19:16), Rt(15:12), imm12(11:0)],
//       imm12: imm12(11:0),
//       index: P(24)=1,
//       is_load_tp: Rn  ==
//               Tp &&
//            index &&
//            not wback &&
//            add &&
//            imm12 in {0, 4},
//       pattern: cccc010pu0w1nnnnttttiiiiiiiiiiii,
//       rule: LDR_immediate,
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         wback &&
//            Rn  ==
//               Rt => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
TEST_F(Arm32DecoderStateTests,
       LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0Tester_Case5_TestCase5) {
  LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0Tester_Case5 baseline_tester;
  NamedActual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1_LDR_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc010pu0w1nnnnttttiiiiiiiiiiii");
}

// A(25)=0 & op1(24:20)=xx0x1 & Rn(19:16)=1111 & op1_repeated(24:20)=~0x011 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {Pc: 15,
//       Rt: Rt(15:12),
//       actual: Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1,
//       base: Pc,
//       baseline: LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0,
//       defs: {Rt},
//       fields: [Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0101u0011111ttttiiiiiiiiiiii,
//       rule: LDR_literal,
//       safety: [Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0Tester_Case6_TestCase6) {
  LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0Tester_Case6 baseline_tester;
  NamedActual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1_LDR_literal actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0101u0011111ttttiiiiiiiiiiii");
}

// A(25)=0 & op1(24:20)=xx1x0 & op1_repeated(24:20)=~0x110
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc010pu1w0nnnnttttiiiiiiiiiiii,
//       rule: STRB_immediate,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
TEST_F(Arm32DecoderStateTests,
       STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0Tester_Case7_TestCase7) {
  STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0Tester_Case7 baseline_tester;
  NamedActual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1_STRB_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc010pu1w0nnnnttttiiiiiiiiiiii");
}

// A(25)=0 & op1(24:20)=xx1x1 & Rn(19:16)=~1111 & op1_repeated(24:20)=~0x111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc010pu1w1nnnnttttiiiiiiiiiiii,
//       rule: LDRB_immediate,
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            Rn  ==
//               Rt => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
TEST_F(Arm32DecoderStateTests,
       LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0Tester_Case8_TestCase8) {
  LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0Tester_Case8 baseline_tester;
  NamedActual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1_LDRB_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc010pu1w1nnnnttttiiiiiiiiiiii");
}

// A(25)=0 & op1(24:20)=xx1x1 & Rn(19:16)=1111 & op1_repeated(24:20)=~0x111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {Pc: 15,
//       Rt: Rt(15:12),
//       actual: Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1,
//       base: Pc,
//       baseline: LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0,
//       defs: {Rt},
//       fields: [Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0101u1011111ttttiiiiiiiiiiii,
//       rule: LDRB_literal,
//       safety: [Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0Tester_Case9_TestCase9) {
  LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0Tester_Case9 baseline_tester;
  NamedActual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1_LDRB_literal actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0101u1011111ttttiiiiiiiiiiii");
}

// A(25)=1 & op1(24:20)=0x010 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u010nnnnttttiiiiitt0mmmm,
//       rule: STRT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0Tester_Case10_TestCase10) {
  STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0Tester_Case10 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_STRT_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0110u010nnnnttttiiiiitt0mmmm");
}

// A(25)=1 & op1(24:20)=0x011 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u011nnnnttttiiiiitt0mmmm,
//       rule: LDRT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0Tester_Case11_TestCase11) {
  LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0Tester_Case11 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_LDRT_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0110u011nnnnttttiiiiitt0mmmm");
}

// A(25)=1 & op1(24:20)=0x110 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u110nnnnttttiiiiitt0mmmm,
//       rule: STRBT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0Tester_Case12_TestCase12) {
  STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0Tester_Case12 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_STRBT_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0110u110nnnnttttiiiiitt0mmmm");
}

// A(25)=1 & op1(24:20)=0x111 & B(4)=0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0,
//       defs: {},
//       pattern: cccc0110u111nnnnttttiiiiitt0mmmm,
//       rule: LDRBT_A2,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0Tester_Case13_TestCase13) {
  LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0Tester_Case13 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_LDRBT_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0110u111nnnnttttiiiiitt0mmmm");
}

// A(25)=1 & op1(24:20)=xx0x0 & B(4)=0 & op1_repeated(24:20)=~0x010
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pd0w0nnnnttttiiiiitt0mmmm,
//       rule: STR_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rm  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rm, Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
TEST_F(Arm32DecoderStateTests,
       STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0Tester_Case14_TestCase14) {
  STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0Tester_Case14 baseline_tester;
  NamedActual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1_STR_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011pd0w0nnnnttttiiiiitt0mmmm");
}

// A(25)=1 & op1(24:20)=xx0x1 & B(4)=0 & op1_repeated(24:20)=~0x011
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pu0w1nnnnttttiiiiitt0mmmm,
//       rule: LDR_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rm  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       uses: {Rm, Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
TEST_F(Arm32DecoderStateTests,
       LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0Tester_Case15_TestCase15) {
  LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0Tester_Case15 baseline_tester;
  NamedActual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1_LDR_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011pu0w1nnnnttttiiiiitt0mmmm");
}

// A(25)=1 & op1(24:20)=xx1x0 & B(4)=0 & op1_repeated(24:20)=~0x110
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pu1w0nnnnttttiiiiitt0mmmm,
//       rule: STRB_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Pc in {Rm, Rt} => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rm, Rn, Rt},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
TEST_F(Arm32DecoderStateTests,
       STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0Tester_Case16_TestCase16) {
  STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0Tester_Case16 baseline_tester;
  NamedActual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1_STRB_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011pu1w0nnnnttttiiiiitt0mmmm");
}

// A(25)=1 & op1(24:20)=xx1x1 & B(4)=0 & op1_repeated(24:20)=~0x111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc011pu1w1nnnnttttiiiiitt0mmmm,
//       rule: LDRB_register,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Pc in {Rt, Rm} => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rn  ==
//               Rm => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rm, Rn},
//       violations: [implied by 'base'],
//       wback: P(24)=0 ||
//            W(21)=1}
TEST_F(Arm32DecoderStateTests,
       LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0Tester_Case17_TestCase17) {
  LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0Tester_Case17 baseline_tester;
  NamedActual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1_LDRB_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011pu1w1nnnnttttiiiiitt0mmmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
