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


// op1(24:20)=10001 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010001nnnn0000ssss0tt1mmmm,
//       rule: TST_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
class TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~10001
  if ((inst.Bits() & 0x01F00000)  !=
          0x01100000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=10011 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010011nnnn0000ssss0tt1mmmm,
//       rule: TEQ_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
class TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~10011
  if ((inst.Bits() & 0x01F00000)  !=
          0x01300000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=10101 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010101nnnn0000ssss0tt1mmmm,
//       rule: CMP_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
class CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~10101
  if ((inst.Bits() & 0x01F00000)  !=
          0x01500000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=10111 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010111nnnn0000ssss0tt1mmmm,
//       rule: CMN_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
class CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~10111
  if ((inst.Bits() & 0x01F00000)  !=
          0x01700000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=0000x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000000snnnnddddssss0tt1mmmm,
//       rule: AND_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~0000x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=0001x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000001snnnnddddssss0tt1mmmm,
//       rule: EOR_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~0001x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00200000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=0010x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000010snnnnddddssss0tt1mmmm,
//       rule: SUB_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~0010x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00400000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=0011x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000011snnnnddddssss0tt1mmmm,
//       rule: RSB_register_shfited_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~0011x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00600000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=0100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000100snnnnddddssss0tt1mmmm,
//       rule: ADD_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~0100x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00800000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=0101x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000101snnnnddddssss0tt1mmmm,
//       rule: ADC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~0101x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00A00000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=0110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000110snnnnddddssss0tt1mmmm,
//       rule: SBC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~0110x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00C00000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=0111x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000111snnnnddddssss0tt1mmmm,
//       rule: RSC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~0111x
  if ((inst.Bits() & 0x01E00000)  !=
          0x00E00000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0001100snnnnddddssss0tt1mmmm,
//       rule: ORR_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1100x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01800000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1101x & op2(6:5)=00 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0001nnnn,
//       rule: LSL_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1101x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01A00000) return false;
  // op2(6:5)=~00
  if ((inst.Bits() & 0x00000060)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1101x & op2(6:5)=01 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0011nnnn,
//       rule: LSR_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1101x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01A00000) return false;
  // op2(6:5)=~01
  if ((inst.Bits() & 0x00000060)  !=
          0x00000020) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1101x & op2(6:5)=10 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0101nnnn,
//       rule: ASR_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1101x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01A00000) return false;
  // op2(6:5)=~10
  if ((inst.Bits() & 0x00000060)  !=
          0x00000040) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1101x & op2(6:5)=11 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0111nnnn,
//       rule: ROR_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0TesterCase16
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1101x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01A00000) return false;
  // op2(6:5)=~11
  if ((inst.Bits() & 0x00000060)  !=
          0x00000060) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0001110snnnnddddssss0tt1mmmm,
//       rule: BIC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0TesterCase17
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1110x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01C00000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1111x & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0001111s0000ddddssss0tt1mmmm,
//       rule: MVN_register_shifted_register,
//       safety: [Pc in {Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rm, Rs}}
class MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0TesterCase18
    : public Arm32DecoderTester {
 public:
  MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0TesterCase18(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0TesterCase18
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1111x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01E00000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
  if ((inst.Bits() & 0x000F0000)  !=
          0x00000000) return false;

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

// op1(24:20)=10001 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010001nnnn0000ssss0tt1mmmm,
//       rule: TST_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
class TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0Tester_Case0
    : public TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0TesterCase0 {
 public:
  TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0Tester_Case0()
    : TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0TesterCase0(
      state_.TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0_TST_register_shifted_register_instance_)
  {}
};

// op1(24:20)=10011 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010011nnnn0000ssss0tt1mmmm,
//       rule: TEQ_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
class TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0Tester_Case1
    : public TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0TesterCase1 {
 public:
  TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0Tester_Case1()
    : TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0TesterCase1(
      state_.TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0_TEQ_register_shifted_register_instance_)
  {}
};

// op1(24:20)=10101 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010101nnnn0000ssss0tt1mmmm,
//       rule: CMP_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
class CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0Tester_Case2
    : public CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0TesterCase2 {
 public:
  CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0Tester_Case2()
    : CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0TesterCase2(
      state_.CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0_CMP_register_shifted_register_instance_)
  {}
};

// op1(24:20)=10111 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010111nnnn0000ssss0tt1mmmm,
//       rule: CMN_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
class CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0Tester_Case3
    : public CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0TesterCase3 {
 public:
  CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0Tester_Case3()
    : CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0TesterCase3(
      state_.CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0_CMN_register_shifted_register_instance_)
  {}
};

// op1(24:20)=0000x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000000snnnnddddssss0tt1mmmm,
//       rule: AND_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0Tester_Case4
    : public AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0TesterCase4 {
 public:
  AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0Tester_Case4()
    : AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0TesterCase4(
      state_.AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0_AND_register_shifted_register_instance_)
  {}
};

// op1(24:20)=0001x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000001snnnnddddssss0tt1mmmm,
//       rule: EOR_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0Tester_Case5
    : public EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0TesterCase5 {
 public:
  EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0Tester_Case5()
    : EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0TesterCase5(
      state_.EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0_EOR_register_shifted_register_instance_)
  {}
};

// op1(24:20)=0010x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000010snnnnddddssss0tt1mmmm,
//       rule: SUB_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0Tester_Case6
    : public SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0TesterCase6 {
 public:
  SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0Tester_Case6()
    : SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0TesterCase6(
      state_.SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0_SUB_register_shifted_register_instance_)
  {}
};

// op1(24:20)=0011x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000011snnnnddddssss0tt1mmmm,
//       rule: RSB_register_shfited_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0Tester_Case7
    : public RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0TesterCase7 {
 public:
  RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0Tester_Case7()
    : RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0TesterCase7(
      state_.RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0_RSB_register_shfited_register_instance_)
  {}
};

// op1(24:20)=0100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000100snnnnddddssss0tt1mmmm,
//       rule: ADD_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0Tester_Case8
    : public ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0TesterCase8 {
 public:
  ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0Tester_Case8()
    : ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0TesterCase8(
      state_.ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0_ADD_register_shifted_register_instance_)
  {}
};

// op1(24:20)=0101x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000101snnnnddddssss0tt1mmmm,
//       rule: ADC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0Tester_Case9
    : public ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0TesterCase9 {
 public:
  ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0Tester_Case9()
    : ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0TesterCase9(
      state_.ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0_ADC_register_shifted_register_instance_)
  {}
};

// op1(24:20)=0110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000110snnnnddddssss0tt1mmmm,
//       rule: SBC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0Tester_Case10
    : public SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0TesterCase10 {
 public:
  SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0Tester_Case10()
    : SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0TesterCase10(
      state_.SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0_SBC_register_shifted_register_instance_)
  {}
};

// op1(24:20)=0111x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000111snnnnddddssss0tt1mmmm,
//       rule: RSC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0Tester_Case11
    : public RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0TesterCase11 {
 public:
  RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0Tester_Case11()
    : RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0TesterCase11(
      state_.RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0_RSC_register_shifted_register_instance_)
  {}
};

// op1(24:20)=1100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0001100snnnnddddssss0tt1mmmm,
//       rule: ORR_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0Tester_Case12
    : public ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0TesterCase12 {
 public:
  ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0Tester_Case12()
    : ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0TesterCase12(
      state_.ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0_ORR_register_shifted_register_instance_)
  {}
};

// op1(24:20)=1101x & op2(6:5)=00 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0001nnnn,
//       rule: LSL_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0Tester_Case13
    : public LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0TesterCase13 {
 public:
  LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0Tester_Case13()
    : LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0TesterCase13(
      state_.LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0_LSL_register_instance_)
  {}
};

// op1(24:20)=1101x & op2(6:5)=01 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0011nnnn,
//       rule: LSR_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0Tester_Case14
    : public LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0TesterCase14 {
 public:
  LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0Tester_Case14()
    : LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0TesterCase14(
      state_.LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0_LSR_register_instance_)
  {}
};

// op1(24:20)=1101x & op2(6:5)=10 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0101nnnn,
//       rule: ASR_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0Tester_Case15
    : public ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0TesterCase15 {
 public:
  ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0Tester_Case15()
    : ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0TesterCase15(
      state_.ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0_ASR_register_instance_)
  {}
};

// op1(24:20)=1101x & op2(6:5)=11 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0111nnnn,
//       rule: ROR_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0Tester_Case16
    : public ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0TesterCase16 {
 public:
  ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0Tester_Case16()
    : ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0TesterCase16(
      state_.ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0_ROR_register_instance_)
  {}
};

// op1(24:20)=1110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0001110snnnnddddssss0tt1mmmm,
//       rule: BIC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
class BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0Tester_Case17
    : public BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0TesterCase17 {
 public:
  BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0Tester_Case17()
    : BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0TesterCase17(
      state_.BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0_BIC_register_shifted_register_instance_)
  {}
};

// op1(24:20)=1111x & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0001111s0000ddddssss0tt1mmmm,
//       rule: MVN_register_shifted_register,
//       safety: [Pc in {Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rm, Rs}}
class MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0Tester_Case18
    : public MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0TesterCase18 {
 public:
  MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0Tester_Case18()
    : MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0TesterCase18(
      state_.MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0_MVN_register_shifted_register_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op1(24:20)=10001 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010001nnnn0000ssss0tt1mmmm,
//       rule: TST_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0Tester_Case0_TestCase0) {
  TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1_TST_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010001nnnn0000ssss0tt1mmmm");
}

// op1(24:20)=10011 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010011nnnn0000ssss0tt1mmmm,
//       rule: TEQ_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0Tester_Case1_TestCase1) {
  TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1_TEQ_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010011nnnn0000ssss0tt1mmmm");
}

// op1(24:20)=10101 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010101nnnn0000ssss0tt1mmmm,
//       rule: CMP_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0Tester_Case2_TestCase2) {
  CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1_CMP_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010101nnnn0000ssss0tt1mmmm");
}

// op1(24:20)=10111 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//       baseline: CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0,
//       defs: {NZCV},
//       fields: [Rn(19:16), Rs(11:8), Rm(3:0)],
//       pattern: cccc00010111nnnn0000ssss0tt1mmmm,
//       rule: CMN_register_shifted_register,
//       safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0Tester_Case3_TestCase3) {
  CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1_CMN_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00010111nnnn0000ssss0tt1mmmm");
}

// op1(24:20)=0000x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000000snnnnddddssss0tt1mmmm,
//       rule: AND_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0Tester_Case4_TestCase4) {
  AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0Tester_Case4 baseline_tester;
  NamedActual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_AND_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000000snnnnddddssss0tt1mmmm");
}

// op1(24:20)=0001x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000001snnnnddddssss0tt1mmmm,
//       rule: EOR_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0Tester_Case5_TestCase5) {
  EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0Tester_Case5 baseline_tester;
  NamedActual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_EOR_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000001snnnnddddssss0tt1mmmm");
}

// op1(24:20)=0010x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000010snnnnddddssss0tt1mmmm,
//       rule: SUB_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0Tester_Case6_TestCase6) {
  SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_SUB_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000010snnnnddddssss0tt1mmmm");
}

// op1(24:20)=0011x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000011snnnnddddssss0tt1mmmm,
//       rule: RSB_register_shfited_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0Tester_Case7_TestCase7) {
  RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_RSB_register_shfited_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000011snnnnddddssss0tt1mmmm");
}

// op1(24:20)=0100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000100snnnnddddssss0tt1mmmm,
//       rule: ADD_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0Tester_Case8_TestCase8) {
  ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0Tester_Case8 baseline_tester;
  NamedActual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_ADD_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000100snnnnddddssss0tt1mmmm");
}

