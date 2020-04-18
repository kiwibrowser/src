/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_INST_CLASSES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_INST_CLASSES_H_

#include <climits>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/validator_arm/model.h"

/*
 * Models the "instruction classes" that the decoder produces.
 */
namespace nacl_arm_val {
class AddressSet;
class SfiValidator;
class DecodedInstruction;
class ProblemSink;
class InstructionPairMatchData;
}

namespace nacl_arm_dec {

// Used to describe whether an instruction is safe, and if not, what the issue
// is.  Only instructions that MAY_BE_SAFE should be allowed in untrusted code,
// and even those may be rejected by the validator.
//
// Note: The enumerated values are used in dgen_core.py (see class
// SafetyAction).  Be sure to update values in that class if this list
// changes, so that the two stay in sync.
//
// Note: All safety levels except MAY_BE_SAFE, also act as a violation
// (see enum Violation below). If you change this enum, also change
// Violation below.  Further, be sure to keep MAY_BE_SAFE as the last
// entry in this enum, since code (elsewhere) assumes that MAY_BE_SAFE
// appears last in the list.
enum SafetyLevel {
  // The initial value of uninitialized SafetyLevels -- treat as unsafe.
  UNINITIALIZED = 0,

  // Values put into one (or more) registers is not known, as specified
  // by the ARMv7 ISA spec.
  // See instructions VSWP, VTRN, VUZP, and VZIP for examples of this.
  UNKNOWN,
  // This instruction is left undefined by the ARMv7 ISA spec.
  UNDEFINED,
  // This instruction is not recognized by the decoder functions.
  NOT_IMPLEMENTED,
  // This instruction has unpredictable effects at runtime.
  UNPREDICTABLE,
  // This instruction is deprecated in ARMv7.
  DEPRECATED,

  // This instruction is forbidden by our SFI model.
  FORBIDDEN,
  // This instruction's operands are forbidden by our SFI model.
  FORBIDDEN_OPERANDS,

  // This instruction was decoded incorrectly, because it should have decoded
  // as a different instruction. This value should never occur, unless there
  // is a bug in our decoder tables (in file armv7.table).
  DECODER_ERROR,

