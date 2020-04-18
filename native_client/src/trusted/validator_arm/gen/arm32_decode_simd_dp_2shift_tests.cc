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


// A(11:8)=0000
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0000lqm1mmmm,
//       rule: VSHR,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0001
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0001lqm1mmmm,
//       rule: VSRA,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000100) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0010
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0010lqm1mmmm,
//       rule: VRSHR,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000200) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0011
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0011lqm1mmmm,
//       rule: VRSRA,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000300) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0100 & U(24)=1
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100111diiiiiidddd0100lqm1mmmm,
//       rule: VSRI,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0100
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000400) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0101 & U(24)=0
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100101diiiiiidddd0101lqm1mmmm,
//       rule: VSHL_immediate,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000500) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0101 & U(24)=1
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100111diiiiiidddd0101lqm1mmmm,
//       rule: VSLI,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000500) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1000 & U(24)=0 & B(6)=0 & L(7)=0
//    = {Vm: Vm(3:0),
//       actual: Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1,
//       baseline: VSHRN_111100101diiiiiidddd100000m1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100101diiiiiidddd100000m1mmmm,
//       rule: VSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vm(0)=1 => UNDEFINED],
//       uses: {}}
class VSHRN_111100101diiiiiidddd100000m1mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  VSHRN_111100101diiiiiidddd100000m1mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSHRN_111100101diiiiiidddd100000m1mmmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000800) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // B(6)=~0
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;
  // L(7)=~0
  if ((inst.Bits() & 0x00000080)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1000 & U(24)=0 & B(6)=1 & L(7)=0
//    = {Vm: Vm(3:0),
//       actual: Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1,
//       baseline: VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100101diiiiiidddd100001m1mmmm,
//       rule: VRSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vm(0)=1 => UNDEFINED],
//       uses: {}}
class VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000800) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // B(6)=~1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;
  // L(7)=~0
  if ((inst.Bits() & 0x00000080)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1000 & U(24)=1 & B(6)=0 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//       rule: VQRSHRUN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000800) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // B(6)=~0
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;
  // L(7)=~0
  if ((inst.Bits() & 0x00000080)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1000 & U(24)=1 & B(6)=1 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//       rule: VQRSHRUN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000800) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // B(6)=~1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;
  // L(7)=~0
  if ((inst.Bits() & 0x00000080)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1001 & U(24)=0 & B(6)=0 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//       rule: VQSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000900) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // B(6)=~0
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;
  // L(7)=~0
  if ((inst.Bits() & 0x00000080)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1001 & U(24)=0 & B(6)=1 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//       rule: VQRSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000900) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // B(6)=~1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;
  // L(7)=~0
  if ((inst.Bits() & 0x00000080)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1001 & U(24)=1 & B(6)=0 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//       rule: VQSHRUN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000900) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // B(6)=~0
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;
  // L(7)=~0
  if ((inst.Bits() & 0x00000080)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1001 & U(24)=1 & B(6)=1 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//       rule: VQRSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000900) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // B(6)=~1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;
  // L(7)=~0
  if ((inst.Bits() & 0x00000080)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1010 & B(6)=0 & L(7)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1,
//       baseline: VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd101000m1mmmm,
//       rule: VSHLL_A1_or_VMOVL,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       uses: {}}
class VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000A00) return false;
  // B(6)=~0
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;
  // L(7)=~0
  if ((inst.Bits() & 0x00000080)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=011x
//    = {L: L(7),
//       Q: Q(6),
//       U: U(24),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1,
//       baseline: VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), Vd(15:12), op(8), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd011plqm1mmmm,
//       rule: VQSHL_VQSHLU_immediate,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => UNDEFINED],
//       uses: {}}
class VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0TesterCase16
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~011x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000600) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=111x & L(7)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1,
//       baseline: VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd111p0qm1mmmm,
//       rule: VCVT_between_floating_point_and_fixed_point,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         imm6(21:16)=0xxxxx => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0TesterCase17
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~111x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000E00) return false;
  // L(7)=~0
  if ((inst.Bits() & 0x00000080)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// A(11:8)=0000
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0000lqm1mmmm,
//       rule: VSHR,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0Tester_Case0
    : public VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0TesterCase0 {
 public:
  VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0Tester_Case0()
    : VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0TesterCase0(
      state_.VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0_VSHR_instance_)
  {}
};

// A(11:8)=0001
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0001lqm1mmmm,
//       rule: VSRA,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0Tester_Case1
    : public VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0TesterCase1 {
 public:
  VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0Tester_Case1()
    : VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0TesterCase1(
      state_.VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0_VSRA_instance_)
  {}
};

// A(11:8)=0010
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0010lqm1mmmm,
//       rule: VRSHR,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0Tester_Case2
    : public VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0TesterCase2 {
 public:
  VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0Tester_Case2()
    : VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0TesterCase2(
      state_.VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0_VRSHR_instance_)
  {}
};

// A(11:8)=0011
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0011lqm1mmmm,
//       rule: VRSRA,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0Tester_Case3
    : public VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0TesterCase3 {
 public:
  VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0Tester_Case3()
    : VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0TesterCase3(
      state_.VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0_VRSRA_instance_)
  {}
};

// A(11:8)=0100 & U(24)=1
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100111diiiiiidddd0100lqm1mmmm,
//       rule: VSRI,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0Tester_Case4
    : public VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0TesterCase4 {
 public:
  VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0Tester_Case4()
    : VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0TesterCase4(
      state_.VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0_VSRI_instance_)
  {}
};

// A(11:8)=0101 & U(24)=0
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100101diiiiiidddd0101lqm1mmmm,
//       rule: VSHL_immediate,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0Tester_Case5
    : public VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0TesterCase5 {
 public:
  VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0Tester_Case5()
    : VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0TesterCase5(
      state_.VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0_VSHL_immediate_instance_)
  {}
};

// A(11:8)=0101 & U(24)=1
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100111diiiiiidddd0101lqm1mmmm,
//       rule: VSLI,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0Tester_Case6
    : public VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0TesterCase6 {
 public:
  VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0Tester_Case6()
    : VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0TesterCase6(
      state_.VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0_VSLI_instance_)
  {}
};

// A(11:8)=1000 & U(24)=0 & B(6)=0 & L(7)=0
//    = {Vm: Vm(3:0),
//       actual: Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1,
//       baseline: VSHRN_111100101diiiiiidddd100000m1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100101diiiiiidddd100000m1mmmm,
//       rule: VSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vm(0)=1 => UNDEFINED],
//       uses: {}}
class VSHRN_111100101diiiiiidddd100000m1mmmm_case_0Tester_Case7
    : public VSHRN_111100101diiiiiidddd100000m1mmmm_case_0TesterCase7 {
 public:
  VSHRN_111100101diiiiiidddd100000m1mmmm_case_0Tester_Case7()
    : VSHRN_111100101diiiiiidddd100000m1mmmm_case_0TesterCase7(
      state_.VSHRN_111100101diiiiiidddd100000m1mmmm_case_0_VSHRN_instance_)
  {}
};

// A(11:8)=1000 & U(24)=0 & B(6)=1 & L(7)=0
//    = {Vm: Vm(3:0),
//       actual: Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1,
//       baseline: VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100101diiiiiidddd100001m1mmmm,
//       rule: VRSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vm(0)=1 => UNDEFINED],
//       uses: {}}
class VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0Tester_Case8
    : public VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0TesterCase8 {
 public:
  VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0Tester_Case8()
    : VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0TesterCase8(
      state_.VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0_VRSHRN_instance_)
  {}
};

// A(11:8)=1000 & U(24)=1 & B(6)=0 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//       rule: VQRSHRUN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case9
    : public VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase9 {
 public:
  VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case9()
    : VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase9(
      state_.VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0_VQRSHRUN_instance_)
  {}
};

// A(11:8)=1000 & U(24)=1 & B(6)=1 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//       rule: VQRSHRUN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case10
    : public VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase10 {
 public:
  VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case10()
    : VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase10(
      state_.VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0_VQRSHRUN_instance_)
  {}
};

// A(11:8)=1001 & U(24)=0 & B(6)=0 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//       rule: VQSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case11
    : public VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase11 {
 public:
  VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case11()
    : VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase11(
      state_.VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0_VQSHRN_instance_)
  {}
};

// A(11:8)=1001 & U(24)=0 & B(6)=1 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//       rule: VQRSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case12
    : public VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase12 {
 public:
  VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case12()
    : VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase12(
      state_.VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0_VQRSHRN_instance_)
  {}
};

// A(11:8)=1001 & U(24)=1 & B(6)=0 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//       rule: VQSHRUN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case13
    : public VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase13 {
 public:
  VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case13()
    : VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0TesterCase13(
      state_.VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0_VQSHRUN_instance_)
  {}
};

// A(11:8)=1001 & U(24)=1 & B(6)=1 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//       rule: VQRSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
class VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case14
    : public VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase14 {
 public:
  VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case14()
    : VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0TesterCase14(
      state_.VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0_VQRSHRN_instance_)
  {}
};

// A(11:8)=1010 & B(6)=0 & L(7)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1,
//       baseline: VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd101000m1mmmm,
//       rule: VSHLL_A1_or_VMOVL,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       uses: {}}
class VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0Tester_Case15
    : public VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0TesterCase15 {
 public:
  VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0Tester_Case15()
    : VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0TesterCase15(
      state_.VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0_VSHLL_A1_or_VMOVL_instance_)
  {}
};

// A(11:8)=011x
//    = {L: L(7),
//       Q: Q(6),
//       U: U(24),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1,
//       baseline: VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), Vd(15:12), op(8), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd011plqm1mmmm,
//       rule: VQSHL_VQSHLU_immediate,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => UNDEFINED],
//       uses: {}}
class VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0Tester_Case16
    : public VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0TesterCase16 {
 public:
  VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0Tester_Case16()
    : VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0TesterCase16(
      state_.VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0_VQSHL_VQSHLU_immediate_instance_)
  {}
};

// A(11:8)=111x & L(7)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1,
//       baseline: VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd111p0qm1mmmm,
//       rule: VCVT_between_floating_point_and_fixed_point,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         imm6(21:16)=0xxxxx => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0Tester_Case17
    : public VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0TesterCase17 {
 public:
  VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0Tester_Case17()
    : VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0TesterCase17(
      state_.VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0_VCVT_between_floating_point_and_fixed_point_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// A(11:8)=0000
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0000lqm1mmmm,
//       rule: VSHR,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0Tester_Case0_TestCase0) {
  VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1_VSHR actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd0000lqm1mmmm");
}

// A(11:8)=0001
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0001lqm1mmmm,
//       rule: VSRA,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0Tester_Case1_TestCase1) {
  VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1_VSRA actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd0001lqm1mmmm");
}

// A(11:8)=0010
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0010lqm1mmmm,
//       rule: VRSHR,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0Tester_Case2_TestCase2) {
  VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1_VRSHR actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd0010lqm1mmmm");
}

// A(11:8)=0011
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd0011lqm1mmmm,
//       rule: VRSRA,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0Tester_Case3_TestCase3) {
  VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1_VRSRA actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd0011lqm1mmmm");
}

// A(11:8)=0100 & U(24)=1
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100111diiiiiidddd0100lqm1mmmm,
//       rule: VSRI,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0Tester_Case4_TestCase4) {
  VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0Tester_Case4 baseline_tester;
  NamedActual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1_VSRI actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111diiiiiidddd0100lqm1mmmm");
}

// A(11:8)=0101 & U(24)=0
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100101diiiiiidddd0101lqm1mmmm,
//       rule: VSHL_immediate,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0Tester_Case5_TestCase5) {
  VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0Tester_Case5 baseline_tester;
  NamedActual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1_VSHL_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100101diiiiiidddd0101lqm1mmmm");
}

// A(11:8)=0101 & U(24)=1
//    = {L: L(7),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1,
//       baseline: VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100111diiiiiidddd0101lqm1mmmm,
//       rule: VSLI,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0Tester_Case6_TestCase6) {
  VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1_VSLI actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111diiiiiidddd0101lqm1mmmm");
}

// A(11:8)=1000 & U(24)=0 & B(6)=0 & L(7)=0
//    = {Vm: Vm(3:0),
//       actual: Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1,
//       baseline: VSHRN_111100101diiiiiidddd100000m1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100101diiiiiidddd100000m1mmmm,
//       rule: VSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vm(0)=1 => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSHRN_111100101diiiiiidddd100000m1mmmm_case_0Tester_Case7_TestCase7) {
  VSHRN_111100101diiiiiidddd100000m1mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1_VSHRN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100101diiiiiidddd100000m1mmmm");
}

