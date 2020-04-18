/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#include "native_client/src/trusted/validator_arm/gen/arm32_decode_actuals.h"
#include "native_client/src/trusted/validator_arm/inst_classes_inline.h"

namespace nacl_arm_dec {

// Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)(0)=1 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x00300000) >> 20) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=11 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=11 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  ==
          0x000C0000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=~10 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(19:18)=~10 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00080000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => MAY_BE_SAFE],
//    uses: {}}

RegisterList Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:16)(0)=1 ||
//         inst(3:0)(0)=1 => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1 => UNDEFINED
  if (((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(15:12)(0)=1 ||
//         (inst(8)=1 &&
//         inst(19:16)(0)=1) => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // inst(15:12)(0)=1 ||
  //       (inst(8)=1 &&
  //       inst(19:16)(0)=1) => UNDEFINED
  if (((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x00000100)  ==
          0x00000100) &&
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(31:28)=1111 => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(31:28)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0xF0000000)  ==
          0xF0000000)
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(11:8)(0)=0 ||
//         inst(11:8)(3:2)=11 => DECODER_ERROR,
//      inst(6)=1 &&
//         inst(15:12)(0)=1 => UNDEFINED],
//    uses: {}}

RegisterList Actual_VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(11:8)(0)=0 ||
  //       inst(11:8)(3:2)=11 => DECODER_ERROR
  if (((((inst.Bits() & 0x00000F00) >> 8) & 0x00000001)  ==
          0x00000000) ||
       ((((inst.Bits() & 0x00000F00) >> 8) & 0x0000000C)  ==
          0x0000000C))
    return DECODER_ERROR;

  // inst(6)=1 &&
  //       inst(15:12)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=~00 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=~00 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00000000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(18:16)=~000 &&
//         inst(18:16)=~10x => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(18:16)=~000 &&
  //       inst(18:16)=~10x => DECODER_ERROR
  if (((inst.Bits() & 0x00070000)  !=
          0x00000000) &&
       ((inst.Bits() & 0x00060000)  !=
          0x00040000))
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:16)=000xxx => DECODER_ERROR,
//      inst(21:16)=0xxxxx => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:16)=000xxx => DECODER_ERROR
  if ((inst.Bits() & 0x00380000)  ==
          0x00000000)
    return DECODER_ERROR;

  // inst(21:16)=0xxxxx => UNDEFINED
  if ((inst.Bits() & 0x00200000)  ==
          0x00000000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1
//
// Actual:
//   {defs: {},
//    safety: [16
//         if inst(7)=0
//         else 32 - inst(3:0):inst(5)  <
//            0 => UNPREDICTABLE],
//    uses: {}}

RegisterList Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 16
  //       if inst(7)=0
  //       else 32 - inst(3:0):inst(5)  <
  //          0 => UNPREDICTABLE
  if (((static_cast<int32_t>(((inst.Bits() & 0x00000080)  ==
          0x00000000
       ? 16
       : 32)) - static_cast<int32_t>(((((inst.Bits() & 0x0000000F)) << 1) | ((inst.Bits() & 0x00000020) >> 5)))) < (0)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VDUP_ARM_core_register_cccc11101bq0ddddtttt1011d0e10000_case_1
//
// Actual:
//   {defs: {},
//    safety: [14  !=
//            inst(31:28) => DEPRECATED,
//      15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(21)=1 &&
//         inst(19:16)(0)=1 => UNDEFINED,
//      inst(22):inst(5)(1:0)=11 => UNDEFINED],
//    uses: {inst(15:12)}}

RegisterList Actual_VDUP_ARM_core_register_cccc11101bq0ddddtttt1011d0e10000_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VDUP_ARM_core_register_cccc11101bq0ddddtttt1011d0e10000_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 14  !=
  //          inst(31:28) => DEPRECATED
  if (((((inst.Bits() & 0xF0000000) >> 28)) != (14)))
    return DEPRECATED;

  // inst(21)=1 &&
  //       inst(19:16)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  // inst(22):inst(5)(1:0)=11 => UNDEFINED
  if (((((((inst.Bits() & 0x00400000) >> 22)) << 1) | ((inst.Bits() & 0x00000020) >> 5)) & 0x00000003)  ==
          0x00000003)
    return UNDEFINED;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_VDUP_ARM_core_register_cccc11101bq0ddddtttt1011d0e10000_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// Actual_VDUP_scalar_111100111d11iiiidddd11000qm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:16)=x000 => UNDEFINED,
//      inst(6)=1 &&
//         inst(15:12)(0)=1 => UNDEFINED],
//    uses: {}}

RegisterList Actual_VDUP_scalar_111100111d11iiiidddd11000qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VDUP_scalar_111100111d11iiiidddd11000qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=x000 => UNDEFINED
  if ((inst.Bits() & 0x00070000)  ==
          0x00000000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       inst(15:12)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VDUP_scalar_111100111d11iiiidddd11000qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VEXT_111100101d11nnnnddddiiiinqm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(6)=0 &&
//         inst(11:8)(3)=1 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VEXT_111100101d11nnnnddddiiiinqm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VEXT_111100101d11nnnnddddiiiinqm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(6)=0 &&
  //       inst(11:8)(3)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000000) &&
       ((((inst.Bits() & 0x00000F00) >> 8) & 0x00000008)  ==
          0x00000008))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VEXT_111100101d11nnnnddddiiiinqm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         32  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:8)=0111
//         else 2
//         if inst(11:8)=1010
//         else 3
//         if inst(11:8)=0110
//         else 4
//         if inst(11:8)=0010
//         else 0 => UNPREDICTABLE,
//      inst(11:8)=0110 &&
//         inst(5:4)(1)=1 => UNDEFINED,
//      inst(11:8)=0111 &&
//         inst(5:4)(1)=1 => UNDEFINED,
//      inst(11:8)=1010 &&
//         inst(5:4)=11 => UNDEFINED,
//      not inst(11:8)=0111 ||
//         inst(11:8)=1010 ||
//         inst(11:8)=0110 ||
//         inst(11:8)=0010 => DECODER_ERROR],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(11:8)=0111 &&
  //       inst(5:4)(1)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000F00)  ==
          0x00000700) &&
       ((((inst.Bits() & 0x00000030) >> 4) & 0x00000002)  ==
          0x00000002))
    return UNDEFINED;

  // inst(11:8)=1010 &&
  //       inst(5:4)=11 => UNDEFINED
  if (((inst.Bits() & 0x00000F00)  ==
          0x00000A00) &&
       ((inst.Bits() & 0x00000030)  ==
          0x00000030))
    return UNDEFINED;

  // inst(11:8)=0110 &&
  //       inst(5:4)(1)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000F00)  ==
          0x00000600) &&
       ((((inst.Bits() & 0x00000030) >> 4) & 0x00000002)  ==
          0x00000002))
    return UNDEFINED;

  // not inst(11:8)=0111 ||
  //       inst(11:8)=1010 ||
  //       inst(11:8)=0110 ||
  //       inst(11:8)=0010 => DECODER_ERROR
  if (!(((inst.Bits() & 0x00000F00)  ==
          0x00000700) ||
       ((inst.Bits() & 0x00000F00)  ==
          0x00000A00) ||
       ((inst.Bits() & 0x00000F00)  ==
          0x00000600) ||
       ((inst.Bits() & 0x00000F00)  ==
          0x00000200)))
    return DECODER_ERROR;

  // 15  ==
  //          inst(19:16) ||
  //       32  <=
  //          inst(22):inst(15:12) + 1
  //       if inst(11:8)=0111
  //       else 2
  //       if inst(11:8)=1010
  //       else 3
  //       if inst(11:8)=0110
  //       else 4
  //       if inst(11:8)=0010
  //       else 0 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + ((inst.Bits() & 0x00000F00)  ==
          0x00000700
       ? 1
       : ((inst.Bits() & 0x00000F00)  ==
          0x00000A00
       ? 2
       : ((inst.Bits() & 0x00000F00)  ==
          0x00000600
       ? 3
       : ((inst.Bits() & 0x00000F00)  ==
          0x00000200
       ? 4
       : 0))))) > (32))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         32  <=