  // This instruction may be safe in untrusted code: in isolation it contains
  // nothing scary, but the validator may overrule this during global analysis.
  MAY_BE_SAFE
};

// Defines the set of validation violations that are found by the
// NaCl validator. Used to speed up generation of diagnostics, by only
// checking for corresponding found violations.
enum Violation {
  // Note: Each (unsafe) safety level also corresponds to a violation. The
  // following violations capture these unsafe violations.
  // Note: Be sure to include an initialization value of the corresponding
  // SafetyLevel entry, so that code can assume the corresponding safety
  // violation has the same value as the safety level.
  UNINITIALIZED_VIOLATION = UNINITIALIZED,
  UNKNOWN_VIOLATION = UNKNOWN,
  UNDEFINED_VIOLATION = UNDEFINED,
  NOT_IMPLEMENTED_VIOLATION = NOT_IMPLEMENTED,
  UNPREDICTABLE_VIOLATION = UNPREDICTABLE,
  DEPRECATED_VIOLATION = DEPRECATED,
  FORBIDDEN_VIOLATION = FORBIDDEN,
  FORBIDDEN_OPERANDS_VIOLATION = FORBIDDEN_OPERANDS,
  DECODER_ERROR_VIOLATION = DECODER_ERROR,
  // Note: The next enumerated value is intentionally set to
  // MAY_BE_SAFE, to guarantee that all remaining violations do not
  // overlap safety violations.
  //
  // Reports that the load/store uses an unsafe base address.  A base address is
  // safe if it
  //     1. Has specific bits masked off by its immediate predecessor, or
  //     2. Is predicated on those bits being clear, as tested by its immediate
  //        predecessor, or
  //     3. Is in a register defined as always containing a safe address.
  // Note: Predication checks (in 2) may be disabled on some architectures.
  LOADSTORE_VIOLATION = MAY_BE_SAFE,
  // Reports that the load/store uses a safe base address, but violates the
  // condition that the instruction pair can't cross a bundle boundary.
  LOADSTORE_CROSSES_BUNDLE_VIOLATION,
  // Reports that the indirect branch uses an unsafe destination address.  A
  // destination address is safe if it has specific bits masked off by its
  // immediate predecessor.
  BRANCH_MASK_VIOLATION,
  // Reports that the indirect branch uses a safe destination address, but
  // violates the condition that the instruction pair can't cross a bundle
  // boundary.
  BRANCH_MASK_CROSSES_BUNDLE_VIOLATION,
  // Reports that the instruction updates a data-address register, but isn't
  // immediately followed by a mask.
  DATA_REGISTER_UPDATE_VIOLATION,
  // Reports that the instruction safely updates a data-address register, but
  // violates the condition that the instruction pair can't cross a bundle
  // boundary.
  //
  // This isn't strictly needed for security. The second instruction (i.e. the
  // mask), can be run without running the first instruction. Further, if
  // the first instruction is run, we can still guarantee that the second will
  // also. However, for simplicity, the current validator assumes that all
  // instruction pairs must be atomic.
  DATA_REGISTER_UPDATE_CROSSES_BUNDLE_VIOLATION,
  // Reports that the call instruction isn't the last instruction in
  // the bundle.
  //
  // This is not a security check per se. Rather, it is a check to prevent
  // imbalancing the CPU's return stack, thereby decreasing performance.
  CALL_POSITION_VIOLATION,
  // Reports that the instruction sets a read-only register.
  READ_ONLY_VIOLATION,
  // Reports if the instruction reads the thread local pointer.
  READ_THREAD_LOCAL_POINTER_VIOLATION,
  // Reports if the program counter was updated without using one of the
  // approved branch instruction.
  PC_WRITES_VIOLATION,
  // A branch that branches into the middle of a multiple instruction
  // pseudo-operation.
  BRANCH_SPLITS_PATTERN_VIOLATION,
  // A branch to instruction outside the code segment.
  BRANCH_OUT_OF_RANGE_VIOLATION,
  // Any other type of violation. Must appear last in the enumeration.
  // Value is used in testing to guarantee that the corresponding
  // bitset ViolationSet can hold all validation violations.
  OTHER_VIOLATION
};

// Defines the bitset of found validation violations.
typedef uint32_t ViolationSet;

// Defines the notion of a empty violation set.
static const ViolationSet kNoViolations = 0x0;

// Returns true if a safety violation.
inline bool IsSafetyViolation(Violation v) {
  return (static_cast<int>(v) >= 0) && (static_cast<int>(v) < MAY_BE_SAFE);
}

// Converts a safety level to the corresponding bit in the violation set.
inline ViolationSet SafetyViolationBit(SafetyLevel level) {
  return static_cast<ViolationSet>(0x1) << level;
}

// Converts a validation violation to a ViolationSet containing
// the corresponding validation violation.
inline ViolationSet ViolationBit(
    Violation violation) {
  NACL_COMPILE_TIME_ASSERT(static_cast<size_t>(OTHER_VIOLATION) <
                           sizeof(ViolationSet) * CHAR_BIT);
  return static_cast<ViolationSet>(0x1) << violation;
}

// Defines the set of all safety violations.
// Note: Assumes that CROSSES_BUNDLE_VIOLATION defines the range
// of safety violations
static const ViolationSet kSafetyViolations =
  SafetyViolationBit(MAY_BE_SAFE) - 1;

// Defines the set of violations that cross bundle boundaries.
static const ViolationSet kCrossesBundleViolations =
  ViolationBit(LOADSTORE_CROSSES_BUNDLE_VIOLATION) |
  ViolationBit(BRANCH_MASK_CROSSES_BUNDLE_VIOLATION) |
  ViolationBit(DATA_REGISTER_UPDATE_CROSSES_BUNDLE_VIOLATION);

// Returns the union of the two validation violation sets.
inline ViolationSet ViolationUnion(ViolationSet vset1, ViolationSet vset2) {
  return vset1 | vset2;
}

// Returns the intersection of two validation violation sets.
inline ViolationSet ViolationIntersect(ViolationSet vset1,
                                       ViolationSet vset2) {
  return vset1 & vset2;
}

// Returns true if the given validation violation set contains the
// given violation.
inline bool ContainsViolation(ViolationSet vset, Violation violation) {
  return ViolationIntersect(vset, ViolationBit(violation)) != kNoViolations;
}

// Returns true if the violation set contains a safety violation.
inline bool ContainsSafetyViolations(ViolationSet vset) {
  return ViolationIntersect(vset, kSafetyViolations) != kNoViolations;
}

// Returns true if the violation set contains a violation that crosses
// bundle boundaries.
inline bool ContainsCrossesBundleViolation(ViolationSet vset) {
  return ViolationIntersect(vset, kCrossesBundleViolations) != kNoViolations;
}

// Returns true if the violation is a violation that crosses
// bundle boundaries.
inline bool IsCrossesBundleViolation(Violation violation) {
  return ContainsCrossesBundleViolation(ViolationBit(violation));
}

// A class decoder is designed to decode a set of instructions that
// have the same semantics, in terms of what the validator needs. This
// includes the bit ranges in the instruction that correspond to
// assigned registers.  as well as whether the instruction is safe to
// use within the validator.
//
// The important property of these class decoders is that the
// corresponding DecoderState (defined in decoder.h) will inspect the
// instruction bits and then dispatch the appropriate class decoder.
//
// The virtuals defined in this class are intended to be used solely
// for the purpose of the validator. For example, for virtual "defs",
// the class decoder will look at the bits defining the assigned
// register of the instruction (typically in bits 12 through 15) and
// add that register to the set of registers returned by the "defs"
// virtual.
//
// There is an underlying assumption that class decoders are constant
// and only provide implementation details for the instructions they
// should be applied to. In general, class decoders should not be
// copied or assigned. Hence, only a no-argument constructor should be
// provided.
class ClassDecoder {
 public:
  // Checks how safe this instruction is, in isolation.
  // This will detect any violation in the ARMv7 spec -- undefined encodings,
  // use of registers that are unpredictable -- and the most basic constraints
  // in our SFI model.  Because ClassDecoders are referentially-transparent and
  // cannot touch global state, this will not check things that may vary with
  // ABI version.
  //
  // Note: To best take advantage of the testing system, define this function
  // to return DECODER_ERROR immediately, if DECODER_ERROR is to be returned by
  // this virtual. This allows testing to (quietly) detect when it is
  // ok that the expected decoder wasn't the actual decoder selected by the
  // instruction decoder.
  //
  // The most positive result this can return is called MAY_BE_SAFE because it
  // is necessary, but not sufficient: the validator has the final say.
  virtual SafetyLevel safety(Instruction i) const = 0;

