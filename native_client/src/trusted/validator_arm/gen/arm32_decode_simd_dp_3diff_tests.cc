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


// A(11:8)=0100 & U(24)=0
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100101dssnnnndddd0100n0m0mmmm,
//       rule: VADDHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0100
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000400) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0100 & U(24)=1
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100111dssnnnndddd0100n0m0mmmm,
//       rule: VRADDHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0TesterCase1
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

// A(11:8)=0101
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0101n0m0mmmm,
//       rule: VABAL_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0TesterCase2
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

// A(11:8)=0110 & U(24)=0
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100101dssnnnndddd0110n0m0mmmm,
//       rule: VSUBHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000600) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0110 & U(24)=1
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100111dssnnnndddd0110n0m0mmmm,
//       rule: VRSUBHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000600) return false;
  // U(24)=~1
  if ((inst.Bits() & 0x01000000)  !=
          0x01000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=0111
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0111n0m0mmmm,
//       rule: VABDL_integer_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~0111
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000700) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1100
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd11p0n0m0mmmm,
//       rule: VMULL_integer_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0TesterCase6
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

// A(11:8)=1101 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1,
//       baseline: VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd1101n0m0mmmm,
//       rule: VQDMULL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 ||
//            Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000D00) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=1110
//    = {U: U(24),
//       Vd: Vd(15:12),
//       actual: Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1,
//       baseline: VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0,
//       defs: {},
//       fields: [U(24), size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd11p0n0m0mmmm,
//       rule: VMULL_polynomial_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         U(24)=1 ||
//            size(21:20)=~00 => UNDEFINED,
//         Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~1110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000E00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=10x0
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd10p0n0m0mmmm,
//       rule: VMLAL_VMLSL_integer_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~10x0
  if ((inst.Bits() & 0x00000D00)  !=
          0x00000800) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=10x1 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1,
