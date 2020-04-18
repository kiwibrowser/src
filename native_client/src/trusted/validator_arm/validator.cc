/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <climits>
#include <cstdarg>

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/validator_arm/model.h"
#include "native_client/src/trusted/validator_arm/validator.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability_bits.h"
#include "native_client/src/shared/platform/nacl_log.h"

using nacl_arm_dec::Instruction;
using nacl_arm_dec::ClassDecoder;
using nacl_arm_dec::Register;
using nacl_arm_dec::RegisterList;

using std::vector;

namespace nacl_arm_val {

/*********************************************************
 *
 * Implementation of SfiValidator itself.
 *
 *********************************************************/

// See ARM ARM A8.3 Conditional execution.
//
// Flags are:
//    N - Negative condition code flag.
//    Z - Zero condition code flag.
//    C - Carry condition code flag.
//    V - Overflow condition code flag.
const bool SfiValidator::
condition_implies[nacl_arm_dec::Instruction::kConditionSize + 1]
                 [nacl_arm_dec::Instruction::kConditionSize + 1] = {
# if defined(T) || defined(_)
#   error Macros already defined.
# endif
# define T true
# define _ false
  //                       EQ NE CS CC MI PL VS VC HI LS GE LT GT LE AL UN
  /* EQ => Z==1        */ { T, _, _, _, _, _, _, _, _, T, _, _, _, _, T, T },
  /* NE => Z==0        */ { _, T, _, _, _, _, _, _, _, _, _, _, _, _, T, T },
  /* CS => C==1        */ { _, _, T, _, _, _, _, _, _, _, _, _, _, _, T, T },
  /* CC => C==0        */ { _, _, _, T, _, _, _, _, _, T, _, _, _, _, T, T },
  /* MI => N==1        */ { _, _, _, _, T, _, _, _, _, _, _, _, _, _, T, T },
  /* PL => N==0        */ { _, _, _, _, _, T, _, _, _, _, _, _, _, _, T, T },
  /* VS => V==1        */ { _, _, _, _, _, _, T, _, _, _, _, _, _, _, T, T },
  /* VC => V==0        */ { _, _, _, _, _, _, _, T, _, _, _, _, _, _, T, T },
  /* HI => C==1 & Z==0 */ { _, T, T, _, _, _, _, _, T, _, _, _, _, _, T, T },
  /* LS => C==0 | Z==1 */ { _, _, _, _, _, _, _, _, _, T, _, _, _, _, T, T },
  /* GE => N==V        */ { _, _, _, _, _, _, _, _, _, _, T, _, _, _, T, T },
  /* LT => N!=V        */ { _, _, _, _, _, _, _, _, _, _, _, T, _, _, T, T },
  /* GT => Z==0 & N==V */ { _, T, _, _, _, _, _, _, _, _, T, _, T, _, T, T },
  /* LE => Z==1 & N!=V */ { T, _, _, _, _, _, _, _, _, T, _, T, _, T, T, T },
  /* AL => Any         */ { _, _, _, _, _, _, _, _, _, _, _, _, _, _, T, T },
  /* UN => Any         */ { _, _, _, _, _, _, _, _, _, _, _, _, _, _, T, T },
# undef _
# undef T
};

SfiValidator::SfiValidator(uint32_t bytes_per_bundle,
                           uint32_t code_region_bytes,
                           uint32_t data_region_bytes,
                           RegisterList read_only_registers,
                           RegisterList data_address_registers,
                           const NaClCPUFeaturesArm *cpu_features)
    : cpu_features_(),
      bytes_per_bundle_(bytes_per_bundle),
      code_region_bytes_(code_region_bytes),
      data_region_bytes_(data_region_bytes),
      read_only_registers_(read_only_registers),
      data_address_registers_(data_address_registers),
      decode_state_(),
      construction_failed_(false),
      is_position_independent_(true) {
  NaClCopyCPUFeaturesArm(&cpu_features_, cpu_features);
  // Make sure we can construct sane masks with the values.
  if ((nacl::PopCount(bytes_per_bundle_) != 1) ||
      (nacl::PopCount(code_region_bytes_) != 1) ||
      (nacl::PopCount(data_region_bytes_) != 1) ||
      (bytes_per_bundle_ < 4) ||
      (code_region_bytes_ < 4) ||
      (data_region_bytes_ < 4) ||
      (code_region_bytes_ < bytes_per_bundle_)) {
    construction_failed_ = true;
  }
}

SfiValidator::SfiValidator(const SfiValidator& v)
    : cpu_features_(),
      bytes_per_bundle_(v.bytes_per_bundle_),
      code_region_bytes_(v.code_region_bytes_),
      data_region_bytes_(v.data_region_bytes_),
      read_only_registers_(v.read_only_registers_),
      data_address_registers_(v.data_address_registers_),
      decode_state_(),
      construction_failed_(v.construction_failed_),
      is_position_independent_(v.is_position_independent_) {
  NaClCopyCPUFeaturesArm(&cpu_features_, v.CpuFeatures());
}

SfiValidator& SfiValidator::operator=(const SfiValidator& v) {
  NaClCopyCPUFeaturesArm(&cpu_features_, v.CpuFeatures());
  bytes_per_bundle_ = v.bytes_per_bundle_;
  code_region_bytes_ = v.code_region_bytes_;
  data_region_bytes_ = v.data_region_bytes_;
  read_only_registers_.Copy(v.read_only_registers_);
  data_address_registers_.Copy(v.data_address_registers_);
  construction_failed_ = v.construction_failed_;
  is_position_independent_ = v.is_position_independent_;
  return *this;
}

nacl_arm_dec::ViolationSet SfiValidator::
find_violations(const vector<CodeSegment>& segments,
                ProblemSink* out) {
  if (ConstructionFailed(out))
    return nacl_arm_dec::ViolationBit(nacl_arm_dec::OTHER_VIOLATION);

  uint32_t base = segments[0].begin_addr();
  uint32_t size = segments.back().end_addr() - base;
  AddressSet branches(base, size);
  AddressSet critical(base, size);

  nacl_arm_dec::ViolationSet found_violations = nacl_arm_dec::kNoViolations;

  for (vector<CodeSegment>::const_iterator it = segments.begin();
      it != segments.end(); ++it) {
    nacl_arm_dec::ViolationSet segment_violations =
        validate_fallthrough(*it, out, &branches, &critical);
    found_violations =
        nacl_arm_dec::ViolationUnion(found_violations, segment_violations);
    if (segment_violations && (out == NULL)) return found_violations;
  }

  return
      nacl_arm_dec::ViolationUnion(
          found_violations,
          validate_branches(segments, branches, critical, out));
}

bool SfiValidator::ValidateSegmentPair(const CodeSegment& old_code,
                                       const CodeSegment& new_code,
                                       ProblemSink* out) {
  // This code verifies that the new code is just like the old code,
  // except a few (acceptable) literal constants have been replaced
  // in the new code segment. Hence, checking of safety etc. is not
  // necessary. We assume that this was done on the old code, and
  // does not need to be done again.
  if (ConstructionFailed(out))
    return false;

  if ((old_code.begin_addr() != new_code.begin_addr()) ||
      (old_code.end_addr() != new_code.end_addr())) {
    return false;
  }

  bool complete_success = true;
  bool current_bundle_is_literal_pool = false;

  // The following loop expects the first address to be
  // bundle-aligned. This invariant is checked in the validator's C
  // interface and it therefore not checked again.
  NACL_COMPILE_TIME_ASSERT((nacl_arm_dec::kArm32InstSize / CHAR_BIT) == 4);
  for (uint32_t va = old_code.begin_addr();
       va != old_code.end_addr();
       va += nacl_arm_dec::kArm32InstSize / CHAR_BIT) {
    Instruction old_insn = old_code[va];
    Instruction new_insn = new_code[va];

    // Keep track of literal pools: it's valid to replace any value in them.
    // Literal pools should still be at the same place in the old and new code.
    if (is_bundle_head(va)) {
      const ClassDecoder& old_decoder = decode_state_.decode(old_insn);
      current_bundle_is_literal_pool =
          (old_decoder.is_literal_pool_head(old_insn) &&
           new_insn.Equals(old_insn));
    }

    // Accept any change inside a literal pool.
    if (current_bundle_is_literal_pool)
      continue;

    // See if the instruction has changed in the new version. If not,
    // there is nothing to check, and we can skip to the next
    // instruction.
    if (new_insn.Equals(old_insn))
      continue;

    // Decode instructions and get corresponding decoders.
    const ClassDecoder& old_decoder = decode_state_.decode(old_insn);
    const ClassDecoder& new_decoder = decode_state_.decode(new_insn);

    // Convert the instructions into their sentinel form (i.e.
    // convert immediate values to zero if applicable).
    Instruction old_sentinel(
        old_decoder.dynamic_code_replacement_sentinel(old_insn));
    Instruction new_sentinel(
        new_decoder.dynamic_code_replacement_sentinel(new_insn));

    // Report problem if the sentinels differ, and reject the replacement.
    if (!new_sentinel.Equals(old_sentinel)) {
      if (out == NULL) return false;
      out->ReportProblemDiagnostic(nacl_arm_dec::OTHER_VIOLATION,
                                   va,
                                   "Sentinels at %08" NACL_PRIx32 " differ.",
                                   va);
      complete_success = false;
    }
  }

  return complete_success;
}

bool SfiValidator::CopyCode(const CodeSegment& source_code,
                            CodeSegment* dest_code,
                            NaClCopyInstructionFunc copy_func,
                            ProblemSink* out) {
  if (ConstructionFailed(out))
    return false;

  vector<CodeSegment> segments;
  segments.push_back(source_code);
  if (!validate(segments, out))
      return false;

  // As on ARM all instructions are 4 bytes in size and aligned
  // we don't have to check instruction boundary invariant.
  for (uintptr_t va = source_code.begin_addr();
       va != source_code.end_addr();
       va += nacl_arm_dec::kArm32InstSize / CHAR_BIT) {
    intptr_t offset = va - source_code.begin_addr();
    // TODO(olonho): this const cast is a bit ugly, but we
    // need to write to dest segment.
    copy_func(const_cast<uint8_t*>(dest_code->base()) + offset,
              const_cast<uint8_t*>(source_code.base()) + offset,
              nacl_arm_dec::kArm32InstSize / CHAR_BIT);
  }

  return true;
}

bool SfiValidator::ConstructionFailed(ProblemSink* out) {
  if (construction_failed_ && (out != NULL)) {
    uint32_t invalid_addr = ~(uint32_t)0;
    out->ReportProblemDiagnostic(nacl_arm_dec::OTHER_VIOLATION,
                                 invalid_addr,
                                 "Construction of validator failed!");
  }
  return construction_failed_;
}

nacl_arm_dec::ViolationSet SfiValidator::validate_fallthrough(
    const CodeSegment& segment,
    ProblemSink* out,
    AddressSet* branches,
    AddressSet* critical) {
  nacl_arm_dec::ViolationSet found_violations = nacl_arm_dec::kNoViolations;

  // Initialize the previous instruction so it always fails validation.
  DecodedInstruction pred(
      0,  // Virtual address 0, which will be in a different bundle;
      Instruction(nacl_arm_dec::kFailValidation),
      decode_state_.fictitious_decoder());

  // Validate each instruction.
  uint32_t va = segment.begin_addr();
  while (va < segment.end_addr()) {
    const ClassDecoder& decoder = decode_state_.decode(segment[va]);
    DecodedInstruction inst(va, segment[va], decoder);
    // Note: get_violations is expecting the address of the next instruction
    // to be validated as its last argument. This address can be updated by
    // by that routine (as with constant pool heads). Hence, rather than
    // updating va at the end of the loop, we update it here and pass it
    // through to get_violations.
    va += 4;
    nacl_arm_dec::ViolationSet violations =
        decoder.get_violations(pred, inst, *this, branches, critical, &va);
    if (violations) {
      found_violations =
          nacl_arm_dec::ViolationUnion(found_violations, violations);
      if (out == NULL) return found_violations;
      decoder.generate_diagnostics(violations, pred, inst, *this, out);
    }
    pred.Copy(inst);
  }

  // Validate the last instruction, paired with a nop.
  const Instruction nop(nacl_arm_dec::kNop);
  const ClassDecoder& nop_decoder = decode_state_.decode(nop);
  DecodedInstruction one_past_end(va, nop, nop_decoder);
  // Note: Like above, we update va to point to the instruction after the nop,
  // so that it meets the requirements of get_violations.
  va += 4;
  nacl_arm_dec::ViolationSet violations =
      nop_decoder.get_violations(pred, one_past_end, *this,
                                 branches, critical, &va);
  if (violations) {
    found_violations =
        nacl_arm_dec::ViolationUnion(found_violations, violations);
    if (out == NULL) return found_violations;
    nop_decoder.generate_diagnostics(violations, pred, one_past_end,
                                     *this, out);
  }

  return found_violations;
}

static bool address_contained(uint32_t va, const vector<CodeSegment>& segs) {
  for (vector<CodeSegment>::const_iterator it = segs.begin(); it != segs.end();
      ++it) {
    if (it->contains_address(va)) return true;
  }
  return false;
}

nacl_arm_dec::ViolationSet SfiValidator::validate_branches(
    const vector<CodeSegment>& segments,
    const AddressSet& branches,
    const AddressSet& critical,
    ProblemSink* out) {
  nacl_arm_dec::ViolationSet found_violations = nacl_arm_dec::kNoViolations;

  vector<CodeSegment>::const_iterator seg_it = segments.begin();

  if (segments.size() > 1) {
    // If there are multiple segments, their relative positioning matters.
    is_position_independent_ = false;
  }

  for (AddressSet::Iterator it = branches.begin(); it != branches.end(); ++it) {
    uint32_t va = *it;

    // Invariant: all addresses in branches are covered by some segment;
    // segments are in sorted order.
    while (!seg_it->contains_address(va)) {
      ++seg_it;
    }

    const CodeSegment &segment = *seg_it;

    DecodedInstruction inst(va, segment[va],
                            decode_state_.decode(segment[va]));

    // We know it is_relative_branch(), so we can simply call:
    uint32_t target_va = inst.branch_target();
    if (address_contained(target_va, segments)) {
      if (critical.contains(target_va)) {
        found_violations =
            nacl_arm_dec::ViolationUnion(
                found_violations,
                nacl_arm_dec::ViolationBit(
                    nacl_arm_dec::BRANCH_SPLITS_PATTERN_VIOLATION));
        if (out == NULL) return found_violations;
        out->ReportProblemDiagnostic(
            nacl_arm_dec::BRANCH_SPLITS_PATTERN_VIOLATION,
            va,
            "Instruction branches into middle of 2-instruction "
            "pattern at %08" NACL_PRIx32 ".",
            target_va);
      }
    } else {
      // If the jump is outside the segment, absolute position matters.
      is_position_independent_ = false;

      if ((target_va & code_address_mask()) == 0) {
        // Ensure that relative direct branches which don't go to known
        // segments are to bundle-aligned targets, and that the branch
        // doesn't escape from the sandbox. ARM relative direct branches
        // have a +/-32MiB range, code at the edges of the sandbox could
        // therefore potentially escape if left unchecked. This implies
        // that validation is position-dependent, which matters for
        // validation caching: code segments need to be mapped at the
        // same addresses as the ones at which they were validated.
        //
        // We could make it a sandbox invariant that user code cannot be
        // mapped within 32MiB of the sandbox's edges (and then only check
        // bundle alignment with is_bundle_head(target_va)), but this may
        // break some assumptions in user code.
        //
        // We could also track the furthest negative and positive relative
        // direct branches seen per segment and record that
        // information. This information would then allow validation
        // caching to be mostly position independent, as long as code
        // segments are mapped far enough from the sandbox's edges.
        //
        // Branches leaving the segment could be useful for dynamic code
        // generation as well as to directly branch to the runtime's
        // trampolines. The IRT curently doesn't do the latter because the
        // compiler doesn't know that the trampolines will be in range of
        // relative direct branch immediates. Indirect branches are
        // instead used and the loader doesn't reoptimize the code.
      } else {
        found_violations =
            nacl_arm_dec::ViolationUnion(
                found_violations,
                nacl_arm_dec::ViolationBit(
                    nacl_arm_dec::BRANCH_OUT_OF_RANGE_VIOLATION));
        if (out == NULL) return found_violations;
        out->ReportProblemDiagnostic(
            nacl_arm_dec::BRANCH_OUT_OF_RANGE_VIOLATION,
            va,
            "Instruction branches to invalid address %08" NACL_PRIx32 ".",
            target_va);
      }
    }
  }

  return found_violations;
}

bool SfiValidator::is_valid_inst_boundary(const CodeSegment& code,
                                          uint32_t addr) {
  // Check addr is on an inst boundary.
  if ((addr & 0x3) != 0)
    return false;

  CHECK(!ConstructionFailed(NULL));

  uint32_t base = code.begin_addr();
  uint32_t size = code.end_addr() - base;
  AddressSet branches(base, size);
  AddressSet critical(base, size);

  uint32_t bundle_addr = addr & ~(bytes_per_bundle_ - 1);
  uint32_t offset = bundle_addr - base;
  uint32_t instr = reinterpret_cast<const uint32_t*>(
      code.base() + offset)[0];

  // Check if addr falls within a constant pool.
  if (nacl_arm_dec::IsBreakPointAndConstantPoolHead(instr))
    return false;

  nacl_arm_dec::ViolationSet violations =
        validate_fallthrough(code, NULL, &branches, &critical);

  // Function should only be called after the code has already been validated.
  CHECK(violations == nacl_arm_dec::kNoViolations);

  return !critical.contains(addr);
}

}  // namespace nacl_arm_val
