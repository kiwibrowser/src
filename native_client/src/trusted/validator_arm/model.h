/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_V2_MODEL_H
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_V2_MODEL_H

/*
 * Models instructions and decode results.
 *
 * Implementation Note:
 * All the classes in this file are designed to be fully inlined as 32-bit
 * (POD) integers.  The goal: to provide a nice, fluent interface for
 * describing instruction bit operations, with zero runtime cost.  Compare:
 *
 *   return (1 << ((insn >> 12) & 0xF)) | (1 << ((insn >> 19) & 0xF));
 *
 *   return insn.reg(15,12) + insn.reg(19,16);
 *
 * Both lines compile to the same machine code, but the second one is easier
 * to read, and eliminates a common error (using X in place of 1 << X).
 *
 * To this end, keep the following in mind:
 *  - Avoid virtual methods.
 *  - Mark all methods as inline.  Small method bodies may be included here,
 *    but anything nontrivial should go in model-inl.h.
 *  - Do not declare destructors.  A destructor causes an object to be passed
 *    on the stack, even when passed by value.  (This may be a GCC bug.)
 *    Adding a destructor to Instruction slowed the decoder down by 10% on
 *    gcc 4.3.3.
 */

#include <cstddef>
#include <string>
#include "native_client/src/include/arm_sandbox.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/portability_bits.h"

namespace nacl_arm_dec {

class RegisterList;

// Defines the architecture version of the ARM processor. Currently assumes
// always 7.
// TODO(karl): Generalize this to handle multiple versions, once we know how
//             to do this.
inline uint32_t ArchVersion() {
  return 7;
}

// Returns FALSE if all of the 32 registers D0-D31 can be accessed, and TRUE if
// only the 16 registers D0-D15 can be accessed.
inline bool VFPSmallRegisterBank() {
  // Note: The minimum supported platform, which is checked in (see
  // native_client/src/trusted/platform_qualify/arch/arm/nacl_arm_qualify.h)
  // allows access to all 32 registers D0-D31.
  return false;
}

// A (POD) value class that names a single register.  We could use a typedef
// for this, but it introduces some ambiguity problems because of the
// implicit conversion to/from int.
//
// The 32-bit ARM v7 ARM and Thumb instruction sets have:
//   - 15 32-bit GPRs, with:
//       - R13/R14/R15 serving as SP/LR/PC.
//       - R8-R15 being inaccessible in most 16-bit Thumb instructions.
//   - FPRs, with:
//       - 16 128-bit quadword registers, denoted Q0-Q15.
//       - 32 64-bit doubleword registers, denoted D0-D31.
//       - 32 32-bit single word registers, denoted S0-S31.
//       - Note that the above holds true for only some ARM processors:
//         different VFP implementations might have only D0-D15 with S0-S31,
//         and Advanced SIMD support is required for the quadword registers.
//       - The above FPRs are overlaid and accessing S/D/Q registers
//         interchangeably is sometimes expected by the ARM ISA.
//         Specifically S0-S3 correspond to D0-D1 as well as Q0,
//         S4-S7 to D2-D3 and Q1, and so on.
//         D16-D31 and Q8-Q15 do not correspond to any single word register.
//
// TODO(jfb) detail aarch64 when we support ARM v8.
class Register {
 public:
  typedef uint8_t Number;
  // RegisterMask wide enough for aarch32, including "special" registers.
  // TODO(jfb) Update for aarch64.
  typedef uint32_t Mask;
    // aarch32 only has 16 GPRs, we steal other registers for flags and such.
  // TODO(jfb) Update for aarch64.
  static const Mask kGprMask = 0xFFFF;
  static const Mask kAllMask = static_cast<Mask>(-1);

  Register() : number_(kNone) {}
  explicit Register(Number number) : number_(number) {}
  Register(const Register& r) : number_(r.number_) {}

  // Produces the bitmask used to represent this register, in both RegisterList
  // and ARM's LDM instruction.
  Mask BitMask() const {
    return (number_ == kNone) ? 0 : (1u << number_);
  }

  Number number() const { return number_; }
  bool Equals(const Register& r) const { return number_ == r.number_; }

  Register& Copy(const Register& r) {
    number_ = r.number_;
    return *this;
  }

  const char* ToString() const;

  // TODO(jfb) Need different numbers for aarch64.
  static const Number kTp = 9;   // Thread local pointer.
  static const Number kSp = 13;  // Stack pointer.
  static const Number kLr = 14;  // Link register.
  static const Number kPc = 15;  // Program counter.
  static const Number kNumberGPRs = 16;  // Number of General purpose registers.
  static const Number kConditions = 16;
  static const Number kNone = 32;  // Out of GPR and FPR range.

  // A special value used to indicate that a register field is not used.
  // This is specially chosen to ensure that bitmask() == 0, so it can be added
  // to any RegisterList with no effect.
  static Register None() { return Register(kNone); }

