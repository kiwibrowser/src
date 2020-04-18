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


// opc2(19:16)=0000 & opc3(7:6)=01
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110000dddd101s01m0mmmm,
//       rule: VMOV_register,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~0000
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;
  // opc3(7:6)=~01
  if ((inst.Bits() & 0x000000C0)  !=
          0x00000040) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc2(19:16)=0000 & opc3(7:6)=11
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VABS_cccc11101d110000dddd101s11m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110000dddd101s11m0mmmm,
//       rule: VABS,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VABS_cccc11101d110000dddd101s11m0mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  VABS_cccc11101d110000dddd101s11m0mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VABS_cccc11101d110000dddd101s11m0mmmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~0000
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;
  // opc3(7:6)=~11
  if ((inst.Bits() & 0x000000C0)  !=
          0x000000C0) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc2(19:16)=0001 & opc3(7:6)=01
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VNEG_cccc11101d110001dddd101s01m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110001dddd101s01m0mmmm,
//       rule: VNEG,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VNEG_cccc11101d110001dddd101s01m0mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  VNEG_cccc11101d110001dddd101s01m0mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VNEG_cccc11101d110001dddd101s01m0mmmm_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~0001
  if ((inst.Bits() & 0x000F0000)  !=
          0x00010000) return false;
  // opc3(7:6)=~01
  if ((inst.Bits() & 0x000000C0)  !=
          0x00000040) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc2(19:16)=0001 & opc3(7:6)=11
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110001dddd101s11m0mmmm,
//       rule: VSQRT,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~0001
  if ((inst.Bits() & 0x000F0000)  !=
          0x00010000) return false;
  // opc3(7:6)=~11
  if ((inst.Bits() & 0x000000C0)  !=
          0x000000C0) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc2(19:16)=0100 & opc3(7:6)=x1
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110100dddd101se1m0mmmm,
//       rule: VCMP_VCMPE,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~0100
  if ((inst.Bits() & 0x000F0000)  !=
          0x00040000) return false;
  // opc3(7:6)=~x1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc2(19:16)=0101 & opc3(7:6)=x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxxx0x0000
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0,
//       defs: {},
//       pattern: cccc11101d110101dddd101se1000000,
//       rule: VCMP_VCMPE,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~0101
  if ((inst.Bits() & 0x000F0000)  !=
          0x00050000) return false;
  // opc3(7:6)=~x1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxxxxxx0x0000
  if ((inst.Bits() & 0x0000002F)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc2(19:16)=0111 & opc3(7:6)=11
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110111dddd101s11m0mmmm,
//       rule: VCVT_between_double_precision_and_single_precision,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~0111
  if ((inst.Bits() & 0x000F0000)  !=
          0x00070000) return false;
  // opc3(7:6)=~11
  if ((inst.Bits() & 0x000000C0)  !=
          0x000000C0) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc2(19:16)=1000 & opc3(7:6)=x1
//    = {actual: Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1,
//       baseline: VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0,
//       defs: {},
//       fields: [opc2(18:16)],
//       opc2: opc2(18:16),
//       pattern: cccc11101d111ooodddd101sp1m0mmmm,
//       rule: VCVT_VCVTR_between_floating_point_and_integer_Floating_point,
//       safety: [opc2(18:16)=~000 &&
//            opc2(18:16)=~10x => DECODER_ERROR],
//       uses: {}}
class VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~1000
  if ((inst.Bits() & 0x000F0000)  !=
          0x00080000) return false;
  // opc3(7:6)=~x1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc2(19:16)=001x & opc3(7:6)=x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxx0xxxxxxxx
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d11001odddd1010t1m0mmmm,
//       rule: VCVTB_VCVTT,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~001x
  if ((inst.Bits() & 0x000E0000)  !=
          0x00020000) return false;
  // opc3(7:6)=~x1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxxx0xxxxxxxx
  if ((inst.Bits() & 0x00000100)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc2(19:16)=101x & opc3(7:6)=x1
//    = {actual: Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1,
//       baseline: VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0,
//       defs: {},
//       fields: [sx(7), i(5), imm4(3:0)],
//       frac_bits: size - imm4:i,
//       i: i(5),
//       imm4: imm4(3:0),
//       pattern: cccc11101d111o1udddd101fx1i0iiii,
//       rule: VCVT_between_floating_point_and_fixed_point_Floating_point,
//       safety: [frac_bits  <
//               0 => UNPREDICTABLE],
//       size: 16
//            if sx(7)=0
//            else 32,
//       sx: sx(7),
//       uses: {}}
class VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~101x
  if ((inst.Bits() & 0x000E0000)  !=
          0x000A0000) return false;
  // opc3(7:6)=~x1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc2(19:16)=110x & opc3(7:6)=x1
//    = {actual: Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1,
//       baseline: VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0,
//       defs: {},
//       fields: [opc2(18:16)],
//       opc2: opc2(18:16),
//       pattern: cccc11101d111ooodddd101sp1m0mmmm,
//       rule: VCVT_VCVTR_between_floating_point_and_integer_Floating_point,
//       safety: [opc2(18:16)=~000 &&
//            opc2(18:16)=~10x => DECODER_ERROR],
//       uses: {}}
class VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~110x
  if ((inst.Bits() & 0x000E0000)  !=
          0x000C0000) return false;
  // opc3(7:6)=~x1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc2(19:16)=111x & opc3(7:6)=x1
//    = {actual: Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1,
//       baseline: VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0,
//       defs: {},
//       fields: [sx(7), i(5), imm4(3:0)],
//       frac_bits: size - imm4:i,
//       i: i(5),
//       imm4: imm4(3:0),
//       pattern: cccc11101d111o1udddd101fx1i0iiii,
//       rule: VCVT_between_floating_point_and_fixed_point_Floating_point,
//       safety: [frac_bits  <
//               0 => UNPREDICTABLE],
//       size: 16
//            if sx(7)=0
//            else 32,
//       sx: sx(7),
//       uses: {}}
class VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc2(19:16)=~111x
  if ((inst.Bits() & 0x000E0000)  !=
          0x000E0000) return false;
  // opc3(7:6)=~x1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc3(7:6)=x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxx0x0xxxxx
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0,
//       defs: {},
//       pattern: cccc11101d11iiiidddd101s0000iiii,
//       rule: VMOV_immediate,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc3(7:6)=~x0
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxxxx0x0xxxxx
  if ((inst.Bits() & 0x000000A0)  !=
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

// opc2(19:16)=0000 & opc3(7:6)=01
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110000dddd101s01m0mmmm,
//       rule: VMOV_register,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0Tester_Case0
    : public VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0TesterCase0 {
 public:
  VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0Tester_Case0()
    : VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0TesterCase0(
      state_.VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0_VMOV_register_instance_)
  {}
};

// opc2(19:16)=0000 & opc3(7:6)=11
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VABS_cccc11101d110000dddd101s11m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110000dddd101s11m0mmmm,
//       rule: VABS,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VABS_cccc11101d110000dddd101s11m0mmmm_case_0Tester_Case1
    : public VABS_cccc11101d110000dddd101s11m0mmmm_case_0TesterCase1 {
 public:
  VABS_cccc11101d110000dddd101s11m0mmmm_case_0Tester_Case1()
    : VABS_cccc11101d110000dddd101s11m0mmmm_case_0TesterCase1(
      state_.VABS_cccc11101d110000dddd101s11m0mmmm_case_0_VABS_instance_)
  {}
};

// opc2(19:16)=0001 & opc3(7:6)=01
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VNEG_cccc11101d110001dddd101s01m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110001dddd101s01m0mmmm,
//       rule: VNEG,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VNEG_cccc11101d110001dddd101s01m0mmmm_case_0Tester_Case2
    : public VNEG_cccc11101d110001dddd101s01m0mmmm_case_0TesterCase2 {
 public:
  VNEG_cccc11101d110001dddd101s01m0mmmm_case_0Tester_Case2()
    : VNEG_cccc11101d110001dddd101s01m0mmmm_case_0TesterCase2(
      state_.VNEG_cccc11101d110001dddd101s01m0mmmm_case_0_VNEG_instance_)
  {}
};

// opc2(19:16)=0001 & opc3(7:6)=11
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110001dddd101s11m0mmmm,
//       rule: VSQRT,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0Tester_Case3
    : public VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0TesterCase3 {
 public:
  VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0Tester_Case3()
    : VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0TesterCase3(
      state_.VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0_VSQRT_instance_)
  {}
};

// opc2(19:16)=0100 & opc3(7:6)=x1
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110100dddd101se1m0mmmm,
//       rule: VCMP_VCMPE,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0Tester_Case4
    : public VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0TesterCase4 {
 public:
  VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0Tester_Case4()
    : VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0TesterCase4(
      state_.VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0_VCMP_VCMPE_instance_)
  {}
};

// opc2(19:16)=0101 & opc3(7:6)=x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxxx0x0000
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0,
//       defs: {},
//       pattern: cccc11101d110101dddd101se1000000,
//       rule: VCMP_VCMPE,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0Tester_Case5
    : public VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0TesterCase5 {
 public:
  VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0Tester_Case5()
    : VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0TesterCase5(
      state_.VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0_VCMP_VCMPE_instance_)
  {}
};

// opc2(19:16)=0111 & opc3(7:6)=11
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110111dddd101s11m0mmmm,
//       rule: VCVT_between_double_precision_and_single_precision,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0Tester_Case6
    : public VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0TesterCase6 {
 public:
  VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0Tester_Case6()
    : VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0TesterCase6(
      state_.VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0_VCVT_between_double_precision_and_single_precision_instance_)
  {}
};

// opc2(19:16)=1000 & opc3(7:6)=x1
//    = {actual: Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1,
//       baseline: VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0,
//       defs: {},
//       fields: [opc2(18:16)],
//       opc2: opc2(18:16),
//       pattern: cccc11101d111ooodddd101sp1m0mmmm,
//       rule: VCVT_VCVTR_between_floating_point_and_integer_Floating_point,
//       safety: [opc2(18:16)=~000 &&
//            opc2(18:16)=~10x => DECODER_ERROR],
//       uses: {}}
class VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0Tester_Case7
    : public VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0TesterCase7 {
 public:
  VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0Tester_Case7()
    : VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0TesterCase7(
      state_.VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_instance_)
  {}
};

// opc2(19:16)=001x & opc3(7:6)=x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxx0xxxxxxxx
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d11001odddd1010t1m0mmmm,
//       rule: VCVTB_VCVTT,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0Tester_Case8
    : public VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0TesterCase8 {
 public:
  VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0Tester_Case8()
    : VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0TesterCase8(
      state_.VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0_VCVTB_VCVTT_instance_)
  {}
};

// opc2(19:16)=101x & opc3(7:6)=x1
//    = {actual: Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1,
//       baseline: VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0,
//       defs: {},
//       fields: [sx(7), i(5), imm4(3:0)],
//       frac_bits: size - imm4:i,
//       i: i(5),
//       imm4: imm4(3:0),
//       pattern: cccc11101d111o1udddd101fx1i0iiii,
//       rule: VCVT_between_floating_point_and_fixed_point_Floating_point,
//       safety: [frac_bits  <
//               0 => UNPREDICTABLE],
//       size: 16
//            if sx(7)=0
//            else 32,
//       sx: sx(7),
//       uses: {}}
class VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0Tester_Case9
    : public VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0TesterCase9 {
 public:
  VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0Tester_Case9()
    : VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0TesterCase9(
      state_.VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0_VCVT_between_floating_point_and_fixed_point_Floating_point_instance_)
  {}
};

// opc2(19:16)=110x & opc3(7:6)=x1
//    = {actual: Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1,
//       baseline: VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0,
//       defs: {},
//       fields: [opc2(18:16)],
//       opc2: opc2(18:16),
//       pattern: cccc11101d111ooodddd101sp1m0mmmm,
//       rule: VCVT_VCVTR_between_floating_point_and_integer_Floating_point,
//       safety: [opc2(18:16)=~000 &&
//            opc2(18:16)=~10x => DECODER_ERROR],
//       uses: {}}
class VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0Tester_Case10
    : public VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0TesterCase10 {
 public:
  VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0Tester_Case10()
    : VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0TesterCase10(
      state_.VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_instance_)
  {}
};

// opc2(19:16)=111x & opc3(7:6)=x1
//    = {actual: Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1,
//       baseline: VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0,
//       defs: {},
//       fields: [sx(7), i(5), imm4(3:0)],
//       frac_bits: size - imm4:i,
//       i: i(5),
//       imm4: imm4(3:0),
//       pattern: cccc11101d111o1udddd101fx1i0iiii,
//       rule: VCVT_between_floating_point_and_fixed_point_Floating_point,
//       safety: [frac_bits  <
//               0 => UNPREDICTABLE],
//       size: 16
//            if sx(7)=0
//            else 32,
//       sx: sx(7),
//       uses: {}}
class VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0Tester_Case11
    : public VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0TesterCase11 {
 public:
  VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0Tester_Case11()
    : VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0TesterCase11(
      state_.VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0_VCVT_between_floating_point_and_fixed_point_Floating_point_instance_)
  {}
};

// opc3(7:6)=x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxx0x0xxxxx
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0,
//       defs: {},
//       pattern: cccc11101d11iiiidddd101s0000iiii,
//       rule: VMOV_immediate,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
class VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0Tester_Case12
    : public VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0TesterCase12 {
 public:
  VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0Tester_Case12()
    : VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0TesterCase12(
      state_.VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0_VMOV_immediate_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// opc2(19:16)=0000 & opc3(7:6)=01
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110000dddd101s01m0mmmm,
//       rule: VMOV_register,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0Tester_Case0_TestCase0) {
  VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_VMOV_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d110000dddd101s01m0mmmm");
}

// opc2(19:16)=0000 & opc3(7:6)=11
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VABS_cccc11101d110000dddd101s11m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110000dddd101s11m0mmmm,
//       rule: VABS,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VABS_cccc11101d110000dddd101s11m0mmmm_case_0Tester_Case1_TestCase1) {
  VABS_cccc11101d110000dddd101s11m0mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_VABS actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d110000dddd101s11m0mmmm");
}

// opc2(19:16)=0001 & opc3(7:6)=01
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VNEG_cccc11101d110001dddd101s01m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110001dddd101s01m0mmmm,
//       rule: VNEG,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VNEG_cccc11101d110001dddd101s01m0mmmm_case_0Tester_Case2_TestCase2) {
  VNEG_cccc11101d110001dddd101s01m0mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_VNEG actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d110001dddd101s01m0mmmm");
}

// opc2(19:16)=0001 & opc3(7:6)=11
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110001dddd101s11m0mmmm,
//       rule: VSQRT,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0Tester_Case3_TestCase3) {
  VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_VSQRT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d110001dddd101s11m0mmmm");
}

// opc2(19:16)=0100 & opc3(7:6)=x1
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110100dddd101se1m0mmmm,
//       rule: VCMP_VCMPE,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0Tester_Case4_TestCase4) {
  VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0Tester_Case4 baseline_tester;
  NamedActual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_VCMP_VCMPE actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d110100dddd101se1m0mmmm");
}

