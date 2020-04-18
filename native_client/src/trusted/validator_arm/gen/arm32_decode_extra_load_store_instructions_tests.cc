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


// op2(6:5)=01 & op1(24:20)=xx0x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w0nnnntttt00001011mmmm,
//       rule: STRH_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rt, Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~01
  if ((inst.Bits() & 0x00000060)  !=
          0x00000020) return false;
  // op1(24:20)=~xx0x0
  if ((inst.Bits() & 0x00500000)  !=
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

// op2(6:5)=01 & op1(24:20)=xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w1nnnntttt00001011mmmm,
//       rule: LDRH_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~01
  if ((inst.Bits() & 0x00000060)  !=
          0x00000020) return false;
  // op1(24:20)=~xx0x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00100000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op2(6:5)=01 & op1(24:20)=xx1x0
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w0nnnnttttiiii1011iiii,
//       rule: STRH_immediate,
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
//       uses: {Rt, Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~01
  if ((inst.Bits() & 0x00000060)  !=
          0x00000020) return false;
  // op1(24:20)=~xx1x0
  if ((inst.Bits() & 0x00500000)  !=
          0x00400000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op2(6:5)=01 & op1(24:20)=xx1x1 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w1nnnnttttiiii1011iiii,
//       rule: LDRH_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc ||
//            (wback &&
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~01
  if ((inst.Bits() & 0x00000060)  !=
          0x00000020) return false;
  // op1(24:20)=~xx1x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00500000) return false;
  // Rn(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op2(6:5)=01 & op1(24:20)=xx1x1 & Rn(19:16)=1111
//    = {P: P(24),
//       Pc: 15,
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//       base: Pc,
//       baseline: LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0,
//       defs: {Rt},
//       fields: [P(24), W(21), Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc000pu1w11111ttttiiii1011iiii,
//       rule: LDRH_literal,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         P  ==
//               W => UNPREDICTABLE,
//         Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~01
  if ((inst.Bits() & 0x00000060)  !=
          0x00000020) return false;
  // op1(24:20)=~xx1x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00500000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op2(6:5)=10 & op1(24:20)=xx0x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1,
//       base: Rn,
//       baseline: LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0,
//       defs: {Rt, Rt2, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w0nnnntttt00001101mmmm,
//       rule: LDRD_register,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         Rt2  ==
//               Pc ||
//            Rm  ==
//               Pc ||
//            Rm  ==
//               Rt ||
//            Rm  ==
//               Rt2 => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~10
  if ((inst.Bits() & 0x00000060)  !=
          0x00000040) return false;
  // op1(24:20)=~xx0x0
  if ((inst.Bits() & 0x00500000)  !=
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

// op2(6:5)=10 & op1(24:20)=xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w1nnnntttt00001101mmmm,
//       rule: LDRSB_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~10
  if ((inst.Bits() & 0x00000060)  !=
          0x00000040) return false;
  // op1(24:20)=~xx0x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00100000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op2(6:5)=10 & op1(24:20)=xx1x0 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1,
//       base: Rn,
//       baseline: LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0,
//       defs: {Rt, Rt2, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w0nnnnttttiiii1101iiii,
//       rule: LDRD_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         Rt2  ==
//               Pc => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~10
  if ((inst.Bits() & 0x00000060)  !=
          0x00000040) return false;
  // op1(24:20)=~xx1x0
  if ((inst.Bits() & 0x00500000)  !=
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

// op2(6:5)=10 & op1(24:20)=xx1x0 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {Pc: 15,
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       actual: Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1,
//       base: Pc,
//       baseline: LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0,
//       defs: {Rt, Rt2},
//       fields: [Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0001u1001111ttttiiii1101iiii,
//       rule: LDRD_literal,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         Rt2  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~10
  if ((inst.Bits() & 0x00000060)  !=
          0x00000040) return false;
  // op1(24:20)=~xx1x0
  if ((inst.Bits() & 0x00500000)  !=
          0x00400000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x01200000)  !=
          0x01000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op2(6:5)=10 & op1(24:20)=xx1x1 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w1nnnnttttiiii1101iiii,
//       rule: LDRSB_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc ||
//            (wback &&
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~10
  if ((inst.Bits() & 0x00000060)  !=
          0x00000040) return false;
  // op1(24:20)=~xx1x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00500000) return false;
  // Rn(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op2(6:5)=10 & op1(24:20)=xx1x1 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {P: P(24),
//       Pc: 15,
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//       base: Pc,
//       baseline: LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0,
//       defs: {Rt},
//       fields: [P(24), W(21), Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0001u1011111ttttiiii1101iiii,
//       rule: LDRSB_literal,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         P  ==
//               W => UNPREDICTABLE,
//         Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~10
  if ((inst.Bits() & 0x00000060)  !=
          0x00000040) return false;
  // op1(24:20)=~xx1x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00500000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x01200000)  !=
          0x01000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op2(6:5)=11 & op1(24:20)=xx0x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1,
//       base: Rn,
//       baseline: STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w0nnnntttt00001111mmmm,
//       rule: STRD_register,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         Rt2  ==
//               Pc ||
//            Rm  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rt, Rt2, Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~11
  if ((inst.Bits() & 0x00000060)  !=
          0x00000060) return false;
  // op1(24:20)=~xx0x0
  if ((inst.Bits() & 0x00500000)  !=
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

// op2(6:5)=11 & op1(24:20)=xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w1nnnntttt00001111mmmm,
//       rule: LDRSH_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~11
  if ((inst.Bits() & 0x00000060)  !=
          0x00000060) return false;
  // op1(24:20)=~xx0x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00100000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op2(6:5)=11 & op1(24:20)=xx1x0
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1,
//       base: Rn,
//       baseline: STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w0nnnnttttiiii1111iiii,
//       rule: STRD_immediate,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         Rt2  ==
//               Pc => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rt, Rt2, Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~11
  if ((inst.Bits() & 0x00000060)  !=
          0x00000060) return false;
  // op1(24:20)=~xx1x0
  if ((inst.Bits() & 0x00500000)  !=
          0x00400000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op2(6:5)=11 & op1(24:20)=xx1x1 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w1nnnnttttiiii1111iiii,
//       rule: LDRSH_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc ||
//            (wback &&
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~11
  if ((inst.Bits() & 0x00000060)  !=
          0x00000060) return false;
  // op1(24:20)=~xx1x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00500000) return false;
  // Rn(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op2(6:5)=11 & op1(24:20)=xx1x1 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {P: P(24),
//       Pc: 15,
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//       base: Pc,
//       baseline: LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0,
//       defs: {Rt},
//       fields: [P(24), W(21), Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0001u1011111ttttiiii1111iiii,
//       rule: LDRSH_literal,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         P  ==
//               W => UNPREDICTABLE,
//         Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op2(6:5)=~11
  if ((inst.Bits() & 0x00000060)  !=
          0x00000060) return false;
  // op1(24:20)=~xx1x1
  if ((inst.Bits() & 0x00500000)  !=
          0x00500000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x01200000)  !=
          0x01000000) return false;

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

// op2(6:5)=01 & op1(24:20)=xx0x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w0nnnntttt00001011mmmm,
//       rule: STRH_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rt, Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0Tester_Case0
    : public STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0TesterCase0 {
 public:
  STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0Tester_Case0()
    : STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0TesterCase0(
      state_.STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0_STRH_register_instance_)
  {}
};

// op2(6:5)=01 & op1(24:20)=xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w1nnnntttt00001011mmmm,
//       rule: LDRH_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0Tester_Case1
    : public LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0TesterCase1 {
 public:
  LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0Tester_Case1()
    : LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0TesterCase1(
      state_.LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0_LDRH_register_instance_)
  {}
};

// op2(6:5)=01 & op1(24:20)=xx1x0
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w0nnnnttttiiii1011iiii,
//       rule: STRH_immediate,
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
//       uses: {Rt, Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0Tester_Case2
    : public STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0TesterCase2 {
 public:
  STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0Tester_Case2()
    : STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0TesterCase2(
      state_.STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0_STRH_immediate_instance_)
  {}
};

// op2(6:5)=01 & op1(24:20)=xx1x1 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w1nnnnttttiiii1011iiii,
//       rule: LDRH_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc ||
//            (wback &&
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0Tester_Case3
    : public LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0TesterCase3 {
 public:
  LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0Tester_Case3()
    : LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0TesterCase3(
      state_.LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0_LDRH_immediate_instance_)
  {}
};

// op2(6:5)=01 & op1(24:20)=xx1x1 & Rn(19:16)=1111
//    = {P: P(24),
//       Pc: 15,
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//       base: Pc,
//       baseline: LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0,
//       defs: {Rt},
//       fields: [P(24), W(21), Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc000pu1w11111ttttiiii1011iiii,
//       rule: LDRH_literal,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         P  ==
//               W => UNPREDICTABLE,
//         Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0Tester_Case4
    : public LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0TesterCase4 {
 public:
  LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0Tester_Case4()
    : LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0TesterCase4(
      state_.LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0_LDRH_literal_instance_)
  {}
};

// op2(6:5)=10 & op1(24:20)=xx0x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1,
//       base: Rn,
//       baseline: LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0,
//       defs: {Rt, Rt2, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w0nnnntttt00001101mmmm,
//       rule: LDRD_register,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         Rt2  ==
//               Pc ||
//            Rm  ==
//               Pc ||
//            Rm  ==
//               Rt ||
//            Rm  ==
//               Rt2 => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0Tester_Case5
    : public LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0TesterCase5 {
 public:
  LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0Tester_Case5()
    : LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0TesterCase5(
      state_.LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0_LDRD_register_instance_)
  {}
};

// op2(6:5)=10 & op1(24:20)=xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w1nnnntttt00001101mmmm,
//       rule: LDRSB_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0Tester_Case6
    : public LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0TesterCase6 {
 public:
  LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0Tester_Case6()
    : LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0TesterCase6(
      state_.LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0_LDRSB_register_instance_)
  {}
};

// op2(6:5)=10 & op1(24:20)=xx1x0 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1,
//       base: Rn,
//       baseline: LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0,
//       defs: {Rt, Rt2, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w0nnnnttttiiii1101iiii,
//       rule: LDRD_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         Rt2  ==
//               Pc => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0Tester_Case7
    : public LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0TesterCase7 {
 public:
  LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0Tester_Case7()
    : LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0TesterCase7(
      state_.LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0_LDRD_immediate_instance_)
  {}
};

// op2(6:5)=10 & op1(24:20)=xx1x0 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {Pc: 15,
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       actual: Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1,
//       base: Pc,
//       baseline: LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0,
//       defs: {Rt, Rt2},
//       fields: [Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0001u1001111ttttiiii1101iiii,
//       rule: LDRD_literal,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         Rt2  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0Tester_Case8
    : public LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0TesterCase8 {
 public:
  LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0Tester_Case8()
    : LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0TesterCase8(
      state_.LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0_LDRD_literal_instance_)
  {}
};

// op2(6:5)=10 & op1(24:20)=xx1x1 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w1nnnnttttiiii1101iiii,
//       rule: LDRSB_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc ||
//            (wback &&
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0Tester_Case9
    : public LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0TesterCase9 {
 public:
  LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0Tester_Case9()
    : LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0TesterCase9(
      state_.LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0_LDRSB_immediate_instance_)
  {}
};

// op2(6:5)=10 & op1(24:20)=xx1x1 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {P: P(24),
//       Pc: 15,
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//       base: Pc,
//       baseline: LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0,
//       defs: {Rt},
//       fields: [P(24), W(21), Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0001u1011111ttttiiii1101iiii,
//       rule: LDRSB_literal,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         P  ==
//               W => UNPREDICTABLE,
//         Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0Tester_Case10
    : public LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0TesterCase10 {
 public:
  LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0Tester_Case10()
    : LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0TesterCase10(
      state_.LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0_LDRSB_literal_instance_)
  {}
};

// op2(6:5)=11 & op1(24:20)=xx0x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1,
//       base: Rn,
//       baseline: STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w0nnnntttt00001111mmmm,
//       rule: STRD_register,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         Rt2  ==
//               Pc ||
//            Rm  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rt, Rt2, Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0Tester_Case11
    : public STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0TesterCase11 {
 public:
  STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0Tester_Case11()
    : STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0TesterCase11(
      state_.STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0_STRD_register_instance_)
  {}
};

// op2(6:5)=11 & op1(24:20)=xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w1nnnntttt00001111mmmm,
//       rule: LDRSH_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0Tester_Case12
    : public LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0TesterCase12 {
 public:
  LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0Tester_Case12()
    : LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0TesterCase12(
      state_.LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0_LDRSH_register_instance_)
  {}
};

// op2(6:5)=11 & op1(24:20)=xx1x0
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1,
//       base: Rn,
//       baseline: STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w0nnnnttttiiii1111iiii,
//       rule: STRD_immediate,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         Rt2  ==
//               Pc => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rt, Rt2, Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0Tester_Case13
    : public STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0TesterCase13 {
 public:
  STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0Tester_Case13()
    : STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0TesterCase13(
      state_.STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0_STRD_immediate_instance_)
  {}
};

// op2(6:5)=11 & op1(24:20)=xx1x1 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w1nnnnttttiiii1111iiii,
//       rule: LDRSH_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc ||
//            (wback &&
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
class LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0Tester_Case14
    : public LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0TesterCase14 {
 public:
  LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0Tester_Case14()
    : LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0TesterCase14(
      state_.LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0_LDRSH_immediate_instance_)
  {}
};

// op2(6:5)=11 & op1(24:20)=xx1x1 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {P: P(24),
//       Pc: 15,
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//       base: Pc,
//       baseline: LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0,
//       defs: {Rt},
//       fields: [P(24), W(21), Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0001u1011111ttttiiii1111iiii,
//       rule: LDRSH_literal,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         P  ==
//               W => UNPREDICTABLE,
//         Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0Tester_Case15
    : public LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0TesterCase15 {
 public:
  LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0Tester_Case15()
    : LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0TesterCase15(
      state_.LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0_LDRSH_literal_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op2(6:5)=01 & op1(24:20)=xx0x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w0nnnntttt00001011mmmm,
//       rule: STRH_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rt, Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0Tester_Case0_TestCase0) {
  STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1_STRH_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu0w0nnnntttt00001011mmmm");
}

// op2(6:5)=01 & op1(24:20)=xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w1nnnntttt00001011mmmm,
//       rule: LDRH_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0Tester_Case1_TestCase1) {
  LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1_LDRH_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu0w1nnnntttt00001011mmmm");
}

// op2(6:5)=01 & op1(24:20)=xx1x0
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w0nnnnttttiiii1011iiii,
//       rule: STRH_immediate,
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
//       uses: {Rt, Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0Tester_Case2_TestCase2) {
  STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0Tester_Case2 baseline_tester;
  NamedActual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1_STRH_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu1w0nnnnttttiiii1011iiii");
}

// op2(6:5)=01 & op1(24:20)=xx1x1 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w1nnnnttttiiii1011iiii,
//       rule: LDRH_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc ||
//            (wback &&
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0Tester_Case3_TestCase3) {
  LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0Tester_Case3 baseline_tester;
  NamedActual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1_LDRH_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu1w1nnnnttttiiii1011iiii");
}

// op2(6:5)=01 & op1(24:20)=xx1x1 & Rn(19:16)=1111
//    = {P: P(24),
//       Pc: 15,
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//       base: Pc,
//       baseline: LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0,
//       defs: {Rt},
//       fields: [P(24), W(21), Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc000pu1w11111ttttiiii1011iiii,
//       rule: LDRH_literal,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         P  ==
//               W => UNPREDICTABLE,
//         Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0Tester_Case4_TestCase4) {
  LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0Tester_Case4 baseline_tester;
  NamedActual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1_LDRH_literal actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu1w11111ttttiiii1011iiii");
}

// op2(6:5)=10 & op1(24:20)=xx0x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1,
//       base: Rn,
//       baseline: LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0,
//       defs: {Rt, Rt2, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w0nnnntttt00001101mmmm,
//       rule: LDRD_register,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         Rt2  ==
//               Pc ||
//            Rm  ==
//               Pc ||
//            Rm  ==
//               Rt ||
//            Rm  ==
//               Rt2 => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0Tester_Case5_TestCase5) {
  LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0Tester_Case5 baseline_tester;
  NamedActual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1_LDRD_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu0w0nnnntttt00001101mmmm");
}

// op2(6:5)=10 & op1(24:20)=xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w1nnnntttt00001101mmmm,
//       rule: LDRSB_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0Tester_Case6_TestCase6) {
  LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1_LDRSB_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu0w1nnnntttt00001101mmmm");
}

