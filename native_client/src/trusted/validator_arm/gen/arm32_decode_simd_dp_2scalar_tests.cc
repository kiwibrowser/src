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
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLA_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1TesterCase0
    : public Arm32DecoderTester {
 public:
  VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1TesterCase0
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
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       baseline: VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLA_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            size(21:20)=01) => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0TesterCase1
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
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0p10n1m0mmmm,
//       rule: VMLAL_by_scalar_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0TesterCase2
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

// A(11:8)=0011 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd0p11n1m0mmmm,
//       rule: VQDMLAL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000300) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0100
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLS_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1TesterCase4
    : public Arm32DecoderTester {
 public:
  VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0100
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000400) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0101
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       baseline: VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLS_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            size(21:20)=01) => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000500) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0110
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0p10n1m0mmmm,
//       rule: VMLSL_by_scalar_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000600) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0111 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd0p11n1m0mmmm,
//       rule: VQDMLSL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0111
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000700) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1000
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd100fn1m0mmmm,
//       rule: VMUL_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1TesterCase8
    : public Arm32DecoderTester {
 public:
  VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000800) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1001
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       baseline: VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd100fn1m0mmmm,
//       rule: VMUL_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            size(21:20)=01) => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000900) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1010
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd1010n1m0mmmm,
//       rule: VMULL_by_scalar_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000A00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1011 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd1011n1m0mmmm,
//       rule: VQDMULL_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000B00) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1100
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd1100n1m0mmmm,
//       rule: VQDMULH_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1100
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000C00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1101
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd1101n1m0mmmm,
//       rule: VQRDMULH,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000D00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// A(11:8)=0000
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLA_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1Tester_Case0
    : public VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1TesterCase0 {
 public:
  VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1Tester_Case0()
    : VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1TesterCase0(
      state_.VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1_VMLA_by_scalar_A1_instance_)
  {}
};

// A(11:8)=0001
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       baseline: VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLA_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            size(21:20)=01) => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0Tester_Case1
    : public VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0TesterCase1 {
 public:
  VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0Tester_Case1()
    : VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0TesterCase1(
      state_.VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0_VMLA_by_scalar_A1_instance_)
  {}
};

// A(11:8)=0010
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0p10n1m0mmmm,
//       rule: VMLAL_by_scalar_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0Tester_Case2
    : public VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0TesterCase2 {
 public:
  VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0Tester_Case2()
    : VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0TesterCase2(
      state_.VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0_VMLAL_by_scalar_A2_instance_)
  {}
};

// A(11:8)=0011 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd0p11n1m0mmmm,
//       rule: VQDMLAL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0Tester_Case3
    : public VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0TesterCase3 {
 public:
  VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0Tester_Case3()
    : VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0TesterCase3(
      state_.VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0_VQDMLAL_A1_instance_)
  {}
};

// A(11:8)=0100
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLS_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1Tester_Case4
    : public VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1TesterCase4 {
 public:
  VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1Tester_Case4()
    : VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1TesterCase4(
      state_.VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1_VMLS_by_scalar_A1_instance_)
  {}
};

// A(11:8)=0101
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       baseline: VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLS_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            size(21:20)=01) => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0Tester_Case5
    : public VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0TesterCase5 {
 public:
  VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0Tester_Case5()
    : VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0TesterCase5(
      state_.VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0_VMLS_by_scalar_A1_instance_)
  {}
};

// A(11:8)=0110
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0p10n1m0mmmm,
//       rule: VMLSL_by_scalar_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0Tester_Case6
    : public VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0TesterCase6 {
 public:
  VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0Tester_Case6()
    : VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0TesterCase6(
      state_.VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0_VMLSL_by_scalar_A2_instance_)
  {}
};

// A(11:8)=0111 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd0p11n1m0mmmm,
//       rule: VQDMLSL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0Tester_Case7
    : public VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0TesterCase7 {
 public:
  VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0Tester_Case7()
    : VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0TesterCase7(
      state_.VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0_VQDMLSL_A1_instance_)
  {}
};

// A(11:8)=1000
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd100fn1m0mmmm,
//       rule: VMUL_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1Tester_Case8
    : public VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1TesterCase8 {
 public:
  VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1Tester_Case8()
    : VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1TesterCase8(
      state_.VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1_VMUL_by_scalar_A1_instance_)
  {}
};

// A(11:8)=1001
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       baseline: VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd100fn1m0mmmm,
//       rule: VMUL_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            size(21:20)=01) => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0Tester_Case9
    : public VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0TesterCase9 {
 public:
  VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0Tester_Case9()
    : VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0TesterCase9(
      state_.VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0_VMUL_by_scalar_A1_instance_)
  {}
};

// A(11:8)=1010
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd1010n1m0mmmm,
//       rule: VMULL_by_scalar_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0Tester_Case10
    : public VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0TesterCase10 {
 public:
  VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0Tester_Case10()
    : VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0TesterCase10(
      state_.VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0_VMULL_by_scalar_A2_instance_)
  {}
};

