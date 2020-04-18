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
//       baseline: UADD16_cccc01100101nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110001mmmm,
//       rule: UADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UADD16_cccc01100101nnnndddd11110001mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  UADD16_cccc01100101nnnndddd11110001mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UADD16_cccc01100101nnnndddd11110001mmmm_case_0TesterCase0
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
//       baseline: UASX_cccc01100101nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110011mmmm,
//       rule: UASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UASX_cccc01100101nnnndddd11110011mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  UASX_cccc01100101nnnndddd11110011mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UASX_cccc01100101nnnndddd11110011mmmm_case_0TesterCase1
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
//       baseline: USAX_cccc01100101nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110101mmmm,
//       rule: USAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class USAX_cccc01100101nnnndddd11110101mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  USAX_cccc01100101nnnndddd11110101mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool USAX_cccc01100101nnnndddd11110101mmmm_case_0TesterCase2
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
//       baseline: USUB16_cccc01100101nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110111mmmm,
//       rule: USUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class USUB16_cccc01100101nnnndddd11110111mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  USUB16_cccc01100101nnnndddd11110111mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool USUB16_cccc01100101nnnndddd11110111mmmm_case_0TesterCase3
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
//       baseline: UADD8_cccc01100101nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11111001mmmm,
//       rule: UADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UADD8_cccc01100101nnnndddd11111001mmmm_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  UADD8_cccc01100101nnnndddd11111001mmmm_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UADD8_cccc01100101nnnndddd11111001mmmm_case_0TesterCase4
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
//       baseline: USUB8_cccc01100101nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11111111mmmm,
//       rule: USUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class USUB8_cccc01100101nnnndddd11111111mmmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  USUB8_cccc01100101nnnndddd11111111mmmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool USUB8_cccc01100101nnnndddd11111111mmmm_case_0TesterCase5
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
//       baseline: UQADD16_cccc01100110nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110001mmmm,
//       rule: UQADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQADD16_cccc01100110nnnndddd11110001mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  UQADD16_cccc01100110nnnndddd11110001mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UQADD16_cccc01100110nnnndddd11110001mmmm_case_0TesterCase6
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
//       baseline: UQASX_cccc01100110nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110011mmmm,
//       rule: UQASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQASX_cccc01100110nnnndddd11110011mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  UQASX_cccc01100110nnnndddd11110011mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UQASX_cccc01100110nnnndddd11110011mmmm_case_0TesterCase7
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
//       baseline: UQSAX_cccc01100110nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110101mmmm,
//       rule: UQSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQSAX_cccc01100110nnnndddd11110101mmmm_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  UQSAX_cccc01100110nnnndddd11110101mmmm_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UQSAX_cccc01100110nnnndddd11110101mmmm_case_0TesterCase8
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
//       baseline: UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110111mmmm,
//       rule: UQSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0TesterCase9
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
//       baseline: UQADD8_cccc01100110nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11111001mmmm,
//       rule: UQADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQADD8_cccc01100110nnnndddd11111001mmmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  UQADD8_cccc01100110nnnndddd11111001mmmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UQADD8_cccc01100110nnnndddd11111001mmmm_case_0TesterCase10
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
//       baseline: UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11111111mmmm,
//       rule: UQSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0TesterCase11
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
//       baseline: UHADD16_cccc01100111nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110001mmmm,
//       rule: UHADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHADD16_cccc01100111nnnndddd11110001mmmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  UHADD16_cccc01100111nnnndddd11110001mmmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UHADD16_cccc01100111nnnndddd11110001mmmm_case_0TesterCase12
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
//       baseline: UHASX_cccc01100111nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110011mmmm,
//       rule: UHASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHASX_cccc01100111nnnndddd11110011mmmm_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  UHASX_cccc01100111nnnndddd11110011mmmm_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UHASX_cccc01100111nnnndddd11110011mmmm_case_0TesterCase13
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
//       baseline: UHSAX_cccc01100111nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110101mmmm,
//       rule: UHSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHSAX_cccc01100111nnnndddd11110101mmmm_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  UHSAX_cccc01100111nnnndddd11110101mmmm_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UHSAX_cccc01100111nnnndddd11110101mmmm_case_0TesterCase14
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
//       baseline: UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110111mmmm,
//       rule: UHSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0TesterCase15
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
//       baseline: UHADD8_cccc01100111nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11111001mmmm,
//       rule: UHADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHADD8_cccc01100111nnnndddd11111001mmmm_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  UHADD8_cccc01100111nnnndddd11111001mmmm_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UHADD8_cccc01100111nnnndddd11111001mmmm_case_0TesterCase16
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
//       baseline: UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11111111mmmm,
//       rule: UHSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0TesterCase17
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
//       baseline: UADD16_cccc01100101nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110001mmmm,
//       rule: UADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UADD16_cccc01100101nnnndddd11110001mmmm_case_0Tester_Case0
    : public UADD16_cccc01100101nnnndddd11110001mmmm_case_0TesterCase0 {
 public:
  UADD16_cccc01100101nnnndddd11110001mmmm_case_0Tester_Case0()
    : UADD16_cccc01100101nnnndddd11110001mmmm_case_0TesterCase0(
      state_.UADD16_cccc01100101nnnndddd11110001mmmm_case_0_UADD16_instance_)
  {}
};