  // Gets the set of registers affected when an instruction executes.  This set
  // is complete, and includes
  //  - explicit destination (general purpose) register(s),
  //  - changes to condition APSR flags NZCV.
  //  - indexed-addressing writeback,
  //  - changes to the program counter by branches,
  //  - implicit register results, like branch-with-link.
  //
  // Note: This virtual only tracks effects to ARM general purpose flags, and
  // NZCV APSR flags.
  //
  // Note: If you are unsure if an instruction changes condition flags, be
  // conservative and add it to the set of registers returned by this
  // function. Failure to do so may cause a potential break in pattern
  // atomicity, which checks that two instructions run under the same condition.
  //
  // The default implementation returns a ridiculous bitmask that suggests that
  // all possible side effects will occur -- override if this is not
  // appropriate. :-)
  virtual RegisterList defs(Instruction i) const;

  // Gets the set of general purpose registers used by the instruction.
  // This set includes:
  //  - explicit source (general purpose) register(s).
  //  - implicit registers, like branch-with-link.
  //
  // The default implementation returns the empty set.
  virtual RegisterList uses(Instruction i) const;

  // Returns true if the base register has small immediate writeback.
  //
  // This distinction is useful for operations like SP-relative loads, because
  // the maximum displacement that immediate addressing can produce is small and
  // will therefore never cross guard pages if the base register isn't
  // constrained to the untrusted address space.
  //
  // Note that this does not include writeback produced by *register* indexed
  // addressing writeback, since they have no useful properties in our model.
  //
  // Stubbed to indicate that no such writeback occurs.
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;

  // For instructions that can read or write memory, gets the register used as
  // the base for generating the effective address.
  //
  // It is stubbed to return nonsense.
  virtual Register base_address_register(Instruction i) const;

  // Checks whether the instruction is a PC-relative load + immediate.
  //
  // It is stubbed to return false.
  virtual bool is_literal_load(Instruction i) const;

  // For indirect branch instructions, returns the register being moved into
  // r15.  Otherwise, reports Register::None().
  //
  // Note that this exclusively describes instructions that write r15 from a
  // register, unmodified.  This means BX, BLX, and MOV without shift.  Not
  // even BIC, which we allow to write to r15, is modeled this way.
  //
  virtual Register branch_target_register(Instruction i) const;