// opc2(19:16)=0101 & opc3(7:6)=x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxxx0x0000
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0,
//       defs: {},
//       pattern: cccc11101d110101dddd101se1000000,
//       rule: VCMP_VCMPE,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0Tester_Case5_TestCase5) {
  VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0Tester_Case5 baseline_tester;
  NamedActual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_VCMP_VCMPE actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d110101dddd101se1000000");
}

// opc2(19:16)=0111 & opc3(7:6)=11
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d110111dddd101s11m0mmmm,
//       rule: VCVT_between_double_precision_and_single_precision,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0Tester_Case6_TestCase6) {
  VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_VCVT_between_double_precision_and_single_precision actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d110111dddd101s11m0mmmm");
}

// opc2(19:16)=1000 & opc3(7:6)=x1
//    = {actual: Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1,
//       baseline: VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0,
//       defs: {},
//       fields: [opc2(18:16)],
//       opc2: opc2(18:16),
//       pattern: cccc11101d111ooodddd101sp1m0mmmm,
//       rule: VCVT_VCVTR_between_floating_point_and_integer_Floating_point,
//       safety: [opc2(18:16)=~000 &&
//            opc2(18:16)=~10x => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0Tester_Case7_TestCase7) {
  VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1_VCVT_VCVTR_between_floating_point_and_integer_Floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d111ooodddd101sp1m0mmmm");
}

