/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#include "native_client/src/trusted/validator_arm/gen/arm32_decode_actuals.h"
#include "native_client/src/trusted/validator_arm/inst_classes_inline.h"

namespace nacl_arm_dec {

// Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16)}}

RegisterList Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16), inst(3:0)}}

RegisterList Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(19:16), inst(3:0), inst(11:8)}}

RegisterList Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      (inst(19:16)=1111 &&
//         inst(20)=0) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16)}}

RegisterList Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // (inst(19:16)=1111 &&
  //       inst(20)=0) => DECODER_ERROR
  if ((((inst.Bits() & 0x000F0000)  ==
          0x000F0000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00000000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {15}}

RegisterList Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

// Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(3:0)}}

RegisterList Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8)}}

RegisterList Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(20:16)  <
//            inst(11:7) => UNPREDICTABLE],
//    uses: {inst(15:12)}}

RegisterList Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  // inst(20:16)  <
  //          inst(11:7) => UNPREDICTABLE
  if (((((inst.Bits() & 0x001F0000) >> 16)) < (((inst.Bits() & 0x00000F80) >> 7))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      15  ==
//            inst(3:0) => DECODER_ERROR,
//      inst(20:16)  <
//            inst(11:7) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(15:12)}}

RegisterList Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(3:0) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000000F)) == (15)))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  // inst(20:16)  <
  //          inst(11:7) => UNPREDICTABLE
  if (((((inst.Bits() & 0x001F0000) >> 16)) < (((inst.Bits() & 0x00000F80) >> 7))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1
//
// Actual:
//   {clears_bits: (ARMExpandImm(inst(11:0)) &&
//         clears_mask())  ==
//            clears_mask(),
//    defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16)}}

bool Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1::
clears_bits(Instruction inst, uint32_t clears_mask) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // clears_bits: '(ARMExpandImm(inst(11:0)) &&
  //       clears_mask())  ==
  //          clears_mask()'
  return ((((nacl_arm_dec::ARMExpandImm((inst.Bits() & 0x00000FFF)) & clears_mask))) == (clears_mask));
}

RegisterList Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1
//
// Actual:
//   {defs: {},
//    is_literal_pool_head: LiteralPoolHeadConstant()  ==
//            inst,
//    safety: [inst(31:28)=~1110 => UNPREDICTABLE,
//      not IsBreakPointAndConstantPoolHead(inst) => FORBIDDEN_OPERANDS],
//    uses: {},
//    violations: [implied by 'is_literal_pool_head']}

RegisterList Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

bool Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1::
is_literal_pool_head(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_pool_head: 'LiteralPoolHeadConstant()  ==
  //          inst'
  return ((inst.Bits()) == (nacl_arm_dec::LiteralPoolHeadConstant()));
}

SafetyLevel Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(31:28)=~1110 => UNPREDICTABLE
  if ((inst.Bits() & 0xF0000000)  !=
          0xE0000000)
    return UNPREDICTABLE;

  // not IsBreakPointAndConstantPoolHead(inst) => FORBIDDEN_OPERANDS
  if (!(nacl_arm_dec::IsBreakPointAndConstantPoolHead(inst.Bits())))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

ViolationSet Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // If a pool head, mark address appropriately and then skip over
  // the constant bundle.
  validate_literal_pool_head(second, sfi, critical, next_inst_addr);

  return violations;
}


// Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => FORBIDDEN],
//    uses: {}}

RegisterList Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_BLX_register_cccc000100101111111111110011mmmm_case_1
//
// Actual:
//   {defs: {15, 14},
//    safety: [inst(3:0)=1111 => FORBIDDEN_OPERANDS],
//    target: inst(3:0),
//    uses: {inst(3:0)},
//    violations: [implied by 'target']}

RegisterList Actual_BLX_register_cccc000100101111111111110011mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{15, 14}'
  return RegisterList().
   Add(Register(15)).
   Add(Register(14));
}

SafetyLevel Actual_BLX_register_cccc000100101111111111110011mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(3:0)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000000F)  ==
          0x0000000F)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


Register Actual_BLX_register_cccc000100101111111111110011mmmm_case_1::
branch_target_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // target: 'inst(3:0)'
  return Register((inst.Bits() & 0x0000000F));
}

RegisterList Actual_BLX_register_cccc000100101111111111110011mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet Actual_BLX_register_cccc000100101111111111110011mmmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Indirect branches (through a target register) need to be masked,
  // and if they represent a call, they need to be end-of-bundle aligned.
  violations = ViolationUnion(
      violations, get_branch_mask_violations(first, second, sfi, critical));
  violations = ViolationUnion(
      violations, get_call_position_violations(second, sfi));

  return violations;
}


// Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {15, 14},
//    relative: true,
//    relative_offset: SignExtend(inst(23:0):'00'(1:0), 32) + 8,
//    safety: [true => MAY_BE_SAFE],
//    uses: {15},
//    violations: [implied by 'relative']}

RegisterList Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{15, 14}'
  return RegisterList().
   Add(Register(15)).
   Add(Register(14));
}

bool Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1::
is_relative_branch(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // relative: 'true'
  return true;
}

int32_t Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1::
branch_target_offset(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // relative_offset: "SignExtend(inst(23:0):'00'(1:0), 32) + 8"
  return (((((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) & 0x02000000)
       ? ((((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) | 0xFC000000)
       : ((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) + 8;
}

SafetyLevel Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Direct (relative) branches can represent a call. If so, they need
  // to be end-of-bundle aligned.
  violations = ViolationUnion(
      violations, get_call_position_violations(second, sfi));

  return violations;
}


// Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {15},
//    relative: true,
//    relative_offset: SignExtend(inst(23:0):'00'(1:0), 32) + 8,
//    safety: [true => MAY_BE_SAFE],
//    uses: {15},
//    violations: [implied by 'relative']}

RegisterList Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{15}'
  return RegisterList().
   Add(Register(15));
}

bool Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1::
is_relative_branch(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // relative: 'true'
  return true;
}

int32_t Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1::
branch_target_offset(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // relative_offset: "SignExtend(inst(23:0):'00'(1:0), 32) + 8"
  return (((((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) & 0x02000000)
       ? ((((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) | 0xFC000000)
       : ((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) + 8;
}

SafetyLevel Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Direct (relative) branches can represent a call. If so, they need
  // to be end-of-bundle aligned.
  violations = ViolationUnion(
      violations, get_call_position_violations(second, sfi));

  return violations;
}


// Actual_Bx_cccc000100101111111111110001mmmm_case_1
//
// Actual:
//   {defs: {15},
//    safety: [inst(3:0)=1111 => FORBIDDEN_OPERANDS],
//    target: inst(3:0),
//    uses: {inst(3:0)},
//    violations: [implied by 'target']}

RegisterList Actual_Bx_cccc000100101111111111110001mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{15}'
  return RegisterList().
   Add(Register(15));
}

SafetyLevel Actual_Bx_cccc000100101111111111110001mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(3:0)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000000F)  ==
          0x0000000F)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


Register Actual_Bx_cccc000100101111111111110001mmmm_case_1::
branch_target_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // target: 'inst(3:0)'
  return Register((inst.Bits() & 0x0000000F));
}

RegisterList Actual_Bx_cccc000100101111111111110001mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet Actual_Bx_cccc000100101111111111110001mmmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Indirect branches (through a target register) need to be masked,
  // and if they represent a call, they need to be end-of-bundle aligned.
  violations = ViolationUnion(
      violations, get_branch_mask_violations(first, second, sfi, critical));
  violations = ViolationUnion(
      violations, get_call_position_violations(second, sfi));

  return violations;
}


// Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE],
//    uses: {inst(3:0)}}

RegisterList Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {16},
//    uses: {inst(19:16)}}

RegisterList Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16}'
  return RegisterList().
   Add(Register(16));
}

SafetyLevel Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1
//
// Actual:
//   {defs: {16
//         if inst(20)
//         else 32},
//    uses: {inst(19:16), inst(3:0)}}

RegisterList Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1
//
// Actual:
//   {defs: {16},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(19:16), inst(3:0), inst(11:8)}}

RegisterList Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16}'
  return RegisterList().
   Add(Register(16));
}

SafetyLevel Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=~01 => UNDEFINED,
//      inst(8)=1 &&
//         inst(15:12)(0)=1 => UNDEFINED,
//      not inst(8)=1 &&
//         inst(3:0)(0)=1 => UNDEFINED],
//    uses: {}}