//            inst(22):inst(15:12) + 1
//         if inst(5)=0
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 ||
//         (inst(7:6)=00 &&
//         inst(4)=1) => UNDEFINED],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(7:6)=11 ||
  //       (inst(7:6)=00 &&
  //       inst(4)=1) => UNDEFINED
  if (((inst.Bits() & 0x000000C0)  ==
          0x000000C0) ||
       ((((inst.Bits() & 0x000000C0)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00000010)  ==
          0x00000010))))
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) ||
  //       32  <=
  //          inst(22):inst(15:12) + 1
  //       if inst(5)=0
  //       else 2 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + ((inst.Bits() & 0x00000020)  ==
          0x00000000
       ? 1
       : 2)) > (32))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) => UNPREDICTABLE,
//      inst(11:10)=00 &&
//         inst(7:4)(0)=~0 => UNDEFINED,
//      inst(11:10)=01 &&
//         inst(7:4)(1)=~0 => UNDEFINED,
//      inst(11:10)=10 &&
//         inst(7:4)(1:0)=~00 &&
//         inst(7:4)(1:0)=~11 => UNDEFINED,
//      inst(11:10)=10 &&
//         inst(7:4)(2)=~0 => UNDEFINED,
//      inst(11:10)=11 => UNDEFINED],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(11:10)=11 => UNDEFINED
  if ((inst.Bits() & 0x00000C00)  ==
          0x00000C00)
    return UNDEFINED;

  // inst(11:10)=00 &&
  //       inst(7:4)(0)=~0 => UNDEFINED
  if (((inst.Bits() & 0x00000C00)  ==
          0x00000000) &&
       ((((inst.Bits() & 0x000000F0) >> 4) & 0x00000001)  !=
          0x00000000))
    return UNDEFINED;

  // inst(11:10)=01 &&
  //       inst(7:4)(1)=~0 => UNDEFINED
  if (((inst.Bits() & 0x00000C00)  ==
          0x00000400) &&
       ((((inst.Bits() & 0x000000F0) >> 4) & 0x00000002)  !=
          0x00000000))
    return UNDEFINED;

  // inst(11:10)=10 &&
  //       inst(7:4)(2)=~0 => UNDEFINED
  if (((inst.Bits() & 0x00000C00)  ==
          0x00000800) &&
       ((((inst.Bits() & 0x000000F0) >> 4) & 0x00000004)  !=
          0x00000000))
    return UNDEFINED;

  // inst(11:10)=10 &&
  //       inst(7:4)(1:0)=~00 &&
  //       inst(7:4)(1:0)=~11 => UNDEFINED
  if (((inst.Bits() & 0x00000C00)  ==
          0x00000800) &&
       ((((inst.Bits() & 0x000000F0) >> 4) & 0x00000003)  !=
          0x00000000) &&
       ((((inst.Bits() & 0x000000F0) >> 4) & 0x00000003)  !=
          0x00000003))
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         32  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:8)=1000
//         else 2 + 1
//         if inst(11:8)=1000 ||
//         inst(11:8)=1001
//         else 2 => UNPREDICTABLE,
//      inst(11:8)=1000 ||
//         inst(11:8)=1001 &&
//         inst(5:4)=11 => UNDEFINED,
//      inst(7:6)=11 => UNDEFINED,
//      not inst(11:8)=1000 ||
//         inst(11:8)=1001 ||
//         inst(11:8)=0011 => DECODER_ERROR],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(7:6)=11 => UNDEFINED
  if ((inst.Bits() & 0x000000C0)  ==
          0x000000C0)
    return UNDEFINED;

  // inst(11:8)=1000 ||
  //       inst(11:8)=1001 &&
  //       inst(5:4)=11 => UNDEFINED
  if ((((inst.Bits() & 0x00000F00)  ==
          0x00000800) ||
       ((inst.Bits() & 0x00000F00)  ==
          0x00000900)) &&
       ((inst.Bits() & 0x00000030)  ==
          0x00000030))
    return UNDEFINED;

  // not inst(11:8)=1000 ||
  //       inst(11:8)=1001 ||
  //       inst(11:8)=0011 => DECODER_ERROR
  if (!(((inst.Bits() & 0x00000F00)  ==
          0x00000800) ||
       ((inst.Bits() & 0x00000F00)  ==
          0x00000900) ||
       ((inst.Bits() & 0x00000F00)  ==
          0x00000300)))
    return DECODER_ERROR;

  // 15  ==
  //          inst(19:16) ||
  //       32  <=
  //          inst(22):inst(15:12) + 1
  //       if inst(11:8)=1000
  //       else 2 + 1
  //       if inst(11:8)=1000 ||
  //       inst(11:8)=1001
  //       else 2 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + ((inst.Bits() & 0x00000F00)  ==
          0x00000800
       ? 1
       : 2) + (((inst.Bits() & 0x00000F00)  ==
          0x00000800) ||
       ((inst.Bits() & 0x00000F00)  ==
          0x00000900)
       ? 1
       : 2)) > (32))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(5)=0
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 => UNDEFINED],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(7:6)=11 => UNDEFINED
  if ((inst.Bits() & 0x000000C0)  ==
          0x000000C0)
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) ||
  //       31  <=
  //          inst(22):inst(15:12) + 1
  //       if inst(5)=0
  //       else 2 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + ((inst.Bits() & 0x00000020)  ==
          0x00000000
       ? 1
       : 2)) > (31))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 => UNPREDICTABLE,
