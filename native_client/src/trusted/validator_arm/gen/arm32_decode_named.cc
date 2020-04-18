/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif

#include "native_client/src/trusted/validator_arm/gen/arm32_decode_named_decoder.h"

using nacl_arm_dec::ClassDecoder;
using nacl_arm_dec::Instruction;

namespace nacl_arm_test {

NamedArm32DecoderState::NamedArm32DecoderState()
{}

/*
 * Implementation of table ARMv7.
 * Specified by: ('See Section A5.1',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_ARMv7(
     const nacl_arm_dec::Instruction inst) const {

  if ((inst.Bits() & 0xF0000000)  !=
          0xF0000000 /* cond(31:28)=~1111 */ &&
      (inst.Bits() & 0x0E000000)  ==
          0x04000000 /* op1(27:25)=010 */) {
    return decode_load_store_word_and_unsigned_byte(inst);
  }

  if ((inst.Bits() & 0xF0000000)  !=
          0xF0000000 /* cond(31:28)=~1111 */ &&
      (inst.Bits() & 0x0E000000)  ==
          0x06000000 /* op1(27:25)=011 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op(4)=0 */) {
    return decode_load_store_word_and_unsigned_byte(inst);
  }

  if ((inst.Bits() & 0xF0000000)  !=
          0xF0000000 /* cond(31:28)=~1111 */ &&
      (inst.Bits() & 0x0E000000)  ==
          0x06000000 /* op1(27:25)=011 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* op(4)=1 */) {
    return decode_media_instructions(inst);
  }

  if ((inst.Bits() & 0xF0000000)  !=
          0xF0000000 /* cond(31:28)=~1111 */ &&
      (inst.Bits() & 0x0C000000)  ==
          0x00000000 /* op1(27:25)=00x */) {
    return decode_data_processing_and_miscellaneous_instructions(inst);
  }

  if ((inst.Bits() & 0xF0000000)  !=
          0xF0000000 /* cond(31:28)=~1111 */ &&
      (inst.Bits() & 0x0C000000)  ==
          0x08000000 /* op1(27:25)=10x */) {
    return decode_branch_branch_with_link_and_block_data_transfer(inst);
  }

  if ((inst.Bits() & 0xF0000000)  !=
          0xF0000000 /* cond(31:28)=~1111 */ &&
      (inst.Bits() & 0x0C000000)  ==
          0x0C000000 /* op1(27:25)=11x */) {
    return decode_coprocessor_instructions_and_supervisor_call(inst);
  }

  if ((inst.Bits() & 0xF0000000)  ==
          0xF0000000 /* cond(31:28)=1111 */) {
    return decode_unconditional_instructions(inst);
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table advanced_simd_data_processing_instructions.
 * Specified by: ('See Section A7.4',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_advanced_simd_data_processing_instructions(
     const nacl_arm_dec::Instruction inst) const {

  if ((inst.Bits() & 0x00B80000)  ==
          0x00800000 /* A(23:19)=1x000 */ &&
      (inst.Bits() & 0x00000090)  ==
          0x00000010 /* C(7:4)=0xx1 */) {
    return decode_simd_dp_1imm(inst);
  }

  if ((inst.Bits() & 0x00B80000)  ==
          0x00880000 /* A(23:19)=1x001 */ &&
      (inst.Bits() & 0x00000090)  ==
          0x00000010 /* C(7:4)=0xx1 */) {
    return decode_simd_dp_2shift(inst);
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00900000 /* A(23:19)=1x01x */ &&
      (inst.Bits() & 0x00000090)  ==
          0x00000010 /* C(7:4)=0xx1 */) {
    return decode_simd_dp_2shift(inst);
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00A00000 /* A(23:19)=1x10x */ &&
      (inst.Bits() & 0x00000050)  ==
          0x00000000 /* C(7:4)=x0x0 */) {
    return decode_simd_dp_3diff(inst);
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00A00000 /* A(23:19)=1x10x */ &&
      (inst.Bits() & 0x00000050)  ==
          0x00000040 /* C(7:4)=x1x0 */) {
    return decode_simd_dp_2scalar(inst);
  }

  if ((inst.Bits() & 0x00A00000)  ==
          0x00800000 /* A(23:19)=1x0xx */ &&
      (inst.Bits() & 0x00000050)  ==
          0x00000000 /* C(7:4)=x0x0 */) {
    return decode_simd_dp_3diff(inst);
  }

  if ((inst.Bits() & 0x00A00000)  ==
          0x00800000 /* A(23:19)=1x0xx */ &&
      (inst.Bits() & 0x00000050)  ==
          0x00000040 /* C(7:4)=x1x0 */) {
    return decode_simd_dp_2scalar(inst);
  }

  if ((inst.Bits() & 0x00A00000)  ==
          0x00A00000 /* A(23:19)=1x1xx */ &&
      (inst.Bits() & 0x00000090)  ==
          0x00000010 /* C(7:4)=0xx1 */) {
    return decode_simd_dp_2shift(inst);
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23:19)=0xxxx */) {
    return decode_simd_dp_3same(inst);
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23:19)=1xxxx */ &&
      (inst.Bits() & 0x00000090)  ==
          0x00000090 /* C(7:4)=1xx1 */) {
    return decode_simd_dp_2shift(inst);
  }

  if ((inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00B00000)  ==
          0x00B00000 /* A(23:19)=1x11x */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* C(7:4)=xxx0 */) {
    return VEXT_111100101d11nnnnddddiiiinqm0mmmm_case_0_VEXT_instance_;
  }

  if ((inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00B00000)  ==
          0x00B00000 /* A(23:19)=1x11x */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000C00 /* B(11:8)=1100 */ &&
      (inst.Bits() & 0x00000090)  ==
          0x00000000 /* C(7:4)=0xx0 */) {
    return VDUP_scalar_111100111d11iiiidddd11000qm0mmmm_case_0_VDUP_scalar_instance_;
  }

  if ((inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00B00000)  ==
          0x00B00000 /* A(23:19)=1x11x */ &&
      (inst.Bits() & 0x00000C00)  ==
          0x00000800 /* B(11:8)=10xx */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* C(7:4)=xxx0 */) {
    return VTBL_VTBX_111100111d11nnnndddd10ccnpm0mmmm_case_0_VTBL_VTBX_instance_;
  }

  if ((inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00B00000)  ==
          0x00B00000 /* A(23:19)=1x11x */ &&
      (inst.Bits() & 0x00000800)  ==
          0x00000000 /* B(11:8)=0xxx */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* C(7:4)=xxx0 */) {
    return decode_simd_dp_2misc(inst);
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table advanced_simd_element_or_structure_load_store_instructions.
 * Specified by: ('See Section A7.7',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_advanced_simd_element_or_structure_load_store_instructions(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000300 /* B(11:8)=0011 */) {
    return VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0_VST2_multiple_2_element_structures_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000700)  ==
          0x00000200 /* B(11:8)=x010 */) {
    return VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0_VST1_multiple_single_elements_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000000 /* B(11:8)=000x */) {
    return VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0_VST4_multiple_4_element_structures_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000400 /* B(11:8)=010x */) {
    return VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0_VST3_multiple_3_element_structures_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000600 /* B(11:8)=011x */) {
    return VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0_VST1_multiple_single_elements_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000800 /* B(11:8)=100x */) {
    return VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0_VST2_multiple_2_element_structures_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000800 /* B(11:8)=1000 */) {
    return VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0_VST1_single_element_from_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000900 /* B(11:8)=1001 */) {
    return VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0_VST2_single_2_element_structure_from_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000A00 /* B(11:8)=1010 */) {
    return VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0_VST3_single_3_element_structure_from_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000B00 /* B(11:8)=1011 */) {
    return VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0_VST4_single_4_element_structure_form_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000000 /* B(11:8)=0x00 */) {
    return VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0_VST1_single_element_from_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000100 /* B(11:8)=0x01 */) {
    return VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0_VST2_single_2_element_structure_from_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000200 /* B(11:8)=0x10 */) {
    return VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0_VST3_single_3_element_structure_from_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00000000 /* L(21)=0 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000300 /* B(11:8)=0x11 */) {
    return VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0_VST4_single_4_element_structure_form_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000300 /* B(11:8)=0011 */) {
    return VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0_VLD2_multiple_2_element_structures_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000700)  ==
          0x00000200 /* B(11:8)=x010 */) {
    return VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0_VLD1_multiple_single_elements_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000000 /* B(11:8)=000x */) {
    return VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0_VLD4_multiple_4_element_structures_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000400 /* B(11:8)=010x */) {
    return VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0_VLD3_multiple_3_element_structures_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000600 /* B(11:8)=011x */) {
    return VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0_VLD1_multiple_single_elements_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000800 /* B(11:8)=100x */) {
    return VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0_VLD2_multiple_2_element_structures_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000800 /* B(11:8)=1000 */) {
    return VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0_VLD1_single_element_to_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000900 /* B(11:8)=1001 */) {
    return VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0_VLD2_single_2_element_structure_to_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000A00 /* B(11:8)=1010 */) {
    return VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0_VLD3_single_3_element_structure_to_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000B00 /* B(11:8)=1011 */) {
    return VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0_VLD4_single_4_element_structure_to_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000C00 /* B(11:8)=1100 */) {
    return VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0_VLD1_single_element_to_all_lanes_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* B(11:8)=1101 */) {
    return VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0_VLD2_single_2_element_structure_to_all_lanes_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* B(11:8)=1110 */) {
    return VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0_VLD3_single_3_element_structure_to_all_lanes_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* B(11:8)=1111 */) {
    return VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0_VLD4_single_4_element_structure_to_all_lanes_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000000 /* B(11:8)=0x00 */) {
    return VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0_VLD1_single_element_to_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000100 /* B(11:8)=0x01 */) {
    return VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0_VLD2_single_2_element_structure_to_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000200 /* B(11:8)=0x10 */) {
    return VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0_VLD3_single_3_element_structure_to_one_lane_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000300 /* B(11:8)=0x11 */) {
    return VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0_VLD4_single_4_element_structure_to_one_lane_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table branch_branch_with_link_and_block_data_transfer.
 * Specified by: ('See Section A5.5',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_branch_branch_with_link_and_block_data_transfer(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x03D00000)  ==
          0x00000000 /* op(25:20)=0000x0 */) {
    return STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0_STMDA_STMED_instance_;
  }

  if ((inst.Bits() & 0x03D00000)  ==
          0x00100000 /* op(25:20)=0000x1 */) {
    return LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0_LDMDA_LDMFA_instance_;
  }

  if ((inst.Bits() & 0x03D00000)  ==
          0x00800000 /* op(25:20)=0010x0 */) {
    return STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0_STM_STMIA_STMEA_instance_;
  }

  if ((inst.Bits() & 0x03D00000)  ==
          0x00900000 /* op(25:20)=0010x1 */) {
    return LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0_LDM_LDMIA_LDMFD_instance_;
  }

  if ((inst.Bits() & 0x03D00000)  ==
          0x01000000 /* op(25:20)=0100x0 */) {
    return STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0_STMDB_STMFD_instance_;
  }

  if ((inst.Bits() & 0x03D00000)  ==
          0x01100000 /* op(25:20)=0100x1 */) {
    return LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0_LDMDB_LDMEA_instance_;
  }

  if ((inst.Bits() & 0x03D00000)  ==
          0x01800000 /* op(25:20)=0110x0 */) {
    return STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0_STMIB_STMFA_instance_;
  }

  if ((inst.Bits() & 0x03D00000)  ==
          0x01900000 /* op(25:20)=0110x1 */) {
    return LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0_LDMIB_LDMED_instance_;
  }

  if ((inst.Bits() & 0x02500000)  ==
          0x00400000 /* op(25:20)=0xx1x0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx */) {
    return STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0_STM_User_registers_instance_;
  }

  if ((inst.Bits() & 0x02500000)  ==
          0x00500000 /* op(25:20)=0xx1x1 */ &&
      (inst.Bits() & 0x00008000)  ==
          0x00000000 /* R(15)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx */) {
    return LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0_LDM_User_registers_instance_;
  }

  if ((inst.Bits() & 0x02500000)  ==
          0x00500000 /* op(25:20)=0xx1x1 */ &&
      (inst.Bits() & 0x00008000)  ==
          0x00008000 /* R(15)=1 */) {
    return LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0_LDM_exception_return_instance_;
  }

  if ((inst.Bits() & 0x03000000)  ==
          0x02000000 /* op(25:20)=10xxxx */) {
    return B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0_B_instance_;
  }

  if ((inst.Bits() & 0x03000000)  ==
          0x03000000 /* op(25:20)=11xxxx */) {
    return BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0_BL_BLX_immediate_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table coprocessor_instructions_and_supervisor_call.
 * Specified by: ('See Section A5.6',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_coprocessor_instructions_and_supervisor_call(
     const nacl_arm_dec::Instruction inst) const {

  if ((inst.Bits() & 0x03E00000)  ==
          0x00000000 /* op1(25:20)=00000x */) {
    return Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x03F00000)  ==
          0x00400000 /* op1(25:20)=000100 */) {
    return MCRR_cccc11000100ttttttttccccoooommmm_case_0_MCRR_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x03F00000)  ==
          0x00500000 /* op1(25:20)=000101 */) {
    return MRRC_cccc11000101ttttttttccccoooommmm_case_0_MRRC_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x03100000)  ==
          0x02000000 /* op1(25:20)=10xxx0 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* op(4)=1 */) {
    return MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0_MCR_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x03100000)  ==
          0x02100000 /* op1(25:20)=10xxx1 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* op(4)=1 */) {
    return MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0_MRC_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x02100000)  ==
          0x00000000 /* op1(25:20)=0xxxx0 */ &&
      (inst.Bits() & 0x03B00000)  !=
          0x00000000 /* op1_repeated(25:20)=~000x00 */) {
    return STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0_STC_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x02100000)  ==
          0x00100000 /* op1(25:20)=0xxxx1 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */ &&
      (inst.Bits() & 0x03B00000)  !=
          0x00100000 /* op1_repeated(25:20)=~000x01 */) {
    return LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0_LDC_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x02100000)  ==
          0x00100000 /* op1(25:20)=0xxxx1 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */ &&
      (inst.Bits() & 0x03B00000)  !=
          0x00100000 /* op1_repeated(25:20)=~000x01 */) {
    return LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0_LDC_literal_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x03000000)  ==
          0x02000000 /* op1(25:20)=10xxxx */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op(4)=0 */) {
    return CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0_CDP_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00 /* coproc(11:8)=101x */ &&
      (inst.Bits() & 0x03E00000)  ==
          0x00400000 /* op1(25:20)=00010x */) {
    return decode_transfer_between_arm_core_and_extension_registers_64_bit(inst);
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00 /* coproc(11:8)=101x */ &&
      (inst.Bits() & 0x03000000)  ==
          0x02000000 /* op1(25:20)=10xxxx */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op(4)=0 */) {
    return decode_floating_point_data_processing_instructions(inst);
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00 /* coproc(11:8)=101x */ &&
      (inst.Bits() & 0x03000000)  ==
          0x02000000 /* op1(25:20)=10xxxx */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* op(4)=1 */) {
    return decode_transfer_between_arm_core_and_extension_register_8_16_and_32_bit(inst);
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000A00 /* coproc(11:8)=101x */ &&
      (inst.Bits() & 0x02000000)  ==
          0x00000000 /* op1(25:20)=0xxxxx */ &&
      (inst.Bits() & 0x03A00000)  !=
          0x00000000 /* op1_repeated(25:20)=~000x0x */) {
    return decode_extension_register_load_store_instructions(inst);
  }

  if ((inst.Bits() & 0x03000000)  ==
          0x03000000 /* op1(25:20)=11xxxx */) {
    return SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0_SVC_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table data_processing_and_miscellaneous_instructions.
 * Specified by: ('See Section A5.2',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_data_processing_and_miscellaneous_instructions(
     const nacl_arm_dec::Instruction inst) const {

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* op(25)=0 */ &&
      (inst.Bits() & 0x01900000)  !=
          0x01000000 /* op1(24:20)=~10xx0 */ &&
      (inst.Bits() & 0x00000090)  ==
          0x00000010 /* op2(7:4)=0xx1 */) {
    return decode_data_processing_register_shifted_register(inst);
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* op(25)=0 */ &&
      (inst.Bits() & 0x01900000)  !=
          0x01000000 /* op1(24:20)=~10xx0 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op2(7:4)=xxx0 */) {
    return decode_data_processing_register(inst);
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* op(25)=0 */ &&
      (inst.Bits() & 0x01900000)  ==
          0x01000000 /* op1(24:20)=10xx0 */ &&
      (inst.Bits() & 0x00000090)  ==
          0x00000080 /* op2(7:4)=1xx0 */) {
    return decode_halfword_multiply_and_multiply_accumulate(inst);
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* op(25)=0 */ &&
      (inst.Bits() & 0x01900000)  ==
          0x01000000 /* op1(24:20)=10xx0 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* op2(7:4)=0xxx */) {
    return decode_miscellaneous_instructions(inst);
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* op(25)=0 */ &&
      (inst.Bits() & 0x01200000)  !=
          0x00200000 /* op1(24:20)=~0xx1x */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x000000B0 /* op2(7:4)=1011 */) {
    return decode_extra_load_store_instructions(inst);
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* op(25)=0 */ &&
      (inst.Bits() & 0x01200000)  !=
          0x00200000 /* op1(24:20)=~0xx1x */ &&
      (inst.Bits() & 0x000000D0)  ==
          0x000000D0 /* op2(7:4)=11x1 */) {
    return decode_extra_load_store_instructions(inst);
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* op(25)=0 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x00200000 /* op1(24:20)=0xx1x */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x000000B0 /* op2(7:4)=1011 */) {
    return extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0_extra_load_store_instructions_unpriviledged_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* op(25)=0 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x00200000 /* op1(24:20)=0xx1x */ &&
      (inst.Bits() & 0x000000D0)  ==
          0x000000D0 /* op2(7:4)=11x1 */) {
    return extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0_extra_load_store_instructions_unpriviledged_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* op(25)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* op1(24:20)=0xxxx */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000090 /* op2(7:4)=1001 */) {
    return decode_multiply_and_multiply_accumulate(inst);
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* op(25)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* op1(24:20)=1xxxx */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000090 /* op2(7:4)=1001 */) {
    return decode_synchronization_primitives(inst);
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* op(25)=1 */ &&
      (inst.Bits() & 0x01F00000)  ==
          0x01000000 /* op1(24:20)=10000 */) {
    return MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0_MOVW_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* op(25)=1 */ &&
      (inst.Bits() & 0x01F00000)  ==
          0x01400000 /* op1(24:20)=10100 */) {
    return MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0_MOVT_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* op(25)=1 */ &&
      (inst.Bits() & 0x01B00000)  ==
          0x01200000 /* op1(24:20)=10x10 */) {
    return decode_msr_immediate_and_hints(inst);
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* op(25)=1 */ &&
      (inst.Bits() & 0x01900000)  !=
          0x01000000 /* op1(24:20)=~10xx0 */) {
    return decode_data_processing_immediate(inst);
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table data_processing_immediate.
 * Specified by: ('See Section A5.2.3',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_data_processing_immediate(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x01F00000)  ==
          0x01100000 /* op(24:20)=10001 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0_TST_immediate_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01300000 /* op(24:20)=10011 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0_TEQ_immediate_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01500000 /* op(24:20)=10101 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0_CMP_immediate_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01700000 /* op(24:20)=10111 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0_CMN_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00000000 /* op(24:20)=0000x */) {
    return AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0_AND_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00200000 /* op(24:20)=0001x */) {
    return EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0_EOR_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00400000 /* op(24:20)=0010x */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */) {
    return SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0_SUB_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00400000 /* op(24:20)=0010x */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */) {
    return ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0_ADR_A2_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00600000 /* op(24:20)=0011x */) {
    return RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0_RSB_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00800000 /* op(24:20)=0100x */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */) {
    return ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0_ADD_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00800000 /* op(24:20)=0100x */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */) {
    return ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0_ADR_A1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00A00000 /* op(24:20)=0101x */) {
    return ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0_ADC_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00C00000 /* op(24:20)=0110x */) {
    return SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0_SBC_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00E00000 /* op(24:20)=0111x */) {
    return RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0_RSC_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01800000 /* op(24:20)=1100x */) {
    return ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0_ORR_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op(24:20)=1101x */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0_MOV_immediate_A1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01C00000 /* op(24:20)=1110x */) {
    return BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0_BIC_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01E00000 /* op(24:20)=1111x */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0_MVN_immediate_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table data_processing_register.
 * Specified by: ('See Section A5.2.1',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_data_processing_register(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x01F00000)  ==
          0x01100000 /* op1(24:20)=10001 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return TST_register_cccc00010001nnnn0000iiiiitt0mmmm_case_0_TST_register_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01300000 /* op1(24:20)=10011 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return TEQ_register_cccc00010011nnnn0000iiiiitt0mmmm_case_0_TEQ_register_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01500000 /* op1(24:20)=10101 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return CMP_register_cccc00010101nnnn0000iiiiitt0mmmm_case_0_CMP_register_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01700000 /* op1(24:20)=10111 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_0_CMN_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00000000 /* op1(24:20)=0000x */) {
    return AND_register_cccc0000000snnnnddddiiiiitt0mmmm_case_0_AND_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00200000 /* op1(24:20)=0001x */) {
    return EOR_register_cccc0000001snnnnddddiiiiitt0mmmm_case_0_EOR_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00400000 /* op1(24:20)=0010x */) {
    return SUB_register_cccc0000010snnnnddddiiiiitt0mmmm_case_0_SUB_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00600000 /* op1(24:20)=0011x */) {
    return RSB_register_cccc0000011snnnnddddiiiiitt0mmmm_case_0_RSB_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00800000 /* op1(24:20)=0100x */) {
    return ADD_register_cccc0000100snnnnddddiiiiitt0mmmm_case_0_ADD_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00A00000 /* op1(24:20)=0101x */) {
    return ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_0_ADC_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00C00000 /* op1(24:20)=0110x */) {
    return SBC_register_cccc0000110snnnnddddiiiiitt0mmmm_case_0_SBC_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00E00000 /* op1(24:20)=0111x */) {
    return RSC_register_cccc0000111snnnnddddiiiiitt0mmmm_case_0_RSC_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01800000 /* op1(24:20)=1100x */) {
    return ORR_register_cccc0001100snnnnddddiiiiitt0mmmm_case_0_ORR_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000F80)  !=
          0x00000000 /* op2(11:7)=~00000 */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000000 /* op3(6:5)=00 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_0_LSL_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000F80)  !=
          0x00000000 /* op2(11:7)=~00000 */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000060 /* op3(6:5)=11 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return ROR_immediate_cccc0001101s0000ddddiiiii110mmmm_case_0_ROR_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000F80)  ==
          0x00000000 /* op2(11:7)=00000 */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000000 /* op3(6:5)=00 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return MOV_register_cccc0001101s0000dddd00000000mmmm_case_0_MOV_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000F80)  ==
          0x00000000 /* op2(11:7)=00000 */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000060 /* op3(6:5)=11 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return RRX_cccc0001101s0000dddd00000110mmmm_case_0_RRX_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000020 /* op3(6:5)=01 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return LSR_immediate_cccc0001101s0000ddddiiiii010mmmm_case_0_LSR_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000040 /* op3(6:5)=10 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_0_ASR_immediate_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01C00000 /* op1(24:20)=1110x */) {
    return BIC_register_cccc0001110snnnnddddiiiiitt0mmmm_case_0_BIC_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01E00000 /* op1(24:20)=1111x */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return MVN_register_cccc0001111s0000ddddiiiiitt0mmmm_case_0_MVN_register_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table data_processing_register_shifted_register.
 * Specified by: ('See Section A5.2.2',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_data_processing_register_shifted_register(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x01F00000)  ==
          0x01100000 /* op1(24:20)=10001 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0_TST_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01300000 /* op1(24:20)=10011 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0_TEQ_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01500000 /* op1(24:20)=10101 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0_CMP_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01700000 /* op1(24:20)=10111 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0_CMN_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00000000 /* op1(24:20)=0000x */) {
    return AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0_AND_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00200000 /* op1(24:20)=0001x */) {
    return EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0_EOR_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00400000 /* op1(24:20)=0010x */) {
    return SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0_SUB_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00600000 /* op1(24:20)=0011x */) {
    return RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0_RSB_register_shfited_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00800000 /* op1(24:20)=0100x */) {
    return ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0_ADD_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00A00000 /* op1(24:20)=0101x */) {
    return ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0_ADC_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00C00000 /* op1(24:20)=0110x */) {
    return SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0_SBC_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00E00000 /* op1(24:20)=0111x */) {
    return RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0_RSC_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01800000 /* op1(24:20)=1100x */) {
    return ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0_ORR_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000000 /* op2(6:5)=00 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0_LSL_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000020 /* op2(6:5)=01 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0_LSR_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0_ASR_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000060 /* op2(6:5)=11 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0_ROR_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01C00000 /* op1(24:20)=1110x */) {
    return BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0_BIC_register_shifted_register_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01E00000 /* op1(24:20)=1111x */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0_MVN_register_shifted_register_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table extension_register_load_store_instructions.
 * Specified by: ('A7.6',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_extension_register_load_store_instructions(
     const nacl_arm_dec::Instruction inst) const {

  if ((inst.Bits() & 0x01B00000)  ==
          0x00900000 /* opcode(24:20)=01x01 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0_VLDM_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x00900000 /* opcode(24:20)=01x01 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0_VLDM_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x00B00000 /* opcode(24:20)=01x11 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000D0000 /* Rn(19:16)=~1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0_VLDM_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x00B00000 /* opcode(24:20)=01x11 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000D0000 /* Rn(19:16)=~1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0_VLDM_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x00B00000 /* opcode(24:20)=01x11 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000D0000 /* Rn(19:16)=1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return VPOP_cccc11001d111101dddd1010iiiiiiii_case_0_VPOP_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x00B00000 /* opcode(24:20)=01x11 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000D0000 /* Rn(19:16)=1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return VPOP_cccc11001d111101dddd1011iiiiiiii_case_0_VPOP_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01200000 /* opcode(24:20)=10x10 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000D0000 /* Rn(19:16)=~1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0_VSTM_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01200000 /* opcode(24:20)=10x10 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000D0000 /* Rn(19:16)=~1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0_VSTM_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01200000 /* opcode(24:20)=10x10 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000D0000 /* Rn(19:16)=1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0_VPUSH_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01200000 /* opcode(24:20)=10x10 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000D0000 /* Rn(19:16)=1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0_VPUSH_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01300000 /* opcode(24:20)=10x11 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0_VLDM_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01300000 /* opcode(24:20)=10x11 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0_VLDM_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00400000 /* opcode(24:20)=0010x */) {
    return decode_transfer_between_arm_core_and_extension_registers_64_bit(inst);
  }

  if ((inst.Bits() & 0x01300000)  ==
          0x01000000 /* opcode(24:20)=1xx00 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0_VSTR_instance_;
  }

  if ((inst.Bits() & 0x01300000)  ==
          0x01000000 /* opcode(24:20)=1xx00 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0_VSTR_instance_;
  }

  if ((inst.Bits() & 0x01300000)  ==
          0x01100000 /* opcode(24:20)=1xx01 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0_VLDR_instance_;
  }

  if ((inst.Bits() & 0x01300000)  ==
          0x01100000 /* opcode(24:20)=1xx01 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0_VLDR_instance_;
  }

  if ((inst.Bits() & 0x01900000)  ==
          0x00800000 /* opcode(24:20)=01xx0 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0_VSTM_instance_;
  }

  if ((inst.Bits() & 0x01900000)  ==
          0x00800000 /* opcode(24:20)=01xx0 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0_VSTM_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table extra_load_store_instructions.
 * Specified by: ('See Section A5.2.8',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_extra_load_store_instructions(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000060)  ==
          0x00000020 /* op2(6:5)=01 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00000000 /* op1(24:20)=xx0x0 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0_STRH_register_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000020 /* op2(6:5)=01 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00100000 /* op1(24:20)=xx0x1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0_LDRH_register_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000020 /* op2(6:5)=01 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */) {
    return STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0_STRH_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000020 /* op2(6:5)=01 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */) {
    return LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0_LDRH_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000020 /* op2(6:5)=01 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */) {
    return LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0_LDRH_literal_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00000000 /* op1(24:20)=xx0x0 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0_LDRD_register_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00100000 /* op1(24:20)=xx0x1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0_LDRSB_register_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */) {
    return LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0_LDRD_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x01000000 /* $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx */) {
    return LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0_LDRD_literal_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */) {
    return LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0_LDRSB_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x01000000 /* $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx */) {
    return LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0_LDRSB_literal_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000060 /* op2(6:5)=11 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00000000 /* op1(24:20)=xx0x0 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0_STRD_register_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000060 /* op2(6:5)=11 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00100000 /* op1(24:20)=xx0x1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0_LDRSH_register_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000060 /* op2(6:5)=11 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */) {
    return STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0_STRD_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000060 /* op2(6:5)=11 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */) {
    return LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0_LDRSH_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000060 /* op2(6:5)=11 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x01000000 /* $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx */) {
    return LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0_LDRSH_literal_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table floating_point_data_processing_instructions.
 * Specified by: ('A7.5 Table A7 - 16',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_floating_point_data_processing_instructions(
     const nacl_arm_dec::Instruction inst) const {

  if ((inst.Bits() & 0x00B00000)  ==
          0x00000000 /* opc1(23:20)=0x00 */) {
    return VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0_VMLA_VMLS_floating_point_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00100000 /* opc1(23:20)=0x01 */) {
    return VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0_VNMLA_VNMLS_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00200000 /* opc1(23:20)=0x10 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* opc3(7:6)=x0 */) {
    return VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0_VMUL_floating_point_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00200000 /* opc1(23:20)=0x10 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */) {
    return VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0_VNMUL_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00300000 /* opc1(23:20)=0x11 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* opc3(7:6)=x0 */) {
    return VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0_VADD_floating_point_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00300000 /* opc1(23:20)=0x11 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */) {
    return VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0_VSUB_floating_point_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00800000 /* opc1(23:20)=1x00 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* opc3(7:6)=x0 */) {
    return VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0_VDIV_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00900000 /* opc1(23:20)=1x01 */) {
    return VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0_VFNMA_VFNMS_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00A00000 /* opc1(23:20)=1x10 */) {
    return VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0_VFMA_VFMS_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00B00000 /* opc1(23:20)=1x11 */) {
    return decode_other_floating_point_data_processing_instructions(inst);
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table halfword_multiply_and_multiply_accumulate.
 * Specified by: ('See Section A5.2.7',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_halfword_multiply_and_multiply_accumulate(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00600000)  ==
          0x00000000 /* op1(22:21)=00 */) {
    return SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0_SMLABB_SMLABT_SMLATB_SMLATT_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00200000 /* op1(22:21)=01 */ &&
      (inst.Bits() & 0x00000020)  ==
          0x00000000 /* op(5)=0 */) {
    return SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0_SMLAWB_SMLAWT_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00200000 /* op1(22:21)=01 */ &&
      (inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0_SMULWB_SMULWT_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00400000 /* op1(22:21)=10 */) {
    return SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0_SMLALBB_SMLALBT_SMLALTB_SMLALTT_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00600000 /* op1(22:21)=11 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0_SMULBB_SMULBT_SMULTB_SMULTT_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table load_store_word_and_unsigned_byte.
 * Specified by: ('See Section A5.3',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_load_store_word_and_unsigned_byte(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x01700000)  ==
          0x00200000 /* op1(24:20)=0x010 */) {
    return STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0_STRT_A1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x01700000)  ==
          0x00300000 /* op1(24:20)=0x011 */) {
    return LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0_LDRT_A1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x01700000)  ==
          0x00600000 /* op1(24:20)=0x110 */) {
    return STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0_STRBT_A1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x01700000)  ==
          0x00700000 /* op1(24:20)=0x111 */) {
    return LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0_LDRBT_A1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00000000 /* op1(24:20)=xx0x0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00200000 /* op1_repeated(24:20)=~0x010 */) {
    return STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0_STR_immediate_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00100000 /* op1(24:20)=xx0x1 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00300000 /* op1_repeated(24:20)=~0x011 */) {
    return LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0_LDR_immediate_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00100000 /* op1(24:20)=xx0x1 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00300000 /* op1_repeated(24:20)=~0x011 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x01000000 /* $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx */) {
    return LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0_LDR_literal_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00600000 /* op1_repeated(24:20)=~0x110 */) {
    return STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0_STRB_immediate_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00700000 /* op1_repeated(24:20)=~0x111 */) {
    return LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0_LDRB_immediate_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00700000 /* op1_repeated(24:20)=~0x111 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x01000000 /* $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx */) {
    return LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0_LDRB_literal_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x01700000)  ==
          0x00200000 /* op1(24:20)=0x010 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0_STRT_A2_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x01700000)  ==
          0x00300000 /* op1(24:20)=0x011 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0_LDRT_A2_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x01700000)  ==
          0x00600000 /* op1(24:20)=0x110 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0_STRBT_A2_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x01700000)  ==
          0x00700000 /* op1(24:20)=0x111 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0_LDRBT_A2_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00000000 /* op1(24:20)=xx0x0 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00200000 /* op1_repeated(24:20)=~0x010 */) {
    return STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0_STR_register_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00100000 /* op1(24:20)=xx0x1 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00300000 /* op1_repeated(24:20)=~0x011 */) {
    return LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0_LDR_register_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00600000 /* op1_repeated(24:20)=~0x110 */) {
    return STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0_STRB_register_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00700000 /* op1_repeated(24:20)=~0x111 */) {
    return LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0_LDRB_register_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table media_instructions.
 * Specified by: ('See Section A5.4',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_media_instructions(
     const nacl_arm_dec::Instruction inst) const {

  if ((inst.Bits() & 0x01F00000)  ==
          0x01800000 /* op1(24:20)=11000 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x0000F000)  !=
          0x0000F000 /* Rd(15:12)=~1111 */) {
    return USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0_USADA8_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01800000 /* op1(24:20)=11000 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* Rd(15:12)=1111 */) {
    return USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0_USAD8_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01F00000 /* op1(24:20)=11111 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */) {
    return UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0_UDF_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(7:5)=x10 */) {
    return SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0_SBFX_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01C00000 /* op1(24:20)=1110x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000000 /* op2(7:5)=x00 */ &&
      (inst.Bits() & 0x0000000F)  !=
          0x0000000F /* Rn(3:0)=~1111 */) {
    return BFI_cccc0111110mmmmmddddlllll001nnnn_case_0_BFI_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01C00000 /* op1(24:20)=1110x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000000 /* op2(7:5)=x00 */ &&
      (inst.Bits() & 0x0000000F)  ==
          0x0000000F /* Rn(3:0)=1111 */) {
    return BFC_cccc0111110mmmmmddddlllll0011111_case_0_BFC_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01E00000 /* op1(24:20)=1111x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(7:5)=x10 */) {
    return UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0_UBFX_instance_;
  }

  if ((inst.Bits() & 0x01C00000)  ==
          0x00000000 /* op1(24:20)=000xx */) {
    return decode_parallel_addition_and_subtraction_signed(inst);
  }

  if ((inst.Bits() & 0x01C00000)  ==
          0x00400000 /* op1(24:20)=001xx */) {
    return decode_parallel_addition_and_subtraction_unsigned(inst);
  }

  if ((inst.Bits() & 0x01800000)  ==
          0x00800000 /* op1(24:20)=01xxx */) {
    return decode_packing_unpacking_saturation_and_reversal(inst);
  }

  if ((inst.Bits() & 0x01800000)  ==
          0x01000000 /* op1(24:20)=10xxx */) {
    return decode_signed_multiply_signed_and_unsigned_divide(inst);
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table memory_hints_advanced_simd_instructions_and_miscellaneous_instructions.
 * Specified by: ('See Section A5.7.1',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_memory_hints_advanced_simd_instructions_and_miscellaneous_instructions(
     const nacl_arm_dec::Instruction inst) const {

  if ((inst.Bits() & 0x07F00000)  ==
          0x01000000 /* op1(26:20)=0010000 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000000 /* op2(7:4)=0000 */ &&
      (inst.Bits() & 0x00010000)  ==
          0x00010000 /* Rn(19:16)=xxx1 */ &&
      (inst.Bits() & 0x000EFD0F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx000x000000x0xxxx0000 */) {
    return SETEND_1111000100000001000000i000000000_case_0_SETEND_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x01000000 /* op1(26:20)=0010000 */ &&
      (inst.Bits() & 0x00000020)  ==
          0x00000000 /* op2(7:4)=xx0x */ &&
      (inst.Bits() & 0x00010000)  ==
          0x00000000 /* Rn(19:16)=xxx0 */ &&
      (inst.Bits() & 0x0000FE00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000000xxxxxxxxx */) {
    return CPS_111100010000iii00000000iii0iiiii_case_0_CPS_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05300000 /* op1(26:20)=1010011 */) {
    return Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000000 /* op2(7:4)=0000 */) {
    return Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000010 /* op2(7:4)=0001 */ &&
      (inst.Bits() & 0x000FFF0F)  ==
          0x000FF00F /* $pattern(31:0)=xxxxxxxxxxxx111111110000xxxx1111 */) {
    return CLREX_11110101011111111111000000011111_case_0_CLREX_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000040 /* op2(7:4)=0100 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x000FF000 /* $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx */) {
    return DSB_1111010101111111111100000100xxxx_case_0_DSB_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000050 /* op2(7:4)=0101 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x000FF000 /* $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx */) {
    return DMB_1111010101111111111100000101xxxx_case_0_DMB_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000060 /* op2(7:4)=0110 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x000FF000 /* $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx */) {
    return ISB_1111010101111111111100000110xxxx_case_0_ISB_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000070 /* op2(7:4)=0111 */) {
    return Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:4)=001x */) {
    return Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000080 /* op2(7:4)=1xxx */) {
    return Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x04100000 /* op1(26:20)=100x001 */) {
    return Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x04500000 /* op1(26:20)=100x101 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0_PLI_immediate_literal_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x05100000 /* op1(26:20)=101x001 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0_PLD_PLDW_immediate_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x05100000 /* op1(26:20)=101x001 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */) {
    return Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x05500000 /* op1(26:20)=101x101 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1_PLD_PLDW_immediate_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x05500000 /* op1(26:20)=101x101 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0_PLD_literal_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x06100000 /* op1(26:20)=110x001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op2(7:4)=xxx0 */) {
    return Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x06500000 /* op1(26:20)=110x101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op2(7:4)=xxx0 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0_PLI_register_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x07100000 /* op1(26:20)=111x001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op2(7:4)=xxx0 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0_PLD_PLDW_register_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x07500000 /* op1(26:20)=111x101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op2(7:4)=xxx0 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0_PLD_PLDW_register_instance_;
  }

  if ((inst.Bits() & 0x07B00000)  ==
          0x05B00000 /* op1(26:20)=1011x11 */) {
    return Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x07300000)  ==
          0x04300000 /* op1(26:20)=100xx11 */) {
    return Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x06300000)  ==
          0x06300000 /* op1(26:20)=11xxx11 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op2(7:4)=xxx0 */) {
    return Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x07100000)  ==
          0x04000000 /* op1(26:20)=100xxx0 */) {
    return decode_advanced_simd_element_or_structure_load_store_instructions(inst);
  }

  if ((inst.Bits() & 0x06000000)  ==
          0x02000000 /* op1(26:20)=01xxxxx */) {
    return decode_advanced_simd_data_processing_instructions(inst);
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table miscellaneous_instructions.
 * Specified by: ('See Section A5.2.12',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_miscellaneous_instructions(
     const nacl_arm_dec::Instruction inst) const {

  if ((inst.Bits() & 0x00000070)  ==
          0x00000000 /* op2(6:4)=000 */ &&
      (inst.Bits() & 0x00000200)  ==
          0x00000000 /* B(9)=0 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */ &&
      (inst.Bits() & 0x00030000)  ==
          0x00000000 /* op1(19:16)=xx00 */ &&
      (inst.Bits() & 0x0000FD00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx111100x0xxxxxxxx */) {
    return MSR_register_cccc00010010mm00111100000000nnnn_case_0_MSR_register_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000000 /* op2(6:4)=000 */ &&
      (inst.Bits() & 0x00000200)  ==
          0x00000000 /* B(9)=0 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */ &&
      (inst.Bits() & 0x00030000)  ==
          0x00010000 /* op1(19:16)=xx01 */ &&
      (inst.Bits() & 0x0000FD00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx111100x0xxxxxxxx */) {
    return MSR_register_cccc00010r10mmmm111100000000nnnn_case_0_MSR_register_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000000 /* op2(6:4)=000 */ &&
      (inst.Bits() & 0x00000200)  ==
          0x00000000 /* B(9)=0 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */ &&
      (inst.Bits() & 0x00020000)  ==
          0x00020000 /* op1(19:16)=xx1x */ &&
      (inst.Bits() & 0x0000FD00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx111100x0xxxxxxxx */) {
    return MSR_register_cccc00010r10mmmm111100000000nnnn_case_0_MSR_register_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000000 /* op2(6:4)=000 */ &&
      (inst.Bits() & 0x00000200)  ==
          0x00000000 /* B(9)=0 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00600000 /* op(22:21)=11 */ &&
      (inst.Bits() & 0x0000FD00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx111100x0xxxxxxxx */) {
    return MSR_register_cccc00010r10mmmm111100000000nnnn_case_0_MSR_register_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000000 /* op2(6:4)=000 */ &&
      (inst.Bits() & 0x00000200)  ==
          0x00000000 /* B(9)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* op(22:21)=x0 */ &&
      (inst.Bits() & 0x000F0D0F)  ==
          0x000F0000 /* $pattern(31:0)=xxxxxxxxxxxx1111xxxx00x0xxxx0000 */) {
    return MRS_cccc00010r001111dddd000000000000_case_0_MRS_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000000 /* op2(6:4)=000 */ &&
      (inst.Bits() & 0x00000200)  ==
          0x00000200 /* B(9)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* op(22:21)=x0 */ &&
      (inst.Bits() & 0x00000C0F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx00xxxxxx0000 */) {
    return MRS_Banked_register_cccc00010r00mmmmdddd001m00000000_case_0_MRS_Banked_register_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000000 /* op2(6:4)=000 */ &&
      (inst.Bits() & 0x00000200)  ==
          0x00000200 /* B(9)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* op(22:21)=x1 */ &&
      (inst.Bits() & 0x0000FC00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx111100xxxxxxxxxx */) {
    return MRS_Banked_register_cccc00010r10mmmm1111001m0000nnnn_case_0_MRS_Banked_register_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000010 /* op2(6:4)=001 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x000FFF00 /* $pattern(31:0)=xxxxxxxxxxxx111111111111xxxxxxxx */) {
    return Bx_cccc000100101111111111110001mmmm_case_0_Bx_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000010 /* op2(6:4)=001 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00600000 /* op(22:21)=11 */ &&
      (inst.Bits() & 0x000F0F00)  ==
          0x000F0F00 /* $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx */) {
    return CLZ_cccc000101101111dddd11110001mmmm_case_0_CLZ_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000020 /* op2(6:4)=010 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x000FFF00 /* $pattern(31:0)=xxxxxxxxxxxx111111111111xxxxxxxx */) {
    return BXJ_cccc000100101111111111110010mmmm_case_0_BXJ_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000030 /* op2(6:4)=011 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x000FFF00 /* $pattern(31:0)=xxxxxxxxxxxx111111111111xxxxxxxx */) {
    return BLX_register_cccc000100101111111111110011mmmm_case_0_BLX_register_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000050 /* op2(6:4)=101 */) {
    return decode_saturating_addition_and_subtraction(inst);
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000060 /* op2(6:4)=110 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00600000 /* op(22:21)=11 */ &&
      (inst.Bits() & 0x000FFF0F)  ==
          0x0000000E /* $pattern(31:0)=xxxxxxxxxxxx000000000000xxxx1110 */) {
    return ERET_cccc0001011000000000000001101110_case_0_ERET_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000070 /* op2(6:4)=111 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */) {
    return BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_0_BKPT_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000070 /* op2(6:4)=111 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00400000 /* op(22:21)=10 */) {
    return HVC_cccc00010100iiiiiiiiiiii0111iiii_case_0_HVC_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000070 /* op2(6:4)=111 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00600000 /* op(22:21)=11 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx000000000000xxxxxxxx */) {
    return SMC_cccc000101100000000000000111iiii_case_0_SMC_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table msr_immediate_and_hints.
 * Specified by: ('See Section A5.2.11',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_msr_immediate_and_hints(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* op1(19:16)=0000 */ &&
      (inst.Bits() & 0x000000FF)  ==
          0x00000000 /* op2(7:0)=00000000 */ &&
      (inst.Bits() & 0x0000FF00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx */) {
    return NOP_cccc0011001000001111000000000000_case_0_NOP_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* op1(19:16)=0000 */ &&
      (inst.Bits() & 0x000000FF)  ==
          0x00000001 /* op2(7:0)=00000001 */ &&
      (inst.Bits() & 0x0000FF00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx */) {
    return YIELD_cccc0011001000001111000000000001_case_0_YIELD_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* op1(19:16)=0000 */ &&
      (inst.Bits() & 0x000000FF)  ==
          0x00000002 /* op2(7:0)=00000010 */ &&
      (inst.Bits() & 0x0000FF00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx */) {
    return WFE_cccc0011001000001111000000000010_case_0_WFE_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* op1(19:16)=0000 */ &&
      (inst.Bits() & 0x000000FF)  ==
          0x00000003 /* op2(7:0)=00000011 */ &&
      (inst.Bits() & 0x0000FF00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx */) {
    return WFI_cccc0011001000001111000000000011_case_0_WFI_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* op1(19:16)=0000 */ &&
      (inst.Bits() & 0x000000FF)  ==
          0x00000004 /* op2(7:0)=00000100 */ &&
      (inst.Bits() & 0x0000FF00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx */) {
    return SEV_cccc0011001000001111000000000100_case_0_SEV_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* op1(19:16)=0000 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x000000F0 /* op2(7:0)=1111xxxx */ &&
      (inst.Bits() & 0x0000FF00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx */) {
    return DBG_cccc001100100000111100001111iiii_case_0_DBG_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00040000 /* op1(19:16)=0100 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0_MSR_immediate_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000B0000)  ==
          0x00080000 /* op1(19:16)=1x00 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0_MSR_immediate_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x00030000)  ==
          0x00010000 /* op1(19:16)=xx01 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0_MSR_immediate_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x00020000)  ==
          0x00020000 /* op1(19:16)=xx1x */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0_MSR_immediate_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00400000 /* op(22)=1 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0_MSR_immediate_instance_;
  }

  if (true) {
    return Unnamed_case_0_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table multiply_and_multiply_accumulate.
 * Specified by: ('See Section A5.2.5',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_multiply_and_multiply_accumulate(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00F00000)  ==
          0x00400000 /* op(23:20)=0100 */) {
    return UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0_UMAAL_A1_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00500000 /* op(23:20)=0101 */) {
    return Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00600000 /* op(23:20)=0110 */) {
    return MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0_MLS_A1_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00700000 /* op(23:20)=0111 */) {
    return Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0_None_instance_;
  }

  if ((inst.Bits() & 0x00E00000)  ==
          0x00000000 /* op(23:20)=000x */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0_MUL_A1_instance_;
  }

  if ((inst.Bits() & 0x00E00000)  ==
          0x00200000 /* op(23:20)=001x */) {
    return MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0_MLA_A1_instance_;
  }

  if ((inst.Bits() & 0x00E00000)  ==
          0x00800000 /* op(23:20)=100x */) {
    return UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0_UMULL_A1_instance_;
  }

  if ((inst.Bits() & 0x00E00000)  ==
          0x00A00000 /* op(23:20)=101x */) {
    return UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0_UMLAL_A1_instance_;
  }

  if ((inst.Bits() & 0x00E00000)  ==
          0x00C00000 /* op(23:20)=110x */) {
    return SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0_SMULL_A1_instance_;
  }

  if ((inst.Bits() & 0x00E00000)  ==
          0x00E00000 /* op(23:20)=111x */) {
    return SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0_SMLAL_A1_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table other_floating_point_data_processing_instructions.
 * Specified by: ('A7.5 Table A7 - 17',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_other_floating_point_data_processing_instructions(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x000F0000)  ==
          0x00000000 /* opc2(19:16)=0000 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000040 /* opc3(7:6)=01 */) {
    return VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0_VMOV_register_instance_;
  }

  if ((inst.Bits() & 0x000F0000)  ==
          0x00000000 /* opc2(19:16)=0000 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x000000C0 /* opc3(7:6)=11 */) {
    return VABS_cccc11101d110000dddd101s11m0mmmm_case_0_VABS_instance_;
  }

  if ((inst.Bits() & 0x000F0000)  ==
          0x00010000 /* opc2(19:16)=0001 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000040 /* opc3(7:6)=01 */) {
    return VNEG_cccc11101d110001dddd101s01m0mmmm_case_0_VNEG_instance_;
  }

  if ((inst.Bits() & 0x000F0000)  ==
          0x00010000 /* opc2(19:16)=0001 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x000000C0 /* opc3(7:6)=11 */) {
    return VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0_VSQRT_instance_;
  }

  if ((inst.Bits() & 0x000F0000)  ==
          0x00040000 /* opc2(19:16)=0100 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */) {
    return VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0_VCMP_VCMPE_instance_;
  }

  if ((inst.Bits() & 0x000F0000)  ==
          0x00050000 /* opc2(19:16)=0101 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */ &&
      (inst.Bits() & 0x0000002F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxxx0x0000 */) {
    return VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0_VCMP_VCMPE_instance_;
  }

  if ((inst.Bits() & 0x000F0000)  ==
          0x00070000 /* opc2(19:16)=0111 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x000000C0 /* opc3(7:6)=11 */) {
    return VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0_VCVT_between_double_precision_and_single_precision_instance_;
  }

  if ((inst.Bits() & 0x000F0000)  ==
          0x00080000 /* opc2(19:16)=1000 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */) {
    return VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_instance_;
  }

  if ((inst.Bits() & 0x000E0000)  ==
          0x00020000 /* opc2(19:16)=001x */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxx0xxxxxxxx */) {
    return VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0_VCVTB_VCVTT_instance_;
  }

  if ((inst.Bits() & 0x000E0000)  ==
          0x000C0000 /* opc2(19:16)=110x */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */) {
    return VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_instance_;
  }

  if ((inst.Bits() & 0x000A0000)  ==
          0x000A0000 /* opc2(19:16)=1x1x */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */) {
    return VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0_VCVT_between_floating_point_and_fixed_point_Floating_point_instance_;
  }

  if ((inst.Bits() & 0x00000040)  ==
          0x00000000 /* opc3(7:6)=x0 */ &&
      (inst.Bits() & 0x000000A0)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxx0x0xxxxx */) {
    return VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0_VMOV_immediate_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table packing_unpacking_saturation_and_reversal.
 * Specified by: ('See Section A5.4.3',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_packing_unpacking_saturation_and_reversal(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* A(19:16)=~1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0_SXTAB16_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* A(19:16)=1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return SXTB16_cccc011010001111ddddrr000111mmmm_case_0_SXTB16_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000A0 /* op2(7:5)=101 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SEL_cccc01101000nnnndddd11111011mmmm_case_0_SEL_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x00000020)  ==
          0x00000000 /* op2(7:5)=xx0 */) {
    return PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0_PKH_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00200000 /* op1(22:20)=010 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:5)=001 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SSAT16_cccc01101010iiiidddd11110011nnnn_case_0_SSAT16_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00200000 /* op1(22:20)=010 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* A(19:16)=~1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0_SXTAB_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00200000 /* op1(22:20)=010 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* A(19:16)=1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return SXTB_cccc011010101111ddddrr000111mmmm_case_0_SXTB_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00300000 /* op1(22:20)=011 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:5)=001 */ &&
      (inst.Bits() & 0x000F0F00)  ==
          0x000F0F00 /* $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx */) {
    return REV_cccc011010111111dddd11110011mmmm_case_0_REV_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00300000 /* op1(22:20)=011 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* A(19:16)=~1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0_SXTAH_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00300000 /* op1(22:20)=011 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* A(19:16)=1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return SXTH_cccc011010111111ddddrr000111mmmm_case_0_SXTH_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00300000 /* op1(22:20)=011 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000A0 /* op2(7:5)=101 */ &&
      (inst.Bits() & 0x000F0F00)  ==
          0x000F0F00 /* $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx */) {
    return REV16_cccc011010111111dddd11111011mmmm_case_0_REV16_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00400000 /* op1(22:20)=100 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* A(19:16)=~1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0_UXTAB16_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00400000 /* op1(22:20)=100 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* A(19:16)=1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return UXTB16_cccc011011001111ddddrr000111mmmm_case_0_UXTB16_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00600000 /* op1(22:20)=110 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:5)=001 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return USAT16_cccc01101110iiiidddd11110011nnnn_case_0_USAT16_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00600000 /* op1(22:20)=110 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* A(19:16)=~1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0_UXTAB_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00600000 /* op1(22:20)=110 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* A(19:16)=1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return UXTB_cccc011011101111ddddrr000111mmmm_case_0_UXTB_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00700000 /* op1(22:20)=111 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:5)=001 */ &&
      (inst.Bits() & 0x000F0F00)  ==
          0x000F0F00 /* $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx */) {
    return RBIT_cccc011011111111dddd11110011mmmm_case_0_RBIT_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00700000 /* op1(22:20)=111 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* A(19:16)=~1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0_UXTAH_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00700000 /* op1(22:20)=111 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* A(19:16)=1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return UXTH_cccc011011111111ddddrr000111mmmm_case_0_UXTH_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00700000 /* op1(22:20)=111 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000A0 /* op2(7:5)=101 */ &&
      (inst.Bits() & 0x000F0F00)  ==
          0x000F0F00 /* $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx */) {
    return REVSH_cccc011011111111dddd11111011mmmm_case_0_REVSH_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00200000 /* op1(22:20)=01x */ &&
      (inst.Bits() & 0x00000020)  ==
          0x00000000 /* op2(7:5)=xx0 */) {
    return SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0_SSAT_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00600000 /* op1(22:20)=11x */ &&
      (inst.Bits() & 0x00000020)  ==
          0x00000000 /* op2(7:5)=xx0 */) {
    return USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0_USAT_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table parallel_addition_and_subtraction_signed.
 * Specified by: ('See Section A5.4.1',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_parallel_addition_and_subtraction_signed(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SADD16_cccc01100001nnnndddd11110001mmmm_case_0_SADD16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:5)=001 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SASX_cccc01100001nnnndddd11110011mmmm_case_0_SASX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000040 /* op2(7:5)=010 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SSAX_cccc01100001nnnndddd11110101mmmm_case_0_SSAX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0_SSSUB16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000080 /* op2(7:5)=100 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SADD8_cccc01100001nnnndddd11111001mmmm_case_0_SADD8_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SSUB8_cccc01100001nnnndddd11111111mmmm_case_0_SSUB8_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return QADD16_cccc01100010nnnndddd11110001mmmm_case_0_QADD16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:5)=001 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return QASX_cccc01100010nnnndddd11110011mmmm_case_0_QASX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000040 /* op2(7:5)=010 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return QSAX_cccc01100010nnnndddd11110101mmmm_case_0_QSAX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return QSUB16_cccc01100010nnnndddd11110111mmmm_case_0_QSUB16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000080 /* op2(7:5)=100 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return QADD8_cccc01100010nnnndddd11111001mmmm_case_0_QADD8_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return QSUB8_cccc01100010nnnndddd11111111mmmm_case_0_QSUB8_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SHADD16_cccc01100011nnnndddd11110001mmmm_case_0_SHADD16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:5)=001 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SHASX_cccc01100011nnnndddd11110011mmmm_case_0_SHASX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000040 /* op2(7:5)=010 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SHSAX_cccc01100011nnnndddd11110101mmmm_case_0_SHSAX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0_SHSUB16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000080 /* op2(7:5)=100 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SHADD8_cccc01100011nnnndddd11111001mmmm_case_0_SHADD8_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0_SHSUB8_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table parallel_addition_and_subtraction_unsigned.
 * Specified by: ('See Section A5.4.2',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_parallel_addition_and_subtraction_unsigned(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UADD16_cccc01100101nnnndddd11110001mmmm_case_0_UADD16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:5)=001 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UASX_cccc01100101nnnndddd11110011mmmm_case_0_UASX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000040 /* op2(7:5)=010 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return USAX_cccc01100101nnnndddd11110101mmmm_case_0_USAX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return USUB16_cccc01100101nnnndddd11110111mmmm_case_0_USUB16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000080 /* op2(7:5)=100 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UADD8_cccc01100101nnnndddd11111001mmmm_case_0_UADD8_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00100000 /* op1(21:20)=01 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return USUB8_cccc01100101nnnndddd11111111mmmm_case_0_USUB8_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UQADD16_cccc01100110nnnndddd11110001mmmm_case_0_UQADD16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:5)=001 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UQASX_cccc01100110nnnndddd11110011mmmm_case_0_UQASX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000040 /* op2(7:5)=010 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UQSAX_cccc01100110nnnndddd11110101mmmm_case_0_UQSAX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0_UQSUB16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000080 /* op2(7:5)=100 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UQADD8_cccc01100110nnnndddd11111001mmmm_case_0_UQADD8_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0_UQSUB8_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UHADD16_cccc01100111nnnndddd11110001mmmm_case_0_UHADD16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:5)=001 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UHASX_cccc01100111nnnndddd11110011mmmm_case_0_UHASX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000040 /* op2(7:5)=010 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UHSAX_cccc01100111nnnndddd11110101mmmm_case_0_UHSAX_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0_UHSUB16_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000080 /* op2(7:5)=100 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UHADD8_cccc01100111nnnndddd11111001mmmm_case_0_UHADD8_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(21:20)=11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0_UHSUB8_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table saturating_addition_and_subtraction.
 * Specified by: ('See Section A5.2.6',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_saturating_addition_and_subtraction(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00600000)  ==
          0x00000000 /* op(22:21)=00 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return QADD_cccc00010000nnnndddd00000101mmmm_case_0_QADD_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return QSUB_cccc00010010nnnndddd00000101mmmm_case_0_QSUB_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00400000 /* op(22:21)=10 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return QDADD_cccc00010100nnnndddd00000101mmmm_case_0_QDADD_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00600000 /* op(22:21)=11 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return QDSUB_cccc00010110nnnndddd00000101mmmm_case_0_QDSUB_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table signed_multiply_signed_and_unsigned_divide.
 * Specified by: ('See Section A5.4.4',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_signed_multiply_signed_and_unsigned_divide(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000000 /* op2(7:5)=00x */ &&
      (inst.Bits() & 0x0000F000)  !=
          0x0000F000 /* A(15:12)=~1111 */) {
    return SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0_SMLAD_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000000 /* op2(7:5)=00x */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* A(15:12)=1111 */) {
    return SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0_SMUAD_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000040 /* op2(7:5)=01x */ &&
      (inst.Bits() & 0x0000F000)  !=
          0x0000F000 /* A(15:12)=~1111 */) {
    return SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0_SMLSD_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000040 /* op2(7:5)=01x */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* A(15:12)=1111 */) {
    return SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0_SMUSD_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00100000 /* op1(22:20)=001 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0_SDIV_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00300000 /* op1(22:20)=011 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0_UDIV_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00400000 /* op1(22:20)=100 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000000 /* op2(7:5)=00x */) {
    return SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0_SMLALD_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00400000 /* op1(22:20)=100 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000040 /* op2(7:5)=01x */) {
    return SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0_SMLSLD_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00500000 /* op1(22:20)=101 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000000 /* op2(7:5)=00x */ &&
      (inst.Bits() & 0x0000F000)  !=
          0x0000F000 /* A(15:12)=~1111 */) {
    return SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0_SMMLA_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00500000 /* op1(22:20)=101 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000000 /* op2(7:5)=00x */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* A(15:12)=1111 */) {
    return SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0_SMMUL_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00500000 /* op1(22:20)=101 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x000000C0 /* op2(7:5)=11x */) {
    return SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0_SMMLS_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table simd_dp_1imm.
 * Specified by: ('See Section A7.4.6',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_simd_dp_1imm(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000020)  ==
          0x00000000 /* op(5)=0 */ &&
      (inst.Bits() & 0x00000D00)  ==
          0x00000800 /* cmode(11:8)=10x0 */) {
    return VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_0_VMOV_immediate_A1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000000 /* op(5)=0 */ &&
      (inst.Bits() & 0x00000D00)  ==
          0x00000900 /* cmode(11:8)=10x1 */) {
    return VORR_immediate_1111001i1d000mmmddddcccc0q01mmmm_case_0_VORR_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000000 /* op(5)=0 */ &&
      (inst.Bits() & 0x00000900)  ==
          0x00000000 /* cmode(11:8)=0xx0 */) {
    return VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_0_VMOV_immediate_A1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000000 /* op(5)=0 */ &&
      (inst.Bits() & 0x00000900)  ==
          0x00000100 /* cmode(11:8)=0xx1 */) {
    return VORR_immediate_1111001i1d000mmmddddcccc0q01mmmm_case_0_VORR_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000000 /* op(5)=0 */ &&
      (inst.Bits() & 0x00000C00)  ==
          0x00000C00 /* cmode(11:8)=11xx */) {
    return VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_0_VMOV_immediate_A1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* cmode(11:8)=1110 */) {
    return VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_0_VMOV_immediate_A1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* cmode(11:8)=1111 */) {
    return Unnamed_case_1_None_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000D00)  ==
          0x00000800 /* cmode(11:8)=10x0 */) {
    return VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0_VMVN_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000D00)  ==
          0x00000900 /* cmode(11:8)=10x1 */) {
    return VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0_VBIC_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000C00 /* cmode(11:8)=110x */) {
    return VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0_VMVN_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000900)  ==
          0x00000000 /* cmode(11:8)=0xx0 */) {
    return VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0_VMVN_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000900)  ==
          0x00000100 /* cmode(11:8)=0xx1 */) {
    return VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0_VBIC_immediate_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table simd_dp_2misc.
 * Specified by: ('See Section A7.4.5',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_simd_dp_2misc(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000000 /* B(10:6)=0000x */) {
    return VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0_VREV64_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000080 /* B(10:6)=0001x */) {
    return VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0_VREV32_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000100 /* B(10:6)=0010x */) {
    return VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0_VREV16_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000400 /* B(10:6)=1000x */) {
    return VCLS_111100111d11ss00dddd01000qm0mmmm_case_0_VCLS_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000480 /* B(10:6)=1001x */) {
    return VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0_VCLZ_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000500 /* B(10:6)=1010x */) {
    return VCNT_111100111d11ss00dddd01010qm0mmmm_case_0_VCNT_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000580 /* B(10:6)=1011x */) {
    return VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0_VMVN_register_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000700 /* B(10:6)=1110x */) {
    return VQABS_111100111d11ss00dddd01110qm0mmmm_case_0_VQABS_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000780 /* B(10:6)=1111x */) {
    return VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0_VQNEG_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000700)  ==
          0x00000200 /* B(10:6)=010xx */) {
    return VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0_VPADDL_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000700)  ==
          0x00000600 /* B(10:6)=110xx */) {
    return VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0_VPADAL_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000000 /* B(10:6)=0000x */) {
    return VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0_VCGT_immediate_0_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000080 /* B(10:6)=0001x */) {
    return VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0_VCGE_immediate_0_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000100 /* B(10:6)=0010x */) {
    return VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0_VCEQ_immediate_0_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000180 /* B(10:6)=0011x */) {
    return VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0_VCLE_immediate_0_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000200 /* B(10:6)=0100x */) {
    return VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0_VCLT_immediate_0_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000300 /* B(10:6)=0110x */) {
    return VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0_VABS_A1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000380 /* B(10:6)=0111x */) {
    return VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0_VNEG_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000400 /* B(10:6)=1000x */) {
    return VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1_VCGT_immediate_0_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000480 /* B(10:6)=1001x */) {
    return VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1_VCGE_immediate_0_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000500 /* B(10:6)=1010x */) {
    return VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1_VCEQ_immediate_0_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000580 /* B(10:6)=1011x */) {
    return VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1_VCLE_immediate_0_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000600 /* B(10:6)=1100x */) {
    return VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1_VCLT_immediate_0_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000700 /* B(10:6)=1110x */) {
    return VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_VABS_A1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000780 /* B(10:6)=1111x */) {
    return VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1_VNEG_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x000007C0)  ==
          0x00000200 /* B(10:6)=01000 */) {
    return VMOVN_111100111d11ss10dddd001000m0mmmm_case_0_VMOVN_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x000007C0)  ==
          0x00000240 /* B(10:6)=01001 */) {
    return VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0_VQMOVUN_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x000007C0)  ==
          0x00000300 /* B(10:6)=01100 */) {
    return VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0_VSHLL_A2_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x000006C0)  ==
          0x00000600 /* B(10:6)=11x00 */) {
    return CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0_CVT_between_half_precision_and_single_precision_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000000 /* B(10:6)=0000x */) {
    return VSWP_111100111d11ss10dddd00000qm0mmmm_case_0_VSWP_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000080 /* B(10:6)=0001x */) {
    return VTRN_111100111d11ss10dddd00001qm0mmmm_case_0_VTRN_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000100 /* B(10:6)=0010x */) {
    return VUZP_111100111d11ss10dddd00010qm0mmmm_case_0_VUZP_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000180 /* B(10:6)=0011x */) {
    return VZIP_111100111d11ss10dddd00011qm0mmmm_case_0_VZIP_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000280 /* B(10:6)=0101x */) {
    return VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0_VQMOVN_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00030000 /* A(17:16)=11 */ &&
      (inst.Bits() & 0x00000680)  ==
          0x00000400 /* B(10:6)=10x0x */) {
    return VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0_VRECPE_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00030000 /* A(17:16)=11 */ &&
      (inst.Bits() & 0x00000680)  ==
          0x00000480 /* B(10:6)=10x1x */) {
    return VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0_VRSQRTE_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00030000 /* A(17:16)=11 */ &&
      (inst.Bits() & 0x00000600)  ==
          0x00000600 /* B(10:6)=11xxx */) {
    return VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0_VCVT_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table simd_dp_2scalar.
 * Specified by: ('See Section A7.4.3',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_simd_dp_2scalar(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000F00)  ==
          0x00000000 /* A(11:8)=0000 */) {
    return VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1_VMLA_by_scalar_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */) {
    return VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0_VMLA_by_scalar_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000200 /* A(11:8)=0010 */) {
    return VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0_VMLAL_by_scalar_A2_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000300 /* A(11:8)=0011 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0_VQDMLAL_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000400 /* A(11:8)=0100 */) {
    return VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1_VMLS_by_scalar_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000500 /* A(11:8)=0101 */) {
    return VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0_VMLS_by_scalar_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000600 /* A(11:8)=0110 */) {
    return VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0_VMLSL_by_scalar_A2_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000700 /* A(11:8)=0111 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0_VQDMLSL_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */) {
    return VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1_VMUL_by_scalar_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */) {
    return VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0_VMUL_by_scalar_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000A00 /* A(11:8)=1010 */) {
    return VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0_VMULL_by_scalar_A2_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000B00 /* A(11:8)=1011 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0_VQDMULL_A2_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000C00 /* A(11:8)=1100 */) {
    return VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0_VQDMULH_A2_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */) {
    return VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0_VQRDMULH_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table simd_dp_2shift.
 * Specified by: ('See Section A7.4.4',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_simd_dp_2shift(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000F00)  ==
          0x00000000 /* A(11:8)=0000 */) {
    return VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0_VSHR_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */) {
    return VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0_VSRA_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000200 /* A(11:8)=0010 */) {
    return VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0_VRSHR_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000300 /* A(11:8)=0011 */) {
    return VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0_VRSRA_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000400 /* A(11:8)=0100 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */) {
    return VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0_VSRI_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000500 /* A(11:8)=0101 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0_VSHL_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000500 /* A(11:8)=0101 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */) {
    return VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0_VSLI_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* B(6)=0 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return VSHRN_111100101diiiiiidddd100000m1mmmm_case_0_VSHRN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* B(6)=1 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0_VRSHRN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* B(6)=0 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0_VQRSHRUN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* B(6)=1 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0_VQRSHRUN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* B(6)=1 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0_VQRSHRN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* B(6)=0 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0_VQSHRN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* B(6)=0 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0_VQSHRUN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000A00 /* A(11:8)=1010 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* B(6)=0 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0_VSHLL_A1_or_VMOVL_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000600 /* A(11:8)=011x */) {
    return VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0_VQSHL_VQSHLU_immediate_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000E00 /* A(11:8)=111x */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0_VCVT_between_floating_point_and_fixed_point_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table simd_dp_3diff.
 * Specified by: ('See Section A7.4.2',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_simd_dp_3diff(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000F00)  ==
          0x00000400 /* A(11:8)=0100 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0_VADDHN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000400 /* A(11:8)=0100 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */) {
    return VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0_VRADDHN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000500 /* A(11:8)=0101 */) {
    return VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0_VABAL_A2_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000600 /* A(11:8)=0110 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0_VSUBHN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000600 /* A(11:8)=0110 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */) {
    return VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0_VRSUBHN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000700 /* A(11:8)=0111 */) {
    return VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0_VABDL_integer_A2_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000C00 /* A(11:8)=1100 */) {
    return VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0_VMULL_integer_A2_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0_VQDMULL_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* A(11:8)=1110 */) {
    return VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0_VMULL_polynomial_A2_instance_;
  }

  if ((inst.Bits() & 0x00000D00)  ==
          0x00000800 /* A(11:8)=10x0 */) {
    return VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0_VMLAL_VMLSL_integer_A2_instance_;
  }

  if ((inst.Bits() & 0x00000D00)  ==
          0x00000900 /* A(11:8)=10x1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0_VQDMLAL_VQDMLSL_A1_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000000 /* A(11:8)=000x */) {
    return VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0_VADDL_VADDW_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000200 /* A(11:8)=001x */) {
    return VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0_VSUBL_VSUBW_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table simd_dp_3same.
 * Specified by: ('See Section A7.4.1',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_simd_dp_3same(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000F00)  ==
          0x00000000 /* A(11:8)=0000 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0_VHADD_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000000 /* A(11:8)=0000 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */) {
    return VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0_VQADD_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0_VRHADD_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00300000)  ==
          0x00000000 /* C(21:20)=00 */) {
    return VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0_VAND_register_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00300000)  ==
          0x00100000 /* C(21:20)=01 */) {
    return VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0_VBIC_register_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00300000)  ==
          0x00200000 /* C(21:20)=10 */) {
    return VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0_VORR_register_or_VMOV_register_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00300000)  ==
          0x00300000 /* C(21:20)=11 */) {
    return VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0_VORN_register_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00300000)  ==
          0x00000000 /* C(21:20)=00 */) {
    return VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0_VEOR_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00300000)  ==
          0x00100000 /* C(21:20)=01 */) {
    return VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0_VBSL_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00300000)  ==
          0x00200000 /* C(21:20)=10 */) {
    return VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0_VBIT_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00300000)  ==
          0x00300000 /* C(21:20)=11 */) {
    return VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0_VBIF_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000200 /* A(11:8)=0010 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0_VHSUB_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000200 /* A(11:8)=0010 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */) {
    return VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0_VQSUB_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000300 /* A(11:8)=0011 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0_VCGT_register_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000300 /* A(11:8)=0011 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */) {
    return VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0_VCGE_register_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000400 /* A(11:8)=0100 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0_VSHL_register_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000400 /* A(11:8)=0100 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */) {
    return VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0_VQSHL_register_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000500 /* A(11:8)=0101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0_VRSHL_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000500 /* A(11:8)=0101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */) {
    return VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0_VQRSHL_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000600 /* A(11:8)=0110 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0_VMAX_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000600 /* A(11:8)=0110 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */) {
    return VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0_VMIN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000700 /* A(11:8)=0111 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0_VABD_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000700 /* A(11:8)=0111 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */) {
    return VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0_VABA_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0_VADD_integer_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */) {
    return VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0_VSUB_integer_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VTST_111100100dssnnnndddd1000nqm1mmmm_case_0_VTST_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */) {
    return VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0_VCEQ_register_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0_VMLA_integer_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */) {
    return VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0_VMLS_integer_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0_VMUL_integer_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */) {
    return VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0_VMUL_polynomial_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000A00 /* A(11:8)=1010 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx */) {
    return VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0_VPMAX_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000A00 /* A(11:8)=1010 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx */) {
    return VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0_VPMIN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000B00 /* A(11:8)=1011 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0_VQDMULH_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000B00 /* A(11:8)=1011 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */) {
    return VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0_VQRDMULH_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000B00 /* A(11:8)=1011 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx */) {
    return VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0_VPADD_integer_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000C00 /* A(11:8)=1100 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */ &&
      (inst.Bits() & 0x00100000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxx0xxxxxxxxxxxxxxxxxxxx */) {
    return VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0_VFMA_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000C00 /* A(11:8)=1100 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */ &&
      (inst.Bits() & 0x00100000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxx0xxxxxxxxxxxxxxxxxxxx */) {
    return VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0_VFMS_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0_VADD_floating_point_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */) {
    return VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0_VSUB_floating_point_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0_VPADD_floating_point_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */) {
    return VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0_VABD_floating_point_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0_VMLA_floating_point_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */) {
    return VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0_VMLS_floating_point_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0_VMUL_floating_point_A1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* A(11:8)=1110 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0_VCEQ_register_A2_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* A(11:8)=1110 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0_VCGE_register_A2_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* A(11:8)=1110 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */) {
    return VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0_VCGT_register_A2_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* A(11:8)=1110 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0_VACGE_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* A(11:8)=1110 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */) {
    return VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0_VACGT_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* A(11:8)=1111 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0_VMAX_floating_point_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* A(11:8)=1111 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */) {
    return VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0_VMIN_floating_point_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* A(11:8)=1111 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0_VPMAX_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* A(11:8)=1111 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */) {
    return VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0_VPMIN_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* A(11:8)=1111 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0_VRECPS_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* A(11:8)=1111 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */) {
    return VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0_VRSQRTS_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table synchronization_primitives.
 * Specified by: ('See Section A5.2.10',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_synchronization_primitives(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00F00000)  ==
          0x00800000 /* op(23:20)=1000 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return STREX_cccc00011000nnnndddd11111001tttt_case_0_STREX_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00900000 /* op(23:20)=1001 */ &&
      (inst.Bits() & 0x00000F0F)  ==
          0x00000F0F /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxx1111 */) {
    return LDREX_cccc00011001nnnntttt111110011111_case_0_LDREX_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00A00000 /* op(23:20)=1010 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return STREXD_cccc00011010nnnndddd11111001tttt_case_0_STREXD_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00B00000 /* op(23:20)=1011 */ &&
      (inst.Bits() & 0x00000F0F)  ==
          0x00000F0F /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxx1111 */) {
    return LDREXD_cccc00011011nnnntttt111110011111_case_0_LDREXD_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00C00000 /* op(23:20)=1100 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return STREXB_cccc00011100nnnndddd11111001tttt_case_0_STREXB_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00D00000 /* op(23:20)=1101 */ &&
      (inst.Bits() & 0x00000F0F)  ==
          0x00000F0F /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxx1111 */) {
    return LDREXB_cccc00011101nnnntttt111110011111_case_0_LDREXB_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00E00000 /* op(23:20)=1110 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return STREXH_cccc00011110nnnndddd11111001tttt_case_0_STREXH_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00F00000 /* op(23:20)=1111 */ &&
      (inst.Bits() & 0x00000F0F)  ==
          0x00000F0F /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxx1111 */) {
    return STREXH_cccc00011111nnnntttt111110011111_case_0_STREXH_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00000000 /* op(23:20)=0x00 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_0_SWP_SWPB_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table transfer_between_arm_core_and_extension_register_8_16_and_32_bit.
 * Specified by: ('A7.8',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_transfer_between_arm_core_and_extension_register_8_16_and_32_bit(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00100000)  ==
          0x00000000 /* L(20)=0 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* C(8)=0 */ &&
      (inst.Bits() & 0x00E00000)  ==
          0x00000000 /* A(23:21)=000 */ &&
      (inst.Bits() & 0x0000006F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx00x0000 */) {
    return VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_0_VMOV_between_ARM_core_register_and_single_precision_register_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00000000 /* L(20)=0 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* C(8)=0 */ &&
      (inst.Bits() & 0x00E00000)  ==
          0x00E00000 /* A(23:21)=111 */ &&
      (inst.Bits() & 0x000F00EF)  ==
          0x00010000 /* $pattern(31:0)=xxxxxxxxxxxx0001xxxxxxxx000x0000 */) {
    return VMSR_cccc111011100001tttt101000010000_case_0_VMSR_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00000000 /* L(20)=0 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* C(8)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23:21)=0xx */ &&
      (inst.Bits() & 0x0000000F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxxxxx0000 */) {
    return VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_0_VMOV_ARM_core_register_to_scalar_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00000000 /* L(20)=0 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* C(8)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23:21)=1xx */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* B(6:5)=0x */ &&
      (inst.Bits() & 0x0000000F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxxxxx0000 */) {
    return VDUP_ARM_core_register_cccc11101bq0ddddtttt1011d0e10000_case_0_VDUP_ARM_core_register_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00100000 /* L(20)=1 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* C(8)=0 */ &&
      (inst.Bits() & 0x00E00000)  ==
          0x00000000 /* A(23:21)=000 */ &&
      (inst.Bits() & 0x0000006F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx00x0000 */) {
    return VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000xnnnntttt1010n0010000_case_0_VMOV_between_ARM_core_register_and_single_precision_register_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00100000 /* L(20)=1 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* C(8)=0 */ &&
      (inst.Bits() & 0x00E00000)  ==
          0x00E00000 /* A(23:21)=111 */ &&
      (inst.Bits() & 0x000F00EF)  ==
          0x00010000 /* $pattern(31:0)=xxxxxxxxxxxx0001xxxxxxxx000x0000 */) {
    return VMRS_cccc111011110001tttt101000010000_case_0_VMRS_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00100000 /* L(20)=1 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* C(8)=1 */ &&
      (inst.Bits() & 0x0000000F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxxxxx0000 */) {
    return MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_0_MOVE_scalar_to_ARM_core_register_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table transfer_between_arm_core_and_extension_registers_64_bit.
 * Specified by: ('A7.9',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_transfer_between_arm_core_and_extension_registers_64_bit(
     const nacl_arm_dec::Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000100)  ==
          0x00000000 /* C(8)=0 */ &&
      (inst.Bits() & 0x000000D0)  ==
          0x00000010 /* op(7:4)=00x1 */) {
    return VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_0_VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_instance_;
  }

  if ((inst.Bits() & 0x00000100)  ==
          0x00000100 /* C(8)=1 */ &&
      (inst.Bits() & 0x000000D0)  ==
          0x00000010 /* op(7:4)=00x1 */) {
    return VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_0_VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_instance_;
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


/*
 * Implementation of table unconditional_instructions.
 * Specified by: ('See Section A5.7',)
 */
const NamedClassDecoder& NamedArm32DecoderState::decode_unconditional_instructions(
     const nacl_arm_dec::Instruction inst) const {

  if ((inst.Bits() & 0x0FF00000)  ==
          0x0C400000 /* op1(27:20)=11000100 */) {
    return MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0_MCRR2_instance_;
  }

  if ((inst.Bits() & 0x0FF00000)  ==
          0x0C500000 /* op1(27:20)=11000101 */) {
    return MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0_MRRC2_instance_;
  }

  if ((inst.Bits() & 0x0E500000)  ==
          0x08100000 /* op1(27:20)=100xx0x1 */ &&
      (inst.Bits() & 0x0000FFFF)  ==
          0x00000A00 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000101000000000 */) {
    return RFE_1111100pu0w1nnnn0000101000000000_case_0_RFE_instance_;
  }

  if ((inst.Bits() & 0x0E500000)  ==
          0x08400000 /* op1(27:20)=100xx1x0 */ &&
      (inst.Bits() & 0x000FFFE0)  ==
          0x000D0500 /* $pattern(31:0)=xxxxxxxxxxxx110100000101000xxxxx */) {
    return SRS_1111100pu1w0110100000101000iiiii_case_0_SRS_instance_;
  }

  if ((inst.Bits() & 0x0F100000)  ==
          0x0E000000 /* op1(27:20)=1110xxx0 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* op(4)=1 */) {
    return MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0_MCR2_instance_;
  }

  if ((inst.Bits() & 0x0F100000)  ==
          0x0E100000 /* op1(27:20)=1110xxx1 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* op(4)=1 */) {
    return MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0_MRC2_instance_;
  }

  if ((inst.Bits() & 0x0E100000)  ==
          0x0C000000 /* op1(27:20)=110xxxx0 */ &&
      (inst.Bits() & 0x0FB00000)  !=
          0x0C000000 /* op1_repeated(27:20)=~11000x00 */) {
    return STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0_STC2_instance_;
  }

  if ((inst.Bits() & 0x0E100000)  ==
          0x0C100000 /* op1(27:20)=110xxxx1 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */ &&
      (inst.Bits() & 0x0FB00000)  !=
          0x0C100000 /* op1_repeated(27:20)=~11000x01 */) {
    return LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0_LDC2_immediate_instance_;
  }

  if ((inst.Bits() & 0x0E100000)  ==
          0x0C100000 /* op1(27:20)=110xxxx1 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */ &&
      (inst.Bits() & 0x0FB00000)  !=
          0x0C100000 /* op1_repeated(27:20)=~11000x01 */) {
    return LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0_LDC2_literal_instance_;
  }

  if ((inst.Bits() & 0x0F000000)  ==
          0x0E000000 /* op1(27:20)=1110xxxx */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op(4)=0 */) {
    return CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0_CDP2_instance_;
  }

  if ((inst.Bits() & 0x0E000000)  ==
          0x0A000000 /* op1(27:20)=101xxxxx */) {
    return BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0_BLX_immediate_instance_;
  }

  if ((inst.Bits() & 0x08000000)  ==
          0x00000000 /* op1(27:20)=0xxxxxxx */) {
    return decode_memory_hints_advanced_simd_instructions_and_miscellaneous_instructions(inst);
  }

  if (true) {
    return Unnamed_case_1_None_instance_;
  }

  // Catch any attempt to fall through...
  return NOT_IMPLEMENTED_case_0_NOT_IMPLEMENTED_instance_;
}


const NamedClassDecoder& NamedArm32DecoderState::
decode_named(const nacl_arm_dec::Instruction inst) const {
  return decode_ARMv7(inst);
}

const nacl_arm_dec::ClassDecoder& NamedArm32DecoderState::
decode(const nacl_arm_dec::Instruction inst) const {
  return decode_named(inst).named_decoder();
}

}  // namespace nacl_arm_test
