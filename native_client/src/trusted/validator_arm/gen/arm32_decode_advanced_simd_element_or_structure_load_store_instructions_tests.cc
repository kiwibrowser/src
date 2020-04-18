/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif


#include "gtest/gtest.h"
#include "native_client/src/trusted/validator_arm/actual_vs_baseline.h"
#include "native_client/src/trusted/validator_arm/arm_helpers.h"
#include "native_client/src/trusted/validator_arm/gen/arm32_decode_named_bases.h"

using nacl_arm_dec::Instruction;
using nacl_arm_dec::ClassDecoder;
using nacl_arm_dec::Register;
using nacl_arm_dec::RegisterList;

namespace nacl_arm_test {

// The following classes are derived class decoder testers that
// add row pattern constraints and decoder restrictions to each tester.
// This is done so that it can be used to make sure that the
// corresponding pattern is not tested for cases that would be excluded
//  due to row checks, or restrictions specified by the row restrictions.


// L(21)=0 & A(23)=0 & B(11:8)=0010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VST1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~0010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000200) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=0 & B(11:8)=0011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VST2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~0011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000300) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=0 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VST1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~1010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000A00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=0 & B(11:8)=000x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       base: n,
//       baseline: VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST4_multiple_4_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         not type in bitset {'0000', '0001'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~000x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=0 & B(11:8)=010x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0100
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST3_multiple_3_element_structures,
//       safety: [size(7:6)=11 ||
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0100', '0101'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~010x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000400) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=0 & B(11:8)=011x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VST1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~011x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000600) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=0 & B(11:8)=100x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VST2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~100x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000800) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=1 & B(11:8)=1000
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST1_single_element_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000800) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=1 & B(11:8)=1001
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST2_single_2_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000900) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=1 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST3_single_3_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000A00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=1 & B(11:8)=1011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST4_single_4_element_structure_form_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000B00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=1 & B(11:8)=0x00
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST1_single_element_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~0x00
  if ((inst.Bits() & 0x00000B00)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=1 & B(11:8)=0x01
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST2_single_2_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~0x01
  if ((inst.Bits() & 0x00000B00)  !=
          0x00000100) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=1 & B(11:8)=0x10
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST3_single_3_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~0x10
  if ((inst.Bits() & 0x00000B00)  !=
          0x00000200) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=0 & A(23)=1 & B(11:8)=0x11
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST4_single_4_element_structure_form_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~0
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~0x11
  if ((inst.Bits() & 0x00000B00)  !=
          0x00000300) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=0 & B(11:8)=0010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VLD1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~0010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000200) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=0 & B(11:8)=0011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VLD2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase16
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~0011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000300) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=0 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VLD1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase17
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~1010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000A00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=0 & B(11:8)=000x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       base: n,
//       baseline: VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_multiple_4_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         not type in bitset {'0000', '0001'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase18
    : public Arm32DecoderTester {
 public:
  VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase18(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase18
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~000x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=0 & B(11:8)=010x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0100
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_multiple_3_element_structures,
//       safety: [size(7:6)=11 ||
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0100', '0101'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase19
    : public Arm32DecoderTester {
 public:
  VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase19(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase19
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~010x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000400) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=0 & B(11:8)=011x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VLD1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase20
    : public Arm32DecoderTester {
 public:
  VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase20(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase20
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~011x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000600) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=0 & B(11:8)=100x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VLD2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase21
    : public Arm32DecoderTester {
 public:
  VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase21(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase21
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~0
  if ((inst.Bits() & 0x00800000)  !=
          0x00000000) return false;
  // B(11:8)=~100x
  if ((inst.Bits() & 0x00000E00)  !=
          0x00000800) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=1000
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD1_single_element_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0TesterCase22
    : public Arm32DecoderTester {
 public:
  VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0TesterCase22(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0TesterCase22
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1000
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000800) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=1001
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD2_single_2_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0TesterCase23
    : public Arm32DecoderTester {
 public:
  VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0TesterCase23(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0TesterCase23
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1001
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000900) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_single_3_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0TesterCase24
    : public Arm32DecoderTester {
 public:
  VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0TesterCase24(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0TesterCase24
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1010
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000A00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=1011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_single_4_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0TesterCase25
    : public Arm32DecoderTester {
 public:
  VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0TesterCase25(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0TesterCase25
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1011
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000B00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=1100
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       a: a(4),
//       actual: Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1,
//       base: n,
//       baseline: VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(7:6),
//         T(5),
//         a(4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1100sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if T(5)=0
//            else 2,
//       rule: VLD1_single_element_to_all_lanes,
//       safety: [size(7:6)=11 ||
//            (size(7:6)=00 &&
//            a(4)=1) => UNDEFINED,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0TesterCase26
    : public Arm32DecoderTester {
 public:
  VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0TesterCase26(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0TesterCase26
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1100
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000C00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=1101
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1,
//       base: n,
//       baseline: VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22), Rn(19:16), Vd(15:12), size(7:6), T(5), Rm(3:0)],
//       inc: 1
//            if T(5)=0
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1101sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD2_single_2_element_structure_to_all_lanes,
//       safety: [size(7:6)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0TesterCase27
    : public Arm32DecoderTester {
 public:
  VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0TesterCase27(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0TesterCase27
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1101
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000D00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=1110
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       a: a(4),
//       actual: Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1,
//       base: n,
//       baseline: VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(7:6),
//         T(5),
//         a(4),
//         Rm(3:0)],
//       inc: 1
//            if T(5)=0
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1110sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_single_3_element_structure_to_all_lanes,
//       safety: [size(7:6)=11 ||
//            a(4)=1 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0TesterCase28
    : public Arm32DecoderTester {
 public:
  VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0TesterCase28(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0TesterCase28
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1110
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000E00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=1111
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       a: a(4),
//       actual: Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1,
//       base: n,
//       baseline: VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(7:6),
//         T(5),
//         a(4),
//         Rm(3:0)],
//       inc: 1
//            if T(5)=0
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1111sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_single_4_element_structure_to_all_lanes,
//       safety: [size(7:6)=11 &&
//            a(4)=0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0TesterCase29
    : public Arm32DecoderTester {
 public:
  VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0TesterCase29(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0TesterCase29
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~1111
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=0x00
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD1_single_element_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0TesterCase30
    : public Arm32DecoderTester {
 public:
  VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0TesterCase30(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0TesterCase30
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~0x00
  if ((inst.Bits() & 0x00000B00)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=0x01
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD2_single_2_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0TesterCase31
    : public Arm32DecoderTester {
 public:
  VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0TesterCase31(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0TesterCase31
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~0x01
  if ((inst.Bits() & 0x00000B00)  !=
          0x00000100) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=0x10
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_single_3_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0TesterCase32
    : public Arm32DecoderTester {
 public:
  VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0TesterCase32(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0TesterCase32
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~0x10
  if ((inst.Bits() & 0x00000B00)  !=
          0x00000200) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// L(21)=1 & A(23)=1 & B(11:8)=0x11
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_single_4_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0TesterCase33
    : public Arm32DecoderTester {
 public:
  VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0TesterCase33(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0TesterCase33
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // L(21)=~1
  if ((inst.Bits() & 0x00200000)  !=
          0x00200000) return false;
  // A(23)=~1
  if ((inst.Bits() & 0x00800000)  !=
          0x00800000) return false;
  // B(11:8)=~0x11
  if ((inst.Bits() & 0x00000B00)  !=
          0x00000300) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// L(21)=0 & A(23)=0 & B(11:8)=0010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VST1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case0
    : public VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase0 {
 public:
  VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case0()
    : VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase0(
      state_.VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0_VST1_multiple_single_elements_instance_)
  {}
};

// L(21)=0 & A(23)=0 & B(11:8)=0011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VST2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case1
    : public VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase1 {
 public:
  VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case1()
    : VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase1(
      state_.VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0_VST2_multiple_2_element_structures_instance_)
  {}
};

// L(21)=0 & A(23)=0 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VST1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case2
    : public VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase2 {
 public:
  VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case2()
    : VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase2(
      state_.VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0_VST1_multiple_single_elements_instance_)
  {}
};

// L(21)=0 & A(23)=0 & B(11:8)=000x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       base: n,
//       baseline: VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST4_multiple_4_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         not type in bitset {'0000', '0001'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case3
    : public VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase3 {
 public:
  VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case3()
    : VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase3(
      state_.VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0_VST4_multiple_4_element_structures_instance_)
  {}
};

// L(21)=0 & A(23)=0 & B(11:8)=010x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0100
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST3_multiple_3_element_structures,
//       safety: [size(7:6)=11 ||
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0100', '0101'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case4
    : public VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase4 {
 public:
  VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case4()
    : VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase4(
      state_.VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0_VST3_multiple_3_element_structures_instance_)
  {}
};

// L(21)=0 & A(23)=0 & B(11:8)=011x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VST1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case5
    : public VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase5 {
 public:
  VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case5()
    : VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0TesterCase5(
      state_.VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0_VST1_multiple_single_elements_instance_)
  {}
};

// L(21)=0 & A(23)=0 & B(11:8)=100x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VST2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case6
    : public VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase6 {
 public:
  VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case6()
    : VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0TesterCase6(
      state_.VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0_VST2_multiple_2_element_structures_instance_)
  {}
};

// L(21)=0 & A(23)=1 & B(11:8)=1000
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST1_single_element_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0Tester_Case7
    : public VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0TesterCase7 {
 public:
  VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0Tester_Case7()
    : VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0TesterCase7(
      state_.VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0_VST1_single_element_from_one_lane_instance_)
  {}
};

// L(21)=0 & A(23)=1 & B(11:8)=1001
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST2_single_2_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0Tester_Case8
    : public VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0TesterCase8 {
 public:
  VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0Tester_Case8()
    : VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0TesterCase8(
      state_.VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0_VST2_single_2_element_structure_from_one_lane_instance_)
  {}
};

// L(21)=0 & A(23)=1 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST3_single_3_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0Tester_Case9
    : public VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0TesterCase9 {
 public:
  VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0Tester_Case9()
    : VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0TesterCase9(
      state_.VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0_VST3_single_3_element_structure_from_one_lane_instance_)
  {}
};

// L(21)=0 & A(23)=1 & B(11:8)=1011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST4_single_4_element_structure_form_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0Tester_Case10
    : public VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0TesterCase10 {
 public:
  VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0Tester_Case10()
    : VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0TesterCase10(
      state_.VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0_VST4_single_4_element_structure_form_one_lane_instance_)
  {}
};

// L(21)=0 & A(23)=1 & B(11:8)=0x00
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST1_single_element_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0Tester_Case11
    : public VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0TesterCase11 {
 public:
  VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0Tester_Case11()
    : VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0TesterCase11(
      state_.VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0_VST1_single_element_from_one_lane_instance_)
  {}
};

// L(21)=0 & A(23)=1 & B(11:8)=0x01
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST2_single_2_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0Tester_Case12
    : public VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0TesterCase12 {
 public:
  VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0Tester_Case12()
    : VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0TesterCase12(
      state_.VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0_VST2_single_2_element_structure_from_one_lane_instance_)
  {}
};

// L(21)=0 & A(23)=1 & B(11:8)=0x10
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST3_single_3_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0Tester_Case13
    : public VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0TesterCase13 {
 public:
  VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0Tester_Case13()
    : VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0TesterCase13(
      state_.VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0_VST3_single_3_element_structure_from_one_lane_instance_)
  {}
};

// L(21)=0 & A(23)=1 & B(11:8)=0x11
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST4_single_4_element_structure_form_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0Tester_Case14
    : public VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0TesterCase14 {
 public:
  VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0Tester_Case14()
    : VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0TesterCase14(
      state_.VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0_VST4_single_4_element_structure_form_one_lane_instance_)
  {}
};

// L(21)=1 & A(23)=0 & B(11:8)=0010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VLD1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case15
    : public VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase15 {
 public:
  VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case15()
    : VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase15(
      state_.VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0_VLD1_multiple_single_elements_instance_)
  {}
};

// L(21)=1 & A(23)=0 & B(11:8)=0011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VLD2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case16
    : public VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase16 {
 public:
  VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case16()
    : VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase16(
      state_.VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0_VLD2_multiple_2_element_structures_instance_)
  {}
};

// L(21)=1 & A(23)=0 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VLD1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case17
    : public VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase17 {
 public:
  VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case17()
    : VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase17(
      state_.VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0_VLD1_multiple_single_elements_instance_)
  {}
};

// L(21)=1 & A(23)=0 & B(11:8)=000x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       base: n,
//       baseline: VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_multiple_4_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         not type in bitset {'0000', '0001'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case18
    : public VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase18 {
 public:
  VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case18()
    : VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase18(
      state_.VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0_VLD4_multiple_4_element_structures_instance_)
  {}
};

// L(21)=1 & A(23)=0 & B(11:8)=010x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0100
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_multiple_3_element_structures,
//       safety: [size(7:6)=11 ||
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0100', '0101'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case19
    : public VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase19 {
 public:
  VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case19()
    : VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase19(
      state_.VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0_VLD3_multiple_3_element_structures_instance_)
  {}
};

// L(21)=1 & A(23)=0 & B(11:8)=011x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VLD1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case20
    : public VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase20 {
 public:
  VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case20()
    : VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0TesterCase20(
      state_.VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0_VLD1_multiple_single_elements_instance_)
  {}
};

// L(21)=1 & A(23)=0 & B(11:8)=100x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VLD2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case21
    : public VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase21 {
 public:
  VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case21()
    : VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0TesterCase21(
      state_.VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0_VLD2_multiple_2_element_structures_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=1000
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD1_single_element_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0Tester_Case22
    : public VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0TesterCase22 {
 public:
  VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0Tester_Case22()
    : VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0TesterCase22(
      state_.VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0_VLD1_single_element_to_one_lane_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=1001
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD2_single_2_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0Tester_Case23
    : public VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0TesterCase23 {
 public:
  VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0Tester_Case23()
    : VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0TesterCase23(
      state_.VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0_VLD2_single_2_element_structure_to_one_lane_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_single_3_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0Tester_Case24
    : public VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0TesterCase24 {
 public:
  VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0Tester_Case24()
    : VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0TesterCase24(
      state_.VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0_VLD3_single_3_element_structure_to_one_lane_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=1011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_single_4_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0Tester_Case25
    : public VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0TesterCase25 {
 public:
  VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0Tester_Case25()
    : VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0TesterCase25(
      state_.VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0_VLD4_single_4_element_structure_to_one_lane_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=1100
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       a: a(4),
//       actual: Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1,
//       base: n,
//       baseline: VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(7:6),
//         T(5),
//         a(4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1100sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if T(5)=0
//            else 2,
//       rule: VLD1_single_element_to_all_lanes,
//       safety: [size(7:6)=11 ||
//            (size(7:6)=00 &&
//            a(4)=1) => UNDEFINED,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0Tester_Case26
    : public VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0TesterCase26 {
 public:
  VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0Tester_Case26()
    : VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0TesterCase26(
      state_.VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0_VLD1_single_element_to_all_lanes_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=1101
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1,
//       base: n,
//       baseline: VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22), Rn(19:16), Vd(15:12), size(7:6), T(5), Rm(3:0)],
//       inc: 1
//            if T(5)=0
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1101sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD2_single_2_element_structure_to_all_lanes,
//       safety: [size(7:6)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0Tester_Case27
    : public VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0TesterCase27 {
 public:
  VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0Tester_Case27()
    : VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0TesterCase27(
      state_.VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0_VLD2_single_2_element_structure_to_all_lanes_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=1110
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       a: a(4),
//       actual: Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1,
//       base: n,
//       baseline: VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(7:6),
//         T(5),
//         a(4),
//         Rm(3:0)],
//       inc: 1
//            if T(5)=0
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1110sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_single_3_element_structure_to_all_lanes,
//       safety: [size(7:6)=11 ||
//            a(4)=1 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0Tester_Case28
    : public VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0TesterCase28 {
 public:
  VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0Tester_Case28()
    : VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0TesterCase28(
      state_.VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0_VLD3_single_3_element_structure_to_all_lanes_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=1111
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       a: a(4),
//       actual: Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1,
//       base: n,
//       baseline: VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(7:6),
//         T(5),
//         a(4),
//         Rm(3:0)],
//       inc: 1
//            if T(5)=0
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1111sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_single_4_element_structure_to_all_lanes,
//       safety: [size(7:6)=11 &&
//            a(4)=0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0Tester_Case29
    : public VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0TesterCase29 {
 public:
  VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0Tester_Case29()
    : VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0TesterCase29(
      state_.VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0_VLD4_single_4_element_structure_to_all_lanes_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=0x00
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD1_single_element_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0Tester_Case30
    : public VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0TesterCase30 {
 public:
  VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0Tester_Case30()
    : VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0TesterCase30(
      state_.VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0_VLD1_single_element_to_one_lane_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=0x01
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD2_single_2_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0Tester_Case31
    : public VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0TesterCase31 {
 public:
  VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0Tester_Case31()
    : VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0TesterCase31(
      state_.VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0_VLD2_single_2_element_structure_to_one_lane_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=0x10
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_single_3_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0Tester_Case32
    : public VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0TesterCase32 {
 public:
  VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0Tester_Case32()
    : VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0TesterCase32(
      state_.VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0_VLD3_single_3_element_structure_to_one_lane_instance_)
  {}
};

// L(21)=1 & A(23)=1 & B(11:8)=0x11
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_single_4_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
class VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0Tester_Case33
    : public VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0TesterCase33 {
 public:
  VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0Tester_Case33()
    : VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0TesterCase33(
      state_.VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0_VLD4_single_4_element_structure_to_one_lane_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// L(21)=0 & A(23)=0 & B(11:8)=0010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VST1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case0_TestCase0) {
  VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case0 baseline_tester;
  NamedActual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1_VST1_multiple_single_elements actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d00nnnnddddttttssaammmm");
}

// L(21)=0 & A(23)=0 & B(11:8)=0011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VST2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case1_TestCase1) {
  VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case1 baseline_tester;
  NamedActual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1_VST2_multiple_2_element_structures actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d00nnnnddddttttssaammmm");
}

// L(21)=0 & A(23)=0 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VST1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case2_TestCase2) {
  VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case2 baseline_tester;
  NamedActual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1_VST1_multiple_single_elements actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d00nnnnddddttttssaammmm");
}

// L(21)=0 & A(23)=0 & B(11:8)=000x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       base: n,
//       baseline: VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST4_multiple_4_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         not type in bitset {'0000', '0001'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case3_TestCase3) {
  VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case3 baseline_tester;
  NamedActual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1_VST4_multiple_4_element_structures actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d00nnnnddddttttssaammmm");
}

// L(21)=0 & A(23)=0 & B(11:8)=010x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0100
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST3_multiple_3_element_structures,
//       safety: [size(7:6)=11 ||
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0100', '0101'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case4_TestCase4) {
  VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case4 baseline_tester;
  NamedActual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1_VST3_multiple_3_element_structures actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d00nnnnddddttttssaammmm");
}

// L(21)=0 & A(23)=0 & B(11:8)=011x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VST1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case5_TestCase5) {
  VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0Tester_Case5 baseline_tester;
  NamedActual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1_VST1_multiple_single_elements actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d00nnnnddddttttssaammmm");
}

// L(21)=0 & A(23)=0 & B(11:8)=100x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d00nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VST2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case6_TestCase6) {
  VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0Tester_Case6 baseline_tester;
  NamedActual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1_VST2_multiple_2_element_structures actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d00nnnnddddttttssaammmm");
}

// L(21)=0 & A(23)=1 & B(11:8)=1000
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST1_single_element_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0Tester_Case7_TestCase7) {
  VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0Tester_Case7 baseline_tester;
  NamedActual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1_VST1_single_element_from_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d00nnnnddddss00aaaammmm");
}

// L(21)=0 & A(23)=1 & B(11:8)=1001
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST2_single_2_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0Tester_Case8_TestCase8) {
  VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0Tester_Case8 baseline_tester;
  NamedActual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1_VST2_single_2_element_structure_from_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d00nnnnddddss01aaaammmm");
}

// L(21)=0 & A(23)=1 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST3_single_3_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0Tester_Case9_TestCase9) {
  VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0Tester_Case9 baseline_tester;
  NamedActual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1_VST3_single_3_element_structure_from_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d00nnnnddddss10aaaammmm");
}

// L(21)=0 & A(23)=1 & B(11:8)=1011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST4_single_4_element_structure_form_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0Tester_Case10_TestCase10) {
  VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0Tester_Case10 baseline_tester;
  NamedActual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1_VST4_single_4_element_structure_form_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d00nnnnddddss11aaaammmm");
}

// L(21)=0 & A(23)=1 & B(11:8)=0x00
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST1_single_element_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0Tester_Case11_TestCase11) {
  VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0Tester_Case11 baseline_tester;
  NamedActual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1_VST1_single_element_from_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d00nnnnddddss00aaaammmm");
}

// L(21)=0 & A(23)=1 & B(11:8)=0x01
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST2_single_2_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0Tester_Case12_TestCase12) {
  VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0Tester_Case12 baseline_tester;
  NamedActual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1_VST2_single_2_element_structure_from_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d00nnnnddddss01aaaammmm");
}

// L(21)=0 & A(23)=1 & B(11:8)=0x10
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST3_single_3_element_structure_from_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0Tester_Case13_TestCase13) {
  VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0Tester_Case13 baseline_tester;
  NamedActual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1_VST3_single_3_element_structure_from_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d00nnnnddddss10aaaammmm");
}

// L(21)=0 & A(23)=1 & B(11:8)=0x11
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d00nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VST4_single_4_element_structure_form_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0Tester_Case14_TestCase14) {
  VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0Tester_Case14 baseline_tester;
  NamedActual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1_VST4_single_4_element_structure_form_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d00nnnnddddss11aaaammmm");
}

// L(21)=1 & A(23)=0 & B(11:8)=0010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VLD1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case15_TestCase15) {
  VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case15 baseline_tester;
  NamedActual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1_VLD1_multiple_single_elements actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d10nnnnddddttttssaammmm");
}

// L(21)=1 & A(23)=0 & B(11:8)=0011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VLD2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case16_TestCase16) {
  VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case16 baseline_tester;
  NamedActual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1_VLD2_multiple_2_element_structures actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d10nnnnddddttttssaammmm");
}

// L(21)=1 & A(23)=0 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VLD1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case17_TestCase17) {
  VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case17 baseline_tester;
  NamedActual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1_VLD1_multiple_single_elements actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d10nnnnddddttttssaammmm");
}

// L(21)=1 & A(23)=0 & B(11:8)=000x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       base: n,
//       baseline: VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_multiple_4_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         not type in bitset {'0000', '0001'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case18_TestCase18) {
  VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case18 baseline_tester;
  NamedActual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1_VLD4_multiple_4_element_structures actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d10nnnnddddttttssaammmm");
}

// L(21)=1 & A(23)=0 & B(11:8)=010x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=0100
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_multiple_3_element_structures,
//       safety: [size(7:6)=11 ||
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0100', '0101'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case19_TestCase19) {
  VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case19 baseline_tester;
  NamedActual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1_VLD3_multiple_3_element_structures actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d10nnnnddddttttssaammmm");
}

// L(21)=1 & A(23)=0 & B(11:8)=011x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         align(5:4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type(11:8)=0111
//            else 2
//            if type(11:8)=1010
//            else 3
//            if type(11:8)=0110
//            else 4
//            if type(11:8)=0010
//            else 0,
//       rule: VLD1_multiple_single_elements,
//       safety: [type(11:8)=0111 &&
//            align(1)=1 => UNDEFINED,
//         type(11:8)=1010 &&
//            align(5:4)=11 => UNDEFINED,
//         type(11:8)=0110 &&
//            align(1)=1 => UNDEFINED,
//         not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case20_TestCase20) {
  VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0Tester_Case20 baseline_tester;
  NamedActual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1_VLD1_multiple_single_elements actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d10nnnnddddttttssaammmm");
}

// L(21)=1 & A(23)=0 & B(11:8)=100x
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1,
//       align: align(5:4),
//       base: n,
//       baseline: VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         type(11:8),
//         size(7:6),
//         align(5:4),
//         Rm(3:0)],
//       inc: 1
//            if type(11:8)=1000
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101000d10nnnnddddttttssaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if type in bitset {'1000', '1001'}
//            else 2,
//       rule: VLD2_multiple_2_element_structures,
//       safety: [size(7:6)=11 => UNDEFINED,
//         type in bitset {'1000', '1001'} &&
//            align(5:4)=11 => UNDEFINED,
//         not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//         n  ==
//               Pc ||
//            d2 + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       type: type(11:8),
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case21_TestCase21) {
  VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0Tester_Case21 baseline_tester;
  NamedActual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1_VLD2_multiple_2_element_structures actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101000d10nnnnddddttttssaammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=1000
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD1_single_element_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0Tester_Case22_TestCase22) {
  VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0Tester_Case22 baseline_tester;
  NamedActual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1_VLD1_single_element_to_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnnddddss00aaaammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=1001
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD2_single_2_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0Tester_Case23_TestCase23) {
  VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0Tester_Case23 baseline_tester;
  NamedActual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1_VLD2_single_2_element_structure_to_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnnddddss01aaaammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=1010
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_single_3_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0Tester_Case24_TestCase24) {
  VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0Tester_Case24 baseline_tester;
  NamedActual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1_VLD3_single_3_element_structure_to_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnnddddss10aaaammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=1011
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_single_4_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0Tester_Case25_TestCase25) {
  VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0Tester_Case25 baseline_tester;
  NamedActual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1_VLD4_single_4_element_structure_to_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnnddddss11aaaammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=1100
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       a: a(4),
//       actual: Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1,
//       base: n,
//       baseline: VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0,
//       d: D:Vd,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(7:6),
//         T(5),
//         a(4),
//         Rm(3:0)],
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1100sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       regs: 1
//            if T(5)=0
//            else 2,
//       rule: VLD1_single_element_to_all_lanes,
//       safety: [size(7:6)=11 ||
//            (size(7:6)=00 &&
//            a(4)=1) => UNDEFINED,
//         n  ==
//               Pc ||
//            d + regs  >
//               32 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0Tester_Case26_TestCase26) {
  VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0Tester_Case26 baseline_tester;
  NamedActual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1_VLD1_single_element_to_all_lanes actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnndddd1100sstammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=1101
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1,
//       base: n,
//       baseline: VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22), Rn(19:16), Vd(15:12), size(7:6), T(5), Rm(3:0)],
//       inc: 1
//            if T(5)=0
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1101sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD2_single_2_element_structure_to_all_lanes,
//       safety: [size(7:6)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0Tester_Case27_TestCase27) {
  VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0Tester_Case27 baseline_tester;
  NamedActual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1_VLD2_single_2_element_structure_to_all_lanes actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnndddd1101sstammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=1110
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       a: a(4),
//       actual: Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1,
//       base: n,
//       baseline: VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(7:6),
//         T(5),
//         a(4),
//         Rm(3:0)],
//       inc: 1
//            if T(5)=0
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1110sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_single_3_element_structure_to_all_lanes,
//       safety: [size(7:6)=11 ||
//            a(4)=1 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0Tester_Case28_TestCase28) {
  VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0Tester_Case28 baseline_tester;
  NamedActual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1_VLD3_single_3_element_structure_to_all_lanes actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnndddd1110sstammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=1111
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       T: T(5),
//       Vd: Vd(15:12),
//       a: a(4),
//       actual: Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1,
//       base: n,
//       baseline: VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(7:6),
//         T(5),
//         a(4),
//         Rm(3:0)],
//       inc: 1
//            if T(5)=0
//            else 2,
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnndddd1111sstammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_single_4_element_structure_to_all_lanes,
//       safety: [size(7:6)=11 &&
//            a(4)=0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(7:6),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0Tester_Case29_TestCase29) {
  VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0Tester_Case29 baseline_tester;
  NamedActual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1_VLD4_single_4_element_structure_to_all_lanes actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnndddd1111sstammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=0x00
//    = {None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       actual: Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1,
//       base: n,
//       baseline: VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0,
//       defs: {base}
//            if wback
//            else {},
//       fields: [Rn(19:16), size(11:10), index_align(7:4), Rm(3:0)],
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss00aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD1_single_element_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(1)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(2)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 &&
//            index_align(1:0)=~11 => UNDEFINED,
//         n  ==
//               Pc => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0Tester_Case30_TestCase30) {
  VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0Tester_Case30 baseline_tester;
  NamedActual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1_VLD1_single_element_to_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnnddddss00aaaammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=0x01
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1,
//       base: n,
//       baseline: VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss01aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD2_single_2_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1)=~0 => UNDEFINED,
//         n  ==
//               Pc ||
//            d2  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0Tester_Case31_TestCase31) {
  VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0Tester_Case31 baseline_tester;
  NamedActual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1_VLD2_single_2_element_structure_to_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnnddddss01aaaammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=0x10
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1,
//       base: n,
//       baseline: VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss10aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD3_single_3_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=00 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=01 &&
//            index_align(0)=~0 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=~00 => UNDEFINED,
//         n  ==
//               Pc ||
//            d3  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0Tester_Case32_TestCase32) {
  VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0Tester_Case32 baseline_tester;
  NamedActual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1_VLD3_single_3_element_structure_to_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnnddddss10aaaammmm");
}

// L(21)=1 & A(23)=1 & B(11:8)=0x11
//    = {D: D(22),
//       None: 32,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Sp: 13,
//       Vd: Vd(15:12),
//       actual: Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1,
//       base: n,
//       baseline: VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0,
//       d: D:Vd,
//       d2: d + inc,
//       d3: d2 + inc,
//       d4: d3 + inc,
//       defs: {base}
//            if wback
//            else {},
//       fields: [D(22),
//         Rn(19:16),
//         Vd(15:12),
//         size(11:10),
//         index_align(7:4),
//         Rm(3:0)],
//       inc: 1
//            if size(11:10)=00
//            else (1
//            if index_align(1)=0
//            else 2)
//            if size(11:10)=01
//            else (1
//            if index_align(2)=0
//            else 2)
//            if size(11:10)=10
//            else 0,
//       index_align: index_align(7:4),
//       m: Rm,
//       n: Rn,
//       pattern: 111101001d10nnnnddddss11aaaammmm,
//       register_index: (m  !=
//               Pc &&
//            m  !=
//               Sp),
//       rule: VLD4_single_4_element_structure_to_one_lane,
//       safety: [size(11:10)=11 => UNDEFINED,
//         size(11:10)=10 &&
//            index_align(1:0)=11 => UNDEFINED,
//         n  ==
//               Pc ||
//            d4  >
//               31 => UNPREDICTABLE],
//       size: size(11:10),
//       small_imm_base_wb: wback &&
//            not register_index,
//       uses: {m
//            if wback
//            else None, n},
//       violations: [implied by 'base'],
//       wback: (m  !=
//               Pc)}
TEST_F(Arm32DecoderStateTests,
       VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0Tester_Case33_TestCase33) {
  VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0Tester_Case33 baseline_tester;
  NamedActual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1_VLD4_single_4_element_structure_to_one_lane actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101001d10nnnnddddss11aaaammmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