  // The conditions (i.e. APSR N, Z, C, and V) are collectively modeled as
  // a single register, out of the usual ARM GPR range.
  // These bits of the APSR register are separately tracked, so we can
  // test when any of the 4 bits (and hence conditional execution) is
  // affected. If you need to track other bits in the APSR, add them as
  // a separate register.
  static Register Conditions() { return Register(kConditions); }

  // Registers with special meaning in our model:
  static Register Tp() { return Register(kTp); }
  static Register Sp() { return Register(kSp); }
  static Register Lr() { return Register(kLr); }
  static Register Pc() { return Register(kPc); }

 private:
  Number number_;
  Register& operator=(const Register& r);  // Disallow assignment.
};


// A collection of Registers.  Used to describe the side effects of operations.
//
// Note that this is technically a set, not a list -- but RegisterSet is a
// well-known term that means something else.
class RegisterList {
 public:
  // Defines an empty register list.
  RegisterList() : bits_(0) {}

  // Produces a RegisterList that contains the registers specified in the
  // given bitmask.  To indicate rN, the bitmask must include (1 << N).
  explicit RegisterList(Register::Mask bitmask) : bits_(bitmask) {}

  RegisterList(const RegisterList& other) : bits_(other.bits_) {}

  // Produces a RegisterList containing a single register.
  explicit RegisterList(const Register& r) : bits_(r.BitMask()) {}

  // Checks whether this list contains the given register.
  bool Contains(const Register& r) const {
    return (bits_ & r.BitMask()) != 0;
  }

  // Checks whether this list contains all the registers in the operand.
  bool ContainsAll(const RegisterList& other) const {
    return (bits_ & other.bits_) == other.bits_;
  }

  // Checks whether this list contains any of the registers in the operand.
  bool ContainsAny(const RegisterList& other) const {
    return (bits_ & other.bits_) != 0;
  }

  // Returns true if the two register lists are identical.
  bool Equals(const RegisterList& other) const {
    return bits_ == other.bits_;
  }

  // Adds a register to the register list.
  RegisterList& Add(const Register& r) {
    bits_ |= r.BitMask();
    return *this;
  }

  // Removes a register from the register list.
  RegisterList& Remove(const Register& r) {
    bits_ &= ~r.BitMask();
    return *this;
  }

  // Unions this given register list into this.
  RegisterList& Union(const RegisterList& other) {
    bits_ |= other.bits_;
    return *this;
  }

  // Intersects the given register list into this.
  RegisterList& Intersect(const RegisterList& other) {
    bits_ &= other.bits_;
    return *this;
  }

  // Copies the other register list into this.
  RegisterList& Copy(const RegisterList& other) {
    bits_ = other.bits_;
    return *this;
  }

  // Returns the bits defined in the register list.
  Register::Mask bits() const {
    return bits_;
  }

  // Number of ARM GPRs in the list.
  uint32_t numGPRs() const {
    uint16_t gprs = bits_ & Register::kGprMask;
    return nacl::PopCount(gprs);
  }

  // Returns the smallest GPR register in the list.
  Register::Number SmallestGPR() const;

  // A list containing every possible register, even some we don't define.
  // Used exclusively as a bogus scary return value for forbidden instructions.
  static RegisterList Everything() { return RegisterList(Register::kAllMask); }

  // A special register list to communicate registers that can't be changed
  // when doing dynamic code replacement.
  static RegisterList DynCodeReplaceFrozenRegs() {
    return RegisterList((1 << Register::kPc) |
                        (1 << Register::kLr) |
                        (1 << Register::kSp) |
                        (1 << Register::kTp));
  }

 private:
  Register::Mask bits_;
  RegisterList& operator=(const RegisterList& r);  // Disallow assignment.
};


// The number of bits in an ARM instruction.
static const int kArm32InstSize = 32;

// Special ARM instructions for sandboxing.
static const uint32_t kLiteralPoolHead = NACL_INSTR_ARM_LITERAL_POOL_HEAD;
static const uint32_t kBreakpoint = NACL_INSTR_ARM_BREAKPOINT;
static const uint32_t kHaltFill = NACL_INSTR_ARM_HALT_FILL;
static const uint32_t kAbortNow = NACL_INSTR_ARM_ABORT_NOW;
static const uint32_t kFailValidation = NACL_INSTR_ARM_FAIL_VALIDATION;

// Not-so-special instructions.
static const uint32_t kNop = NACL_INSTR_ARM_NOP;

// Models a 32-bit ARM instruction.
//
// This class is designed for efficiency:
//  - Its public methods for bitfield extraction are short and inline.
//  - It has no vtable, so on 32-bit platforms it's exactly the size of the
//    instruction it models.
class Instruction {
 public:
  Instruction() : bits_(0) {}

  Instruction(const Instruction& inst) : bits_(inst.bits_) {}

  // Creates an a 32-bit ARM instruction.
  explicit Instruction(uint32_t bits)  : bits_(bits) {}

  // Returns the entire sequence of bits defined by an instruction.
  uint32_t Bits() const {
    return bits_;
  }

  // Mask for the lowest bits.
  static uint32_t LowestBitsMask(int bit_count) {
    return (~(uint32_t)0) >> (32 - bit_count);
  }