// op1(24:20)=0101x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000101snnnnddddssss0tt1mmmm,
//       rule: ADC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0Tester_Case9_TestCase9) {
  ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0Tester_Case9 baseline_tester;
  NamedActual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_ADC_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000101snnnnddddssss0tt1mmmm");
}

// op1(24:20)=0110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000110snnnnddddssss0tt1mmmm,
//       rule: SBC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0Tester_Case10_TestCase10) {
  SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0Tester_Case10 baseline_tester;
  NamedActual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_SBC_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000110snnnnddddssss0tt1mmmm");
}

// op1(24:20)=0111x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0000111snnnnddddssss0tt1mmmm,
//       rule: RSC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0Tester_Case11_TestCase11) {
  RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0Tester_Case11 baseline_tester;
  NamedActual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_RSC_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000111snnnnddddssss0tt1mmmm");
}

// op1(24:20)=1100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0001100snnnnddddssss0tt1mmmm,
//       rule: ORR_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0Tester_Case12_TestCase12) {
  ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0Tester_Case12 baseline_tester;
  NamedActual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_ORR_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0001100snnnnddddssss0tt1mmmm");
}

// op1(24:20)=1101x & op2(6:5)=00 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0001nnnn,
//       rule: LSL_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0Tester_Case13_TestCase13) {
  LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0Tester_Case13 baseline_tester;
  NamedActual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1_LSL_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0001101s0000ddddmmmm0001nnnn");
}