//      inst(11:10)=10 &&
//         inst(7:4)(1)=~0 => UNDEFINED,
//      inst(11:10)=11 => UNDEFINED],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(11:10)=11 => UNDEFINED
  if ((inst.Bits() & 0x00000C00)  ==
          0x00000C00)
    return UNDEFINED;

  // inst(11:10)=10 &&
  //       inst(7:4)(1)=~0 => UNDEFINED
  if (((inst.Bits() & 0x00000C00)  ==
          0x00000800) &&
       ((((inst.Bits() & 0x000000F0) >> 4) & 0x00000002)  !=
          0x00000000))
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) ||
  //       31  <=
  //          inst(22):inst(15:12) + 1
  //       if inst(11:10)=00
  //       else (1
  //       if inst(7:4)(1)=0
  //       else 2)
  //       if inst(11:10)=01
  //       else (1
  //       if inst(7:4)(2)=0
  //       else 2)
  //       if inst(11:10)=10
  //       else 0 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + ((inst.Bits() & 0x00000C00)  ==
          0x00000000
       ? 1
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000400
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000002)  ==
          0x00000000
       ? 1
       : 2))
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000800
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000004)  ==
          0x00000000
       ? 1
       : 2))
       : 0)))) > (31))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:8)=0100
//         else 2 + 1
//         if inst(11:8)=0100
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 ||
//         inst(5:4)(1)=1 => UNDEFINED,
//      not inst(11:8)=0100 ||
//         inst(11:8)=0101 => DECODER_ERROR],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(7:6)=11 ||
  //       inst(5:4)(1)=1 => UNDEFINED
  if (((inst.Bits() & 0x000000C0)  ==
          0x000000C0) ||
       ((((inst.Bits() & 0x00000030) >> 4) & 0x00000002)  ==
          0x00000002))
    return UNDEFINED;

  // not inst(11:8)=0100 ||
  //       inst(11:8)=0101 => DECODER_ERROR
  if (!(((inst.Bits() & 0x00000F00)  ==
          0x00000400) ||
       ((inst.Bits() & 0x00000F00)  ==
          0x00000500)))
    return DECODER_ERROR;

  // 15  ==
  //          inst(19:16) ||
  //       31  <=
  //          inst(22):inst(15:12) + 1
  //       if inst(11:8)=0100
  //       else 2 + 1
  //       if inst(11:8)=0100
  //       else 2 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + ((inst.Bits() & 0x00000F00)  ==
          0x00000400
       ? 1
       : 2) + ((inst.Bits() & 0x00000F00)  ==
          0x00000400
       ? 1
       : 2)) > (31))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(5)=0
//         else 2 + 1
//         if inst(5)=0
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 ||
//         inst(4)=1 => UNDEFINED],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(7:6)=11 ||
  //       inst(4)=1 => UNDEFINED
  if (((inst.Bits() & 0x000000C0)  ==
          0x000000C0) ||
       ((inst.Bits() & 0x00000010)  ==
          0x00000010))
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) ||
  //       31  <=
  //          inst(22):inst(15:12) + 1
  //       if inst(5)=0
  //       else 2 + 1
  //       if inst(5)=0
  //       else 2 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + ((inst.Bits() & 0x00000020)  ==
          0x00000000
       ? 1
       : 2) + ((inst.Bits() & 0x00000020)  ==
          0x00000000
       ? 1
       : 2)) > (31))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 => UNPREDICTABLE,
//      inst(11:10)=00 &&
//         inst(7:4)(0)=~0 => UNDEFINED,
//      inst(11:10)=01 &&
//         inst(7:4)(0)=~0 => UNDEFINED,
//      inst(11:10)=10 &&
//         inst(7:4)(1:0)=~00 => UNDEFINED,
//      inst(11:10)=11 => UNDEFINED],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(11:10)=11 => UNDEFINED
  if ((inst.Bits() & 0x00000C00)  ==
          0x00000C00)
    return UNDEFINED;

  // inst(11:10)=00 &&
  //       inst(7:4)(0)=~0 => UNDEFINED
  if (((inst.Bits() & 0x00000C00)  ==
          0x00000000) &&
       ((((inst.Bits() & 0x000000F0) >> 4) & 0x00000001)  !=
          0x00000000))
    return UNDEFINED;

  // inst(11:10)=01 &&
  //       inst(7:4)(0)=~0 => UNDEFINED
  if (((inst.Bits() & 0x00000C00)  ==
          0x00000400) &&
       ((((inst.Bits() & 0x000000F0) >> 4) & 0x00000001)  !=
          0x00000000))
    return UNDEFINED;

  // inst(11:10)=10 &&
  //       inst(7:4)(1:0)=~00 => UNDEFINED
  if (((inst.Bits() & 0x00000C00)  ==
          0x00000800) &&
       ((((inst.Bits() & 0x000000F0) >> 4) & 0x00000003)  !=
          0x00000000))
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) ||
  //       31  <=
  //          inst(22):inst(15:12) + 1
  //       if inst(11:10)=00
  //       else (1
  //       if inst(7:4)(1)=0
  //       else 2)
  //       if inst(11:10)=01
  //       else (1
  //       if inst(7:4)(2)=0
  //       else 2)
  //       if inst(11:10)=10
  //       else 0 + 1
  //       if inst(11:10)=00
  //       else (1
  //       if inst(7:4)(1)=0
  //       else 2)
  //       if inst(11:10)=01
  //       else (1
  //       if inst(7:4)(2)=0
  //       else 2)
  //       if inst(11:10)=10
  //       else 0 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + ((inst.Bits() & 0x00000C00)  ==
          0x00000000
       ? 1
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000400
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000002)  ==
          0x00000000
       ? 1
       : 2))
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000800
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000004)  ==
          0x00000000
       ? 1
       : 2))
       : 0))) + ((inst.Bits() & 0x00000C00)  ==
          0x00000000
       ? 1
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000400
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000002)  ==
          0x00000000
       ? 1
       : 2))
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000800
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000004)  ==
          0x00000000
       ? 1
       : 2))
       : 0)))) > (31))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:8)=0000