//       baseline: VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd10p1n0m0mmmm,
//       rule: VQDMLAL_VQDMLSL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 ||
//            Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~10x1
  if ((inst.Bits() & 0x00000D00)  !=
          0x00000900) return false;
  // U(24)=~0
  if ((inst.Bits() & 0x01000000)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=000x
//    = {Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1,
//       baseline: VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), op(8)],
//       op: op(8),
//       pattern: 1111001u1dssnnnndddd000pn0m0mmmm,
//       rule: VADDL_VADDW,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vd(0)=1 ||
//            (op(8)=1 &&
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~000x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// A(11:8)=001x
//    = {Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1,
//       baseline: VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), op(8)],
//       op: op(8),
//       pattern: 1111001u1dssnnnndddd001pn0m0mmmm,
//       rule: VSUBL_VSUBW,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vd(0)=1 ||
//            (op(8)=1 &&
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // A(11:8)=~001x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000200) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// A(11:8)=0100 & U(24)=0
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100101dssnnnndddd0100n0m0mmmm,
//       rule: VADDHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0Tester_Case0
    : public VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0TesterCase0 {
 public:
  VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0Tester_Case0()
    : VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0TesterCase0(
      state_.VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0_VADDHN_instance_)
  {}
};

// A(11:8)=0100 & U(24)=1
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100111dssnnnndddd0100n0m0mmmm,
//       rule: VRADDHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0Tester_Case1
    : public VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0TesterCase1 {
 public:
  VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0Tester_Case1()
    : VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0TesterCase1(
      state_.VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0_VRADDHN_instance_)
  {}
};

// A(11:8)=0101
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0101n0m0mmmm,
//       rule: VABAL_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0Tester_Case2
    : public VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0TesterCase2 {
 public:
  VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0Tester_Case2()
    : VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0TesterCase2(
      state_.VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0_VABAL_A2_instance_)
  {}
};

// A(11:8)=0110 & U(24)=0
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100101dssnnnndddd0110n0m0mmmm,
//       rule: VSUBHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0Tester_Case3
    : public VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0TesterCase3 {
 public:
  VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0Tester_Case3()
    : VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0TesterCase3(
      state_.VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0_VSUBHN_instance_)
  {}
};

// A(11:8)=0110 & U(24)=1
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100111dssnnnndddd0110n0m0mmmm,
//       rule: VRSUBHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0Tester_Case4
    : public VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0TesterCase4 {
 public:
  VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0Tester_Case4()
    : VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0TesterCase4(
      state_.VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0_VRSUBHN_instance_)
  {}
};

// A(11:8)=0111
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0111n0m0mmmm,
//       rule: VABDL_integer_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0Tester_Case5
    : public VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0TesterCase5 {
 public:
  VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0Tester_Case5()
    : VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0TesterCase5(
      state_.VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0_VABDL_integer_A2_instance_)
  {}
};

// A(11:8)=1100
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd11p0n0m0mmmm,
//       rule: VMULL_integer_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0Tester_Case6
    : public VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0TesterCase6 {
 public:
  VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0Tester_Case6()
    : VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0TesterCase6(
      state_.VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0_VMULL_integer_A2_instance_)
  {}
};

// A(11:8)=1101 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1,
//       baseline: VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd1101n0m0mmmm,
//       rule: VQDMULL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 ||
//            Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0Tester_Case7
    : public VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0TesterCase7 {
 public:
  VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0Tester_Case7()
    : VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0TesterCase7(
      state_.VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0_VQDMULL_A1_instance_)
  {}
};

// A(11:8)=1110
//    = {U: U(24),
//       Vd: Vd(15:12),
//       actual: Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1,
//       baseline: VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0,
//       defs: {},
//       fields: [U(24), size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd11p0n0m0mmmm,
//       rule: VMULL_polynomial_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         U(24)=1 ||
//            size(21:20)=~00 => UNDEFINED,
//         Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0Tester_Case8
    : public VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0TesterCase8 {
 public:
  VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0Tester_Case8()
    : VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0TesterCase8(
      state_.VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0_VMULL_polynomial_A2_instance_)
  {}
};

// A(11:8)=10x0
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd10p0n0m0mmmm,
//       rule: VMLAL_VMLSL_integer_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0Tester_Case9
    : public VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0TesterCase9 {
 public:
  VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0Tester_Case9()
    : VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0TesterCase9(
      state_.VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0_VMLAL_VMLSL_integer_A2_instance_)
  {}
};

// A(11:8)=10x1 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1,
//       baseline: VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd10p1n0m0mmmm,
//       rule: VQDMLAL_VQDMLSL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 ||
//            Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0Tester_Case10
    : public VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0TesterCase10 {
 public:
  VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0Tester_Case10()
    : VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0TesterCase10(
      state_.VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0_VQDMLAL_VQDMLSL_A1_instance_)
  {}
};

// A(11:8)=000x
//    = {Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1,
//       baseline: VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), op(8)],
//       op: op(8),
//       pattern: 1111001u1dssnnnndddd000pn0m0mmmm,
//       rule: VADDL_VADDW,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vd(0)=1 ||
//            (op(8)=1 &&
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0Tester_Case11
    : public VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0TesterCase11 {
 public:
  VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0Tester_Case11()
    : VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0TesterCase11(
      state_.VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0_VADDL_VADDW_instance_)
  {}
};

// A(11:8)=001x
//    = {Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1,
//       baseline: VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), op(8)],
//       op: op(8),
//       pattern: 1111001u1dssnnnndddd001pn0m0mmmm,
//       rule: VSUBL_VSUBW,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vd(0)=1 ||
//            (op(8)=1 &&
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
class VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0Tester_Case12
    : public VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0TesterCase12 {
 public:
  VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0Tester_Case12()
    : VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0TesterCase12(
      state_.VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0_VSUBL_VSUBW_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// A(11:8)=0100 & U(24)=0
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100101dssnnnndddd0100n0m0mmmm,
//       rule: VADDHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0Tester_Case0_TestCase0) {
  VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1_VADDHN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100101dssnnnndddd0100n0m0mmmm");
}

// A(11:8)=0100 & U(24)=1
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100111dssnnnndddd0100n0m0mmmm,
//       rule: VRADDHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0Tester_Case1_TestCase1) {
  VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1_VRADDHN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111dssnnnndddd0100n0m0mmmm");
}

// A(11:8)=0101
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0101n0m0mmmm,
//       rule: VABAL_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0Tester_Case2_TestCase2) {
  VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1_VABAL_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1dssnnnndddd0101n0m0mmmm");
}

// A(11:8)=0110 & U(24)=0
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100101dssnnnndddd0110n0m0mmmm,
//       rule: VSUBHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0Tester_Case3_TestCase3) {
  VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1_VSUBHN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100101dssnnnndddd0110n0m0mmmm");
}

