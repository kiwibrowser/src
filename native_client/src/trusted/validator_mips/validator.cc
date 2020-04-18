/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/validator_mips/validator.h"
#include "native_client/src/include/nacl_macros.h"


using nacl_mips_dec::Instruction;
using nacl_mips_dec::ClassDecoder;
using nacl_mips_dec::Register;
using nacl_mips_dec::RegisterList;

using nacl_mips_dec::kInstrSize;
using nacl_mips_dec::kInstrAlign;

using std::vector;

namespace nacl_mips_val {

/*
 * TODO(petarj): MIPS validator and runtime port need an external security
 * review before they are delivered in products.
 */

/*********************************************************
 * Implementations of patterns used in the first pass.
 *
 * N.B. IF YOU ADD A PATTERN HERE, REGISTER IT BELOW.
 * See the list in apply_patterns.
 *********************************************************/

// A possible result from a validator pattern.
enum PatternMatch {
  // The pattern does not apply to the instructions it was given.
  NO_MATCH,
  // The pattern matches, and is safe; do not allow jumps to split it.
  PATTERN_SAFE,
  // The pattern matches, and has detected a problem.
  PATTERN_UNSAFE
};


/*********************************************************
 * One instruction patterns.
 *********************************************************/

/*
 * Checks for instructions that are not allowed.
 */
static PatternMatch CheckSafety(const SfiValidator &sfi,
                                const DecodedInstruction &inst,
                                ProblemSink *out) {
  UNREFERENCED_PARAMETER(sfi);
  if (nacl_mips_dec::MAY_BE_SAFE != inst.safety()) {
    out->ReportProblem(inst.addr(), inst.safety(), kProblemUnsafe);
    return PATTERN_UNSAFE;
  }
  return PATTERN_SAFE;
}

/*
 * Checks for instructions that alter read-only registers.
 */
static PatternMatch CheckReadOnly(const SfiValidator &sfi,
                                  const DecodedInstruction &inst,
                                  ProblemSink *out) {
  if (inst.IsDestGprReg(sfi.read_only_registers())) {
    out->ReportProblem(inst.addr(), inst.safety(), kProblemReadOnlyRegister);
    return PATTERN_UNSAFE;
  }
  return NO_MATCH;
}

/*
 * Checks the location of linking branches -- in order to be correct, the
 * branches must be in the slot before the last slot.
 *
 */
static PatternMatch CheckCallPosition(const SfiValidator &sfi,
                                      const DecodedInstruction &inst,
                                      ProblemSink *out) {
  if (inst.IsJal()) {
    uint32_t end_addr = sfi.BundleForAddress(inst.addr()).EndAddr();
    uint32_t branch_slot = end_addr - (2 * kInstrSize);
    if (inst.addr() != branch_slot) {
      out->ReportProblem(inst.addr(), inst.safety(), kProblemMisalignedCall);
      return PATTERN_UNSAFE;
    }
    return PATTERN_SAFE;
  }
  return NO_MATCH;
}

/*
 * Checks for jumps to unsafe area.
 */
static PatternMatch CheckJumpDestAddr(const SfiValidator &sfi,
                                      const DecodedInstruction &instr,
                                      ProblemSink *out) {
  if (instr.IsDirectJump()) {
    uint32_t dest_addr = instr.DestAddr();
    if (dest_addr < sfi.trampoline_region_start()) {
      //  Safe guard region, allow jumps if somebody is suicidal.
      return PATTERN_SAFE;
    } else if (dest_addr < sfi.code_region_start()) {
      //  Trampoline region, allow only 0mod16.
      if ((dest_addr & (sfi.bytes_per_bundle() - 1)) != 0) {
        out->ReportProblem(instr.addr(), instr.safety(),
                           kProblemUnalignedJumpToTrampoline);
        return PATTERN_UNSAFE;
      }
      return PATTERN_SAFE;
    } else {
      // Jumps outside of unsafe region are not allowed.
      if ((dest_addr & ~(sfi.code_address_mask())) != 0) {
        out->ReportProblem(instr.addr(), instr.safety(),
                           kProblemBranchInvalidDest);
        return PATTERN_UNSAFE;
      }
      //  Another pattern is responsible for checking if it lands inside of a
      //  pseudo-instruction.
      return PATTERN_SAFE;
    }
  }
  return NO_MATCH;
}

/*********************************************************
 * Two instruction patterns.
 *********************************************************/

/*
 * Checks if indirect jumps are guarded properly.
 */
static PatternMatch CheckJmpReg(const SfiValidator &sfi,
                                const DecodedInstruction &first,
                                const DecodedInstruction &second,
                                ProblemSink *out) {
  UNREFERENCED_PARAMETER(sfi);
  if (second.IsJmpReg()) {
    if (first.IsMask(second.TargetReg(), Register::JumpMask())) {
      return PATTERN_SAFE;
    }
    out->ReportProblem(second.addr(), second.safety(),
                       kProblemUnsafeJumpRegister);
    return PATTERN_UNSAFE;
  }
  return NO_MATCH;
}


/*
 * Checks if change of the data register ($sp) is followed by load/store mask.
 */
static PatternMatch CheckDataRegisterUpdate(const SfiValidator &sfi,
                                            const DecodedInstruction &first,
                                            const DecodedInstruction &second,
                                            ProblemSink *out) {
  UNREFERENCED_PARAMETER(sfi);
  if (first.DestGprReg().Equals(Register::Sp())
      && !first.IsMask(first.DestGprReg(), Register::LoadStoreMask())) {
    if (second.IsMask(first.DestGprReg(), Register::LoadStoreMask())) {
      return PATTERN_SAFE;
    }
    out->ReportProblem(first.addr(), first.safety(), kProblemUnsafeDataWrite);
    return PATTERN_UNSAFE;
  }
  return NO_MATCH;
}

/*
 * Checks if data register ($sp) change is in the delay slot.
 */
static PatternMatch CheckDataRegisterDslot(const SfiValidator &sfi,
                                           const DecodedInstruction &first,
                                           const DecodedInstruction &second,
                                           ProblemSink *out) {
  UNREFERENCED_PARAMETER(sfi);
  if (second.DestGprReg().Equals(Register::Sp())
      && !second.IsMask(second.DestGprReg(), Register::LoadStoreMask())) {
    if (first.HasDelaySlot()) {
      out->ReportProblem(second.addr(), second.safety(),
                         kProblemDataRegInDelaySlot);
      return PATTERN_UNSAFE;
    }
  }
  return NO_MATCH;
}

/*
 * Checks if load and store instructions are preceded by load/store mask.
 */
static PatternMatch CheckLoadStore(const SfiValidator &sfi,
                                   const DecodedInstruction &first,
                                   const DecodedInstruction &second,
                                   ProblemSink *out) {
  if (second.IsLoadStore()) {
    Register base_addr_reg = second.BaseAddressRegister();
    if (!sfi.data_address_registers().
           ContainsAll(RegisterList(base_addr_reg))) {
      if (first.IsMask(base_addr_reg, Register::LoadStoreMask())) {
        return PATTERN_SAFE;
      }
      out->ReportProblem(second.addr(), second.safety(),
                         kProblemUnsafeLoadStore);
      return PATTERN_UNSAFE;
    }
  }
  return NO_MATCH;
}


/*
 * A thread pointer access is only allowed by these two instructions:
 * lw Rn, 0($t8)  ; load user thread pointer.
 * lw Rn, 4($t8)  ; load IRT thread pointer.
 */
static PatternMatch CheckLoadThreadPointer(const SfiValidator &sfi,
                                           const DecodedInstruction &instr,
                                           ProblemSink *out) {
  UNREFERENCED_PARAMETER(sfi);
  if (!instr.IsLoadStore())
    return NO_MATCH;

  Register base_addr_reg = instr.BaseAddressRegister();
  if (!base_addr_reg.Equals(Register::Tls()))
    return NO_MATCH;

  if (instr.IsLoadWord()) {
    uint32_t offset = instr.GetImm();
    if (offset == 0 || offset == 4)
      return PATTERN_SAFE;
  }

  out->ReportProblem(instr.addr(), instr.safety(),
                     kProblemUnsafeLoadStoreThreadPointer);
  return PATTERN_UNSAFE;
}

/*
 * Checks if there is jump/branch in the delay slot.
 */
static PatternMatch CheckBranchInDelaySlot(const SfiValidator &sfi,
                                   const DecodedInstruction &first,
                                   const DecodedInstruction &second,
                                   ProblemSink *out) {
  UNREFERENCED_PARAMETER(sfi);
  if (first.HasDelaySlot() && second.HasDelaySlot()) {
    out->ReportProblem(second.addr(), second.safety(),
                       kProblemBranchInDelaySlot);
    return PATTERN_UNSAFE;
  }
  return NO_MATCH;
}


/*********************************************************
 * Pseudo-instruction patterns.
 *********************************************************/

/*
 * Checks if a pseudo-instruction that starts with instr will cross bundle
 * border (i.e. if it starts in one and ends in second).
 * The exception to this rule are pseudo-instructions altering the data register
 * value (because mask is the second instruction).
 */
static PatternMatch CheckBundleCross(const SfiValidator &sfi,
                                     const DecodedInstruction instr,
                                     ProblemSink *out) {
  uint32_t begin_addr = sfi.BundleForAddress(instr.addr()).BeginAddr();
  if ((instr.addr() == begin_addr) && !instr.IsDataRegMask()) {
    out->ReportProblem(instr.addr(), instr.safety(),
                       kProblemPatternCrossesBundle);
    return PATTERN_UNSAFE;
  }
  return PATTERN_SAFE;
}

/*
 * Checks if branch instruction will jump in the middle of pseudo-instruction.
 */
static PatternMatch CheckJumpToPseudo(const SfiValidator &sfi,
                                      const std::vector<CodeSegment> &segms,
                                      const DecodedInstruction pseudoinstr,
                                      const AddressSet &branches,
                                      const AddressSet &branch_targets,
                                      ProblemSink *out) {
  uint32_t target_va = pseudoinstr.addr();
  if (branch_targets.Contains(target_va)) {
    std::vector<DecodedInstruction> instrs;
    if (sfi.FindBranch(segms, branches, target_va, &instrs)) {
      for (uint32_t i = 0; i < instrs.size(); i++) {
        out->ReportProblem(instrs[i].addr(), instrs[i].safety(),
                           kProblemBranchSplitsPattern);
      }
      return PATTERN_UNSAFE;
    } else {
      assert(0);
    }
  }
  return PATTERN_SAFE;
}


/*********************************************************
 *
 * Implementation of SfiValidator itself.
 *
 *********************************************************/
SfiValidator::SfiValidator(uint32_t bytes_per_bundle,
                           uint32_t code_region_bytes,
                           uint32_t data_region_bytes,
                           RegisterList read_only_registers,
                           RegisterList data_address_registers)
    : bytes_per_bundle_(bytes_per_bundle),
      code_region_bytes_(code_region_bytes),
      data_address_mask_(~(data_region_bytes - 1)),
      code_address_mask_((code_region_bytes - 1) & kInstrAlign),  // 0x0FFFFFFC
      read_only_registers_(read_only_registers),
      data_address_registers_(data_address_registers),
      decode_state_(nacl_mips_dec::init_decode()),
      is_position_independent_(false) {}

bool SfiValidator::Validate(const vector<CodeSegment> &segments,
                            ProblemSink *out) {
  uint32_t base = segments[0].BeginAddr();
  uint32_t size = segments.back().EndAddr() - base;
  AddressSet branches(base, size);
  AddressSet branch_targets(base, size);
  AddressSet critical(base, size);

  bool complete_success = true;

  for (vector<CodeSegment>::const_iterator it = segments.begin();
      it != segments.end(); ++it) {
    complete_success &= ValidateFallthrough(*it, out, &branches,
                                            &branch_targets, &critical);

    if (!out->ShouldContinue()) {
      return false;
    }
  }

  if (segments.size() == 1) {
    bool external_target_addr = false;
    for (AddressSet::Iterator it = branch_targets.Begin();
         !it.Equals(branch_targets.End()); it.Next()) {
        if (!segments[0].ContainsAddress(it.GetAddress())) {
          external_target_addr = true;
          break;
        }
    }
    is_position_independent_ = !external_target_addr;
  }

  complete_success &= ValidatePseudos(*this, segments,
                                      branches, branch_targets, critical, out);
  return complete_success;
}

bool SfiValidator::ValidateFallthrough(const CodeSegment &segment,
                                       ProblemSink *out,
                                       AddressSet *branches,
                                       AddressSet *branch_targets,
                                       AddressSet *critical) {
  bool complete_success = true;

  nacl_mips_dec::Forbidden initial_decoder;
  // Initialize the previous instruction to a syscall, so patterns all fail.
  DecodedInstruction prev(
      0,         // Virtual address 0, which will be in a different bundle.
      Instruction(0x0000000c),  // syscall.
      initial_decoder);         // and ensure that it decodes as Forbidden.

  for (uint32_t va = segment.BeginAddr(); va != segment.EndAddr();
       va += kInstrSize) {
    DecodedInstruction inst(va, segment[va],
                            nacl_mips_dec::decode(segment[va], decode_state_));

    complete_success &= ApplyPatterns(inst, out);
    if (!out->ShouldContinue()) return false;

    complete_success &= ApplyPatterns(prev, inst, critical, out);
    if (!out->ShouldContinue()) return false;

    if (inst.IsDirectJump()) {
      branches->Add(inst.addr());
      branch_targets->Add(inst.DestAddr());
    }

    prev = inst;
  }

  // Validate the last instruction, paired with a nop.
  const Instruction nop(nacl_mips_dec::kNop);
  DecodedInstruction one_past_end(segment.EndAddr(), nop,
                                  nacl_mips_dec::decode(nop, decode_state_));
  complete_success &= ApplyPatterns(prev, one_past_end, critical, out);

  return complete_success;
}

bool SfiValidator::ValidatePseudos(const SfiValidator &sfi,
                                   const std::vector<CodeSegment> &segments,
                                   const AddressSet &branches,
                                   const AddressSet &branch_targets,
                                   const AddressSet &critical,
                                   ProblemSink* out) {
  bool complete_success = true;
  vector<CodeSegment>::const_iterator seg_it = segments.begin();

  for (AddressSet::Iterator it = critical.Begin(); !it.Equals(critical.End());
       it.Next()) {
    uint32_t va = it.GetAddress();

    while (!seg_it->ContainsAddress(va)) {
      ++seg_it;
    }

    const CodeSegment &segment = *seg_it;
    DecodedInstruction inst_p(va,
                              segment[va],
                              nacl_mips_dec::decode(segment[va],
                              decode_state_));

    //  Check if the pseudo-instruction will cross bundle borders.
    complete_success &= CheckBundleCross(sfi, inst_p, out);

    //  Check if direct jumps destination is inside of a pseudo-instruction.
    complete_success &= CheckJumpToPseudo(sfi, segments, inst_p, branches,
                                          branch_targets, out);
  }

  return complete_success;
}


bool SfiValidator::ApplyPatterns(const DecodedInstruction &inst,
    ProblemSink *out) {
  //  Single-instruction patterns.
  typedef PatternMatch (*OneInstPattern)(const SfiValidator &,
                                         const DecodedInstruction &,
                                         ProblemSink *out);
  static const OneInstPattern one_inst_patterns[] = {
    &CheckSafety,
    &CheckReadOnly,
    &CheckCallPosition,
    &CheckJumpDestAddr,
    &CheckLoadThreadPointer
  };

  bool complete_success = true;

  for (uint32_t i = 0; i < NACL_ARRAY_SIZE(one_inst_patterns); i++) {
    PatternMatch r = one_inst_patterns[i](*this, inst, out);
    switch (r) {
      case PATTERN_SAFE:
      case NO_MATCH:
        break;

      case PATTERN_UNSAFE:
        complete_success = false;
        break;
    }
  }
  return complete_success;
}

bool SfiValidator::ApplyPatterns(const DecodedInstruction &first,
    const DecodedInstruction &second, AddressSet *critical, ProblemSink *out) {
  //  Type for two-instruction pattern functions.
  typedef PatternMatch (*TwoInstPattern)(const SfiValidator &,
                                         const DecodedInstruction &first,
                                         const DecodedInstruction &second,
                                         ProblemSink *out);
  //  The list of patterns -- defined in static functions up top.
  static const TwoInstPattern two_inst_patterns[] = {
    &CheckJmpReg,
    &CheckDataRegisterUpdate,
    &CheckDataRegisterDslot,
    &CheckLoadStore,
    &CheckBranchInDelaySlot
  };

  bool complete_success = true;

  for (uint32_t i = 0; i < NACL_ARRAY_SIZE(two_inst_patterns); i++) {
    PatternMatch r = two_inst_patterns[i](*this, first, second, out);
    switch (r) {
      case NO_MATCH:
         break;
      case PATTERN_UNSAFE:
        // Pattern is in charge of reporting specific issue.
        complete_success = false;
        break;
      case PATTERN_SAFE:
        critical->Add(second.addr());
        break;
    }
  }
  return complete_success;
}

bool SfiValidator::IsDataAddressRegister(Register reg) const {
  return data_address_registers_[reg];
}

bool SfiValidator::IsBundleHead(uint32_t address) const {
  return (address % bytes_per_bundle_) == 0;
}

const Bundle SfiValidator::BundleForAddress(uint32_t address) const {
  uint32_t base = address - (address % bytes_per_bundle_);
  return Bundle(base, bytes_per_bundle_);
}

bool SfiValidator::FindBranch(const std::vector<CodeSegment> &segments,
                              const AddressSet &branches,
                              uint32_t dest_address,
                              std::vector<DecodedInstruction> *instrs) const {
  vector<CodeSegment>::const_iterator seg_it = segments.begin();

  for (AddressSet::Iterator it = branches.Begin(); !it.Equals(branches.End());
       it.Next()) {
    uint32_t va = it.GetAddress();

    while (!seg_it->ContainsAddress(va)) {
      ++seg_it;
    }

    const CodeSegment &segment = *seg_it;
    DecodedInstruction instr = DecodedInstruction(va, segment[va],
                             nacl_mips_dec::decode(segment[va], decode_state_));
    if (instr.DestAddr() == dest_address) {
      instrs->push_back(instr);
    }
  }
  if (!instrs->empty()) return true;
  return false;
}
/*
 * DecodedInstruction.
 */
DecodedInstruction::DecodedInstruction(uint32_t vaddr,
                                       Instruction inst,
                                       const ClassDecoder &decoder)
    : vaddr_(vaddr),
      inst_(inst),
      decoder_(&decoder),
      safety_(decoder.safety(inst_))
{}

}  // namespace nacl_mips_val