RegisterList Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=~01 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00040000)
    return UNDEFINED;

  // inst(8)=1 &&
  //       inst(15:12)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000100)  ==
          0x00000100) &&
       ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  // not inst(8)=1 &&
  //       inst(3:0)(0)=1 => UNDEFINED
  if ((!((inst.Bits() & 0x00000100)  ==
          0x00000100)) &&
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_DMB_1111010101111111111100000101xxxx_case_1
//
// Actual:
//   {defs: {},
//    safety: [not '1111'(3:0)  ==
//            inst(3:0) ||
//         '1110'(3:0)  ==
//            inst(3:0) ||
//         '1011'(3:0)  ==
//            inst(3:0) ||
//         '1010'(3:0)  ==
//            inst(3:0) ||
//         '0111'(3:0)  ==
//            inst(3:0) ||
//         '0110'(3:0)  ==
//            inst(3:0) ||
//         '0011'(3:0)  ==
//            inst(3:0) ||
//         '0010'(3:0)  ==
//            inst(3:0) => FORBIDDEN_OPERANDS],
//    uses: {}}

RegisterList Actual_DMB_1111010101111111111100000101xxxx_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_DMB_1111010101111111111100000101xxxx_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // not '1111'(3:0)  ==
  //          inst(3:0) ||
  //       '1110'(3:0)  ==
  //          inst(3:0) ||
  //       '1011'(3:0)  ==
  //          inst(3:0) ||
  //       '1010'(3:0)  ==
  //          inst(3:0) ||
  //       '0111'(3:0)  ==
  //          inst(3:0) ||
  //       '0110'(3:0)  ==
  //          inst(3:0) ||
  //       '0011'(3:0)  ==
  //          inst(3:0) ||
  //       '0010'(3:0)  ==
  //          inst(3:0) => FORBIDDEN_OPERANDS
  if (!(((((inst.Bits() & 0x0000000F)) == ((15 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((14 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((11 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((10 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((7 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((6 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((3 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((2 & 0x0000000F))))))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_DMB_1111010101111111111100000101xxxx_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_ISB_1111010101111111111100000110xxxx_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(3:0)=~1111 => FORBIDDEN_OPERANDS],
//    uses: {}}

RegisterList Actual_ISB_1111010101111111111100000110xxxx_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_ISB_1111010101111111111100000110xxxx_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(3:0)=~1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000000F)  !=
          0x0000000F)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_ISB_1111010101111111111100000110xxxx_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: Union({inst(19:16)
//         if inst(21)=1
//         else 32}, RegisterList(inst(15:0))),
//    safety: [15  ==
//            inst(19:16) ||
//         NumGPRs(RegisterList(inst(15:0)))  <
//            1 => UNPREDICTABLE,
//      Contains(RegisterList(inst(15:0)), 15) => FORBIDDEN_OPERANDS,
//      inst(21)=1 &&
//         Contains(RegisterList(inst(15:0)), inst(19:16)) => UNKNOWN],
//    small_imm_base_wb: inst(21)=1,
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: 'Union({inst(19:16)
  //       if inst(21)=1
  //       else 32}, RegisterList(inst(15:0)))'
  return nacl_arm_dec::Union(RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32))), nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)));
}

SafetyLevel Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       NumGPRs(RegisterList(inst(15:0)))  <
  //          1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((nacl_arm_dec::NumGPRs(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)))) < (1))))
    return UNPREDICTABLE;

  // inst(21)=1 &&
  //       Contains(RegisterList(inst(15:0)), inst(19:16)) => UNKNOWN
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(((inst.Bits() & 0x000F0000) >> 16)))))
    return UNKNOWN;

  // Contains(RegisterList(inst(15:0)), 15) => FORBIDDEN_OPERANDS
  if (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


bool Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1::
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


// Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      15  ==
//            inst(19:16) => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    small_imm_base_wb: inst(24)=0 ||
//         inst(21)=1,
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) => DECODER_ERROR
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (15)))
    return DECODER_ERROR;

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(24)=0 ||
  //       inst(21)=1'
  return ((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000);
}

RegisterList Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1::
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


// Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: 15,
//    defs: {inst(15:12)},
//    is_literal_load: true,
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE],
//    uses: {15},
//    violations: [implied by 'base']}

Register Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

bool Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1::
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


// Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         inst(24)=0 ||
//         inst(21)=1 &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(3:0), inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1::
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


// Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(15:12) + 1, inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12) + 1  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) + 1 => UNPREDICTABLE,
//      inst(15:12)(0)=1 => UNPREDICTABLE,
//      inst(19:16)=1111 => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(21)=1 => UNPREDICTABLE],
//    small_imm_base_wb: (inst(24)=0) ||
//         (inst(21)=1),
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(15:12) + 1, inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1)).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // inst(15:12)(0)=1 => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNPREDICTABLE;

  // inst(24)=0 &&
  //       inst(21)=1 => UNPREDICTABLE
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12) + 1  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12) + 1))))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) + 1 => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12) + 1) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(inst(24)=0) ||
  //       (inst(21)=1)'
  return (((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000));
}

RegisterList Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1::
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


// Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1
//
// Actual:
//   {base: 15,
//    defs: {inst(15:12), inst(15:12) + 1},
//    is_literal_load: true,
//    safety: [15  ==
//            inst(15:12) + 1 => UNPREDICTABLE,
//      inst(15:12)(0)=1 => UNPREDICTABLE],
//    uses: {15},
//    violations: [implied by 'base']}

Register Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(15:12) + 1}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1));
}

bool Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)(0)=1 => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) + 1 => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12) + 1) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1::
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


// Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(15:12) + 1, inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12) + 1  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) + 1 ||
//         15  ==
//            inst(3:0) ||
//         inst(15:12)  ==
//            inst(3:0) ||
//         inst(15:12) + 1  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         (inst(24)=0) ||
//         (inst(21)=1) &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(15:12)(0)=1 => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(19:16), inst(3:0)},
//    violations: [implied by 'base']}

Register Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(15:12) + 1, inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1)).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)(0)=1 => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNPREDICTABLE;

  // inst(24)=0 &&
  //       inst(21)=1 => UNPREDICTABLE
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) + 1 ||
  //       15  ==
  //          inst(3:0) ||
  //       inst(15:12)  ==
  //          inst(3:0) ||
  //       inst(15:12) + 1  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12) + 1) == (15))) ||
       ((((inst.Bits() & 0x0000000F)) == (15))) ||
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x0000F000) >> 12) + 1))))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12) + 1  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12) + 1))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       ((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1::
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


// Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1::
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


// Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(15:12) + 1},
//    safety: [inst(15:12)(0)=1 ||
//         14  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(15:12) + 1}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1));
}

SafetyLevel Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)(0)=1 ||
  //       14  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == (14))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (15))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1::
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


// Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [15  ==
//            inst(15:12) => FORBIDDEN_OPERANDS,
//      15  ==
//            inst(15:12) ||
//         ((inst(24)=0) ||
//         (inst(21)=1) &&
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      inst(19:16)=1111 => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR],
//    small_imm_base_wb: (inst(24)=0) ||
//         (inst(21)=1),
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       ((inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12)) == (15))) ||
       ((((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


bool Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(inst(24)=0) ||
  //       (inst(21)=1)'
  return (((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000));
}

RegisterList Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1::
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


// Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1
//
// Actual:
//   {base: 15,
//    defs: {inst(15:12)},
//    is_literal_load: true,
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(21)  ==
//            inst(24) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR],
//    uses: {15},
//    violations: [implied by 'base']}

Register Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

bool Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // inst(21)  ==
  //          inst(24) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000) >> 24)) == (((inst.Bits() & 0x00200000) >> 21))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1::
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


// Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         (inst(24)=0) ||
//         (inst(21)=1) &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(19:16), inst(3:0)},
//    violations: [implied by 'base']}

Register Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       ((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1::
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


// Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    is_load_tp: 9  ==
//            inst(19:16) &&
//         inst(24)=1 &&
//         not inst(24)=0 ||
//         inst(21)=1 &&
//         inst(23)=1 &&
//         0  ==
//            inst(11:0) ||
//         4  ==
//            inst(11:0),
//    safety: [15  ==
//            inst(15:12) => FORBIDDEN_OPERANDS,
//      15  ==
//            inst(19:16) => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    small_imm_base_wb: inst(24)=0 ||
//         inst(21)=1,
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

bool Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1::
is_load_thread_address_pointer(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_load_tp: '9  ==
  //          inst(19:16) &&
  //       inst(24)=1 &&
  //       not inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(23)=1 &&
  //       0  ==
  //          inst(11:0) ||
  //       4  ==
  //          inst(11:0)'
  return (((((inst.Bits() & 0x000F0000) >> 16)) == (9))) &&
       ((inst.Bits() & 0x01000000)  ==
          0x01000000) &&
       (!(((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       ((inst.Bits() & 0x00800000)  ==
          0x00800000) &&
       (((((inst.Bits() & 0x00000FFF)) == (0))) ||
       ((((inst.Bits() & 0x00000FFF)) == (4))));
}

SafetyLevel Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) => DECODER_ERROR
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (15)))
    return DECODER_ERROR;

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


bool Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(24)=0 ||
  //       inst(21)=1'
  return ((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000);
}

RegisterList Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1::
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


// Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: 15,
//    defs: {inst(15:12)},
//    is_literal_load: true,
//    safety: [15  ==
//            inst(15:12) => FORBIDDEN_OPERANDS],
//    uses: {15},
//    violations: [implied by 'base']}

Register Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

bool Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1::
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


// Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) => FORBIDDEN_OPERANDS,
//      15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         inst(24)=0 ||
//         inst(21)=1 &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(3:0), inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000000F)) == (15)))
    return UNPREDICTABLE;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  // 15  ==
  //          inst(15:12) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1::
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


// Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(11:7)=00000 => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(3:0)}}

RegisterList Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(11:7)=00000 => DECODER_ERROR
  if ((inst.Bits() & 0x00000F80)  ==
          0x00000000)
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1
//
// Actual:
//   {defs: {},
//    diagnostics: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//     error('Consider using DSB (defined in ARMv7) for memory barrier')],
//    safety: [true => FORBIDDEN],
//    uses: {},
//    violations: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//     error('Consider using DSB (defined in ARMv7) for memory barrier')]}

RegisterList Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

void Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1::
generate_diagnostics(ViolationSet violations,
                     const nacl_arm_val::DecodedInstruction& first,
                     const nacl_arm_val::DecodedInstruction& second,
                     const nacl_arm_val::SfiValidator& sfi,
                     nacl_arm_val::ProblemSink* out) const {
  ClassDecoder::generate_diagnostics(violations, first, second, sfi, out);
  const Instruction& inst = second.inst();
  if (ContainsViolation(violations, OTHER_VIOLATION)) {

    // inst(31:0)=xxxx111000000111xxxx111110111010 =>
    //   error('Consider using DSB (defined in ARMv7) for memory barrier')
    if ((inst.Bits() & 0x0FFF0FFF)  ==
          0x0E070FBA) {
      out->ReportProblemDiagnostic(OTHER_VIOLATION, second.addr(),
         "Consider using DSB (defined in ARMv7) for memory barrier");
    }

  }
}


SafetyLevel Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

ViolationSet Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);
  const Instruction& inst = second.inst();

  // inst(31:0)=xxxx111000000111xxxx111110111010 =>
  //   error('Consider using DSB (defined in ARMv7) for memory barrier')
  if ((inst.Bits() & 0x0FFF0FFF)  ==
          0x0E070FBA)
     violations = ViolationUnion(violations, ViolationBit(OTHER_VIOLATION));

  return violations;
}


// Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16), 16
//         if inst(20)=1
//         else 32},
//    safety: [(ArchVersion()  <
//            6 &&
//         inst(19:16)  ==
//            inst(3:0)) => UNPREDICTABLE,
//      15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) ||
//         15  ==
//            inst(15:12) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8), inst(15:12)}}

RegisterList Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) ||
  //       15  ==
  //          inst(15:12) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))))
    return UNPREDICTABLE;

  // (ArchVersion()  <
  //          6 &&
  //       inst(19:16)  ==
  //          inst(3:0)) => UNPREDICTABLE
  if (((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F))))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16)},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) ||