// A(11:8)=0110 & U(24)=1
//    = {Vm: Vm(3:0),
//       Vn: Vn(19:16),
//       actual: Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1,
//       baseline: VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vm(3:0)],
//       pattern: 111100111dssnnnndddd0110n0m0mmmm,
//       rule: VRSUBHN,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vn(0)=1 ||
//            Vm(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0Tester_Case4_TestCase4) {
  VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0Tester_Case4 baseline_tester;
  NamedActual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1_VRSUBHN actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100111dssnnnndddd0110n0m0mmmm");
}

// A(11:8)=0111
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd0111n0m0mmmm,
//       rule: VABDL_integer_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0Tester_Case5_TestCase5) {
  VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0Tester_Case5 baseline_tester;
  NamedActual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1_VABDL_integer_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1dssnnnndddd0111n0m0mmmm");
}

// A(11:8)=1100
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd11p0n0m0mmmm,
//       rule: VMULL_integer_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0Tester_Case6_TestCase6) {
  VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1_VMULL_integer_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1dssnnnndddd11p0n0m0mmmm");
}

// A(11:8)=1101 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1,
//       baseline: VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd1101n0m0mmmm,
//       rule: VQDMULL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 ||
//            Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0Tester_Case7_TestCase7) {
  VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1_VQDMULL_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100101dssnnnndddd1101n0m0mmmm");
}

// A(11:8)=1110
//    = {U: U(24),
//       Vd: Vd(15:12),
//       actual: Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1,
//       baseline: VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0,
//       defs: {},
//       fields: [U(24), size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd11p0n0m0mmmm,
//       rule: VMULL_polynomial_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         U(24)=1 ||
//            size(21:20)=~00 => UNDEFINED,
//         Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0Tester_Case8_TestCase8) {
  VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0Tester_Case8 baseline_tester;
  NamedActual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1_VMULL_polynomial_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1dssnnnndddd11p0n0m0mmmm");
}

// A(11:8)=10x0
//    = {Vd: Vd(15:12),
//       actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//       baseline: VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 1111001u1dssnnnndddd10p0n0m0mmmm,
//       rule: VMLAL_VMLSL_integer_A2,
//       safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0Tester_Case9_TestCase9) {
  VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0Tester_Case9 baseline_tester;
  NamedActual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1_VMLAL_VMLSL_integer_A2 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1dssnnnndddd10p0n0m0mmmm");
}

// A(11:8)=10x1 & U(24)=0
//    = {Vd: Vd(15:12),
//       actual: Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1,
//       baseline: VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vd(15:12)],
//       pattern: 111100101dssnnnndddd10p1n0m0mmmm,
//       rule: VQDMLAL_VQDMLSL_A1,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         size(21:20)=00 ||
//            Vd(0)=1 => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0Tester_Case10_TestCase10) {
  VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0Tester_Case10 baseline_tester;
  NamedActual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1_VQDMLAL_VQDMLSL_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100101dssnnnndddd10p1n0m0mmmm");
}

// A(11:8)=000x
//    = {Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1,
//       baseline: VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), op(8)],
//       op: op(8),
//       pattern: 1111001u1dssnnnndddd000pn0m0mmmm,
//       rule: VADDL_VADDW,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vd(0)=1 ||
//            (op(8)=1 &&
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0Tester_Case11_TestCase11) {
  VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0Tester_Case11 baseline_tester;
  NamedActual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1_VADDL_VADDW actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1dssnnnndddd000pn0m0mmmm");
}

// A(11:8)=001x
//    = {Vd: Vd(15:12),
//       Vn: Vn(19:16),
//       actual: Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1,
//       baseline: VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0,
//       defs: {},
//       fields: [size(21:20), Vn(19:16), Vd(15:12), op(8)],
//       op: op(8),
//       pattern: 1111001u1dssnnnndddd001pn0m0mmmm,
//       rule: VSUBL_VSUBW,
//       safety: [size(21:20)=11 => DECODER_ERROR,
//         Vd(0)=1 ||
//            (op(8)=1 &&
//            Vn(0)=1) => UNDEFINED],
//       size: size(21:20),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0Tester_Case12_TestCase12) {
  VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0Tester_Case12 baseline_tester;
  NamedActual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1_VSUBL_VSUBW actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111001u1dssnnnndddd001pn0m0mmmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