// A(11:8)=1000 & U(24)=0 & B(6)=1 & L(7)=0
//    = {Vm: Vm(3:0),
//       actual: Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1,
//       baseline: VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 111100101diiiiiidddd100001m1mmmm,
//       rule: VRSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vm(0)=1 => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0Tester_Case8_TestCase8) {
  VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0Tester_Case8 baseline_tester;
  NamedActual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1_VRSHRN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100101diiiiiidddd100001m1mmmm");
}

// A(11:8)=1000 & U(24)=1 & B(6)=0 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//       rule: VQRSHRUN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case9_TestCase9) {
  VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case9 baseline_tester;
  NamedActual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1_VQRSHRUN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd100p00m1mmmm");
}

// A(11:8)=1000 & U(24)=1 & B(6)=1 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//       rule: VQRSHRUN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case10_TestCase10) {
  VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case10 baseline_tester;
  NamedActual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1_VQRSHRUN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd100p01m1mmmm");
}

// A(11:8)=1001 & U(24)=0 & B(6)=0 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//       rule: VQSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case11_TestCase11) {
  VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case11 baseline_tester;
  NamedActual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1_VQSHRN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd100p00m1mmmm");
}

// A(11:8)=1001 & U(24)=0 & B(6)=1 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//       rule: VQRSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case12_TestCase12) {
  VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case12 baseline_tester;
  NamedActual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1_VQRSHRN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd100p01m1mmmm");
}

