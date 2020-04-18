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


// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01111000ddddaaaammmm0001nnnn,
//       rule: USADA8,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~11000
  if ((inst.Bits() & 0x01F00000)  !=
          0x01800000) return false;
  // op2(7:5)=~000
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000000) return false;
  // Rd(15:12)=1111
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//       baseline: USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01111000dddd1111mmmm0001nnnn,
//       rule: USAD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~11000
  if ((inst.Bits() & 0x01F00000)  !=
          0x01800000) return false;
  // op2(7:5)=~000
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000000) return false;
  // Rd(15:12)=~1111
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=11111 & op2(7:5)=111
//    = {actual: Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1,
//       baseline: UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0,
//       defs: {},
//       inst: inst,
//       pattern: cccc01111111iiiiiiiiiiii1111iiii,
//       rule: UDF,
//       safety: [not IsUDFNaClSafe(inst) => FORBIDDEN_OPERANDS],
//       uses: {}}
class UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~11111
  if ((inst.Bits() & 0x01F00000)  !=
          0x01F00000) return false;
  // op2(7:5)=~111
  if ((inst.Bits() & 0x000000E0)  !=
          0x000000E0) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1101x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1,
//       baseline: SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       lsb: lsb(11:7),
//       pattern: cccc0111101wwwwwddddlllll101nnnn,
//       rule: SBFX,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
class SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1101x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01A00000) return false;
  // op2(7:5)=~x10
  if ((inst.Bits() & 0x00000060)  !=
          0x00000040) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=~1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1,
//       baseline: BFI_cccc0111110mmmmmddddlllll001nnnn_case_0,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       pattern: cccc0111110mmmmmddddlllll001nnnn,
//       rule: BFI,
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rn, Rd}}
class BFI_cccc0111110mmmmmddddlllll001nnnn_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  BFI_cccc0111110mmmmmddddlllll001nnnn_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool BFI_cccc0111110mmmmmddddlllll001nnnn_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1110x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01C00000) return false;
  // op2(7:5)=~x00
  if ((inst.Bits() & 0x00000060)  !=
          0x00000000) return false;
  // Rn(3:0)=1111
  if ((inst.Bits() & 0x0000000F)  ==
          0x0000000F) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1,
//       baseline: BFC_cccc0111110mmmmmddddlllll0011111_case_0,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7)],
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       pattern: cccc0111110mmmmmddddlllll0011111,
//       rule: BFC,
//       safety: [Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rd}}
class BFC_cccc0111110mmmmmddddlllll0011111_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  BFC_cccc0111110mmmmmddddlllll0011111_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool BFC_cccc0111110mmmmmddddlllll0011111_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1110x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01C00000) return false;
  // op2(7:5)=~x00
  if ((inst.Bits() & 0x00000060)  !=
          0x00000000) return false;
  // Rn(3:0)=~1111
  if ((inst.Bits() & 0x0000000F)  !=
          0x0000000F) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1111x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1,
//       baseline: UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       lsb: lsb(11:7),
//       pattern: cccc0111111mmmmmddddlllll101nnnn,
//       rule: UBFX,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
class UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1111x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01E00000) return false;
  // op2(7:5)=~x10
  if ((inst.Bits() & 0x00000060)  !=
          0x00000040) return false;

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

// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01111000ddddaaaammmm0001nnnn,
//       rule: USADA8,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0Tester_Case0
    : public USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0TesterCase0 {
 public:
  USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0Tester_Case0()
    : USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0TesterCase0(
      state_.USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0_USADA8_instance_)
  {}
};

// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//       baseline: USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01111000dddd1111mmmm0001nnnn,
//       rule: USAD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0Tester_Case1
    : public USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0TesterCase1 {
 public:
  USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0Tester_Case1()
    : USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0TesterCase1(
      state_.USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0_USAD8_instance_)
  {}
};

// op1(24:20)=11111 & op2(7:5)=111
//    = {actual: Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1,
//       baseline: UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0,
//       defs: {},
//       inst: inst,
//       pattern: cccc01111111iiiiiiiiiiii1111iiii,
//       rule: UDF,
//       safety: [not IsUDFNaClSafe(inst) => FORBIDDEN_OPERANDS],
//       uses: {}}
class UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0Tester_Case2
    : public UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0TesterCase2 {
 public:
  UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0Tester_Case2()
    : UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0TesterCase2(
      state_.UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0_UDF_instance_)
  {}
};