  // Checks whether the instruction is a direct relative branch -- meaning it
  // adds a constant offset to r15.
  virtual bool is_relative_branch(Instruction i) const;

  // For relative branches, gets the offset added to the instruction's
  // virtual address to find the target.  The results are bogus unless
  // is_relative_branch() returns true.
  //
  // Note that this is different than the offset added to r15 at runtime, since
  // r15 reads as 8 bytes ahead.  This function does the math so you don't have
  // to.
  virtual int32_t branch_target_offset(Instruction i) const;

  // Checks whether this instruction is the special bit sequence that marks
  // the start of a literal pool.
  virtual bool is_literal_pool_head(Instruction i) const;

  // Checks that an instruction clears a certain pattern of bits in all its
  // (non-flag) result registers.  The mask should include 1s in the positions
  // that should be cleared.
  virtual bool clears_bits(Instruction i, uint32_t mask) const;

  // Checks that an instruction will set Z if certain bits in r (chosen by 1s in
  // the mask) are clear.
  //
  // Note that the inverse does not hold: the actual instruction i may require
  // *more* bits to be clear to set Z.  This is fine.
  virtual bool sets_Z_if_bits_clear(Instruction i,
                                    Register r,
                                    uint32_t mask) const;

  // Returns true only if the given thread register (r9) is used in one of
  // the following forms:
  //    ldr Rn, [r9]     ; load use thread pointer.
  //    ldr Rn, [r9, #4] ; load IRT thread pointer.
  // That is, accesses one of the two legal thread pointers.
  //
  // The default virtual returns false.
  virtual bool is_load_thread_address_pointer(Instruction i) const;

  // Returns the sentinel version of the instruction for dynamic code
  // replacement. In dynamic code replacement, only certain immediate
  // constants for specialized instructions may be modified by a dynamic
  // code replacement. For such instructions, this method returns the
  // instruction with the immediate constant normalized to zero. For
  // all other instructions, this method returns a copy of the instruction.
  //
  // This result is used by method ValidateSegmentPair in validator.cc to
  // verify that only such constant changes are allowed.
  //
  // Note: This method should not be defined if any of the following
  // virtuals are overridden by the decoder class, since they make assumptions
  // about the literal constants within them:
  //     offset_is_immediate
  //     is_relative_branch
  //     branch_target_offset
  //     is_literal_pool_head
  //     clears_bits
  //     sets_Z_if_bits_clear
  virtual Instruction dynamic_code_replacement_sentinel(Instruction i) const;

  // Checks the given pair of instructions, and returns found validation
  // violations. Called with the class decoder associated with the second
  // instruction. For violations that only look at a single instruction,
  // they are assumed to apply to the second instruction in the pair.
  //
  // As a side effect, if the instructions are found not to include any
  // violations, but affect state of the validation, the corresponding
  // updates of the validation state is done.
  //
  //   first: The first instruction in the instruction pair to be validated.
  //   second: The second instruction in the instruction pair to be validated.
  //   sfi: The validator being used.
  //   branches: gets filled in with the address of every direct branch.
  //   critical: gets filled in with every address that isn't safe to jump to,
  //       because it would split an otherwise-safe pseudo-op, or jumps into
  //       the middle of a constant pool.
  //   next_inst_addr: The address of the next instruction to be validated.
  //       Set by the caller to the address of the instruction immediately
  //       following the second instruction. If additional instruction should
  //       be skipped (as with contant pool heads), this value should be updated
  //       to point to the next instruction to be processed.
  //
  // Returns the validation violations found.
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;

  // Generates diagnostic messages for validation violations found
  // for the instruction pair. Called with the class decoder associated
  // with the second instruction. Assumes it is only called if virtual
  // get_violations returned a non-empty set.
  //
  //   violations: The set of validation violations detected by get_violations.
  //   first: The first instruction in the instruction pair to be validated.
  //   second: The second instruction in the instruction pair to be validated.
  //   sfi: The validator being used.
  //   out: The problem reporter to use to report diagnostics.
  virtual void generate_diagnostics(
      ViolationSet violations,
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::ProblemSink* out) const;

 protected:
  ClassDecoder() {}
  virtual ~ClassDecoder() {}

 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(ClassDecoder);
};

}  // namespace nacl_arm_dec

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_INST_CLASSES_H_