// A(11:8)=1001 & U(24)=1 & B(6)=0 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//       rule: VQSHRUN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case13_TestCase13) {
  VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0Tester_Case13 baseline_tester;
  NamedActual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1_VQSHRUN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd100p00m1mmmm");
}

// A(11:8)=1001 & U(24)=1 & B(6)=1 & L(7)=0
//    = {U: U(24),
//       Vm: Vm(3:0),
//       actual: Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1,
//       baseline: VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), op(8), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//       rule: VQRSHRN,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         Vm(0)=1 => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case14_TestCase14) {
  VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0Tester_Case14 baseline_tester;
  NamedActual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1_VQRSHRN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd100p01m1mmmm");
}

// A(11:8)=1010 & B(6)=0 & L(7)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1,
//       baseline: VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd101000m1mmmm,
//       rule: VSHLL_A1_or_VMOVL,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0Tester_Case15_TestCase15) {
  VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0Tester_Case15 baseline_tester;
  NamedActual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1_VSHLL_A1_or_VMOVL actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd101000m1mmmm");
}

// A(11:8)=011x
//    = {L: L(7),
//       Q: Q(6),
//       U: U(24),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1,
//       baseline: VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0,
//       defs: {},
//       fields: [U(24), imm6(21:16), Vd(15:12), op(8), L(7), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       op: op(8),
//       pattern: 1111001u1diiiiiidddd011plqm1mmmm,
//       rule: VQSHL_VQSHLU_immediate,
//       safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         U(24)=0 &&
//            op(8)=0 => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0Tester_Case16_TestCase16) {
  VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0Tester_Case16 baseline_tester;
  NamedActual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1_VQSHL_VQSHLU_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd011plqm1mmmm");
}

// A(11:8)=111x & L(7)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1,
//       baseline: VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0,
//       defs: {},
//       fields: [imm6(21:16), Vd(15:12), Q(6), Vm(3:0)],
//       imm6: imm6(21:16),
//       pattern: 1111001u1diiiiiidddd111p0qm1mmmm,
//       rule: VCVT_between_floating_point_and_fixed_point,
//       safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//         imm6(21:16)=0xxxxx => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0Tester_Case17_TestCase17) {
  VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0Tester_Case17 baseline_tester;
  NamedActual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1_VCVT_between_floating_point_and_fixed_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1diiiiiidddd111p0qm1mmmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