//         else 2 + 1
//         if inst(11:8)=0000
//         else 2 + 1
//         if inst(11:8)=0000
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 => UNDEFINED,
//      not inst(11:8)=0000 ||
//         inst(11:8)=0001 => DECODER_ERROR],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(7:6)=11 => UNDEFINED
  if ((inst.Bits() & 0x000000C0)  ==
          0x000000C0)
    return UNDEFINED;

  // not inst(11:8)=0000 ||
  //       inst(11:8)=0001 => DECODER_ERROR
  if (!(((inst.Bits() & 0x00000F00)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00000F00)  ==
          0x00000100)))
    return DECODER_ERROR;

  // 15  ==
  //          inst(19:16) ||
  //       31  <=
  //          inst(22):inst(15:12) + 1
  //       if inst(11:8)=0000
  //       else 2 + 1
  //       if inst(11:8)=0000
  //       else 2 + 1
  //       if inst(11:8)=0000
  //       else 2 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + ((inst.Bits() & 0x00000F00)  ==
          0x00000000
       ? 1
       : 2) + ((inst.Bits() & 0x00000F00)  ==
          0x00000000
       ? 1
       : 2) + ((inst.Bits() & 0x00000F00)  ==
          0x00000000
       ? 1
       : 2)) > (31))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(5)=0
//         else 2 + 1
//         if inst(5)=0
//         else 2 + 1
//         if inst(5)=0
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 &&
//         inst(4)=0 => UNDEFINED],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(7:6)=11 &&
  //       inst(4)=0 => UNDEFINED
  if (((inst.Bits() & 0x000000C0)  ==
          0x000000C0) &&
       ((inst.Bits() & 0x00000010)  ==
          0x00000000))
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) ||
  //       31  <=
  //          inst(22):inst(15:12) + 1
  //       if inst(5)=0
  //       else 2 + 1
  //       if inst(5)=0
  //       else 2 + 1
  //       if inst(5)=0
  //       else 2 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + ((inst.Bits() & 0x00000020)  ==
          0x00000000
       ? 1
       : 2) + ((inst.Bits() & 0x00000020)  ==
          0x00000000
       ? 1
       : 2) + ((inst.Bits() & 0x00000020)  ==
          0x00000000
       ? 1
       : 2)) > (31))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 => UNPREDICTABLE,
//      inst(11:10)=10 &&
//         inst(7:4)(1:0)=11 => UNDEFINED,
//      inst(11:10)=11 => UNDEFINED],
//    small_imm_base_wb: (15  !=
//            inst(3:0)) &&
//         not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}
  //       if (15  !=
  //          inst(3:0))
  //       else {}'
  return (((((inst.Bits() & 0x0000000F)) != (15)))
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(11:10)=11 => UNDEFINED
  if ((inst.Bits() & 0x00000C00)  ==
          0x00000C00)
    return UNDEFINED;

  // inst(11:10)=10 &&
  //       inst(7:4)(1:0)=11 => UNDEFINED
  if (((inst.Bits() & 0x00000C00)  ==
          0x00000800) &&
       ((((inst.Bits() & 0x000000F0) >> 4) & 0x00000003)  ==
          0x00000003))
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) ||
  //       31  <=
  //          inst(22):inst(15:12) + 1
  //       if inst(11:10)=00
  //       else (1
  //       if inst(7:4)(1)=0
  //       else 2)
  //       if inst(11:10)=01
  //       else (1
  //       if inst(7:4)(2)=0
  //       else 2)
  //       if inst(11:10)=10
  //       else 0 + 1
  //       if inst(11:10)=00
  //       else (1
  //       if inst(7:4)(1)=0
  //       else 2)
  //       if inst(11:10)=01
  //       else (1
  //       if inst(7:4)(2)=0
  //       else 2)
  //       if inst(11:10)=10
  //       else 0 + 1
  //       if inst(11:10)=00
  //       else (1
  //       if inst(7:4)(1)=0
  //       else 2)
  //       if inst(11:10)=01
  //       else (1
  //       if inst(7:4)(2)=0
  //       else 2)
  //       if inst(11:10)=10
  //       else 0 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + ((inst.Bits() & 0x00000C00)  ==
          0x00000000
       ? 1
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000400
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000002)  ==
          0x00000000
       ? 1
       : 2))
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000800
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000004)  ==
          0x00000000
       ? 1
       : 2))
       : 0))) + ((inst.Bits() & 0x00000C00)  ==
          0x00000000
       ? 1
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000400
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000002)  ==
          0x00000000
       ? 1
       : 2))
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000800
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000004)  ==
          0x00000000
       ? 1
       : 2))
       : 0))) + ((inst.Bits() & 0x00000C00)  ==
          0x00000000
       ? 1
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000400
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000002)  ==
          0x00000000
       ? 1
       : 2))
       : ((inst.Bits() & 0x00000C00)  ==
          0x00000800
       ? (((((inst.Bits() & 0x000000F0) >> 4) & 0x00000004)  ==
          0x00000000
       ? 1
       : 2))
       : 0)))) > (31))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(15  !=
  //          inst(3:0)) &&
  //       not (15  !=
  //          inst(3:0) &&
  //       13  !=
  //          inst(3:0))'
  return (((((inst.Bits() & 0x0000000F)) != (15)))) &&
       (!((((((inst.Bits() & 0x0000000F)) != (15))) &&
       ((((inst.Bits() & 0x0000000F)) != (13))))));
}

RegisterList Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)
  //       if (15  !=
  //          inst(3:0))
  //       else 32, inst(19:16)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000000F)) != (15)))
       ? (inst.Bits() & 0x0000000F)
       : 32))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(21)=1
//         else 32},
//    is_literal_load: 15  ==
//            inst(19:16),
//    safety: [0  ==
//            inst(7:0) ||
//         32  <=
//            inst(15:12):inst(22) + inst(7:0) => UNPREDICTABLE,
//      15  ==
//            inst(19:16) &&
//         inst(21)=1 => UNPREDICTABLE,
//      inst(23)  ==
//            inst(24) &&
//         inst(21)=1 => UNDEFINED,
//      inst(24)=0 &&
//         inst(23)=0 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(23)=1 &&
//         inst(21)=1 &&
//         13  ==
//            inst(19:16) => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(21)=0 => DECODER_ERROR],
//    small_imm_base_wb: inst(21)=1,
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

