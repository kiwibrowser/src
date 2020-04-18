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


// op1(21:20)=01 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SADD16_cccc01100001nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110001mmmm,
//       rule: SADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SADD16_cccc01100001nnnndddd11110001mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  SADD16_cccc01100001nnnndddd11110001mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SADD16_cccc01100001nnnndddd11110001mmmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~01
  if ((inst.Bits() & 0x00300000)  !=
          0x00100000) return false;
  // op2(7:5)=~000
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=01 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SASX_cccc01100001nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110011mmmm,
//       rule: SASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SASX_cccc01100001nnnndddd11110011mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  SASX_cccc01100001nnnndddd11110011mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SASX_cccc01100001nnnndddd11110011mmmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~01
  if ((inst.Bits() & 0x00300000)  !=
          0x00100000) return false;
  // op2(7:5)=~001
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000020) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=01 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SSAX_cccc01100001nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110101mmmm,
//       rule: SSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SSAX_cccc01100001nnnndddd11110101mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  SSAX_cccc01100001nnnndddd11110101mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SSAX_cccc01100001nnnndddd11110101mmmm_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~01
  if ((inst.Bits() & 0x00300000)  !=
          0x00100000) return false;
  // op2(7:5)=~010
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000040) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=01 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110111mmmm,
//       rule: SSSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~01
  if ((inst.Bits() & 0x00300000)  !=
          0x00100000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=01 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SADD8_cccc01100001nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11111001mmmm,
//       rule: SADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SADD8_cccc01100001nnnndddd11111001mmmm_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  SADD8_cccc01100001nnnndddd11111001mmmm_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SADD8_cccc01100001nnnndddd11111001mmmm_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~01
  if ((inst.Bits() & 0x00300000)  !=
          0x00100000) return false;
  // op2(7:5)=~100
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000080) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=01 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SSUB8_cccc01100001nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11111111mmmm,
//       rule: SSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SSUB8_cccc01100001nnnndddd11111111mmmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  SSUB8_cccc01100001nnnndddd11111111mmmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SSUB8_cccc01100001nnnndddd11111111mmmm_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~01
  if ((inst.Bits() & 0x00300000)  !=
          0x00100000) return false;
  // op2(7:5)=~111
  if ((inst.Bits() & 0x000000E0)  !=
          0x000000E0) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=10 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QADD16_cccc01100010nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110001mmmm,
//       rule: QADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QADD16_cccc01100010nnnndddd11110001mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  QADD16_cccc01100010nnnndddd11110001mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool QADD16_cccc01100010nnnndddd11110001mmmm_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~10
  if ((inst.Bits() & 0x00300000)  !=
          0x00200000) return false;
  // op2(7:5)=~000
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=10 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QASX_cccc01100010nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110011mmmm,
//       rule: QASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QASX_cccc01100010nnnndddd11110011mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  QASX_cccc01100010nnnndddd11110011mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool QASX_cccc01100010nnnndddd11110011mmmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~10
  if ((inst.Bits() & 0x00300000)  !=
          0x00200000) return false;
  // op2(7:5)=~001
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000020) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=10 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSAX_cccc01100010nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110101mmmm,
//       rule: QSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QSAX_cccc01100010nnnndddd11110101mmmm_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  QSAX_cccc01100010nnnndddd11110101mmmm_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool QSAX_cccc01100010nnnndddd11110101mmmm_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~10
  if ((inst.Bits() & 0x00300000)  !=
          0x00200000) return false;
  // op2(7:5)=~010
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000040) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=10 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSUB16_cccc01100010nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110111mmmm,
//       rule: QSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QSUB16_cccc01100010nnnndddd11110111mmmm_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  QSUB16_cccc01100010nnnndddd11110111mmmm_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool QSUB16_cccc01100010nnnndddd11110111mmmm_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~10
  if ((inst.Bits() & 0x00300000)  !=
          0x00200000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=10 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QADD8_cccc01100010nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11111001mmmm,
//       rule: QADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QADD8_cccc01100010nnnndddd11111001mmmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  QADD8_cccc01100010nnnndddd11111001mmmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool QADD8_cccc01100010nnnndddd11111001mmmm_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~10
  if ((inst.Bits() & 0x00300000)  !=
          0x00200000) return false;
  // op2(7:5)=~100
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000080) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=10 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSUB8_cccc01100010nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11111111mmmm,
//       rule: QSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QSUB8_cccc01100010nnnndddd11111111mmmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  QSUB8_cccc01100010nnnndddd11111111mmmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool QSUB8_cccc01100010nnnndddd11111111mmmm_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~10
  if ((inst.Bits() & 0x00300000)  !=
          0x00200000) return false;
  // op2(7:5)=~111
  if ((inst.Bits() & 0x000000E0)  !=
          0x000000E0) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=11 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHADD16_cccc01100011nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110001mmmm,