//         15  ==
//            inst(15:12) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8), inst(15:12)}}

RegisterList Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) ||
  //       15  ==
  //          inst(15:12) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// Actual_MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(23):inst(22:21):inst(6:5)(4:0)=10x00 ||
//         inst(23):inst(22:21):inst(6:5)(4:0)=x0x10 => UNDEFINED]}

RegisterList Actual_MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(23):inst(22:21):inst(6:5)(4:0)=10x00 ||
  //       inst(23):inst(22:21):inst(6:5)(4:0)=x0x10 => UNDEFINED
  if (((((((((((inst.Bits() & 0x00800000) >> 23)) << 2) | ((inst.Bits() & 0x00600000) >> 21))) << 2) | ((inst.Bits() & 0x00000060) >> 5)) & 0x0000001B)  ==
          0x00000010) ||
       ((((((((((inst.Bits() & 0x00800000) >> 23)) << 2) | ((inst.Bits() & 0x00600000) >> 21))) << 2) | ((inst.Bits() & 0x00000060) >> 5)) & 0x0000000B)  ==
          0x00000002))
    return UNDEFINED;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


// Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)
//         else 32},
//    dynamic_code_replace_immediates: {inst(19:16), inst(11:0)},
//    safety: [inst(15:12)=1111 => UNPREDICTABLE],
//    uses: {}}

RegisterList Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

Instruction Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1::
dynamic_code_replacement_sentinel(
     Instruction inst) const {
  if (!defs(inst).ContainsAny(RegisterList::DynCodeReplaceFrozenRegs())) {
    // inst(19:16)
    inst.SetBits(19, 16, 0);
    // inst(11:0)
    inst.SetBits(11, 0, 0);
  }
  return inst;
}

SafetyLevel Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)=1111 => UNPREDICTABLE
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    dynamic_code_replace_immediates: {inst(11:0)},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {}}

RegisterList Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

Instruction Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1::
dynamic_code_replacement_sentinel(
     Instruction inst) const {
  if (!defs(inst).ContainsAny(RegisterList::DynCodeReplaceFrozenRegs())) {
    // inst(11:0)
    inst.SetBits(11, 0, 0);
  }
  return inst;
}

SafetyLevel Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_MRS_cccc00010r001111dddd000000000000_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [inst(15:12)=1111 => UNPREDICTABLE,
//      inst(22)=1 => FORBIDDEN_OPERANDS],
//    uses: {}}

RegisterList Actual_MRS_cccc00010r001111dddd000000000000_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_MRS_cccc00010r001111dddd000000000000_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(22)=1 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x00400000)  ==
          0x00400000)
    return FORBIDDEN_OPERANDS;

  // inst(15:12)=1111 => UNPREDICTABLE
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_MRS_cccc00010r001111dddd000000000000_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {16
//         if inst(19:18)(1)=1
//         else 32},
//    safety: [inst(19:18)=00 => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16
  //       if inst(19:18)(1)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x000C0000) >> 18) & 0x00000002)  ==
          0x00000002
       ? 16
       : 32)));
}

