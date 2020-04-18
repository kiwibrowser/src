/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file contains common parts of x86-32 and x86-64 internals (inline
 * functions and defines).
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_VALIDATOR_INTERNAL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_VALIDATOR_INTERNAL_H_

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator_ragel/decoding.h"
#include "native_client/src/trusted/validator_ragel/validator.h"

/* Maximum set of R-DFA allowable CPUID features.  */
extern const NaClCPUFeaturesX86 kValidatorCPUIDFeatures;

/* Macroses to support CPUID handling.  */

/*
 * Main macro: FEATURE parameter here is one of the macroses below, e.g.
 * SET_CPU_FEATURE(CPUFeature_AESAVX).
 */
#define SET_CPU_FEATURE(FEATURE) \
  if (!(FEATURE(kValidatorCPUIDFeatures.data))) { \
    instruction_info_collected |= UNRECOGNIZED_INSTRUCTION; \
  } \
  if (!(FEATURE(cpu_features->data))) { \
    instruction_info_collected |= CPUID_UNSUPPORTED_INSTRUCTION; \
  }
/*
 * Macroses to access induvidual elements of NaClCPUFeaturesX86 structure,
 * e.g. CPUFeature_AESAVX(kValidatorCPUIDFeatures.data).
 */
#define CPUFeature_3DNOW(FEATURES)    FEATURES[NaClCPUFeatureX86_3DNOW]
/*
 * AMD documentation claims it's always available if CPUFeature_LM is present,
 * But Intel documentation does not even mention it!
 * Keep it as 3DNow! instruction.
 */
#define CPUFeature_3DPRFTCH(FEATURES) \
  (CPUFeature_3DNOW(FEATURES) || CPUFeature_PRE(FEATURES))
#define CPUFeature_AES(FEATURES)      FEATURES[NaClCPUFeatureX86_AES]
#define CPUFeature_AESAVX(FEATURES) \
  (CPUFeature_AES(FEATURES) && CPUFeature_AVX(FEATURES))
#define CPUFeature_AVX(FEATURES)      FEATURES[NaClCPUFeatureX86_AVX]
#define CPUFeature_AVX2(FEATURES)     FEATURES[NaClCPUFeatureX86_AVX2]
#define CPUFeature_BMI1(FEATURES)     FEATURES[NaClCPUFeatureX86_BMI1]
#define CPUFeature_CLFLUSH(FEATURES)  FEATURES[NaClCPUFeatureX86_CLFLUSH]
#define CPUFeature_CLMUL(FEATURES)    FEATURES[NaClCPUFeatureX86_CLMUL]
#define CPUFeature_CLMULAVX(FEATURES) \
  (CPUFeature_CLMUL(FEATURES) && CPUFeature_AVX(FEATURES))
#define CPUFeature_CMOV(FEATURES)     FEATURES[NaClCPUFeatureX86_CMOV]
#define CPUFeature_CMOVx87(FEATURES)  \
  (CPUFeature_CMOV(FEATURES) && CPUFeature_x87(FEATURES))
#define CPUFeature_CX16(FEATURES)     FEATURES[NaClCPUFeatureX86_CX16]
#define CPUFeature_CX8(FEATURES)      FEATURES[NaClCPUFeatureX86_CX8]
#define CPUFeature_E3DNOW(FEATURES)   FEATURES[NaClCPUFeatureX86_E3DNOW]
#define CPUFeature_EMMX(FEATURES)     FEATURES[NaClCPUFeatureX86_EMMX]
#define CPUFeature_EMMXSSE(FEATURES) \
  (CPUFeature_EMMX(FEATURES) || CPUFeature_SSE(FEATURES))
#define CPUFeature_F16C(FEATURES)     FEATURES[NaClCPUFeatureX86_F16C]
#define CPUFeature_FMA(FEATURES)      FEATURES[NaClCPUFeatureX86_FMA]
#define CPUFeature_FMA4(FEATURES)     FEATURES[NaClCPUFeatureX86_FMA4]
#define CPUFeature_FXSR(FEATURES)     FEATURES[NaClCPUFeatureX86_FXSR]
#define CPUFeature_LAHF(FEATURES)     FEATURES[NaClCPUFeatureX86_LAHF]
#define CPUFeature_LM(FEATURES)       FEATURES[NaClCPUFeatureX86_LM]
#define CPUFeature_LWP(FEATURES)      FEATURES[NaClCPUFeatureX86_LWP]
/*
 * We allow lzcnt unconditionally
 * See http://code.google.com/p/nativeclient/issues/detail?id=2869
 */
