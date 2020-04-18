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


// A(17:16)=00 & B(10:6)=0000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1,
//       baseline: VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8:7), Q(6), Vm(3:0)],
//       op: op(8:7),
//       pattern: 111100111d11ss00dddd000ppqm0mmmm,
//       rule: VREV64,
//       safety: [op + size  >=
//               3 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~00
  if ((inst.Bits() & 0x00030000)  !=
          0x00000000) return false;
  // B(10:6)=~0000x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=00 & B(10:6)=0001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1,
//       baseline: VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8:7), Q(6), Vm(3:0)],
//       op: op(8:7),
//       pattern: 111100111d11ss00dddd000ppqm0mmmm,
//       rule: VREV32,
//       safety: [op + size  >=
//               3 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~00
  if ((inst.Bits() & 0x00030000)  !=
          0x00000000) return false;
  // B(10:6)=~0001x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000080) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=00 & B(10:6)=0010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1,
//       baseline: VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8:7), Q(6), Vm(3:0)],
//       op: op(8:7),
//       pattern: 111100111d11ss00dddd000ppqm0mmmm,
//       rule: VREV16,
//       safety: [op + size  >=
//               3 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~00
  if ((inst.Bits() & 0x00030000)  !=
          0x00000000) return false;
  // B(10:6)=~0010x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000100) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=00 & B(10:6)=1000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLS_111100111d11ss00dddd01000qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01000qm0mmmm,
//       rule: VCLS,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLS_111100111d11ss00dddd01000qm0mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  VCLS_111100111d11ss00dddd01000qm0mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCLS_111100111d11ss00dddd01000qm0mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~00
  if ((inst.Bits() & 0x00030000)  !=
          0x00000000) return false;
  // B(10:6)=~1000x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000400) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=00 & B(10:6)=1001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01001qm0mmmm,
//       rule: VCLZ,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~00
  if ((inst.Bits() & 0x00030000)  !=
          0x00000000) return false;
  // B(10:6)=~1001x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000480) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=00 & B(10:6)=1010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1,
//       baseline: VCNT_111100111d11ss00dddd01010qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01010qm0mmmm,
//       rule: VCNT,
//       safety: [size(19:18)=~00 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCNT_111100111d11ss00dddd01010qm0mmmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  VCNT_111100111d11ss00dddd01010qm0mmmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCNT_111100111d11ss00dddd01010qm0mmmm_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~00
  if ((inst.Bits() & 0x00030000)  !=
          0x00000000) return false;
  // B(10:6)=~1010x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000500) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=00 & B(10:6)=1011x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1,
//       baseline: VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01011qm0mmmm,
//       rule: VMVN_register,
//       safety: [size(19:18)=~00 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~00
  if ((inst.Bits() & 0x00030000)  !=
          0x00000000) return false;
  // B(10:6)=~1011x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000580) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=00 & B(10:6)=1110x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VQABS_111100111d11ss00dddd01110qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01110qm0mmmm,
//       rule: VQABS,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VQABS_111100111d11ss00dddd01110qm0mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  VQABS_111100111d11ss00dddd01110qm0mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQABS_111100111d11ss00dddd01110qm0mmmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~00
  if ((inst.Bits() & 0x00030000)  !=
          0x00000000) return false;
  // B(10:6)=~1110x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000700) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=00 & B(10:6)=1111x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01111qm0mmmm,
//       rule: VQNEG,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~00
  if ((inst.Bits() & 0x00030000)  !=
          0x00000000) return false;
  // B(10:6)=~1111x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000780) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=00 & B(10:6)=010xx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd0010pqm0mmmm,
//       rule: VPADDL,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~00
  if ((inst.Bits() & 0x00030000)  !=
          0x00000000) return false;
  // B(10:6)=~010xx
  if ((inst.Bits() & 0x00000700)  !=
          0x00000200) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=00 & B(10:6)=110xx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd0110pqm0mmmm,
//       rule: VPADAL,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~00
  if ((inst.Bits() & 0x00030000)  !=
          0x00000000) return false;
  // B(10:6)=~110xx
  if ((inst.Bits() & 0x00000700)  !=
          0x00000600) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=0000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f000qm0mmmm,