// op1(24:20)=1101x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1,
//       baseline: SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       lsb: lsb(11:7),
//       pattern: cccc0111101wwwwwddddlllll101nnnn,
//       rule: SBFX,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
class SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0Tester_Case3
    : public SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0TesterCase3 {
 public:
  SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0Tester_Case3()
    : SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0TesterCase3(
      state_.SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0_SBFX_instance_)
  {}
};

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=~1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1,
//       baseline: BFI_cccc0111110mmmmmddddlllll001nnnn_case_0,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       pattern: cccc0111110mmmmmddddlllll001nnnn,
//       rule: BFI,
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rn, Rd}}
class BFI_cccc0111110mmmmmddddlllll001nnnn_case_0Tester_Case4
    : public BFI_cccc0111110mmmmmddddlllll001nnnn_case_0TesterCase4 {
 public:
  BFI_cccc0111110mmmmmddddlllll001nnnn_case_0Tester_Case4()
    : BFI_cccc0111110mmmmmddddlllll001nnnn_case_0TesterCase4(
      state_.BFI_cccc0111110mmmmmddddlllll001nnnn_case_0_BFI_instance_)
  {}
};

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1,
//       baseline: BFC_cccc0111110mmmmmddddlllll0011111_case_0,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7)],
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       pattern: cccc0111110mmmmmddddlllll0011111,
//       rule: BFC,
//       safety: [Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rd}}
class BFC_cccc0111110mmmmmddddlllll0011111_case_0Tester_Case5
    : public BFC_cccc0111110mmmmmddddlllll0011111_case_0TesterCase5 {
 public:
  BFC_cccc0111110mmmmmddddlllll0011111_case_0Tester_Case5()
    : BFC_cccc0111110mmmmmddddlllll0011111_case_0TesterCase5(
      state_.BFC_cccc0111110mmmmmddddlllll0011111_case_0_BFC_instance_)
  {}
};

// op1(24:20)=1111x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1,
//       baseline: UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       lsb: lsb(11:7),
//       pattern: cccc0111111mmmmmddddlllll101nnnn,
//       rule: UBFX,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
class UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0Tester_Case6
    : public UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0TesterCase6 {
 public:
  UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0Tester_Case6()
    : UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0TesterCase6(
      state_.UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0_UBFX_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01111000ddddaaaammmm0001nnnn,
//       rule: USADA8,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
TEST_F(Arm32DecoderStateTests,
       USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0Tester_Case0_TestCase0) {
  USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0Tester_Case0 baseline_tester;
  NamedActual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1_USADA8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01111000ddddaaaammmm0001nnnn");
}

// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//       baseline: USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01111000dddd1111mmmm0001nnnn,
//       rule: USAD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0Tester_Case1_TestCase1) {
  USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0Tester_Case1 baseline_tester;
  NamedActual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1_USAD8 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01111000dddd1111mmmm0001nnnn");
}

// op1(24:20)=11111 & op2(7:5)=111
//    = {actual: Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1,
//       baseline: UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0,
//       defs: {},
//       inst: inst,
//       pattern: cccc01111111iiiiiiiiiiii1111iiii,
//       rule: UDF,
//       safety: [not IsUDFNaClSafe(inst) => FORBIDDEN_OPERANDS],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0Tester_Case2_TestCase2) {
  UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0Tester_Case2 baseline_tester;
  NamedActual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1_UDF actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01111111iiiiiiiiiiii1111iiii");
}

// op1(24:20)=1101x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1,
//       baseline: SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       lsb: lsb(11:7),
//       pattern: cccc0111101wwwwwddddlllll101nnnn,
//       rule: SBFX,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
TEST_F(Arm32DecoderStateTests,
       SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0Tester_Case3_TestCase3) {
  SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0Tester_Case3 baseline_tester;
  NamedActual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1_SBFX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0111101wwwwwddddlllll101nnnn");
}

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=~1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1,
//       baseline: BFI_cccc0111110mmmmmddddlllll001nnnn_case_0,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       pattern: cccc0111110mmmmmddddlllll001nnnn,
//       rule: BFI,
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rn, Rd}}
TEST_F(Arm32DecoderStateTests,
       BFI_cccc0111110mmmmmddddlllll001nnnn_case_0Tester_Case4_TestCase4) {
  BFI_cccc0111110mmmmmddddlllll001nnnn_case_0Tester_Case4 baseline_tester;
  NamedActual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1_BFI actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0111110mmmmmddddlllll001nnnn");
}

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1,
//       baseline: BFC_cccc0111110mmmmmddddlllll0011111_case_0,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7)],
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       pattern: cccc0111110mmmmmddddlllll0011111,
//       rule: BFC,
//       safety: [Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rd}}
TEST_F(Arm32DecoderStateTests,
       BFC_cccc0111110mmmmmddddlllll0011111_case_0Tester_Case5_TestCase5) {
  BFC_cccc0111110mmmmmddddlllll0011111_case_0Tester_Case5 baseline_tester;
  NamedActual_BFC_cccc0111110mmmmmddddlllll0011111_case_1_BFC actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0111110mmmmmddddlllll0011111");
}

// op1(24:20)=1111x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1,
//       baseline: UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       lsb: lsb(11:7),
//       pattern: cccc0111111mmmmmddddlllll101nnnn,
//       rule: UBFX,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
TEST_F(Arm32DecoderStateTests,
       UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0Tester_Case6_TestCase6) {
  UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0Tester_Case6 baseline_tester;
  NamedActual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1_UBFX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0111111mmmmmddddlllll101nnnn");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