// op1(24:20)=1101x & op2(6:5)=01 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0011nnnn,
//       rule: LSR_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0Tester_Case14_TestCase14) {
  LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0Tester_Case14 baseline_tester;
  NamedActual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1_LSR_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0001101s0000ddddmmmm0011nnnn");
}

// op1(24:20)=1101x & op2(6:5)=10 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0101nnnn,
//       rule: ASR_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0Tester_Case15_TestCase15) {
  ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0Tester_Case15 baseline_tester;
  NamedActual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1_ASR_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0001101s0000ddddmmmm0101nnnn");
}

// op1(24:20)=1101x & op2(6:5)=11 & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0001101s0000ddddmmmm0111nnnn,
//       rule: ROR_register,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0Tester_Case16_TestCase16) {
  ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0Tester_Case16 baseline_tester;
  NamedActual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1_ROR_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0001101s0000ddddmmmm0111nnnn");
}

// op1(24:20)=1110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//       baseline: BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rn(19:16), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0001110snnnnddddssss0tt1mmmm,
//       rule: BIC_register_shifted_register,
//       safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0Tester_Case17_TestCase17) {
  BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0Tester_Case17 baseline_tester;
  NamedActual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_BIC_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0001110snnnnddddssss0tt1mmmm");
}

// op1(24:20)=1111x & $pattern(31:0)=xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rs: Rs(11:8),
//       S: S(20),
//       actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//       baseline: MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(15:12), Rs(11:8), Rm(3:0)],
//       pattern: cccc0001111s0000ddddssss0tt1mmmm,
//       rule: MVN_register_shifted_register,
//       safety: [Pc in {Rd, Rm, Rs} => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rm, Rs}}
TEST_F(Arm32DecoderStateTests,
       MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0Tester_Case18_TestCase18) {
  MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0Tester_Case18 baseline_tester;
  NamedActual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1_MVN_register_shifted_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0001111s0000ddddssss0tt1mmmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