// op2(6:5)=10 & op1(24:20)=xx1x0 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1,
//       base: Rn,
//       baseline: LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0,
//       defs: {Rt, Rt2, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w0nnnnttttiiii1101iiii,
//       rule: LDRD_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         Rt2  ==
//               Pc => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0Tester_Case7_TestCase7) {
  LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0Tester_Case7 baseline_tester;
  NamedActual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1_LDRD_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu1w0nnnnttttiiii1101iiii");
}

// op2(6:5)=10 & op1(24:20)=xx1x0 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {Pc: 15,
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       actual: Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1,
//       base: Pc,
//       baseline: LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0,
//       defs: {Rt, Rt2},
//       fields: [Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0001u1001111ttttiiii1101iiii,
//       rule: LDRD_literal,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         Rt2  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0Tester_Case8_TestCase8) {
  LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0Tester_Case8 baseline_tester;
  NamedActual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1_LDRD_literal actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0001u1001111ttttiiii1101iiii");
}

// op2(6:5)=10 & op1(24:20)=xx1x1 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w1nnnnttttiiii1101iiii,
//       rule: LDRSB_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc ||
//            (wback &&
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0Tester_Case9_TestCase9) {
  LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0Tester_Case9 baseline_tester;
  NamedActual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1_LDRSB_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu1w1nnnnttttiiii1101iiii");
}

