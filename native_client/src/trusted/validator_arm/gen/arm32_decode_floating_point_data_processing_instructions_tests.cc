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


// opc1(23:20)=0x00
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d00nnnndddd101snom0mmmm,
//       rule: VMLA_VMLS_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc1(23:20)=~0x00
  if ((inst.Bits() & 0x00B00000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc1(23:20)=0x01
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d01nnnndddd101snom0mmmm,
//       rule: VNMLA_VNMLS,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc1(23:20)=~0x01
  if ((inst.Bits() & 0x00B00000)  !=
          0x00100000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc1(23:20)=0x10 & opc3(7:6)=x0
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d10nnnndddd101sn0m0mmmm,
//       rule: VMUL_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc1(23:20)=~0x10
  if ((inst.Bits() & 0x00B00000)  !=
          0x00200000) return false;
  // opc3(7:6)=~x0
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc1(23:20)=0x10 & opc3(7:6)=x1
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d10nnnndddd101sn1m0mmmm,
//       rule: VNMUL,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc1(23:20)=~0x10
  if ((inst.Bits() & 0x00B00000)  !=
          0x00200000) return false;
  // opc3(7:6)=~x1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc1(23:20)=0x11 & opc3(7:6)=x0
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d11nnnndddd101sn0m0mmmm,
//       rule: VADD_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc1(23:20)=~0x11
  if ((inst.Bits() & 0x00B00000)  !=
          0x00300000) return false;
  // opc3(7:6)=~x0
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc1(23:20)=0x11 & opc3(7:6)=x1
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d11nnnndddd101sn1m0mmmm,
//       rule: VSUB_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc1(23:20)=~0x11
  if ((inst.Bits() & 0x00B00000)  !=
          0x00300000) return false;
  // opc3(7:6)=~x1
  if ((inst.Bits() & 0x00000040)  !=
          0x00000040) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc1(23:20)=1x00 & opc3(7:6)=x0
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11101d00nnnndddd101sn0m0mmmm,
//       rule: VDIV,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc1(23:20)=~1x00
  if ((inst.Bits() & 0x00B00000)  !=
          0x00800000) return false;
  // opc3(7:6)=~x0
  if ((inst.Bits() & 0x00000040)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc1(23:20)=1x01
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11101d01nnnndddd101snom0mmmm,
//       rule: VFNMA_VFNMS,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc1(23:20)=~1x01
  if ((inst.Bits() & 0x00B00000)  !=
          0x00900000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// opc1(23:20)=1x10
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11101d10nnnndddd101snom0mmmm,
//       rule: VFMA_VFMS,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // opc1(23:20)=~1x10
  if ((inst.Bits() & 0x00B00000)  !=
          0x00A00000) return false;

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

// opc1(23:20)=0x00
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d00nnnndddd101snom0mmmm,
//       rule: VMLA_VMLS_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0Tester_Case0
    : public VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0TesterCase0 {
 public:
  VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0Tester_Case0()
    : VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0TesterCase0(
      state_.VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0_VMLA_VMLS_floating_point_instance_)
  {}
};

// opc1(23:20)=0x01
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d01nnnndddd101snom0mmmm,
//       rule: VNMLA_VNMLS,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0Tester_Case1
    : public VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0TesterCase1 {
 public:
  VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0Tester_Case1()
    : VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0TesterCase1(
      state_.VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0_VNMLA_VNMLS_instance_)
  {}
};

// opc1(23:20)=0x10 & opc3(7:6)=x0
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d10nnnndddd101sn0m0mmmm,
//       rule: VMUL_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0Tester_Case2
    : public VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0TesterCase2 {
 public:
  VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0Tester_Case2()
    : VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0TesterCase2(
      state_.VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0_VMUL_floating_point_instance_)
  {}
};

// opc1(23:20)=0x10 & opc3(7:6)=x1
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d10nnnndddd101sn1m0mmmm,
//       rule: VNMUL,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0Tester_Case3
    : public VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0TesterCase3 {
 public:
  VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0Tester_Case3()
    : VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0TesterCase3(
      state_.VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0_VNMUL_instance_)
  {}
};

// opc1(23:20)=0x11 & opc3(7:6)=x0
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d11nnnndddd101sn0m0mmmm,
//       rule: VADD_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0Tester_Case4
    : public VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0TesterCase4 {
 public:
  VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0Tester_Case4()
    : VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0TesterCase4(
      state_.VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0_VADD_floating_point_instance_)
  {}
};

// opc1(23:20)=0x11 & opc3(7:6)=x1
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d11nnnndddd101sn1m0mmmm,
//       rule: VSUB_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0Tester_Case5
    : public VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0TesterCase5 {
 public:
  VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0Tester_Case5()
    : VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0TesterCase5(
      state_.VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0_VSUB_floating_point_instance_)
  {}
};

// opc1(23:20)=1x00 & opc3(7:6)=x0
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11101d00nnnndddd101sn0m0mmmm,
//       rule: VDIV,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0Tester_Case6
    : public VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0TesterCase6 {
 public:
  VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0Tester_Case6()
    : VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0TesterCase6(
      state_.VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0_VDIV_instance_)
  {}
};

// opc1(23:20)=1x01
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11101d01nnnndddd101snom0mmmm,
//       rule: VFNMA_VFNMS,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0Tester_Case7
    : public VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0TesterCase7 {
 public:
  VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0Tester_Case7()
    : VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0TesterCase7(
      state_.VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0_VFNMA_VFNMS_instance_)
  {}
};

// opc1(23:20)=1x10
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11101d10nnnndddd101snom0mmmm,
//       rule: VFMA_VFMS,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
class VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0Tester_Case8
    : public VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0TesterCase8 {
 public:
  VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0Tester_Case8()
    : VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0TesterCase8(
      state_.VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0_VFMA_VFMS_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// opc1(23:20)=0x00
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d00nnnndddd101snom0mmmm,
//       rule: VMLA_VMLS_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0Tester_Case0_TestCase0) {
  VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_VMLA_VMLS_floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11100d00nnnndddd101snom0mmmm");
}

// opc1(23:20)=0x01
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d01nnnndddd101snom0mmmm,
//       rule: VNMLA_VNMLS,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0Tester_Case1_TestCase1) {
  VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_VNMLA_VNMLS actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11100d01nnnndddd101snom0mmmm");
}

// opc1(23:20)=0x10 & opc3(7:6)=x0
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d10nnnndddd101sn0m0mmmm,
//       rule: VMUL_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0Tester_Case2_TestCase2) {
  VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_VMUL_floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11100d10nnnndddd101sn0m0mmmm");
}

// opc1(23:20)=0x10 & opc3(7:6)=x1
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d10nnnndddd101sn1m0mmmm,
//       rule: VNMUL,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0Tester_Case3_TestCase3) {
  VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_VNMUL actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11100d10nnnndddd101sn1m0mmmm");
}

// opc1(23:20)=0x11 & opc3(7:6)=x0
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d11nnnndddd101sn0m0mmmm,
//       rule: VADD_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0Tester_Case4_TestCase4) {
  VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0Tester_Case4 baseline_tester;
  NamedActual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_VADD_floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11100d11nnnndddd101sn0m0mmmm");
}

// opc1(23:20)=0x11 & opc3(7:6)=x1
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11100d11nnnndddd101sn1m0mmmm,
//       rule: VSUB_floating_point,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0Tester_Case5_TestCase5) {
  VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0Tester_Case5 baseline_tester;
  NamedActual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_VSUB_floating_point actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11100d11nnnndddd101sn1m0mmmm");
}

// opc1(23:20)=1x00 & opc3(7:6)=x0
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11101d00nnnndddd101sn0m0mmmm,
//       rule: VDIV,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0Tester_Case6_TestCase6) {
  VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_VDIV actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d00nnnndddd101sn0m0mmmm");
}

// opc1(23:20)=1x01
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11101d01nnnndddd101snom0mmmm,
//       rule: VFNMA_VFNMS,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0Tester_Case7_TestCase7) {
  VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_VFNMA_VFNMS actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d01nnnndddd101snom0mmmm");
}

// opc1(23:20)=1x10
//    = {actual: Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1,
//       baseline: VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0,
//       cond: cond(31:28),
//       defs: {},
//       fields: [cond(31:28)],
//       pattern: cccc11101d10nnnndddd101snom0mmmm,
//       rule: VFMA_VFMS,
//       safety: [cond(31:28)=1111 => DECODER_ERROR],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0Tester_Case8_TestCase8) {
  VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0Tester_Case8 baseline_tester;
  NamedActual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_VFMA_VFMS actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc11101d10nnnndddd101snom0mmmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
