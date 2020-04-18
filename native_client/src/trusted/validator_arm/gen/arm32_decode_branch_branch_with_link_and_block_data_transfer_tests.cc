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


// op(25:20)=0000x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100000w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STMDA_STMED,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~0000x0
  if ((inst.Bits() & 0x03D00000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=0000x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100000w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDMDA_LDMFA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~0000x1
  if ((inst.Bits() & 0x03D00000)  !=
          0x00100000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=0010x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100010w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STM_STMIA_STMEA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~0010x0
  if ((inst.Bits() & 0x03D00000)  !=
          0x00800000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=0010x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100010w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDM_LDMIA_LDMFD,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~0010x1
  if ((inst.Bits() & 0x03D00000)  !=
          0x00900000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=0100x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100100w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STMDB_STMFD,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~0100x0
  if ((inst.Bits() & 0x03D00000)  !=
          0x01000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=0100x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100100w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDMDB_LDMEA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~0100x1
  if ((inst.Bits() & 0x03D00000)  !=
          0x01100000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=0110x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100110w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STMIB_STMFA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~0110x0
  if ((inst.Bits() & 0x03D00000)  !=
          0x01800000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=0110x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100110w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDMIB_LDMED,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~0110x1
  if ((inst.Bits() & 0x03D00000)  !=
          0x01900000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=0xx1x0 & $pattern(31:0)=xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {},
//       pattern: cccc100pu100nnnnrrrrrrrrrrrrrrrr,
//       rule: STM_User_registers,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~0xx1x0
  if ((inst.Bits() & 0x02500000)  !=
          0x00400000) return false;
  // $pattern(31:0)=~xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=0xx1x1 & R(15)=0 & $pattern(31:0)=xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0,
//       defs: {},
//       pattern: cccc100pu101nnnn0rrrrrrrrrrrrrrr,
//       rule: LDM_User_registers,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~0xx1x1
  if ((inst.Bits() & 0x02500000)  !=
          0x00500000) return false;
  // R(15)=~0
  if ((inst.Bits() & 0x00008000)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x00200000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=0xx1x1 & R(15)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0,
//       defs: {},
//       pattern: cccc100pu1w1nnnn1rrrrrrrrrrrrrrr,
//       rule: LDM_exception_return,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~0xx1x1
  if ((inst.Bits() & 0x02500000)  !=
          0x00500000) return false;
  // R(15)=~1
  if ((inst.Bits() & 0x00008000)  !=
          0x00008000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=10xxxx
//    = {Pc: 15,
//       actual: Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {Pc},
//       fields: [imm24(23:0)],
//       imm24: imm24(23:0),
//       imm32: SignExtend(imm24:'00'(1:0), 32),
//       pattern: cccc1010iiiiiiiiiiiiiiiiiiiiiiii,
//       relative: true,
//       relative_offset: imm32 + 8,
//       rule: B,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'relative']}
class B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~10xxxx
  if ((inst.Bits() & 0x03000000)  !=
          0x02000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(25:20)=11xxxx
//    = {Lr: 14,
//       Pc: 15,
//       actual: Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {Pc, Lr},
//       fields: [imm24(23:0)],
//       imm24: imm24(23:0),
//       imm32: SignExtend(imm24:'00'(1:0), 32),
//       pattern: cccc1011iiiiiiiiiiiiiiiiiiiiiiii,
//       relative: true,
//       relative_offset: imm32 + 8,
//       rule: BL_BLX_immediate,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'relative']}
class BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(25:20)=~11xxxx
  if ((inst.Bits() & 0x03000000)  !=
          0x03000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// op(25:20)=0000x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100000w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STMDA_STMED,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case0
    : public STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase0 {
 public:
  STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case0()
    : STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase0(
      state_.STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0_STMDA_STMED_instance_)
  {}
};

// op(25:20)=0000x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100000w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDMDA_LDMFA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case1
    : public LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase1 {
 public:
  LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case1()
    : LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase1(
      state_.LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0_LDMDA_LDMFA_instance_)
  {}
};

// op(25:20)=0010x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100010w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STM_STMIA_STMEA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case2
    : public STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase2 {
 public:
  STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case2()
    : STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase2(
      state_.STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0_STM_STMIA_STMEA_instance_)
  {}
};

// op(25:20)=0010x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100010w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDM_LDMIA_LDMFD,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case3
    : public LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase3 {
 public:
  LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case3()
    : LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase3(
      state_.LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0_LDM_LDMIA_LDMFD_instance_)
  {}
};

// op(25:20)=0100x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100100w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STMDB_STMFD,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case4
    : public STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase4 {
 public:
  STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case4()
    : STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase4(
      state_.STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0_STMDB_STMFD_instance_)
  {}
};

// op(25:20)=0100x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100100w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDMDB_LDMEA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case5
    : public LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase5 {
 public:
  LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case5()
    : LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase5(
      state_.LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0_LDMDB_LDMEA_instance_)
  {}
};

// op(25:20)=0110x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100110w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STMIB_STMFA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case6
    : public STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase6 {
 public:
  STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case6()
    : STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0TesterCase6(
      state_.STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0_STMIB_STMFA_instance_)
  {}
};

// op(25:20)=0110x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100110w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDMIB_LDMED,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
class LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case7
    : public LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase7 {
 public:
  LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case7()
    : LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0TesterCase7(
      state_.LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0_LDMIB_LDMED_instance_)
  {}
};

// op(25:20)=0xx1x0 & $pattern(31:0)=xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {},
//       pattern: cccc100pu100nnnnrrrrrrrrrrrrrrrr,
//       rule: STM_User_registers,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case8
    : public STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0TesterCase8 {
 public:
  STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case8()
    : STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0TesterCase8(
      state_.STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0_STM_User_registers_instance_)
  {}
};

// op(25:20)=0xx1x1 & R(15)=0 & $pattern(31:0)=xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0,
//       defs: {},
//       pattern: cccc100pu101nnnn0rrrrrrrrrrrrrrr,
//       rule: LDM_User_registers,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0Tester_Case9
    : public LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0TesterCase9 {
 public:
  LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0Tester_Case9()
    : LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0TesterCase9(
      state_.LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0_LDM_User_registers_instance_)
  {}
};

// op(25:20)=0xx1x1 & R(15)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0,
//       defs: {},
//       pattern: cccc100pu1w1nnnn1rrrrrrrrrrrrrrr,
//       rule: LDM_exception_return,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0Tester_Case10
    : public LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0TesterCase10 {
 public:
  LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0Tester_Case10()
    : LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0TesterCase10(
      state_.LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0_LDM_exception_return_instance_)
  {}
};

// op(25:20)=10xxxx
//    = {Pc: 15,
//       actual: Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {Pc},
//       fields: [imm24(23:0)],
//       imm24: imm24(23:0),
//       imm32: SignExtend(imm24:'00'(1:0), 32),
//       pattern: cccc1010iiiiiiiiiiiiiiiiiiiiiiii,
//       relative: true,
//       relative_offset: imm32 + 8,
//       rule: B,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'relative']}
class B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case11
    : public B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase11 {
 public:
  B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case11()
    : B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase11(
      state_.B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0_B_instance_)
  {}
};

// op(25:20)=11xxxx
//    = {Lr: 14,
//       Pc: 15,
//       actual: Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {Pc, Lr},
//       fields: [imm24(23:0)],
//       imm24: imm24(23:0),
//       imm32: SignExtend(imm24:'00'(1:0), 32),
//       pattern: cccc1011iiiiiiiiiiiiiiiiiiiiiiii,
//       relative: true,
//       relative_offset: imm32 + 8,
//       rule: BL_BLX_immediate,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'relative']}
class BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case12
    : public BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase12 {
 public:
  BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case12()
    : BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0TesterCase12(
      state_.BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0_BL_BLX_immediate_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op(25:20)=0000x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100000w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STMDA_STMED,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case0_TestCase0) {
  STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case0 baseline_tester;
  NamedActual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1_STMDA_STMED actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc100000w0nnnnrrrrrrrrrrrrrrrr");
}

// op(25:20)=0000x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100000w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDMDA_LDMFA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case1_TestCase1) {
  LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case1 baseline_tester;
  NamedActual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1_LDMDA_LDMFA actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc100000w1nnnnrrrrrrrrrrrrrrrr");
}

// op(25:20)=0010x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100010w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STM_STMIA_STMEA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case2_TestCase2) {
  STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case2 baseline_tester;
  NamedActual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1_STM_STMIA_STMEA actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc100010w0nnnnrrrrrrrrrrrrrrrr");
}

// op(25:20)=0010x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100010w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDM_LDMIA_LDMFD,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case3_TestCase3) {
  LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case3 baseline_tester;
  NamedActual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1_LDM_LDMIA_LDMFD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc100010w1nnnnrrrrrrrrrrrrrrrr");
}

// op(25:20)=0100x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100100w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STMDB_STMFD,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case4_TestCase4) {
  STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case4 baseline_tester;
  NamedActual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1_STMDB_STMFD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc100100w0nnnnrrrrrrrrrrrrrrrr");
}

// op(25:20)=0100x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100100w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDMDB_LDMEA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case5_TestCase5) {
  LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case5 baseline_tester;
  NamedActual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1_LDMDB_LDMEA actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc100100w1nnnnrrrrrrrrrrrrrrrr");
}

// op(25:20)=0110x0
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {Rn
//            if wback
//            else None},
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100110w0nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: STMIB_STMFA,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) &&
//            Rn  !=
//               SmallestGPR(registers) => UNKNOWN],
//       small_imm_base_wb: wback,
//       uses: Union({Rn}, registers),
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case6_TestCase6) {
  STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case6 baseline_tester;
  NamedActual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1_STMIB_STMFA actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc100110w0nnnnrrrrrrrrrrrrrrrr");
}

// op(25:20)=0110x1
//    = {None: 32,
//       Pc: 15,
//       Rn: Rn(19:16),
//       W: W(21),
//       actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//       base: Rn,
//       baseline: LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: Union({Rn
//            if wback
//            else None}, registers),
//       fields: [W(21), Rn(19:16), register_list(15:0)],
//       pattern: cccc100110w1nnnnrrrrrrrrrrrrrrrr,
//       register_list: register_list(15:0),
//       registers: RegisterList(register_list),
//       rule: LDMIB_LDMED,
//       safety: [Rn  ==
//               Pc ||
//            NumGPRs(registers)  <
//               1 => UNPREDICTABLE,
//         wback &&
//            Contains(registers, Rn) => UNKNOWN,
//         Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//       small_imm_base_wb: wback,
//       uses: {Rn},
//       violations: [implied by 'base'],
//       wback: W(21)=1}
TEST_F(Arm32DecoderStateTests,
       LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case7_TestCase7) {
  LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case7 baseline_tester;
  NamedActual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1_LDMIB_LDMED actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc100110w1nnnnrrrrrrrrrrrrrrrr");
}

// op(25:20)=0xx1x0 & $pattern(31:0)=xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0,
//       defs: {},
//       pattern: cccc100pu100nnnnrrrrrrrrrrrrrrrr,
//       rule: STM_User_registers,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case8_TestCase8) {
  STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0Tester_Case8 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_STM_User_registers actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc100pu100nnnnrrrrrrrrrrrrrrrr");
}

// op(25:20)=0xx1x1 & R(15)=0 & $pattern(31:0)=xxxxxxxxxx0xxxxxxxxxxxxxxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0,
//       defs: {},
//       pattern: cccc100pu101nnnn0rrrrrrrrrrrrrrr,
//       rule: LDM_User_registers,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0Tester_Case9_TestCase9) {
  LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0Tester_Case9 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_LDM_User_registers actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc100pu101nnnn0rrrrrrrrrrrrrrr");
}

// op(25:20)=0xx1x1 & R(15)=1
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0,
//       defs: {},
//       pattern: cccc100pu1w1nnnn1rrrrrrrrrrrrrrr,
//       rule: LDM_exception_return,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0Tester_Case10_TestCase10) {
  LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0Tester_Case10 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_LDM_exception_return actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc100pu1w1nnnn1rrrrrrrrrrrrrrr");
}

// op(25:20)=10xxxx
//    = {Pc: 15,
//       actual: Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {Pc},
//       fields: [imm24(23:0)],
//       imm24: imm24(23:0),
//       imm32: SignExtend(imm24:'00'(1:0), 32),
//       pattern: cccc1010iiiiiiiiiiiiiiiiiiiiiiii,
//       relative: true,
//       relative_offset: imm32 + 8,
//       rule: B,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'relative']}
TEST_F(Arm32DecoderStateTests,
       B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case11_TestCase11) {
  B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case11 baseline_tester;
  NamedActual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1_B actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc1010iiiiiiiiiiiiiiiiiiiiiiii");
}

// op(25:20)=11xxxx
//    = {Lr: 14,
//       Pc: 15,
//       actual: Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0,
//       defs: {Pc, Lr},
//       fields: [imm24(23:0)],
//       imm24: imm24(23:0),
//       imm32: SignExtend(imm24:'00'(1:0), 32),
//       pattern: cccc1011iiiiiiiiiiiiiiiiiiiiiiii,
//       relative: true,
//       relative_offset: imm32 + 8,
//       rule: BL_BLX_immediate,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'relative']}
TEST_F(Arm32DecoderStateTests,
       BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case12_TestCase12) {
  BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0Tester_Case12 baseline_tester;
  NamedActual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1_BL_BLX_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc1011iiiiiiiiiiiiiiiiiiiiiiii");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
