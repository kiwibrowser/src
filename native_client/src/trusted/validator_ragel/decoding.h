/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file contains common parts of ia32 and x86-64 decoder and validator
 * internals (inline functions which are used to pull useful information from
 * "well-known" bytes of the instruction: REX and VEX prefixes, ModR/M byte and
 * so on).
 *
 * See full description in AMD/Intel manuals.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_DECODING_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_DECODING_H_

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/validator_ragel/decoder.h"

#if NACL_WINDOWS
# define FORCEINLINE __forceinline
#else
# define FORCEINLINE __inline __attribute__ ((always_inline))
#endif


/*
 * Opcode-with-register byte format:
 *
 * bits 0-2: register number
 * bits 3-7: actual opcode
 */
static FORCEINLINE uint8_t RegFromOpcode(uint8_t modrm) {
  return modrm & 0x07;
}

/*
 * ModRM byte format:
 *
 * bits 0-2: r/m
 * bits 3-5: reg
 * bits 6-7: mod
 */
static FORCEINLINE uint8_t ModFromModRM(uint8_t modrm) {
  return modrm >> 6;
}

static FORCEINLINE uint8_t RegFromModRM(uint8_t modrm) {
  return (modrm >> 3) & 0x07;
}

static FORCEINLINE uint8_t RMFromModRM(uint8_t modrm) {
  return modrm & 0x07;
}

/*
 * SIB byte format:
 *
 * bits 0-2: base
 * bits 3-5: index
 * bits 6-7: scale
 */
static FORCEINLINE uint8_t ScaleFromSIB(uint8_t sib) {
  return sib >> 6;
}

static FORCEINLINE uint8_t IndexFromSIB(uint8_t sib) {
  return (sib >> 3) & 0x07;
}

static FORCEINLINE uint8_t BaseFromSIB(uint8_t sib) {
  return sib & 0x07;
}

/*
 * REX byte format:
 *
 * bit 0: B (Base)
 * bit 1: X (indeX)
 * bit 2: R (Register)
 * bit 3: W (Wide)
 * 4-7 bits: 0x4 (REX signature)
 */

enum {
  REX_B = 1,
  REX_X = 2,
  REX_R = 4,
  REX_W = 8
};

/* How much to add to "base register" number: 0 or 8  */
static FORCEINLINE uint8_t BaseExtentionFromREX(uint8_t rex) {
  return (rex & REX_B) << 3;
}

/* How much to add to "index register" number: 0 or 8  */
static FORCEINLINE uint8_t IndexExtentionFromREX(uint8_t rex) {
  return (rex & REX_X) << 2;
}

/* How much to add to "register operand" number: 0 or 8  */
static FORCEINLINE uint8_t RegisterExtentionFromREX(uint8_t rex) {
  return (rex & REX_R) << 1;
}

/*
 * VEX 2nd byte format:
 *
 * bits 0-4: opcode selector
 * bit 5: inverted B (Base)
 * bit 6: inverted X (indeX)
 * bit 7: inverted R (Register)
 *
 */

enum {
  VEX_MAP1 = 0x01,
  VEX_MAP2 = 0x02,
  VEX_MAP3 = 0x03,
  VEX_MAP8 = 0x08,
  VEX_MAP9 = 0x09,
  VEX_MAPA = 0x0a,
  VEX_B    = 0x20,
  VEX_X    = 0x40,
  VEX_R    = 0x80
};

/* How much to add to "base register" number: 0 or 8  */
static FORCEINLINE uint8_t BaseExtentionFromVEX(uint8_t vex2) {
  return ((~vex2) & VEX_B) >> 2;
}

/* How much to add to "index register" number: 0 or 8  */
static FORCEINLINE uint8_t IndexExtentionFromVEX(uint8_t vex2) {
  return ((~vex2) & VEX_X) >> 3;
}

/* How much to add to "register operand" number: 0 or 8  */
static FORCEINLINE uint8_t RegisterExtentionFromVEX(uint8_t vex2) {
  return ((~vex2) & VEX_R) >> 4;
}

/*
 * VEX 3rd byte format:
 *
 * bits 0-1: pp (Packed Prefix)
 * bit 2: L (Long)
 * bits 3-6: negated vvvv (register number)
 * bit 7: W (Wide)
 */