// op1(21:20)=01 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UASX_cccc01100101nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110011mmmm,
//       rule: UASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UASX_cccc01100101nnnndddd11110011mmmm_case_0Tester_Case1
    : public UASX_cccc01100101nnnndddd11110011mmmm_case_0TesterCase1 {
 public:
  UASX_cccc01100101nnnndddd11110011mmmm_case_0Tester_Case1()
    : UASX_cccc01100101nnnndddd11110011mmmm_case_0TesterCase1(
      state_.UASX_cccc01100101nnnndddd11110011mmmm_case_0_UASX_instance_)
  {}
};

// op1(21:20)=01 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: USAX_cccc01100101nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110101mmmm,
//       rule: USAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class USAX_cccc01100101nnnndddd11110101mmmm_case_0Tester_Case2
    : public USAX_cccc01100101nnnndddd11110101mmmm_case_0TesterCase2 {
 public:
  USAX_cccc01100101nnnndddd11110101mmmm_case_0Tester_Case2()
    : USAX_cccc01100101nnnndddd11110101mmmm_case_0TesterCase2(
      state_.USAX_cccc01100101nnnndddd11110101mmmm_case_0_USAX_instance_)
  {}
};

// op1(21:20)=01 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: USUB16_cccc01100101nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110111mmmm,
//       rule: USUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class USUB16_cccc01100101nnnndddd11110111mmmm_case_0Tester_Case3
    : public USUB16_cccc01100101nnnndddd11110111mmmm_case_0TesterCase3 {
 public:
  USUB16_cccc01100101nnnndddd11110111mmmm_case_0Tester_Case3()
    : USUB16_cccc01100101nnnndddd11110111mmmm_case_0TesterCase3(
      state_.USUB16_cccc01100101nnnndddd11110111mmmm_case_0_USUB16_instance_)
  {}
};

// op1(21:20)=01 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UADD8_cccc01100101nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11111001mmmm,
//       rule: UADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UADD8_cccc01100101nnnndddd11111001mmmm_case_0Tester_Case4
    : public UADD8_cccc01100101nnnndddd11111001mmmm_case_0TesterCase4 {
 public:
  UADD8_cccc01100101nnnndddd11111001mmmm_case_0Tester_Case4()
    : UADD8_cccc01100101nnnndddd11111001mmmm_case_0TesterCase4(
      state_.UADD8_cccc01100101nnnndddd11111001mmmm_case_0_UADD8_instance_)
  {}
};