#define CPUFeature_LZCNT(FEATURES)    TRUE
#define CPUFeature_MMX(FEATURES)      FEATURES[NaClCPUFeatureX86_MMX]
#define CPUFeature_MON(FEATURES)      FEATURES[NaClCPUFeatureX86_MON]
#define CPUFeature_MOVBE(FEATURES)    FEATURES[NaClCPUFeatureX86_MOVBE]
#define CPUFeature_OSXSAVE(FEATURES)  FEATURES[NaClCPUFeatureX86_OSXSAVE]
#define CPUFeature_POPCNT(FEATURES)   FEATURES[NaClCPUFeatureX86_POPCNT]
#define CPUFeature_PRE(FEATURES)      FEATURES[NaClCPUFeatureX86_PRE]
#define CPUFeature_SSE(FEATURES)      FEATURES[NaClCPUFeatureX86_SSE]
#define CPUFeature_SSE2(FEATURES)     FEATURES[NaClCPUFeatureX86_SSE2]
#define CPUFeature_SSE3(FEATURES)     FEATURES[NaClCPUFeatureX86_SSE3]
#define CPUFeature_SSE41(FEATURES)    FEATURES[NaClCPUFeatureX86_SSE41]
#define CPUFeature_SSE42(FEATURES)    FEATURES[NaClCPUFeatureX86_SSE42]
#define CPUFeature_SSE4A(FEATURES)    FEATURES[NaClCPUFeatureX86_SSE4A]
#define CPUFeature_SSSE3(FEATURES)    FEATURES[NaClCPUFeatureX86_SSSE3]
#define CPUFeature_TBM(FEATURES)      FEATURES[NaClCPUFeatureX86_TBM]
#define CPUFeature_TSC(FEATURES)      FEATURES[NaClCPUFeatureX86_TSC]
/*
 * We allow tzcnt unconditionally
 * See http://code.google.com/p/nativeclient/issues/detail?id=2869
 */
#define CPUFeature_TZCNT(FEATURES)    TRUE
#define CPUFeature_x87(FEATURES)      FEATURES[NaClCPUFeatureX86_x87]
#define CPUFeature_XOP(FEATURES)      FEATURES[NaClCPUFeatureX86_XOP]

/* Remember some information about instruction for further processing.  */
#define GET_REX_PREFIX() rex_prefix
#define SET_REX_PREFIX(PREFIX_BYTE) rex_prefix = (PREFIX_BYTE)
#define GET_VEX_PREFIX2() vex_prefix2
#define SET_VEX_PREFIX2(PREFIX_BYTE) vex_prefix2 = (PREFIX_BYTE)
#define GET_VEX_PREFIX3() vex_prefix3
#define SET_VEX_PREFIX3(PREFIX_BYTE) vex_prefix3 = (PREFIX_BYTE)
#define SET_MODRM_BASE(REG_NUMBER) base = (REG_NUMBER)
#define SET_MODRM_INDEX(REG_NUMBER) index = (REG_NUMBER)

/* Ignore this information for now.  */
#define SET_DATA16_PREFIX(STATUS)
#define SET_REPZ_PREFIX(STATUS)
#define SET_REPNZ_PREFIX(STATUS)
#define SET_MODRM_SCALE(VALUE)
#define SET_DISPLACEMENT_POINTER(POINTER)
#define SET_IMMEDIATE_POINTER(POINTER)
#define SET_SECOND_IMMEDIATE_POINTER(POINTER)

/*
 * Collect information about anyfields (offsets and immediates).
 * Note: we use += below instead of |=. This means two immediate fields will
 * be treated as one.  It's not important for safety.
 */
#define SET_DISPLACEMENT_FORMAT(FORMAT) SET_DISPLACEMENT_FORMAT_##FORMAT
#define SET_DISPLACEMENT_FORMAT_DISPNONE
#define SET_DISPLACEMENT_FORMAT_DISP8 \
  (instruction_info_collected += DISPLACEMENT_8BIT)
#define SET_DISPLACEMENT_FORMAT_DISP32 \
  (instruction_info_collected += DISPLACEMENT_32BIT)
#define SET_IMMEDIATE_FORMAT(FORMAT) SET_IMMEDIATE_FORMAT_##FORMAT
/* imm2 field is a flag, not accumulator, like other immediates  */
#define SET_IMMEDIATE_FORMAT_IMM2 \
  (instruction_info_collected |= IMMEDIATE_2BIT)
#define SET_IMMEDIATE_FORMAT_IMM8 \
  (instruction_info_collected += IMMEDIATE_8BIT)
#define SET_IMMEDIATE_FORMAT_IMM16 \
  (instruction_info_collected += IMMEDIATE_16BIT)
#define SET_IMMEDIATE_FORMAT_IMM32 \
  (instruction_info_collected += IMMEDIATE_32BIT)
#define SET_IMMEDIATE_FORMAT_IMM64 \
  (instruction_info_collected += IMMEDIATE_64BIT)