enum {
  VEX_PP_NONE = 0x00,
  VEX_PP_0X66 = 0x01,
  VEX_PP_0XF3 = 0x02,
  VEX_PP_0XF2 = 0x03,
  VEX_L       = 0x04,
  VEX_VVVV    = 0x78,
  VEX_W       = 0x80
};


static FORCEINLINE uint8_t GetOperandFromVexIA32(uint8_t vex3) {
  return ((~vex3) & VEX_VVVV) >> 3;
}

static FORCEINLINE uint8_t GetOperandFromVexAMD64(uint8_t vex3) {
  return ((~vex3) & VEX_VVVV) >> 3;
}

/*
 * is4/is5 byte format:
 *
 * bits 0-1: imm2 or zero
 * bits 2-3: 0
 * bits 4-7: register number
 */
static FORCEINLINE uint8_t RegisterFromIS4(uint8_t is4) {
  return is4 >> 4;
}

/*
 * SignExtendXXBit is used to sign-extend XX-bit value to unsigned 64-bit value.
 *
 * To do that you need to pass unsigned value of smaller then 64-bit size
 * to this function: it will be converted to signed value and then
 * sign-extended to become 64-bit value.
 *
 * Return values can be restricted to smaller unsigned type when needed (which
 * is safe according to the C language specification: see 6.2.1.2 in C90 and
 * 6.3.1.3.2 in C99 specification).
 *
 * Note that these operations are safe but slightly unusual: they come very
 * close to the edge of what "well-behaved C program is not supposed to do",
 * but they stay on the "safe" side of this boundary.  Specifically: this
 * (conversion to intXX_t) behavior triggers "implementation-defined behavior"
 * (see 6.2.1.2 in C90 specification and 6.3.1.3.3 in C99 specification) which
 * sounds suspiciously similar to the dreaded "undefined behavior", but in
 * reality these two are quite different: any program which triggers "undefined
 * behavior" is not a valid C program at all, but program which triggers
 * "implementation-defined behavior" is quite valid C program.  What this
 * program actually *does* depends on the specification of a given C compiler:
 * each particular implementation must decide for itself what it'll do in this
 * particular case and *stick* *to* *it*.  If the implementation actually uses
 * two's-complement negative numbers (and all the implementation which can
 * compile this code *must* support two's-complement arythmetic - see 7.18.1.1
 * in C99 specification) then the easiest thing to do is to do what we need
 * here - this is what all known compilers for all known platforms are actually
 * doing.
 *
 * Conversion from intXX_t to uint64_t is always safe (same as before: see
 * see 6.2.1.2 in C90 specification and 6.3.1.3.2 in C99 specification).
 */
static FORCEINLINE uint64_t SignExtend8Bit(uint64_t value) {
  return (int8_t)value;
}

static FORCEINLINE uint64_t SignExtend16Bit(uint64_t value) {
  return (int16_t)value;
}

static FORCEINLINE uint64_t SignExtend32Bit(uint64_t value) {
  return (int32_t)value;
}

static FORCEINLINE uint64_t AnyFieldValue8bit(const uint8_t *start) {
  return *start;
}

static FORCEINLINE uint64_t AnyFieldValue16bit(const uint8_t *start) {
  return (start[0] + 256U * start[1]);
}

static FORCEINLINE uint64_t AnyFieldValue32bit(const uint8_t *start) {
  return (start[0] + 256U * (start[1] + 256U * (start[2] + 256U * (start[3]))));
}
static FORCEINLINE uint64_t AnyFieldValue64bit(const uint8_t *start) {
  return (*start + 256ULL * (start[1] + 256ULL * (start[2] + 256ULL *
          (start[3] + 256ULL * (start[4] + 256ULL * (start[5] + 256ULL *
           (start[6] + 256ULL * start[7])))))));
}

static const uint8_t index_registers[] = {
  /* Note how NC_REG_RIZ falls out of the pattern. */
  NC_REG_RAX, NC_REG_RCX, NC_REG_RDX, NC_REG_RBX,
  NC_REG_RIZ, NC_REG_RBP, NC_REG_RSI, NC_REG_RDI,
  NC_REG_R8,  NC_REG_R9,  NC_REG_R10, NC_REG_R11,
  NC_REG_R12, NC_REG_R13, NC_REG_R14, NC_REG_R15
};

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_DECODING_H_ */
