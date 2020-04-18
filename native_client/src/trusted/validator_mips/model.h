/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_MODEL_H
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_MODEL_H

/*
 * Models instructions and decode results.
 *
 * TODO(petarj): This file and the classes have been based on the same named
 * module within validator_arm folder. The code has been reduced and changed
 * slightly to accommodate some style issues. There still may be room for
 * improvement as we have not investigated performance impact of this classes.
 * Thus, this should be done later and the code should be revisited.
 */

#include <stdint.h>

namespace nacl_mips_dec {

/*
 * A value class that names a single register.  We could use a typedef for this,
 * but it introduces some ambiguity problems because of the implicit conversion
 * to/from int.
 *
 * May be implicitly converted to a single-entry RegisterList (see below).
 */
class Register {
 public:
  typedef uint8_t Number;
  explicit inline Register(uint32_t);

  /*
   * Produces the bitmask used to represent this register.
   */
  inline uint32_t Bitmask() const;

  inline bool Equals(const Register &) const;

  static const Number kJumpMask       = 14;  // $t6 = holds mask for jr.
  static const Number kLoadStoreMask  = 15;  // $t7 = holds mask for load/store.
  static const Number kTls            = 24;  // $t8 = holds address of
                                             // tls_value1 in NaClThreadContext.
  static const Number kSp             = 29;  // Stack pointer.

  static const Number kNone           = 32;

  /*
   * A special value used to indicate that a register field is not used.
   * This is specially chosen to ensure that bitmask() == 0, so it can be added
   * to any RegisterList with no effect.
   */
  static Register None() { return Register(kNone); }

  /* Registers with special meaning in our model: */
  static Register JumpMask() { return Register(kJumpMask); }
  static Register LoadStoreMask() { return Register(kLoadStoreMask); }
  static Register Tls() { return Register(kTls); }
  static Register Sp() { return Register(kSp); }

 private:
  uint32_t _number;
};

/*
 * A collection of Registers.  Used to describe the side effects of operations.
 *
 * Note that this is technically a set, not a list -- but RegisterSet is a
 * well-known term that means something else.
 */
class RegisterList {
 public:
  /*
   * Produces a RegisterList that contains the registers specified in the
   * given bitmask.  To indicate rN, the bitmask must include (1 << N).
   */
  explicit inline RegisterList(uint32_t bitmask);

  /*
   * Produces a RegisterList containing a single register.
   *
   * Note that this is an implicit constructor.  This is okay in this case
   * because
   *  - It converts between two types that we control,
   *  - It converts at most one step (no implicit conversions to Register),
   *  - It inlines to a single machine instruction,
   *  - The readability gain in inst_classes.cc is large.
   */
  explicit inline RegisterList(const Register r);

  /*
   * Checks whether this list contains the given register.
   */
  inline bool operator[](const Register) const;

  // Checks whether this list contains all the registers in the operand.
  inline bool ContainsAll(RegisterList) const;

  // Checks whether this list contains any of the registers in the operand.
  inline bool ContainsAny(RegisterList) const;

  inline const RegisterList operator&(const RegisterList) const;

  /*
   * A list containing every possible register, even some we do not define.
   * Used exclusively as a bogus scary return value for forbidden instructions.
   */
  static RegisterList Everything() { return RegisterList(-1); }

  /* A list of registers that can not be modified by untrusted code. */
  static RegisterList ReservedRegs() {
    return RegisterList(((1 << Register::kJumpMask) |
                         (1 << Register::kLoadStoreMask) |
                         (1 << Register::kTls)));
  }

  /* A list of registers that can be used for addressing memory locations. */
  static RegisterList DataAddrRegs() {
    return RegisterList(((1 << Register::kSp) |
                         (1 << Register::kTls)));
  }

 private:
  uint32_t _bits;
};

/*
 * A 32-bit Mips instruction of unspecified type.
 *
 * This class is designed for efficiency:
 *  - Its public methods for bitfield extraction are short and inline.
 *  - It has no vtable, so on 32-bit platforms it's exactly the size of the
 *    instruction it models.
 */
class Instruction {
 public:
  explicit inline Instruction(uint32_t bits);

  /*
   * Extracts a range of contiguous bits, right-justifies it, and returns it.
   * Note that the range follows hardware convention, with the high bit first.
   */
  inline uint32_t Bits(int hi, int lo) const;

  /*
   * A convenience method that's exactly equivalent to
   *   Register(instruction.bits(hi, lo))
   *
   * This sequence is quite common in inst_classes.cc.
   */
  inline const Register Reg(int hi, int lo) const;

  /*
   * Extracts a single bit (0 - 31).
   */
  inline bool Bit(int index) const;

  /*
   * Returns an integer equivalent of this instruction, masked by the given
   * mask.  Used during decoding to test bitfields.
   */
  inline uint32_t operator&(uint32_t) const;

 private:
  uint32_t _bits;
};

uint32_t const kInstrSize   = 4;
uint32_t const kInstrAlign  = 0xFFFFFFFC;
uint32_t const kBundleAlign = 0xFFFFFFF0;

// Opcode for nop instruction.
uint32_t const kNop = 0x0;

}  // namespace nacl_mips_dec

// Definitions for our inlined functions.
#include "native_client/src/trusted/validator_mips/model-inl.h"

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_MODEL_H