bool Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: '15  ==
  //          inst(19:16)'
  return ((((inst.Bits() & 0x000F0000) >> 16)) == (15));
}

SafetyLevel Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(23)=0 &&
  //       inst(21)=0 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00800000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00000000))
    return DECODER_ERROR;

  // inst(24)=1 &&
  //       inst(21)=0 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x01000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00000000))
    return DECODER_ERROR;

  // inst(23)  ==
  //          inst(24) &&
  //       inst(21)=1 => UNDEFINED
  if ((((((inst.Bits() & 0x01000000) >> 24)) == (((inst.Bits() & 0x00800000) >> 23)))) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) &&
  //       inst(21)=1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNPREDICTABLE;

  // inst(24)=0 &&
  //       inst(23)=1 &&
  //       inst(21)=1 &&
  //       13  ==
  //          inst(19:16) => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00800000)  ==
          0x00800000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (13))))
    return DECODER_ERROR;

  // 0  ==
  //          inst(7:0) ||
  //       32  <=
  //          inst(15:12):inst(22) + inst(7:0) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000000FF)) == (0))) ||
       ((((((((inst.Bits() & 0x0000F000) >> 12)) << 1) | ((inst.Bits() & 0x00400000) >> 22)) + (inst.Bits() & 0x000000FF)) > (32))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(21)=1
//         else 32},
//    is_literal_load: 15  ==
//            inst(19:16),
//    safety: [0  ==
//            inst(7:0) / 2 ||
//         16  <=
//            inst(7:0) / 2 ||
//         32  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE,
//      1  ==
//            inst(7:0)(0) => DEPRECATED,
//      15  ==
//            inst(19:16) &&
//         inst(21)=1 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         16  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE,
//      inst(23)  ==
//            inst(24) &&
//         inst(21)=1 => UNDEFINED,
//      inst(24)=0 &&
//         inst(23)=0 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(23)=1 &&
//         inst(21)=1 &&
//         13  ==
//            inst(19:16) => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(21)=0 => DECODER_ERROR],
//    small_imm_base_wb: inst(21)=1,
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

bool Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: '15  ==
  //          inst(19:16)'
  return ((((inst.Bits() & 0x000F0000) >> 16)) == (15));
}

SafetyLevel Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(23)=0 &&
  //       inst(21)=0 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00800000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00000000))
    return DECODER_ERROR;

  // inst(24)=1 &&
  //       inst(21)=0 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x01000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00000000))
    return DECODER_ERROR;

  // inst(23)  ==
  //          inst(24) &&
  //       inst(21)=1 => UNDEFINED
  if ((((((inst.Bits() & 0x01000000) >> 24)) == (((inst.Bits() & 0x00800000) >> 23)))) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) &&
  //       inst(21)=1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNPREDICTABLE;

  // inst(24)=0 &&
  //       inst(23)=1 &&
  //       inst(21)=1 &&
  //       13  ==
  //          inst(19:16) => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00800000)  ==
          0x00800000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (13))))
    return DECODER_ERROR;

  // 0  ==
  //          inst(7:0) / 2 ||
  //       16  <=
  //          inst(7:0) / 2 ||
  //       32  <=
  //          inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE
  if (((((inst.Bits() & 0x000000FF) / 2) == (0))) ||
       ((((inst.Bits() & 0x000000FF) / 2) > (16))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + (inst.Bits() & 0x000000FF) / 2) > (32))))
    return UNPREDICTABLE;

  // VFPSmallRegisterBank() &&
  //       16  <=
  //          inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE
  if ((nacl_arm_dec::VFPSmallRegisterBank()) &&
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + (inst.Bits() & 0x000000FF) / 2) > (16))))
    return UNPREDICTABLE;

  // 1  ==
  //          inst(7:0)(0) => DEPRECATED
  if (((((inst.Bits() & 0x000000FF) & 0x00000001)) == (1)))
    return DEPRECATED;

  return MAY_BE_SAFE;
}


bool Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {},
//    is_literal_load: 15  ==
//            inst(19:16),
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

bool Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: '15  ==
  //          inst(19:16)'
  return ((((inst.Bits() & 0x000F0000) >> 16)) == (15));
}

SafetyLevel Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [(inst(21:20)=00 ||
//         inst(15:12)(0)=1) => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // (inst(21:20)=00 ||
  //       inst(15:12)(0)=1) => UNDEFINED
  if ((((inst.Bits() & 0x00300000)  ==
          0x00000000) ||
       ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [(inst(21:20)=00 ||
//         inst(21:20)=01) => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR,
//      inst(24)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // (inst(21:20)=00 ||
  //       inst(21:20)=01) => UNDEFINED
  if ((((inst.Bits() & 0x00300000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00300000)  ==
          0x00100000)))
    return UNDEFINED;

  // inst(24)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x01000000)  ==
          0x01000000) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)=00 => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR,
//      inst(24)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // inst(21:20)=00 => UNDEFINED
  if ((inst.Bits() & 0x00300000)  ==
          0x00000000)
    return UNDEFINED;

  // inst(24)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x01000000)  ==
          0x01000000) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=11 => UNDEFINED,
//      inst(3:0)(0)=1 => UNDEFINED],
//    uses: {}}

RegisterList Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=11 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  ==
          0x000C0000)
    return UNDEFINED;

  // inst(3:0)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_1
//
// Actual:
//   {defs: {},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(22:21):inst(6:5)(3:0)=0x10 => UNDEFINED],
//    uses: {inst(15:12)}}

RegisterList Actual_VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(22:21):inst(6:5)(3:0)=0x10 => UNDEFINED
  if (((((((inst.Bits() & 0x00600000) >> 21)) << 2) | ((inst.Bits() & 0x00000060) >> 5)) & 0x0000000B)  ==
          0x00000002)
    return UNDEFINED;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// Actual_VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_1
//
// Actual:
//   {defs: {inst(15:12)
//         if inst(20)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE],
//    uses: {inst(15:12)
//         if not inst(20)=1
//         else 32}}

RegisterList Actual_VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? ((inst.Bits() & 0x0000F000) >> 12)
       : 32)));
}

SafetyLevel Actual_VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12)
  //       if not inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register((!((inst.Bits() & 0x00100000)  ==
          0x00100000)
       ? ((inst.Bits() & 0x0000F000) >> 12)
       : 32)));
}

// Actual_VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), inst(19:16)}
//         if inst(20)=1
//         else {},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) => UNPREDICTABLE,
//      inst(20)=1 &&
//         inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {}
//         if inst(20)=1
//         else {inst(15:12), inst(19:16)}}