SafetyLevel Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=00 => DECODER_ERROR
  if ((inst.Bits() & 0x000C0000)  ==
          0x00000000)
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1
//
// Actual:
//   {defs: {16
//         if inst(19:18)(1)=1
//         else 32},
//    safety: [15  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(19:18)=00 => UNPREDICTABLE],
//    uses: {inst(3:0)}}

RegisterList Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16
  //       if inst(19:18)(1)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x000C0000) >> 18) & 0x00000002)  ==
          0x00000002
       ? 16
       : 32)));
}

SafetyLevel Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=00 => UNPREDICTABLE
  if ((inst.Bits() & 0x000C0000)  ==
          0x00000000)
    return UNPREDICTABLE;

  // 15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000000F)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16), 16
//         if inst(20)=1
//         else 32},
//    safety: [(ArchVersion()  <
//            6 &&
//         inst(19:16)  ==
//            inst(3:0)) => UNPREDICTABLE,
//      15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(11:8), inst(3:0)}}

RegisterList Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  // (ArchVersion()  <
  //          6 &&
  //       inst(19:16)  ==
  //          inst(3:0)) => UNPREDICTABLE
  if (((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F))))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(11:8), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_NOP_cccc0011001000001111000000000000_case_1
//
// Actual:
//   {defs: {},
//    uses: {}}

RegisterList Actual_NOP_cccc0011001000001111000000000000_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_NOP_cccc0011001000001111000000000000_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList Actual_NOP_cccc0011001000001111000000000000_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_NOT_IMPLEMENTED_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => NOT_IMPLEMENTED],
//    uses: {}}

RegisterList Actual_NOT_IMPLEMENTED_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_NOT_IMPLEMENTED_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => NOT_IMPLEMENTED
  if (true)
    return NOT_IMPLEMENTED;

  return MAY_BE_SAFE;
}


RegisterList Actual_NOT_IMPLEMENTED_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    dynamic_code_replace_immediates: {inst(11:0)},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16)}}

RegisterList Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

Instruction Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1::
dynamic_code_replacement_sentinel(
     Instruction inst) const {
  if (!defs(inst).ContainsAny(RegisterList::DynCodeReplaceFrozenRegs())) {
    // inst(11:0)
    inst.SetBits(11, 0, 0);
  }
  return inst;
}

SafetyLevel Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE],
//    uses: {inst(19:16), inst(3:0)}}

RegisterList Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {},
//    is_literal_load: 15  ==
//            inst(19:16),
//    safety: [inst(19:16)=1111 => DECODER_ERROR],
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

bool Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: '15  ==
  //          inst(19:16)'
  return ((((inst.Bits() & 0x000F0000) >> 16)) == (15));
}

SafetyLevel Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
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


// Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {},
//    safety: [15  ==
//            inst(3:0) ||
//         (15  ==
//            inst(19:16) &&
//         inst(22)=1) => UNPREDICTABLE,
//      true => FORBIDDEN_OPERANDS],
//    uses: {inst(3:0), inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(3:0) ||
  //       (15  ==
  //          inst(19:16) &&
  //       inst(22)=1) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000000F)) == (15))) ||
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) &&
       ((inst.Bits() & 0x00400000)  ==
          0x00400000))))
    return UNPREDICTABLE;

  // true => FORBIDDEN_OPERANDS
  if (true)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1::
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


// Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1
//
// Actual:
//   {base: 15,
//    defs: {},
//    is_literal_load: true,
//    safety: [true => MAY_BE_SAFE],
//    uses: {15},
//    violations: [implied by 'base']}

Register Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

bool Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1::
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


// Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {},
//    is_literal_load: 15  ==
//            inst(19:16),
//    safety: [true => MAY_BE_SAFE],
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

bool Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: '15  ==
  //          inst(19:16)'
  return ((((inst.Bits() & 0x000F0000) >> 16)) == (15));
}

SafetyLevel Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1::
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


// Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {},
//    safety: [15  ==
//            inst(3:0) => UNPREDICTABLE,
//      true => FORBIDDEN_OPERANDS],
//    uses: {inst(3:0), inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000000F)) == (15)))
    return UNPREDICTABLE;

  // true => FORBIDDEN_OPERANDS
  if (true)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1::
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


// Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      31  <=
//            inst(11:7) + inst(20:16) => UNPREDICTABLE],
//    uses: {inst(3:0)}}

RegisterList Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // 31  <=
  //          inst(11:7) + inst(20:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x00000F80) >> 7) + ((inst.Bits() & 0x001F0000) >> 16)) > (31)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16)},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(11:8) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE],
