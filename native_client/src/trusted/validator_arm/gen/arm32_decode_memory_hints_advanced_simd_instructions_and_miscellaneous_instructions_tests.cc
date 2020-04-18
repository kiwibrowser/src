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


// op1(26:20)=0010000 & op2(7:4)=0000 & Rn(19:16)=xxx1 & $pattern(31:0)=xxxxxxxxxxxx000x000000x0xxxx0000
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SETEND_1111000100000001000000i000000000_case_0,
//       defs: {},
//       pattern: 1111000100000001000000i000000000,
//       rule: SETEND,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class SETEND_1111000100000001000000i000000000_case_0TesterCase0
    : public Arm32DecoderTester {
 public:
  SETEND_1111000100000001000000i000000000_case_0TesterCase0(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool SETEND_1111000100000001000000i000000000_case_0TesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~0010000
  if ((inst.Bits() & 0x07F00000)  !=
          0x01000000) return false;
  // op2(7:4)=~0000
  if ((inst.Bits() & 0x000000F0)  !=
          0x00000000) return false;
  // Rn(19:16)=~xxx1
  if ((inst.Bits() & 0x00010000)  !=
          0x00010000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx000x000000x0xxxx0000
  if ((inst.Bits() & 0x000EFD0F)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=0010000 & op2(7:4)=xx0x & Rn(19:16)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000000xxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CPS_111100010000iii00000000iii0iiiii_case_0,
//       defs: {},
//       pattern: 111100010000iii00000000iii0iiiii,
//       rule: CPS,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class CPS_111100010000iii00000000iii0iiiii_case_0TesterCase1
    : public Arm32DecoderTester {
 public:
  CPS_111100010000iii00000000iii0iiiii_case_0TesterCase1(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool CPS_111100010000iii00000000iii0iiiii_case_0TesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~0010000
  if ((inst.Bits() & 0x07F00000)  !=
          0x01000000) return false;
  // op2(7:4)=~xx0x
  if ((inst.Bits() & 0x00000020)  !=
          0x00000000) return false;
  // Rn(19:16)=~xxx0
  if ((inst.Bits() & 0x00010000)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx0000000xxxxxxxxx
  if ((inst.Bits() & 0x0000FE00)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=1010011
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 111101010011xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0TesterCase2
    : public Arm32DecoderTester {
 public:
  Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0TesterCase2(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0TesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~1010011
  if ((inst.Bits() & 0x07F00000)  !=
          0x05300000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=1010111 & op2(7:4)=0000
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx0000xxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0TesterCase3
    : public Arm32DecoderTester {
 public:
  Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0TesterCase3(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0TesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~1010111
  if ((inst.Bits() & 0x07F00000)  !=
          0x05700000) return false;
  // op2(7:4)=~0000
  if ((inst.Bits() & 0x000000F0)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=1010111 & op2(7:4)=0001 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxx1111
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CLREX_11110101011111111111000000011111_case_0,
//       defs: {},
//       pattern: 11110101011111111111000000011111,
//       rule: CLREX,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class CLREX_11110101011111111111000000011111_case_0TesterCase4
    : public Arm32DecoderTester {
 public:
  CLREX_11110101011111111111000000011111_case_0TesterCase4(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool CLREX_11110101011111111111000000011111_case_0TesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~1010111
  if ((inst.Bits() & 0x07F00000)  !=
          0x05700000) return false;
  // op2(7:4)=~0001
  if ((inst.Bits() & 0x000000F0)  !=
          0x00000010) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx111111110000xxxx1111
  if ((inst.Bits() & 0x000FFF0F)  !=
          0x000FF00F) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=1010111 & op2(7:4)=0100 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx
//    = {actual: Actual_DMB_1111010101111111111100000101xxxx_case_1,
//       baseline: DSB_1111010101111111111100000100xxxx_case_0,
//       defs: {},
//       fields: [option(3:0)],
//       option: option(3:0),
//       pattern: 1111010101111111111100000100xxxx,
//       rule: DSB,
//       safety: [not option in {'1111'(3:0), '1110'(3:0), '1011'(3:0), '1010'(3:0), '0111'(3:0), '0110'(3:0), '0011'(3:0), '0010'(3:0)} => FORBIDDEN_OPERANDS],
//       uses: {}}
class DSB_1111010101111111111100000100xxxx_case_0TesterCase5
    : public Arm32DecoderTester {
 public:
  DSB_1111010101111111111100000100xxxx_case_0TesterCase5(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool DSB_1111010101111111111100000100xxxx_case_0TesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~1010111
  if ((inst.Bits() & 0x07F00000)  !=
          0x05700000) return false;
  // op2(7:4)=~0100
  if ((inst.Bits() & 0x000000F0)  !=
          0x00000040) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx111111110000xxxxxxxx
  if ((inst.Bits() & 0x000FFF00)  !=
          0x000FF000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=1010111 & op2(7:4)=0101 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx
//    = {actual: Actual_DMB_1111010101111111111100000101xxxx_case_1,
//       baseline: DMB_1111010101111111111100000101xxxx_case_0,
//       defs: {},
//       fields: [option(3:0)],
//       option: option(3:0),
//       pattern: 1111010101111111111100000101xxxx,
//       rule: DMB,
//       safety: [not option in {'1111'(3:0), '1110'(3:0), '1011'(3:0), '1010'(3:0), '0111'(3:0), '0110'(3:0), '0011'(3:0), '0010'(3:0)} => FORBIDDEN_OPERANDS],
//       uses: {}}
class DMB_1111010101111111111100000101xxxx_case_0TesterCase6
    : public Arm32DecoderTester {
 public:
  DMB_1111010101111111111100000101xxxx_case_0TesterCase6(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool DMB_1111010101111111111100000101xxxx_case_0TesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~1010111
  if ((inst.Bits() & 0x07F00000)  !=
          0x05700000) return false;
  // op2(7:4)=~0101
  if ((inst.Bits() & 0x000000F0)  !=
          0x00000050) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx111111110000xxxxxxxx
  if ((inst.Bits() & 0x000FFF00)  !=
          0x000FF000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=1010111 & op2(7:4)=0110 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx
//    = {actual: Actual_ISB_1111010101111111111100000110xxxx_case_1,
//       baseline: ISB_1111010101111111111100000110xxxx_case_0,
//       defs: {},
//       fields: [option(3:0)],
//       option: option(3:0),
//       pattern: 1111010101111111111100000110xxxx,
//       rule: ISB,
//       safety: [option(3:0)=~1111 => FORBIDDEN_OPERANDS],
//       uses: {}}
class ISB_1111010101111111111100000110xxxx_case_0TesterCase7
    : public Arm32DecoderTester {
 public:
  ISB_1111010101111111111100000110xxxx_case_0TesterCase7(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool ISB_1111010101111111111100000110xxxx_case_0TesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~1010111
  if ((inst.Bits() & 0x07F00000)  !=
          0x05700000) return false;
  // op2(7:4)=~0110
  if ((inst.Bits() & 0x000000F0)  !=
          0x00000060) return false;
  // $pattern(31:0)=~xxxxxxxxxxxx111111110000xxxxxxxx
  if ((inst.Bits() & 0x000FFF00)  !=
          0x000FF000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=1010111 & op2(7:4)=0111
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx0111xxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0TesterCase8
    : public Arm32DecoderTester {
 public:
  Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0TesterCase8(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0TesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~1010111
  if ((inst.Bits() & 0x07F00000)  !=
          0x05700000) return false;
  // op2(7:4)=~0111
  if ((inst.Bits() & 0x000000F0)  !=
          0x00000070) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=1010111 & op2(7:4)=001x
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx001xxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0TesterCase9
    : public Arm32DecoderTester {
 public:
  Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0TesterCase9(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0TesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~1010111
  if ((inst.Bits() & 0x07F00000)  !=
          0x05700000) return false;
  // op2(7:4)=~001x
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000020) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=1010111 & op2(7:4)=1xxx
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx1xxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0TesterCase10
    : public Arm32DecoderTester {
 public:
  Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0TesterCase10(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0TesterCase10
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~1010111
  if ((inst.Bits() & 0x07F00000)  !=
          0x05700000) return false;
  // op2(7:4)=~1xxx
  if ((inst.Bits() & 0x00000080)  !=
          0x00000080) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=100x001
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 11110100x001xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0TesterCase11
    : public Arm32DecoderTester {
 public:
  Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0TesterCase11(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0TesterCase11
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~100x001
  if ((inst.Bits() & 0x07700000)  !=
          0x04100000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=100x101 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: 11110100u101nnnn1111iiiiiiiiiiii,
//       rule: PLI_immediate_literal,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Rn},
//       violations: [implied by 'base']}
class PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0TesterCase12
    : public Arm32DecoderTester {
 public:
  PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0TesterCase12(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0TesterCase12
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~100x101
  if ((inst.Bits() & 0x07700000)  !=
          0x04500000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=101x001 & Rn(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: base  ==
//               Pc,
//       pattern: 11110101ur01nnnn1111iiiiiiiiiiii,
//       rule: PLD_PLDW_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR],
//       uses: {Rn},
//       violations: [implied by 'base']}
class PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0TesterCase13
    : public Arm32DecoderTester {
 public:
  PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0TesterCase13(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0TesterCase13
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~101x001
  if ((inst.Bits() & 0x07700000)  !=
          0x05100000) return false;
  // Rn(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=101x001 & Rn(19:16)=1111
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 11110101x001xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0TesterCase14
    : public Arm32DecoderTester {
 public:
  Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0TesterCase14(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0TesterCase14
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~101x001
  if ((inst.Bits() & 0x07700000)  !=
          0x05100000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=101x101 & Rn(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: base  ==
//               Pc,
//       pattern: 11110101ur01nnnn1111iiiiiiiiiiii,
//       rule: PLD_PLDW_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR],
//       uses: {Rn},
//       violations: [implied by 'base']}
class PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1TesterCase15
    : public Arm32DecoderTester {
 public:
  PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1TesterCase15(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1TesterCase15
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~101x101
  if ((inst.Bits() & 0x07700000)  !=
          0x05500000) return false;
  // Rn(19:16)=1111
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=101x101 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       actual: Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1,
//       base: Pc,
//       baseline: PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0,
//       defs: {},
//       is_literal_load: true,
//       pattern: 11110101u10111111111iiiiiiiiiiii,
//       rule: PLD_literal,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0TesterCase16
    : public Arm32DecoderTester {
 public:
  PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0TesterCase16(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0TesterCase16
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~101x101
  if ((inst.Bits() & 0x07700000)  !=
          0x05500000) return false;
  // Rn(19:16)=~1111
  if ((inst.Bits() & 0x000F0000)  !=
          0x000F0000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=110x001 & op2(7:4)=xxx0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0,
//       defs: {},
//       pattern: 11110110x001xxxxxxxxxxxxxxx0xxxx,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0TesterCase17
    : public Arm32DecoderTester {
 public:
  Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0TesterCase17(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0TesterCase17
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~110x001
  if ((inst.Bits() & 0x07700000)  !=
          0x06100000) return false;
  // op2(7:4)=~xxx0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=110x101 & op2(7:4)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0,
//       defs: {},
//       fields: [Rn(19:16), Rm(3:0)],
//       pattern: 11110110u101nnnn1111iiiiitt0mmmm,
//       rule: PLI_register,
//       safety: [Rm  ==
//               Pc => UNPREDICTABLE,
//         true => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Rm, Rn},
//       violations: [implied by 'base']}
class PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0TesterCase18
    : public Arm32DecoderTester {
 public:
  PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0TesterCase18(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0TesterCase18
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~110x101
  if ((inst.Bits() & 0x07700000)  !=
          0x06500000) return false;
  // op2(7:4)=~xxx0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=111x001 & op2(7:4)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       R: R(22),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0,
//       defs: {},
//       fields: [R(22), Rn(19:16), Rm(3:0)],
//       is_pldw: R(22)=1,
//       pattern: 11110111u001nnnn1111iiiiitt0mmmm,
//       rule: PLD_PLDW_register,
//       safety: [Rm  ==
//               Pc ||
//            (Rn  ==
//               Pc &&
//            is_pldw) => UNPREDICTABLE,
//         true => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Rm, Rn},
//       violations: [implied by 'base']}
class PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0TesterCase19
    : public Arm32DecoderTester {
 public:
  PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0TesterCase19(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0TesterCase19
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~111x001
  if ((inst.Bits() & 0x07700000)  !=
          0x07100000) return false;
  // op2(7:4)=~xxx0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=111x101 & op2(7:4)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       R: R(22),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0,
//       defs: {},
//       fields: [R(22), Rn(19:16), Rm(3:0)],
//       is_pldw: R(22)=1,
//       pattern: 11110111u101nnnn1111iiiiitt0mmmm,
//       rule: PLD_PLDW_register,
//       safety: [Rm  ==
//               Pc ||
//            (Rn  ==
//               Pc &&
//            is_pldw) => UNPREDICTABLE,
//         true => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Rm, Rn},
//       violations: [implied by 'base']}
class PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0TesterCase20
    : public Arm32DecoderTester {
 public:
  PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0TesterCase20(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0TesterCase20
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~111x101
  if ((inst.Bits() & 0x07700000)  !=
          0x07500000) return false;
  // op2(7:4)=~xxx0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;
  // $pattern(31:0)=~xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=1011x11
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 111101011x11xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0TesterCase21
    : public Arm32DecoderTester {
 public:
  Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0TesterCase21(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0TesterCase21
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~1011x11
  if ((inst.Bits() & 0x07B00000)  !=
          0x05B00000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=100xx11
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 11110100xx11xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0TesterCase22
    : public Arm32DecoderTester {
 public:
  Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0TesterCase22(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0TesterCase22
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~100xx11
  if ((inst.Bits() & 0x07300000)  !=
          0x04300000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(26:20)=11xxx11 & op2(7:4)=xxx0
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0,
//       defs: {},
//       pattern: 1111011xxx11xxxxxxxxxxxxxxx0xxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0TesterCase23
    : public Arm32DecoderTester {
 public:
  Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0TesterCase23(const NamedClassDecoder& decoder)
    : Arm32DecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0TesterCase23
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(26:20)=~11xxx11
  if ((inst.Bits() & 0x06300000)  !=
          0x06300000) return false;
  // op2(7:4)=~xxx0
  if ((inst.Bits() & 0x00000010)  !=
          0x00000000) return false;

  // Check other preconditions defined for the base decoder.
  return Arm32DecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// op1(26:20)=0010000 & op2(7:4)=0000 & Rn(19:16)=xxx1 & $pattern(31:0)=xxxxxxxxxxxx000x000000x0xxxx0000
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SETEND_1111000100000001000000i000000000_case_0,
//       defs: {},
//       pattern: 1111000100000001000000i000000000,
//       rule: SETEND,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class SETEND_1111000100000001000000i000000000_case_0Tester_Case0
    : public SETEND_1111000100000001000000i000000000_case_0TesterCase0 {
 public:
  SETEND_1111000100000001000000i000000000_case_0Tester_Case0()
    : SETEND_1111000100000001000000i000000000_case_0TesterCase0(
      state_.SETEND_1111000100000001000000i000000000_case_0_SETEND_instance_)
  {}
};

// op1(26:20)=0010000 & op2(7:4)=xx0x & Rn(19:16)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000000xxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CPS_111100010000iii00000000iii0iiiii_case_0,
//       defs: {},
//       pattern: 111100010000iii00000000iii0iiiii,
//       rule: CPS,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class CPS_111100010000iii00000000iii0iiiii_case_0Tester_Case1
    : public CPS_111100010000iii00000000iii0iiiii_case_0TesterCase1 {
 public:
  CPS_111100010000iii00000000iii0iiiii_case_0Tester_Case1()
    : CPS_111100010000iii00000000iii0iiiii_case_0TesterCase1(
      state_.CPS_111100010000iii00000000iii0iiiii_case_0_CPS_instance_)
  {}
};

// op1(26:20)=1010011
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 111101010011xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case2
    : public Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0TesterCase2 {
 public:
  Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case2()
    : Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0TesterCase2(
      state_.Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0_None_instance_)
  {}
};

// op1(26:20)=1010111 & op2(7:4)=0000
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx0000xxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0Tester_Case3
    : public Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0TesterCase3 {
 public:
  Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0Tester_Case3()
    : Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0TesterCase3(
      state_.Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0_None_instance_)
  {}
};

// op1(26:20)=1010111 & op2(7:4)=0001 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxx1111
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CLREX_11110101011111111111000000011111_case_0,
//       defs: {},
//       pattern: 11110101011111111111000000011111,
//       rule: CLREX,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class CLREX_11110101011111111111000000011111_case_0Tester_Case4
    : public CLREX_11110101011111111111000000011111_case_0TesterCase4 {
 public:
  CLREX_11110101011111111111000000011111_case_0Tester_Case4()
    : CLREX_11110101011111111111000000011111_case_0TesterCase4(
      state_.CLREX_11110101011111111111000000011111_case_0_CLREX_instance_)
  {}
};

// op1(26:20)=1010111 & op2(7:4)=0100 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx
//    = {actual: Actual_DMB_1111010101111111111100000101xxxx_case_1,
//       baseline: DSB_1111010101111111111100000100xxxx_case_0,
//       defs: {},
//       fields: [option(3:0)],
//       option: option(3:0),
//       pattern: 1111010101111111111100000100xxxx,
//       rule: DSB,
//       safety: [not option in {'1111'(3:0), '1110'(3:0), '1011'(3:0), '1010'(3:0), '0111'(3:0), '0110'(3:0), '0011'(3:0), '0010'(3:0)} => FORBIDDEN_OPERANDS],
//       uses: {}}
class DSB_1111010101111111111100000100xxxx_case_0Tester_Case5
    : public DSB_1111010101111111111100000100xxxx_case_0TesterCase5 {
 public:
  DSB_1111010101111111111100000100xxxx_case_0Tester_Case5()
    : DSB_1111010101111111111100000100xxxx_case_0TesterCase5(
      state_.DSB_1111010101111111111100000100xxxx_case_0_DSB_instance_)
  {}
};

// op1(26:20)=1010111 & op2(7:4)=0101 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx
//    = {actual: Actual_DMB_1111010101111111111100000101xxxx_case_1,
//       baseline: DMB_1111010101111111111100000101xxxx_case_0,
//       defs: {},
//       fields: [option(3:0)],
//       option: option(3:0),
//       pattern: 1111010101111111111100000101xxxx,
//       rule: DMB,
//       safety: [not option in {'1111'(3:0), '1110'(3:0), '1011'(3:0), '1010'(3:0), '0111'(3:0), '0110'(3:0), '0011'(3:0), '0010'(3:0)} => FORBIDDEN_OPERANDS],
//       uses: {}}
class DMB_1111010101111111111100000101xxxx_case_0Tester_Case6
    : public DMB_1111010101111111111100000101xxxx_case_0TesterCase6 {
 public:
  DMB_1111010101111111111100000101xxxx_case_0Tester_Case6()
    : DMB_1111010101111111111100000101xxxx_case_0TesterCase6(
      state_.DMB_1111010101111111111100000101xxxx_case_0_DMB_instance_)
  {}
};

// op1(26:20)=1010111 & op2(7:4)=0110 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx
//    = {actual: Actual_ISB_1111010101111111111100000110xxxx_case_1,
//       baseline: ISB_1111010101111111111100000110xxxx_case_0,
//       defs: {},
//       fields: [option(3:0)],
//       option: option(3:0),
//       pattern: 1111010101111111111100000110xxxx,
//       rule: ISB,
//       safety: [option(3:0)=~1111 => FORBIDDEN_OPERANDS],
//       uses: {}}
class ISB_1111010101111111111100000110xxxx_case_0Tester_Case7
    : public ISB_1111010101111111111100000110xxxx_case_0TesterCase7 {
 public:
  ISB_1111010101111111111100000110xxxx_case_0Tester_Case7()
    : ISB_1111010101111111111100000110xxxx_case_0TesterCase7(
      state_.ISB_1111010101111111111100000110xxxx_case_0_ISB_instance_)
  {}
};

// op1(26:20)=1010111 & op2(7:4)=0111
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx0111xxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0Tester_Case8
    : public Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0TesterCase8 {
 public:
  Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0Tester_Case8()
    : Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0TesterCase8(
      state_.Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0_None_instance_)
  {}
};

// op1(26:20)=1010111 & op2(7:4)=001x
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx001xxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0Tester_Case9
    : public Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0TesterCase9 {
 public:
  Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0Tester_Case9()
    : Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0TesterCase9(
      state_.Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0_None_instance_)
  {}
};

// op1(26:20)=1010111 & op2(7:4)=1xxx
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx1xxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0Tester_Case10
    : public Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0TesterCase10 {
 public:
  Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0Tester_Case10()
    : Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0TesterCase10(
      state_.Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0_None_instance_)
  {}
};

// op1(26:20)=100x001
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 11110100x001xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case11
    : public Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0TesterCase11 {
 public:
  Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case11()
    : Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0TesterCase11(
      state_.Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0_None_instance_)
  {}
};

// op1(26:20)=100x101 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: 11110100u101nnnn1111iiiiiiiiiiii,
//       rule: PLI_immediate_literal,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Rn},
//       violations: [implied by 'base']}
class PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0Tester_Case12
    : public PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0TesterCase12 {
 public:
  PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0Tester_Case12()
    : PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0TesterCase12(
      state_.PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0_PLI_immediate_literal_instance_)
  {}
};

// op1(26:20)=101x001 & Rn(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: base  ==
//               Pc,
//       pattern: 11110101ur01nnnn1111iiiiiiiiiiii,
//       rule: PLD_PLDW_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR],
//       uses: {Rn},
//       violations: [implied by 'base']}
class PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0Tester_Case13
    : public PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0TesterCase13 {
 public:
  PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0Tester_Case13()
    : PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0TesterCase13(
      state_.PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0_PLD_PLDW_immediate_instance_)
  {}
};

// op1(26:20)=101x001 & Rn(19:16)=1111
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 11110101x001xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case14
    : public Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0TesterCase14 {
 public:
  Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case14()
    : Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0TesterCase14(
      state_.Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0_None_instance_)
  {}
};

// op1(26:20)=101x101 & Rn(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: base  ==
//               Pc,
//       pattern: 11110101ur01nnnn1111iiiiiiiiiiii,
//       rule: PLD_PLDW_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR],
//       uses: {Rn},
//       violations: [implied by 'base']}
class PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1Tester_Case15
    : public PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1TesterCase15 {
 public:
  PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1Tester_Case15()
    : PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1TesterCase15(
      state_.PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1_PLD_PLDW_immediate_instance_)
  {}
};

// op1(26:20)=101x101 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       actual: Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1,
//       base: Pc,
//       baseline: PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0,
//       defs: {},
//       is_literal_load: true,
//       pattern: 11110101u10111111111iiiiiiiiiiii,
//       rule: PLD_literal,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
class PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0Tester_Case16
    : public PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0TesterCase16 {
 public:
  PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0Tester_Case16()
    : PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0TesterCase16(
      state_.PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0_PLD_literal_instance_)
  {}
};

// op1(26:20)=110x001 & op2(7:4)=xxx0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0,
//       defs: {},
//       pattern: 11110110x001xxxxxxxxxxxxxxx0xxxx,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
class Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0Tester_Case17
    : public Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0TesterCase17 {
 public:
  Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0Tester_Case17()
    : Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0TesterCase17(
      state_.Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0_None_instance_)
  {}
};

// op1(26:20)=110x101 & op2(7:4)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0,
//       defs: {},
//       fields: [Rn(19:16), Rm(3:0)],
//       pattern: 11110110u101nnnn1111iiiiitt0mmmm,
//       rule: PLI_register,
//       safety: [Rm  ==
//               Pc => UNPREDICTABLE,
//         true => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Rm, Rn},
//       violations: [implied by 'base']}
class PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0Tester_Case18
    : public PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0TesterCase18 {
 public:
  PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0Tester_Case18()
    : PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0TesterCase18(
      state_.PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0_PLI_register_instance_)
  {}
};

// op1(26:20)=111x001 & op2(7:4)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       R: R(22),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0,
//       defs: {},
//       fields: [R(22), Rn(19:16), Rm(3:0)],
//       is_pldw: R(22)=1,
//       pattern: 11110111u001nnnn1111iiiiitt0mmmm,
//       rule: PLD_PLDW_register,
//       safety: [Rm  ==
//               Pc ||
//            (Rn  ==
//               Pc &&
//            is_pldw) => UNPREDICTABLE,
//         true => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Rm, Rn},
//       violations: [implied by 'base']}
class PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0Tester_Case19
    : public PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0TesterCase19 {
 public:
  PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0Tester_Case19()
    : PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0TesterCase19(
      state_.PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0_PLD_PLDW_register_instance_)
  {}
};

// op1(26:20)=111x101 & op2(7:4)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       R: R(22),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0,
//       defs: {},
//       fields: [R(22), Rn(19:16), Rm(3:0)],
//       is_pldw: R(22)=1,
//       pattern: 11110111u101nnnn1111iiiiitt0mmmm,
//       rule: PLD_PLDW_register,
//       safety: [Rm  ==
//               Pc ||
//            (Rn  ==
//               Pc &&
//            is_pldw) => UNPREDICTABLE,
//         true => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Rm, Rn},
//       violations: [implied by 'base']}
class PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0Tester_Case20
    : public PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0TesterCase20 {
 public:
  PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0Tester_Case20()
    : PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0TesterCase20(
      state_.PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0_PLD_PLDW_register_instance_)
  {}
};

// op1(26:20)=1011x11
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 111101011x11xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case21
    : public Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0TesterCase21 {
 public:
  Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case21()
    : Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0TesterCase21(
      state_.Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0_None_instance_)
  {}
};

// op1(26:20)=100xx11
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 11110100xx11xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case22
    : public Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0TesterCase22 {
 public:
  Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case22()
    : Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0TesterCase22(
      state_.Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0_None_instance_)
  {}
};

// op1(26:20)=11xxx11 & op2(7:4)=xxx0
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0,
//       defs: {},
//       pattern: 1111011xxx11xxxxxxxxxxxxxxx0xxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
class Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0Tester_Case23
    : public Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0TesterCase23 {
 public:
  Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0Tester_Case23()
    : Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0TesterCase23(
      state_.Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0_None_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op1(26:20)=0010000 & op2(7:4)=0000 & Rn(19:16)=xxx1 & $pattern(31:0)=xxxxxxxxxxxx000x000000x0xxxx0000
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: SETEND_1111000100000001000000i000000000_case_0,
//       defs: {},
//       pattern: 1111000100000001000000i000000000,
//       rule: SETEND,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       SETEND_1111000100000001000000i000000000_case_0Tester_Case0_TestCase0) {
  SETEND_1111000100000001000000i000000000_case_0Tester_Case0 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_SETEND actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111000100000001000000i000000000");
}

// op1(26:20)=0010000 & op2(7:4)=xx0x & Rn(19:16)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx0000000xxxxxxxxx
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CPS_111100010000iii00000000iii0iiiii_case_0,
//       defs: {},
//       pattern: 111100010000iii00000000iii0iiiii,
//       rule: CPS,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       CPS_111100010000iii00000000iii0iiiii_case_0Tester_Case1_TestCase1) {
  CPS_111100010000iii00000000iii0iiiii_case_0Tester_Case1 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_CPS actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111100010000iii00000000iii0iiiii");
}

// op1(26:20)=1010011
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 111101010011xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case2_TestCase2) {
  Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case2 baseline_tester;
  NamedActual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101010011xxxxxxxxxxxxxxxxxxxx");
}

// op1(26:20)=1010111 & op2(7:4)=0000
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx0000xxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0Tester_Case3_TestCase3) {
  Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0Tester_Case3 baseline_tester;
  NamedActual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101010111xxxxxxxxxxxx0000xxxx");
}

// op1(26:20)=1010111 & op2(7:4)=0001 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxx1111
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: CLREX_11110101011111111111000000011111_case_0,
//       defs: {},
//       pattern: 11110101011111111111000000011111,
//       rule: CLREX,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       CLREX_11110101011111111111000000011111_case_0Tester_Case4_TestCase4) {
  CLREX_11110101011111111111000000011111_case_0Tester_Case4 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_CLREX actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110101011111111111000000011111");
}

// op1(26:20)=1010111 & op2(7:4)=0100 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx
//    = {actual: Actual_DMB_1111010101111111111100000101xxxx_case_1,
//       baseline: DSB_1111010101111111111100000100xxxx_case_0,
//       defs: {},
//       fields: [option(3:0)],
//       option: option(3:0),
//       pattern: 1111010101111111111100000100xxxx,
//       rule: DSB,
//       safety: [not option in {'1111'(3:0), '1110'(3:0), '1011'(3:0), '1010'(3:0), '0111'(3:0), '0110'(3:0), '0011'(3:0), '0010'(3:0)} => FORBIDDEN_OPERANDS],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       DSB_1111010101111111111100000100xxxx_case_0Tester_Case5_TestCase5) {
  DSB_1111010101111111111100000100xxxx_case_0Tester_Case5 baseline_tester;
  NamedActual_DMB_1111010101111111111100000101xxxx_case_1_DSB actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111010101111111111100000100xxxx");
}

// op1(26:20)=1010111 & op2(7:4)=0101 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx
//    = {actual: Actual_DMB_1111010101111111111100000101xxxx_case_1,
//       baseline: DMB_1111010101111111111100000101xxxx_case_0,
//       defs: {},
//       fields: [option(3:0)],
//       option: option(3:0),
//       pattern: 1111010101111111111100000101xxxx,
//       rule: DMB,
//       safety: [not option in {'1111'(3:0), '1110'(3:0), '1011'(3:0), '1010'(3:0), '0111'(3:0), '0110'(3:0), '0011'(3:0), '0010'(3:0)} => FORBIDDEN_OPERANDS],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       DMB_1111010101111111111100000101xxxx_case_0Tester_Case6_TestCase6) {
  DMB_1111010101111111111100000101xxxx_case_0Tester_Case6 baseline_tester;
  NamedActual_DMB_1111010101111111111100000101xxxx_case_1_DMB actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111010101111111111100000101xxxx");
}

// op1(26:20)=1010111 & op2(7:4)=0110 & $pattern(31:0)=xxxxxxxxxxxx111111110000xxxxxxxx
//    = {actual: Actual_ISB_1111010101111111111100000110xxxx_case_1,
//       baseline: ISB_1111010101111111111100000110xxxx_case_0,
//       defs: {},
//       fields: [option(3:0)],
//       option: option(3:0),
//       pattern: 1111010101111111111100000110xxxx,
//       rule: ISB,
//       safety: [option(3:0)=~1111 => FORBIDDEN_OPERANDS],
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       ISB_1111010101111111111100000110xxxx_case_0Tester_Case7_TestCase7) {
  ISB_1111010101111111111100000110xxxx_case_0Tester_Case7 baseline_tester;
  NamedActual_ISB_1111010101111111111100000110xxxx_case_1_ISB actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111010101111111111100000110xxxx");
}

// op1(26:20)=1010111 & op2(7:4)=0111
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx0111xxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0Tester_Case8_TestCase8) {
  Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0Tester_Case8 baseline_tester;
  NamedActual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101010111xxxxxxxxxxxx0111xxxx");
}

// op1(26:20)=1010111 & op2(7:4)=001x
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx001xxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0Tester_Case9_TestCase9) {
  Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0Tester_Case9 baseline_tester;
  NamedActual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101010111xxxxxxxxxxxx001xxxxx");
}

// op1(26:20)=1010111 & op2(7:4)=1xxx
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0,
//       defs: {},
//       pattern: 111101010111xxxxxxxxxxxx1xxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0Tester_Case10_TestCase10) {
  Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0Tester_Case10 baseline_tester;
  NamedActual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101010111xxxxxxxxxxxx1xxxxxxx");
}

// op1(26:20)=100x001
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 11110100x001xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case11_TestCase11) {
  Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case11 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110100x001xxxxxxxxxxxxxxxxxxxx");
}

// op1(26:20)=100x101 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: Rn  ==
//               Pc,
//       pattern: 11110100u101nnnn1111iiiiiiiiiiii,
//       rule: PLI_immediate_literal,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Rn},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0Tester_Case12_TestCase12) {
  PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0Tester_Case12 baseline_tester;
  NamedActual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1_PLI_immediate_literal actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110100u101nnnn1111iiiiiiiiiiii");
}

// op1(26:20)=101x001 & Rn(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: base  ==
//               Pc,
//       pattern: 11110101ur01nnnn1111iiiiiiiiiiii,
//       rule: PLD_PLDW_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR],
//       uses: {Rn},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0Tester_Case13_TestCase13) {
  PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0Tester_Case13 baseline_tester;
  NamedActual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1_PLD_PLDW_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110101ur01nnnn1111iiiiiiiiiiii");
}

// op1(26:20)=101x001 & Rn(19:16)=1111
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 11110101x001xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case14_TestCase14) {
  Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case14 baseline_tester;
  NamedActual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110101x001xxxxxxxxxxxxxxxxxxxx");
}

// op1(26:20)=101x101 & Rn(19:16)=~1111 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
//       defs: {},
//       fields: [Rn(19:16)],
//       is_literal_load: base  ==
//               Pc,
//       pattern: 11110101ur01nnnn1111iiiiiiiiiiii,
//       rule: PLD_PLDW_immediate,
//       safety: [Rn(19:16)=1111 => DECODER_ERROR],
//       uses: {Rn},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1Tester_Case15_TestCase15) {
  PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1Tester_Case15 baseline_tester;
  NamedActual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1_PLD_PLDW_immediate actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110101ur01nnnn1111iiiiiiiiiiii");
}

// op1(26:20)=101x101 & Rn(19:16)=1111 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       actual: Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1,
//       base: Pc,
//       baseline: PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0,
//       defs: {},
//       is_literal_load: true,
//       pattern: 11110101u10111111111iiiiiiiiiiii,
//       rule: PLD_literal,
//       safety: [true => MAY_BE_SAFE],
//       true: true,
//       uses: {Pc},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0Tester_Case16_TestCase16) {
  PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0Tester_Case16 baseline_tester;
  NamedActual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1_PLD_literal actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110101u10111111111iiiiiiiiiiii");
}

// op1(26:20)=110x001 & op2(7:4)=xxx0
//    = {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//       baseline: Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0,
//       defs: {},
//       pattern: 11110110x001xxxxxxxxxxxxxxx0xxxx,
//       safety: [true => FORBIDDEN],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0Tester_Case17_TestCase17) {
  Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0Tester_Case17 baseline_tester;
  NamedActual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110110x001xxxxxxxxxxxxxxx0xxxx");
}

// op1(26:20)=110x101 & op2(7:4)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0,
//       defs: {},
//       fields: [Rn(19:16), Rm(3:0)],
//       pattern: 11110110u101nnnn1111iiiiitt0mmmm,
//       rule: PLI_register,
//       safety: [Rm  ==
//               Pc => UNPREDICTABLE,
//         true => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Rm, Rn},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0Tester_Case18_TestCase18) {
  PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0Tester_Case18 baseline_tester;
  NamedActual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1_PLI_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110110u101nnnn1111iiiiitt0mmmm");
}

// op1(26:20)=111x001 & op2(7:4)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       R: R(22),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0,
//       defs: {},
//       fields: [R(22), Rn(19:16), Rm(3:0)],
//       is_pldw: R(22)=1,
//       pattern: 11110111u001nnnn1111iiiiitt0mmmm,
//       rule: PLD_PLDW_register,
//       safety: [Rm  ==
//               Pc ||
//            (Rn  ==
//               Pc &&
//            is_pldw) => UNPREDICTABLE,
//         true => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Rm, Rn},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0Tester_Case19_TestCase19) {
  PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0Tester_Case19 baseline_tester;
  NamedActual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1_PLD_PLDW_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110111u001nnnn1111iiiiitt0mmmm");
}

// op1(26:20)=111x101 & op2(7:4)=xxx0 & $pattern(31:0)=xxxxxxxxxxxxxxxx1111xxxxxxxxxxxx
//    = {Pc: 15,
//       R: R(22),
//       Rm: Rm(3:0),
//       Rn: Rn(19:16),
//       actual: Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1,
//       base: Rn,
//       baseline: PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0,
//       defs: {},
//       fields: [R(22), Rn(19:16), Rm(3:0)],
//       is_pldw: R(22)=1,
//       pattern: 11110111u101nnnn1111iiiiitt0mmmm,
//       rule: PLD_PLDW_register,
//       safety: [Rm  ==
//               Pc ||
//            (Rn  ==
//               Pc &&
//            is_pldw) => UNPREDICTABLE,
//         true => FORBIDDEN_OPERANDS],
//       true: true,
//       uses: {Rm, Rn},
//       violations: [implied by 'base']}
TEST_F(Arm32DecoderStateTests,
       PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0Tester_Case20_TestCase20) {
  PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0Tester_Case20 baseline_tester;
  NamedActual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1_PLD_PLDW_register actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110111u101nnnn1111iiiiitt0mmmm");
}

// op1(26:20)=1011x11
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 111101011x11xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case21_TestCase21) {
  Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case21 baseline_tester;
  NamedActual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("111101011x11xxxxxxxxxxxxxxxxxxxx");
}

// op1(26:20)=100xx11
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0,
//       defs: {},
//       pattern: 11110100xx11xxxxxxxxxxxxxxxxxxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case22_TestCase22) {
  Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0Tester_Case22 baseline_tester;
  NamedActual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("11110100xx11xxxxxxxxxxxxxxxxxxxx");
}

// op1(26:20)=11xxx11 & op2(7:4)=xxx0
//    = {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//       baseline: Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0,
//       defs: {},
//       pattern: 1111011xxx11xxxxxxxxxxxxxxx0xxxx,
//       safety: [true => UNPREDICTABLE],
//       true: true,
//       uses: {}}
TEST_F(Arm32DecoderStateTests,
       Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0Tester_Case23_TestCase23) {
  Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0Tester_Case23 baseline_tester;
  NamedActual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("1111011xxx11xxxxxxxxxxxxxxx0xxxx");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
