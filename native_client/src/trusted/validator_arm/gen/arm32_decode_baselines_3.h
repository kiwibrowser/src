/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_BASELINES_3_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_BASELINES_3_H_

#include "native_client/src/trusted/validator_arm/arm_helpers.h"
#include "native_client/src/trusted/validator_arm/inst_classes.h"

namespace nacl_arm_dec {

// VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d10nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    regs: 1
//         if type(11:8)=0111
//         else 2
//         if type(11:8)=1010
//         else 3
//         if type(11:8)=0110
//         else 4
//         if type(11:8)=0010
//         else 0,
//    rule: VLD1_multiple_single_elements,
//    safety: [type(11:8)=0111 &&
//         align(1)=1 => UNDEFINED,
//      type(11:8)=1010 &&
//         align(5:4)=11 => UNDEFINED,
//      type(11:8)=0110 &&
//         align(1)=1 => UNDEFINED,
//      not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0
     : public ClassDecoder {
 public:
  VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0);
};

// VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    T: T(5),
//    Vd: Vd(15:12),
//    a: a(4),
//    alignment: 1
//         if a(4)=0
//         else ebytes,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(7:6),
//      T(5),
//      a(4),
//      Rm(3:0)],
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnndddd1100sstammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    regs: 1
//         if T(5)=0
//         else 2,
//    rule: VLD1_single_element_to_all_lanes,
//    safety: [size(7:6)=11 ||
//         (size(7:6)=00 &&
//         a(4)=1) => UNDEFINED,
//      n  ==
//            Pc ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0
     : public ClassDecoder {
 public:
  VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0);
};

// VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: 1
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(1:0)=00
//         else 4)
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnnddddss00aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD1_single_element_to_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=00 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=01 &&
//         index_align(1)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(2)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=~00 &&
//         index_align(1:0)=~11 => UNDEFINED,
//      n  ==
//            Pc => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0
     : public ClassDecoder {
 public:
  VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0);
};

// VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    inc: 1
//         if type(11:8)=1000
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d10nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    regs: 1
//         if type in bitset {'1000', '1001'}
//         else 2,
//    rule: VLD2_multiple_2_element_structures,
//    safety: [size(7:6)=11 => UNDEFINED,
//      type in bitset {'1000', '1001'} &&
//         align(5:4)=11 => UNDEFINED,
//      not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d2 + regs  >
//            32 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0
     : public ClassDecoder {
 public:
  VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0);
};

// VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    T: T(5),
//    Vd: Vd(15:12),
//    a: a(4),
//    alignment: 1
//         if a(4)=0
//         else 2 * ebytes,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(7:6),
//      T(5),
//      a(4),
//      Rm(3:0)],
//    inc: 1
//         if T(5)=0
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnndddd1101sstammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD2_single_2_element_structure_to_all_lanes,
//    safety: [size(7:6)=11 => UNDEFINED,
//      n  ==
//            Pc ||
//         d2  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0
     : public ClassDecoder {
 public:
  VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0);
};

// VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: (1
//         if index_align(0)=0
//         else 2)
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 4)
//         if size(11:10)=01
//         else (1
//         if index_align(0)=0
//         else 8)
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnnddddss01aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD2_single_2_element_structure_to_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1)=~0 => UNDEFINED,
//      n  ==
//            Pc ||
//         d2  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0
     : public ClassDecoder {
 public:
  VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0);
};

// VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(0)=0
//         else 8,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    inc: 1
//         if type(11:8)=0100
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d10nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD3_multiple_3_element_structures,
//    safety: [size(7:6)=11 ||
//         align(1)=1 => UNDEFINED,
//      not type in bitset {'0100', '0101'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d3  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0
     : public ClassDecoder {
 public:
  VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0);
};

// VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    T: T(5),
//    Vd: Vd(15:12),
//    a: a(4),
//    alignment: 1,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(7:6),
//      T(5),
//      a(4),
//      Rm(3:0)],
//    inc: 1
//         if T(5)=0
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnndddd1110sstammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD3_single_3_element_structure_to_all_lanes,
//    safety: [size(7:6)=11 ||
//         a(4)=1 => UNDEFINED,
//      n  ==
//            Pc ||
//         d3  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0
     : public ClassDecoder {
 public:
  VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0);
};

// VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: 1,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnnddddss10aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD3_single_3_element_structure_to_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=00 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=01 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=~00 => UNDEFINED,
//      n  ==
//            Pc ||
//         d3  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0
     : public ClassDecoder {
 public:
  VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0);
};

// VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    d4: d3 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    inc: 1
//         if type(11:8)=0000
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d10nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD4_multiple_4_element_structures,
//    safety: [size(7:6)=11 => UNDEFINED,
//      not type in bitset {'0000', '0001'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d4  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0
     : public ClassDecoder {
 public:
  VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0);
};

// VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    T: T(5),
//    Vd: Vd(15:12),
//    a: a(4),
//    alignment: 16
//         if size(7:6)=11
//         else (1
//         if a(4)=0
//         else 8)
//         if size(7:6)=10
//         else (1
//         if a(4)=0
//         else 4 * ebytes),
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    d4: d3 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(7:6),
//      T(5),
//      a(4),
//      Rm(3:0)],
//    inc: 1
//         if T(5)=0
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnndddd1111sstammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD4_single_4_element_structure_to_all_lanes,
//    safety: [size(7:6)=11 &&
//         a(4)=0 => UNDEFINED,
//      n  ==
//            Pc ||
//         d4  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0
     : public ClassDecoder {
 public:
  VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0);
};

// VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: (1
//         if index_align(0)=0
//         else 4)
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 8)
//         if size(11:10)=01
//         else (1
//         if index_align(1:0)=00
//         else 4 << index_align(1:0))
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    d4: d3 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnnddddss11aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD4_single_4_element_structure_to_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=11 => UNDEFINED,
//      n  ==
//            Pc ||
//         d4  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0
     : public ClassDecoder {
 public:
  VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0);
};

// VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0:
//
//   {D: D(22),
//    None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Sp: 13,
//    U: U(23),
//    Vd: Vd(15:12),
//    W: W(21),
//    add: U(23)=1,
//    arch: VFPv2,
//    base: Rn,
//    cond: cond(31:28),
//    d: Vd:D,
//    defs: {Rn
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      D(22),
//      W(21),
//      Rn(19:16),
//      Vd(15:12),
//      imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    is_literal_load: Rn  ==
//            Pc,
//    n: Rn,
//    pattern: cccc110pudw1nnnndddd1010iiiiiiii,
//    regs: imm8,
//    rule: VLDM,
//    safety: [P(24)=0 &&
//         U(23)=0 &&
//         W(21)=0 => DECODER_ERROR,
//      P(24)=1 &&
//         W(21)=0 => DECODER_ERROR,
//      P  ==
//            U &&
//         W(21)=1 => UNDEFINED,
//      n  ==
//            Pc &&
//         wback => UNPREDICTABLE,
//      P(24)=0 &&
//         U(23)=1 &&
//         W(21)=1 &&
//         Rn  ==
//            Sp => DECODER_ERROR,
//      regs  ==
//            0 ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    single_regs: true,
//    small_imm_base_wb: wback,
//    true: true,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0);
};

// VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0:
//
//   {D: D(22),
//    None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Sp: 13,
//    U: U(23),
//    Vd: Vd(15:12),
//    W: W(21),
//    add: U(23)=1,
//    arch: ['VFPv2', 'AdvSIMD'],
//    base: Rn,
//    cond: cond(31:28),
//    d: D:Vd,
//    defs: {Rn
//         if wback
//         else None},
//    false: false,
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      D(22),
//      W(21),
//      Rn(19:16),
//      Vd(15:12),
//      imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    is_literal_load: Rn  ==
//            Pc,
//    n: Rn,
//    pattern: cccc110pudw1nnnndddd1011iiiiiiii,
//    regs: imm8 / 2,
//    rule: VLDM,
//    safety: [P(24)=0 &&
//         U(23)=0 &&
//         W(21)=0 => DECODER_ERROR,
//      P(24)=1 &&
//         W(21)=0 => DECODER_ERROR,
//      P  ==
//            U &&
//         W(21)=1 => UNDEFINED,
//      n  ==
//            Pc &&
//         wback => UNPREDICTABLE,
//      P(24)=0 &&
//         U(23)=1 &&
//         W(21)=1 &&
//         Rn  ==
//            Sp => DECODER_ERROR,
//      regs  ==
//            0 ||
//         regs  >
//            16 ||
//         d + regs  >
//            32 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         d + regs  >
//            16 => UNPREDICTABLE,
//      imm8(0)  ==
//            1 => DEPRECATED],
//    single_regs: false,
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: W(21)=1}
class VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0);
};

// VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0:
//
//   {D: D(22),
//    Pc: 15,
//    Rn: Rn(19:16),
//    U: U(23),
//    Vd: Vd(15:12),
//    add: U(23)=1,
//    arch: VFPv2,
//    base: Rn,
//    cond: cond(31:28),
//    d: D:Vd,
//    defs: {},
//    fields: [cond(31:28), U(23), D(22), Rn(19:16), Vd(15:12), imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    is_literal_load: Rn  ==
//            Pc,
//    n: Rn,
//    pattern: cccc1101ud01nnnndddd1010iiiiiiii,
//    rule: VLDR,
//    single_reg: true,
//    true: true,
//    uses: {Rn},
//    violations: [implied by 'base']}
class VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0);
};

// VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0:
//
//   {D: D(22),
//    Pc: 15,
//    Rn: Rn(19:16),
//    U: U(23),
//    Vd: Vd(15:12),
//    add: U(23)=1,
//    arch: ['VFPv2', 'AdvSIMD'],
//    base: Rn,
//    cond: cond(31:28),
//    d: D:Vd,
//    defs: {},
//    false: false,
//    fields: [cond(31:28), U(23), D(22), Rn(19:16), Vd(15:12), imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    is_literal_load: Rn  ==
//            Pc,
//    n: Rn,
//    pattern: cccc1101ud01nnnndddd1011iiiiiiii,
//    rule: VLDR,
//    single_reg: false,
//    uses: {Rn},
//    violations: [implied by 'base']}
class VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0);
};

// VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0110nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMAX,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0);
};

// VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100dssnnnndddd1111nqm0mmmm,
//    rule: VMAX_floating_point,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0);
};

// VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0110nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMIN,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0);
};

// VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100dssnnnndddd1111nqm0mmmm,
//    rule: VMIN_floating_point,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0);
};

// VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd10p0n0m0mmmm,
//    rule: VMLAL_VMLSL_integer_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0);
};

// VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001u1dssnnnndddd0p10n1m0mmmm,
//    regs: 1,
//    rule: VMLAL_by_scalar_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
class VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0);
};

// VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    add: op(6)=0,
//    advsimd: false,
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      op(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    op: op(6),
//    pattern: cccc11100d00nnnndddd101snom0mmmm,
//    rule: VMLA_VMLS_floating_point,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
class VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0
     : public ClassDecoder {
 public:
  VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0);
};

// VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M,
//    m: Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMLA_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         size(21:20)=01) => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0);
};

// VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMLA_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1
     : public ClassDecoder {
 public:
  VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1);
};

// VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100dpsnnnndddd1101nqm1mmmm,
//    rule: VMLA_floating_point_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0);
};

// VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1001nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMLA_integer_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0);
};

// VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001u1dssnnnndddd0p10n1m0mmmm,
//    regs: 1,
//    rule: VMLSL_by_scalar_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
class VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0);
};

// VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M,
//    m: Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMLS_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         size(21:20)=01) => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0);
};

// VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMLS_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1
     : public ClassDecoder {
 public:
  VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1);
};

// VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100dpsnnnndddd1101nqm1mmmm,
//    rule: VMLS_floating_point_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0);
};

// VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1001nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMLS_integer_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0);
};

// VMOVN_111100111d11ss10dddd001000m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd001000m0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMOVN,
//    safety: [size(19:18)=11 => UNDEFINED, Vm(0)=1 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VMOVN_111100111d11ss10dddd001000m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMOVN_111100111d11ss10dddd001000m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMOVN_111100111d11ss10dddd001000m0mmmm_case_0);
};

// VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_0:
//
//   {D: D(7),
//    Pc: 15,
//    Rt: Rt(15:12),
//    Vd: Vd(19:16),
//    advsimd: sel in bitset {'1xxx', '0xx1'},
//    arch: ['VFPv2', 'AdvSIMD'],
//    cond: cond(31:28),
//    d: D:Vd,
//    defs: {},
//    esize: 8
//         if opc1:opc2(3:0)=1xxx
//         else 16
//         if opc1:opc2(3:0)=0xx1
//         else 32
//         if opc1:opc2(3:0)=0x00
//         else 0,
//    fields: [cond(31:28),
//      opc1(22:21),
//      Vd(19:16),
//      Rt(15:12),
//      D(7),
//      opc2(6:5)],
//    index: opc1(0):opc2
//         if opc1:opc2(3:0)=1xxx
//         else opc1(0):opc2(1)
//         if opc1:opc2(3:0)=0xx1
//         else opc1(0)
//         if opc1:opc2(3:0)=0x00
//         else 0,
//    opc1: opc1(22:21),
//    opc2: opc2(6:5),
//    pattern: cccc11100ii0ddddtttt1011dii10000,
//    rule: VMOV_ARM_core_register_to_scalar,
//    safety: [opc1:opc2(3:0)=0x10 => UNDEFINED,
//      t  ==
//            Pc => UNPREDICTABLE],
//    sel: opc1:opc2,
//    t: Rt,
//    uses: {Rt}}
class VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_0
     : public ClassDecoder {
 public:
  VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_0);
};

// VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_0:
//
//   {N: N(7),
//    None: 32,
//    Pc: 15,
//    Rt: Rt(15:12),
//    Vn: Vn(19:16),
//    arch: VFPv2,
//    cond: cond(31:28),
//    defs: {Rt
//         if to_arm_register
//         else None},
//    fields: [cond(31:28), op(20), Vn(19:16), Rt(15:12), N(7)],
//    n: Vn:N,
//    op: op(20),
//    pattern: cccc1110000onnnntttt1010n0010000,
//    rule: VMOV_between_ARM_core_register_and_single_precision_register,
//    safety: [t  ==
//            Pc => UNPREDICTABLE],
//    t: Rt,
//    to_arm_register: op(20)=1,
//    uses: {Rt
//         if not to_arm_register
//         else None}}
class VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_0
     : public ClassDecoder {
 public:
  VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_0);
};

// VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000xnnnntttt1010n0010000_case_0:
//
//   {N: N(7),
//    None: 32,
//    Pc: 15,
//    Rt: Rt(15:12),
//    Vn: Vn(19:16),
//    arch: VFPv2,
//    cond: cond(31:28),
//    defs: {Rt
//         if to_arm_register
//         else None},
//    fields: [cond(31:28), op(20), Vn(19:16), Rt(15:12), N(7)],
//    n: Vn:N,
//    op: op(20),
//    pattern: cccc1110000xnnnntttt1010n0010000,
//    rule: VMOV_between_ARM_core_register_and_single_precision_register,
//    safety: [t  ==
//            Pc => UNPREDICTABLE],
//    t: Rt,
//    to_arm_register: op(20)=1,
//    uses: {Rt
//         if not to_arm_register
//         else None}}
class VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000xnnnntttt1010n0010000_case_0
     : public ClassDecoder {
 public:
  VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000xnnnntttt1010n0010000_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000xnnnntttt1010n0010000_case_0);
};

// VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_0:
//
//   {M: M(5),
//    Pc: 15,
//    Rt: Rt(15:12),
//    Rt2: Rt2(19:16),
//    Vm: Vm(3:0),
//    arch: ['VFPv2', 'AdvSIMD'],
//    cond: cond(31:28),
//    defs: {Rt, Rt2}
//         if to_arm_registers
//         else {},
//    fields: [cond(31:28), op(20), Rt2(19:16), Rt(15:12), M(5), Vm(3:0)],
//    m: M:Vm,
//    op: op(20),
//    pattern: cccc1100010otttttttt101100m1mmmm,
//    rule: VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register,
//    safety: [Pc in {t, t2} => UNPREDICTABLE,
//      to_arm_registers &&
//         t  ==
//            t2 => UNPREDICTABLE],
//    t: Rt,
//    t2: Rt2,
//    to_arm_registers: op(20)=1,
//    uses: {}
//         if to_arm_registers
//         else {Rt, Rt2}}
class VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_0
     : public ClassDecoder {
 public:
  VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_0);
};

// VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_0:
//
//   {M: M(5),
//    Pc: 15,
//    Rt: Rt(15:12),
//    Rt2: Rt2(19:16),
//    Vm: Vm(3:0),
//    arch: ['VFPv2'],
//    cond: cond(31:28),
//    defs: {Rt, Rt2}
//         if to_arm_registers
//         else {},
//    fields: [cond(31:28), op(20), Rt2(19:16), Rt(15:12), M(5), Vm(3:0)],
//    m: Vm:M,
//    op: op(20),
//    pattern: cccc1100010otttttttt101000m1mmmm,
//    rule: VMOV_between_two_ARM_core_registers_and_two_single_precision_registers,
//    safety: [Pc in {t, t2} ||
//         m  ==
//            31 => UNPREDICTABLE,
//      to_arm_registers &&
//         t  ==
//            t2 => UNPREDICTABLE],
//    t: Rt,
//    t2: Rt2,
//    to_arm_registers: op(20)=1,
//    uses: {}
//         if to_arm_registers
//         else {Rt, Rt2}}
class VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_0
     : public ClassDecoder {
 public:
  VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_0);
};

// VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_0:
//
//   {D: D(22),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    arch: ASIMD,
//    cmode: cmode(11:8),
//    d: D:Vd,
//    defs: {},
//    false: false,
//    fields: [i(24),
//      D(22),
//      imm3(18:16),
//      Vd(15:12),
//      cmode(11:8),
//      Q(6),
//      op(5),
//      imm4(3:0)],
//    i: i(24),
//    imm3: imm3(18:16),
//    imm4: imm4(3:0),
//    imm64: AdvSIMDExpandImm(op, cmode, i:imm3:imm4),
//    op: op(5),
//    pattern: 1111001m1d000mmmddddcccc0qp1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMOV_immediate_A1,
//    safety: [op(5)=0 &&
//         cmode(0)=1 &&
//         cmode(3:2)=~11 => DECODER_ERROR,
//      op(5)=1 &&
//         cmode(11:8)=~1110 => DECODER_ERROR,
//      Q(6)=1 &&
//         Vd(0)=1 => UNDEFINED],
//    single_register: false,
//    uses: {}}
class VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_0
     : public ClassDecoder {
 public:
  VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_0);
};

// VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0:
//
//   {D: D(22),
//    Vd: Vd(15:12),
//    advsimd: false,
//    arch: VFPv3,
//    cond: cond(31:28),
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      imm4H(19:16),
//      Vd(15:12),
//      sz(8),
//      imm4L(3:0)],
//    imm32: VFPExpandImm(imm4H:imm4L, 32),
//    imm4H: imm4H(19:16),
//    imm4L: imm4L(3:0),
//    imm64: VFPExpandImm(imm4H:imm4L, 64),
//    pattern: cccc11101d11iiiidddd101s0000iiii,
//    regs: 1,
//    rule: VMOV_immediate,
//    safety: [true => MAY_BE_SAFE],
//    single_register: sz(8)=0,
//    sz: sz(8),
//    true: true,
//    uses: {}}
class VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0
     : public ClassDecoder {
 public:
  VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0);
};

// VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    advsimd: false,
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), M(5), Vm(3:0)],
//    m: Vm:D
//         if sz(8)=0
//         else M:Vm,
//    pattern: cccc11101d110000dddd101s01m0mmmm,
//    regs: 1,
//    rule: VMOV_register,
//    safety: [true => MAY_BE_SAFE],
//    single_register: sz(8)=0,
//    sz: sz(8),
//    true: true,
//    uses: {}}
class VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0);
};