//    uses: {inst(11:8), inst(3:0)}}

RegisterList Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(11:8) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(11:8), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16)},
//    safety: [15  ==
//            inst(15:12) => DECODER_ERROR,
//      15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8), inst(15:12)}}

RegisterList Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) => DECODER_ERROR
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return DECODER_ERROR;

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12), inst(19:16)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(15:12), inst(19:16), inst(3:0), inst(11:8)}}

RegisterList Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16), inst(15:12)},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(19:16), inst(15:12), inst(11:8), inst(3:0)}}

RegisterList Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16), inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(15:12), inst(11:8), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12), inst(19:16), 16
//         if inst(20)=1
//         else 32},
//    safety: [(ArchVersion()  <
//            6 &&
//         (inst(19:16)  ==
//            inst(3:0) ||
//         inst(15:12)  ==
//            inst(3:0))) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(15:12), inst(19:16), inst(3:0), inst(11:8)}}

RegisterList Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12))))
    return UNPREDICTABLE;

  // (ArchVersion()  <
  //          6 &&
  //       (inst(19:16)  ==
  //          inst(3:0) ||
  //       inst(15:12)  ==
  //          inst(3:0))) => UNPREDICTABLE
  if (((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F))))))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16)},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8)}}

RegisterList Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12), inst(19:16), 16
//         if inst(20)=1
//         else 32},
//    safety: [(ArchVersion()  <
//            6 &&
//         (inst(19:16)  ==
//            inst(3:0) ||
//         inst(15:12)  ==
//            inst(3:0))) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8)}}

RegisterList Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12))))
    return UNPREDICTABLE;

  // (ArchVersion()  <
  //          6 &&
  //       (inst(19:16)  ==
  //          inst(3:0) ||
  //       inst(15:12)  ==
  //          inst(3:0))) => UNPREDICTABLE
  if (((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F))))))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(19:16) ||
//         NumGPRs(RegisterList(inst(15:0)))  <
//            1 => UNPREDICTABLE,
//      inst(21)=1 &&
//         Contains(RegisterList(inst(15:0)), inst(19:16)) &&
//         SmallestGPR(RegisterList(inst(15:0)))  !=
//            inst(19:16) => UNKNOWN],
//    small_imm_base_wb: inst(21)=1,
//    uses: Union({inst(19:16)}, RegisterList(inst(15:0))),
//    violations: [implied by 'base']}

Register Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1::
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

SafetyLevel Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       NumGPRs(RegisterList(inst(15:0)))  <
  //          1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((nacl_arm_dec::NumGPRs(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)))) < (1))))
    return UNPREDICTABLE;

  // inst(21)=1 &&
  //       Contains(RegisterList(inst(15:0)), inst(19:16)) &&
  //       SmallestGPR(RegisterList(inst(15:0)))  !=
  //          inst(19:16) => UNKNOWN
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(((inst.Bits() & 0x000F0000) >> 16)))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) != (nacl_arm_dec::SmallestGPR(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)))))))
    return UNKNOWN;

  return MAY_BE_SAFE;
}


bool Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: 'Union({inst(19:16)}, RegisterList(inst(15:0)))'
  return nacl_arm_dec::Union(RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))), nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)));
}

ViolationSet Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1::
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


// Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE],
//    small_imm_base_wb: inst(24)=0 ||
//         inst(21)=1,
//    uses: {inst(19:16), inst(15:12)},
//    violations: [implied by 'base']}

Register Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(24)=0 ||
  //       inst(21)=1'
  return ((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000);
}

RegisterList Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

ViolationSet Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1::
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


// Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(3:0) ||
//         15  ==
//            inst(15:12) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         inst(24)=0 ||
//         inst(21)=1 &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(3:0), inst(19:16), inst(15:12)},
//    violations: [implied by 'base']}

Register Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(15:12) => UNPREDICTABLE
  if ((((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))))
    return UNPREDICTABLE;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

ViolationSet Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1::
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


// Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12) + 1  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) + 1 => UNPREDICTABLE,
//      inst(15:12)(0)=1 => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => UNPREDICTABLE],
//    small_imm_base_wb: (inst(24)=0) ||
//         (inst(21)=1),
//    uses: {inst(15:12), inst(15:12) + 1, inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)(0)=1 => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNPREDICTABLE;

  // inst(24)=0 &&
  //       inst(21)=1 => UNPREDICTABLE
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12) + 1  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12) + 1))))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) + 1 => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12) + 1) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(inst(24)=0) ||
  //       (inst(21)=1)'
  return (((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000));
}

RegisterList Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(15:12) + 1, inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1)).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1::
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


// Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12) + 1  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) + 1 ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         (inst(24)=0) ||
//         (inst(21)=1) &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(15:12)(0)=1 => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(15:12), inst(15:12) + 1, inst(19:16), inst(3:0)},
//    violations: [implied by 'base']}

