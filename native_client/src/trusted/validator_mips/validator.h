/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_VALIDATOR_H
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_VALIDATOR_H

/*
 * The SFI validator, and some utility classes it uses.
 */

#include <stdint.h>
#include <stdlib.h>
#include <vector>

#include "native_client/src/include/nacl_string.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/validator_mips/address_set.h"
#include "native_client/src/trusted/validator_mips/decode.h"
#include "native_client/src/trusted/validator_mips/inst_classes.h"
#include "native_client/src/trusted/validator_mips/model.h"

namespace nacl_mips_val {

/*
 * Forward declarations of classes used by-reference in the validator, and
 * defined at the end of this file.
 */
class CodeSegment;
class DecodedInstruction;
class ProblemSink;


/*
 * A simple model of an instruction bundle.  Bundles consist of one or more
 * instructions (two or more, in the useful case); the precise size is
 * controlled by the parameters passed into SfiValidator, below.
 */
class Bundle {
 public:
  Bundle(uint32_t virtual_base, uint32_t size_bytes)
      : virtual_base_(virtual_base), size_(size_bytes) {}

  uint32_t BeginAddr() const { return virtual_base_; }
  uint32_t EndAddr() const { return virtual_base_ + size_; }

  bool operator!=(const Bundle &other) const {
    // Note that all Bundles are currently assumed to be the same size.
    return virtual_base_ != other.virtual_base_;
  }

 private:
  uint32_t virtual_base_;
  uint32_t size_;
};


/*
 * The SFI validator itself.  The validator is controlled by the following
 * inputs:
 *   bytes_per_bundle: the number of bytes in each bundle of instructions.
 *       Currently this tends to be 16, but we've evaluated alternatives.
 *   code_region_bytes: number of bytes in the code region, starting at address
 *       0 and including the trampolines, etc.  Must be a power of two.
 *   data_region_bits: number of bytes in the data region, starting at address
 *       0 and including the code region.  Must be a power of two.
 *   read_only_registers: registers that untrusted code must not alter (but may
 *       read).  This currently applies to t6 - jump mask, t7 - load/store mask
 *       and t8 - tls index.
 *   data_address_registers: registers that must contain a valid data-region
 *       address at all times.  This currently applies to the stack pointer and
 *       TLS register but could be extended to include a frame pointer for
 *       C-like languages. Adding register to data_address_registers only means
 *       that load/store access can be done without checks. Check for register
 *       value change still needs to be executed.
 *
 * The values of these inputs will typically be taken from the headers of
 * untrusted code -- either by the ABI version they indicate, or (perhaps in
 * the future) explicit indicators of what SFI model they follow.
 */
class SfiValidator {
 public:
  SfiValidator(uint32_t bytes_per_bundle,
               uint32_t code_region_bytes,
               uint32_t data_region_bytes,
               nacl_mips_dec::RegisterList read_only_registers,
               nacl_mips_dec::RegisterList data_address_registers);

  /*
   * The main validator entry point.  Validates the provided CodeSegments,
   * which must be in sorted order, reporting any problems through the
   * ProblemSink.
   *
   * Returns true iff no problems were found.
   */
  bool Validate(const std::vector<CodeSegment> &, ProblemSink *out);

  // Returns true if validation did not depend on the code's base address.
  bool is_position_independent() {
    return is_position_independent_;
  }

  /*
   * Checks whether the given Register always holds a valid data region address.
   * This implies that the register is safe to use in unguarded stores.
   */
  bool IsDataAddressRegister(nacl_mips_dec::Register) const;

  uint32_t data_address_mask() const { return data_address_mask_; }
  uint32_t code_address_mask() const { return code_address_mask_; }
  uint32_t code_region_bytes() const { return code_region_bytes_; }
  uint32_t bytes_per_bundle() const { return bytes_per_bundle_; }
  uint32_t code_region_start() const { return code_region_start_; }
  uint32_t trampoline_region_start() const { return trampoline_region_start_; }

  nacl_mips_dec::RegisterList read_only_registers() const {
    return read_only_registers_;
  }
  nacl_mips_dec::RegisterList data_address_registers() const {
    return data_address_registers_;
  }

  // Returns the Bundle containing a given address.
  const Bundle BundleForAddress(uint32_t) const;

  /*
   * Change masks: this is useful for debugging and cannot be completely
   *               controlled with constructor arguments
   */
  void ChangeMasks(uint32_t code_address_mask, uint32_t data_address_mask) {
    code_address_mask_ = code_address_mask;
    data_address_mask_ = data_address_mask;
  }

  /*
   * Find all the branch instructions which jump on the dest_address.
   */
  bool FindBranch(const std::vector<CodeSegment> &segments,
                  const AddressSet &branches,
                  uint32_t dest_address,
                  std::vector<DecodedInstruction> *instrs) const;

 private:
  bool IsBundleHead(uint32_t address) const;