// opc2(19:16)=001x & opc3(7:6)=x1 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxx0xxxxxxxx
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0,
//       defs: {},
//       pattern: cccc11101d11001odddd1010t1m0mmmm,
//       rule: VCVTB_VCVTT,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0Tester_Case8_TestCase8) {
  VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0Tester_Case8 baseline_tester;
  NamedActual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_VCVTB_VCVTT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d11001odddd1010t1m0mmmm");
}

// opc2(19:16)=101x & opc3(7:6)=x1
//    = {actual: Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1,
//       baseline: VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0,
//       defs: {},
//       fields: [sx(7), i(5), imm4(3:0)],
//       frac_bits: size - imm4:i,
//       i: i(5),
//       imm4: imm4(3:0),
//       pattern: cccc11101d111o1udddd101fx1i0iiii,
//       rule: VCVT_between_floating_point_and_fixed_point_Floating_point,
//       safety: [frac_bits  <
//               0 => UNPREDICTABLE],
//       size: 16
//            if sx(7)=0
//            else 32,
//       sx: sx(7),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0Tester_Case9_TestCase9) {
  VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0Tester_Case9 baseline_tester;
  NamedActual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1_VCVT_between_floating_point_and_fixed_point_Floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d111o1udddd101fx1i0iiii");
}

