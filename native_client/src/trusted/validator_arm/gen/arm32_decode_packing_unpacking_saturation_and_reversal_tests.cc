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


// op1(22:20)=000 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101000nnnnddddrr000111mmmm,
//       rule: SXTAB16,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~000
  if ((inst.Bits() & 0x00700000)  !=
          0x00000000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=000 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SXTB16_cccc011010001111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010001111ddddrr000111mmmm,
//       rule: SXTB16,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class SXTB16_cccc011010001111ddddrr000111mmmm_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  SXTB16_cccc011010001111ddddrr000111mmmm_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SXTB16_cccc011010001111ddddrr000111mmmm_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~000
  if ((inst.Bits() & 0x00700000)  !=
          0x00000000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=000 & op2(7:5)=101 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SEL_cccc01101000nnnndddd11111011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101000nnnndddd11111011mmmm,
//       rule: SEL,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SEL_cccc01101000nnnndddd11111011mmmm_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  SEL_cccc01101000nnnndddd11111011mmmm_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SEL_cccc01101000nnnndddd11111011mmmm_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~000
  if ((inst.Bits() & 0x00700000)  !=
          0x00000000) return false;
  // op2(7:5)=~101
  if ((inst.Bits() & 0x000000E0)  !=
          0x000000A0) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
  if ((inst.Bits() & 0x00000F00)  !=
          0x00000F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=000 & op2(7:5)=xx0
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101000nnnnddddiiiiit01mmmm,
//       rule: PKH,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~000
  if ((inst.Bits() & 0x00700000)  !=
          0x00000000) return false;
  // op2(7:5)=~xx0
  if ((inst.Bits() & 0x00000020)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=010 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SSAT16_cccc01101010iiiidddd11110011nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc01101010iiiidddd11110011nnnn,
//       rule: SSAT16,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
class SSAT16_cccc01101010iiiidddd11110011nnnn_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  SSAT16_cccc01101010iiiidddd11110011nnnn_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SSAT16_cccc01101010iiiidddd11110011nnnn_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~010
  if ((inst.Bits() & 0x00700000)  !=
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

// op1(22:20)=010 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101010nnnnddddrr000111mmmm,
//       rule: SXTAB,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~010
  if ((inst.Bits() & 0x00700000)  !=
          0x00200000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=010 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SXTB_cccc011010101111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010101111ddddrr000111mmmm,
//       rule: SXTB,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class SXTB_cccc011010101111ddddrr000111mmmm_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  SXTB_cccc011010101111ddddrr000111mmmm_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SXTB_cccc011010101111ddddrr000111mmmm_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~010
  if ((inst.Bits() & 0x00700000)  !=
          0x00200000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=011 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: REV_cccc011010111111dddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010111111dddd11110011mmmm,
//       rule: REV,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class REV_cccc011010111111dddd11110011mmmm_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  REV_cccc011010111111dddd11110011mmmm_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool REV_cccc011010111111dddd11110011mmmm_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~011
  if ((inst.Bits() & 0x00700000)  !=
          0x00300000) return false;
  // op2(7:5)=~001
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000020) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx1111xxxx1111xxxxxxxx
  if ((inst.Bits() & 0x000F0F00)  !=
          0x000F0F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=011 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101011nnnnddddrr000111mmmm,
//       rule: SXTAH,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~011
  if ((inst.Bits() & 0x00700000)  !=
          0x00300000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=011 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SXTH_cccc011010111111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010111111ddddrr000111mmmm,
//       rule: SXTH,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class SXTH_cccc011010111111ddddrr000111mmmm_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  SXTH_cccc011010111111ddddrr000111mmmm_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SXTH_cccc011010111111ddddrr000111mmmm_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~011
  if ((inst.Bits() & 0x00700000)  !=
          0x00300000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=011 & op2(7:5)=101 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: REV16_cccc011010111111dddd11111011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010111111dddd11111011mmmm,
//       rule: REV16,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class REV16_cccc011010111111dddd11111011mmmm_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  REV16_cccc011010111111dddd11111011mmmm_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool REV16_cccc011010111111dddd11111011mmmm_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~011
  if ((inst.Bits() & 0x00700000)  !=
          0x00300000) return false;
  // op2(7:5)=~101
  if ((inst.Bits() & 0x000000E0)  !=
          0x000000A0) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx1111xxxx1111xxxxxxxx
  if ((inst.Bits() & 0x000F0F00)  !=
          0x000F0F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=100 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101100nnnnddddrr000111mmmm,
//       rule: UXTAB16,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~100
  if ((inst.Bits() & 0x00700000)  !=
          0x00400000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=100 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: UXTB16_cccc011011001111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011001111ddddrr000111mmmm,
//       rule: UXTB16,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class UXTB16_cccc011011001111ddddrr000111mmmm_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  UXTB16_cccc011011001111ddddrr000111mmmm_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UXTB16_cccc011011001111ddddrr000111mmmm_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~100
  if ((inst.Bits() & 0x00700000)  !=
          0x00400000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=110 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: USAT16_cccc01101110iiiidddd11110011nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc01101110iiiidddd11110011nnnn,
//       rule: USAT16,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
class USAT16_cccc01101110iiiidddd11110011nnnn_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  USAT16_cccc01101110iiiidddd11110011nnnn_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool USAT16_cccc01101110iiiidddd11110011nnnn_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~110
  if ((inst.Bits() & 0x00700000)  !=
          0x00600000) return false;
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

// op1(22:20)=110 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101110nnnnddddrr000111mmmm,
//       rule: UXTAB,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~110
  if ((inst.Bits() & 0x00700000)  !=
          0x00600000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=110 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: UXTB_cccc011011101111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011101111ddddrr000111mmmm,
//       rule: UXTB,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class UXTB_cccc011011101111ddddrr000111mmmm_case_0TesterCase15
    : public Arm32DecoderTester {
 public:
  UXTB_cccc011011101111ddddrr000111mmmm_case_0TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UXTB_cccc011011101111ddddrr000111mmmm_case_0TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~110
  if ((inst.Bits() & 0x00700000)  !=
          0x00600000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=111 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: RBIT_cccc011011111111dddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011111111dddd11110011mmmm,
//       rule: RBIT,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class RBIT_cccc011011111111dddd11110011mmmm_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  RBIT_cccc011011111111dddd11110011mmmm_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool RBIT_cccc011011111111dddd11110011mmmm_case_0TesterCase16
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~111
  if ((inst.Bits() & 0x00700000)  !=
          0x00700000) return false;
  // op2(7:5)=~001
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000020) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx1111xxxx1111xxxxxxxx
  if ((inst.Bits() & 0x000F0F00)  !=
          0x000F0F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=111 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101111nnnnddddrr000111mmmm,
//       rule: UXTAH,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0TesterCase17
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~111
  if ((inst.Bits() & 0x00700000)  !=
          0x00700000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=111 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: UXTH_cccc011011111111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011111111ddddrr000111mmmm,
//       rule: UXTH,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class UXTH_cccc011011111111ddddrr000111mmmm_case_0TesterCase18
    : public Arm32DecoderTester {
 public:
  UXTH_cccc011011111111ddddrr000111mmmm_case_0TesterCase18(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UXTH_cccc011011111111ddddrr000111mmmm_case_0TesterCase18
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~111
  if ((inst.Bits() & 0x00700000)  !=
          0x00700000) return false;
  // op2(7:5)=~011
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000060) return false;
  // A(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
  if ((inst.Bits() & 0x00000300)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=111 & op2(7:5)=101 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: REVSH_cccc011011111111dddd11111011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011111111dddd11111011mmmm,
//       rule: REVSH,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class REVSH_cccc011011111111dddd11111011mmmm_case_0TesterCase19
    : public Arm32DecoderTester {
 public:
  REVSH_cccc011011111111dddd11111011mmmm_case_0TesterCase19(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool REVSH_cccc011011111111dddd11111011mmmm_case_0TesterCase19
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~111
  if ((inst.Bits() & 0x00700000)  !=
          0x00700000) return false;
  // op2(7:5)=~101
  if ((inst.Bits() & 0x000000E0)  !=
          0x000000A0) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx1111xxxx1111xxxxxxxx
  if ((inst.Bits() & 0x000F0F00)  !=
          0x000F0F00) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=01x & op2(7:5)=xx0
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc0110101iiiiiddddiiiiis01nnnn,
//       rule: SSAT,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
class SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0TesterCase20
    : public Arm32DecoderTester {
 public:
  SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0TesterCase20(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0TesterCase20
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~01x
  if ((inst.Bits() & 0x00600000)  !=
          0x00200000) return false;
  // op2(7:5)=~xx0
  if ((inst.Bits() & 0x00000020)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=11x & op2(7:5)=xx0
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc0110111iiiiiddddiiiiis01nnnn,
//       rule: USAT,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
class USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0TesterCase21
    : public Arm32DecoderTester {
 public:
  USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0TesterCase21(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0TesterCase21
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~11x
  if ((inst.Bits() & 0x00600000)  !=
          0x00600000) return false;
  // op2(7:5)=~xx0
  if ((inst.Bits() & 0x00000020)  !=
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

// op1(22:20)=000 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101000nnnnddddrr000111mmmm,
//       rule: SXTAB16,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0Tester_Case0
    : public SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0TesterCase0 {
 public:
  SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0Tester_Case0()
    : SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0TesterCase0(
      state_.SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0_SXTAB16_instance_)
  {}
};

// op1(22:20)=000 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SXTB16_cccc011010001111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010001111ddddrr000111mmmm,
//       rule: SXTB16,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class SXTB16_cccc011010001111ddddrr000111mmmm_case_0Tester_Case1
    : public SXTB16_cccc011010001111ddddrr000111mmmm_case_0TesterCase1 {
 public:
  SXTB16_cccc011010001111ddddrr000111mmmm_case_0Tester_Case1()
    : SXTB16_cccc011010001111ddddrr000111mmmm_case_0TesterCase1(
      state_.SXTB16_cccc011010001111ddddrr000111mmmm_case_0_SXTB16_instance_)
  {}
};

// op1(22:20)=000 & op2(7:5)=101 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SEL_cccc01101000nnnndddd11111011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101000nnnndddd11111011mmmm,
//       rule: SEL,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SEL_cccc01101000nnnndddd11111011mmmm_case_0Tester_Case2
    : public SEL_cccc01101000nnnndddd11111011mmmm_case_0TesterCase2 {
 public:
  SEL_cccc01101000nnnndddd11111011mmmm_case_0Tester_Case2()
    : SEL_cccc01101000nnnndddd11111011mmmm_case_0TesterCase2(
      state_.SEL_cccc01101000nnnndddd11111011mmmm_case_0_SEL_instance_)
  {}
};

// op1(22:20)=000 & op2(7:5)=xx0
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101000nnnnddddiiiiit01mmmm,
//       rule: PKH,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0Tester_Case3
    : public PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0TesterCase3 {
 public:
  PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0Tester_Case3()
    : PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0TesterCase3(
      state_.PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0_PKH_instance_)
  {}
};

// op1(22:20)=010 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SSAT16_cccc01101010iiiidddd11110011nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc01101010iiiidddd11110011nnnn,
//       rule: SSAT16,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
class SSAT16_cccc01101010iiiidddd11110011nnnn_case_0Tester_Case4
    : public SSAT16_cccc01101010iiiidddd11110011nnnn_case_0TesterCase4 {
 public:
  SSAT16_cccc01101010iiiidddd11110011nnnn_case_0Tester_Case4()
    : SSAT16_cccc01101010iiiidddd11110011nnnn_case_0TesterCase4(
      state_.SSAT16_cccc01101010iiiidddd11110011nnnn_case_0_SSAT16_instance_)
  {}
};

// op1(22:20)=010 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101010nnnnddddrr000111mmmm,
//       rule: SXTAB,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0Tester_Case5
    : public SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0TesterCase5 {
 public:
  SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0Tester_Case5()
    : SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0TesterCase5(
      state_.SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0_SXTAB_instance_)
  {}
};

// op1(22:20)=010 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SXTB_cccc011010101111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010101111ddddrr000111mmmm,
//       rule: SXTB,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class SXTB_cccc011010101111ddddrr000111mmmm_case_0Tester_Case6
    : public SXTB_cccc011010101111ddddrr000111mmmm_case_0TesterCase6 {
 public:
  SXTB_cccc011010101111ddddrr000111mmmm_case_0Tester_Case6()
    : SXTB_cccc011010101111ddddrr000111mmmm_case_0TesterCase6(
      state_.SXTB_cccc011010101111ddddrr000111mmmm_case_0_SXTB_instance_)
  {}
};

// op1(22:20)=011 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: REV_cccc011010111111dddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010111111dddd11110011mmmm,
//       rule: REV,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class REV_cccc011010111111dddd11110011mmmm_case_0Tester_Case7
    : public REV_cccc011010111111dddd11110011mmmm_case_0TesterCase7 {
 public:
  REV_cccc011010111111dddd11110011mmmm_case_0Tester_Case7()
    : REV_cccc011010111111dddd11110011mmmm_case_0TesterCase7(
      state_.REV_cccc011010111111dddd11110011mmmm_case_0_REV_instance_)
  {}
};

// op1(22:20)=011 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101011nnnnddddrr000111mmmm,
//       rule: SXTAH,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0Tester_Case8
    : public SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0TesterCase8 {
 public:
  SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0Tester_Case8()
    : SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0TesterCase8(
      state_.SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0_SXTAH_instance_)
  {}
};

// op1(22:20)=011 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SXTH_cccc011010111111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010111111ddddrr000111mmmm,
//       rule: SXTH,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class SXTH_cccc011010111111ddddrr000111mmmm_case_0Tester_Case9
    : public SXTH_cccc011010111111ddddrr000111mmmm_case_0TesterCase9 {
 public:
  SXTH_cccc011010111111ddddrr000111mmmm_case_0Tester_Case9()
    : SXTH_cccc011010111111ddddrr000111mmmm_case_0TesterCase9(
      state_.SXTH_cccc011010111111ddddrr000111mmmm_case_0_SXTH_instance_)
  {}
};

// op1(22:20)=011 & op2(7:5)=101 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: REV16_cccc011010111111dddd11111011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010111111dddd11111011mmmm,
//       rule: REV16,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class REV16_cccc011010111111dddd11111011mmmm_case_0Tester_Case10
    : public REV16_cccc011010111111dddd11111011mmmm_case_0TesterCase10 {
 public:
  REV16_cccc011010111111dddd11111011mmmm_case_0Tester_Case10()
    : REV16_cccc011010111111dddd11111011mmmm_case_0TesterCase10(
      state_.REV16_cccc011010111111dddd11111011mmmm_case_0_REV16_instance_)
  {}
};

// op1(22:20)=100 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101100nnnnddddrr000111mmmm,
//       rule: UXTAB16,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0Tester_Case11
    : public UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0TesterCase11 {
 public:
  UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0Tester_Case11()
    : UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0TesterCase11(
      state_.UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0_UXTAB16_instance_)
  {}
};

// op1(22:20)=100 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: UXTB16_cccc011011001111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011001111ddddrr000111mmmm,
//       rule: UXTB16,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class UXTB16_cccc011011001111ddddrr000111mmmm_case_0Tester_Case12
    : public UXTB16_cccc011011001111ddddrr000111mmmm_case_0TesterCase12 {
 public:
  UXTB16_cccc011011001111ddddrr000111mmmm_case_0Tester_Case12()
    : UXTB16_cccc011011001111ddddrr000111mmmm_case_0TesterCase12(
      state_.UXTB16_cccc011011001111ddddrr000111mmmm_case_0_UXTB16_instance_)
  {}
};

// op1(22:20)=110 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: USAT16_cccc01101110iiiidddd11110011nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc01101110iiiidddd11110011nnnn,
//       rule: USAT16,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
class USAT16_cccc01101110iiiidddd11110011nnnn_case_0Tester_Case13
    : public USAT16_cccc01101110iiiidddd11110011nnnn_case_0TesterCase13 {
 public:
  USAT16_cccc01101110iiiidddd11110011nnnn_case_0Tester_Case13()
    : USAT16_cccc01101110iiiidddd11110011nnnn_case_0TesterCase13(
      state_.USAT16_cccc01101110iiiidddd11110011nnnn_case_0_USAT16_instance_)
  {}
};

// op1(22:20)=110 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101110nnnnddddrr000111mmmm,
//       rule: UXTAB,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0Tester_Case14
    : public UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0TesterCase14 {
 public:
  UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0Tester_Case14()
    : UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0TesterCase14(
      state_.UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0_UXTAB_instance_)
  {}
};

// op1(22:20)=110 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: UXTB_cccc011011101111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011101111ddddrr000111mmmm,
//       rule: UXTB,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class UXTB_cccc011011101111ddddrr000111mmmm_case_0Tester_Case15
    : public UXTB_cccc011011101111ddddrr000111mmmm_case_0TesterCase15 {
 public:
  UXTB_cccc011011101111ddddrr000111mmmm_case_0Tester_Case15()
    : UXTB_cccc011011101111ddddrr000111mmmm_case_0TesterCase15(
      state_.UXTB_cccc011011101111ddddrr000111mmmm_case_0_UXTB_instance_)
  {}
};

// op1(22:20)=111 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: RBIT_cccc011011111111dddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011111111dddd11110011mmmm,
//       rule: RBIT,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class RBIT_cccc011011111111dddd11110011mmmm_case_0Tester_Case16
    : public RBIT_cccc011011111111dddd11110011mmmm_case_0TesterCase16 {
 public:
  RBIT_cccc011011111111dddd11110011mmmm_case_0Tester_Case16()
    : RBIT_cccc011011111111dddd11110011mmmm_case_0TesterCase16(
      state_.RBIT_cccc011011111111dddd11110011mmmm_case_0_RBIT_instance_)
  {}
};

// op1(22:20)=111 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101111nnnnddddrr000111mmmm,
//       rule: UXTAH,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0Tester_Case17
    : public UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0TesterCase17 {
 public:
  UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0Tester_Case17()
    : UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0TesterCase17(
      state_.UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0_UXTAH_instance_)
  {}
};

// op1(22:20)=111 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: UXTH_cccc011011111111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011111111ddddrr000111mmmm,
//       rule: UXTH,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class UXTH_cccc011011111111ddddrr000111mmmm_case_0Tester_Case18
    : public UXTH_cccc011011111111ddddrr000111mmmm_case_0TesterCase18 {
 public:
  UXTH_cccc011011111111ddddrr000111mmmm_case_0Tester_Case18()
    : UXTH_cccc011011111111ddddrr000111mmmm_case_0TesterCase18(
      state_.UXTH_cccc011011111111ddddrr000111mmmm_case_0_UXTH_instance_)
  {}
};

// op1(22:20)=111 & op2(7:5)=101 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: REVSH_cccc011011111111dddd11111011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011111111dddd11111011mmmm,
//       rule: REVSH,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
class REVSH_cccc011011111111dddd11111011mmmm_case_0Tester_Case19
    : public REVSH_cccc011011111111dddd11111011mmmm_case_0TesterCase19 {
 public:
  REVSH_cccc011011111111dddd11111011mmmm_case_0Tester_Case19()
    : REVSH_cccc011011111111dddd11111011mmmm_case_0TesterCase19(
      state_.REVSH_cccc011011111111dddd11111011mmmm_case_0_REVSH_instance_)
  {}
};

// op1(22:20)=01x & op2(7:5)=xx0
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc0110101iiiiiddddiiiiis01nnnn,
//       rule: SSAT,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
class SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0Tester_Case20
    : public SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0TesterCase20 {
 public:
  SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0Tester_Case20()
    : SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0TesterCase20(
      state_.SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0_SSAT_instance_)
  {}
};

// op1(22:20)=11x & op2(7:5)=xx0
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc0110111iiiiiddddiiiiis01nnnn,
//       rule: USAT,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
class USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0Tester_Case21
    : public USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0TesterCase21 {
 public:
  USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0Tester_Case21()
    : USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0TesterCase21(
      state_.USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0_USAT_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op1(22:20)=000 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101000nnnnddddrr000111mmmm,
//       rule: SXTAB16,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0Tester_Case0_TestCase0) {
  SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0Tester_Case0 baseline_tester;
  NamedActual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1_SXTAB16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01101000nnnnddddrr000111mmmm");
}

// op1(22:20)=000 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SXTB16_cccc011010001111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010001111ddddrr000111mmmm,
//       rule: SXTB16,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
TEST_F(Arm32DecoderStateTests,
       SXTB16_cccc011010001111ddddrr000111mmmm_case_0Tester_Case1_TestCase1) {
  SXTB16_cccc011010001111ddddrr000111mmmm_case_0Tester_Case1 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_SXTB16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011010001111ddddrr000111mmmm");
}

// op1(22:20)=000 & op2(7:5)=101 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: SEL_cccc01101000nnnndddd11111011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101000nnnndddd11111011mmmm,
//       rule: SEL,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SEL_cccc01101000nnnndddd11111011mmmm_case_0Tester_Case2_TestCase2) {
  SEL_cccc01101000nnnndddd11111011mmmm_case_0Tester_Case2 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_SEL actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01101000nnnndddd11111011mmmm");
}

// op1(22:20)=000 & op2(7:5)=xx0
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//       baseline: PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101000nnnnddddiiiiit01mmmm,
//       rule: PKH,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0Tester_Case3_TestCase3) {
  PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0Tester_Case3 baseline_tester;
  NamedActual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_PKH actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01101000nnnnddddiiiiit01mmmm");
}

// op1(22:20)=010 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SSAT16_cccc01101010iiiidddd11110011nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc01101010iiiidddd11110011nnnn,
//       rule: SSAT16,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       SSAT16_cccc01101010iiiidddd11110011nnnn_case_0Tester_Case4_TestCase4) {
  SSAT16_cccc01101010iiiidddd11110011nnnn_case_0Tester_Case4 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_SSAT16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01101010iiiidddd11110011nnnn");
}

// op1(22:20)=010 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101010nnnnddddrr000111mmmm,
//       rule: SXTAB,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0Tester_Case5_TestCase5) {
  SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0Tester_Case5 baseline_tester;
  NamedActual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1_SXTAB actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01101010nnnnddddrr000111mmmm");
}

// op1(22:20)=010 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SXTB_cccc011010101111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010101111ddddrr000111mmmm,
//       rule: SXTB,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
TEST_F(Arm32DecoderStateTests,
       SXTB_cccc011010101111ddddrr000111mmmm_case_0Tester_Case6_TestCase6) {
  SXTB_cccc011010101111ddddrr000111mmmm_case_0Tester_Case6 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_SXTB actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011010101111ddddrr000111mmmm");
}

// op1(22:20)=011 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: REV_cccc011010111111dddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010111111dddd11110011mmmm,
//       rule: REV,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
TEST_F(Arm32DecoderStateTests,
       REV_cccc011010111111dddd11110011mmmm_case_0Tester_Case7_TestCase7) {
  REV_cccc011010111111dddd11110011mmmm_case_0Tester_Case7 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_REV actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011010111111dddd11110011mmmm");
}

// op1(22:20)=011 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101011nnnnddddrr000111mmmm,
//       rule: SXTAH,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0Tester_Case8_TestCase8) {
  SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0Tester_Case8 baseline_tester;
  NamedActual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1_SXTAH actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01101011nnnnddddrr000111mmmm");
}

// op1(22:20)=011 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SXTH_cccc011010111111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010111111ddddrr000111mmmm,
//       rule: SXTH,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
TEST_F(Arm32DecoderStateTests,
       SXTH_cccc011010111111ddddrr000111mmmm_case_0Tester_Case9_TestCase9) {
  SXTH_cccc011010111111ddddrr000111mmmm_case_0Tester_Case9 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_SXTH actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011010111111ddddrr000111mmmm");
}

// op1(22:20)=011 & op2(7:5)=101 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: REV16_cccc011010111111dddd11111011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011010111111dddd11111011mmmm,
//       rule: REV16,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
TEST_F(Arm32DecoderStateTests,
       REV16_cccc011010111111dddd11111011mmmm_case_0Tester_Case10_TestCase10) {
  REV16_cccc011010111111dddd11111011mmmm_case_0Tester_Case10 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_REV16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011010111111dddd11111011mmmm");
}

// op1(22:20)=100 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101100nnnnddddrr000111mmmm,
//       rule: UXTAB16,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0Tester_Case11_TestCase11) {
  UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0Tester_Case11 baseline_tester;
  NamedActual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1_UXTAB16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01101100nnnnddddrr000111mmmm");
}

// op1(22:20)=100 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: UXTB16_cccc011011001111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011001111ddddrr000111mmmm,
//       rule: UXTB16,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
TEST_F(Arm32DecoderStateTests,
       UXTB16_cccc011011001111ddddrr000111mmmm_case_0Tester_Case12_TestCase12) {
  UXTB16_cccc011011001111ddddrr000111mmmm_case_0Tester_Case12 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_UXTB16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011011001111ddddrr000111mmmm");
}

// op1(22:20)=110 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: USAT16_cccc01101110iiiidddd11110011nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc01101110iiiidddd11110011nnnn,
//       rule: USAT16,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       USAT16_cccc01101110iiiidddd11110011nnnn_case_0Tester_Case13_TestCase13) {
  USAT16_cccc01101110iiiidddd11110011nnnn_case_0Tester_Case13 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_USAT16 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01101110iiiidddd11110011nnnn");
}

// op1(22:20)=110 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101110nnnnddddrr000111mmmm,
//       rule: UXTAB,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0Tester_Case14_TestCase14) {
  UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0Tester_Case14 baseline_tester;
  NamedActual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1_UXTAB actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01101110nnnnddddrr000111mmmm");
}

// op1(22:20)=110 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: UXTB_cccc011011101111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011101111ddddrr000111mmmm,
//       rule: UXTB,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
TEST_F(Arm32DecoderStateTests,
       UXTB_cccc011011101111ddddrr000111mmmm_case_0Tester_Case15_TestCase15) {
  UXTB_cccc011011101111ddddrr000111mmmm_case_0Tester_Case15 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_UXTB actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011011101111ddddrr000111mmmm");
}

// op1(22:20)=111 & op2(7:5)=001 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: RBIT_cccc011011111111dddd11110011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011111111dddd11110011mmmm,
//       rule: RBIT,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
TEST_F(Arm32DecoderStateTests,
       RBIT_cccc011011111111dddd11110011mmmm_case_0Tester_Case16_TestCase16) {
  RBIT_cccc011011111111dddd11110011mmmm_case_0Tester_Case16 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_RBIT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011011111111dddd11110011mmmm");
}

// op1(22:20)=111 & op2(7:5)=011 & A(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//       baseline: UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rn(19:16), Rd(15:12), Rm(3:0)],
//       pattern: cccc01101111nnnnddddrr000111mmmm,
//       rule: UXTAH,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR,
//         Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0Tester_Case17_TestCase17) {
  UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0Tester_Case17 baseline_tester;
  NamedActual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1_UXTAH actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01101111nnnnddddrr000111mmmm");
}

// op1(22:20)=111 & op2(7:5)=011 & A(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxxxxxxxx00xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: UXTH_cccc011011111111ddddrr000111mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011111111ddddrr000111mmmm,
//       rule: UXTH,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
TEST_F(Arm32DecoderStateTests,
       UXTH_cccc011011111111ddddrr000111mmmm_case_0Tester_Case18_TestCase18) {
  UXTH_cccc011011111111ddddrr000111mmmm_case_0Tester_Case18 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_UXTH actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011011111111ddddrr000111mmmm");
}

// op1(22:20)=111 & op2(7:5)=101 & $pattern(31:0)=xxxxxxxxxxxx1111xxxx1111xxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rm: Rm(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: REVSH_cccc011011111111dddd11111011mmmm_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rm(3:0)],
//       pattern: cccc011011111111dddd11111011mmmm,
//       rule: REVSH,
//       safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//       uses: {Rm}}
TEST_F(Arm32DecoderStateTests,
       REVSH_cccc011011111111dddd11111011mmmm_case_0Tester_Case19_TestCase19) {
  REVSH_cccc011011111111dddd11111011mmmm_case_0Tester_Case19 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_REVSH actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc011011111111dddd11111011mmmm");
}

// op1(22:20)=01x & op2(7:5)=xx0
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc0110101iiiiiddddiiiiis01nnnn,
//       rule: SSAT,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0Tester_Case20_TestCase20) {
  SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0Tester_Case20 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_SSAT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0110101iiiiiddddiiiiis01nnnn");
}

// op1(22:20)=11x & op2(7:5)=xx0
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//       baseline: USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(15:12), Rn(3:0)],
//       pattern: cccc0110111iiiiiddddiiiiis01nnnn,
//       rule: USAT,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//       uses: {Rn}}
TEST_F(Arm32DecoderStateTests,
       USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0Tester_Case21_TestCase21) {
  USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0Tester_Case21 baseline_tester;
  NamedActual_CLZ_cccc000101101111dddd11110001mmmm_case_1_USAT actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0110111iiiiiddddiiiiis01nnnn");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
