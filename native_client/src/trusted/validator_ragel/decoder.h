/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Data structures for decoding instructions.  Includes definitions which are
 * used by all decoders (full-blown standalone one and reduced one in validator,
 * both ia32 version and x86-64 version).
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_DECODER_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_DECODER_H_

#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"

EXTERN_C_BEGIN

/*
 * Instruction operand FORMAT: register size (8-bit, 32-bit, MMX, XXM, etc), or
 * in-memory structure (far pointer, 256-bit SIMD operands, etc).
 */
enum OperandFormat {
  /*
   * These are for general-purpose registers, memory access and immediates.
   * They are not used for XMM, MMX etc.
   */
  OPERAND_FORMAT_8_BIT,
  OPERAND_FORMAT_16_BIT,
  OPERAND_FORMAT_32_BIT,
  OPERAND_FORMAT_64_BIT,

  /* Non-GP registers.  */
  OPERAND_FORMAT_ST,               /* Any X87 register.                       */
  OPERAND_FORMAT_MMX,              /* MMX registers: %mmX.                    */
  OPERAND_FORMAT_XMM,              /* XMM register: %xmmX.                    */
  OPERAND_FORMAT_YMM,              /* YMM registers: %ymmX.                   */
  OPERAND_FORMAT_SEGMENT_REGISTER, /* Operand is segment register: %es...%gs. */
  OPERAND_FORMAT_CONTROL_REGISTER, /* Operand is control register: %crX.      */
  OPERAND_FORMAT_DEBUG_REGISTER,   /* Operand is debug register: %drX.        */
  OPERAND_FORMAT_TEST_REGISTER,    /* Operand is test register: %trX.         */

  /*
   * All other operand format are not used as register arguments.  These are
   * immediates or memory.
   */
  OPERAND_FORMATS_REGISTER_MAX = OPERAND_FORMAT_TEST_REGISTER + 1,

  /* See VPERMIL2Px instruction for description of 2-bit operand type. */
  OPERAND_FORMAT_2_BIT = OPERAND_FORMATS_REGISTER_MAX,
  OPERAND_FORMAT_MEMORY
};

/*
 * Instruction operand NAME: register number (NC_REG_RAX means any of the
 * following registers:
 * %al/%ax/%eax/%rax/%st(0)/%mm0/%xmm0/%ymm0/%es/%cr0/%db0/%tr0), or
 * non-register operand (NC_REG_RM means address in memory specified via "ModR/M
 * byte" (plus may be "SIB byte" or displacement), NC_REG_DS_RBX is special
 * operand of "xlat" instruction, NC_REG_ST is to of x87 stack and so on - see
 * below for for the full list).
 */
enum OperandName {
  /* First 16 registers are compatible with encoding of registers in x86 ABI. */
  NC_REG_RAX,
  NC_REG_RCX,
  NC_REG_RDX,
  NC_REG_RBX,
  NC_REG_RSP,
  NC_REG_RBP,
  NC_REG_RSI,
  NC_REG_RDI,
  NC_REG_R8,
  NC_REG_R9,
  NC_REG_R10,
  NC_REG_R11,
  NC_REG_R12,
  NC_REG_R13,
  NC_REG_R14,
  NC_REG_R15,
  NC_REG_MASK = 0x0f,
  /* These are different kinds of operands used in special cases.             */
  NC_REG_RM,        /* Address in memory via ModR/M (+SIB).                   */
  NC_REG_RIP,       /* RIP - used as base in x86-64 mode.                     */
  NC_REG_RIZ,       /* EIZ/RIZ - used as "always zero index" register.        */
  NC_REG_IMM,       /* Fixed value in imm field.                              */
  NC_REG_IMM2,      /* Fixed value in second imm field.                       */
  NC_REG_DS_RBX,    /* For xlat: %ds:(%rbx).                                  */
  NC_REG_ES_RDI,    /* For string instructions: %es:(%rsi).                   */
  NC_REG_DS_RSI,    /* For string instructions: %ds:(%rdi).                   */
  NC_REG_PORT_DX,   /* 16-bit DX: for in/out instructions.                    */
  NC_NO_REG,        /* For modrm: both index and base can be absent.          */
  NC_REG_ST,        /* For x87 instructions: implicit %st.                    */
  NC_JMP_TO         /* Operand is jump target address: usually %rip+offset.   */
};

/*
 * Displacement can be of four different sizes in x86 instruction set: nothing,
 * 8-bit, 16-bit, 32-bit, and 64-bit.  These are traditionally treated slightly
 * differently by decoders: 8-bit are usually printed as signed offset, while
 * 32-bit (in ia32 mode) and 64-bit (in amd64 mode) are printed as unsigned
 * offset.
 */
enum DisplacementMode {
  DISPNONE,
  DISP8,
  DISP16,
  DISP32,
  DISP64
};

/*
 * Information about decoded instruction: name, operands, prefixes, etc.
 */
struct Instruction {
  const char *name;
  unsigned char operands_count;
  struct {
    unsigned char rex;        /* Mostly to distingush cases like %ah vs %spl. */
    Bool rex_b_spurious;
    Bool rex_x_spurious;
    Bool rex_r_spurious;
    Bool rex_w_spurious;
    Bool data16;              /* "Normal", non-rex prefixes. */
    Bool lock;
    Bool repnz;
    Bool repz;
    Bool branch_not_taken;
    Bool branch_taken;
  } prefix;
  struct {
    enum OperandName name;
    enum OperandFormat format;
  } operands[5];
  struct {
    enum OperandName base;
    enum OperandName index;
    int scale;
    int64_t offset;
    enum DisplacementMode disp_type;
  } rm;
  uint64_t imm[2];
  const char* att_instruction_suffix;
};

/*
 * Instruction processing callback: called once for each instruction in a stream
 *
 * "begin" points to first byte of the instruction
 * "end" points to the first byte of the next instruction
 *   for single-byte instruction "begin + 1" == "end"
 * "instruction" contains detailed information about instruction
 */
typedef void (*ProcessInstructionFunc) (const uint8_t *begin,
                                        const uint8_t *end,
                                        struct Instruction *instruction,
                                        void *callback_data);

/*
 * Decoding error: called when decoder's DFA does not recognize the instruction.
 *
 * "ptr" points to the first byte rejected by DFA and can be used in more
 * advanced decoders to try to do some kind of recovery.
 */
typedef void (*ProcessDecodingErrorFunc) (const uint8_t *ptr,
                                          void *callback_data);

/*
 * kFullCPUIDFeatures is pre-defined constant of NaClCPUFeaturesX86 type with
 * all possible CPUID features enabled.
 */
extern const NaClCPUFeaturesX86 kFullCPUIDFeatures;

/*
 * Returns TRUE when piece of code is valid piece of code.
 * Returns FALSE If ragel machine does not accept piece of code.
 * process_instruction callback is called for each instruction accepted by
 * ragel machine
 */
int DecodeChunkAMD64(const uint8_t *data, size_t size,
                     ProcessInstructionFunc process_instruction,
                     ProcessDecodingErrorFunc process_error,
                     void *callback_data);

int DecodeChunkIA32(const uint8_t *data, size_t size,
                    ProcessInstructionFunc process_instruction,
                    ProcessDecodingErrorFunc process_error,
                    void *callback_data);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_DECODER_H_ */