RegisterList Actual_VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)}
  //       if inst(20)=1
  //       else {}'
  return ((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(20)=1 &&
  //       inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((inst.Bits() & 0x00100000)  ==
          0x00100000) &&
       (((((inst.Bits() & 0x0000F000) >> 12)) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}
  //       if inst(20)=1
  //       else {inst(15:12), inst(19:16)}'
  return ((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? RegisterList()
       : RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))));
}

// Actual_VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), inst(19:16)}
//         if inst(20)=1
//         else {},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         31  ==
//            inst(3:0):inst(5) => UNPREDICTABLE,
//      inst(20)=1 &&
//         inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {}
//         if inst(20)=1
//         else {inst(15:12), inst(19:16)}}

RegisterList Actual_VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)}
  //       if inst(20)=1
  //       else {}'
  return ((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))
       : RegisterList());
}

SafetyLevel Actual_VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       31  ==
  //          inst(3:0):inst(5) => UNPREDICTABLE
  if (((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16))))) ||
       (((((((inst.Bits() & 0x0000000F)) << 1) | ((inst.Bits() & 0x00000020) >> 5))) == (31))))
    return UNPREDICTABLE;

  // inst(20)=1 &&
  //       inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((inst.Bits() & 0x00100000)  ==
          0x00100000) &&
       (((((inst.Bits() & 0x0000F000) >> 12)) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}
  //       if inst(20)=1
  //       else {inst(15:12), inst(19:16)}'
  return ((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? RegisterList()
       : RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))));
}

// Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(5)=0 &&
//         inst(11:8)(0)=1 &&
//         inst(11:8)(3:2)=~11 => DECODER_ERROR,
//      inst(5)=1 &&
//         inst(11:8)=~1110 => DECODER_ERROR,
//      inst(6)=1 &&
//         inst(15:12)(0)=1 => UNDEFINED],
//    uses: {}}

RegisterList Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(5)=0 &&
  //       inst(11:8)(0)=1 &&
  //       inst(11:8)(3:2)=~11 => DECODER_ERROR
  if (((inst.Bits() & 0x00000020)  ==
          0x00000000) &&
       ((((inst.Bits() & 0x00000F00) >> 8) & 0x00000001)  ==
          0x00000001) &&
       ((((inst.Bits() & 0x00000F00) >> 8) & 0x0000000C)  !=
          0x0000000C))
    return DECODER_ERROR;

  // inst(5)=1 &&
  //       inst(11:8)=~1110 => DECODER_ERROR
  if (((inst.Bits() & 0x00000020)  ==
          0x00000020) &&
       ((inst.Bits() & 0x00000F00)  !=
          0x00000E00))
    return DECODER_ERROR;

  // inst(6)=1 &&
  //       inst(15:12)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VMRS_cccc111011110001tttt101000010000_case_1
//
// Actual:
//   {defs: {16
//         if 15  ==
//            inst(15:12)
//         else inst(15:12)}}

RegisterList Actual_VMRS_cccc111011110001tttt101000010000_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16
  //       if 15  ==
  //          inst(15:12)
  //       else inst(15:12)}'
  return RegisterList().
   Add(Register((((((inst.Bits() & 0x0000F000) >> 12)) == (15))
       ? 16
       : ((inst.Bits() & 0x0000F000) >> 12))));
}

SafetyLevel Actual_VMRS_cccc111011110001tttt101000010000_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


// Actual_VMSR_cccc111011100001tttt101000010000_case_1
//
// Actual:
//   {defs: {},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE],
//    uses: {inst(15:12)}}

RegisterList Actual_VMSR_cccc111011100001tttt101000010000_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VMSR_cccc111011100001tttt101000010000_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMSR_cccc111011100001tttt101000010000_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(15:12)(0)=1 => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR,
//      inst(24)=1 ||
//         inst(21:20)=~00 => UNDEFINED],
//    uses: {}}

RegisterList Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // inst(24)=1 ||
  //       inst(21:20)=~00 => UNDEFINED
  if (((inst.Bits() & 0x01000000)  ==
          0x01000000) ||
       ((inst.Bits() & 0x00300000)  !=
          0x00000000))
    return UNDEFINED;

  // inst(15:12)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)=~00 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)=~00 => UNDEFINED
  if ((inst.Bits() & 0x00300000)  !=
          0x00000000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [(inst(11:8)(0)=1 &&
//         inst(11:8)(3:2)=~11) ||
//         inst(11:8)(3:1)=111 => DECODER_ERROR,
//      inst(6)=1 &&
//         inst(15:12)(0)=1 => UNDEFINED],
//    uses: {}}

RegisterList Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(11:8)(0)=1 &&
  //       inst(11:8)(3:2)=~11) ||
  //       inst(11:8)(3:1)=111 => DECODER_ERROR
  if (((((((inst.Bits() & 0x00000F00) >> 8) & 0x00000001)  ==
          0x00000001) &&
       ((((inst.Bits() & 0x00000F00) >> 8) & 0x0000000C)  !=
          0x0000000C))) ||
       ((((inst.Bits() & 0x00000F00) >> 8) & 0x0000000E)  ==
          0x0000000E))
    return DECODER_ERROR;

  // inst(6)=1 &&
  //       inst(15:12)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)(0)=1 ||
//         inst(6)=1 => UNDEFINED],
//    uses: {}}

RegisterList Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)(0)=1 ||
  //       inst(6)=1 => UNDEFINED
  if (((((inst.Bits() & 0x00300000) >> 20) & 0x00000001)  ==
          0x00000001) ||
       ((inst.Bits() & 0x00000040)  ==
          0x00000040))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)=11 => UNDEFINED,
//      inst(6)=1 => UNDEFINED],
//    uses: {}}

RegisterList Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => UNDEFINED
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return UNDEFINED;

  // inst(6)=1 => UNDEFINED
  if ((inst.Bits() & 0x00000040)  ==
          0x00000040)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1
//
// Actual:
//   {base: 13,
//    defs: {13},
//    safety: [0  ==
//            inst(7:0) ||
//         32  <=
//            inst(15:12):inst(22) + inst(7:0) => UNPREDICTABLE],
//    small_imm_base_wb: true,
//    uses: {13},
//    violations: [implied by 'base']}

Register Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '13'
  return Register(13);
}

RegisterList Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{13}'
  return RegisterList().
   Add(Register(13));
}

