/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE


#include "native_client/src/trusted/validator_arm/gen/arm32_decode.h"

namespace nacl_arm_dec {


Arm32DecoderState::Arm32DecoderState() : DecoderState()
  , Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_instance_()
  , Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1_instance_()
  , Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_instance_()
  , Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1_instance_()
  , Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1_instance_()
  , Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1_instance_()
  , Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1_instance_()
  , Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1_instance_()
  , Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1_instance_()
  , Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1_instance_()
  , Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1_instance_()
  , Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_()
  , Actual_BLX_register_cccc000100101111111111110011mmmm_case_1_instance_()
  , Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_()
  , Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_()
  , Actual_Bx_cccc000100101111111111110001mmmm_case_1_instance_()
  , Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1_instance_()
  , Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1_instance_()
  , Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1_instance_()
  , Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1_instance_()
  , Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1_instance_()
  , Actual_DMB_1111010101111111111100000101xxxx_case_1_instance_()
  , Actual_ISB_1111010101111111111100000110xxxx_case_1_instance_()
  , Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1_instance_()
  , Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1_instance_()
  , Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1_instance_()
  , Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1_instance_()
  , Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1_instance_()
  , Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1_instance_()
  , Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1_instance_()
  , Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1_instance_()
  , Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1_instance_()
  , Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1_instance_()
  , Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1_instance_()
  , Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1_instance_()
  , Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1_instance_()
  , Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1_instance_()
  , Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1_instance_()
  , Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1_instance_()
  , Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1_instance_()
  , Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1_instance_()
  , Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1_instance_()
  , Actual_MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_1_instance_()
  , Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1_instance_()
  , Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1_instance_()
  , Actual_MRS_cccc00010r001111dddd000000000000_case_1_instance_()
  , Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1_instance_()
  , Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1_instance_()
  , Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1_instance_()
  , Actual_NOP_cccc0011001000001111000000000000_case_1_instance_()
  , Actual_NOT_IMPLEMENTED_case_1_instance_()
  , Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1_instance_()
  , Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_()
  , Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1_instance_()
  , Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1_instance_()
  , Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1_instance_()
  , Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1_instance_()
  , Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1_instance_()
  , Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1_instance_()
  , Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1_instance_()
  , Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1_instance_()
  , Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1_instance_()
  , Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1_instance_()
  , Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1_instance_()
  , Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1_instance_()
  , Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1_instance_()
  , Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1_instance_()
  , Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1_instance_()
  , Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1_instance_()
  , Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1_instance_()
  , Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1_instance_()
  , Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1_instance_()
  , Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1_instance_()
  , Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1_instance_()
  , Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1_instance_()
  , Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1_instance_()
  , Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1_instance_()
  , Actual_SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_1_instance_()
  , Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1_instance_()
  , Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1_instance_()
  , Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1_instance_()
  , Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_instance_()
  , Actual_Unnamed_case_1_instance_()
  , Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1_instance_()
  , Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_instance_()
  , Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_instance_()
  , Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_instance_()
  , Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_instance_()
  , Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_instance_()
  , Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1_instance_()
  , Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1_instance_()
  , Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_instance_()
  , Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_instance_()
  , Actual_VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1_instance_()
  , Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1_instance_()
  , Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1_instance_()
  , Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1_instance_()
  , Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1_instance_()
  , Actual_VDUP_ARM_core_register_cccc11101bq0ddddtttt1011d0e10000_case_1_instance_()
  , Actual_VDUP_scalar_111100111d11iiiidddd11000qm0mmmm_case_1_instance_()
  , Actual_VEXT_111100101d11nnnnddddiiiinqm0mmmm_case_1_instance_()
  , Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1_instance_()
  , Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1_instance_()
  , Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1_instance_()
  , Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1_instance_()
  , Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1_instance_()
  , Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1_instance_()
  , Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1_instance_()
  , Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1_instance_()
  , Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1_instance_()
  , Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1_instance_()
  , Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1_instance_()
  , Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1_instance_()
  , Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1_instance_()
  , Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1_instance_()
  , Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1_instance_()
  , Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1_instance_()
  , Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1_instance_()
  , Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2_instance_()
  , Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1_instance_()
  , Actual_VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_1_instance_()
  , Actual_VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_1_instance_()
  , Actual_VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_1_instance_()
  , Actual_VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_1_instance_()
  , Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1_instance_()
  , Actual_VMRS_cccc111011110001tttt101000010000_case_1_instance_()
  , Actual_VMSR_cccc111011100001tttt101000010000_case_1_instance_()
  , Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1_instance_()
  , Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1_instance_()
  , Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1_instance_()
  , Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1_instance_()
  , Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1_instance_()
  , Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1_instance_()
  , Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1_instance_()
  , Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1_instance_()
  , Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1_instance_()
  , Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1_instance_()
  , Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1_instance_()
  , Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1_instance_()
  , Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1_instance_()
  , Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1_instance_()
  , Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1_instance_()
  , Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1_instance_()
  , Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1_instance_()
  , Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1_instance_()
  , Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1_instance_()
  , Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1_instance_()
  , Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1_instance_()
  , Actual_VTBL_VTBX_111100111d11nnnndddd10ccnpm0mmmm_case_1_instance_()
  , Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1_instance_()
  , Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1_instance_()
{}

// Implementation of table: ARMv7.
// Specified by: See Section A5.1
const ClassDecoder& Arm32DecoderState::decode_ARMv7(
     const Instruction inst) const
{
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

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: advanced_simd_data_processing_instructions.
// Specified by: See Section A7.4
const ClassDecoder& Arm32DecoderState::decode_advanced_simd_data_processing_instructions(
     const Instruction inst) const
{
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
    return Actual_VEXT_111100101d11nnnnddddiiiinqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00B00000)  ==
          0x00B00000 /* A(23:19)=1x11x */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000C00 /* B(11:8)=1100 */ &&
      (inst.Bits() & 0x00000090)  ==
          0x00000000 /* C(7:4)=0xx0 */) {
    return Actual_VDUP_scalar_111100111d11iiiidddd11000qm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00B00000)  ==
          0x00B00000 /* A(23:19)=1x11x */ &&
      (inst.Bits() & 0x00000C00)  ==
          0x00000800 /* B(11:8)=10xx */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* C(7:4)=xxx0 */) {
    return Actual_VTBL_VTBX_111100111d11nnnndddd10ccnpm0mmmm_case_1_instance_;
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
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: advanced_simd_element_or_structure_load_store_instructions.
// Specified by: See Section A7.7
const ClassDecoder& Arm32DecoderState::decode_advanced_simd_element_or_structure_load_store_instructions(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000C00 /* B(11:8)=1100 */) {
    return Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* B(11:8)=1101 */) {
    return Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* B(11:8)=1110 */) {
    return Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* L(21)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* B(11:8)=1111 */) {
    return Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000300 /* B(11:8)=0011 */) {
    return Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000700)  ==
          0x00000200 /* B(11:8)=x010 */) {
    return Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000000 /* B(11:8)=000x */) {
    return Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000400 /* B(11:8)=010x */) {
    return Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000600 /* B(11:8)=011x */) {
    return Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23)=0 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000800 /* B(11:8)=100x */) {
    return Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000800 /* B(11:8)=1000 */) {
    return Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000900 /* B(11:8)=1001 */) {
    return Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000A00 /* B(11:8)=1010 */) {
    return Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000B00 /* B(11:8)=1011 */) {
    return Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000000 /* B(11:8)=0x00 */) {
    return Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000100 /* B(11:8)=0x01 */) {
    return Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000200 /* B(11:8)=0x10 */) {
    return Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00800000)  ==
          0x00800000 /* A(23)=1 */ &&
      (inst.Bits() & 0x00000B00)  ==
          0x00000300 /* B(11:8)=0x11 */) {
    return Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: branch_branch_with_link_and_block_data_transfer.
// Specified by: See Section A5.5
const ClassDecoder& Arm32DecoderState::decode_branch_branch_with_link_and_block_data_transfer(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x02500000)  ==
          0x00000000 /* op(25:20)=0xx0x0 */) {
    return Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1_instance_;
  }

  if ((inst.Bits() & 0x02500000)  ==
          0x00100000 /* op(25:20)=0xx0x1 */) {
    return Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1_instance_;
  }

  if ((inst.Bits() & 0x02500000)  ==
          0x00400000 /* op(25:20)=0xx1x0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x02500000)  ==
          0x00500000 /* op(25:20)=0xx1x1 */ &&
      (inst.Bits() & 0x00008000)  ==
          0x00000000 /* R(15)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x02500000)  ==
          0x00500000 /* op(25:20)=0xx1x1 */ &&
      (inst.Bits() & 0x00008000)  ==
          0x00008000 /* R(15)=1 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x03000000)  ==
          0x02000000 /* op(25:20)=10xxxx */) {
    return Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x03000000)  ==
          0x03000000 /* op(25:20)=11xxxx */) {
    return Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: coprocessor_instructions_and_supervisor_call.
// Specified by: See Section A5.6
const ClassDecoder& Arm32DecoderState::decode_coprocessor_instructions_and_supervisor_call(
     const Instruction inst) const
{
  if ((inst.Bits() & 0x03E00000)  ==
          0x00000000 /* op1(25:20)=00000x */) {
    return Actual_Unnamed_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x03E00000)  ==
          0x00400000 /* op1(25:20)=00010x */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x03100000)  ==
          0x02000000 /* op1(25:20)=10xxx0 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* op(4)=1 */) {
    return Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x03100000)  ==
          0x02100000 /* op1(25:20)=10xxx1 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* op(4)=1 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x02100000)  ==
          0x00000000 /* op1(25:20)=0xxxx0 */ &&
      (inst.Bits() & 0x03B00000)  !=
          0x00000000 /* op1_repeated(25:20)=~000x00 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x02100000)  ==
          0x00100000 /* op1(25:20)=0xxxx1 */ &&
      (inst.Bits() & 0x03B00000)  !=
          0x00100000 /* op1_repeated(25:20)=~000x01 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  !=
          0x00000A00 /* coproc(11:8)=~101x */ &&
      (inst.Bits() & 0x03000000)  ==
          0x02000000 /* op1(25:20)=10xxxx */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op(4)=0 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
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
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: data_processing_and_miscellaneous_instructions.
// Specified by: See Section A5.2
const ClassDecoder& Arm32DecoderState::decode_data_processing_and_miscellaneous_instructions(
     const Instruction inst) const
{
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
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* op(25)=0 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x00200000 /* op1(24:20)=0xx1x */ &&
      (inst.Bits() & 0x000000D0)  ==
          0x000000D0 /* op2(7:4)=11x1 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
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
      (inst.Bits() & 0x01B00000)  ==
          0x01000000 /* op1(24:20)=10x00 */) {
    return Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1_instance_;
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

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: data_processing_immediate.
// Specified by: See Section A5.2.3
const ClassDecoder& Arm32DecoderState::decode_data_processing_immediate(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x01F00000)  ==
          0x01100000 /* op(24:20)=10001 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01500000 /* op(24:20)=10101 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01300000 /* op(24:20)=10x11 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00400000 /* op(24:20)=0010x */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */) {
    return Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00400000 /* op(24:20)=0010x */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */) {
    return Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00800000 /* op(24:20)=0100x */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */) {
    return Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00800000 /* op(24:20)=0100x */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */) {
    return Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00A00000 /* op(24:20)=0101x */) {
    return Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00C00000 /* op(24:20)=0110x */) {
    return Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01800000 /* op(24:20)=1100x */) {
    return Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01C00000 /* op(24:20)=1110x */) {
    return Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01600000)  ==
          0x00600000 /* op(24:20)=0x11x */) {
    return Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01A00000)  ==
          0x01A00000 /* op(24:20)=11x1x */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01C00000)  ==
          0x00000000 /* op(24:20)=000xx */) {
    return Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: data_processing_register.
// Specified by: See Section A5.2.1
const ClassDecoder& Arm32DecoderState::decode_data_processing_register(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000F80)  !=
          0x00000000 /* op2(11:7)=~00000 */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000000 /* op3(6:5)=00 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000F80)  !=
          0x00000000 /* op2(11:7)=~00000 */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000060 /* op3(6:5)=11 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000F80)  ==
          0x00000000 /* op2(11:7)=00000 */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000000 /* op3(6:5)=00 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000F80)  ==
          0x00000000 /* op2(11:7)=00000 */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000060 /* op3(6:5)=11 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000020 /* op3(6:5)=01 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01A00000 /* op1(24:20)=1101x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000040 /* op3(6:5)=10 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01E00000 /* op1(24:20)=1111x */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01900000)  ==
          0x01100000 /* op1(24:20)=10xx1 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01A00000)  ==
          0x01800000 /* op1(24:20)=11x0x */) {
    return Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01000000)  ==
          0x00000000 /* op1(24:20)=0xxxx */) {
    return Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: data_processing_register_shifted_register.
// Specified by: See Section A5.2.2
const ClassDecoder& Arm32DecoderState::decode_data_processing_register_shifted_register(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x01900000)  ==
          0x01100000 /* op1(24:20)=10xx1 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01A00000)  ==
          0x01800000 /* op1(24:20)=11x0x */) {
    return Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x01A00000)  ==
          0x01A00000 /* op1(24:20)=11x1x */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx */) {
    return Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x01000000)  ==
          0x00000000 /* op1(24:20)=0xxxx */) {
    return Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: extension_register_load_store_instructions.
// Specified by: A7.6
const ClassDecoder& Arm32DecoderState::decode_extension_register_load_store_instructions(
     const Instruction inst) const
{
  if ((inst.Bits() & 0x01B00000)  ==
          0x00900000 /* opcode(24:20)=01x01 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x00900000 /* opcode(24:20)=01x01 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x00B00000 /* opcode(24:20)=01x11 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000D0000 /* Rn(19:16)=~1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x00B00000 /* opcode(24:20)=01x11 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000D0000 /* Rn(19:16)=~1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x00B00000 /* opcode(24:20)=01x11 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000D0000 /* Rn(19:16)=1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x00B00000 /* opcode(24:20)=01x11 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000D0000 /* Rn(19:16)=1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01200000 /* opcode(24:20)=10x10 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000D0000 /* Rn(19:16)=~1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01200000 /* opcode(24:20)=10x10 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000D0000 /* Rn(19:16)=~1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01200000 /* opcode(24:20)=10x10 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000D0000 /* Rn(19:16)=1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01200000 /* opcode(24:20)=10x10 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000D0000 /* Rn(19:16)=1101 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01300000 /* opcode(24:20)=10x11 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01B00000)  ==
          0x01300000 /* opcode(24:20)=10x11 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x00400000 /* opcode(24:20)=0010x */) {
    return decode_transfer_between_arm_core_and_extension_registers_64_bit(inst);
  }

  if ((inst.Bits() & 0x01300000)  ==
          0x01000000 /* opcode(24:20)=1xx00 */) {
    return Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01300000)  ==
          0x01100000 /* opcode(24:20)=1xx01 */) {
    return Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01900000)  ==
          0x00800000 /* opcode(24:20)=01xx0 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* S(8)=0 */) {
    return Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01900000)  ==
          0x00800000 /* opcode(24:20)=01xx0 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* S(8)=1 */) {
    return Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: extra_load_store_instructions.
// Specified by: See Section A5.2.8
const ClassDecoder& Arm32DecoderState::decode_extra_load_store_instructions(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000060)  ==
          0x00000020 /* op2(6:5)=01 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00000000 /* op1(24:20)=xx0x0 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000020 /* op2(6:5)=01 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */) {
    return Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000020 /* op2(6:5)=01 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */) {
    return Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00000000 /* op1(24:20)=xx0x0 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00100000 /* op1(24:20)=xx0x1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */) {
    return Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x01000000 /* $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx */) {
    return Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(6:5)=10 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */) {
    return Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000060 /* op2(6:5)=11 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00000000 /* op1(24:20)=xx0x0 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000060)  ==
          0x00000060 /* op2(6:5)=11 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */) {
    return Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op2(6:5)=x1 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00100000 /* op1(24:20)=xx0x1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op2(6:5)=x1 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */) {
    return Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000040)  ==
          0x00000040 /* op2(6:5)=1x */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x01000000 /* $pattern(31:0)=xxxxxxx1xx0xxxxxxxxxxxxxxxxxxxxx */) {
    return Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: floating_point_data_processing_instructions.
// Specified by: A7.5 Table A7 - 16
const ClassDecoder& Arm32DecoderState::decode_floating_point_data_processing_instructions(
     const Instruction inst) const
{
  if ((inst.Bits() & 0x00B00000)  ==
          0x00300000 /* opc1(23:20)=0x11 */) {
    return Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00800000 /* opc1(23:20)=1x00 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* opc3(7:6)=x0 */) {
    return Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00900000 /* opc1(23:20)=1x01 */) {
    return Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00B00000 /* opc1(23:20)=1x11 */) {
    return decode_other_floating_point_data_processing_instructions(inst);
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* opc1(23:20)=xx10 */) {
    return Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00A00000)  ==
          0x00000000 /* opc1(23:20)=0x0x */) {
    return Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: halfword_multiply_and_multiply_accumulate.
// Specified by: See Section A5.2.7
const ClassDecoder& Arm32DecoderState::decode_halfword_multiply_and_multiply_accumulate(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00600000)  ==
          0x00000000 /* op1(22:21)=00 */) {
    return Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00200000 /* op1(22:21)=01 */ &&
      (inst.Bits() & 0x00000020)  ==
          0x00000000 /* op(5)=0 */) {
    return Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00200000 /* op1(22:21)=01 */ &&
      (inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00400000 /* op1(22:21)=10 */) {
    return Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00600000)  ==
          0x00600000 /* op1(22:21)=11 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: load_store_word_and_unsigned_byte.
// Specified by: See Section A5.3
const ClassDecoder& Arm32DecoderState::decode_load_store_word_and_unsigned_byte(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00000000 /* op1(24:20)=xx0x0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00200000 /* op1_repeated(24:20)=~0x010 */) {
    return Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00100000 /* op1(24:20)=xx0x1 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00300000 /* op1_repeated(24:20)=~0x011 */) {
    return Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1_instance_;
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
    return Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00600000 /* op1_repeated(24:20)=~0x110 */) {
    return Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00700000 /* op1_repeated(24:20)=~0x111 */) {
    return Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1_instance_;
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
    return Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x00000000 /* A(25)=0 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x00200000 /* op1(24:20)=0xx1x */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00000000 /* op1(24:20)=xx0x0 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00200000 /* op1_repeated(24:20)=~0x010 */) {
    return Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00100000 /* op1(24:20)=xx0x1 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00300000 /* op1_repeated(24:20)=~0x011 */) {
    return Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00400000 /* op1(24:20)=xx1x0 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00600000 /* op1_repeated(24:20)=~0x110 */) {
    return Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x00500000)  ==
          0x00500000 /* op1(24:20)=xx1x1 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01700000)  !=
          0x00700000 /* op1_repeated(24:20)=~0x111 */) {
    return Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x02000000)  ==
          0x02000000 /* A(25)=1 */ &&
      (inst.Bits() & 0x01200000)  ==
          0x00200000 /* op1(24:20)=0xx1x */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: media_instructions.
