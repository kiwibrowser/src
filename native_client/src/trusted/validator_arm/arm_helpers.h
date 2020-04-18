/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_V2_ARM_HELPERS_H
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_V2_ARM_HELPERS_H

/* This file defines arm helper functions that are used by the
   method definitions in armv7.table. They exists because either
   (a) ARM specifications use them, or (b) the limitations of
   (dgen) bit expressions (defined in dgen_input.py) do not allow us
   to specify the function explicitly.
*/

#include "native_client/src/trusted/validator_arm/model.h"

namespace nacl_arm_dec {

// Function to get the number of general purpose registers in
// a register list.
inline uint32_t NumGPRs(RegisterList registers) {
  return registers.numGPRs();
}

// Function to return the smallest general purpose register in
// a register list.
inline uint32_t SmallestGPR(RegisterList registers) {
  return registers.SmallestGPR();
}

// Function that returns true if the general purpose register
// in in the register list.
inline bool Contains(RegisterList registers, Register r) {
  return registers.Contains(r);
}

// Function that unions together to register lists.
inline RegisterList Union(RegisterList r1, RegisterList r2) {
  return r1.Union(r2);
}

// Function returning the register index of a register.
inline Register::Number RegIndex(Register r) {
  return r.number();
}

// Function that expands an immediate value, corresponding
// to ARMExpandImm, as described in section A5.2.4 of the manual.
inline uint32_t ARMExpandImm(uint32_t imm12) {
  uint32_t unrotated_value = imm12 & 0xFF;
  uint32_t ror_amount = ((imm12 >> 8) & 0xF) << 1;
  return (ror_amount == 0) ?
      unrotated_value :
      ((unrotated_value >> ror_amount) |
       (unrotated_value << (32 - ror_amount)));
}

// Function ARMExpandImm_C, less the carry, as described in
// section A5.2.4 of the manual.
inline uint32_t ARMExpandImm_C(uint32_t imm12) {
  return ARMExpandImm(imm12);
}

// Function that expands an imm8 to a corresponding floating point
// value with the given (n) number of bits, where n is 32 or 64.
uint64_t VFPExpandImm(uint32_t imm8, int n);

// Returns the bits used as a literal pool head.
inline uint32_t LiteralPoolHeadConstant() {
  return kLiteralPoolHead;
}

// Returns true if the UDF instruction matches encoding values we've chosen
// to be safe.
inline bool IsUDFNaClSafe(uint32_t inst_bits) {
  return inst_bits == kHaltFill || inst_bits == kAbortNow;
}

}  // namespace nacl_arm_dec

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_V2_ARM_HELPERS_H