//       rule: VCGT_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~0000x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=0001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f001qm0mmmm,
//       rule: VCGE_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~0001x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000080) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=0010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f010qm0mmmm,
//       rule: VCEQ_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~0010x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000100) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=0011x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f011qm0mmmm,
//       rule: VCLE_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~0011x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000180) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=0100x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f100qm0mmmm,
//       rule: VCLT_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~0100x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000200) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=0110x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f110qm0mmmm,
//       rule: VABS_A1,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0TesterCase16
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~0110x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000300) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=0111x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f111qm0mmmm,
//       rule: VNEG,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0TesterCase17
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~0111x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000380) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=1000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f000qm0mmmm,
//       rule: VCGT_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1TesterCase18
    : public Arm32DecoderTester {
 public:
  VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1TesterCase18(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1TesterCase18
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~1000x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000400) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=1001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f001qm0mmmm,
//       rule: VCGE_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1TesterCase19
    : public Arm32DecoderTester {
 public:
  VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1TesterCase19(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1TesterCase19
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~1001x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000480) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=1010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f010qm0mmmm,
//       rule: VCEQ_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1TesterCase20
    : public Arm32DecoderTester {
 public:
  VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1TesterCase20(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1TesterCase20
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~1010x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000500) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=1011x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f011qm0mmmm,
//       rule: VCLE_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1TesterCase21
    : public Arm32DecoderTester {
 public:
  VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1TesterCase21(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1TesterCase21
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~1011x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000580) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=1100x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f100qm0mmmm,
//       rule: VCLT_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1TesterCase22
    : public Arm32DecoderTester {
 public:
  VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1TesterCase22(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1TesterCase22
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~1100x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000600) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=1110x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f110qm0mmmm,
//       rule: VABS_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1TesterCase23
    : public Arm32DecoderTester {
 public:
  VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1TesterCase23(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1TesterCase23
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~1110x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000700) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=01 & B(10:6)=1111x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f111qm0mmmm,
//       rule: VNEG,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1TesterCase24
    : public Arm32DecoderTester {
 public:
  VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1TesterCase24(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1TesterCase24
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~01
  if ((inst.Bits() & 0x00030000)  !=
          0x00010000) return false;
  // B(10:6)=~1111x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000780) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=10 & B(10:6)=01000
//    = {Vm: Vm(3:0),
//       actual: Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1,
//       baseline: VMOVN_111100111d11ss10dddd001000m0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vm(3:0)],
//       pattern: 111100111d11ss10dddd001000m0mmmm,
//       rule: VMOVN,
//       safety: [size(19:18)=11 => UNDEFINED, Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VMOVN_111100111d11ss10dddd001000m0mmmm_case_0TesterCase25
    : public Arm32DecoderTester {
 public:
  VMOVN_111100111d11ss10dddd001000m0mmmm_case_0TesterCase25(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMOVN_111100111d11ss10dddd001000m0mmmm_case_0TesterCase25
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~10
  if ((inst.Bits() & 0x00030000)  !=
          0x00020000) return false;
  // B(10:6)=~01000
  if ((inst.Bits() & 0x000007C0)  !=
          0x00000200) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=10 & B(10:6)=01001
//    = {Vm: Vm(3:0),
//       actual: Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1,
//       baseline: VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), op(7:6), Vm(3:0)],
//       op: op(7:6),
//       pattern: 111100111d11ss10dddd0010ppm0mmmm,
//       rule: VQMOVUN,
//       safety: [op(7:6)=00 => DECODER_ERROR,
//         size(19:18)=11 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0TesterCase26
    : public Arm32DecoderTester {
 public:
  VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0TesterCase26(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0TesterCase26
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~10
  if ((inst.Bits() & 0x00030000)  !=
          0x00020000) return false;
  // B(10:6)=~01001
  if ((inst.Bits() & 0x000007C0)  !=
          0x00000240) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=10 & B(10:6)=01100
//    = {Vd: Vd(15:12),
//       actual: Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1,
//       baseline: VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12)],
//       pattern: 111100111d11ss10dddd001100m0mmmm,
//       rule: VSHLL_A2,
//       safety: [size(19:18)=11 ||
//            Vd(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0TesterCase27
    : public Arm32DecoderTester {
 public:
  VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0TesterCase27(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0TesterCase27
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~10
  if ((inst.Bits() & 0x00030000)  !=
          0x00020000) return false;
  // B(10:6)=~01100
  if ((inst.Bits() & 0x000007C0)  !=
          0x00000300) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=10 & B(10:6)=11x00
//    = {Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1,
//       baseline: CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8), Vm(3:0)],
//       half_to_single: op(8)=1,
//       op: op(8),
//       pattern: 111100111d11ss10dddd011p00m0mmmm,
//       rule: CVT_between_half_precision_and_single_precision,
//       safety: [size(19:18)=~01 => UNDEFINED,
//         half_to_single &&
//            Vd(0)=1 => UNDEFINED,
//         not half_to_single &&
//            Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0TesterCase28
    : public Arm32DecoderTester {
 public:
  CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0TesterCase28(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0TesterCase28
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~10
  if ((inst.Bits() & 0x00030000)  !=
          0x00020000) return false;
  // B(10:6)=~11x00
  if ((inst.Bits() & 0x000006C0)  !=
          0x00000600) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=10 & B(10:6)=0000x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1,
//       baseline: VSWP_111100111d11ss10dddd00000qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00000qm0mmmm,
//       rule: VSWP,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=~00 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VSWP_111100111d11ss10dddd00000qm0mmmm_case_0TesterCase29
    : public Arm32DecoderTester {
 public:
  VSWP_111100111d11ss10dddd00000qm0mmmm_case_0TesterCase29(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSWP_111100111d11ss10dddd00000qm0mmmm_case_0TesterCase29
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~10
  if ((inst.Bits() & 0x00030000)  !=
          0x00020000) return false;
  // B(10:6)=~0000x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=10 & B(10:6)=0001x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1,
//       baseline: VTRN_111100111d11ss10dddd00001qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00001qm0mmmm,
//       rule: VTRN,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VTRN_111100111d11ss10dddd00001qm0mmmm_case_0TesterCase30
    : public Arm32DecoderTester {
 public:
  VTRN_111100111d11ss10dddd00001qm0mmmm_case_0TesterCase30(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VTRN_111100111d11ss10dddd00001qm0mmmm_case_0TesterCase30
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~10
  if ((inst.Bits() & 0x00030000)  !=
          0x00020000) return false;
  // B(10:6)=~0001x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000080) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=10 & B(10:6)=0010x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1,
//       baseline: VUZP_111100111d11ss10dddd00010qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00010qm0mmmm,
//       rule: VUZP,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=11 ||
//            (Q(6)=0 &&
//            size(19:18)=10) => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VUZP_111100111d11ss10dddd00010qm0mmmm_case_0TesterCase31
    : public Arm32DecoderTester {
 public:
  VUZP_111100111d11ss10dddd00010qm0mmmm_case_0TesterCase31(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VUZP_111100111d11ss10dddd00010qm0mmmm_case_0TesterCase31
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~10
  if ((inst.Bits() & 0x00030000)  !=
          0x00020000) return false;
  // B(10:6)=~0010x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000100) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=10 & B(10:6)=0011x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1,
//       baseline: VZIP_111100111d11ss10dddd00011qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00011qm0mmmm,
//       rule: VZIP,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=11 ||
//            (Q(6)=0 &&
//            size(19:18)=10) => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VZIP_111100111d11ss10dddd00011qm0mmmm_case_0TesterCase32
    : public Arm32DecoderTester {
 public:
  VZIP_111100111d11ss10dddd00011qm0mmmm_case_0TesterCase32(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VZIP_111100111d11ss10dddd00011qm0mmmm_case_0TesterCase32
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~10
  if ((inst.Bits() & 0x00030000)  !=
          0x00020000) return false;
  // B(10:6)=~0011x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000180) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=10 & B(10:6)=0101x
//    = {Vm: Vm(3:0),
//       actual: Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1,
//       baseline: VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), op(7:6), Vm(3:0)],
//       op: op(7:6),
//       pattern: 111100111d11ss10dddd0010ppm0mmmm,
//       rule: VQMOVN,
//       safety: [op(7:6)=00 => DECODER_ERROR,
//         size(19:18)=11 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0TesterCase33
    : public Arm32DecoderTester {
 public:
  VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0TesterCase33(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0TesterCase33
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~10
  if ((inst.Bits() & 0x00030000)  !=
          0x00020000) return false;
  // B(10:6)=~0101x
  if ((inst.Bits() & 0x00000780)  !=
          0x00000280) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=11 & B(10:6)=10x0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss11dddd010f0qm0mmmm,
//       rule: VRECPE,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0TesterCase34
    : public Arm32DecoderTester {
 public:
  VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0TesterCase34(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0TesterCase34
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~11
  if ((inst.Bits() & 0x00030000)  !=
          0x00030000) return false;
  // B(10:6)=~10x0x
  if ((inst.Bits() & 0x00000680)  !=
          0x00000400) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=11 & B(10:6)=10x1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss11dddd010f1qm0mmmm,
//       rule: VRSQRTE,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0TesterCase35
    : public Arm32DecoderTester {
 public:
  VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0TesterCase35(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0TesterCase35
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~11
  if ((inst.Bits() & 0x00030000)  !=
          0x00030000) return false;
  // B(10:6)=~10x1x
  if ((inst.Bits() & 0x00000680)  !=
          0x00000480) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(17:16)=11 & B(10:6)=11xxx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss11dddd011ppqm0mmmm,
//       rule: VCVT,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0TesterCase36
    : public Arm32DecoderTester {
 public:
  VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0TesterCase36(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0TesterCase36
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(17:16)=~11
  if ((inst.Bits() & 0x00030000)  !=
          0x00030000) return false;
  // B(10:6)=~11xxx
  if ((inst.Bits() & 0x00000600)  !=
          0x00000600) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// A(17:16)=00 & B(10:6)=0000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1,
//       baseline: VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8:7), Q(6), Vm(3:0)],
//       op: op(8:7),
//       pattern: 111100111d11ss00dddd000ppqm0mmmm,
//       rule: VREV64,
//       safety: [op + size  >=
//               3 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case0
    : public VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase0 {
 public:
  VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case0()
    : VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase0(
      state_.VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0_VREV64_instance_)
  {}
};

// A(17:16)=00 & B(10:6)=0001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1,
//       baseline: VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8:7), Q(6), Vm(3:0)],
//       op: op(8:7),
//       pattern: 111100111d11ss00dddd000ppqm0mmmm,
//       rule: VREV32,
//       safety: [op + size  >=
//               3 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case1
    : public VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase1 {
 public:
  VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case1()
    : VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase1(
      state_.VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0_VREV32_instance_)
  {}
};

// A(17:16)=00 & B(10:6)=0010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1,
//       baseline: VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8:7), Q(6), Vm(3:0)],
//       op: op(8:7),
//       pattern: 111100111d11ss00dddd000ppqm0mmmm,
//       rule: VREV16,
//       safety: [op + size  >=
//               3 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case2
    : public VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase2 {
 public:
  VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case2()
    : VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0TesterCase2(
      state_.VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0_VREV16_instance_)
  {}
};

// A(17:16)=00 & B(10:6)=1000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLS_111100111d11ss00dddd01000qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01000qm0mmmm,
//       rule: VCLS,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLS_111100111d11ss00dddd01000qm0mmmm_case_0Tester_Case3
    : public VCLS_111100111d11ss00dddd01000qm0mmmm_case_0TesterCase3 {
 public:
  VCLS_111100111d11ss00dddd01000qm0mmmm_case_0Tester_Case3()
    : VCLS_111100111d11ss00dddd01000qm0mmmm_case_0TesterCase3(
      state_.VCLS_111100111d11ss00dddd01000qm0mmmm_case_0_VCLS_instance_)
  {}
};

// A(17:16)=00 & B(10:6)=1001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01001qm0mmmm,
//       rule: VCLZ,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0Tester_Case4
    : public VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0TesterCase4 {
 public:
  VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0Tester_Case4()
    : VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0TesterCase4(
      state_.VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0_VCLZ_instance_)
  {}
};

// A(17:16)=00 & B(10:6)=1010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1,
//       baseline: VCNT_111100111d11ss00dddd01010qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01010qm0mmmm,
//       rule: VCNT,
//       safety: [size(19:18)=~00 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCNT_111100111d11ss00dddd01010qm0mmmm_case_0Tester_Case5
    : public VCNT_111100111d11ss00dddd01010qm0mmmm_case_0TesterCase5 {
 public:
  VCNT_111100111d11ss00dddd01010qm0mmmm_case_0Tester_Case5()
    : VCNT_111100111d11ss00dddd01010qm0mmmm_case_0TesterCase5(
      state_.VCNT_111100111d11ss00dddd01010qm0mmmm_case_0_VCNT_instance_)
  {}
};

// A(17:16)=00 & B(10:6)=1011x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1,
//       baseline: VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01011qm0mmmm,
//       rule: VMVN_register,
//       safety: [size(19:18)=~00 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0Tester_Case6
    : public VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0TesterCase6 {
 public:
  VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0Tester_Case6()
    : VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0TesterCase6(
      state_.VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0_VMVN_register_instance_)
  {}
};

// A(17:16)=00 & B(10:6)=1110x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VQABS_111100111d11ss00dddd01110qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01110qm0mmmm,
//       rule: VQABS,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VQABS_111100111d11ss00dddd01110qm0mmmm_case_0Tester_Case7
    : public VQABS_111100111d11ss00dddd01110qm0mmmm_case_0TesterCase7 {
 public:
  VQABS_111100111d11ss00dddd01110qm0mmmm_case_0Tester_Case7()
    : VQABS_111100111d11ss00dddd01110qm0mmmm_case_0TesterCase7(
      state_.VQABS_111100111d11ss00dddd01110qm0mmmm_case_0_VQABS_instance_)
  {}
};

// A(17:16)=00 & B(10:6)=1111x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01111qm0mmmm,
//       rule: VQNEG,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0Tester_Case8
    : public VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0TesterCase8 {
 public:
  VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0Tester_Case8()
    : VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0TesterCase8(
      state_.VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0_VQNEG_instance_)
  {}
};

// A(17:16)=00 & B(10:6)=010xx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd0010pqm0mmmm,
//       rule: VPADDL,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0Tester_Case9
    : public VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0TesterCase9 {
 public:
  VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0Tester_Case9()
    : VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0TesterCase9(
      state_.VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0_VPADDL_instance_)
  {}
};

// A(17:16)=00 & B(10:6)=110xx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd0110pqm0mmmm,
//       rule: VPADAL,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0Tester_Case10
    : public VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0TesterCase10 {
 public:
  VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0Tester_Case10()
    : VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0TesterCase10(
      state_.VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0_VPADAL_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=0000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f000qm0mmmm,
//       rule: VCGT_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0Tester_Case11
    : public VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0TesterCase11 {
 public:
  VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0Tester_Case11()
    : VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0TesterCase11(
      state_.VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0_VCGT_immediate_0_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=0001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f001qm0mmmm,
//       rule: VCGE_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0Tester_Case12
    : public VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0TesterCase12 {
 public:
  VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0Tester_Case12()
    : VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0TesterCase12(
      state_.VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0_VCGE_immediate_0_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=0010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f010qm0mmmm,
//       rule: VCEQ_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0Tester_Case13
    : public VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0TesterCase13 {
 public:
  VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0Tester_Case13()
    : VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0TesterCase13(
      state_.VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0_VCEQ_immediate_0_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=0011x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f011qm0mmmm,
//       rule: VCLE_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0Tester_Case14
    : public VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0TesterCase14 {
 public:
  VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0Tester_Case14()
    : VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0TesterCase14(
      state_.VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0_VCLE_immediate_0_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=0100x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f100qm0mmmm,
//       rule: VCLT_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0Tester_Case15
    : public VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0TesterCase15 {
 public:
  VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0Tester_Case15()
    : VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0TesterCase15(
      state_.VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0_VCLT_immediate_0_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=0110x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f110qm0mmmm,
//       rule: VABS_A1,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0Tester_Case16
    : public VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0TesterCase16 {
 public:
  VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0Tester_Case16()
    : VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0TesterCase16(
      state_.VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0_VABS_A1_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=0111x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f111qm0mmmm,
//       rule: VNEG,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0Tester_Case17
    : public VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0TesterCase17 {
 public:
  VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0Tester_Case17()
    : VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0TesterCase17(
      state_.VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0_VNEG_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=1000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f000qm0mmmm,
//       rule: VCGT_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1Tester_Case18
    : public VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1TesterCase18 {
 public:
  VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1Tester_Case18()
    : VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1TesterCase18(
      state_.VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1_VCGT_immediate_0_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=1001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f001qm0mmmm,
//       rule: VCGE_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1Tester_Case19
    : public VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1TesterCase19 {
 public:
  VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1Tester_Case19()
    : VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1TesterCase19(
      state_.VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1_VCGE_immediate_0_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=1010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f010qm0mmmm,
//       rule: VCEQ_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1Tester_Case20
    : public VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1TesterCase20 {
 public:
  VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1Tester_Case20()
    : VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1TesterCase20(
      state_.VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1_VCEQ_immediate_0_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=1011x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f011qm0mmmm,
//       rule: VCLE_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1Tester_Case21
    : public VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1TesterCase21 {
 public:
  VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1Tester_Case21()
    : VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1TesterCase21(
      state_.VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1_VCLE_immediate_0_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=1100x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f100qm0mmmm,
//       rule: VCLT_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1Tester_Case22
    : public VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1TesterCase22 {
 public:
  VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1Tester_Case22()
    : VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1TesterCase22(
      state_.VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1_VCLT_immediate_0_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=1110x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f110qm0mmmm,
//       rule: VABS_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1Tester_Case23
    : public VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1TesterCase23 {
 public:
  VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1Tester_Case23()
    : VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1TesterCase23(
      state_.VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VABS_A1_instance_)
  {}
};

// A(17:16)=01 & B(10:6)=1111x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f111qm0mmmm,
//       rule: VNEG,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1Tester_Case24
    : public VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1TesterCase24 {
 public:
  VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1Tester_Case24()
    : VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1TesterCase24(
      state_.VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1_VNEG_instance_)
  {}
};

// A(17:16)=10 & B(10:6)=01000
//    = {Vm: Vm(3:0),
//       actual: Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1,
//       baseline: VMOVN_111100111d11ss10dddd001000m0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vm(3:0)],
//       pattern: 111100111d11ss10dddd001000m0mmmm,
//       rule: VMOVN,
//       safety: [size(19:18)=11 => UNDEFINED, Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VMOVN_111100111d11ss10dddd001000m0mmmm_case_0Tester_Case25
    : public VMOVN_111100111d11ss10dddd001000m0mmmm_case_0TesterCase25 {
 public:
  VMOVN_111100111d11ss10dddd001000m0mmmm_case_0Tester_Case25()
    : VMOVN_111100111d11ss10dddd001000m0mmmm_case_0TesterCase25(
      state_.VMOVN_111100111d11ss10dddd001000m0mmmm_case_0_VMOVN_instance_)
  {}
};

// A(17:16)=10 & B(10:6)=01001
//    = {Vm: Vm(3:0),
//       actual: Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1,
//       baseline: VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), op(7:6), Vm(3:0)],
//       op: op(7:6),
//       pattern: 111100111d11ss10dddd0010ppm0mmmm,
//       rule: VQMOVUN,
//       safety: [op(7:6)=00 => DECODER_ERROR,
//         size(19:18)=11 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0Tester_Case26
    : public VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0TesterCase26 {
 public:
  VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0Tester_Case26()
    : VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0TesterCase26(
      state_.VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0_VQMOVUN_instance_)
  {}
};

// A(17:16)=10 & B(10:6)=01100
//    = {Vd: Vd(15:12),
//       actual: Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1,
//       baseline: VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12)],
//       pattern: 111100111d11ss10dddd001100m0mmmm,
//       rule: VSHLL_A2,
//       safety: [size(19:18)=11 ||
//            Vd(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0Tester_Case27
    : public VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0TesterCase27 {
 public:
  VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0Tester_Case27()
    : VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0TesterCase27(
      state_.VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0_VSHLL_A2_instance_)
  {}
};

// A(17:16)=10 & B(10:6)=11x00
//    = {Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1,
//       baseline: CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8), Vm(3:0)],
//       half_to_single: op(8)=1,
//       op: op(8),
//       pattern: 111100111d11ss10dddd011p00m0mmmm,
//       rule: CVT_between_half_precision_and_single_precision,
//       safety: [size(19:18)=~01 => UNDEFINED,
//         half_to_single &&
//            Vd(0)=1 => UNDEFINED,
//         not half_to_single &&
//            Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0Tester_Case28
    : public CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0TesterCase28 {
 public:
  CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0Tester_Case28()
    : CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0TesterCase28(
      state_.CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0_CVT_between_half_precision_and_single_precision_instance_)
  {}
};

// A(17:16)=10 & B(10:6)=0000x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1,
//       baseline: VSWP_111100111d11ss10dddd00000qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00000qm0mmmm,
//       rule: VSWP,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=~00 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VSWP_111100111d11ss10dddd00000qm0mmmm_case_0Tester_Case29
    : public VSWP_111100111d11ss10dddd00000qm0mmmm_case_0TesterCase29 {
 public:
  VSWP_111100111d11ss10dddd00000qm0mmmm_case_0Tester_Case29()
    : VSWP_111100111d11ss10dddd00000qm0mmmm_case_0TesterCase29(
      state_.VSWP_111100111d11ss10dddd00000qm0mmmm_case_0_VSWP_instance_)
  {}
};

// A(17:16)=10 & B(10:6)=0001x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1,
//       baseline: VTRN_111100111d11ss10dddd00001qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00001qm0mmmm,
//       rule: VTRN,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VTRN_111100111d11ss10dddd00001qm0mmmm_case_0Tester_Case30
    : public VTRN_111100111d11ss10dddd00001qm0mmmm_case_0TesterCase30 {
 public:
  VTRN_111100111d11ss10dddd00001qm0mmmm_case_0Tester_Case30()
    : VTRN_111100111d11ss10dddd00001qm0mmmm_case_0TesterCase30(
      state_.VTRN_111100111d11ss10dddd00001qm0mmmm_case_0_VTRN_instance_)
  {}
};

// A(17:16)=10 & B(10:6)=0010x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1,
//       baseline: VUZP_111100111d11ss10dddd00010qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00010qm0mmmm,
//       rule: VUZP,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=11 ||
//            (Q(6)=0 &&
//            size(19:18)=10) => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VUZP_111100111d11ss10dddd00010qm0mmmm_case_0Tester_Case31
    : public VUZP_111100111d11ss10dddd00010qm0mmmm_case_0TesterCase31 {
 public:
  VUZP_111100111d11ss10dddd00010qm0mmmm_case_0Tester_Case31()
    : VUZP_111100111d11ss10dddd00010qm0mmmm_case_0TesterCase31(
      state_.VUZP_111100111d11ss10dddd00010qm0mmmm_case_0_VUZP_instance_)
  {}
};

// A(17:16)=10 & B(10:6)=0011x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1,
//       baseline: VZIP_111100111d11ss10dddd00011qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00011qm0mmmm,
//       rule: VZIP,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=11 ||
//            (Q(6)=0 &&
//            size(19:18)=10) => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VZIP_111100111d11ss10dddd00011qm0mmmm_case_0Tester_Case32
    : public VZIP_111100111d11ss10dddd00011qm0mmmm_case_0TesterCase32 {
 public:
  VZIP_111100111d11ss10dddd00011qm0mmmm_case_0Tester_Case32()
    : VZIP_111100111d11ss10dddd00011qm0mmmm_case_0TesterCase32(
      state_.VZIP_111100111d11ss10dddd00011qm0mmmm_case_0_VZIP_instance_)
  {}
};

// A(17:16)=10 & B(10:6)=0101x
//    = {Vm: Vm(3:0),
//       actual: Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1,
//       baseline: VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), op(7:6), Vm(3:0)],
//       op: op(7:6),
//       pattern: 111100111d11ss10dddd0010ppm0mmmm,
//       rule: VQMOVN,
//       safety: [op(7:6)=00 => DECODER_ERROR,
//         size(19:18)=11 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0Tester_Case33
    : public VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0TesterCase33 {
 public:
  VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0Tester_Case33()
    : VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0TesterCase33(
      state_.VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0_VQMOVN_instance_)
  {}
};

// A(17:16)=11 & B(10:6)=10x0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss11dddd010f0qm0mmmm,
//       rule: VRECPE,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0Tester_Case34
    : public VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0TesterCase34 {
 public:
  VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0Tester_Case34()
    : VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0TesterCase34(
      state_.VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0_VRECPE_instance_)
  {}
};

// A(17:16)=11 & B(10:6)=10x1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss11dddd010f1qm0mmmm,
//       rule: VRSQRTE,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0Tester_Case35
    : public VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0TesterCase35 {
 public:
  VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0Tester_Case35()
    : VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0TesterCase35(
      state_.VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0_VRSQRTE_instance_)
  {}
};

// A(17:16)=11 & B(10:6)=11xxx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss11dddd011ppqm0mmmm,
//       rule: VCVT,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
class VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0Tester_Case36
    : public VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0TesterCase36 {
 public:
  VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0Tester_Case36()
    : VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0TesterCase36(
      state_.VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0_VCVT_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// A(17:16)=00 & B(10:6)=0000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1,
//       baseline: VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8:7), Q(6), Vm(3:0)],
//       op: op(8:7),
//       pattern: 111100111d11ss00dddd000ppqm0mmmm,
//       rule: VREV64,
//       safety: [op + size  >=
//               3 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case0_TestCase0) {
  VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1_VREV64 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss00dddd000ppqm0mmmm");
}

// A(17:16)=00 & B(10:6)=0001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1,
//       baseline: VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8:7), Q(6), Vm(3:0)],
//       op: op(8:7),
//       pattern: 111100111d11ss00dddd000ppqm0mmmm,
//       rule: VREV32,
//       safety: [op + size  >=
//               3 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case1_TestCase1) {
  VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1_VREV32 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss00dddd000ppqm0mmmm");
}

// A(17:16)=00 & B(10:6)=0010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1,
//       baseline: VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8:7), Q(6), Vm(3:0)],
//       op: op(8:7),
//       pattern: 111100111d11ss00dddd000ppqm0mmmm,
//       rule: VREV16,
//       safety: [op + size  >=
//               3 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case2_TestCase2) {
  VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1_VREV16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss00dddd000ppqm0mmmm");
}

// A(17:16)=00 & B(10:6)=1000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLS_111100111d11ss00dddd01000qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01000qm0mmmm,
//       rule: VCLS,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCLS_111100111d11ss00dddd01000qm0mmmm_case_0Tester_Case3_TestCase3) {
  VCLS_111100111d11ss00dddd01000qm0mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VCLS actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss00dddd01000qm0mmmm");
}

// A(17:16)=00 & B(10:6)=1001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01001qm0mmmm,
//       rule: VCLZ,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0Tester_Case4_TestCase4) {
  VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0Tester_Case4 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VCLZ actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss00dddd01001qm0mmmm");
}

// A(17:16)=00 & B(10:6)=1010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1,
//       baseline: VCNT_111100111d11ss00dddd01010qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01010qm0mmmm,
//       rule: VCNT,
//       safety: [size(19:18)=~00 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCNT_111100111d11ss00dddd01010qm0mmmm_case_0Tester_Case5_TestCase5) {
  VCNT_111100111d11ss00dddd01010qm0mmmm_case_0Tester_Case5 baseline_tester;
  NamedActual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1_VCNT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss00dddd01010qm0mmmm");
}

// A(17:16)=00 & B(10:6)=1011x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1,
//       baseline: VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01011qm0mmmm,
//       rule: VMVN_register,
//       safety: [size(19:18)=~00 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0Tester_Case6_TestCase6) {
  VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1_VMVN_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss00dddd01011qm0mmmm");
}

// A(17:16)=00 & B(10:6)=1110x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VQABS_111100111d11ss00dddd01110qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01110qm0mmmm,
//       rule: VQABS,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQABS_111100111d11ss00dddd01110qm0mmmm_case_0Tester_Case7_TestCase7) {
  VQABS_111100111d11ss00dddd01110qm0mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VQABS actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss00dddd01110qm0mmmm");
}

// A(17:16)=00 & B(10:6)=1111x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd01111qm0mmmm,
//       rule: VQNEG,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0Tester_Case8_TestCase8) {
  VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0Tester_Case8 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VQNEG actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss00dddd01111qm0mmmm");
}

// A(17:16)=00 & B(10:6)=010xx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd0010pqm0mmmm,
//       rule: VPADDL,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0Tester_Case9_TestCase9) {
  VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0Tester_Case9 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VPADDL actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss00dddd0010pqm0mmmm");
}

// A(17:16)=00 & B(10:6)=110xx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss00dddd0110pqm0mmmm,
//       rule: VPADAL,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0Tester_Case10_TestCase10) {
  VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0Tester_Case10 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VPADAL actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss00dddd0110pqm0mmmm");
}

// A(17:16)=01 & B(10:6)=0000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f000qm0mmmm,
//       rule: VCGT_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0Tester_Case11_TestCase11) {
  VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0Tester_Case11 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VCGT_immediate_0 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f000qm0mmmm");
}

// A(17:16)=01 & B(10:6)=0001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f001qm0mmmm,
//       rule: VCGE_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0Tester_Case12_TestCase12) {
  VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0Tester_Case12 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VCGE_immediate_0 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f001qm0mmmm");
}

// A(17:16)=01 & B(10:6)=0010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f010qm0mmmm,
//       rule: VCEQ_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0Tester_Case13_TestCase13) {
  VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0Tester_Case13 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VCEQ_immediate_0 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f010qm0mmmm");
}

// A(17:16)=01 & B(10:6)=0011x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f011qm0mmmm,
//       rule: VCLE_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0Tester_Case14_TestCase14) {
  VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0Tester_Case14 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VCLE_immediate_0 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f011qm0mmmm");
}

// A(17:16)=01 & B(10:6)=0100x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f100qm0mmmm,
//       rule: VCLT_immediate_0,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0Tester_Case15_TestCase15) {
  VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0Tester_Case15 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VCLT_immediate_0 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f100qm0mmmm");
}

// A(17:16)=01 & B(10:6)=0110x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f110qm0mmmm,
//       rule: VABS_A1,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0Tester_Case16_TestCase16) {
  VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0Tester_Case16 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VABS_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f110qm0mmmm");
}

// A(17:16)=01 & B(10:6)=0111x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       baseline: VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f111qm0mmmm,
//       rule: VNEG,
//       safety: [size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0Tester_Case17_TestCase17) {
  VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0Tester_Case17 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VNEG actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f111qm0mmmm");
}

// A(17:16)=01 & B(10:6)=1000x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f000qm0mmmm,
//       rule: VCGT_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1Tester_Case18_TestCase18) {
  VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1Tester_Case18 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_VCGT_immediate_0 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f000qm0mmmm");
}

// A(17:16)=01 & B(10:6)=1001x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f001qm0mmmm,
//       rule: VCGE_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1Tester_Case19_TestCase19) {
  VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1Tester_Case19 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_VCGE_immediate_0 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f001qm0mmmm");
}

// A(17:16)=01 & B(10:6)=1010x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f010qm0mmmm,
//       rule: VCEQ_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1Tester_Case20_TestCase20) {
  VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1Tester_Case20 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_VCEQ_immediate_0 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f010qm0mmmm");
}

// A(17:16)=01 & B(10:6)=1011x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f011qm0mmmm,
//       rule: VCLE_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1Tester_Case21_TestCase21) {
  VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1Tester_Case21 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_VCLE_immediate_0 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f011qm0mmmm");
}

// A(17:16)=01 & B(10:6)=1100x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f100qm0mmmm,
//       rule: VCLT_immediate_0,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1Tester_Case22_TestCase22) {
  VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1Tester_Case22 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_VCLT_immediate_0 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f100qm0mmmm");
}

// A(17:16)=01 & B(10:6)=1110x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f110qm0mmmm,
//       rule: VABS_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1Tester_Case23_TestCase23) {
  VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1Tester_Case23 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_VABS_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f110qm0mmmm");
}

// A(17:16)=01 & B(10:6)=1111x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss01dddd0f111qm0mmmm,
//       rule: VNEG,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1Tester_Case24_TestCase24) {
  VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1Tester_Case24 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_VNEG actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss01dddd0f111qm0mmmm");
}

// A(17:16)=10 & B(10:6)=01000
//    = {Vm: Vm(3:0),
//       actual: Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1,
//       baseline: VMOVN_111100111d11ss10dddd001000m0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vm(3:0)],
//       pattern: 111100111d11ss10dddd001000m0mmmm,
//       rule: VMOVN,
//       safety: [size(19:18)=11 => UNDEFINED, Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMOVN_111100111d11ss10dddd001000m0mmmm_case_0Tester_Case25_TestCase25) {
  VMOVN_111100111d11ss10dddd001000m0mmmm_case_0Tester_Case25 baseline_tester;
  NamedActual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1_VMOVN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss10dddd001000m0mmmm");
}

// A(17:16)=10 & B(10:6)=01001
//    = {Vm: Vm(3:0),
//       actual: Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1,
//       baseline: VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), op(7:6), Vm(3:0)],
//       op: op(7:6),
//       pattern: 111100111d11ss10dddd0010ppm0mmmm,
//       rule: VQMOVUN,
//       safety: [op(7:6)=00 => DECODER_ERROR,
//         size(19:18)=11 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0Tester_Case26_TestCase26) {
  VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0Tester_Case26 baseline_tester;
  NamedActual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1_VQMOVUN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss10dddd0010ppm0mmmm");
}

// A(17:16)=10 & B(10:6)=01100
//    = {Vd: Vd(15:12),
//       actual: Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1,
//       baseline: VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12)],
//       pattern: 111100111d11ss10dddd001100m0mmmm,
//       rule: VSHLL_A2,
//       safety: [size(19:18)=11 ||
//            Vd(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0Tester_Case27_TestCase27) {
  VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0Tester_Case27 baseline_tester;
  NamedActual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1_VSHLL_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss10dddd001100m0mmmm");
}

// A(17:16)=10 & B(10:6)=11x00
//    = {Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1,
//       baseline: CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), op(8), Vm(3:0)],
//       half_to_single: op(8)=1,
//       op: op(8),
//       pattern: 111100111d11ss10dddd011p00m0mmmm,
//       rule: CVT_between_half_precision_and_single_precision,
//       safety: [size(19:18)=~01 => UNDEFINED,
//         half_to_single &&
//            Vd(0)=1 => UNDEFINED,
//         not half_to_single &&
//            Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0Tester_Case28_TestCase28) {
  CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0Tester_Case28 baseline_tester;
  NamedActual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1_CVT_between_half_precision_and_single_precision actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss10dddd011p00m0mmmm");
}

// A(17:16)=10 & B(10:6)=0000x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1,
//       baseline: VSWP_111100111d11ss10dddd00000qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00000qm0mmmm,
//       rule: VSWP,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=~00 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSWP_111100111d11ss10dddd00000qm0mmmm_case_0Tester_Case29_TestCase29) {
  VSWP_111100111d11ss10dddd00000qm0mmmm_case_0Tester_Case29 baseline_tester;
  NamedActual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1_VSWP actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss10dddd00000qm0mmmm");
}

// A(17:16)=10 & B(10:6)=0001x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1,
//       baseline: VTRN_111100111d11ss10dddd00001qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00001qm0mmmm,
//       rule: VTRN,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=11 => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VTRN_111100111d11ss10dddd00001qm0mmmm_case_0Tester_Case30_TestCase30) {
  VTRN_111100111d11ss10dddd00001qm0mmmm_case_0Tester_Case30 baseline_tester;
  NamedActual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1_VTRN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss10dddd00001qm0mmmm");
}

// A(17:16)=10 & B(10:6)=0010x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1,
//       baseline: VUZP_111100111d11ss10dddd00010qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00010qm0mmmm,
//       rule: VUZP,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=11 ||
//            (Q(6)=0 &&
//            size(19:18)=10) => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VUZP_111100111d11ss10dddd00010qm0mmmm_case_0Tester_Case31_TestCase31) {
  VUZP_111100111d11ss10dddd00010qm0mmmm_case_0Tester_Case31 baseline_tester;
  NamedActual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1_VUZP actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss10dddd00010qm0mmmm");
}

// A(17:16)=10 & B(10:6)=0011x
//    = {D: D(22),
//       M: M(5),
//       Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1,
//       baseline: VZIP_111100111d11ss10dddd00011qm0mmmm_case_0,
//       d: D:Vd,
//       defs: {},
//       fields: [D(22), size(19:18), Vd(15:12), Q(6), M(5), Vm(3:0)],
//       m: M:Vm,
//       pattern: 111100111d11ss10dddd00011qm0mmmm,
//       rule: VZIP,
//       safety: [d  ==
//               m => UNKNOWN,
//         size(19:18)=11 ||
//            (Q(6)=0 &&
//            size(19:18)=10) => UNDEFINED,
//         Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VZIP_111100111d11ss10dddd00011qm0mmmm_case_0Tester_Case32_TestCase32) {
  VZIP_111100111d11ss10dddd00011qm0mmmm_case_0Tester_Case32 baseline_tester;
  NamedActual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1_VZIP actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss10dddd00011qm0mmmm");
}

// A(17:16)=10 & B(10:6)=0101x
//    = {Vm: Vm(3:0),
//       actual: Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1,
//       baseline: VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), op(7:6), Vm(3:0)],
//       op: op(7:6),
//       pattern: 111100111d11ss10dddd0010ppm0mmmm,
//       rule: VQMOVN,
//       safety: [op(7:6)=00 => DECODER_ERROR,
//         size(19:18)=11 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0Tester_Case33_TestCase33) {
  VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0Tester_Case33 baseline_tester;
  NamedActual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1_VQMOVN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss10dddd0010ppm0mmmm");
}

// A(17:16)=11 & B(10:6)=10x0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss11dddd010f0qm0mmmm,
//       rule: VRECPE,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0Tester_Case34_TestCase34) {
  VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0Tester_Case34 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_VRECPE actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss11dddd010f0qm0mmmm");
}

// A(17:16)=11 & B(10:6)=10x1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss11dddd010f1qm0mmmm,
//       rule: VRSQRTE,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0Tester_Case35_TestCase35) {
  VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0Tester_Case35 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_VRSQRTE actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss11dddd010f1qm0mmmm");
}

// A(17:16)=11 & B(10:6)=11xxx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       actual: Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2,
//       baseline: VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0,
//       defs: {},
//       fields: [size(19:18), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100111d11ss11dddd011ppqm0mmmm,
//       rule: VCVT,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(19:18)=~10 => UNDEFINED],
//       size: size(19:18),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0Tester_Case36_TestCase36) {
  VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0Tester_Case36 baseline_tester;
  NamedActual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_VCVT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111d11ss11dddd011ppqm0mmmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