Register Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)(0)=1 => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNPREDICTABLE;

  // inst(24)=0 &&
  //       inst(21)=1 => UNPREDICTABLE
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) + 1 ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12) + 1) == (15))) ||
       ((((inst.Bits() & 0x0000000F)) == (15))))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12) + 1  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12) + 1))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       ((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(15:12) + 1, inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1)).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1::
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


// Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(19:16) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(3:0) => UNPREDICTABLE],
//    uses: {inst(19:16), inst(3:0)},
//    violations: [implied by 'base']}

Register Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12)) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1::
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


// Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         inst(3:0)(0)=1 ||
//         14  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(3:0) ||
//         inst(15:12)  ==
//            inst(3:0) + 1 => UNPREDICTABLE],
//    uses: {inst(19:16), inst(3:0), inst(3:0) + 1},
//    violations: [implied by 'base']}

Register Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       inst(3:0)(0)=1 ||
  //       14  ==
  //          inst(3:0) => UNPREDICTABLE
  if (((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16))))) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x0000000F)) == (14))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(3:0) ||
  //       inst(15:12)  ==
  //          inst(3:0) + 1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12)) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F)))) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F) + 1))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(3:0) + 1}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register((inst.Bits() & 0x0000000F) + 1));
}

ViolationSet Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1::
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


// Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR],
//    small_imm_base_wb: (inst(24)=0) ||
//         (inst(21)=1),
//    uses: {inst(15:12), inst(19:16)},
//    violations: [implied by 'base']}

Register Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(inst(24)=0) ||
  //       (inst(21)=1)'
  return (((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000));
}

RegisterList Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1::
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


// Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         (inst(24)=0) ||
//         (inst(21)=1) &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(15:12), inst(19:16), inst(3:0)},
//    violations: [implied by 'base']}

Register Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       ((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1::
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


// Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE],
//    small_imm_base_wb: inst(24)=0 ||
//         inst(21)=1,
//    uses: {inst(19:16), inst(15:12)},
//    violations: [implied by 'base']}

Register Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(24)=0 ||
  //       inst(21)=1'
  return ((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000);
}

RegisterList Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

ViolationSet Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1::
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


// Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         inst(24)=0 ||
//         inst(21)=1 &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(3:0), inst(19:16), inst(15:12)},
//    violations: [implied by 'base']}

Register Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000000F)) == (15)))
    return UNPREDICTABLE;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

ViolationSet Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1::
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


// Actual_SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => DEPRECATED],
//    uses: {}}

RegisterList Actual_SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => DEPRECATED
  if (true)
    return DEPRECATED;

  return MAY_BE_SAFE;
}


RegisterList Actual_SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(19:16)=1111 => DECODER_ERROR],
//    uses: {inst(19:16), inst(3:0)}}

RegisterList Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {16},
//    sets_Z_if_clear_bits: RegIndex(test_register())  ==
//            inst(19:16) &&
//         (ARMExpandImm_C(inst(11:0)) &&
//         clears_mask())  ==
//            clears_mask(),
//    uses: {inst(19:16)}}

RegisterList Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16}'
  return RegisterList().
   Add(Register(16));
}

SafetyLevel Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


bool Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1::
sets_Z_if_bits_clear(
      Instruction inst, Register test_register,
      uint32_t clears_mask) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // sets_Z_if_clear_bits: 'RegIndex(test_register())  ==
  //          inst(19:16) &&
  //       (ARMExpandImm_C(inst(11:0)) &&
  //       clears_mask())  ==
  //          clears_mask()'
  return (((((inst.Bits() & 0x000F0000) >> 16)) == (nacl_arm_dec::RegIndex(test_register)))) &&
       (((((nacl_arm_dec::ARMExpandImm_C((inst.Bits() & 0x00000FFF)) & clears_mask))) == (clears_mask)));
}

RegisterList Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1
//
// Actual:
//   {defs: {},
//    safety: [not IsUDFNaClSafe(inst) => FORBIDDEN_OPERANDS],
//    uses: {}}

RegisterList Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // not IsUDFNaClSafe(inst) => FORBIDDEN_OPERANDS
  if (!(nacl_arm_dec::IsUDFNaClSafe(inst.Bits())))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => UNPREDICTABLE],
//    uses: {}}

RegisterList Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNPREDICTABLE
  if (true)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_Unnamed_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => UNDEFINED],
//    uses: {}}

RegisterList Actual_Unnamed_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_Unnamed_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNDEFINED
  if (true)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_Unnamed_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(15:12)(0)=1 => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR],
//    uses: {}}

RegisterList Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // inst(15:12)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)=11 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}

RegisterList Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1::
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

  // inst(21:20)=11 => UNDEFINED
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

}  // namespace nacl_arm_dec
