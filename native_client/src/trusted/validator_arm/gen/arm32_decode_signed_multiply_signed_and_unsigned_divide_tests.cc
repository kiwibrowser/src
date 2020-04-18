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


// op1(22:20)=000 & op2(7:5)=00x & A(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000ddddaaaammmm00m1nnnn,
//       rule: SMLAD,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~000
  if ((inst.Bits() & 0x00700000)  !=
          0x00000000) return false;
  // op2(7:5)=~00x
  if ((inst.Bits() & 0x000000C0)  !=
          0x00000000) return false;
  // A(15:12)=1111
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=000 & op2(7:5)=00x & A(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000dddd1111mmmm00m1nnnn,
//       rule: SMUAD,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
class SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~000
  if ((inst.Bits() & 0x00700000)  !=
          0x00000000) return false;
  // op2(7:5)=~00x
  if ((inst.Bits() & 0x000000C0)  !=
          0x00000000) return false;
  // A(15:12)=~1111
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=000 & op2(7:5)=01x & A(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000ddddaaaammmm01m1nnnn,
//       rule: SMLSD,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~000
  if ((inst.Bits() & 0x00700000)  !=
          0x00000000) return false;
  // op2(7:5)=~01x
  if ((inst.Bits() & 0x000000C0)  !=
          0x00000040) return false;
  // A(15:12)=1111
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=000 & op2(7:5)=01x & A(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000dddd1111mmmm01m1nnnn,
//       rule: SMUSD,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
class SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~000
  if ((inst.Bits() & 0x00700000)  !=
          0x00000000) return false;
  // op2(7:5)=~01x
  if ((inst.Bits() & 0x000000C0)  !=
          0x00000040) return false;
  // A(15:12)=~1111
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=001 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110001dddd1111mmmm0001nnnn,
//       rule: SDIV,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
class SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~001
  if ((inst.Bits() & 0x00700000)  !=
          0x00100000) return false;
  // op2(7:5)=~000
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=011 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110011dddd1111mmmm0001nnnn,
//       rule: UDIV,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
class UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~011
  if ((inst.Bits() & 0x00700000)  !=
          0x00300000) return false;
  // op2(7:5)=~000
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=100 & op2(7:5)=00x
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1,
//       baseline: SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0,
//       defs: {RdHi, RdLo},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110100hhhhllllmmmm00m1nnnn,
//       rule: SMLALD,
//       safety: [Pc in {RdHi, RdLo, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdHi, RdLo, Rm, Rn}}
class SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~100
  if ((inst.Bits() & 0x00700000)  !=
          0x00400000) return false;
  // op2(7:5)=~00x
  if ((inst.Bits() & 0x000000C0)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=100 & op2(7:5)=01x
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1,
//       baseline: SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0,
//       defs: {RdHi, RdLo},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110100hhhhllllmmmm01m1nnnn,
//       rule: SMLSLD,
//       safety: [Pc in {RdHi, RdLo, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdHi, RdLo, Rm, Rn}}
class SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~100
  if ((inst.Bits() & 0x00700000)  !=
          0x00400000) return false;
  // op2(7:5)=~01x
  if ((inst.Bits() & 0x000000C0)  !=
          0x00000040) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=101 & op2(7:5)=00x & A(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110101ddddaaaammmm00r1nnnn,
//       rule: SMMLA,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~101
  if ((inst.Bits() & 0x00700000)  !=
          0x00500000) return false;
  // op2(7:5)=~00x
  if ((inst.Bits() & 0x000000C0)  !=
          0x00000000) return false;
  // A(15:12)=1111
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=101 & op2(7:5)=00x & A(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110101dddd1111mmmm00r1nnnn,
//       rule: SMMUL,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
class SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~101
  if ((inst.Bits() & 0x00700000)  !=
          0x00500000) return false;
  // op2(7:5)=~00x
  if ((inst.Bits() & 0x000000C0)  !=
          0x00000000) return false;
  // A(15:12)=~1111
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(22:20)=101 & op2(7:5)=11x
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110101ddddaaaammmm11r1nnnn,
//       rule: SMMLS,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(22:20)=~101
  if ((inst.Bits() & 0x00700000)  !=
          0x00500000) return false;
  // op2(7:5)=~11x
  if ((inst.Bits() & 0x000000C0)  !=
          0x000000C0) return false;

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

// op1(22:20)=000 & op2(7:5)=00x & A(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000ddddaaaammmm00m1nnnn,
//       rule: SMLAD,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0Tester_Case0
    : public SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0TesterCase0 {
 public:
  SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0Tester_Case0()
    : SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0TesterCase0(
      state_.SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0_SMLAD_instance_)
  {}
};

// op1(22:20)=000 & op2(7:5)=00x & A(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000dddd1111mmmm00m1nnnn,
//       rule: SMUAD,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
class SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0Tester_Case1
    : public SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0TesterCase1 {
 public:
  SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0Tester_Case1()
    : SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0TesterCase1(
      state_.SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0_SMUAD_instance_)
  {}
};

// op1(22:20)=000 & op2(7:5)=01x & A(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000ddddaaaammmm01m1nnnn,
//       rule: SMLSD,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0Tester_Case2
    : public SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0TesterCase2 {
 public:
  SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0Tester_Case2()
    : SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0TesterCase2(
      state_.SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0_SMLSD_instance_)
  {}
};

// op1(22:20)=000 & op2(7:5)=01x & A(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000dddd1111mmmm01m1nnnn,
//       rule: SMUSD,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
class SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0Tester_Case3
    : public SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0TesterCase3 {
 public:
  SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0Tester_Case3()
    : SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0TesterCase3(
      state_.SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0_SMUSD_instance_)
  {}
};

// op1(22:20)=001 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110001dddd1111mmmm0001nnnn,
//       rule: SDIV,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
class SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0Tester_Case4
    : public SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0TesterCase4 {
 public:
  SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0Tester_Case4()
    : SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0TesterCase4(
      state_.SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0_SDIV_instance_)
  {}
};

// op1(22:20)=011 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110011dddd1111mmmm0001nnnn,
//       rule: UDIV,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
class UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0Tester_Case5
    : public UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0TesterCase5 {
 public:
  UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0Tester_Case5()
    : UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0TesterCase5(
      state_.UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0_UDIV_instance_)
  {}
};

// op1(22:20)=100 & op2(7:5)=00x
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1,
//       baseline: SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0,
//       defs: {RdHi, RdLo},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110100hhhhllllmmmm00m1nnnn,
//       rule: SMLALD,
//       safety: [Pc in {RdHi, RdLo, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdHi, RdLo, Rm, Rn}}
class SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0Tester_Case6
    : public SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0TesterCase6 {
 public:
  SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0Tester_Case6()
    : SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0TesterCase6(
      state_.SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0_SMLALD_instance_)
  {}
};

// op1(22:20)=100 & op2(7:5)=01x
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1,
//       baseline: SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0,
//       defs: {RdHi, RdLo},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110100hhhhllllmmmm01m1nnnn,
//       rule: SMLSLD,
//       safety: [Pc in {RdHi, RdLo, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdHi, RdLo, Rm, Rn}}
class SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0Tester_Case7
    : public SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0TesterCase7 {
 public:
  SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0Tester_Case7()
    : SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0TesterCase7(
      state_.SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0_SMLSLD_instance_)
  {}
};

// op1(22:20)=101 & op2(7:5)=00x & A(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110101ddddaaaammmm00r1nnnn,
//       rule: SMMLA,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0Tester_Case8
    : public SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0TesterCase8 {
 public:
  SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0Tester_Case8()
    : SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0TesterCase8(
      state_.SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0_SMMLA_instance_)
  {}
};

// op1(22:20)=101 & op2(7:5)=00x & A(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110101dddd1111mmmm00r1nnnn,
//       rule: SMMUL,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
class SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0Tester_Case9
    : public SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0TesterCase9 {
 public:
  SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0Tester_Case9()
    : SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0TesterCase9(
      state_.SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0_SMMUL_instance_)
  {}
};

// op1(22:20)=101 & op2(7:5)=11x
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110101ddddaaaammmm11r1nnnn,
//       rule: SMMLS,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0Tester_Case10
    : public SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0TesterCase10 {
 public:
  SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0Tester_Case10()
    : SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0TesterCase10(
      state_.SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0_SMMLS_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op1(22:20)=000 & op2(7:5)=00x & A(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000ddddaaaammmm00m1nnnn,
//       rule: SMLAD,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
TEST_F(Arm32DecoderStateTests,
       SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0Tester_Case0_TestCase0) {
  SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0Tester_Case0 baseline_tester;
  NamedActual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1_SMLAD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01110000ddddaaaammmm00m1nnnn");
}

// op1(22:20)=000 & op2(7:5)=00x & A(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000dddd1111mmmm00m1nnnn,
//       rule: SMUAD,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
TEST_F(Arm32DecoderStateTests,
       SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0Tester_Case1_TestCase1) {
  SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0Tester_Case1 baseline_tester;
  NamedActual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1_SMUAD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01110000dddd1111mmmm00m1nnnn");
}

// op1(22:20)=000 & op2(7:5)=01x & A(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000ddddaaaammmm01m1nnnn,
//       rule: SMLSD,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
TEST_F(Arm32DecoderStateTests,
       SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0Tester_Case2_TestCase2) {
  SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0Tester_Case2 baseline_tester;
  NamedActual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1_SMLSD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01110000ddddaaaammmm01m1nnnn");
}

// op1(22:20)=000 & op2(7:5)=01x & A(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110000dddd1111mmmm01m1nnnn,
//       rule: SMUSD,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
TEST_F(Arm32DecoderStateTests,
       SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0Tester_Case3_TestCase3) {
  SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0Tester_Case3 baseline_tester;
  NamedActual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1_SMUSD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01110000dddd1111mmmm01m1nnnn");
}

// op1(22:20)=001 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110001dddd1111mmmm0001nnnn,
//       rule: SDIV,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
TEST_F(Arm32DecoderStateTests,
       SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0Tester_Case4_TestCase4) {
  SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0Tester_Case4 baseline_tester;
  NamedActual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1_SDIV actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01110001dddd1111mmmm0001nnnn");
}

// op1(22:20)=011 & op2(7:5)=000 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110011dddd1111mmmm0001nnnn,
//       rule: UDIV,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
TEST_F(Arm32DecoderStateTests,
       UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0Tester_Case5_TestCase5) {
  UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0Tester_Case5 baseline_tester;
  NamedActual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1_UDIV actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01110011dddd1111mmmm0001nnnn");
}

// op1(22:20)=100 & op2(7:5)=00x
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1,
//       baseline: SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0,
//       defs: {RdHi, RdLo},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110100hhhhllllmmmm00m1nnnn,
//       rule: SMLALD,
//       safety: [Pc in {RdHi, RdLo, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdHi, RdLo, Rm, Rn}}
TEST_F(Arm32DecoderStateTests,
       SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0Tester_Case6_TestCase6) {
  SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0Tester_Case6 baseline_tester;
  NamedActual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1_SMLALD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01110100hhhhllllmmmm00m1nnnn");
}

// op1(22:20)=100 & op2(7:5)=01x
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1,
//       baseline: SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0,
//       defs: {RdHi, RdLo},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110100hhhhllllmmmm01m1nnnn,
//       rule: SMLSLD,
//       safety: [Pc in {RdHi, RdLo, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdHi, RdLo, Rm, Rn}}
TEST_F(Arm32DecoderStateTests,
       SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0Tester_Case7_TestCase7) {
  SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0Tester_Case7 baseline_tester;
  NamedActual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1_SMLSLD actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01110100hhhhllllmmmm01m1nnnn");
}

// op1(22:20)=101 & op2(7:5)=00x & A(15:12)=~1111
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110101ddddaaaammmm00r1nnnn,
//       rule: SMMLA,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
TEST_F(Arm32DecoderStateTests,
       SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0Tester_Case8_TestCase8) {
  SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0Tester_Case8 baseline_tester;
  NamedActual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1_SMMLA actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01110101ddddaaaammmm00r1nnnn");
}

// op1(22:20)=101 & op2(7:5)=00x & A(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//       baseline: SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110101dddd1111mmmm00r1nnnn,
//       rule: SMMUL,
//       safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//       uses: {Rm, Rn}}
TEST_F(Arm32DecoderStateTests,
       SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0Tester_Case9_TestCase9) {
  SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0Tester_Case9 baseline_tester;
  NamedActual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1_SMMUL actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01110101dddd1111mmmm00r1nnnn");
}

// op1(22:20)=101 & op2(7:5)=11x
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//       baseline: SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc01110101ddddaaaammmm11r1nnnn,
//       rule: SMMLS,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
TEST_F(Arm32DecoderStateTests,
       SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0Tester_Case10_TestCase10) {
  SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0Tester_Case10 baseline_tester;
  NamedActual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1_SMMLS actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc01110101ddddaaaammmm11r1nnnn");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