SafetyLevel Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 0  ==
  //          inst(7:0) ||
  //       32  <=
  //          inst(15:12):inst(22) + inst(7:0) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000000FF)) == (0))) ||
       ((((((((inst.Bits() & 0x0000F000) >> 12)) << 1) | ((inst.Bits() & 0x00400000) >> 22)) + (inst.Bits() & 0x000000FF)) > (32))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'true'
  return true;
}

RegisterList Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{13}'
  return RegisterList().
   Add(Register(13));
}

ViolationSet Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1
//
// Actual:
//   {base: 13,
//    defs: {13},
//    safety: [0  ==
//            inst(7:0) / 2 ||
//         16  <=
//            inst(7:0) / 2 ||
//         32  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE,
//      1  ==
//            inst(7:0)(0) => DEPRECATED,
//      VFPSmallRegisterBank() &&
//         16  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE],
//    small_imm_base_wb: true,
//    uses: {13},
//    violations: [implied by 'base']}

Register Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '13'
  return Register(13);
}

RegisterList Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{13}'
  return RegisterList().
   Add(Register(13));
}

SafetyLevel Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 0  ==
  //          inst(7:0) / 2 ||
  //       16  <=
  //          inst(7:0) / 2 ||
  //       32  <=
  //          inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE
  if (((((inst.Bits() & 0x000000FF) / 2) == (0))) ||
       ((((inst.Bits() & 0x000000FF) / 2) > (16))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + (inst.Bits() & 0x000000FF) / 2) > (32))))
    return UNPREDICTABLE;

  // VFPSmallRegisterBank() &&
  //       16  <=
  //          inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE
  if ((nacl_arm_dec::VFPSmallRegisterBank()) &&
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + (inst.Bits() & 0x000000FF) / 2) > (16))))
    return UNPREDICTABLE;

  // 1  ==
  //          inst(7:0)(0) => DEPRECATED
  if (((((inst.Bits() & 0x000000FF) & 0x00000001)) == (1)))
    return DEPRECATED;

  return MAY_BE_SAFE;
}


bool Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'true'
  return true;
}

RegisterList Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{13}'
  return RegisterList().
   Add(Register(13));
}

ViolationSet Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)=00 ||
//         inst(15:12)(0)=1 => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // inst(21:20)=00 ||
  //       inst(15:12)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x00300000)  ==
          0x00000000) ||
       ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [(inst(21:20)=11 ||
//         inst(21:20)=00) => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // (inst(21:20)=11 ||
  //       inst(21:20)=00) => UNDEFINED
  if ((((inst.Bits() & 0x00300000)  ==
          0x00300000) ||
       ((inst.Bits() & 0x00300000)  ==
          0x00000000)))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=11 ||
//         inst(3:0)(0)=1 => UNDEFINED,
//      inst(7:6)=00 => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(7:6)=00 => DECODER_ERROR
  if ((inst.Bits() & 0x000000C0)  ==
          0x00000000)
    return DECODER_ERROR;

  // inst(19:18)=11 ||
  //       inst(3:0)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x000C0000)  ==
          0x000C0000) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:16)=000xxx => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(8)=0 => DECODER_ERROR,
//      inst(3:0)(0)=1 => UNDEFINED],
//    uses: {}}

RegisterList Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:16)=000xxx => DECODER_ERROR
  if ((inst.Bits() & 0x00380000)  ==
          0x00000000)
    return DECODER_ERROR;

  // inst(3:0)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  // inst(24)=0 &&
  //       inst(8)=0 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00000100)  ==
          0x00000000))
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(24)=0 &&
//         inst(8)=0 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED,
//      inst(7):inst(21:16)(6:0)=0000xxx => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(7):inst(21:16)(6:0)=0000xxx => DECODER_ERROR
  if (((((((inst.Bits() & 0x00000080) >> 7)) << 6) | ((inst.Bits() & 0x003F0000) >> 16)) & 0x00000078)  ==
          0x00000000)
    return DECODER_ERROR;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(24)=0 &&
  //       inst(8)=0 => UNDEFINED
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00000100)  ==
          0x00000000))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [3  <
//            inst(8:7) + inst(19:18) => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 3  <
  //          inst(8:7) + inst(19:18) => UNDEFINED
  if (((((inst.Bits() & 0x00000180) >> 7) + ((inst.Bits() & 0x000C0000) >> 18)) >= (3)))
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:16)=000xxx => DECODER_ERROR,
//      inst(3:0)(0)=1 => UNDEFINED],
//    uses: {}}

RegisterList Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:16)=000xxx => DECODER_ERROR
  if ((inst.Bits() & 0x00380000)  ==
          0x00000000)
    return DECODER_ERROR;

  // inst(3:0)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED,
//      inst(7):inst(21:16)(6:0)=0000xxx => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(7):inst(21:16)(6:0)=0000xxx => DECODER_ERROR
  if (((((((inst.Bits() & 0x00000080) >> 7)) << 6) | ((inst.Bits() & 0x003F0000) >> 16)) & 0x00000078)  ==
          0x00000000)
    return DECODER_ERROR;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(15:12)(0)=1 => UNDEFINED,
//      inst(21:16)=000xxx => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:16)=000xxx => DECODER_ERROR
  if ((inst.Bits() & 0x00380000)  ==
          0x00000000)
    return DECODER_ERROR;

  // inst(15:12)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=11 ||
//         inst(15:12)(0)=1 => UNDEFINED],
//    uses: {}}

RegisterList Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=11 ||
  //       inst(15:12)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x000C0000)  ==
          0x000C0000) ||
       ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(21)=1