  // Extracts a range of contiguous bits, right-justifies it, and returns it.
  // Note that the range follows hardware convention, with the high bit first.
  uint32_t Bits(int hi, int lo) const {
    // When both arguments are constant (the usual case), this can be inlined
    // as
    //    ubfx r0, r0, #hi, #(hi+1-lo)
    //
    // Curiously, even at aggressive optimization levels, GCC 4.3.2 generates a
    // less-efficient sequence of ands, bics, and shifts.
    //
    // TODO(jfb) Validate and fix this: this function is called very often and
    //           could speed up validation quite a bit.
    uint32_t right_justified = bits_ >> lo;
    int bit_count = hi - lo + 1;
    return right_justified & LowestBitsMask(bit_count);
  }

  // Changes the range of contiguous bits, with the given value.
  // Note: Assumes the value fits, if not, it is truncated.
  void SetBits(int hi, int lo, uint32_t value) {
    // TODO(jfb) Same as the above function, this should generate a BFI
    //           when hi and lo are compile-time constants.
    //
    // Compute bit mask for range of bits.
    int bit_count = hi - lo + 1;
    uint32_t clear_mask = LowestBitsMask(bit_count);
    // Remove from the value any bits out of range, then shift to location
    // to add.
    value = (value & clear_mask) << lo;
    // Shift mask to corresponding position and replace bits with value.
    clear_mask = ~(clear_mask << lo);
    bits_ = (bits_ & clear_mask) | value;
  }

  // A convenience method for converting bits to a register.
  const Register Reg(int hi, int lo) const {
    return Register(Bits(hi, lo));
  }

  // Extracts a single bit (0 - 31).
  bool Bit(int index) const {
    return (bits_ >> index) & 1;
  }

  // Sets the specified bit to the specified value for an ARM instruction.
  void SetBit(int index, bool value) {
    uint32_t mask = (1 << index);
    if (value) {
      // Set to 1
      bits_ |= mask;
    } else {
      // Set to 0
      bits_ &= ~mask;
    }
  }

  // Possible values for the condition field, from the ARM ARM section A8.3.
  // Conditional execution is determined by the APSR's condition flags: NZCV.
  enum Condition {
    EQ = 0x0,  // Equal                         |  Z == 1
    NE = 0x1,  // Not equal                     |  Z == 0
    CS = 0x2,  // Carry set                     |  C == 1
    CC = 0x3,  // Carry clear                   |  C == 0
    MI = 0x4,  // Minus, negative               |  N == 1
    PL = 0x5,  // Plus, positive or zero        |  N == 0
    VS = 0x6,  // Overflow                      |  V == 1
    VC = 0x7,  // No overflow                   |  V == 0
    HI = 0x8,  // Unsigned higher               |  C == 1 && Z == 0
    LS = 0x9,  // Unsigned lower or same        |  C == 0 || Z == 1
    GE = 0xA,  // Signed greater than or equal  |  N == V
    LT = 0xB,  // Signed less than              |  N != V
    GT = 0xC,  // Signed greater than           |  Z == 0 && N == V
    LE = 0xD,  // Signed less than or equal     |  Z == 1 || N != V
    AL = 0xE,  // Always (unconditional)        |  Any
    UNCONDITIONAL = 0xF,  // Equivalent to AL -- converted in our API
    // Aliases:
    HS = CS,  // Unsigned higher or same
    LO = CC   // Unsigned lower
  };

  // Defines the size of enumerated type Condition, minus
  // UNCONDITIONAL (assuming one uses GetCondition() to get the
  // condition of an instruction).
  static const size_t kConditionSize = 15;

  static const char* ToString(Condition cond);

  static Condition Next(Condition cond) {
    return static_cast<Condition>(static_cast<int>(cond) + 1);
  }

  // Extracts the condition field.  UNCONDITIONAL is converted to AL -- in the
  // event that you need to distinguish, (1) make sure that's really true and
  // then (2) explicitly extract bits(31,28).
  Condition GetCondition() const {
    Instruction::Condition cc = Instruction::Condition(Bits(31, 28));
    if (cc == Instruction::UNCONDITIONAL) {
      return Instruction::AL;
    }
    return cc;
  }

  // Returns true if this and the given instruction are equal.
  bool Equals(const Instruction& inst) const {
    return bits_ == inst.bits_;
  }

  // Copies insn into this.
  Instruction& Copy(const Instruction& insn) {
    bits_ = insn.bits_;
    return *this;
  }

 private:
  uint32_t bits_;
  Instruction& operator=(const Instruction& insn);  // Disallow assignment.
};

// Checks if instruction is a valid constant pool head.
inline bool IsBreakPointAndConstantPoolHead(Instruction i) {
  return ((i.Bits(31, 0) == kLiteralPoolHead) ||
          (i.Bits(31, 0) == kBreakpoint));
}

// Same as above, but with integer contents of instruction as argument.
inline bool IsBreakPointAndConstantPoolHead(uint32_t i) {
  return IsBreakPointAndConstantPoolHead(Instruction(i));
}

}  // namespace nacl_arm_dec

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_V2_MODEL_H
