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


// op(23:20)=0100
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1,
//       baseline: UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00000100hhhhllllmmmm1001nnnn,
//       rule: UMAAL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdLo, RdHi, Rn, Rm}}
class UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(23:20)=~0100
  if ((inst.Bits() & 0x00F00000)  !=
          0x00400000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(23:20)=0101
//    = {actual: Actual_Unnamed_case_1,
//       baseline: Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0,
//       defs: {},
//       pattern: cccc00000101xxxxxxxxxxxx1001xxxx,
//       safety: [true => UNDEFINED],
//       true: true,
//       uses: {}}
class Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(23:20)=~0101
  if ((inst.Bits() & 0x00F00000)  !=
          0x00500000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(23:20)=0110
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//       baseline: MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00000110ddddaaaammmm1001nnnn,
//       rule: MLS_A1,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(23:20)=~0110
  if ((inst.Bits() & 0x00F00000)  !=
          0x00600000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(23:20)=0111
//    = {actual: Actual_Unnamed_case_1,
//       baseline: Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0,
//       defs: {},
//       pattern: cccc00000111xxxxxxxxxxxx1001xxxx,
//       safety: [true => UNDEFINED],
//       true: true,
//       uses: {}}
class Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(23:20)=~0111
  if ((inst.Bits() & 0x00F00000)  !=
          0x00700000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(23:20)=000x & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1,
//       baseline: MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000000sdddd0000mmmm1001nnnn,
//       rule: MUL_A1,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            Rd  ==
//               Rn) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rm, Rn}}
class MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(23:20)=~000x
  if ((inst.Bits() & 0x00E00000)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x00000000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(23:20)=001x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1,
//       baseline: MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000001sddddaaaammmm1001nnnn,
//       rule: MLA_A1,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            Rd  ==
//               Rn) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Ra}}
class MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(23:20)=~001x
  if ((inst.Bits() & 0x00E00000)  !=
          0x00200000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(23:20)=100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1,
//       baseline: UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000100shhhhllllmmmm1001nnnn,
//       rule: UMULL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(23:20)=~100x
  if ((inst.Bits() & 0x00E00000)  !=
          0x00800000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(23:20)=101x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1,
//       baseline: UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000101shhhhllllmmmm1001nnnn,
//       rule: UMLAL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {RdLo, RdHi, Rn, Rm}}
class UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(23:20)=~101x
  if ((inst.Bits() & 0x00E00000)  !=
          0x00A00000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(23:20)=110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1,
//       baseline: SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000110shhhhllllmmmm1001nnnn,
//       rule: SMULL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(23:20)=~110x
  if ((inst.Bits() & 0x00E00000)  !=
          0x00C00000) return false;

  // if cond(31:28)=1111, don't test instruction.
  if ((inst.Bits() & 0xF0000000) == 0xF0000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op(23:20)=111x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1,
//       baseline: SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000111shhhhllllmmmm1001nnnn,
//       rule: SMLAL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {RdLo, RdHi, Rn, Rm}}
class SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op(23:20)=~111x
  if ((inst.Bits() & 0x00E00000)  !=
          0x00E00000) return false;

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

// op(23:20)=0100
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1,
//       baseline: UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00000100hhhhllllmmmm1001nnnn,
//       rule: UMAAL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdLo, RdHi, Rn, Rm}}
class UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0Tester_Case0
    : public UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0TesterCase0 {
 public:
  UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0Tester_Case0()
    : UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0TesterCase0(
      state_.UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0_UMAAL_A1_instance_)
  {}
};

// op(23:20)=0101
//    = {actual: Actual_Unnamed_case_1,
//       baseline: Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0,
//       defs: {},
//       pattern: cccc00000101xxxxxxxxxxxx1001xxxx,
//       safety: [true => UNDEFINED],
//       true: true,
//       uses: {}}
class Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0Tester_Case1
    : public Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0TesterCase1 {
 public:
  Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0Tester_Case1()
    : Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0TesterCase1(
      state_.Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0_None_instance_)
  {}
};

// op(23:20)=0110
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//       baseline: MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00000110ddddaaaammmm1001nnnn,
//       rule: MLS_A1,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0Tester_Case2
    : public MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0TesterCase2 {
 public:
  MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0Tester_Case2()
    : MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0TesterCase2(
      state_.MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0_MLS_A1_instance_)
  {}
};

// op(23:20)=0111
//    = {actual: Actual_Unnamed_case_1,
//       baseline: Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0,
//       defs: {},
//       pattern: cccc00000111xxxxxxxxxxxx1001xxxx,
//       safety: [true => UNDEFINED],
//       true: true,
//       uses: {}}
class Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0Tester_Case3
    : public Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0TesterCase3 {
 public:
  Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0Tester_Case3()
    : Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0TesterCase3(
      state_.Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0_None_instance_)
  {}
};

// op(23:20)=000x & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1,
//       baseline: MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000000sdddd0000mmmm1001nnnn,
//       rule: MUL_A1,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            Rd  ==
//               Rn) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rm, Rn}}
class MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0Tester_Case4
    : public MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0TesterCase4 {
 public:
  MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0Tester_Case4()
    : MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0TesterCase4(
      state_.MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0_MUL_A1_instance_)
  {}
};

// op(23:20)=001x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1,
//       baseline: MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000001sddddaaaammmm1001nnnn,
//       rule: MLA_A1,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            Rd  ==
//               Rn) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Ra}}
class MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0Tester_Case5
    : public MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0TesterCase5 {
 public:
  MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0Tester_Case5()
    : MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0TesterCase5(
      state_.MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0_MLA_A1_instance_)
  {}
};

// op(23:20)=100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1,
//       baseline: UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000100shhhhllllmmmm1001nnnn,
//       rule: UMULL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0Tester_Case6
    : public UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0TesterCase6 {
 public:
  UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0Tester_Case6()
    : UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0TesterCase6(
      state_.UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0_UMULL_A1_instance_)
  {}
};

// op(23:20)=101x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1,
//       baseline: UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000101shhhhllllmmmm1001nnnn,
//       rule: UMLAL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {RdLo, RdHi, Rn, Rm}}
class UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0Tester_Case7
    : public UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0TesterCase7 {
 public:
  UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0Tester_Case7()
    : UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0TesterCase7(
      state_.UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0_UMLAL_A1_instance_)
  {}
};

// op(23:20)=110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1,
//       baseline: SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000110shhhhllllmmmm1001nnnn,
//       rule: SMULL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
class SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0Tester_Case8
    : public SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0TesterCase8 {
 public:
  SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0Tester_Case8()
    : SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0TesterCase8(
      state_.SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0_SMULL_A1_instance_)
  {}
};

// op(23:20)=111x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1,
//       baseline: SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000111shhhhllllmmmm1001nnnn,
//       rule: SMLAL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {RdLo, RdHi, Rn, Rm}}
class SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0Tester_Case9
    : public SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0TesterCase9 {
 public:
  SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0Tester_Case9()
    : SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0TesterCase9(
      state_.SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0_SMLAL_A1_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op(23:20)=0100
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1,
//       baseline: UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00000100hhhhllllmmmm1001nnnn,
//       rule: UMAAL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE],
//       uses: {RdLo, RdHi, Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0Tester_Case0_TestCase0) {
  UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0Tester_Case0 baseline_tester;
  NamedActual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1_UMAAL_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00000100hhhhllllmmmm1001nnnn");
}

// op(23:20)=0101
//    = {actual: Actual_Unnamed_case_1,
//       baseline: Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0,
//       defs: {},
//       pattern: cccc00000101xxxxxxxxxxxx1001xxxx,
//       safety: [true => UNDEFINED],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0Tester_Case1_TestCase1) {
  Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0Tester_Case1 baseline_tester;
  NamedActual_Unnamed_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00000101xxxxxxxxxxxx1001xxxx");
}

// op(23:20)=0110
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//       baseline: MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00000110ddddaaaammmm1001nnnn,
//       rule: MLS_A1,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
TEST_F(Arm32DecoderStateTests,
       MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0Tester_Case2_TestCase2) {
  MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0Tester_Case2 baseline_tester;
  NamedActual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1_MLS_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00000110ddddaaaammmm1001nnnn");
}

// op(23:20)=0111
//    = {actual: Actual_Unnamed_case_1,
//       baseline: Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0,
//       defs: {},
//       pattern: cccc00000111xxxxxxxxxxxx1001xxxx,
//       safety: [true => UNDEFINED],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0Tester_Case3_TestCase3) {
  Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0Tester_Case3 baseline_tester;
  NamedActual_Unnamed_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00000111xxxxxxxxxxxx1001xxxx");
}

// op(23:20)=000x & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1,
//       baseline: MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000000sdddd0000mmmm1001nnnn,
//       rule: MUL_A1,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            Rd  ==
//               Rn) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rm, Rn}}
TEST_F(Arm32DecoderStateTests,
       MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0Tester_Case4_TestCase4) {
  MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0Tester_Case4 baseline_tester;
  NamedActual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1_MUL_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000000sdddd0000mmmm1001nnnn");
}

// op(23:20)=001x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1,
//       baseline: MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0,
//       defs: {Rd, NZCV
//            if setflags
//            else None},
//       fields: [S(20), Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000001sddddaaaammmm1001nnnn,
//       rule: MLA_A1,
//       safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            Rd  ==
//               Rn) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm, Ra}}
TEST_F(Arm32DecoderStateTests,
       MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0Tester_Case5_TestCase5) {
  MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0Tester_Case5 baseline_tester;
  NamedActual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1_MLA_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000001sddddaaaammmm1001nnnn");
}

// op(23:20)=100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1,
//       baseline: UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000100shhhhllllmmmm1001nnnn,
//       rule: UMULL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0Tester_Case6_TestCase6) {
  UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0Tester_Case6 baseline_tester;
  NamedActual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1_UMULL_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000100shhhhllllmmmm1001nnnn");
}

// op(23:20)=101x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1,
//       baseline: UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000101shhhhllllmmmm1001nnnn,
//       rule: UMLAL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {RdLo, RdHi, Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0Tester_Case7_TestCase7) {
  UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0Tester_Case7 baseline_tester;
  NamedActual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1_UMLAL_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000101shhhhllllmmmm1001nnnn");
}

// op(23:20)=110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1,
//       baseline: SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000110shhhhllllmmmm1001nnnn,
//       rule: SMULL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0Tester_Case8_TestCase8) {
  SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0Tester_Case8 baseline_tester;
  NamedActual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1_SMULL_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000110shhhhllllmmmm1001nnnn");
}

// op(23:20)=111x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1,
//       baseline: SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0,
//       defs: {RdLo, RdHi, NZCV
//            if setflags
//            else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000111shhhhllllmmmm1001nnnn,
//       rule: SMLAL_A1,
//       safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//         RdHi  ==
//               RdLo => UNPREDICTABLE,
//         (ArchVersion()  <
//               6 &&
//            (RdHi  ==
//               Rn ||
//            RdLo  ==
//               Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1,
//       uses: {RdLo, RdHi, Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0Tester_Case9_TestCase9) {
  SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0Tester_Case9 baseline_tester;
  NamedActual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1_SMLAL_A1 actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc0000111shhhhllllmmmm1001nnnn");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