//         else 32},
//    safety: [0  ==
//            inst(7:0) ||
//         32  <=
//            inst(15:12):inst(22) + inst(7:0) => UNPREDICTABLE,
//      15  ==
//            inst(19:16) &&
//         inst(21)=1 => UNPREDICTABLE,
//      15  ==
//            inst(19:16) => FORBIDDEN_OPERANDS,
//      inst(23)  ==
//            inst(24) &&
//         inst(21)=1 => UNDEFINED,
//      inst(24)=0 &&
//         inst(23)=0 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(23)=0 &&
//         inst(21)=1 &&
//         13  ==
//            inst(19:16) => DECODER_ERROR],
//    small_imm_base_wb: inst(21)=1,
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(23)=0 &&
  //       inst(21)=0 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00800000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00000000))
    return DECODER_ERROR;

  // inst(24)=1 &&
  //       inst(21)=0 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x01000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00000000))
    return DECODER_ERROR;

  // inst(23)  ==
  //          inst(24) &&
  //       inst(21)=1 => UNDEFINED
  if ((((((inst.Bits() & 0x01000000) >> 24)) == (((inst.Bits() & 0x00800000) >> 23)))) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) &&
  //       inst(21)=1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNPREDICTABLE;

  // inst(24)=1 &&
  //       inst(23)=0 &&
  //       inst(21)=1 &&
  //       13  ==
  //          inst(19:16) => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x01000000) &&
       ((inst.Bits() & 0x00800000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (13))))
    return DECODER_ERROR;

  // 15  ==
  //          inst(19:16) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (15)))
    return FORBIDDEN_OPERANDS;

  // 0  ==
  //          inst(7:0) ||
  //       32  <=
  //          inst(15:12):inst(22) + inst(7:0) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000000FF)) == (0))) ||
       ((((((((inst.Bits() & 0x0000F000) >> 12)) << 1) | ((inst.Bits() & 0x00400000) >> 22)) + (inst.Bits() & 0x000000FF)) > (32))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(21)=1
//         else 32},
//    safety: [0  ==
//            inst(7:0) / 2 ||
//         16  <=
//            inst(7:0) / 2 ||
//         32  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE,
//      1  ==
//            inst(7:0)(0) => DEPRECATED,
//      15  ==
//            inst(19:16) &&
//         inst(21)=1 => UNPREDICTABLE,
//      15  ==
//            inst(19:16) => FORBIDDEN_OPERANDS,
//      VFPSmallRegisterBank() &&
//         16  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE,
//      inst(23)  ==
//            inst(24) &&
//         inst(21)=1 => UNDEFINED,
//      inst(24)=0 &&
//         inst(23)=0 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(23)=0 &&
//         inst(21)=1 &&
//         13  ==
//            inst(19:16) => DECODER_ERROR],
//    small_imm_base_wb: inst(21)=1,
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(23)=0 &&
  //       inst(21)=0 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00800000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00000000))
    return DECODER_ERROR;

  // inst(24)=1 &&
  //       inst(21)=0 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x01000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00000000))
    return DECODER_ERROR;

  // inst(23)  ==
  //          inst(24) &&
  //       inst(21)=1 => UNDEFINED
  if ((((((inst.Bits() & 0x01000000) >> 24)) == (((inst.Bits() & 0x00800000) >> 23)))) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNDEFINED;

  // 15  ==
  //          inst(19:16) &&
  //       inst(21)=1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNPREDICTABLE;

  // inst(24)=1 &&
  //       inst(23)=0 &&
  //       inst(21)=1 &&
  //       13  ==
  //          inst(19:16) => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x01000000) &&
       ((inst.Bits() & 0x00800000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (13))))
    return DECODER_ERROR;

  // 15  ==
  //          inst(19:16) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (15)))
    return FORBIDDEN_OPERANDS;

  // 0  ==
  //          inst(7:0) / 2 ||
  //       16  <=
  //          inst(7:0) / 2 ||
  //       32  <=
  //          inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE
  if (((((inst.Bits() & 0x000000FF) / 2) == (0))) ||
       ((((inst.Bits() & 0x000000FF) / 2) > (16))) ||
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + (inst.Bits() & 0x000000FF) / 2) > (32))))
    return UNPREDICTABLE;

  // VFPSmallRegisterBank() &&
  //       16  <=
  //          inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE
  if ((nacl_arm_dec::VFPSmallRegisterBank()) &&
       ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12)) + (inst.Bits() & 0x000000FF) / 2) > (16))))
    return UNPREDICTABLE;

  // 1  ==
  //          inst(7:0)(0) => DEPRECATED
  if (((((inst.Bits() & 0x000000FF) & 0x00000001)) == (1)))
    return DEPRECATED;

  return MAY_BE_SAFE;
}


bool Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {},
//    safety: [15  ==
//            inst(19:16) => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=~00 => UNDEFINED,
//      inst(22):inst(15:12)  ==
//            inst(5):inst(3:0) => UNKNOWN,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(22):inst(15:12)  ==
  //          inst(5):inst(3:0) => UNKNOWN
  if ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12))) == ((((((inst.Bits() & 0x00000020) >> 5)) << 4) | (inst.Bits() & 0x0000000F)))))
    return UNKNOWN;

  // inst(19:18)=~00 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00000000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VTBL_VTBX_111100111d11nnnndddd10ccnpm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [32  <=
//            inst(7):inst(19:16) + inst(9:8) + 1 => UNPREDICTABLE],
//    uses: {}}

RegisterList Actual_VTBL_VTBX_111100111d11nnnndddd10ccnpm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VTBL_VTBX_111100111d11nnnndddd10ccnpm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 32  <=
  //          inst(7):inst(19:16) + inst(9:8) + 1 => UNPREDICTABLE
  if ((((((((inst.Bits() & 0x00000080) >> 7)) << 4) | ((inst.Bits() & 0x000F0000) >> 16)) + ((inst.Bits() & 0x00000300) >> 8) + 1) > (32)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_VTBL_VTBX_111100111d11nnnndddd10ccnpm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=11 => UNDEFINED,
//      inst(22):inst(15:12)  ==
//            inst(5):inst(3:0) => UNKNOWN,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(22):inst(15:12)  ==
  //          inst(5):inst(3:0) => UNKNOWN
  if ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12))) == ((((((inst.Bits() & 0x00000020) >> 5)) << 4) | (inst.Bits() & 0x0000000F)))))
    return UNKNOWN;

  // inst(19:18)=11 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  ==
          0x000C0000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=11 ||
//         (inst(6)=0 &&
//         inst(19:18)=10) => UNDEFINED,
//      inst(22):inst(15:12)  ==
//            inst(5):inst(3:0) => UNKNOWN,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(22):inst(15:12)  ==
  //          inst(5):inst(3:0) => UNKNOWN
  if ((((((((inst.Bits() & 0x00400000) >> 22)) << 4) | ((inst.Bits() & 0x0000F000) >> 12))) == ((((((inst.Bits() & 0x00000020) >> 5)) << 4) | (inst.Bits() & 0x0000000F)))))
    return UNKNOWN;

  // inst(19:18)=11 ||
  //       (inst(6)=0 &&
  //       inst(19:18)=10) => UNDEFINED
  if (((inst.Bits() & 0x000C0000)  ==
          0x000C0000) ||
       ((((inst.Bits() & 0x00000040)  ==
          0x00000000) &&
       ((inst.Bits() & 0x000C0000)  ==
          0x00080000))))
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

}  // namespace nacl_arm_dec