  /*
   * Validates a straight-line execution of the code, applying patterns.  This
   * is the first validation pass, which fills out the AddressSets for
   * consumption by later analyses.
   *   branches: gets filled in with the address of every direct branch.
   *   branch_targets: gets filled in with the target address of every direct
   *   branch.
   *   critical: gets filled in with every address that isn't safe to jump to,
   *       because it would split an otherwise-safe pseudo-op.
   *
   * Returns true iff no problems were found.
   */
  bool ValidateFallthrough(const CodeSegment &, ProblemSink *,
                           AddressSet *branches, AddressSet *branch_targets,
                           AddressSet *critical);

  /*
   * Factor of validate_fallthrough, above.  Checks a single instruction using
   * the instruction patterns defined in the .cc file, with two possible
   * results:
   *   1. No patterns matched, or all were safe: nothing happens.
   *   2. Patterns matched and were unsafe: problems get sent to 'out'.
   */
  bool ApplyPatterns(const DecodedInstruction &, ProblemSink *out);

  /*
   * Factor of validate_fallthrough, above.  Checks a pair of instructions using
   * the instruction patterns defined in the .cc file, with three possible
   * results:
   *   1. No patterns matched: nothing happens.
   *   2. Patterns matched and were safe: the addresses are filled into
   *      'critical' for use by the second pass.
   *   3. Patterns matched and were unsafe: problems get sent to 'out'.
   */
  bool ApplyPatterns(const DecodedInstruction &first,
      const DecodedInstruction &second, AddressSet *critical, ProblemSink *out);


  /*
   * 2nd pass - checks if some branch instruction tries to jump onto the middle
   * of the pseudo-instruction, and if some pseudo-instruction crosses bundle
   * borders.
   */
  bool ValidatePseudos(const SfiValidator &sfi,
                       const std::vector<CodeSegment> &segments,
                       const AddressSet &branches,
                       const AddressSet &branch_targets,
                       const AddressSet &critical,
                       ProblemSink *out);

  uint32_t const bytes_per_bundle_;
  uint32_t const code_region_bytes_;
  uint32_t data_address_mask_;
  uint32_t code_address_mask_;

  // TODO(petarj): Think about pulling these values from some config header.
  static uint32_t const code_region_start_ = 0x20000;
  static uint32_t const trampoline_region_start_ = 0x10000;

  // Registers which cannot be modified by untrusted code.
  nacl_mips_dec::RegisterList read_only_registers_;
  // Registers which must always contain a valid data region address.
  nacl_mips_dec::RegisterList data_address_registers_;
  const nacl_mips_dec::DecoderState *decode_state_;
  // True if validation did not depend on the code's base address.
  bool is_position_independent_;
};


/*
 * A facade that combines an Instruction with its address and a ClassDecoder.
 * This makes the patterns substantially easier to write and read than managing
 * all three variables separately.
 *
 * ClassDecoders do all decoding on-demand, with no caching.  DecodedInstruction
 * has knowledge of the validator, and pairs a ClassDecoder with a constant
 * Instruction -- so it can cache commonly used values, and does.  Caching
 * safety and defs doubles validator performance.  Add other values only
 * under guidance of a profiler.
 */
class DecodedInstruction {
 public:
  DecodedInstruction(uint32_t vaddr, nacl_mips_dec::Instruction inst,
      const nacl_mips_dec::ClassDecoder &decoder);
  // We permit the default copy ctor and assignment operator.

  uint32_t addr() const { return vaddr_; }

  // The methods below mirror those on ClassDecoder, but are cached and cheap.
  nacl_mips_dec::SafetyLevel safety() const { return safety_; }

  // The methods below pull values from ClassDecoder on demand.
  const nacl_mips_dec::Register BaseAddressRegister() const {
    return decoder_->BaseAddressRegister(inst_);
  }

  nacl_mips_dec::Register DestGprReg() const {
    return decoder_->DestGprReg(inst_);
  }

  nacl_mips_dec::Register TargetReg() const {
    return decoder_->TargetReg(inst_);
  }

  uint32_t DestAddr() const {
    return decoder_->DestAddr(inst_, addr());
  }

  bool HasDelaySlot() const {
    return decoder_-> HasDelaySlot();
  }

  bool IsJal() const {
    return decoder_-> IsJal();
  }

  bool IsMask(const nacl_mips_dec::Register& dest,
              const nacl_mips_dec::Register& mask) const {
    return decoder_->IsMask(inst_, dest, mask);
  }

  bool IsJmpReg() const {
    return decoder_->IsJmpReg();
  }

  bool IsLoadStore() const {
    return decoder_->IsLoadStore();
  }

  bool IsLoadWord() const {
    return decoder_->IsLoadWord();
  }

  uint32_t GetImm() const {
    return decoder_->GetImm(inst_);
  }

  bool IsDirectJump() const {
    return decoder_->IsDirectJump();
  }

  bool IsDestGprReg(nacl_mips_dec::RegisterList rl) const {
    return rl.ContainsAny(nacl_mips_dec::RegisterList(DestGprReg()));
  }

  bool IsDataRegMask() const {
    return IsMask(DestGprReg(), nacl_mips_dec::Register::LoadStoreMask());
  }