// A(11:8)=1011 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd1011n1m0mmmm,
//       rule: VQDMULL_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0Tester_Case11
    : public VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0TesterCase11 {
 public:
  VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0Tester_Case11()
    : VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0TesterCase11(
      state_.VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0_VQDMULL_A2_instance_)
  {}
};

// A(11:8)=1100
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd1100n1m0mmmm,
//       rule: VQDMULH_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0Tester_Case12
    : public VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0TesterCase12 {
 public:
  VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0Tester_Case12()
    : VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0TesterCase12(
      state_.VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0_VQDMULH_A2_instance_)
  {}
};

// A(11:8)=1101
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd1101n1m0mmmm,
//       rule: VQRDMULH,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0Tester_Case13
    : public VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0TesterCase13 {
 public:
  VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0Tester_Case13()
    : VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0TesterCase13(
      state_.VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0_VQRDMULH_instance_)
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
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLA_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1Tester_Case0_TestCase0) {
  VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1Tester_Case0 baseline_tester;
  NamedActual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2_VMLA_by_scalar_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001q1dssnnnndddd0p0fn1m0mmmm");
}

// A(11:8)=0001
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       baseline: VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLA_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            size(21:20)=01) => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0Tester_Case1_TestCase1) {
  VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1_VMLA_by_scalar_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001q1dssnnnndddd0p0fn1m0mmmm");
}

// A(11:8)=0010
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0p10n1m0mmmm,
//       rule: VMLAL_by_scalar_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0Tester_Case2_TestCase2) {
  VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1_VMLAL_by_scalar_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1dssnnnndddd0p10n1m0mmmm");
}

// A(11:8)=0011 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd0p11n1m0mmmm,
//       rule: VQDMLAL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0Tester_Case3_TestCase3) {
  VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1_VQDMLAL_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100101dssnnnndddd0p11n1m0mmmm");
}

// A(11:8)=0100
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLS_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1Tester_Case4_TestCase4) {
  VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1Tester_Case4 baseline_tester;
  NamedActual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2_VMLS_by_scalar_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001q1dssnnnndddd0p0fn1m0mmmm");
}

// A(11:8)=0101
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       baseline: VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//       rule: VMLS_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            size(21:20)=01) => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0Tester_Case5_TestCase5) {
  VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0Tester_Case5 baseline_tester;
  NamedActual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1_VMLS_by_scalar_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001q1dssnnnndddd0p0fn1m0mmmm");
}

// A(11:8)=0110
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0p10n1m0mmmm,
//       rule: VMLSL_by_scalar_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0Tester_Case6_TestCase6) {
  VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1_VMLSL_by_scalar_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1dssnnnndddd0p10n1m0mmmm");
}

// A(11:8)=0111 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd0p11n1m0mmmm,
//       rule: VQDMLSL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0Tester_Case7_TestCase7) {
  VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1_VQDMLSL_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100101dssnnnndddd0p11n1m0mmmm");
}

// A(11:8)=1000
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd100fn1m0mmmm,
//       rule: VMUL_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1Tester_Case8_TestCase8) {
  VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1Tester_Case8 baseline_tester;
  NamedActual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2_VMUL_by_scalar_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001q1dssnnnndddd100fn1m0mmmm");
}

// A(11:8)=1001
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//       baseline: VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd100fn1m0mmmm,
//       rule: VMUL_by_scalar_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            size(21:20)=01) => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0Tester_Case9_TestCase9) {
  VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0Tester_Case9 baseline_tester;
  NamedActual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1_VMUL_by_scalar_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001q1dssnnnndddd100fn1m0mmmm");
}

// A(11:8)=1010
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd1010n1m0mmmm,
//       rule: VMULL_by_scalar_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0Tester_Case10_TestCase10) {
  VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0Tester_Case10 baseline_tester;
  NamedActual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1_VMULL_by_scalar_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1dssnnnndddd1010n1m0mmmm");
}

// A(11:8)=1011 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1,
//       baseline: VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd1011n1m0mmmm,
//       rule: VQDMULL_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         (size(21:20)=00 ||
//            Vd(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0Tester_Case11_TestCase11) {
  VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0Tester_Case11 baseline_tester;
  NamedActual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1_VQDMULL_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100101dssnnnndddd1011n1m0mmmm");
}

// A(11:8)=1100
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd1100n1m0mmmm,
//       rule: VQDMULH_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0Tester_Case12_TestCase12) {
  VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0Tester_Case12 baseline_tester;
  NamedActual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2_VQDMULH_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001q1dssnnnndddd1100n1m0mmmm");
}

// A(11:8)=1101
//    = {Q: Q(24),
//       Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2,
//       baseline: VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0,
//       defs: {},
//       fields: [Q(24), size(21:20), Vn(19:16), Vd(15:12)],
//       pattern: 1111001q1dssnnnndddd1101n1m0mmmm,
//       rule: VQRDMULH,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 => UNDEFINED,
//         Q(24)=1 &&
//            (Vd(0)=1 ||
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0Tester_Case13_TestCase13) {
  VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0Tester_Case13 baseline_tester;
  NamedActual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2_VQRDMULH actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001q1dssnnnndddd1101n1m0mmmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