// opc2(19:16)=110x & opc3(7:6)=x1
//    = {actual: Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1,
//       baseline: VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0,
//       defs: {},
//       fields: [opc2(18:16)],
//       opc2: opc2(18:16),
//       pattern: cccc11101d111ooodddd101sp1m0mmmm,
//       rule: VCVT_VCVTR_between_floating_point_and_integer_Floating_point,
//       safety: [opc2(18:16)=~000 &&
//            opc2(18:16)=~10x => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0Tester_Case10_TestCase10) {
  VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0Tester_Case10 baseline_tester;
  NamedActual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1_VCVT_VCVTR_between_floating_point_and_integer_Floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d111ooodddd101sp1m0mmmm");
}

// opc2(19:16)=111x & opc3(7:6)=x1
//    = {actual: Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1,
//       baseline: VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0,
//       defs: {},
//       fields: [sx(7), i(5), imm4(3:0)],
//       frac_bits: size - imm4:i,
//       i: i(5),
//       imm4: imm4(3:0),
//       pattern: cccc11101d111o1udddd101fx1i0iiii,
//       rule: VCVT_between_floating_point_and_fixed_point_Floating_point,
//       safety: [frac_bits  <
//               0 => UNPREDICTABLE],
//       size: 16
//            if sx(7)=0
//            else 32,
//       sx: sx(7),
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0Tester_Case11_TestCase11) {
  VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0Tester_Case11 baseline_tester;
  NamedActual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1_VCVT_between_floating_point_and_fixed_point_Floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d111o1udddd101fx1i0iiii");
}

// opc3(7:6)=x0 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxx0x0xxxxx
//    = {actual: Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1,
//       baseline: VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0,
//       defs: {},
//       pattern: cccc11101d11iiiidddd101s0000iiii,
//       rule: VMOV_immediate,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0Tester_Case12_TestCase12) {
  VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0Tester_Case12 baseline_tester;
  NamedActual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_VMOV_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d11iiiidddd101s0000iiii");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
