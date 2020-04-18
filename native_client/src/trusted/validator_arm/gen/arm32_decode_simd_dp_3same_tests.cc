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


// A(11:8)=0000 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0000nqm0mmmm,
//       rule: VHADD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000000) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0000 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0000nqm1mmmm,
//       rule: VQADD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000000) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0001 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0001nqm0mmmm,
//       rule: VRHADD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000100) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=00
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d00nnnndddd0001nqm1mmmm,
//       rule: VAND_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000100) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~00
  if ((inst.Bits() & 0x00300000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=01
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d01nnnndddd0001nqm1mmmm,
//       rule: VBIC_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000100) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~01
  if ((inst.Bits() & 0x00300000)  !=
          0x00100000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=10
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d10nnnndddd0001nqm1mmmm,
//       rule: VORR_register_or_VMOV_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000100) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~10
  if ((inst.Bits() & 0x00300000)  !=
          0x00200000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=11
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d11nnnndddd0001nqm1mmmm,
//       rule: VORN_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000100) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~11
  if ((inst.Bits() & 0x00300000)  !=
          0x00300000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=00
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d00nnnndddd0001nqm1mmmm,
//       rule: VEOR,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000100) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~00
  if ((inst.Bits() & 0x00300000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=01
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d01nnnndddd0001nqm1mmmm,
//       rule: VBSL,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000100) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~01
  if ((inst.Bits() & 0x00300000)  !=
          0x00100000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=10
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d10nnnndddd0001nqm1mmmm,
//       rule: VBIT,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000100) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~10
  if ((inst.Bits() & 0x00300000)  !=
          0x00200000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=11
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d11nnnndddd0001nqm1mmmm,
//       rule: VBIF,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000100) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~11
  if ((inst.Bits() & 0x00300000)  !=
          0x00300000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0010 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0010nqm0mmmm,
//       rule: VHSUB,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000200) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0010 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0010nqm1mmmm,
//       rule: VQSUB,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000200) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0011 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0011nqm0mmmm,
//       rule: VCGT_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000300) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0011 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0011nqm1mmmm,
//       rule: VCGE_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000300) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0100 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0100nqm0mmmm,
//       rule: VSHL_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0100
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000400) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0100 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0100nqm1mmmm,
//       rule: VQSHL_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0TesterCase16
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0100
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000400) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0101 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0101nqm0mmmm,
//       rule: VRSHL,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0TesterCase17
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000500) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0101 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0101nqm1mmmm,
//       rule: VQRSHL,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0TesterCase18
    : public Arm32DecoderTester {
 public:
  VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0TesterCase18(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0TesterCase18
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000500) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0110 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0110nqm0mmmm,
//       rule: VMAX,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0TesterCase19
    : public Arm32DecoderTester {
 public:
  VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0TesterCase19(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0TesterCase19
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000600) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0110 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0110nqm1mmmm,
//       rule: VMIN,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0TesterCase20
    : public Arm32DecoderTester {
 public:
  VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0TesterCase20(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0TesterCase20
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000600) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0111 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0111nqm0mmmm,
//       rule: VABD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0TesterCase21
    : public Arm32DecoderTester {
 public:
  VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0TesterCase21(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0TesterCase21
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0111
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000700) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0111 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0111nqm1mmmm,
//       rule: VABA,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0TesterCase22
    : public Arm32DecoderTester {
 public:
  VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0TesterCase22(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0TesterCase22
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0111
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000700) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1000 & B(4)=0 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1000nqm0mmmm,
//       rule: VADD_integer,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0TesterCase23
    : public Arm32DecoderTester {
 public:
  VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0TesterCase23(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0TesterCase23
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000800) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1000 & B(4)=0 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1000nqm0mmmm,
//       rule: VSUB_integer,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0TesterCase24
    : public Arm32DecoderTester {
 public:
  VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0TesterCase24(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0TesterCase24
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000800) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1000 & B(4)=1 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VTST_111100100dssnnnndddd1000nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1000nqm1mmmm,
//       rule: VTST,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VTST_111100100dssnnnndddd1000nqm1mmmm_case_0TesterCase25
    : public Arm32DecoderTester {
 public:
  VTST_111100100dssnnnndddd1000nqm1mmmm_case_0TesterCase25(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VTST_111100100dssnnnndddd1000nqm1mmmm_case_0TesterCase25
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000800) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1000 & B(4)=1 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1000nqm1mmmm,
//       rule: VCEQ_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0TesterCase26
    : public Arm32DecoderTester {
 public:
  VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0TesterCase26(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0TesterCase26
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000800) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1001 & B(4)=0 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm0mmmm,
//       rule: VMLA_integer_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0TesterCase27
    : public Arm32DecoderTester {
 public:
  VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0TesterCase27(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0TesterCase27
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000900) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1001 & B(4)=0 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm0mmmm,
//       rule: VMLS_integer_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0TesterCase28
    : public Arm32DecoderTester {
 public:
  VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0TesterCase28(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0TesterCase28
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000900) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1001 & B(4)=1 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm1mmmm,
//       rule: VMUL_integer_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0TesterCase29
    : public Arm32DecoderTester {
 public:
  VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0TesterCase29(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0TesterCase29
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000900) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1001 & B(4)=1 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1,
//       baseline: VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm1mmmm,
//       rule: VMUL_polynomial_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=~00 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0TesterCase30
    : public Arm32DecoderTester {
 public:
  VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0TesterCase30(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0TesterCase30
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000900) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1010 & B(4)=0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
//    = {Q: Q(6),
//       actual: Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1,
//       baseline: VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 1111001u0dssnnnndddd1010n0m0mmmm,
//       rule: VPMAX,
//       safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0TesterCase31
    : public Arm32DecoderTester {
 public:
  VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0TesterCase31(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0TesterCase31
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000A00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1010 & B(4)=1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
//    = {Q: Q(6),
//       actual: Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1,
//       baseline: VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 1111001u0dssnnnndddd1010n0m1mmmm,
//       rule: VPMIN,
//       safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0TesterCase32
    : public Arm32DecoderTester {
 public:
  VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0TesterCase32(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0TesterCase32
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000A00) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1011 & B(4)=0 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1,
//       baseline: VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1011nqm0mmmm,
//       rule: VQDMULH_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         (size(21:20)=11 ||
//            size(21:20)=00) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0TesterCase33
    : public Arm32DecoderTester {
 public:
  VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0TesterCase33(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0TesterCase33
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000B00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1011 & B(4)=0 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1,
//       baseline: VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1011nqm0mmmm,
//       rule: VQRDMULH_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         (size(21:20)=11 ||
//            size(21:20)=00) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0TesterCase34
    : public Arm32DecoderTester {
 public:
  VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0TesterCase34(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0TesterCase34
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000B00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1011 & B(4)=1 & U(24)=0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
//    = {Q: Q(6),
//       actual: Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1,
//       baseline: VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100100dssnnnndddd1011n0m1mmmm,
//       rule: VPADD_integer,
//       safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0TesterCase35
    : public Arm32DecoderTester {
 public:
  VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0TesterCase35(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0TesterCase35
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000B00) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1100 & B(4)=1 & U(24)=0 & C(21:20)=0x & $pattern(31:0)=xxxxxxxxxxx0xxxxxxxxxxxxxxxxxxxx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d00nnnndddd1100nqm1mmmm,
//       rule: VFMA_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0TesterCase36
    : public Arm32DecoderTester {
 public:
  VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0TesterCase36(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0TesterCase36
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1100
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000C00) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~0x
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxx0xxxxxxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x00100000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1100 & B(4)=1 & U(24)=0 & C(21:20)=1x & $pattern(31:0)=xxxxxxxxxxx0xxxxxxxxxxxxxxxxxxxx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d10nnnndddd1100nqm1mmmm,
//       rule: VFMS_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0TesterCase37
    : public Arm32DecoderTester {
 public:
  VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0TesterCase37(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0TesterCase37
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1100
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000C00) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~1x
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // $pattern(31:0)=~xxxxxxxxxxx0xxxxxxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x00100000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1101 & B(4)=0 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d0snnnndddd1101nqm0mmmm,
//       rule: VADD_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0TesterCase38
    : public Arm32DecoderTester {
 public:
  VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0TesterCase38(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0TesterCase38
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000D00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~0x
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1101 & B(4)=0 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d1snnnndddd1101nqm0mmmm,
//       rule: VSUB_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0TesterCase39
    : public Arm32DecoderTester {
 public:
  VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0TesterCase39(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0TesterCase39
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000D00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~1x
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1101 & B(4)=0 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       actual: Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1,
//       baseline: VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100110d0snnnndddd1101nqm0mmmm,
//       rule: VPADD_floating_point,
//       safety: [size(0)=1 ||
//            Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0TesterCase40
    : public Arm32DecoderTester {
 public:
  VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0TesterCase40(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0TesterCase40
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000D00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~0x
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1101 & B(4)=0 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d1snnnndddd1101nqm0mmmm,
//       rule: VABD_floating_point,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0TesterCase41
    : public Arm32DecoderTester {
 public:
  VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0TesterCase41(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0TesterCase41
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000D00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~1x
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1101 & B(4)=1 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dpsnnnndddd1101nqm1mmmm,
//       rule: VMLA_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0TesterCase42
    : public Arm32DecoderTester {
 public:
  VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0TesterCase42(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0TesterCase42
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000D00) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~0x
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1101 & B(4)=1 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dpsnnnndddd1101nqm1mmmm,
//       rule: VMLS_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0TesterCase43
    : public Arm32DecoderTester {
 public:
  VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0TesterCase43(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0TesterCase43
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000D00) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~1x
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1101 & B(4)=1 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d0snnnndddd1101nqm1mmmm,
//       rule: VMUL_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0TesterCase44
    : public Arm32DecoderTester {
 public:
  VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0TesterCase44(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0TesterCase44
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000D00) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~0x
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1110 & B(4)=0 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d0snnnndddd1110nqm0mmmm,
//       rule: VCEQ_register_A2,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0TesterCase45
    : public Arm32DecoderTester {
 public:
  VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0TesterCase45(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0TesterCase45
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000E00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~0x
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1110 & B(4)=0 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d0snnnndddd1110nqm0mmmm,
//       rule: VCGE_register_A2,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0TesterCase46
    : public Arm32DecoderTester {
 public:
  VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0TesterCase46(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0TesterCase46
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000E00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~0x
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1110 & B(4)=0 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d1snnnndddd1110nqm0mmmm,
//       rule: VCGT_register_A2,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0TesterCase47
    : public Arm32DecoderTester {
 public:
  VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0TesterCase47(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0TesterCase47
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000E00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~1x
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1110 & B(4)=1 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1110nqm1mmmm,
//       rule: VACGE,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0TesterCase48
    : public Arm32DecoderTester {
 public:
  VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0TesterCase48(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0TesterCase48
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000E00) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~0x
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1110 & B(4)=1 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1110nqm1mmmm,
//       rule: VACGT,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0TesterCase49
    : public Arm32DecoderTester {
 public:
  VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0TesterCase49(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0TesterCase49
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000E00) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~1x
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1111 & B(4)=0 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1111nqm0mmmm,
//       rule: VMAX_floating_point,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0TesterCase50
    : public Arm32DecoderTester {
 public:
  VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0TesterCase50(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0TesterCase50
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1111
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~0x
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1111 & B(4)=0 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1111nqm0mmmm,
//       rule: VMIN_floating_point,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0TesterCase51
    : public Arm32DecoderTester {
 public:
  VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0TesterCase51(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0TesterCase51
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1111
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~1x
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1111 & B(4)=0 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       actual: Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1,
//       baseline: VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100110dssnnnndddd1111nqm0mmmm,
//       rule: VPMAX,
//       safety: [size(0)=1 ||
//            Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0TesterCase52
    : public Arm32DecoderTester {
 public:
  VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0TesterCase52(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0TesterCase52
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1111
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~0x
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1111 & B(4)=0 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       actual: Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1,
//       baseline: VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100110dssnnnndddd1111nqm0mmmm,
//       rule: VPMIN,
//       safety: [size(0)=1 ||
//            Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0TesterCase53
    : public Arm32DecoderTester {
 public:
  VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0TesterCase53(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0TesterCase53
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1111
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;
  // B(4)=~0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;
  // C(21:20)=~1x
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1111 & B(4)=1 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d0snnnndddd1111nqm1mmmm,
//       rule: VRECPS,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0TesterCase54
    : public Arm32DecoderTester {
 public:
  VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0TesterCase54(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0TesterCase54
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1111
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~0x
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1111 & B(4)=1 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d1snnnndddd1111nqm1mmmm,
//       rule: VRSQRTS,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0TesterCase55
    : public Arm32DecoderTester {
 public:
  VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0TesterCase55(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0TesterCase55
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1111
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;
  // B(4)=~1
  if ((inst.Bits() & 0x00000010)  !=
          0x00000010) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;
  // C(21:20)=~1x
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// A(11:8)=0000 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0000nqm0mmmm,
//       rule: VHADD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0Tester_Case0
    : public VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0TesterCase0 {
 public:
  VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0Tester_Case0()
    : VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0TesterCase0(
      state_.VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0_VHADD_instance_)
  {}
};

// A(11:8)=0000 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0000nqm1mmmm,
//       rule: VQADD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0Tester_Case1
    : public VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0TesterCase1 {
 public:
  VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0Tester_Case1()
    : VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0TesterCase1(
      state_.VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0_VQADD_instance_)
  {}
};

// A(11:8)=0001 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0001nqm0mmmm,
//       rule: VRHADD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0Tester_Case2
    : public VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0TesterCase2 {
 public:
  VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0Tester_Case2()
    : VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0TesterCase2(
      state_.VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0_VRHADD_instance_)
  {}
};

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=00
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d00nnnndddd0001nqm1mmmm,
//       rule: VAND_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0Tester_Case3
    : public VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0TesterCase3 {
 public:
  VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0Tester_Case3()
    : VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0TesterCase3(
      state_.VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0_VAND_register_instance_)
  {}
};

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=01
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d01nnnndddd0001nqm1mmmm,
//       rule: VBIC_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0Tester_Case4
    : public VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0TesterCase4 {
 public:
  VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0Tester_Case4()
    : VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0TesterCase4(
      state_.VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0_VBIC_register_instance_)
  {}
};

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=10
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d10nnnndddd0001nqm1mmmm,
//       rule: VORR_register_or_VMOV_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0Tester_Case5
    : public VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0TesterCase5 {
 public:
  VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0Tester_Case5()
    : VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0TesterCase5(
      state_.VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0_VORR_register_or_VMOV_register_A1_instance_)
  {}
};

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=11
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d11nnnndddd0001nqm1mmmm,
//       rule: VORN_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0Tester_Case6
    : public VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0TesterCase6 {
 public:
  VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0Tester_Case6()
    : VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0TesterCase6(
      state_.VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0_VORN_register_instance_)
  {}
};

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=00
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d00nnnndddd0001nqm1mmmm,
//       rule: VEOR,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0Tester_Case7
    : public VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0TesterCase7 {
 public:
  VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0Tester_Case7()
    : VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0TesterCase7(
      state_.VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0_VEOR_instance_)
  {}
};

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=01
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d01nnnndddd0001nqm1mmmm,
//       rule: VBSL,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0Tester_Case8
    : public VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0TesterCase8 {
 public:
  VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0Tester_Case8()
    : VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0TesterCase8(
      state_.VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0_VBSL_instance_)
  {}
};

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=10
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d10nnnndddd0001nqm1mmmm,
//       rule: VBIT,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0Tester_Case9
    : public VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0TesterCase9 {
 public:
  VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0Tester_Case9()
    : VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0TesterCase9(
      state_.VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0_VBIT_instance_)
  {}
};

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=11
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d11nnnndddd0001nqm1mmmm,
//       rule: VBIF,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0Tester_Case10
    : public VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0TesterCase10 {
 public:
  VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0Tester_Case10()
    : VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0TesterCase10(
      state_.VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0_VBIF_instance_)
  {}
};

// A(11:8)=0010 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0010nqm0mmmm,
//       rule: VHSUB,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0Tester_Case11
    : public VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0TesterCase11 {
 public:
  VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0Tester_Case11()
    : VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0TesterCase11(
      state_.VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0_VHSUB_instance_)
  {}
};

// A(11:8)=0010 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0010nqm1mmmm,
//       rule: VQSUB,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0Tester_Case12
    : public VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0TesterCase12 {
 public:
  VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0Tester_Case12()
    : VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0TesterCase12(
      state_.VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0_VQSUB_instance_)
  {}
};

// A(11:8)=0011 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0011nqm0mmmm,
//       rule: VCGT_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0Tester_Case13
    : public VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0TesterCase13 {
 public:
  VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0Tester_Case13()
    : VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0TesterCase13(
      state_.VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0_VCGT_register_A1_instance_)
  {}
};

// A(11:8)=0011 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0011nqm1mmmm,
//       rule: VCGE_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0Tester_Case14
    : public VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0TesterCase14 {
 public:
  VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0Tester_Case14()
    : VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0TesterCase14(
      state_.VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0_VCGE_register_A1_instance_)
  {}
};

// A(11:8)=0100 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0100nqm0mmmm,
//       rule: VSHL_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0Tester_Case15
    : public VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0TesterCase15 {
 public:
  VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0Tester_Case15()
    : VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0TesterCase15(
      state_.VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0_VSHL_register_instance_)
  {}
};

// A(11:8)=0100 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0100nqm1mmmm,
//       rule: VQSHL_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0Tester_Case16
    : public VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0TesterCase16 {
 public:
  VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0Tester_Case16()
    : VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0TesterCase16(
      state_.VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0_VQSHL_register_instance_)
  {}
};

// A(11:8)=0101 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0101nqm0mmmm,
//       rule: VRSHL,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0Tester_Case17
    : public VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0TesterCase17 {
 public:
  VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0Tester_Case17()
    : VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0TesterCase17(
      state_.VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0_VRSHL_instance_)
  {}
};

// A(11:8)=0101 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0101nqm1mmmm,
//       rule: VQRSHL,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0Tester_Case18
    : public VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0TesterCase18 {
 public:
  VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0Tester_Case18()
    : VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0TesterCase18(
      state_.VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0_VQRSHL_instance_)
  {}
};

// A(11:8)=0110 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0110nqm0mmmm,
//       rule: VMAX,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0Tester_Case19
    : public VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0TesterCase19 {
 public:
  VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0Tester_Case19()
    : VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0TesterCase19(
      state_.VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0_VMAX_instance_)
  {}
};

// A(11:8)=0110 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0110nqm1mmmm,
//       rule: VMIN,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0Tester_Case20
    : public VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0TesterCase20 {
 public:
  VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0Tester_Case20()
    : VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0TesterCase20(
      state_.VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0_VMIN_instance_)
  {}
};

// A(11:8)=0111 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0111nqm0mmmm,
//       rule: VABD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0Tester_Case21
    : public VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0TesterCase21 {
 public:
  VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0Tester_Case21()
    : VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0TesterCase21(
      state_.VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0_VABD_instance_)
  {}
};

// A(11:8)=0111 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0111nqm1mmmm,
//       rule: VABA,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0Tester_Case22
    : public VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0TesterCase22 {
 public:
  VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0Tester_Case22()
    : VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0TesterCase22(
      state_.VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0_VABA_instance_)
  {}
};

// A(11:8)=1000 & B(4)=0 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1000nqm0mmmm,
//       rule: VADD_integer,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0Tester_Case23
    : public VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0TesterCase23 {
 public:
  VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0Tester_Case23()
    : VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0TesterCase23(
      state_.VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0_VADD_integer_instance_)
  {}
};

// A(11:8)=1000 & B(4)=0 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1000nqm0mmmm,
//       rule: VSUB_integer,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
class VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0Tester_Case24
    : public VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0TesterCase24 {
 public:
  VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0Tester_Case24()
    : VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0TesterCase24(
      state_.VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0_VSUB_integer_instance_)
  {}
};

// A(11:8)=1000 & B(4)=1 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VTST_111100100dssnnnndddd1000nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1000nqm1mmmm,
//       rule: VTST,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VTST_111100100dssnnnndddd1000nqm1mmmm_case_0Tester_Case25
    : public VTST_111100100dssnnnndddd1000nqm1mmmm_case_0TesterCase25 {
 public:
  VTST_111100100dssnnnndddd1000nqm1mmmm_case_0Tester_Case25()
    : VTST_111100100dssnnnndddd1000nqm1mmmm_case_0TesterCase25(
      state_.VTST_111100100dssnnnndddd1000nqm1mmmm_case_0_VTST_instance_)
  {}
};

// A(11:8)=1000 & B(4)=1 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1000nqm1mmmm,
//       rule: VCEQ_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0Tester_Case26
    : public VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0TesterCase26 {
 public:
  VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0Tester_Case26()
    : VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0TesterCase26(
      state_.VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0_VCEQ_register_A1_instance_)
  {}
};

// A(11:8)=1001 & B(4)=0 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm0mmmm,
//       rule: VMLA_integer_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0Tester_Case27
    : public VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0TesterCase27 {
 public:
  VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0Tester_Case27()
    : VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0TesterCase27(
      state_.VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0_VMLA_integer_A1_instance_)
  {}
};

// A(11:8)=1001 & B(4)=0 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm0mmmm,
//       rule: VMLS_integer_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0Tester_Case28
    : public VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0TesterCase28 {
 public:
  VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0Tester_Case28()
    : VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0TesterCase28(
      state_.VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0_VMLS_integer_A1_instance_)
  {}
};

// A(11:8)=1001 & B(4)=1 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm1mmmm,
//       rule: VMUL_integer_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0Tester_Case29
    : public VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0TesterCase29 {
 public:
  VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0Tester_Case29()
    : VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0TesterCase29(
      state_.VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0_VMUL_integer_A1_instance_)
  {}
};

// A(11:8)=1001 & B(4)=1 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1,
//       baseline: VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm1mmmm,
//       rule: VMUL_polynomial_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=~00 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0Tester_Case30
    : public VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0TesterCase30 {
 public:
  VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0Tester_Case30()
    : VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0TesterCase30(
      state_.VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0_VMUL_polynomial_A1_instance_)
  {}
};

// A(11:8)=1010 & B(4)=0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
//    = {Q: Q(6),
//       actual: Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1,
//       baseline: VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 1111001u0dssnnnndddd1010n0m0mmmm,
//       rule: VPMAX,
//       safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0Tester_Case31
    : public VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0TesterCase31 {
 public:
  VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0Tester_Case31()
    : VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0TesterCase31(
      state_.VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0_VPMAX_instance_)
  {}
};

// A(11:8)=1010 & B(4)=1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
//    = {Q: Q(6),
//       actual: Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1,
//       baseline: VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 1111001u0dssnnnndddd1010n0m1mmmm,
//       rule: VPMIN,
//       safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0Tester_Case32
    : public VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0TesterCase32 {
 public:
  VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0Tester_Case32()
    : VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0TesterCase32(
      state_.VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0_VPMIN_instance_)
  {}
};

// A(11:8)=1011 & B(4)=0 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1,
//       baseline: VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1011nqm0mmmm,
//       rule: VQDMULH_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         (size(21:20)=11 ||
//            size(21:20)=00) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0Tester_Case33
    : public VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0TesterCase33 {
 public:
  VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0Tester_Case33()
    : VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0TesterCase33(
      state_.VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0_VQDMULH_A1_instance_)
  {}
};

// A(11:8)=1011 & B(4)=0 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1,
//       baseline: VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1011nqm0mmmm,
//       rule: VQRDMULH_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         (size(21:20)=11 ||
//            size(21:20)=00) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0Tester_Case34
    : public VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0TesterCase34 {
 public:
  VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0Tester_Case34()
    : VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0TesterCase34(
      state_.VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0_VQRDMULH_A1_instance_)
  {}
};

// A(11:8)=1011 & B(4)=1 & U(24)=0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
//    = {Q: Q(6),
//       actual: Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1,
//       baseline: VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100100dssnnnndddd1011n0m1mmmm,
//       rule: VPADD_integer,
//       safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0Tester_Case35
    : public VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0TesterCase35 {
 public:
  VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0Tester_Case35()
    : VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0TesterCase35(
      state_.VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0_VPADD_integer_instance_)
  {}
};

// A(11:8)=1100 & B(4)=1 & U(24)=0 & C(21:20)=0x & $pattern(31:0)=xxxxxxxxxxx0xxxxxxxxxxxxxxxxxxxx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d00nnnndddd1100nqm1mmmm,
//       rule: VFMA_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0Tester_Case36
    : public VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0TesterCase36 {
 public:
  VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0Tester_Case36()
    : VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0TesterCase36(
      state_.VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0_VFMA_A1_instance_)
  {}
};

// A(11:8)=1100 & B(4)=1 & U(24)=0 & C(21:20)=1x & $pattern(31:0)=xxxxxxxxxxx0xxxxxxxxxxxxxxxxxxxx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d10nnnndddd1100nqm1mmmm,
//       rule: VFMS_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0Tester_Case37
    : public VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0TesterCase37 {
 public:
  VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0Tester_Case37()
    : VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0TesterCase37(
      state_.VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0_VFMS_A1_instance_)
  {}
};

// A(11:8)=1101 & B(4)=0 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d0snnnndddd1101nqm0mmmm,
//       rule: VADD_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0Tester_Case38
    : public VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0TesterCase38 {
 public:
  VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0Tester_Case38()
    : VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0TesterCase38(
      state_.VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0_VADD_floating_point_A1_instance_)
  {}
};

// A(11:8)=1101 & B(4)=0 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d1snnnndddd1101nqm0mmmm,
//       rule: VSUB_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0Tester_Case39
    : public VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0TesterCase39 {
 public:
  VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0Tester_Case39()
    : VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0TesterCase39(
      state_.VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0_VSUB_floating_point_A1_instance_)
  {}
};

// A(11:8)=1101 & B(4)=0 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       actual: Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1,
//       baseline: VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100110d0snnnndddd1101nqm0mmmm,
//       rule: VPADD_floating_point,
//       safety: [size(0)=1 ||
//            Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0Tester_Case40
    : public VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0TesterCase40 {
 public:
  VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0Tester_Case40()
    : VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0TesterCase40(
      state_.VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0_VPADD_floating_point_instance_)
  {}
};

// A(11:8)=1101 & B(4)=0 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d1snnnndddd1101nqm0mmmm,
//       rule: VABD_floating_point,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0Tester_Case41
    : public VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0TesterCase41 {
 public:
  VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0Tester_Case41()
    : VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0TesterCase41(
      state_.VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0_VABD_floating_point_instance_)
  {}
};

// A(11:8)=1101 & B(4)=1 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dpsnnnndddd1101nqm1mmmm,
//       rule: VMLA_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0Tester_Case42
    : public VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0TesterCase42 {
 public:
  VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0Tester_Case42()
    : VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0TesterCase42(
      state_.VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0_VMLA_floating_point_A1_instance_)
  {}
};

// A(11:8)=1101 & B(4)=1 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dpsnnnndddd1101nqm1mmmm,
//       rule: VMLS_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0Tester_Case43
    : public VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0TesterCase43 {
 public:
  VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0Tester_Case43()
    : VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0TesterCase43(
      state_.VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0_VMLS_floating_point_A1_instance_)
  {}
};

// A(11:8)=1101 & B(4)=1 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d0snnnndddd1101nqm1mmmm,
//       rule: VMUL_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0Tester_Case44
    : public VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0TesterCase44 {
 public:
  VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0Tester_Case44()
    : VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0TesterCase44(
      state_.VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0_VMUL_floating_point_A1_instance_)
  {}
};

// A(11:8)=1110 & B(4)=0 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d0snnnndddd1110nqm0mmmm,
//       rule: VCEQ_register_A2,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0Tester_Case45
    : public VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0TesterCase45 {
 public:
  VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0Tester_Case45()
    : VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0TesterCase45(
      state_.VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0_VCEQ_register_A2_instance_)
  {}
};

// A(11:8)=1110 & B(4)=0 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d0snnnndddd1110nqm0mmmm,
//       rule: VCGE_register_A2,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0Tester_Case46
    : public VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0TesterCase46 {
 public:
  VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0Tester_Case46()
    : VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0TesterCase46(
      state_.VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0_VCGE_register_A2_instance_)
  {}
};

// A(11:8)=1110 & B(4)=0 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d1snnnndddd1110nqm0mmmm,
//       rule: VCGT_register_A2,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0Tester_Case47
    : public VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0TesterCase47 {
 public:
  VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0Tester_Case47()
    : VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0TesterCase47(
      state_.VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0_VCGT_register_A2_instance_)
  {}
};

// A(11:8)=1110 & B(4)=1 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1110nqm1mmmm,
//       rule: VACGE,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0Tester_Case48
    : public VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0TesterCase48 {
 public:
  VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0Tester_Case48()
    : VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0TesterCase48(
      state_.VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0_VACGE_instance_)
  {}
};

// A(11:8)=1110 & B(4)=1 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1110nqm1mmmm,
//       rule: VACGT,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0Tester_Case49
    : public VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0TesterCase49 {
 public:
  VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0Tester_Case49()
    : VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0TesterCase49(
      state_.VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0_VACGT_instance_)
  {}
};

// A(11:8)=1111 & B(4)=0 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1111nqm0mmmm,
//       rule: VMAX_floating_point,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0Tester_Case50
    : public VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0TesterCase50 {
 public:
  VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0Tester_Case50()
    : VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0TesterCase50(
      state_.VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0_VMAX_floating_point_instance_)
  {}
};

// A(11:8)=1111 & B(4)=0 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1111nqm0mmmm,
//       rule: VMIN_floating_point,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0Tester_Case51
    : public VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0TesterCase51 {
 public:
  VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0Tester_Case51()
    : VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0TesterCase51(
      state_.VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0_VMIN_floating_point_instance_)
  {}
};

// A(11:8)=1111 & B(4)=0 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       actual: Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1,
//       baseline: VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100110dssnnnndddd1111nqm0mmmm,
//       rule: VPMAX,
//       safety: [size(0)=1 ||
//            Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0Tester_Case52
    : public VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0TesterCase52 {
 public:
  VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0Tester_Case52()
    : VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0TesterCase52(
      state_.VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0_VPMAX_instance_)
  {}
};

// A(11:8)=1111 & B(4)=0 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       actual: Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1,
//       baseline: VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100110dssnnnndddd1111nqm0mmmm,
//       rule: VPMIN,
//       safety: [size(0)=1 ||
//            Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0Tester_Case53
    : public VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0TesterCase53 {
 public:
  VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0Tester_Case53()
    : VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0TesterCase53(
      state_.VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0_VPMIN_instance_)
  {}
};

// A(11:8)=1111 & B(4)=1 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d0snnnndddd1111nqm1mmmm,
//       rule: VRECPS,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0Tester_Case54
    : public VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0TesterCase54 {
 public:
  VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0Tester_Case54()
    : VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0TesterCase54(
      state_.VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0_VRECPS_instance_)
  {}
};

// A(11:8)=1111 & B(4)=1 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d1snnnndddd1111nqm1mmmm,
//       rule: VRSQRTS,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0Tester_Case55
    : public VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0TesterCase55 {
 public:
  VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0Tester_Case55()
    : VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0TesterCase55(
      state_.VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0_VRSQRTS_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// A(11:8)=0000 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0000nqm0mmmm,
//       rule: VHADD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0Tester_Case0_TestCase0) {
  VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VHADD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0000nqm0mmmm");
}

// A(11:8)=0000 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0000nqm1mmmm,
//       rule: VQADD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0Tester_Case1_TestCase1) {
  VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VQADD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0000nqm1mmmm");
}

// A(11:8)=0001 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0001nqm0mmmm,
//       rule: VRHADD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0Tester_Case2_TestCase2) {
  VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VRHADD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0001nqm0mmmm");
}

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=00
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d00nnnndddd0001nqm1mmmm,
//       rule: VAND_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0Tester_Case3_TestCase3) {
  VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VAND_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100d00nnnndddd0001nqm1mmmm");
}

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=01
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d01nnnndddd0001nqm1mmmm,
//       rule: VBIC_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0Tester_Case4_TestCase4) {
  VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0Tester_Case4 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VBIC_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100d01nnnndddd0001nqm1mmmm");
}

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=10
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d10nnnndddd0001nqm1mmmm,
//       rule: VORR_register_or_VMOV_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0Tester_Case5_TestCase5) {
  VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0Tester_Case5 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VORR_register_or_VMOV_register_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100d10nnnndddd0001nqm1mmmm");
}

// A(11:8)=0001 & B(4)=1 & U(24)=0 & C(21:20)=11
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d11nnnndddd0001nqm1mmmm,
//       rule: VORN_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0Tester_Case6_TestCase6) {
  VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VORN_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100d11nnnndddd0001nqm1mmmm");
}

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=00
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d00nnnndddd0001nqm1mmmm,
//       rule: VEOR,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0Tester_Case7_TestCase7) {
  VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VEOR actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110d00nnnndddd0001nqm1mmmm");
}

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=01
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d01nnnndddd0001nqm1mmmm,
//       rule: VBSL,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0Tester_Case8_TestCase8) {
  VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0Tester_Case8 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VBSL actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110d01nnnndddd0001nqm1mmmm");
}

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=10
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d10nnnndddd0001nqm1mmmm,
//       rule: VBIT,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0Tester_Case9_TestCase9) {
  VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0Tester_Case9 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VBIT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110d10nnnndddd0001nqm1mmmm");
}

// A(11:8)=0001 & B(4)=1 & U(24)=1 & C(21:20)=11
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d11nnnndddd0001nqm1mmmm,
//       rule: VBIF,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0Tester_Case10_TestCase10) {
  VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0Tester_Case10 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VBIF actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110d11nnnndddd0001nqm1mmmm");
}

// A(11:8)=0010 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0010nqm0mmmm,
//       rule: VHSUB,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0Tester_Case11_TestCase11) {
  VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0Tester_Case11 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VHSUB actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0010nqm0mmmm");
}

// A(11:8)=0010 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0010nqm1mmmm,
//       rule: VQSUB,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0Tester_Case12_TestCase12) {
  VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0Tester_Case12 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VQSUB actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0010nqm1mmmm");
}

// A(11:8)=0011 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0011nqm0mmmm,
//       rule: VCGT_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0Tester_Case13_TestCase13) {
  VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0Tester_Case13 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VCGT_register_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0011nqm0mmmm");
}

// A(11:8)=0011 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0011nqm1mmmm,
//       rule: VCGE_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0Tester_Case14_TestCase14) {
  VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0Tester_Case14 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VCGE_register_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0011nqm1mmmm");
}

// A(11:8)=0100 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0100nqm0mmmm,
//       rule: VSHL_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0Tester_Case15_TestCase15) {
  VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0Tester_Case15 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VSHL_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0100nqm0mmmm");
}

// A(11:8)=0100 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0100nqm1mmmm,
//       rule: VQSHL_register,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0Tester_Case16_TestCase16) {
  VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0Tester_Case16 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VQSHL_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0100nqm1mmmm");
}

// A(11:8)=0101 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0101nqm0mmmm,
//       rule: VRSHL,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0Tester_Case17_TestCase17) {
  VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0Tester_Case17 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VRSHL actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0101nqm0mmmm");
}

// A(11:8)=0101 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0101nqm1mmmm,
//       rule: VQRSHL,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0Tester_Case18_TestCase18) {
  VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0Tester_Case18 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VQRSHL actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0101nqm1mmmm");
}

// A(11:8)=0110 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0110nqm0mmmm,
//       rule: VMAX,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0Tester_Case19_TestCase19) {
  VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0Tester_Case19 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VMAX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0110nqm0mmmm");
}

// A(11:8)=0110 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0110nqm1mmmm,
//       rule: VMIN,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0Tester_Case20_TestCase20) {
  VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0Tester_Case20 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VMIN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0110nqm1mmmm");
}

// A(11:8)=0111 & B(4)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0111nqm0mmmm,
//       rule: VABD,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0Tester_Case21_TestCase21) {
  VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0Tester_Case21 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VABD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0111nqm0mmmm");
}

// A(11:8)=0111 & B(4)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd0111nqm1mmmm,
//       rule: VABA,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0Tester_Case22_TestCase22) {
  VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0Tester_Case22 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VABA actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd0111nqm1mmmm");
}

// A(11:8)=1000 & B(4)=0 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1000nqm0mmmm,
//       rule: VADD_integer,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0Tester_Case23_TestCase23) {
  VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0Tester_Case23 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VADD_integer actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100dssnnnndddd1000nqm0mmmm");
}

// A(11:8)=1000 & B(4)=0 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1,
//       baseline: VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0,
//       defs: {},
//       fields: [Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1000nqm0mmmm,
//       rule: VSUB_integer,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0Tester_Case24_TestCase24) {
  VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0Tester_Case24 baseline_tester;
  NamedActual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_VSUB_integer actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110dssnnnndddd1000nqm0mmmm");
}

// A(11:8)=1000 & B(4)=1 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VTST_111100100dssnnnndddd1000nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1000nqm1mmmm,
//       rule: VTST,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VTST_111100100dssnnnndddd1000nqm1mmmm_case_0Tester_Case25_TestCase25) {
  VTST_111100100dssnnnndddd1000nqm1mmmm_case_0Tester_Case25 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VTST actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100dssnnnndddd1000nqm1mmmm");
}

// A(11:8)=1000 & B(4)=1 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1000nqm1mmmm,
//       rule: VCEQ_register_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0Tester_Case26_TestCase26) {
  VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0Tester_Case26 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VCEQ_register_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110dssnnnndddd1000nqm1mmmm");
}

// A(11:8)=1001 & B(4)=0 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm0mmmm,
//       rule: VMLA_integer_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0Tester_Case27_TestCase27) {
  VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0Tester_Case27 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VMLA_integer_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd1001nqm0mmmm");
}

// A(11:8)=1001 & B(4)=0 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm0mmmm,
//       rule: VMLS_integer_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0Tester_Case28_TestCase28) {
  VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0Tester_Case28 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VMLS_integer_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd1001nqm0mmmm");
}

// A(11:8)=1001 & B(4)=1 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//       baseline: VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm1mmmm,
//       rule: VMUL_integer_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=11 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0Tester_Case29_TestCase29) {
  VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0Tester_Case29 baseline_tester;
  NamedActual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_VMUL_integer_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd1001nqm1mmmm");
}

// A(11:8)=1001 & B(4)=1 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1,
//       baseline: VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 1111001u0dssnnnndddd1001nqm1mmmm,
//       rule: VMUL_polynomial_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(21:20)=~00 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0Tester_Case30_TestCase30) {
  VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0Tester_Case30 baseline_tester;
  NamedActual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1_VMUL_polynomial_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd1001nqm1mmmm");
}

// A(11:8)=1010 & B(4)=0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
//    = {Q: Q(6),
//       actual: Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1,
//       baseline: VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 1111001u0dssnnnndddd1010n0m0mmmm,
//       rule: VPMAX,
//       safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0Tester_Case31_TestCase31) {
  VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0Tester_Case31 baseline_tester;
  NamedActual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1_VPMAX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd1010n0m0mmmm");
}

// A(11:8)=1010 & B(4)=1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
//    = {Q: Q(6),
//       actual: Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1,
//       baseline: VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 1111001u0dssnnnndddd1010n0m1mmmm,
//       rule: VPMIN,
//       safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0Tester_Case32_TestCase32) {
  VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0Tester_Case32 baseline_tester;
  NamedActual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1_VPMIN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u0dssnnnndddd1010n0m1mmmm");
}

// A(11:8)=1011 & B(4)=0 & U(24)=0
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1,
//       baseline: VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1011nqm0mmmm,
//       rule: VQDMULH_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         (size(21:20)=11 ||
//            size(21:20)=00) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0Tester_Case33_TestCase33) {
  VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0Tester_Case33 baseline_tester;
  NamedActual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1_VQDMULH_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100dssnnnndddd1011nqm0mmmm");
}

// A(11:8)=1011 & B(4)=0 & U(24)=1
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1,
//       baseline: VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1011nqm0mmmm,
//       rule: VQRDMULH_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         (size(21:20)=11 ||
//            size(21:20)=00) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0Tester_Case34_TestCase34) {
  VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0Tester_Case34 baseline_tester;
  NamedActual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1_VQRDMULH_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110dssnnnndddd1011nqm0mmmm");
}

// A(11:8)=1011 & B(4)=1 & U(24)=0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx
//    = {Q: Q(6),
//       actual: Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1,
//       baseline: VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100100dssnnnndddd1011n0m1mmmm,
//       rule: VPADD_integer,
//       safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0Tester_Case35_TestCase35) {
  VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0Tester_Case35 baseline_tester;
  NamedActual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1_VPADD_integer actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100dssnnnndddd1011n0m1mmmm");
}

// A(11:8)=1100 & B(4)=1 & U(24)=0 & C(21:20)=0x & $pattern(31:0)=xxxxxxxxxxx0xxxxxxxxxxxxxxxxxxxx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d00nnnndddd1100nqm1mmmm,
//       rule: VFMA_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0Tester_Case36_TestCase36) {
  VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0Tester_Case36 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VFMA_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100d00nnnndddd1100nqm1mmmm");
}

// A(11:8)=1100 & B(4)=1 & U(24)=0 & C(21:20)=1x & $pattern(31:0)=xxxxxxxxxxx0xxxxxxxxxxxxxxxxxxxx
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d10nnnndddd1100nqm1mmmm,
//       rule: VFMS_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0Tester_Case37_TestCase37) {
  VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0Tester_Case37 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VFMS_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100d10nnnndddd1100nqm1mmmm");
}

// A(11:8)=1101 & B(4)=0 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d0snnnndddd1101nqm0mmmm,
//       rule: VADD_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0Tester_Case38_TestCase38) {
  VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0Tester_Case38 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VADD_floating_point_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100d0snnnndddd1101nqm0mmmm");
}

// A(11:8)=1101 & B(4)=0 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d1snnnndddd1101nqm0mmmm,
//       rule: VSUB_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0Tester_Case39_TestCase39) {
  VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0Tester_Case39 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VSUB_floating_point_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100d1snnnndddd1101nqm0mmmm");
}

// A(11:8)=1101 & B(4)=0 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       actual: Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1,
//       baseline: VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100110d0snnnndddd1101nqm0mmmm,
//       rule: VPADD_floating_point,
//       safety: [size(0)=1 ||
//            Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0Tester_Case40_TestCase40) {
  VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0Tester_Case40 baseline_tester;
  NamedActual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1_VPADD_floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110d0snnnndddd1101nqm0mmmm");
}

// A(11:8)=1101 & B(4)=0 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d1snnnndddd1101nqm0mmmm,
//       rule: VABD_floating_point,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0Tester_Case41_TestCase41) {
  VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0Tester_Case41 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VABD_floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110d1snnnndddd1101nqm0mmmm");
}

// A(11:8)=1101 & B(4)=1 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dpsnnnndddd1101nqm1mmmm,
//       rule: VMLA_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0Tester_Case42_TestCase42) {
  VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0Tester_Case42 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VMLA_floating_point_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100dpsnnnndddd1101nqm1mmmm");
}

// A(11:8)=1101 & B(4)=1 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dpsnnnndddd1101nqm1mmmm,
//       rule: VMLS_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0Tester_Case43_TestCase43) {
  VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0Tester_Case43 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VMLS_floating_point_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100dpsnnnndddd1101nqm1mmmm");
}

// A(11:8)=1101 & B(4)=1 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d0snnnndddd1101nqm1mmmm,
//       rule: VMUL_floating_point_A1,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0Tester_Case44_TestCase44) {
  VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0Tester_Case44 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VMUL_floating_point_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110d0snnnndddd1101nqm1mmmm");
}

// A(11:8)=1110 & B(4)=0 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d0snnnndddd1110nqm0mmmm,
//       rule: VCEQ_register_A2,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0Tester_Case45_TestCase45) {
  VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0Tester_Case45 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VCEQ_register_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100d0snnnndddd1110nqm0mmmm");
}

// A(11:8)=1110 & B(4)=0 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d0snnnndddd1110nqm0mmmm,
//       rule: VCGE_register_A2,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0Tester_Case46_TestCase46) {
  VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0Tester_Case46 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VCGE_register_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110d0snnnndddd1110nqm0mmmm");
}

// A(11:8)=1110 & B(4)=0 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110d1snnnndddd1110nqm0mmmm,
//       rule: VCGT_register_A2,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0Tester_Case47_TestCase47) {
  VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0Tester_Case47 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VCGT_register_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110d1snnnndddd1110nqm0mmmm");
}

// A(11:8)=1110 & B(4)=1 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1110nqm1mmmm,
//       rule: VACGE,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0Tester_Case48_TestCase48) {
  VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0Tester_Case48 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VACGE actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110dssnnnndddd1110nqm1mmmm");
}

// A(11:8)=1110 & B(4)=1 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100110dssnnnndddd1110nqm1mmmm,
//       rule: VACGT,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0Tester_Case49_TestCase49) {
  VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0Tester_Case49 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VACGT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110dssnnnndddd1110nqm1mmmm");
}

// A(11:8)=1111 & B(4)=0 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1111nqm0mmmm,
//       rule: VMAX_floating_point,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0Tester_Case50_TestCase50) {
  VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0Tester_Case50 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VMAX_floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100dssnnnndddd1111nqm0mmmm");
}

// A(11:8)=1111 & B(4)=0 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100dssnnnndddd1111nqm0mmmm,
//       rule: VMIN_floating_point,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0Tester_Case51_TestCase51) {
  VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0Tester_Case51 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VMIN_floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100dssnnnndddd1111nqm0mmmm");
}

// A(11:8)=1111 & B(4)=0 & U(24)=1 & C(21:20)=0x
//    = {Q: Q(6),
//       actual: Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1,
//       baseline: VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100110dssnnnndddd1111nqm0mmmm,
//       rule: VPMAX,
//       safety: [size(0)=1 ||
//            Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0Tester_Case52_TestCase52) {
  VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0Tester_Case52 baseline_tester;
  NamedActual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1_VPMAX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110dssnnnndddd1111nqm0mmmm");
}

// A(11:8)=1111 & B(4)=0 & U(24)=1 & C(21:20)=1x
//    = {Q: Q(6),
//       actual: Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1,
//       baseline: VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Q(6)],
//       pattern: 111100110dssnnnndddd1111nqm0mmmm,
//       rule: VPMIN,
//       safety: [size(0)=1 ||
//            Q(6)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0Tester_Case53_TestCase53) {
  VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0Tester_Case53 baseline_tester;
  NamedActual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1_VPMIN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100110dssnnnndddd1111nqm0mmmm");
}

// A(11:8)=1111 & B(4)=1 & U(24)=0 & C(21:20)=0x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d0snnnndddd1111nqm1mmmm,
//       rule: VRECPS,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0Tester_Case54_TestCase54) {
  VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0Tester_Case54 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VRECPS actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100d0snnnndddd1111nqm1mmmm");
}

// A(11:8)=1111 & B(4)=1 & U(24)=0 & C(21:20)=1x
//    = {Q: Q(6),
//       Vd: Vd(15:12),
//       Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1,
//       baseline: VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), Q(6), Vm(3:0)],
//       pattern: 111100100d1snnnndddd1111nqm1mmmm,
//       rule: VRSQRTS,
//       safety: [Q(6)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1 ||
//            Vm(0)=1) => UNDEFINED,
//         size(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0Tester_Case55_TestCase55) {
  VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0Tester_Case55 baseline_tester;
  NamedActual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_VRSQRTS actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100100d1snnnndddd1111nqm1mmmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