//       rule: SHADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHADD16_cccc01100011nnnndddd11110001mmmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  SHADD16_cccc01100011nnnndddd11110001mmmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SHADD16_cccc01100011nnnndddd11110001mmmm_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~11
  if ((inst.Bits() & 0x00300000)  !=
          0x00300000) return false;
  // op2(7:5)=~000
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=11 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHASX_cccc01100011nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110011mmmm,
//       rule: SHASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHASX_cccc01100011nnnndddd11110011mmmm_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  SHASX_cccc01100011nnnndddd11110011mmmm_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SHASX_cccc01100011nnnndddd11110011mmmm_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~11
  if ((inst.Bits() & 0x00300000)  !=
          0x00300000) return false;
  // op2(7:5)=~001
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000020) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=11 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHSAX_cccc01100011nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110101mmmm,
//       rule: SHSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHSAX_cccc01100011nnnndddd11110101mmmm_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  SHSAX_cccc01100011nnnndddd11110101mmmm_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SHSAX_cccc01100011nnnndddd11110101mmmm_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~11
  if ((inst.Bits() & 0x00300000)  !=
          0x00300000) return false;
  // op2(7:5)=~010
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000040) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=11 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110111mmmm,
//       rule: SHSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~11
  if ((inst.Bits() & 0x00300000)  !=
          0x00300000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=11 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHADD8_cccc01100011nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11111001mmmm,
//       rule: SHADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHADD8_cccc01100011nnnndddd11111001mmmm_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  SHADD8_cccc01100011nnnndddd11111001mmmm_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SHADD8_cccc01100011nnnndddd11111001mmmm_case_0TesterCase16
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~11
  if ((inst.Bits() & 0x00300000)  !=
          0x00300000) return false;
  // op2(7:5)=~100
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000080) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(21:20)=11 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11111111mmmm,
//       rule: SHSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0TesterCase17
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(21:20)=~11
  if ((inst.Bits() & 0x00300000)  !=
          0x00300000) return false;
  // op2(7:5)=~111
  if ((inst.Bits() & 0x000000E0)  !=
          0x000000E0) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

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

// op1(21:20)=01 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SADD16_cccc01100001nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110001mmmm,
//       rule: SADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SADD16_cccc01100001nnnndddd11110001mmmm_case_0Tester_Case0
    : public SADD16_cccc01100001nnnndddd11110001mmmm_case_0TesterCase0 {
 public:
  SADD16_cccc01100001nnnndddd11110001mmmm_case_0Tester_Case0()
    : SADD16_cccc01100001nnnndddd11110001mmmm_case_0TesterCase0(
      state_.SADD16_cccc01100001nnnndddd11110001mmmm_case_0_SADD16_instance_)
  {}
};

// op1(21:20)=01 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SASX_cccc01100001nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110011mmmm,
//       rule: SASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SASX_cccc01100001nnnndddd11110011mmmm_case_0Tester_Case1
    : public SASX_cccc01100001nnnndddd11110011mmmm_case_0TesterCase1 {
 public:
  SASX_cccc01100001nnnndddd11110011mmmm_case_0Tester_Case1()
    : SASX_cccc01100001nnnndddd11110011mmmm_case_0TesterCase1(
      state_.SASX_cccc01100001nnnndddd11110011mmmm_case_0_SASX_instance_)
  {}
};

// op1(21:20)=01 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SSAX_cccc01100001nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110101mmmm,
//       rule: SSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SSAX_cccc01100001nnnndddd11110101mmmm_case_0Tester_Case2
    : public SSAX_cccc01100001nnnndddd11110101mmmm_case_0TesterCase2 {
 public:
  SSAX_cccc01100001nnnndddd11110101mmmm_case_0Tester_Case2()
    : SSAX_cccc01100001nnnndddd11110101mmmm_case_0TesterCase2(
      state_.SSAX_cccc01100001nnnndddd11110101mmmm_case_0_SSAX_instance_)
  {}
};

// op1(21:20)=01 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110111mmmm,
//       rule: SSSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0Tester_Case3
    : public SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0TesterCase3 {
 public:
  SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0Tester_Case3()
    : SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0TesterCase3(
      state_.SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0_SSSUB16_instance_)
  {}
};