 private:
  uint32_t vaddr_;
  nacl_mips_dec::Instruction inst_;
  const nacl_mips_dec::ClassDecoder *decoder_;

  nacl_mips_dec::SafetyLevel safety_;
};


/*
 * Describes a memory region that contains executable code.  Note that the code
 * need not live in its final location -- we pretend the code lives at the
 * provided start_addr, regardless of where the base pointer actually points.
 */
class CodeSegment {
 public:
  CodeSegment(const uint8_t *base, uint32_t start_addr, size_t size)
      : base_(base), start_addr_(start_addr), size_(size) {}

  uint32_t BeginAddr() const { return start_addr_; }
  uint32_t EndAddr() const { return start_addr_ + size_; }
  uint32_t size() const { return size_; }
  bool ContainsAddress(uint32_t a) const {
    return (a >= BeginAddr()) && (a < EndAddr());
  }

  const nacl_mips_dec::Instruction operator[](uint32_t address) const {
    const uint8_t *element = &base_[address - start_addr_];
    return nacl_mips_dec::Instruction(
        *reinterpret_cast<const uint32_t *>(element));
  }

  bool operator<(const CodeSegment &other) const {
    return start_addr_ < other.start_addr_;
  }

 private:
  const uint8_t *base_;
  uint32_t start_addr_;
  size_t size_;
};


/*
 * A class that consumes reports of validation problems, and may decide whether
 * to continue validating, or early-exit.
 *
 * In a sel_ldr context, we early-exit at the first problem we find.  In an SDK
 * context, however, we collect more reports to give the developer feedback;
 * even then it may be desirable to exit after the first, say, 200 reports.
 */
class ProblemSink {
 public:
  virtual ~ProblemSink() {}

  /*
   * Reports a problem in untrusted code.
   *  vaddr: the virtual address where the problem occurred.  Note that this is
   *      probably not the address of memory that contains the offending
   *      instruction, since we allow CodeSegments to lie about their base
   *      addresses.
   *  safety: the safety level of the instruction, as reported by the decoder.
   *      This may be MAY_BE_SAFE while still indicating a problem.
   *  problem_code: a constant string, defined below, that uniquely identifies
   *      the problem.  These are not intended to be human-readable, and should
   *      be looked up for localization and presentation to the developer.
   *  ref_vaddr: A second virtual address of more code that affected the
   *      decision -- typically a branch target.
   */
  virtual void ReportProblem(uint32_t vaddr, nacl_mips_dec::SafetyLevel safety,
      const nacl::string &problem_code, uint32_t ref_vaddr = 0) {
    UNREFERENCED_PARAMETER(vaddr);
    UNREFERENCED_PARAMETER(safety);
    UNREFERENCED_PARAMETER(problem_code);
    UNREFERENCED_PARAMETER(ref_vaddr);
  }

  /*
   * Called after each invocation of report_problem.  If this returns false,
   * the validator exits.
   */
  virtual bool ShouldContinue() { return false; }
};

/*
 * Strings used to describe the current set of validator problems.  These may
 * be worth splitting into a separate header file, so that dev tools can
 * process them into localized messages without needing to pull in the whole
 * validator...we'll see.
 */

// An instruction is unsafe -- more information in the SafetyLevel.
const char * const kProblemUnsafe = "kProblemUnsafe";
// A branch would break a pseudo-operation pattern.
const char * const kProblemBranchSplitsPattern = "kProblemBranchSplitsPattern";
// A branch targets an invalid code address (out of segment).
const char * const kProblemBranchInvalidDest = "kProblemBranchInvalidDest";
// A load/store uses an unsafe (non-masked) base address.
const char * const kProblemUnsafeLoadStore = "kProblemUnsafeLoadStore";
// A thread pointer load/store is unsafe.
const char * const kProblemUnsafeLoadStoreThreadPointer =
    "kProblemUnsafeLoadStoreThreadPointer";
// An instruction updates a data-address register (e.g. SP) without masking.
const char * const kProblemUnsafeDataWrite = "kProblemUnsafeDataWrite";
// An instruction updates a read-only register (e.g. t6, t7, t8).
const char * const kProblemReadOnlyRegister = "kProblemReadOnlyRegister";
// A pseudo-op pattern crosses a bundle boundary.
const char * const kProblemPatternCrossesBundle =
    "kProblemPatternCrossesBundle";
// A linking branch instruction is not in the last bundle slot.
const char * const kProblemMisalignedCall = "kProblemMisalignedCall";
// A data register is found in a branch delay slot.
const char * const kProblemDataRegInDelaySlot = "kProblemDataRegInDelaySlot";
// A jump to trampoline instruction which is not a start of a bundle.
const char * const kProblemUnalignedJumpToTrampoline =
    "kProblemUnalignedJumpToTrampoline";
// A jump register instruction is not guarded.
const char * const kProblemUnsafeJumpRegister = "kProblemUnsafeJumpRegister";
// Two consecutive branches/jumps. Branch/jump in the delay slot.
const char * const kProblemBranchInDelaySlot = "kProblemBranchInDelaySlot";
}  // namespace nacl_mips_val

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_VALIDATOR_H
