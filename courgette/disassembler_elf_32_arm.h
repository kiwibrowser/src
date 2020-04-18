// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COURGETTE_DISASSEMBLER_ELF_32_ARM_H_
#define COURGETTE_DISASSEMBLER_ELF_32_ARM_H_

#include <stddef.h>
#include <stdint.h>

#include <map>

#include "base/macros.h"
#include "courgette/disassembler_elf_32.h"
#include "courgette/types_elf.h"

namespace courgette {

class InstructionReceptor;

enum ARM_RVA {
  ARM_OFF8,
  ARM_OFF11,
  ARM_OFF24,
  ARM_OFF25,
  ARM_OFF21,
};

class DisassemblerElf32ARM : public DisassemblerElf32 {
 public:
  // Returns true if a valid executable is detected using only quick checks.
  static bool QuickDetect(const uint8_t* start, size_t length) {
    return DisassemblerElf32::QuickDetect(start, length, EM_ARM);
  }

  class TypedRVAARM : public TypedRVA {
   public:
    TypedRVAARM(ARM_RVA type, RVA rva) : TypedRVA(rva), type_(type) { }
    ~TypedRVAARM() override { }

    // TypedRVA interfaces.
    CheckBool ComputeRelativeTarget(const uint8_t* op_pointer) override;
    CheckBool EmitInstruction(Label* label,
                              InstructionReceptor* receptor) override;
    uint16_t op_size() const override;

    uint16_t c_op() const { return c_op_; }

   private:
    ARM_RVA type_;
    uint16_t c_op_;  // Set by ComputeRelativeTarget().
    const uint8_t* arm_op_;
  };

  DisassemblerElf32ARM(const uint8_t* start, size_t length);

  ~DisassemblerElf32ARM() override { }

  // DisassemblerElf32 interfaces.
  ExecutableType kind() const override { return EXE_ELF_32_ARM; }
  e_machine_values ElfEM() const override { return EM_ARM; }

  // Takes an ARM or thumb opcode |arm_op| of specified |type| and located at
  // |rva|, extracts the instruction-relative target RVA into |*addr| and
  // encodes the corresponding Courgette opcode as |*c_op|.
  //
  // Details on ARM opcodes, and target RVA extraction are taken from
  // "ARM Architecture Reference Manual", section A4.1.5 and
  // "Thumb-2 supplement", section 4.6.12.
  // ARM_OFF24 is for the ARM opcode. The rest are for thumb opcodes.
  static CheckBool Compress(ARM_RVA type,
                            uint32_t arm_op,
                            RVA rva,
                            uint16_t* c_op /* out */,
                            uint32_t* addr /* out */);

  // Inverse for Compress(). Takes Courgette op |c_op| and relative address
  // |addr| to reconstruct the original ARM or thumb op |*arm_op|.
  static CheckBool Decompress(ARM_RVA type,
                              uint16_t c_op,
                              uint32_t addr,
                              uint32_t* arm_op /* out */);

 protected:
  // DisassemblerElf32 interfaces.
  CheckBool RelToRVA(Elf32_Rel rel,
                     RVA* result) const override WARN_UNUSED_RESULT;
  CheckBool ParseRelocationSection(const Elf32_Shdr* section_header,
                                   InstructionReceptor* receptor) const override
      WARN_UNUSED_RESULT;
  CheckBool ParseRel32RelocsFromSection(const Elf32_Shdr* section)
      override WARN_UNUSED_RESULT;

#if COURGETTE_HISTOGRAM_TARGETS
  std::map<RVA, int> rel32_target_rvas_;
#endif

 private:
  DISALLOW_COPY_AND_ASSIGN(DisassemblerElf32ARM);
};

}  // namespace courgette

#endif  // COURGETTE_DISASSEMBLER_ELF_32_ARM_H_