// op1(21:20)=01 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: USUB8_cccc01100101nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11111111mmmm,
//       rule: USUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class USUB8_cccc01100101nnnndddd11111111mmmm_case_0Tester_Case5
    : public USUB8_cccc01100101nnnndddd11111111mmmm_case_0TesterCase5 {
 public:
  USUB8_cccc01100101nnnndddd11111111mmmm_case_0Tester_Case5()
    : USUB8_cccc01100101nnnndddd11111111mmmm_case_0TesterCase5(
      state_.USUB8_cccc01100101nnnndddd11111111mmmm_case_0_USUB8_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQADD16_cccc01100110nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110001mmmm,
//       rule: UQADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQADD16_cccc01100110nnnndddd11110001mmmm_case_0Tester_Case6
    : public UQADD16_cccc01100110nnnndddd11110001mmmm_case_0TesterCase6 {
 public:
  UQADD16_cccc01100110nnnndddd11110001mmmm_case_0Tester_Case6()
    : UQADD16_cccc01100110nnnndddd11110001mmmm_case_0TesterCase6(
      state_.UQADD16_cccc01100110nnnndddd11110001mmmm_case_0_UQADD16_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQASX_cccc01100110nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110011mmmm,
//       rule: UQASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQASX_cccc01100110nnnndddd11110011mmmm_case_0Tester_Case7
    : public UQASX_cccc01100110nnnndddd11110011mmmm_case_0TesterCase7 {
 public:
  UQASX_cccc01100110nnnndddd11110011mmmm_case_0Tester_Case7()
    : UQASX_cccc01100110nnnndddd11110011mmmm_case_0TesterCase7(
      state_.UQASX_cccc01100110nnnndddd11110011mmmm_case_0_UQASX_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQSAX_cccc01100110nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110101mmmm,
//       rule: UQSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQSAX_cccc01100110nnnndddd11110101mmmm_case_0Tester_Case8
    : public UQSAX_cccc01100110nnnndddd11110101mmmm_case_0TesterCase8 {
 public:
  UQSAX_cccc01100110nnnndddd11110101mmmm_case_0Tester_Case8()
    : UQSAX_cccc01100110nnnndddd11110101mmmm_case_0TesterCase8(
      state_.UQSAX_cccc01100110nnnndddd11110101mmmm_case_0_UQSAX_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110111mmmm,
//       rule: UQSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0Tester_Case9
    : public UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0TesterCase9 {
 public:
  UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0Tester_Case9()
    : UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0TesterCase9(
      state_.UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0_UQSUB16_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQADD8_cccc01100110nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11111001mmmm,
//       rule: UQADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQADD8_cccc01100110nnnndddd11111001mmmm_case_0Tester_Case10
    : public UQADD8_cccc01100110nnnndddd11111001mmmm_case_0TesterCase10 {
 public:
  UQADD8_cccc01100110nnnndddd11111001mmmm_case_0Tester_Case10()
    : UQADD8_cccc01100110nnnndddd11111001mmmm_case_0TesterCase10(
      state_.UQADD8_cccc01100110nnnndddd11111001mmmm_case_0_UQADD8_instance_)
  {}
};

// op1(21:20)=10 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11111111mmmm,
//       rule: UQSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0Tester_Case11
    : public UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0TesterCase11 {
 public:
  UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0Tester_Case11()
    : UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0TesterCase11(
      state_.UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0_UQSUB8_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHADD16_cccc01100111nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110001mmmm,
//       rule: UHADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHADD16_cccc01100111nnnndddd11110001mmmm_case_0Tester_Case12
    : public UHADD16_cccc01100111nnnndddd11110001mmmm_case_0TesterCase12 {
 public:
  UHADD16_cccc01100111nnnndddd11110001mmmm_case_0Tester_Case12()
    : UHADD16_cccc01100111nnnndddd11110001mmmm_case_0TesterCase12(
      state_.UHADD16_cccc01100111nnnndddd11110001mmmm_case_0_UHADD16_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHASX_cccc01100111nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110011mmmm,
//       rule: UHASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHASX_cccc01100111nnnndddd11110011mmmm_case_0Tester_Case13
    : public UHASX_cccc01100111nnnndddd11110011mmmm_case_0TesterCase13 {
 public:
  UHASX_cccc01100111nnnndddd11110011mmmm_case_0Tester_Case13()
    : UHASX_cccc01100111nnnndddd11110011mmmm_case_0TesterCase13(
      state_.UHASX_cccc01100111nnnndddd11110011mmmm_case_0_UHASX_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHSAX_cccc01100111nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110101mmmm,
//       rule: UHSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHSAX_cccc01100111nnnndddd11110101mmmm_case_0Tester_Case14
    : public UHSAX_cccc01100111nnnndddd11110101mmmm_case_0TesterCase14 {
 public:
  UHSAX_cccc01100111nnnndddd11110101mmmm_case_0Tester_Case14()
    : UHSAX_cccc01100111nnnndddd11110101mmmm_case_0TesterCase14(
      state_.UHSAX_cccc01100111nnnndddd11110101mmmm_case_0_UHSAX_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110111mmmm,
//       rule: UHSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0Tester_Case15
    : public UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0TesterCase15 {
 public:
  UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0Tester_Case15()
    : UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0TesterCase15(
      state_.UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0_UHSUB16_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHADD8_cccc01100111nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11111001mmmm,
//       rule: UHADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHADD8_cccc01100111nnnndddd11111001mmmm_case_0Tester_Case16
    : public UHADD8_cccc01100111nnnndddd11111001mmmm_case_0TesterCase16 {
 public:
  UHADD8_cccc01100111nnnndddd11111001mmmm_case_0Tester_Case16()
    : UHADD8_cccc01100111nnnndddd11111001mmmm_case_0TesterCase16(
      state_.UHADD8_cccc01100111nnnndddd11111001mmmm_case_0_UHADD8_instance_)
  {}
};

// op1(21:20)=11 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11111111mmmm,
//       rule: UHSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0Tester_Case17
    : public UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0TesterCase17 {
 public:
  UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0Tester_Case17()
    : UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0TesterCase17(
      state_.UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0_UHSUB8_instance_)
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
//       baseline: UADD16_cccc01100101nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110001mmmm,
//       rule: UADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UADD16_cccc01100101nnnndddd11110001mmmm_case_0Tester_Case0_TestCase0) {
  UADD16_cccc01100101nnnndddd11110001mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UADD16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100101nnnndddd11110001mmmm");
}

// op1(21:20)=01 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UASX_cccc01100101nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110011mmmm,
//       rule: UASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UASX_cccc01100101nnnndddd11110011mmmm_case_0Tester_Case1_TestCase1) {
  UASX_cccc01100101nnnndddd11110011mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UASX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100101nnnndddd11110011mmmm");
}

// op1(21:20)=01 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: USAX_cccc01100101nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110101mmmm,
//       rule: USAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       USAX_cccc01100101nnnndddd11110101mmmm_case_0Tester_Case2_TestCase2) {
  USAX_cccc01100101nnnndddd11110101mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_USAX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100101nnnndddd11110101mmmm");
}

// op1(21:20)=01 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: USUB16_cccc01100101nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11110111mmmm,
//       rule: USUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       USUB16_cccc01100101nnnndddd11110111mmmm_case_0Tester_Case3_TestCase3) {
  USUB16_cccc01100101nnnndddd11110111mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_USUB16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100101nnnndddd11110111mmmm");
}

// op1(21:20)=01 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UADD8_cccc01100101nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11111001mmmm,
//       rule: UADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UADD8_cccc01100101nnnndddd11111001mmmm_case_0Tester_Case4_TestCase4) {
  UADD8_cccc01100101nnnndddd11111001mmmm_case_0Tester_Case4 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UADD8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100101nnnndddd11111001mmmm");
}

// op1(21:20)=01 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: USUB8_cccc01100101nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100101nnnndddd11111111mmmm,
//       rule: USUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       USUB8_cccc01100101nnnndddd11111111mmmm_case_0Tester_Case5_TestCase5) {
  USUB8_cccc01100101nnnndddd11111111mmmm_case_0Tester_Case5 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_USUB8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100101nnnndddd11111111mmmm");
}

// op1(21:20)=10 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQADD16_cccc01100110nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110001mmmm,
//       rule: UQADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UQADD16_cccc01100110nnnndddd11110001mmmm_case_0Tester_Case6_TestCase6) {
  UQADD16_cccc01100110nnnndddd11110001mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UQADD16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100110nnnndddd11110001mmmm");
}

// op1(21:20)=10 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQASX_cccc01100110nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110011mmmm,
//       rule: UQASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UQASX_cccc01100110nnnndddd11110011mmmm_case_0Tester_Case7_TestCase7) {
  UQASX_cccc01100110nnnndddd11110011mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UQASX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100110nnnndddd11110011mmmm");
}

// op1(21:20)=10 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQSAX_cccc01100110nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110101mmmm,
//       rule: UQSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UQSAX_cccc01100110nnnndddd11110101mmmm_case_0Tester_Case8_TestCase8) {
  UQSAX_cccc01100110nnnndddd11110101mmmm_case_0Tester_Case8 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UQSAX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100110nnnndddd11110101mmmm");
}

// op1(21:20)=10 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11110111mmmm,
//       rule: UQSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0Tester_Case9_TestCase9) {
  UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0Tester_Case9 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UQSUB16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100110nnnndddd11110111mmmm");
}

// op1(21:20)=10 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQADD8_cccc01100110nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11111001mmmm,
//       rule: UQADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UQADD8_cccc01100110nnnndddd11111001mmmm_case_0Tester_Case10_TestCase10) {
  UQADD8_cccc01100110nnnndddd11111001mmmm_case_0Tester_Case10 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UQADD8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100110nnnndddd11111001mmmm");
}

// op1(21:20)=10 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100110nnnndddd11111111mmmm,
//       rule: UQSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0Tester_Case11_TestCase11) {
  UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0Tester_Case11 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UQSUB8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100110nnnndddd11111111mmmm");
}

// op1(21:20)=11 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHADD16_cccc01100111nnnndddd11110001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110001mmmm,
//       rule: UHADD16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UHADD16_cccc01100111nnnndddd11110001mmmm_case_0Tester_Case12_TestCase12) {
  UHADD16_cccc01100111nnnndddd11110001mmmm_case_0Tester_Case12 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UHADD16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100111nnnndddd11110001mmmm");
}

// op1(21:20)=11 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHASX_cccc01100111nnnndddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110011mmmm,
//       rule: UHASX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UHASX_cccc01100111nnnndddd11110011mmmm_case_0Tester_Case13_TestCase13) {
  UHASX_cccc01100111nnnndddd11110011mmmm_case_0Tester_Case13 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UHASX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100111nnnndddd11110011mmmm");
}

// op1(21:20)=11 & op2(7:5)=010 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHSAX_cccc01100111nnnndddd11110101mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110101mmmm,
//       rule: UHSAX,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UHSAX_cccc01100111nnnndddd11110101mmmm_case_0Tester_Case14_TestCase14) {
  UHSAX_cccc01100111nnnndddd11110101mmmm_case_0Tester_Case14 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UHSAX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100111nnnndddd11110101mmmm");
}

// op1(21:20)=11 & op2(7:5)=011 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11110111mmmm,
//       rule: UHSUB16,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0Tester_Case15_TestCase15) {
  UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0Tester_Case15 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UHSUB16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100111nnnndddd11110111mmmm");
}

// op1(21:20)=11 & op2(7:5)=100 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHADD8_cccc01100111nnnndddd11111001mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11111001mmmm,
//       rule: UHADD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UHADD8_cccc01100111nnnndddd11111001mmmm_case_0Tester_Case16_TestCase16) {
  UHADD8_cccc01100111nnnndddd11111001mmmm_case_0Tester_Case16 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UHADD8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100111nnnndddd11111001mmmm");
}

// op1(21:20)=11 & op2(7:5)=111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01100111nnnndddd11111111mmmm,
//       rule: UHSUB8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0Tester_Case17_TestCase17) {
  UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0Tester_Case17 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_UHSUB8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01100111nnnndddd11111111mmmm");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