// op2(6:5)=10 & op1(24:20)=xx1x1 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {P: P(24),
//       Pc: 15,
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//       base: Pc,
//       baseline: LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0,
//       defs: {Rt},
//       fields: [P(24), W(21), Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0001u1011111ttttiiii1101iiii,
//       rule: LDRSB_literal,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         P  ==
//               W => UNPREDICTABLE,
//         Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0Tester_Case10_TestCase10) {
  LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0Tester_Case10 baseline_tester;
  NamedActual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1_LDRSB_literal actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0001u1011111ttttiiii1101iiii");
}

// op2(6:5)=11 & op1(24:20)=xx0x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1,
//       base: Rn,
//       baseline: STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w0nnnntttt00001111mmmm,
//       rule: STRD_register,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         Rt2  ==
//               Pc ||
//            Rm  ==
//               Pc => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         ArchVersion()  <
//               6 &&
//            wback &&
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rt, Rt2, Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0Tester_Case11_TestCase11) {
  STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0Tester_Case11 baseline_tester;
  NamedActual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1_STRD_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu0w0nnnntttt00001111mmmm");
}

// op2(6:5)=11 & op1(24:20)=xx0x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//       base: Rn,
//       baseline: LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12), Rm(3:0)],
//       index: P(24)=1,
//       pattern: cccc000pu0w1nnnntttt00001111mmmm,
//       rule: LDRSH_register,
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
//            Rm  ==
//               Rn => UNPREDICTABLE,
//         index => FORBIDDEN],
//       uses: {Rn, Rm},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0Tester_Case12_TestCase12) {
  LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0Tester_Case12 baseline_tester;
  NamedActual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1_LDRSH_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu0w1nnnntttt00001111mmmm");
}

