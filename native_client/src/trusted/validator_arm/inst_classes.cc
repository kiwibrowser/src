/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator_arm/inst_classes_inline.h"

// Implementations of instruction classes, for those not completely defined in
// in the header.

namespace nacl_arm_dec {

// ClassDecoder
RegisterList ClassDecoder::defs(Instruction i) const {
  UNREFERENCED_PARAMETER(i);
  return RegisterList::Everything();
}

RegisterList ClassDecoder::uses(Instruction i) const {
  UNREFERENCED_PARAMETER(i);
  return RegisterList();
}

bool ClassDecoder::base_address_register_writeback_small_immediate(
    Instruction i) const {
  UNREFERENCED_PARAMETER(i);
  return false;
}

Register ClassDecoder::base_address_register(Instruction i) const {
  UNREFERENCED_PARAMETER(i);
  return Register::None();
}

bool ClassDecoder::is_literal_load(Instruction i) const {
  UNREFERENCED_PARAMETER(i);
  return false;
}

Register ClassDecoder::branch_target_register(Instruction i) const {
  UNREFERENCED_PARAMETER(i);
  return Register::None();
}

bool ClassDecoder::is_relative_branch(Instruction i) const {
  UNREFERENCED_PARAMETER(i);
  return false;
}

int32_t ClassDecoder::branch_target_offset(Instruction i) const {
  UNREFERENCED_PARAMETER(i);
  return 0;
}

bool ClassDecoder::is_literal_pool_head(Instruction i) const {
  UNREFERENCED_PARAMETER(i);
  return false;
}

bool ClassDecoder::clears_bits(Instruction i, uint32_t mask) const {
  UNREFERENCED_PARAMETER(i);
  UNREFERENCED_PARAMETER(mask);
  return false;
}

bool ClassDecoder::sets_Z_if_bits_clear(Instruction i,
                                        Register r,
                                        uint32_t mask) const {
  UNREFERENCED_PARAMETER(i);
  UNREFERENCED_PARAMETER(r);
  UNREFERENCED_PARAMETER(mask);
  return false;
}

bool ClassDecoder::is_load_thread_address_pointer(Instruction i) const {
  UNREFERENCED_PARAMETER(i);
  return false;
}

Instruction ClassDecoder::
dynamic_code_replacement_sentinel(Instruction i) const {
  return i;
}

ViolationSet ClassDecoder::get_violations(
    const nacl_arm_val::DecodedInstruction& first,
    const nacl_arm_val::DecodedInstruction& second,
    const nacl_arm_val::SfiValidator& sfi,
    nacl_arm_val::AddressSet* branches,
    nacl_arm_val::AddressSet* critical,
    uint32_t* next_inst_addr) const {
  UNREFERENCED_PARAMETER(next_inst_addr);
  ViolationSet violations = kNoViolations;

  // Start by checking safety.
  if (second.safety() != nacl_arm_dec::MAY_BE_SAFE)
    violations = SafetyViolationBit(second.safety());

  // Skip adding get_loadstore_violations, assuming the code generator adds them
  // whenever field 'base' is defined for a class decoder in armv7.table.

  // Skip get_branch_mask_violations, assuming the code generator adds them
  // whenever field 'target' is defined for a class decoder in armv7.table.

  violations = ViolationUnion(
      violations,
      get_data_register_update_violations(first, second, sfi, critical));

  // Skip get_call_position_violations, assuming the code generator adds them
  // whenever fields 'target' or 'relative' is defined for a class decoder
  // in armv7.table.

  violations = ViolationUnion(
      violations, get_read_only_violations(second, sfi));
  violations = ViolationUnion(
      violations, get_read_thread_local_pointer_violations(second));
  violations = ViolationUnion(
      violations, get_pc_writes_violations(second, sfi, branches));

  // Skip processing constant pool heads, assuming the code generator adds them
  // whenever field 'is_literal_pool_head' is defined for a class decoder
  // in armv7.table.

  return violations;
}

void ClassDecoder::generate_diagnostics(
    ViolationSet violations,
    const nacl_arm_val::DecodedInstruction& first,
    const nacl_arm_val::DecodedInstruction& second,
    const nacl_arm_val::SfiValidator& sfi,
    nacl_arm_val::ProblemSink* out) const {

  if (ContainsSafetyViolations(violations)) {
    // Note: We assume that safety levels end with MAY_BE_SAFE, as stated
    // for enum type SafetyLevel.
    uint32_t addr = second.addr();
    for (uint32_t safety = 0; safety != MAY_BE_SAFE; ++safety) {
      Violation violation = static_cast<Violation>(safety);
      if (ContainsViolation(violations, violation)) {
        switch (static_cast<int>(violation)) {
          case nacl_arm_dec::UNINITIALIZED_VIOLATION:
          default:
            out->ReportProblemDiagnostic(
                violation, addr,
                "Unknown error occurred decoding this instruction.");
            break;
          case nacl_arm_dec::UNKNOWN_VIOLATION:
            out->ReportProblemDiagnostic(
                violation, addr,
                "The value assigned to registers by this instruction "
                "is unknown.");
            break;
          case nacl_arm_dec::UNDEFINED_VIOLATION:
            out->ReportProblemDiagnostic(
                violation, addr,
                "Instruction is undefined according to the ARMv7"
                " ISA specifications.");
            break;
          case nacl_arm_dec::NOT_IMPLEMENTED_VIOLATION:
            // This instruction is not recognized by the decoder functions.
            out->ReportProblemDiagnostic(
                violation, addr,
                "Instruction not understood by Native Client.");
            break;
          case nacl_arm_dec::UNPREDICTABLE_VIOLATION:
            out->ReportProblemDiagnostic(
                violation, addr,
                "Instruction has unpredictable effects at runtime.");
            break;
          case nacl_arm_dec::DEPRECATED_VIOLATION:
            out->ReportProblemDiagnostic(
                violation, addr,
                "Instruction is deprecated in ARMv7.");
            break;
          case nacl_arm_dec::FORBIDDEN_VIOLATION:
            out->ReportProblemDiagnostic(
                violation, addr,
                "Instruction not allowed by Native Client.");
            break;
          case nacl_arm_dec::FORBIDDEN_OPERANDS_VIOLATION:
            out->ReportProblemDiagnostic(
                violation, addr,
                "Instruction has operand(s) forbidden by Native Client.");
            break;
          case nacl_arm_dec::DECODER_ERROR_VIOLATION:
            out->ReportProblemDiagnostic(
                violation, addr,
                "Instruction decoded incorrectly by NativeClient.");
            break;
        };
      }
    }
  }

  generate_loadstore_diagnostics(violations, first, second, sfi, out);
  generate_branch_mask_diagnostics(violations, first, second, sfi, out);
  generate_data_register_update_diagnostics(violations, first, second,
                                            sfi, out);
  generate_call_position_diagnostics(violations, second, sfi, out);
  generate_read_only_diagnostics(violations, second, sfi, out);
  generate_read_thread_local_pointer_diagnostics(violations, second, sfi, out);
  generate_pc_writes_diagnostics(violations, second, sfi, out);
}

}  // namespace nacl_arm_dec