#define SET_SECOND_IMMEDIATE_FORMAT(FORMAT) \
  SET_SECOND_IMMEDIATE_FORMAT_##FORMAT
#define SET_SECOND_IMMEDIATE_FORMAT_IMM8 \
    (instruction_info_collected += SECOND_IMMEDIATE_8BIT)
#define SET_SECOND_IMMEDIATE_FORMAT_IMM16 \
    (instruction_info_collected += SECOND_IMMEDIATE_16BIT)

/*
 * Mark the destination of a jump instruction and make an early validity check:
 * jump target outside of given code region must be aligned.
 *
 * Returns TRUE iff the jump passes the early validity check.
 */
static FORCEINLINE int MarkJumpTarget(size_t jump_dest,
                                      bitmap_word *jump_dests,
                                      size_t size) {
  if ((jump_dest & kBundleMask) == 0) {
    return TRUE;
  }
  if (jump_dest >= size) {
    return FALSE;
  }
  BitmapSetBit(jump_dests, jump_dest);
  return TRUE;
}

/*
 * Mark the given address as valid jump target address.
 */
static FORCEINLINE void MarkValidJumpTarget(size_t address,
                                            bitmap_word *valid_targets) {
  BitmapSetBit(valid_targets, address);
}

/*
 * Mark the given address as invalid jump target address (that is: unmark it).
 */
static FORCEINLINE void UnmarkValidJumpTarget(size_t address,
                                              bitmap_word *valid_targets) {
  BitmapClearBit(valid_targets, address);
}

/*
 * Mark the given addresses as invalid jump target addresses (that is: unmark
 * them).
 */
static FORCEINLINE void UnmarkValidJumpTargets(size_t address,
                                               size_t bytes,
                                               bitmap_word *valid_targets) {
  BitmapClearBits(valid_targets, address, bytes);
}

/*
 * Compare valid_targets and jump_dests and call callback for any address in
 * jump_dests which is not present in valid_targets.
 */
static INLINE Bool ProcessInvalidJumpTargets(
    const uint8_t codeblock[],
    size_t size,
    bitmap_word *valid_targets,
    bitmap_word *jump_dests,
    ValidationCallbackFunc user_callback,
    void *callback_data) {
  size_t elements = (size + NACL_HOST_WORDSIZE - 1) / NACL_HOST_WORDSIZE;
  size_t i, j;
  Bool result = TRUE;

  for (i = 0; i < elements; i++) {
    bitmap_word jump_dest_mask = jump_dests[i];
    bitmap_word valid_target_mask = valid_targets[i];
    if ((jump_dest_mask & ~valid_target_mask) != 0) {
      for (j = i * NACL_HOST_WORDSIZE; j < (i + 1) * NACL_HOST_WORDSIZE; j++)
        if (BitmapIsBitSet(jump_dests, j) &&
            !BitmapIsBitSet(valid_targets, j)) {
          result &= user_callback(codeblock + j,
                                  codeblock + j,
                                  BAD_JUMP_TARGET,
                                  callback_data);
        }
    }
  }

  return result;
}


/*
 * Process rel8_operand.  Note: rip points to the beginning of the next
 * instruction here and x86 encoding guarantees rel8 field is the last one
 * in a current instruction.
 */
static FORCEINLINE void Rel8Operand(const uint8_t *rip,
                                    const uint8_t codeblock[],
                                    bitmap_word *jump_dests,
                                    size_t jumpdests_size,
                                    uint32_t *instruction_info_collected) {
  int8_t offset = rip[-1];
  size_t jump_dest = offset + (rip - codeblock);

  if (MarkJumpTarget(jump_dest, jump_dests, jumpdests_size))
    *instruction_info_collected |= RELATIVE_8BIT;
  else
    *instruction_info_collected |= RELATIVE_8BIT | DIRECT_JUMP_OUT_OF_RANGE;
}

/*
 * Process rel32_operand.  Note: rip points to the beginning of the next
 * instruction here and x86 encoding guarantees rel32 field is the last one
 * in a current instruction.
 */
static FORCEINLINE void Rel32Operand(const uint8_t *rip,
                                     const uint8_t codeblock[],
                                     bitmap_word *jump_dests,
                                     size_t jumpdests_size,
                                     uint32_t *instruction_info_collected) {
  int32_t offset =
      rip[-4] + 256U * (rip[-3] + 256U * (rip[-2] + 256U * (rip[-1])));
  size_t jump_dest = offset + (rip - codeblock);

  if (MarkJumpTarget(jump_dest, jump_dests, jumpdests_size))
    *instruction_info_collected |= RELATIVE_32BIT;
  else
    *instruction_info_collected |= RELATIVE_32BIT | DIRECT_JUMP_OUT_OF_RANGE;
}

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_VALIDATOR_INTERNAL_H_ */
