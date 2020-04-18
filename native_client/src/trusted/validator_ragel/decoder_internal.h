/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file contains common parts of x86-32 and x86-64 internals (inline
 * functions and defines).
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_DECODER_INTERNAL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_DECODER_INTERNAL_H_

#include "native_client/src/trusted/validator_ragel/decoding.h"

/*
 * Set of macroses used in actions defined in parse_instruction.rl to pull
 * parts of the instruction from a byte stream and store them for future use.
 */
#define GET_VEX_PREFIX3() vex_prefix3
#define SET_VEX_PREFIX3(PREFIX_BYTE) vex_prefix3 = (PREFIX_BYTE)
#define SET_DATA16_PREFIX(STATUS) instruction.prefix.data16 = (STATUS)
#define SET_LOCK_PREFIX(STATUS) instruction.prefix.lock = (STATUS)
#define SET_REPZ_PREFIX(STATUS) instruction.prefix.repz = (STATUS)
#define SET_REPNZ_PREFIX(STATUS) instruction.prefix.repnz = (STATUS)
#define SET_BRANCH_TAKEN(STATUS) instruction.prefix.branch_taken = (STATUS)
#define SET_BRANCH_NOT_TAKEN(STATUS) \
  instruction.prefix.branch_not_taken = (STATUS)
#define SET_INSTRUCTION_NAME(NAME) instruction.name = (NAME)
#define GET_OPERAND_NAME(INDEX) instruction.operands[(INDEX)].name
#define SET_OPERAND_NAME(INDEX, NAME) \
  instruction.operands[(INDEX)].name = (NAME)
#define SET_OPERAND_FORMAT(INDEX, FORMAT) \
  instruction.operands[(INDEX)].format = (FORMAT)
#define SET_OPERANDS_COUNT(COUNT) instruction.operands_count = (COUNT)
#define SET_MODRM_BASE(REG_NUMBER) instruction.rm.base = (REG_NUMBER)
#define SET_MODRM_INDEX(REG_NUMBER) instruction.rm.index = (REG_NUMBER)
#define SET_MODRM_SCALE(VALUE) instruction.rm.scale = (VALUE)
#define SET_DISPLACEMENT_FORMAT(FORMAT) instruction.rm.disp_type = (FORMAT)
#define SET_DISPLACEMENT_POINTER(POINTER) \
  instruction.rm.offset = \
    DecodeDisplacementValue(instruction.rm.disp_type, (POINTER))
#define SET_IMMEDIATE_FORMAT(FORMAT) imm_operand = (FORMAT)
#define SET_IMMEDIATE_POINTER(POINTER) \
  instruction.imm[0] = DecodeImmediateValue(imm_operand, (POINTER))
#define SET_SECOND_IMMEDIATE_FORMAT(FORMAT) imm2_operand = (FORMAT)
#define SET_SECOND_IMMEDIATE_POINTER(POINTER) \
  instruction.imm[1] = DecodeImmediateValue(imm2_operand, (POINTER))
/* We don't support CPUID feature detection in decoder.  */
#define SET_CPU_FEATURE(FEATURE)
#define SET_ATT_INSTRUCTION_SUFFIX(SUFFIX_STRING) \
  instruction.att_instruction_suffix = (SUFFIX_STRING)

/*
 * Immediate mode: size of the instruction's immediate operand.  Note that there
 * IMMNONE (which means there are no immediate operand) and IMM2 (which is
 * two-bit immediate which shares it's byte with other operands).
 */
enum ImmediateMode {
  IMMNONE,
  IMM2,
  IMM8,
  IMM16,
  IMM32,
  IMM64
};

static FORCEINLINE uint64_t DecodeDisplacementValue(
    enum DisplacementMode disp_mode, const uint8_t *disp_ptr) {
  switch(disp_mode) {
    case DISPNONE: return 0;
    case DISP8:    return SignExtend8Bit(AnyFieldValue8bit(disp_ptr));
    case DISP16:   return SignExtend16Bit(AnyFieldValue16bit(disp_ptr));
    case DISP32:   return SignExtend32Bit(AnyFieldValue32bit(disp_ptr));
    case DISP64:   return AnyFieldValue64bit(disp_ptr);
  }
  assert(FALSE);
  return 0;
}


static FORCEINLINE uint64_t DecodeImmediateValue(enum ImmediateMode imm_mode,
                                                 const uint8_t *imm_ptr) {
  switch(imm_mode) {
    case IMMNONE: return 0;
    case IMM2:    return imm_ptr[0] & 0x03;
    case IMM8:    return AnyFieldValue8bit(imm_ptr);
    case IMM16:   return AnyFieldValue16bit(imm_ptr);
    case IMM32:   return AnyFieldValue32bit(imm_ptr);
    case IMM64:   return AnyFieldValue64bit(imm_ptr);
  }
  assert(FALSE);
  return 0;
}

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_DECODER_INTERNAL_H_ */