// Specified by: See Section A5.4
const ClassDecoder& Arm32DecoderState::decode_media_instructions(
     const Instruction inst) const
{
  if ((inst.Bits() & 0x01F00000)  ==
          0x01800000 /* op1(24:20)=11000 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x0000F000)  !=
          0x0000F000 /* Rd(15:12)=~1111 */) {
    return Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01800000 /* op1(24:20)=11000 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* Rd(15:12)=1111 */) {
    return Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x01F00000)  ==
          0x01F00000 /* op1(24:20)=11111 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */) {
    return Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01C00000 /* op1(24:20)=1110x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000000 /* op2(7:5)=x00 */ &&
      (inst.Bits() & 0x0000000F)  !=
          0x0000000F /* Rn(3:0)=~1111 */) {
    return Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x01E00000)  ==
          0x01C00000 /* op1(24:20)=1110x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000000 /* op2(7:5)=x00 */ &&
      (inst.Bits() & 0x0000000F)  ==
          0x0000000F /* Rn(3:0)=1111 */) {
    return Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1_instance_;
  }

  if ((inst.Bits() & 0x01A00000)  ==
          0x01A00000 /* op1(24:20)=11x1x */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000040 /* op2(7:5)=x10 */) {
    return Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1_instance_;
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
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: memory_hints_advanced_simd_instructions_and_miscellaneous_instructions.
// Specified by: See Section A5.7.1
const ClassDecoder& Arm32DecoderState::decode_memory_hints_advanced_simd_instructions_and_miscellaneous_instructions(
     const Instruction inst) const
{
  if ((inst.Bits() & 0x07F00000)  ==
          0x01000000 /* op1(26:20)=0010000 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000000 /* op2(7:4)=0000 */ &&
      (inst.Bits() & 0x00010000)  ==
          0x00010000 /* Rn(19:16)=xxx1 */ &&
      (inst.Bits() & 0x000EFD0F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx000x000000x0xxxx0000 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x01000000 /* op1(26:20)=0010000 */ &&
      (inst.Bits() & 0x00000020)  ==
          0x00000000 /* op2(7:4)=xx0x */ &&
      (inst.Bits() & 0x00010000)  ==
          0x00000000 /* Rn(19:16)=xxx0 */ &&
      (inst.Bits() & 0x0000FE00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000000xxxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05300000 /* op1(26:20)=1010011 */) {
    return Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000000 /* op2(7:4)=0000 */) {
    return Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000010 /* op2(7:4)=0001 */ &&
      (inst.Bits() & 0x000FFF0F)  ==
          0x000FF00F /* $pattern(31:0)=xxxxxxxxxxxx111111110000xxxx1111 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000060 /* op2(7:4)=0110 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x000FF000 /* $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx */) {
    return Actual_ISB_1111010101111111111100000110xxxx_case_1_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x00000070 /* op2(7:4)=0111 */) {
    return Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:4)=001x */) {
    return Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000040 /* op2(7:4)=010x */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x000FF000 /* $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx */) {
    return Actual_DMB_1111010101111111111100000101xxxx_case_1_instance_;
  }

  if ((inst.Bits() & 0x07F00000)  ==
          0x05700000 /* op1(26:20)=1010111 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000080 /* op2(7:4)=1xxx */) {
    return Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x04100000 /* op1(26:20)=100x001 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x04500000 /* op1(26:20)=100x101 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x05100000 /* op1(26:20)=101x001 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */) {
    return Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x05500000 /* op1(26:20)=101x101 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* Rn(19:16)=1111 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x06100000 /* op1(26:20)=110x001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op2(7:4)=xxx0 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x07700000)  ==
          0x06500000 /* op1(26:20)=110x101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op2(7:4)=xxx0 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x07B00000)  ==
          0x05B00000 /* op1(26:20)=1011x11 */) {
    return Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_instance_;
  }

  if ((inst.Bits() & 0x07300000)  ==
          0x04300000 /* op1(26:20)=100xx11 */) {
    return Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_instance_;
  }

  if ((inst.Bits() & 0x07300000)  ==
          0x05100000 /* op1(26:20)=101xx01 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* Rn(19:16)=~1111 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x07300000)  ==
          0x07100000 /* op1(26:20)=111xx01 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op2(7:4)=xxx0 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x06300000)  ==
          0x06300000 /* op1(26:20)=11xxx11 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* op2(7:4)=xxx0 */) {
    return Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_instance_;
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
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: miscellaneous_instructions.
// Specified by: See Section A5.2.12
const ClassDecoder& Arm32DecoderState::decode_miscellaneous_instructions(
     const Instruction inst) const
{
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
    return Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1_instance_;
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
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
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
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000000 /* op2(6:4)=000 */ &&
      (inst.Bits() & 0x00000200)  ==
          0x00000000 /* B(9)=0 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00600000 /* op(22:21)=11 */ &&
      (inst.Bits() & 0x0000FD00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx111100x0xxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000000 /* op2(6:4)=000 */ &&
      (inst.Bits() & 0x00000200)  ==
          0x00000000 /* B(9)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* op(22:21)=x0 */ &&
      (inst.Bits() & 0x000F0D0F)  ==
          0x000F0000 /* $pattern(31:0)=xxxxxxxxxxxx1111xxxx00x0xxxx0000 */) {
    return Actual_MRS_cccc00010r001111dddd000000000000_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000000 /* op2(6:4)=000 */ &&
      (inst.Bits() & 0x00000200)  ==
          0x00000200 /* B(9)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* op(22:21)=x0 */ &&
      (inst.Bits() & 0x00000C0F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx00xxxxxx0000 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000000 /* op2(6:4)=000 */ &&
      (inst.Bits() & 0x00000200)  ==
          0x00000200 /* B(9)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* op(22:21)=x1 */ &&
      (inst.Bits() & 0x0000FC00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx111100xxxxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000010 /* op2(6:4)=001 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x000FFF00 /* $pattern(31:0)=xxxxxxxxxxxx111111111111xxxxxxxx */) {
    return Actual_Bx_cccc000100101111111111110001mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000010 /* op2(6:4)=001 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00600000 /* op(22:21)=11 */ &&
      (inst.Bits() & 0x000F0F00)  ==
          0x000F0F00 /* $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx */) {
    return Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000020 /* op2(6:4)=010 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x000FFF00 /* $pattern(31:0)=xxxxxxxxxxxx111111111111xxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000030 /* op2(6:4)=011 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x000FFF00 /* $pattern(31:0)=xxxxxxxxxxxx111111111111xxxxxxxx */) {
    return Actual_BLX_register_cccc000100101111111111110011mmmm_case_1_instance_;
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
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000070 /* op2(6:4)=111 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00200000 /* op(22:21)=01 */) {
    return Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000070 /* op2(6:4)=111 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00400000 /* op(22:21)=10 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000070)  ==
          0x00000070 /* op2(6:4)=111 */ &&
      (inst.Bits() & 0x00600000)  ==
          0x00600000 /* op(22:21)=11 */ &&
      (inst.Bits() & 0x000FFF00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxx000000000000xxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: msr_immediate_and_hints.
// Specified by: See Section A5.2.11
const ClassDecoder& Arm32DecoderState::decode_msr_immediate_and_hints(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* op1(19:16)=0000 */ &&
      (inst.Bits() & 0x000000FF)  ==
          0x00000004 /* op2(7:0)=00000100 */ &&
      (inst.Bits() & 0x0000FF00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* op1(19:16)=0000 */ &&
      (inst.Bits() & 0x000000FE)  ==
          0x00000000 /* op2(7:0)=0000000x */ &&
      (inst.Bits() & 0x0000FF00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx */) {
    return Actual_NOP_cccc0011001000001111000000000000_case_1_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* op1(19:16)=0000 */ &&
      (inst.Bits() & 0x000000FE)  ==
          0x00000002 /* op2(7:0)=0000001x */ &&
      (inst.Bits() & 0x0000FF00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00000000 /* op1(19:16)=0000 */ &&
      (inst.Bits() & 0x000000F0)  ==
          0x000000F0 /* op2(7:0)=1111xxxx */ &&
      (inst.Bits() & 0x0000FF00)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx11110000xxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x00040000 /* op1(19:16)=0100 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x000B0000)  ==
          0x00080000 /* op1(19:16)=1x00 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x00030000)  ==
          0x00010000 /* op1(19:16)=xx01 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00000000 /* op(22)=0 */ &&
      (inst.Bits() & 0x00020000)  ==
          0x00020000 /* op1(19:16)=xx1x */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00400000)  ==
          0x00400000 /* op(22)=1 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if (true) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: multiply_and_multiply_accumulate.
// Specified by: See Section A5.2.5
const ClassDecoder& Arm32DecoderState::decode_multiply_and_multiply_accumulate(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00F00000)  ==
          0x00400000 /* op(23:20)=0100 */) {
    return Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00600000 /* op(23:20)=0110 */) {
    return Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00D00000)  ==
          0x00500000 /* op(23:20)=01x1 */) {
    return Actual_Unnamed_case_1_instance_;
  }

  if ((inst.Bits() & 0x00E00000)  ==
          0x00000000 /* op(23:20)=000x */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) {
    return Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00E00000)  ==
          0x00200000 /* op(23:20)=001x */) {
    return Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00A00000)  ==
          0x00800000 /* op(23:20)=1x0x */) {
    return Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00A00000)  ==
          0x00A00000 /* op(23:20)=1x1x */) {
    return Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: other_floating_point_data_processing_instructions.
// Specified by: A7.5 Table A7 - 17
const ClassDecoder& Arm32DecoderState::decode_other_floating_point_data_processing_instructions(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x000F0000)  ==
          0x00010000 /* opc2(19:16)=0001 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */) {
    return Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x000F0000)  ==
          0x00050000 /* opc2(19:16)=0101 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */ &&
      (inst.Bits() & 0x0000002F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxxx0x0000 */) {
    return Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x000F0000)  ==
          0x00070000 /* opc2(19:16)=0111 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x000000C0 /* opc3(7:6)=11 */) {
    return Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x000F0000)  ==
          0x00080000 /* opc2(19:16)=1000 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */) {
    return Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x000B0000)  ==
          0x00000000 /* opc2(19:16)=0x00 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */) {
    return Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x000E0000)  ==
          0x00020000 /* opc2(19:16)=001x */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxx0xxxxxxxx */) {
    return Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x000E0000)  ==
          0x000C0000 /* opc2(19:16)=110x */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */) {
    return Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x000A0000)  ==
          0x000A0000 /* opc2(19:16)=1x1x */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000040 /* opc3(7:6)=x1 */) {
    return Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000040)  ==
          0x00000000 /* opc3(7:6)=x0 */ &&
      (inst.Bits() & 0x000000A0)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxx0x0xxxxx */) {
    return Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: packing_unpacking_saturation_and_reversal.
// Specified by: See Section A5.4.3
const ClassDecoder& Arm32DecoderState::decode_packing_unpacking_saturation_and_reversal(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000A0 /* op2(7:5)=101 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x00000020)  ==
          0x00000000 /* op2(7:5)=xx0 */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(22:20)=x10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000020 /* op2(7:5)=001 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(22:20)=x11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* A(19:16)=~1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(22:20)=x11 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* A(19:16)=1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00300000 /* op1(22:20)=x11 */ &&
      (inst.Bits() & 0x00000060)  ==
          0x00000020 /* op2(7:5)=x01 */ &&
      (inst.Bits() & 0x000F0F00)  ==
          0x000F0F00 /* $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx */) {
    return Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00000000 /* op1(22:20)=xx0 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  !=
          0x000F0000 /* A(19:16)=~1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00000000 /* op1(22:20)=xx0 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000060 /* op2(7:5)=011 */ &&
      (inst.Bits() & 0x000F0000)  ==
          0x000F0000 /* A(19:16)=1111 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx */) {
    return Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00200000)  ==
          0x00200000 /* op1(22:20)=x1x */ &&
      (inst.Bits() & 0x00000020)  ==
          0x00000000 /* op2(7:5)=xx0 */) {
    return Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: parallel_addition_and_subtraction_signed.
// Specified by: See Section A5.4.1
const ClassDecoder& Arm32DecoderState::decode_parallel_addition_and_subtraction_signed(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000080 /* op2(7:5)=100 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* op2(7:5)=0xx */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00100000 /* op1(21:20)=x1 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000080 /* op2(7:5)=100 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00100000 /* op1(21:20)=x1 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00100000 /* op1(21:20)=x1 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* op2(7:5)=0xx */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: parallel_addition_and_subtraction_unsigned.
// Specified by: See Section A5.4.2
const ClassDecoder& Arm32DecoderState::decode_parallel_addition_and_subtraction_unsigned(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000080 /* op2(7:5)=100 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00300000)  ==
          0x00200000 /* op1(21:20)=10 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* op2(7:5)=0xx */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00100000 /* op1(21:20)=x1 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000080 /* op2(7:5)=100 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00100000 /* op1(21:20)=x1 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x000000E0 /* op2(7:5)=111 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00100000 /* op1(21:20)=x1 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* op2(7:5)=0xx */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: saturating_addition_and_subtraction.
// Specified by: See Section A5.2.6
const ClassDecoder& Arm32DecoderState::decode_saturating_addition_and_subtraction(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: signed_multiply_signed_and_unsigned_divide.
// Specified by: See Section A5.4.4
const ClassDecoder& Arm32DecoderState::decode_signed_multiply_signed_and_unsigned_divide(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* op2(7:5)=0xx */ &&
      (inst.Bits() & 0x0000F000)  !=
          0x0000F000 /* A(15:12)=~1111 */) {
    return Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00000000 /* op1(22:20)=000 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* op2(7:5)=0xx */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* A(15:12)=1111 */) {
    return Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00400000 /* op1(22:20)=100 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* op2(7:5)=0xx */) {
    return Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00500000 /* op1(22:20)=101 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000000 /* op2(7:5)=00x */ &&
      (inst.Bits() & 0x0000F000)  !=
          0x0000F000 /* A(15:12)=~1111 */) {
    return Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00500000 /* op1(22:20)=101 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x00000000 /* op2(7:5)=00x */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* A(15:12)=1111 */) {
    return Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00700000)  ==
          0x00500000 /* op1(22:20)=101 */ &&
      (inst.Bits() & 0x000000C0)  ==
          0x000000C0 /* op2(7:5)=11x */) {
    return Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1_instance_;
  }

  if ((inst.Bits() & 0x00500000)  ==
          0x00100000 /* op1(22:20)=0x1 */ &&
      (inst.Bits() & 0x000000E0)  ==
          0x00000000 /* op2(7:5)=000 */ &&
      (inst.Bits() & 0x0000F000)  ==
          0x0000F000 /* $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx */) {
    return Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: simd_dp_1imm.
// Specified by: See Section A7.4.6
const ClassDecoder& Arm32DecoderState::decode_simd_dp_1imm(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000D00)  ==
          0x00000900 /* cmode(11:8)=10x1 */) {
    return Actual_VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000900)  ==
          0x00000100 /* cmode(11:8)=0xx1 */) {
    return Actual_VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000000 /* op(5)=0 */ &&
      (inst.Bits() & 0x00000D00)  ==
          0x00000800 /* cmode(11:8)=10x0 */) {
    return Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000000 /* op(5)=0 */ &&
      (inst.Bits() & 0x00000900)  ==
          0x00000000 /* cmode(11:8)=0xx0 */) {
    return Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000000 /* op(5)=0 */ &&
      (inst.Bits() & 0x00000C00)  ==
          0x00000C00 /* cmode(11:8)=11xx */) {
    return Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* cmode(11:8)=1110 */) {
    return Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* cmode(11:8)=1111 */) {
    return Actual_Unnamed_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000D00)  ==
          0x00000800 /* cmode(11:8)=10x0 */) {
    return Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000E00)  ==
          0x00000C00 /* cmode(11:8)=110x */) {
    return Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000020)  ==
          0x00000020 /* op(5)=1 */ &&
      (inst.Bits() & 0x00000900)  ==
          0x00000000 /* cmode(11:8)=0xx0 */) {
    return Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: simd_dp_2misc.
// Specified by: See Section A7.4.5
const ClassDecoder& Arm32DecoderState::decode_simd_dp_2misc(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000100 /* B(10:6)=0010x */) {
    return Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000700)  ==
          0x00000000 /* B(10:6)=000xx */) {
    return Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000700)  ==
          0x00000400 /* B(10:6)=100xx */) {
    return Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000700)  ==
          0x00000500 /* B(10:6)=101xx */) {
    return Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000700)  ==
          0x00000700 /* B(10:6)=111xx */) {
    return Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00000000 /* A(17:16)=00 */ &&
      (inst.Bits() & 0x00000300)  ==
          0x00000200 /* B(10:6)=x10xx */) {
    return Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000380 /* B(10:6)=0111x */) {
    return Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000780 /* B(10:6)=1111x */) {
    return Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000680)  ==
          0x00000200 /* B(10:6)=01x0x */) {
    return Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000680)  ==
          0x00000600 /* B(10:6)=11x0x */) {
    return Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000600)  ==
          0x00000000 /* B(10:6)=00xxx */) {
    return Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00010000 /* A(17:16)=01 */ &&
      (inst.Bits() & 0x00000600)  ==
          0x00000400 /* B(10:6)=10xxx */) {
    return Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x000007C0)  ==
          0x00000200 /* B(10:6)=01000 */) {
    return Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x000007C0)  ==
          0x00000240 /* B(10:6)=01001 */) {
    return Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x000007C0)  ==
          0x00000300 /* B(10:6)=01100 */) {
    return Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x000006C0)  ==
          0x00000600 /* B(10:6)=11x00 */) {
    return Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000000 /* B(10:6)=0000x */) {
    return Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000080 /* B(10:6)=0001x */) {
    return Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x00000780)  ==
          0x00000280 /* B(10:6)=0101x */) {
    return Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00020000 /* A(17:16)=10 */ &&
      (inst.Bits() & 0x00000700)  ==
          0x00000100 /* B(10:6)=001xx */) {
    return Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00030000)  ==
          0x00030000 /* A(17:16)=11 */ &&
      (inst.Bits() & 0x00000400)  ==
          0x00000400 /* B(10:6)=1xxxx */) {
    return Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: simd_dp_2scalar.
// Specified by: See Section A7.4.3
const ClassDecoder& Arm32DecoderState::decode_simd_dp_2scalar(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */) {
    return Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000A00 /* A(11:8)=1010 */) {
    return Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000B00 /* A(11:8)=1011 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */) {
    return Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2_instance_;
  }

  if ((inst.Bits() & 0x00000B00)  ==
          0x00000100 /* A(11:8)=0x01 */) {
    return Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000B00)  ==
          0x00000200 /* A(11:8)=0x10 */) {
    return Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000B00)  ==
          0x00000300 /* A(11:8)=0x11 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000300)  ==
          0x00000000 /* A(11:8)=xx00 */) {
    return Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: simd_dp_2shift.
// Specified by: See Section A7.4.4
const ClassDecoder& Arm32DecoderState::decode_simd_dp_2shift(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000F00)  ==
          0x00000500 /* A(11:8)=0101 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000A00 /* A(11:8)=1010 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* B(6)=0 */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000400 /* A(11:8)=010x */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */) {
    return Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000600 /* A(11:8)=011x */) {
    return Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000E00 /* A(11:8)=111x */ &&
      (inst.Bits() & 0x00000080)  ==
          0x00000000 /* L(7)=0 */) {
    return Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000C00)  ==
          0x00000000 /* A(11:8)=00xx */) {
    return Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: simd_dp_3diff.
// Specified by: See Section A7.4.2
const ClassDecoder& Arm32DecoderState::decode_simd_dp_3diff(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000F00)  ==
          0x00000C00 /* A(11:8)=1100 */) {
    return Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* A(11:8)=1110 */) {
    return Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000D00)  ==
          0x00000400 /* A(11:8)=01x0 */) {
    return Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000D00)  ==
          0x00000500 /* A(11:8)=01x1 */) {
    return Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000D00)  ==
          0x00000800 /* A(11:8)=10x0 */) {
    return Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000D00)  ==
          0x00000900 /* A(11:8)=10x1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000C00)  ==
          0x00000000 /* A(11:8)=00xx */) {
    return Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: simd_dp_3same.
// Specified by: See Section A7.4.1
const ClassDecoder& Arm32DecoderState::decode_simd_dp_3same(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000F00)  ==
          0x00000100 /* A(11:8)=0001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */) {
    return Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000600 /* A(11:8)=0110 */) {
    return Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000700 /* A(11:8)=0111 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000800 /* A(11:8)=1000 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */) {
    return Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000900 /* A(11:8)=1001 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */) {
    return Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000A00 /* A(11:8)=1010 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx */) {
    return Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000B00 /* A(11:8)=1011 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000B00 /* A(11:8)=1011 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00000040)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx0xxxxxx */) {
    return Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000C00 /* A(11:8)=1100 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */ &&
      (inst.Bits() & 0x00100000)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxx0xxxxxxxxxxxxxxxxxxxx */) {
    return Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */) {
    return Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000D00 /* A(11:8)=1101 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* A(11:8)=1110 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* A(11:8)=1110 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000E00 /* A(11:8)=1110 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */) {
    return Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* A(11:8)=1111 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00200000 /* C(21:20)=1x */) {
    return Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000B00)  ==
          0x00000300 /* A(11:8)=0x11 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */) {
    return Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000D00)  ==
          0x00000000 /* A(11:8)=00x0 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000010 /* B(4)=1 */) {
    return Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000D00)  ==
          0x00000D00 /* A(11:8)=11x1 */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x01000000 /* U(24)=1 */ &&
      (inst.Bits() & 0x00200000)  ==
          0x00000000 /* C(21:20)=0x */) {
    return Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000D00)  ==
          0x00000D00 /* A(11:8)=11x1 */ &&
      (inst.Bits() & 0x01000000)  ==
          0x00000000 /* U(24)=0 */) {
    return Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000E00)  ==
          0x00000400 /* A(11:8)=010x */) {
    return Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000C00)  ==
          0x00000000 /* A(11:8)=00xx */ &&
      (inst.Bits() & 0x00000010)  ==
          0x00000000 /* B(4)=0 */) {
    return Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: synchronization_primitives.
// Specified by: See Section A5.2.10
const ClassDecoder& Arm32DecoderState::decode_synchronization_primitives(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00F00000)  ==
          0x00A00000 /* op(23:20)=1010 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00B00000 /* op(23:20)=1011 */ &&
      (inst.Bits() & 0x00000F0F)  ==
          0x00000F0F /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxx1111 */) {
    return Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00E00000 /* op(23:20)=1110 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1_instance_;
  }

  if ((inst.Bits() & 0x00F00000)  ==
          0x00F00000 /* op(23:20)=1111 */ &&
      (inst.Bits() & 0x00000F0F)  ==
          0x00000F0F /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxx1111 */) {
    return Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00000000 /* op(23:20)=0x00 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx0000xxxxxxxx */) {
    return Actual_SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_1_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00800000 /* op(23:20)=1x00 */ &&
      (inst.Bits() & 0x00000F00)  ==
          0x00000F00 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx */) {
    return Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1_instance_;
  }

  if ((inst.Bits() & 0x00B00000)  ==
          0x00900000 /* op(23:20)=1x01 */ &&
      (inst.Bits() & 0x00000F0F)  ==
          0x00000F0F /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxx1111 */) {
    return Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: transfer_between_arm_core_and_extension_register_8_16_and_32_bit.
// Specified by: A7.8
const ClassDecoder& Arm32DecoderState::decode_transfer_between_arm_core_and_extension_register_8_16_and_32_bit(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000100)  ==
          0x00000000 /* C(8)=0 */ &&
      (inst.Bits() & 0x00E00000)  ==
          0x00000000 /* A(23:21)=000 */ &&
      (inst.Bits() & 0x0000006F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxx00x0000 */) {
    return Actual_VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00000000 /* L(20)=0 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* C(8)=0 */ &&
      (inst.Bits() & 0x00E00000)  ==
          0x00E00000 /* A(23:21)=111 */ &&
      (inst.Bits() & 0x000F00EF)  ==
          0x00010000 /* $pattern(31:0)=xxxxxxxxxxxx0001xxxxxxxx000x0000 */) {
    return Actual_VMSR_cccc111011100001tttt101000010000_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00000000 /* L(20)=0 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* C(8)=1 */ &&
      (inst.Bits() & 0x00800000)  ==
          0x00000000 /* A(23:21)=0xx */ &&
      (inst.Bits() & 0x0000000F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxxxxx0000 */) {
    return Actual_VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_1_instance_;
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
    return Actual_VDUP_ARM_core_register_cccc11101bq0ddddtttt1011d0e10000_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00100000 /* L(20)=1 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000000 /* C(8)=0 */ &&
      (inst.Bits() & 0x00E00000)  ==
          0x00E00000 /* A(23:21)=111 */ &&
      (inst.Bits() & 0x000F00EF)  ==
          0x00010000 /* $pattern(31:0)=xxxxxxxxxxxx0001xxxxxxxx000x0000 */) {
    return Actual_VMRS_cccc111011110001tttt101000010000_case_1_instance_;
  }

  if ((inst.Bits() & 0x00100000)  ==
          0x00100000 /* L(20)=1 */ &&
      (inst.Bits() & 0x00000100)  ==
          0x00000100 /* C(8)=1 */ &&
      (inst.Bits() & 0x0000000F)  ==
          0x00000000 /* $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxxxxxxxx0000 */) {
    return Actual_MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: transfer_between_arm_core_and_extension_registers_64_bit.
// Specified by: A7.9
const ClassDecoder& Arm32DecoderState::decode_transfer_between_arm_core_and_extension_registers_64_bit(
     const Instruction inst) const
{
  UNREFERENCED_PARAMETER(inst);
  if ((inst.Bits() & 0x00000100)  ==
          0x00000000 /* C(8)=0 */ &&
      (inst.Bits() & 0x000000D0)  ==
          0x00000010 /* op(7:4)=00x1 */) {
    return Actual_VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_1_instance_;
  }

  if ((inst.Bits() & 0x00000100)  ==
          0x00000100 /* C(8)=1 */ &&
      (inst.Bits() & 0x000000D0)  ==
          0x00000010 /* op(7:4)=00x1 */) {
    return Actual_VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_1_instance_;
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

// Implementation of table: unconditional_instructions.
// Specified by: See Section A5.7
const ClassDecoder& Arm32DecoderState::decode_unconditional_instructions(
     const Instruction inst) const
{
  if ((inst.Bits() & 0x0FE00000)  ==
          0x0C400000 /* op1(27:20)=1100010x */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x0E500000)  ==
          0x08100000 /* op1(27:20)=100xx0x1 */ &&
      (inst.Bits() & 0x0000FFFF)  ==
          0x00000A00 /* $pattern(31:0)=xxxxxxxxxxxxxxxx0000101000000000 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x0E500000)  ==
          0x08400000 /* op1(27:20)=100xx1x0 */ &&
      (inst.Bits() & 0x000FFFE0)  ==
          0x000D0500 /* $pattern(31:0)=xxxxxxxxxxxx110100000101000xxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x0E100000)  ==
          0x0C000000 /* op1(27:20)=110xxxx0 */ &&
      (inst.Bits() & 0x0FB00000)  !=
          0x0C000000 /* op1_repeated(27:20)=~11000x00 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x0E100000)  ==
          0x0C100000 /* op1(27:20)=110xxxx1 */ &&
      (inst.Bits() & 0x0FB00000)  !=
          0x0C100000 /* op1_repeated(27:20)=~11000x01 */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x0F000000)  ==
          0x0E000000 /* op1(27:20)=1110xxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x0E000000)  ==
          0x0A000000 /* op1(27:20)=101xxxxx */) {
    return Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  }

  if ((inst.Bits() & 0x08000000)  ==
          0x00000000 /* op1(27:20)=0xxxxxxx */) {
    return decode_memory_hints_advanced_simd_instructions_and_miscellaneous_instructions(inst);
  }

  if (true) {
    return Actual_Unnamed_case_1_instance_;
  }

  // Catch any attempt to fall though ...
  return Actual_NOT_IMPLEMENTED_case_1_instance_;
}

const ClassDecoder& Arm32DecoderState::decode(const Instruction inst) const {
  return decode_ARMv7(inst);
}

}  // namespace nacl_arm_dec