// op1(21:20)=01 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SADD8_cccc01100001nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11111001mmmm,
//       rule: SADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SADD8_cccc01100001nnnndddd11111001mmmm_case_0Tester_Case4
    : public SADD8_cccc01100001nnnndddd11111001mmmm_case_0TesterCase4 {
 public:
  SADD8_cccc01100001nnnndddd11111001mmmm_case_0Tester_Case4()
    : SADD8_cccc01100001nnnndddd11111001mmmm_case_0TesterCase4(
      state_.SADD8_cccc01100001nnnndddd11111001mmmm_case_0_SADD8_instance_)
  {}
};

// op1(21:20)=01 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SSUB8_cccc01100001nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11111111mmmm,
//       rule: SSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SSUB8_cccc01100001nnnndddd11111111mmmm_case_0Tester_Case5
    : public SSUB8_cccc01100001nnnndddd11111111mmmm_case_0TesterCase5 {
 public:
  SSUB8_cccc01100001nnnndddd11111111mmmm_case_0Tester_Case5()
    : SSUB8_cccc01100001nnnndddd11111111mmmm_case_0TesterCase5(
      state_.SSUB8_cccc01100001nnnndddd11111111mmmm_case_0_SSUB8_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QADD16_cccc01100010nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110001mmmm,
//       rule: QADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QADD16_cccc01100010nnnndddd11110001mmmm_case_0Tester_Case6
    : public QADD16_cccc01100010nnnndddd11110001mmmm_case_0TesterCase6 {
 public:
  QADD16_cccc01100010nnnndddd11110001mmmm_case_0Tester_Case6()
    : QADD16_cccc01100010nnnndddd11110001mmmm_case_0TesterCase6(
      state_.QADD16_cccc01100010nnnndddd11110001mmmm_case_0_QADD16_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QASX_cccc01100010nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110011mmmm,
//       rule: QASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QASX_cccc01100010nnnndddd11110011mmmm_case_0Tester_Case7
    : public QASX_cccc01100010nnnndddd11110011mmmm_case_0TesterCase7 {
 public:
  QASX_cccc01100010nnnndddd11110011mmmm_case_0Tester_Case7()
    : QASX_cccc01100010nnnndddd11110011mmmm_case_0TesterCase7(
      state_.QASX_cccc01100010nnnndddd11110011mmmm_case_0_QASX_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSAX_cccc01100010nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110101mmmm,
//       rule: QSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QSAX_cccc01100010nnnndddd11110101mmmm_case_0Tester_Case8
    : public QSAX_cccc01100010nnnndddd11110101mmmm_case_0TesterCase8 {
 public:
  QSAX_cccc01100010nnnndddd11110101mmmm_case_0Tester_Case8()
    : QSAX_cccc01100010nnnndddd11110101mmmm_case_0TesterCase8(
      state_.QSAX_cccc01100010nnnndddd11110101mmmm_case_0_QSAX_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSUB16_cccc01100010nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110111mmmm,
//       rule: QSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QSUB16_cccc01100010nnnndddd11110111mmmm_case_0Tester_Case9
    : public QSUB16_cccc01100010nnnndddd11110111mmmm_case_0TesterCase9 {
 public:
  QSUB16_cccc01100010nnnndddd11110111mmmm_case_0Tester_Case9()
    : QSUB16_cccc01100010nnnndddd11110111mmmm_case_0TesterCase9(
      state_.QSUB16_cccc01100010nnnndddd11110111mmmm_case_0_QSUB16_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QADD8_cccc01100010nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11111001mmmm,
//       rule: QADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QADD8_cccc01100010nnnndddd11111001mmmm_case_0Tester_Case10
    : public QADD8_cccc01100010nnnndddd11111001mmmm_case_0TesterCase10 {
 public:
  QADD8_cccc01100010nnnndddd11111001mmmm_case_0Tester_Case10()
    : QADD8_cccc01100010nnnndddd11111001mmmm_case_0TesterCase10(
      state_.QADD8_cccc01100010nnnndddd11111001mmmm_case_0_QADD8_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSUB8_cccc01100010nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11111111mmmm,
//       rule: QSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class QSUB8_cccc01100010nnnndddd11111111mmmm_case_0Tester_Case11
    : public QSUB8_cccc01100010nnnndddd11111111mmmm_case_0TesterCase11 {
 public:
  QSUB8_cccc01100010nnnndddd11111111mmmm_case_0Tester_Case11()
    : QSUB8_cccc01100010nnnndddd11111111mmmm_case_0TesterCase11(
      state_.QSUB8_cccc01100010nnnndddd11111111mmmm_case_0_QSUB8_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHADD16_cccc01100011nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110001mmmm,
//       rule: SHADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHADD16_cccc01100011nnnndddd11110001mmmm_case_0Tester_Case12
    : public SHADD16_cccc01100011nnnndddd11110001mmmm_case_0TesterCase12 {
 public:
  SHADD16_cccc01100011nnnndddd11110001mmmm_case_0Tester_Case12()
    : SHADD16_cccc01100011nnnndddd11110001mmmm_case_0TesterCase12(
      state_.SHADD16_cccc01100011nnnndddd11110001mmmm_case_0_SHADD16_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHASX_cccc01100011nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110011mmmm,
//       rule: SHASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHASX_cccc01100011nnnndddd11110011mmmm_case_0Tester_Case13
    : public SHASX_cccc01100011nnnndddd11110011mmmm_case_0TesterCase13 {
 public:
  SHASX_cccc01100011nnnndddd11110011mmmm_case_0Tester_Case13()
    : SHASX_cccc01100011nnnndddd11110011mmmm_case_0TesterCase13(
      state_.SHASX_cccc01100011nnnndddd11110011mmmm_case_0_SHASX_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHSAX_cccc01100011nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110101mmmm,
//       rule: SHSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHSAX_cccc01100011nnnndddd11110101mmmm_case_0Tester_Case14
    : public SHSAX_cccc01100011nnnndddd11110101mmmm_case_0TesterCase14 {
 public:
  SHSAX_cccc01100011nnnndddd11110101mmmm_case_0Tester_Case14()
    : SHSAX_cccc01100011nnnndddd11110101mmmm_case_0TesterCase14(
      state_.SHSAX_cccc01100011nnnndddd11110101mmmm_case_0_SHSAX_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110111mmmm,
//       rule: SHSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0Tester_Case15
    : public SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0TesterCase15 {
 public:
  SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0Tester_Case15()
    : SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0TesterCase15(
      state_.SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0_SHSUB16_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHADD8_cccc01100011nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11111001mmmm,
//       rule: SHADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHADD8_cccc01100011nnnndddd11111001mmmm_case_0Tester_Case16
    : public SHADD8_cccc01100011nnnndddd11111001mmmm_case_0TesterCase16 {
 public:
  SHADD8_cccc01100011nnnndddd11111001mmmm_case_0Tester_Case16()
    : SHADD8_cccc01100011nnnndddd11111001mmmm_case_0TesterCase16(
      state_.SHADD8_cccc01100011nnnndddd11111001mmmm_case_0_SHADD8_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11111111mmmm,
//       rule: SHSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0Tester_Case17
    : public SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0TesterCase17 {
 public:
  SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0Tester_Case17()
    : SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0TesterCase17(
      state_.SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0_SHSUB8_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op1(21:20)=01 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SADD16_cccc01100001nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110001mmmm,
//       rule: SADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SADD16_cccc01100001nnnndddd11110001mmmm_case_0Tester_Case0_TestCase0) {
  SADD16_cccc01100001nnnndddd11110001mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SADD16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100001nnnndddd11110001mmmm");
}

// op1(21:20)=01 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SASX_cccc01100001nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110011mmmm,
//       rule: SASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SASX_cccc01100001nnnndddd11110011mmmm_case_0Tester_Case1_TestCase1) {
  SASX_cccc01100001nnnndddd11110011mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SASX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100001nnnndddd11110011mmmm");
}

// op1(21:20)=01 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SSAX_cccc01100001nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110101mmmm,
//       rule: SSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SSAX_cccc01100001nnnndddd11110101mmmm_case_0Tester_Case2_TestCase2) {
  SSAX_cccc01100001nnnndddd11110101mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SSAX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100001nnnndddd11110101mmmm");
}

// op1(21:20)=01 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11110111mmmm,
//       rule: SSSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0Tester_Case3_TestCase3) {
  SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SSSUB16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100001nnnndddd11110111mmmm");
}

// op1(21:20)=01 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SADD8_cccc01100001nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11111001mmmm,
//       rule: SADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SADD8_cccc01100001nnnndddd11111001mmmm_case_0Tester_Case4_TestCase4) {
  SADD8_cccc01100001nnnndddd11111001mmmm_case_0Tester_Case4 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SADD8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100001nnnndddd11111001mmmm");
}

// op1(21:20)=01 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SSUB8_cccc01100001nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100001nnnndddd11111111mmmm,
//       rule: SSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SSUB8_cccc01100001nnnndddd11111111mmmm_case_0Tester_Case5_TestCase5) {
  SSUB8_cccc01100001nnnndddd11111111mmmm_case_0Tester_Case5 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SSUB8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100001nnnndddd11111111mmmm");
}

// op1(21:20)=10 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QADD16_cccc01100010nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110001mmmm,
//       rule: QADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       QADD16_cccc01100010nnnndddd11110001mmmm_case_0Tester_Case6_TestCase6) {
  QADD16_cccc01100010nnnndddd11110001mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_QADD16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100010nnnndddd11110001mmmm");
}

// op1(21:20)=10 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QASX_cccc01100010nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110011mmmm,
//       rule: QASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       QASX_cccc01100010nnnndddd11110011mmmm_case_0Tester_Case7_TestCase7) {
  QASX_cccc01100010nnnndddd11110011mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_QASX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100010nnnndddd11110011mmmm");
}

// op1(21:20)=10 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSAX_cccc01100010nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110101mmmm,
//       rule: QSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       QSAX_cccc01100010nnnndddd11110101mmmm_case_0Tester_Case8_TestCase8) {
  QSAX_cccc01100010nnnndddd11110101mmmm_case_0Tester_Case8 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_QSAX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100010nnnndddd11110101mmmm");
}

// op1(21:20)=10 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSUB16_cccc01100010nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11110111mmmm,
//       rule: QSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       QSUB16_cccc01100010nnnndddd11110111mmmm_case_0Tester_Case9_TestCase9) {
  QSUB16_cccc01100010nnnndddd11110111mmmm_case_0Tester_Case9 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_QSUB16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100010nnnndddd11110111mmmm");
}

// op1(21:20)=10 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QADD8_cccc01100010nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11111001mmmm,
//       rule: QADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       QADD8_cccc01100010nnnndddd11111001mmmm_case_0Tester_Case10_TestCase10) {
  QADD8_cccc01100010nnnndddd11111001mmmm_case_0Tester_Case10 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_QADD8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100010nnnndddd11111001mmmm");
}

// op1(21:20)=10 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: QSUB8_cccc01100010nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100010nnnndddd11111111mmmm,
//       rule: QSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       QSUB8_cccc01100010nnnndddd11111111mmmm_case_0Tester_Case11_TestCase11) {
  QSUB8_cccc01100010nnnndddd11111111mmmm_case_0Tester_Case11 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_QSUB8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100010nnnndddd11111111mmmm");
}

// op1(21:20)=11 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHADD16_cccc01100011nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110001mmmm,
//       rule: SHADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SHADD16_cccc01100011nnnndddd11110001mmmm_case_0Tester_Case12_TestCase12) {
  SHADD16_cccc01100011nnnndddd11110001mmmm_case_0Tester_Case12 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SHADD16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100011nnnndddd11110001mmmm");
}

// op1(21:20)=11 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHASX_cccc01100011nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110011mmmm,
//       rule: SHASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SHASX_cccc01100011nnnndddd11110011mmmm_case_0Tester_Case13_TestCase13) {
  SHASX_cccc01100011nnnndddd11110011mmmm_case_0Tester_Case13 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SHASX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100011nnnndddd11110011mmmm");
}

// op1(21:20)=11 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHSAX_cccc01100011nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110101mmmm,
//       rule: SHSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SHSAX_cccc01100011nnnndddd11110101mmmm_case_0Tester_Case14_TestCase14) {
  SHSAX_cccc01100011nnnndddd11110101mmmm_case_0Tester_Case14 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SHSAX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100011nnnndddd11110101mmmm");
}

// op1(21:20)=11 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11110111mmmm,
//       rule: SHSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0Tester_Case15_TestCase15) {
  SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0Tester_Case15 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SHSUB16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100011nnnndddd11110111mmmm");
}

// op1(21:20)=11 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHADD8_cccc01100011nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11111001mmmm,
//       rule: SHADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SHADD8_cccc01100011nnnndddd11111001mmmm_case_0Tester_Case16_TestCase16) {
  SHADD8_cccc01100011nnnndddd11111001mmmm_case_0Tester_Case16 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SHADD8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100011nnnndddd11111001mmmm");
}

// op1(21:20)=11 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100011nnnndddd11111111mmmm,
//       rule: SHSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0Tester_Case17_TestCase17) {
  SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0Tester_Case17 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SHSUB8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100011nnnndddd11111111mmmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