// op2(6:5)=11 & op1(24:20)=xx1x0
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       Rt2: Rt + 1,
//       W: W(21),
//       actual: Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1,
//       base: Rn,
//       baseline: STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0,
//       defs: {base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w0nnnnttttiiii1111iiii,
//       rule: STRD_immediate,
//       safety: [Rt(0)=1 => UNPREDICTABLE,
//         P(24)=0 &&
//            W(21)=1 => UNPREDICTABLE,
//         wback &&
//            (Rn  ==
//               Pc ||
//            Rn  ==
//               Rt ||
//            Rn  ==
//               Rt2) => UNPREDICTABLE,
//         Rt2  ==
//               Pc => UNPREDICTABLE],
//       small_imm_base_wb: wback,
//       uses: {Rt, Rt2, Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0Tester_Case13_TestCase13) {
  STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0Tester_Case13 baseline_tester;
  NamedActual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1_STRD_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu1w0nnnnttttiiii1111iiii");
}

// op2(6:5)=11 & op1(24:20)=xx1x1 & Rn(19:16)=~1111
//    = {None: 32,
//       P: P(24),
//       Pc: 15,
//       Rn: Rn(19:16),
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//       base: Rn,
//       baseline: LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0,
//       defs: {Rt, base
//            if wback
//            else None},
//       fields: [P(24), W(21), Rn(19:16), Rt(15:12)],
//       pattern: cccc000pu1w1nnnnttttiiii1111iiii,
//       rule: LDRSH_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         Rt  ==
//               Pc ||
//            (wback &&
//            Rn  ==
//               Rt) => UNPREDICTABLE,
//         Rt  ==
//               Pc => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: (P(24)=0) ||
//            (W(21)=1)}
TEST_F(Arm32DecoderStateTests,
       LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0Tester_Case14_TestCase14) {
  LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0Tester_Case14 baseline_tester;
  NamedActual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1_LDRSH_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc000pu1w1nnnnttttiiii1111iiii");
}

// op2(6:5)=11 & op1(24:20)=xx1x1 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx
//    = {P: P(24),
//       Pc: 15,
//       Rt: Rt(15:12),
//       W: W(21),
//       actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//       base: Pc,
//       baseline: LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0,
//       defs: {Rt},
//       fields: [P(24), W(21), Rt(15:12)],
//       is_literal_load: true,
//       pattern: cccc0001u1011111ttttiiii1111iiii,
//       rule: LDRSH_literal,
//       safety: [P(24)=0 &&
//            W(21)=1 => DECODER_ERROR,
//         P  ==
//               W => UNPREDICTABLE,
//         Rt  ==
//               Pc => UNPREDICTABLE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0Tester_Case15_TestCase15) {
  LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0Tester_Case15 baseline_tester;
  NamedActual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1_LDRSH_literal actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0001u1011111ttttiiii1111iiii");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