// VMRS_cccc111011110001tttt101000010000_case_0:
//
//   {NZCV: 16,
//    Pc: 15,
//    Rt: Rt(15:12),
//    arch: ['VFPv2', 'AdvSIMD'],
//    cond: cond(31:28),
//    defs: {NZCV
//         if t  ==
//            Pc
//         else Rt},
//    fields: [cond(31:28), Rt(15:12)],
//    pattern: cccc111011110001tttt101000010000,
//    rule: VMRS,
//    t: Rt}
class VMRS_cccc111011110001tttt101000010000_case_0
     : public ClassDecoder {
 public:
  VMRS_cccc111011110001tttt101000010000_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMRS_cccc111011110001tttt101000010000_case_0);
};

// VMSR_cccc111011100001tttt101000010000_case_0:
//
//   {Pc: 15,
//    Rt: Rt(15:12),
//    arch: ['VFPv2', 'AdvSIMD'],
//    cond: cond(31:28),
//    defs: {},
//    fields: [cond(31:28), Rt(15:12)],
//    pattern: cccc111011100001tttt101000010000,
//    rule: VMSR,
//    safety: [t  ==
//            Pc => UNPREDICTABLE],
//    t: Rt,
//    uses: {Rt}}
class VMSR_cccc111011100001tttt101000010000_case_0
     : public ClassDecoder {
 public:
  VMSR_cccc111011100001tttt101000010000_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMSR_cccc111011100001tttt101000010000_case_0);
};

// VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001u1dssnnnndddd1010n1m0mmmm,
//    regs: 1,
//    rule: VMULL_by_scalar_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
class VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0);
};

// VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd11p0n0m0mmmm,
//    rule: VMULL_integer_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0);
};

// VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd11p0n0m0mmmm,
//    rule: VMULL_polynomial_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      U(24)=1 ||
//         size(21:20)=~00 => UNDEFINED,
//      Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0);
};

// VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M,
//    m: Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd100fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMUL_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         size(21:20)=01) => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0);
};

// VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd100fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMUL_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1
     : public ClassDecoder {
 public:
  VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1);
};

// VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110d0snnnndddd1101nqm1mmmm,
//    rule: VMUL_floating_point_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0);
};

// VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    advsimd: false,
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    pattern: cccc11100d10nnnndddd101sn0m0mmmm,
//    rule: VMUL_floating_point,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
class VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0
     : public ClassDecoder {
 public:
  VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0);
};

// VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMUL_integer_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0);
};

// VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    false: false,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMUL_polynomial_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=~00 => UNDEFINED],
//    size: size(21:20),
//    unsigned: false,
//    uses: {}}
class VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0);
};

// VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0:
//
//   {D: D(22),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    arch: ASIMD,
//    cmode: cmode(11:8),
//    d: D:Vd,
//    defs: {},
//    fields: [i(24),
//      D(22),
//      imm3(18:16),
//      Vd(15:12),
//      cmode(11:8),
//      Q(6),
//      op(5),
//      imm4(3:0)],
//    i: i(24),
//    imm3: imm3(18:16),
//    imm4: imm4(3:0),
//    imm64: AdvSIMDExpandImm(op, cmode, i:imm3:imm4),
//    op: op(5),
//    pattern: 1111001i1d000mmmddddcccc0q11mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMVN_immediate,
//    safety: [(cmode(0)=1 &&
//         cmode(3:2)=~11) ||
//         cmode(3:1)=111 => DECODER_ERROR,
//      Q(6)=1 &&
//         Vd(0)=1 => UNDEFINED],
//    uses: {}}
class VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0
     : public ClassDecoder {
 public:
  VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0);
};

// VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01011qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMVN_register,
//    safety: [size(19:18)=~00 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0
     : public ClassDecoder {
 public:
  VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0);
};

// VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f111qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VNEG,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0
     : public ClassDecoder {
 public:
  VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0);
};

// VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f111qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VNEG,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1
     : public ClassDecoder {
 public:
  VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1);
};

// VNEG_cccc11101d110001dddd101s01m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    advsimd: false,
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), M(5), Vm(3:0)],
//    m: Vm:D
//         if sz(8)=0
//         else M:Vm,
//    pattern: cccc11101d110001dddd101s01m0mmmm,
//    rule: VNEG,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {}}
class VNEG_cccc11101d110001dddd101s01m0mmmm_case_0
     : public ClassDecoder {
 public:
  VNEG_cccc11101d110001dddd101s01m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VNEG_cccc11101d110001dddd101s01m0mmmm_case_0);
};

// VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    VFPNegMul_VNMLA: 1,
//    VFPNegMul_VNMLS: 2,
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      op(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    op: op(6),
//    pattern: cccc11100d01nnnndddd101snom0mmmm,
//    rule: VNMLA_VNMLS,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    type: VFPNegMul_VNMLA
//         if op(6)=1
//         else VFPNegMul_VNMLS,
//    uses: {}}
class VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0
     : public ClassDecoder {
 public:
  VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0);
};

// VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    VFPNegMul_VNMUL: 3,
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    pattern: cccc11100d10nnnndddd101sn1m0mmmm,
//    rule: VNMUL,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    type: VFPNegMul_VNMUL,
//    uses: {}}
class VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0);
};

// VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100d11nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VORN_register,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0);
};

// VORR_immediate_1111001i1d000mmmddddcccc0q01mmmm_case_0:
//
//   {D: D(22),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    arch: ASIMD,
//    cmode: cmode(11:8),
//    d: D:Vd,
//    defs: {},
//    fields: [i(24),
//      D(22),
//      imm3(18:16),
//      Vd(15:12),
//      cmode(11:8),
//      Q(6),
//      op(5),
//      imm4(3:0)],
//    i: i(24),
//    imm3: imm3(18:16),
//    imm4: imm4(3:0),
//    imm64: AdvSIMDExpandImm(op, cmode, i:imm3:imm4),
//    op: op(5),
//    pattern: 1111001i1d000mmmddddcccc0q01mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VORR_immediate,
//    safety: [cmode(0)=0 ||
//         cmode(3:2)=11 => DECODER_ERROR,
//      Q(6)=1 &&
//         Vd(0)=1 => UNDEFINED],
//    uses: {}}
class VORR_immediate_1111001i1d000mmmddddcccc0q01mmmm_case_0
     : public ClassDecoder {
 public:
  VORR_immediate_1111001i1d000mmmddddcccc0q01mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VORR_immediate_1111001i1d000mmmddddcccc0q01mmmm_case_0);
};

// VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100d10nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VORR_register_or_VMOV_register_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0);
};

// VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd0110pqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VPADAL,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    unsigned: (op(0)=1),
//    uses: {}}
class VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPADAL_111100111d11ss00dddd0110pqm0mmmm_case_0);
};

// VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd0010pqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VPADDL,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    unsigned: (op(0)=1),
//    uses: {}}
class VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPADDL_111100111d11ss00dddd0010pqm0mmmm_case_0);
};

// VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110d0snnnndddd1101nqm0mmmm,
//    rule: VPADD_floating_point,
//    safety: [size(0)=1 ||
//         Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0);
};

// VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100dssnnnndddd1011n0m1mmmm,
//    rule: VPADD_integer,
//    safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0
     : public ClassDecoder {
 public:
  VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0);
};

// VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110dssnnnndddd1111nqm0mmmm,
//    rule: VPMAX,
//    safety: [size(0)=1 ||
//         Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0);
};

// VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1010n0m0mmmm,
//    rule: VPMAX,
//    safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0
     : public ClassDecoder {
 public:
  VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0);
};

// VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110dssnnnndddd1111nqm0mmmm,
//    rule: VPMIN,
//    safety: [size(0)=1 ||
//         Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0);
};

// VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1010n0m1mmmm,
//    rule: VPMIN,
//    safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0
     : public ClassDecoder {
 public:
  VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0);
};

// VPOP_cccc11001d111101dddd1010iiiiiiii_case_0:
//
//   {D: D(22),
//    Sp: 13,
//    Vd: Vd(15:12),
//    arch: VFPv2,
//    base: Sp,
//    cond: cond(31:28),
//    d: Vd:D,
//    defs: {Sp},
//    fields: [cond(31:28), D(22), Vd(15:12), imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    pattern: cccc11001d111101dddd1010iiiiiiii,
//    regs: imm8,
//    rule: VPOP,
//    safety: [regs  ==
//            0 ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    single_regs: true,
//    small_imm_base_wb: true,
//    true: true,
//    uses: {Sp},
//    violations: [implied by 'base']}
class VPOP_cccc11001d111101dddd1010iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VPOP_cccc11001d111101dddd1010iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPOP_cccc11001d111101dddd1010iiiiiiii_case_0);
};

// VPOP_cccc11001d111101dddd1011iiiiiiii_case_0:
//
//   {D: D(22),
//    Sp: 13,
//    Vd: Vd(15:12),
//    arch: ['VFPv2', 'AdvSIMD'],
//    base: Sp,
//    cond: cond(31:28),
//    d: D:Vd,
//    defs: {Sp},
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    pattern: cccc11001d111101dddd1011iiiiiiii,
//    regs: imm8 / 2,
//    rule: VPOP,
//    safety: [regs  ==
//            0 ||
//         regs  >
//            16 ||
//         d + regs  >
//            32 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         d + regs  >
//            16 => UNPREDICTABLE,
//      imm8(0)  ==
//            1 => DEPRECATED],
//    single_regs: false,
//    small_imm_base_wb: true,
//    true: true,
//    uses: {Sp},
//    violations: [implied by 'base']}
class VPOP_cccc11001d111101dddd1011iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VPOP_cccc11001d111101dddd1011iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPOP_cccc11001d111101dddd1011iiiiiiii_case_0);
};

// VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0:
//
//   {D: D(22),
//    Sp: 13,
//    Vd: Vd(15:12),
//    arch: VFPv2,
//    base: Sp,
//    cond: cond(31:28),
//    d: Vd:D,
//    defs: {Sp},
//    fields: [cond(31:28), D(22), Vd(15:12), imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    pattern: cccc11010d101101dddd1010iiiiiiii,
//    regs: imm8,
//    rule: VPUSH,
//    safety: [regs  ==
//            0 ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    single_regs: true,
//    small_imm_base_wb: true,
//    true: true,
//    uses: {Sp},
//    violations: [implied by 'base']}
class VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0);
};

// VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0:
//
//   {D: D(22),
//    Sp: 13,
//    Vd: Vd(15:12),
//    arch: ['VFPv2', 'AdvSIMD'],
//    base: Sp,
//    cond: cond(31:28),
//    d: D:Vd,
//    defs: {Sp},
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    pattern: cccc11010d101101dddd1011iiiiiiii,
//    regs: imm8 / 2,
//    rule: VPUSH,
//    safety: [regs  ==
//            0 ||
//         regs  >
//            16 ||
//         d + regs  >
//            32 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         d + regs  >
//            16 => UNPREDICTABLE,
//      imm8(0)  ==
//            1 => DEPRECATED],
//    single_regs: false,
//    small_imm_base_wb: true,
//    true: true,
//    uses: {Sp},
//    violations: [implied by 'base']}
class VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0);
};

// VQABS_111100111d11ss00dddd01110qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01110qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQABS,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VQABS_111100111d11ss00dddd01110qm0mmmm_case_0
     : public ClassDecoder {
 public:
  VQABS_111100111d11ss00dddd01110qm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQABS_111100111d11ss00dddd01110qm0mmmm_case_0);
};

// VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0000nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQADD,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0);
};

// VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 111100101dssnnnndddd0p11n1m0mmmm,
//    regs: 1,
//    rule: VQDMLAL_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
class VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0);
};

// VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    add: op(8)=0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 111100101dssnnnndddd10p1n0m0mmmm,
//    rule: VQDMLAL_VQDMLSL_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 ||
//         Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0
     : public ClassDecoder {
 public:
  VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0);
};

// VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 111100101dssnnnndddd0p11n1m0mmmm,
//    regs: 1,
//    rule: VQDMLSL_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
class VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0);
};

// VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100dssnnnndddd1011nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQDMULH_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      (size(21:20)=11 ||
//         size(21:20)=00) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0);
};

// VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd1100n1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VQDMULH_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0);
};

// VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    add: op(8)=0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 111100101dssnnnndddd1101n0m0mmmm,
//    rule: VQDMULL_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 ||
//         Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0
     : public ClassDecoder {
 public:
  VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0);
};

// VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 111100101dssnnnndddd1011n1m0mmmm,
//    regs: 1,
//    rule: VQDMULL_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
class VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0);
};

// VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: op(0)=1,
//    fields: [D(22), size(19:18), Vd(15:12), op(7:6), M(5), Vm(3:0)],
//    m: M:Vm,
//    op: op(7:6),
//    pattern: 111100111d11ss10dddd0010ppm0mmmm,
//    rule: VQMOVN,
//    safety: [op(7:6)=00 => DECODER_ERROR,
//      size(19:18)=11 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(19:18),
//    src_unsigned: op(7:6)=11,
//    uses: {}}
class VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0
     : public ClassDecoder {
 public:
  VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0);
};

// VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: op(0)=1,
//    fields: [D(22), size(19:18), Vd(15:12), op(7:6), M(5), Vm(3:0)],
//    m: M:Vm,
//    op: op(7:6),
//    pattern: 111100111d11ss10dddd0010ppm0mmmm,
//    rule: VQMOVUN,
//    safety: [op(7:6)=00 => DECODER_ERROR,
//      size(19:18)=11 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(19:18),
//    src_unsigned: op(7:6)=11,
//    uses: {}}
class VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0
     : public ClassDecoder {
 public:
  VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0);
};

// VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01111qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQNEG,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0
     : public ClassDecoder {
 public:
  VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0);
};

// VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd1101n1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VQRDMULH,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0);
};

// VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100110dssnnnndddd1011nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQRDMULH_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      (size(21:20)=11 ||
//         size(21:20)=00) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0);
};

// VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0101nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQRSHL,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0);
};

// VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQRSHRN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      Vm(0)=1 => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => DECODER_ERROR],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    uses: {}}
class VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0
     : public ClassDecoder {
 public:
  VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0);
};

// VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQRSHRUN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      Vm(0)=1 => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => DECODER_ERROR],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    uses: {}}
class VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0
     : public ClassDecoder {
 public:
  VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0);
};

// VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQRSHRUN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      Vm(0)=1 => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => DECODER_ERROR],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    uses: {}}
class VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0
     : public ClassDecoder {
 public:
  VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0);
};

// VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd011plqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQSHL_VQSHLU_immediate,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => UNDEFINED],
//    shift_amount: imm6 - 8
//         if L:imm6(6:0)=0001xxx
//         else imm6 - 16
//         if L:imm6(6:0)=001xxxx
//         else imm6 - 32
//         if L:imm6(6:0)=01xxxxx
//         else imm6
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    unsigned: U(24)=1,
//    uses: {}}
class VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0);
};

// VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0100nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQSHL_register,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0);
};

// VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQSHRN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      Vm(0)=1 => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => DECODER_ERROR],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    uses: {}}
class VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0
     : public ClassDecoder {
 public:
  VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0);
};

// VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQSHRUN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      Vm(0)=1 => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => DECODER_ERROR],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    uses: {}}
class VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0
     : public ClassDecoder {
 public:
  VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0);
};

// VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0010nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQSUB,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0);
};

// VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 111100111dssnnnndddd0100n0m0mmmm,
//    rule: VRADDHN,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vn(0)=1 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0
     : public ClassDecoder {
 public:
  VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0);
};

// VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    floating_point: F(10)=1,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss11dddd010f0qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRECPE,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0
     : public ClassDecoder {
 public:
  VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0);
};

// VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100d0snnnndddd1111nqm1mmmm,
//    rule: VRECPS,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0);
};

// VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    groupsize: rev_groupsize(op, size),
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd000ppqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    reverse_mask: rev_mask(groupsize, esize),
//    rule: VREV16,
//    safety: [op + size  >=
//            3 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0);
};

// VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    groupsize: rev_groupsize(op, size),
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd000ppqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    reverse_mask: rev_mask(groupsize, esize),
//    rule: VREV32,
//    safety: [op + size  >=
//            3 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0);
};

// VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    groupsize: rev_groupsize(op, size),
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd000ppqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    reverse_mask: rev_mask(groupsize, esize),
//    rule: VREV64,
//    safety: [op + size  >=
//            3 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0);
};

// VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0001nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRHADD,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0);
};

// VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0101nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRSHL,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0);
};

// VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 111100101diiiiiidddd100001m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRSHRN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vm(0)=1 => UNDEFINED],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    uses: {}}
class VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0
     : public ClassDecoder {
 public:
  VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0);
};

// VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd0010lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRSHR,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: 16 - imm6
//         if L:imm6(6:0)=0001xxx
//         else 32 - imm6
//         if L:imm6(6:0)=001xxxx
//         else 64 - imm6,
//    unsigned: U(24)=1,
//    uses: {}}
class VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0);
};

// VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    floating_point: F(10)=1,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss11dddd010f1qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRSQRTE,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0
     : public ClassDecoder {
 public:
  VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0);
};

// VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100d1snnnndddd1111nqm1mmmm,
//    rule: VRSQRTS,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0);
};

// VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd0011lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRSRA,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: 16 - imm6
//         if L:imm6(6:0)=0001xxx
//         else 32 - imm6
//         if L:imm6(6:0)=001xxxx
//         else 64 - imm6,
//    unsigned: U(24)=1,
//    uses: {}}
class VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0);
};

// VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 111100111dssnnnndddd0110n0m0mmmm,
//    rule: VRSUBHN,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vn(0)=1 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0
     : public ClassDecoder {
 public:
  VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0);
};

// VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd101000m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHLL_A1_or_VMOVL,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//    shift_amount: imm6 - 8
//         if imm6(21:16)=001xxx
//         else imm6 - 16
//         if imm6(21:16)=01xxxx
//         else imm6 - 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    uses: {}}
class VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0
     : public ClassDecoder {
 public:
  VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0);
};

// VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd001100m0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHLL_A2,
//    safety: [size(19:18)=11 ||
//         Vd(0)=1 => UNDEFINED],
//    shift_amount: esize,
//    size: size(19:18),
//    uses: {}}
class VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0
     : public ClassDecoder {
 public:
  VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0);
};

// VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 111100101diiiiiidddd0101lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHL_immediate,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: imm6 - 8
//         if L:imm6(6:0)=0001xxx
//         else imm6 - 16
//         if L:imm6(6:0)=001xxxx
//         else imm6 - 32
//         if L:imm6(6:0)=01xxxxx
//         else imm6
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    unsigned: U(24)=1,
//    uses: {}}
class VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0);
};

// VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0100nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHL_register,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0);
};

// VSHRN_111100101diiiiiidddd100000m1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 111100101diiiiiidddd100000m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHRN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vm(0)=1 => UNDEFINED],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    uses: {}}
class VSHRN_111100101diiiiiidddd100000m1mmmm_case_0
     : public ClassDecoder {
 public:
  VSHRN_111100101diiiiiidddd100000m1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSHRN_111100101diiiiiidddd100000m1mmmm_case_0);
};

// VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd0000lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHR,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: 16 - imm6
//         if L:imm6(6:0)=0001xxx
//         else 32 - imm6
//         if L:imm6(6:0)=001xxxx
//         else 64 - imm6,
//    unsigned: U(24)=1,
//    uses: {}}
class VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0);
};

// VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 111100111diiiiiidddd0101lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSLI,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: imm6 - 8
//         if L:imm6(6:0)=0001xxx
//         else imm6 - 16
//         if L:imm6(6:0)=001xxxx
//         else imm6 - 32
//         if L:imm6(6:0)=01xxxxx
//         else imm6
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    unsigned: U(24)=1,
//    uses: {}}
class VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0);
};

// VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    advsimd: false,
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), M(5), Vm(3:0)],
//    m: Vm:D
//         if sz(8)=0
//         else M:Vm,
//    pattern: cccc11101d110001dddd101s11m0mmmm,
//    rule: VSQRT,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {}}
class VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0
     : public ClassDecoder {
 public:
  VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0);
};

// VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd0001lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSRA,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: 16 - imm6
//         if L:imm6(6:0)=0001xxx
//         else 32 - imm6
//         if L:imm6(6:0)=001xxxx
//         else 64 - imm6,
//    unsigned: U(24)=1,
//    uses: {}}
class VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0);
};

// VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 111100111diiiiiidddd0100lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSRI,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: 16 - imm6
//         if L:imm6(6:0)=0001xxx
//         else 32 - imm6
//         if L:imm6(6:0)=001xxxx
//         else 64 - imm6,
//    unsigned: U(24)=1,
//    uses: {}}
class VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0);
};

// VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d00nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    regs: 1
//         if type(11:8)=0111
//         else 2
//         if type(11:8)=1010
//         else 3
//         if type(11:8)=0110
//         else 4
//         if type(11:8)=0010
//         else 0,
//    rule: VST1_multiple_single_elements,
//    safety: [type(11:8)=0111 &&
//         align(1)=1 => UNDEFINED,
//      type(11:8)=1010 &&
//         align(5:4)=11 => UNDEFINED,
//      type(11:8)=0110 &&
//         align(1)=1 => UNDEFINED,
//      not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0
     : public ClassDecoder {
 public:
  VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0);
};

// VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: 1
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(1:0)=00
//         else 4)
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d00nnnnddddss00aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST1_single_element_from_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=00 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=01 &&
//         index_align(1)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(2)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=~00 &&
//         index_align(1:0)=~11 => UNDEFINED,
//      n  ==
//            Pc => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0
     : public ClassDecoder {
 public:
  VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0);
};

// VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    inc: 1
//         if type(11:8)=1000
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d00nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    regs: 1
//         if type in bitset {'1000', '1001'}
//         else 2,
//    rule: VST2_multiple_2_element_structures,
//    safety: [size(7:6)=11 => UNDEFINED,
//      type in bitset {'1000', '1001'} &&
//         align(5:4)=11 => UNDEFINED,
//      not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d2 + regs  >
//            32 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0
     : public ClassDecoder {
 public:
  VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0);
};

// VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: (1
//         if index_align(0)=0
//         else 2)
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 4)
//         if size(11:10)=01
//         else (1
//         if index_align(0)=0
//         else 8)
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d00nnnnddddss01aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST2_single_2_element_structure_from_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1)=~0 => UNDEFINED,
//      n  ==
//            Pc ||
//         d2  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0
     : public ClassDecoder {
 public:
  VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0);
};

// VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(0)=0
//         else 8,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    inc: 1
//         if type(11:8)=0100
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d00nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST3_multiple_3_element_structures,
//    safety: [size(7:6)=11 ||
//         align(1)=1 => UNDEFINED,
//      not type in bitset {'0100', '0101'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d3  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0
     : public ClassDecoder {
 public:
  VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0);
};

// VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: 1,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d00nnnnddddss10aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST3_single_3_element_structure_from_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=00 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=01 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=~00 => UNDEFINED,
//      n  ==
//            Pc ||
//         d3  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0
     : public ClassDecoder {
 public:
  VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0);
};

// VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    d4: d3 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    inc: 1
//         if type(11:8)=0000
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d00nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST4_multiple_4_element_structures,
//    safety: [size(7:6)=11 => UNDEFINED,
//      not type in bitset {'0000', '0001'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d4  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: wback &&
//         not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0
     : public ClassDecoder {
 public:
  VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0);
};

// VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0:
//
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: (1
//         if index_align(0)=0
//         else 4)
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 8)
//         if size(11:10)=01
//         else (1
//         if index_align(1:0)=00
//         else 4 << index_align(1:0))
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    d4: d3 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d00nnnnddddss11aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST4_single_4_element_structure_form_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=11 => UNDEFINED,
//      n  ==
//            Pc ||
//         d4  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: wback &&
//         not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    violations: [implied by 'base'],
//    wback: (m  !=
//            Pc)}
class VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0
     : public ClassDecoder {
 public:
  VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0);
};

// VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0:
//
//   {D: D(22),
//    None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Sp: 13,
//    U: U(23),
//    Vd: Vd(15:12),
//    W: W(21),
//    add: U(23)=1,
//    arch: VFPv2,
//    base: Rn,
//    cond: cond(31:28),
//    d: Vd:D,
//    defs: {Rn
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      D(22),
//      W(21),
//      Rn(19:16),
//      Vd(15:12),
//      imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    n: Rn,
//    pattern: cccc110pudw0nnnndddd1010iiiiiiii,
//    regs: imm8,
//    rule: VSTM,
//    safety: [P(24)=0 &&
//         U(23)=0 &&
//         W(21)=0 => DECODER_ERROR,
//      P(24)=1 &&
//         W(21)=0 => DECODER_ERROR,
//      P  ==
//            U &&
//         W(21)=1 => UNDEFINED,
//      n  ==
//            Pc &&
//         wback => UNPREDICTABLE,
//      P(24)=1 &&
//         U(23)=0 &&
//         W(21)=1 &&
//         Rn  ==
//            Sp => DECODER_ERROR,
//      Rn  ==
//            Pc => FORBIDDEN_OPERANDS,
//      regs  ==
//            0 ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    single_regs: true,
//    small_imm_base_wb: wback,
//    true: true,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0);
};

// VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0:
//
//   {D: D(22),
//    None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Sp: 13,
//    U: U(23),
//    Vd: Vd(15:12),
//    W: W(21),
//    add: U(23)=1,
//    arch: ['VFPv2', 'AdvSIMD'],
//    base: Rn,
//    cond: cond(31:28),
//    d: D:Vd,
//    defs: {Rn
//         if wback
//         else None},
//    false: false,
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      D(22),
//      W(21),
//      Rn(19:16),
//      Vd(15:12),
//      imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    n: Rn,
//    pattern: cccc110pudw0nnnndddd1011iiiiiiii,
//    regs: imm8 / 2,
//    rule: VSTM,
//    safety: [P(24)=0 &&
//         U(23)=0 &&
//         W(21)=0 => DECODER_ERROR,
//      P(24)=1 &&
//         W(21)=0 => DECODER_ERROR,
//      P  ==
//            U &&
//         W(21)=1 => UNDEFINED,
//      n  ==
//            Pc &&
//         wback => UNPREDICTABLE,
//      P(24)=1 &&
//         U(23)=0 &&
//         W(21)=1 &&
//         Rn  ==
//            Sp => DECODER_ERROR,
//      Rn  ==
//            Pc => FORBIDDEN_OPERANDS,
//      regs  ==
//            0 ||
//         regs  >
//            16 ||
//         d + regs  >
//            32 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         d + regs  >
//            16 => UNPREDICTABLE,
//      imm8(0)  ==
//            1 => DEPRECATED],
//    single_regs: false,
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: W(21)=1}
class VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0);
};

// VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0:
//
//   {D: D(22),
//    Pc: 15,
//    Rn: Rn(19:16),
//    U: U(23),
//    Vd: Vd(15:12),
//    add: U(23)=1,
//    arch: VFPv2,
//    base: Rn,
//    cond: cond(31:28),
//    d: Vd:D,
//    defs: {},
//    fields: [cond(31:28), U(23), D(22), Rn(19:16), Vd(15:12), imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    n: Rn,
//    pattern: cccc1101ud00nnnndddd1010iiiiiiii,
//    rule: VSTR,
//    safety: [n  ==
//            Pc => FORBIDDEN_OPERANDS],
//    single_reg: true,
//    true: true,
//    uses: {Rn},
//    violations: [implied by 'base']}
class VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0);
};

// VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0:
//
//   {D: D(22),
//    Pc: 15,
//    Rn: Rn(19:16),
//    U: U(23),
//    Vd: Vd(15:12),
//    add: U(23)=1,
//    arch: ['VFPv2', 'AdvSIMD'],
//    base: Rn,
//    cond: cond(31:28),
//    d: D:Vd,
//    defs: {},
//    false: false,
//    fields: [cond(31:28), U(23), D(22), Rn(19:16), Vd(15:12), imm8(7:0)],
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    n: Rn,
//    pattern: cccc1101ud00nnnndddd1011iiiiiiii,
//    rule: VSTR,
//    safety: [n  ==
//            Pc => FORBIDDEN_OPERANDS],
//    single_reg: false,
//    uses: {Rn},
//    violations: [implied by 'base']}
class VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0
     : public ClassDecoder {
 public:
  VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0);
};

// VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 111100101dssnnnndddd0110n0m0mmmm,
//    rule: VSUBHN,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vn(0)=1 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0
     : public ClassDecoder {
 public:
  VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0);
};

// VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    is_w: op(8)=1,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd001pn0m0mmmm,
//    rule: VSUBL_VSUBW,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vd(0)=1 ||
//         (op(8)=1 &&
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0
     : public ClassDecoder {
 public:
  VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0);
};

// VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100d1snnnndddd1101nqm0mmmm,
//    rule: VSUB_floating_point_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0);
};

// VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    advsimd: false,
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    pattern: cccc11100d11nnnndddd101sn1m0mmmm,
//    rule: VSUB_floating_point,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
class VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0
     : public ClassDecoder {
 public:
  VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0);
};

// VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100110dssnnnndddd1000nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSUB_integer,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0
     : public ClassDecoder {
 public:
  VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0);
};

// VSWP_111100111d11ss10dddd00000qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd00000qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSWP,
//    safety: [d  ==
//            m => UNKNOWN,
//      size(19:18)=~00 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VSWP_111100111d11ss10dddd00000qm0mmmm_case_0
     : public ClassDecoder {
 public:
  VSWP_111100111d11ss10dddd00000qm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VSWP_111100111d11ss10dddd00000qm0mmmm_case_0);
};

// VTBL_VTBX_111100111d11nnnndddd10ccnpm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    d: D:Vd,
//    defs: {},
//    fields: [D(22),
//      Vn(19:16),
//      Vd(15:12),
//      len(9:8),
//      N(7),
//      op(6),
//      M(5),
//      Vm(3:0)],
//    is_vtbl: op(6)=0,
//    len: len(9:8),
//    length: len + 1,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(6),
//    pattern: 111100111d11nnnndddd10ccnpm0mmmm,
//    rule: VTBL_VTBX,
//    safety: [n + length  >
//            32 => UNPREDICTABLE],
//    uses: {}}
class VTBL_VTBX_111100111d11nnnndddd10ccnpm0mmmm_case_0
     : public ClassDecoder {
 public:
  VTBL_VTBX_111100111d11nnnndddd10ccnpm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VTBL_VTBX_111100111d11nnnndddd10ccnpm0mmmm_case_0);
};

// VTRN_111100111d11ss10dddd00001qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd00001qm0mmmm,
//    quadword_operation: Q(6)=1,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VTRN,
//    safety: [d  ==
//            m => UNKNOWN,
//      size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VTRN_111100111d11ss10dddd00001qm0mmmm_case_0
     : public ClassDecoder {
 public:
  VTRN_111100111d11ss10dddd00001qm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VTRN_111100111d11ss10dddd00001qm0mmmm_case_0);
};

// VTST_111100100dssnnnndddd1000nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100dssnnnndddd1000nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VTST,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class VTST_111100100dssnnnndddd1000nqm1mmmm_case_0
     : public ClassDecoder {
 public:
  VTST_111100100dssnnnndddd1000nqm1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VTST_111100100dssnnnndddd1000nqm1mmmm_case_0);
};

// VUZP_111100111d11ss10dddd00010qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd00010qm0mmmm,
//    quadword_operation: Q(6)=1,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VUZP,
//    safety: [d  ==
//            m => UNKNOWN,
//      size(19:18)=11 ||
//         (Q(6)=0 &&
//         size(19:18)=10) => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VUZP_111100111d11ss10dddd00010qm0mmmm_case_0
     : public ClassDecoder {
 public:
  VUZP_111100111d11ss10dddd00010qm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VUZP_111100111d11ss10dddd00010qm0mmmm_case_0);
};

// VZIP_111100111d11ss10dddd00011qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd00011qm0mmmm,
//    quadword_operation: Q(6)=1,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VZIP,
//    safety: [d  ==
//            m => UNKNOWN,
//      size(19:18)=11 ||
//         (Q(6)=0 &&
//         size(19:18)=10) => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class VZIP_111100111d11ss10dddd00011qm0mmmm_case_0
     : public ClassDecoder {
 public:
  VZIP_111100111d11ss10dddd00011qm0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      VZIP_111100111d11ss10dddd00011qm0mmmm_case_0);
};

// WFE_cccc0011001000001111000000000010_case_0:
//
//   {arch: v6K,
//    defs: {},
//    pattern: cccc0011001000001111000000000010,
//    rule: WFE,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class WFE_cccc0011001000001111000000000010_case_0
     : public ClassDecoder {
 public:
  WFE_cccc0011001000001111000000000010_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      WFE_cccc0011001000001111000000000010_case_0);
};

// WFI_cccc0011001000001111000000000011_case_0:
//
//   {arch: v6K,
//    defs: {},
//    pattern: cccc0011001000001111000000000011,
//    rule: WFI,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class WFI_cccc0011001000001111000000000011_case_0
     : public ClassDecoder {
 public:
  WFI_cccc0011001000001111000000000011_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      WFI_cccc0011001000001111000000000011_case_0);
};

// YIELD_cccc0011001000001111000000000001_case_0:
//
//   {arch: v6K,
//    defs: {},
//    pattern: cccc0011001000001111000000000001,
//    rule: YIELD,
//    uses: {}}
class YIELD_cccc0011001000001111000000000001_case_0
     : public ClassDecoder {
 public:
  YIELD_cccc0011001000001111000000000001_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      YIELD_cccc0011001000001111000000000001_case_0);
};

// extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0:
//
//   {defs: {},
//    pattern: cccc0000xx1xxxxxxxxxxxxx1xx1xxxx,
//    rule: extra_load_store_instructions_unpriviledged,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0
     : public ClassDecoder {
 public:
  extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0);
};

} // namespace nacl_arm_test

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_BASELINES_3_H_
