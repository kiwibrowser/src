/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#include "native_client/src/trusted/validator_arm/gen/arm32_decode_baselines.h"
#include "native_client/src/trusted/validator_arm/inst_classes_inline.h"

namespace nacl_arm_dec {

// ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0010101snnnnddddiiiiiiiiiiii,
//    rule: ADC_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {Rn}}
RegisterList ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0000101snnnnddddiiiiitt0mmmm,
//    rule: ADC_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      Rs(11:8),
//      type(6:5),
//      Rm(3:0)],
//    pattern: cccc0000101snnnnddddssss0tt1mmmm,
//    rule: ADC_register_shifted_register,
//    safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0010100snnnnddddiiiiiiiiiiii,
//    rule: ADD_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      (Rn(19:16)=1111 &&
//         S(20)=0) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {Rn}}
RegisterList ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // (inst(19:16)=1111 &&
  //       inst(20)=0) => DECODER_ERROR
  if ((((inst.Bits() & 0x000F0000)  ==
          0x000F0000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00000000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// ADD_register_cccc0000100snnnnddddiiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0000100snnnnddddiiiiitt0mmmm,
//    rule: ADD_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList ADD_register_cccc0000100snnnnddddiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel ADD_register_cccc0000100snnnnddddiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList ADD_register_cccc0000100snnnnddddiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      Rs(11:8),
//      type(6:5),
//      Rm(3:0)],
//    pattern: cccc0000100snnnnddddssss0tt1mmmm,
//    rule: ADD_register_shifted_register,
//    safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc001010001111ddddiiiiiiiiiiii,
//    rule: ADR_A1,
//    safety: [Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {Pc}}
RegisterList ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

// ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc001001001111ddddiiiiiiiiiiii,
//    rule: ADR_A2,
//    safety: [Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {Pc}}
RegisterList ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

// AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0010000snnnnddddiiiiiiiiiiii,
//    rule: AND_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {Rn}}
RegisterList AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// AND_register_cccc0000000snnnnddddiiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0000000snnnnddddiiiiitt0mmmm,
//    rule: AND_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList AND_register_cccc0000000snnnnddddiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel AND_register_cccc0000000snnnnddddiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList AND_register_cccc0000000snnnnddddiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      Rs(11:8),
//      type(6:5),
//      Rm(3:0)],
//    pattern: cccc0000000snnnnddddssss0tt1mmmm,
//    rule: AND_register_shifted_register,
//    safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0001101s0000ddddiiiii100mmmm,
//    rule: ASR_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rm}}
RegisterList ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//    pattern: cccc0001101s0000ddddmmmm0101nnnn,
//    rule: ASR_register,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    uses: {Rn, Rm}}
RegisterList ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// BFC_cccc0111110mmmmmddddlllll0011111_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    arch: v6T2,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), msb(20:16), Rd(15:12), lsb(11:7)],
//    lsb: lsb(11:7),
//    msb: msb(20:16),
//    pattern: cccc0111110mmmmmddddlllll0011111,
//    rule: BFC,
//    safety: [Rd  ==
//            Pc => UNPREDICTABLE,
//      msb  <
//            lsb => UNPREDICTABLE],
//    uses: {Rd}}
RegisterList BFC_cccc0111110mmmmmddddlllll0011111_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel BFC_cccc0111110mmmmmddddlllll0011111_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  // inst(20:16)  <
  //          inst(11:7) => UNPREDICTABLE
  if (((((inst.Bits() & 0x001F0000) >> 16)) < (((inst.Bits() & 0x00000F80) >> 7))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList BFC_cccc0111110mmmmmddddlllll0011111_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// BFI_cccc0111110mmmmmddddlllll001nnnn_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    arch: v6T2,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), msb(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//    lsb: lsb(11:7),
//    msb: msb(20:16),
//    pattern: cccc0111110mmmmmddddlllll001nnnn,
//    rule: BFI,
//    safety: [Rn  ==
//            Pc => DECODER_ERROR,
//      Rd  ==
//            Pc => UNPREDICTABLE,
//      msb  <
//            lsb => UNPREDICTABLE],
//    uses: {Rn, Rd}}
RegisterList BFI_cccc0111110mmmmmddddlllll001nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel BFI_cccc0111110mmmmmddddlllll001nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(3:0) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000000F)) == (15)))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  // inst(20:16)  <
  //          inst(11:7) => UNPREDICTABLE
  if (((((inst.Bits() & 0x001F0000) >> 16)) < (((inst.Bits() & 0x00000F80) >> 7))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList BFI_cccc0111110mmmmmddddlllll001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    clears_bits: (imm32 &&
//         clears_mask())  ==
//            clears_mask(),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0011110snnnnddddiiiiiiiiiiii,
//    rule: BIC_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {Rn}}
bool BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0::
clears_bits(Instruction inst, uint32_t clears_mask) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // clears_bits: '(ARMExpandImm(inst(11:0)) &&
  //       clears_mask())  ==
  //          clears_mask()'
  return ((((nacl_arm_dec::ARMExpandImm((inst.Bits() & 0x00000FFF)) & clears_mask))) == (clears_mask));
}

RegisterList BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// BIC_register_cccc0001110snnnnddddiiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0001110snnnnddddiiiiitt0mmmm,
//    rule: BIC_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList BIC_register_cccc0001110snnnnddddiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel BIC_register_cccc0001110snnnnddddiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList BIC_register_cccc0001110snnnnddddiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      Rs(11:8),
//      type(6:5),
//      Rm(3:0)],
//    pattern: cccc0001110snnnnddddssss0tt1mmmm,
//    rule: BIC_register_shifted_register,
//    safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_0:
//
//   {arch: v5T,
//    cond: cond(31:28),
//    defs: {},
//    fields: [cond(31:28), imm12(19:8), imm4(3:0)],
//    imm12: imm12(19:8),
//    imm32: ZeroExtend(imm12:imm4, 32),
//    imm4: imm4(3:0),
//    inst: inst,
//    is_literal_pool_head: inst  ==
//            LiteralPoolHeadConstant(),
//    pattern: cccc00010010iiiiiiiiiiii0111iiii,
//    rule: BKPT,
//    safety: [cond(31:28)=~1110 => UNPREDICTABLE,
//      not IsBreakPointAndConstantPoolHead(inst) => FORBIDDEN_OPERANDS],
//    uses: {},
//    violations: [implied by 'is_literal_pool_head']}
RegisterList BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

bool BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_0::
is_literal_pool_head(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_pool_head: 'LiteralPoolHeadConstant()  ==
  //          inst'
  return ((inst.Bits()) == (nacl_arm_dec::LiteralPoolHeadConstant()));
}

SafetyLevel BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(31:28)=~1110 => UNPREDICTABLE
  if ((inst.Bits() & 0xF0000000)  !=
          0xE0000000)
    return UNPREDICTABLE;

  // not IsBreakPointAndConstantPoolHead(inst) => FORBIDDEN_OPERANDS
  if (!(nacl_arm_dec::IsBreakPointAndConstantPoolHead(inst.Bits())))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

ViolationSet BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // If a pool head, mark address appropriately and then skip over
  // the constant bundle.
  validate_literal_pool_head(second, sfi, critical, next_inst_addr);

  return violations;
}


// BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 1111101hiiiiiiiiiiiiiiiiiiiiiiii,
//    rule: BLX_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// BLX_register_cccc000100101111111111110011mmmm_case_0:
//
//   {Lr: 14,
//    Pc: 15,
//    Rm: Rm(3:0),
//    arch: v5T,
//    cond: cond(31:28),
//    defs: {Pc, Lr},
//    fields: [cond(31:28), Rm(3:0)],
//    pattern: cccc000100101111111111110011mmmm,
//    rule: BLX_register,
//    safety: [Rm(3:0)=1111 => FORBIDDEN_OPERANDS],
//    target: Rm,
//    uses: {Rm},
//    violations: [implied by 'target']}
RegisterList BLX_register_cccc000100101111111111110011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{15, 14}'
  return RegisterList().
   Add(Register(15)).
   Add(Register(14));
}

SafetyLevel BLX_register_cccc000100101111111111110011mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(3:0)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000000F)  ==
          0x0000000F)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


Register BLX_register_cccc000100101111111111110011mmmm_case_0::
branch_target_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // target: 'inst(3:0)'
  return Register((inst.Bits() & 0x0000000F));
}

RegisterList BLX_register_cccc000100101111111111110011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet BLX_register_cccc000100101111111111110011mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Indirect branches (through a target register) need to be masked,
  // and if they represent a call, they need to be end-of-bundle aligned.
  violations = ViolationUnion(
      violations, get_branch_mask_violations(first, second, sfi, critical));
  violations = ViolationUnion(
      violations, get_call_position_violations(second, sfi));

  return violations;
}


// BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0:
//
//   {Cond: Cond(31:28),
//    Lr: 14,
//    Pc: 15,
//    defs: {Pc, Lr},
//    fields: [Cond(31:28), imm24(23:0)],
//    imm24: imm24(23:0),
//    imm32: SignExtend(imm24:'00'(1:0), 32),
//    pattern: cccc1011iiiiiiiiiiiiiiiiiiiiiiii,
//    relative: true,
//    relative_offset: imm32 + 8,
//    rule: BL_BLX_immediate,
//    safety: [true => MAY_BE_SAFE],
//    true: true,
//    uses: {Pc},
//    violations: [implied by 'relative']}
RegisterList BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{15, 14}'
  return RegisterList().
   Add(Register(15)).
   Add(Register(14));
}

bool BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0::
is_relative_branch(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // relative: 'true'
  return true;
}

int32_t BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0::
branch_target_offset(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // relative_offset: "SignExtend(inst(23:0):'00'(1:0), 32) + 8"
  return (((((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) & 0x02000000)
       ? ((((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) | 0xFC000000)
       : ((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) + 8;
}

SafetyLevel BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Direct (relative) branches can represent a call. If so, they need
  // to be end-of-bundle aligned.
  violations = ViolationUnion(
      violations, get_call_position_violations(second, sfi));

  return violations;
}


// BXJ_cccc000100101111111111110010mmmm_case_0:
//
//   {arch: v5TEJ,
//    defs: {},
//    pattern: cccc000100101111111111110010mmmm,
//    rule: BXJ,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList BXJ_cccc000100101111111111110010mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel BXJ_cccc000100101111111111110010mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList BXJ_cccc000100101111111111110010mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0:
//
//   {Cond: Cond(31:28),
//    Pc: 15,
//    defs: {Pc},
//    fields: [Cond(31:28), imm24(23:0)],
//    imm24: imm24(23:0),
//    imm32: SignExtend(imm24:'00'(1:0), 32),
//    pattern: cccc1010iiiiiiiiiiiiiiiiiiiiiiii,
//    relative: true,
//    relative_offset: imm32 + 8,
//    rule: B,
//    safety: [true => MAY_BE_SAFE],
//    true: true,
//    uses: {Pc},
//    violations: [implied by 'relative']}
RegisterList B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{15}'
  return RegisterList().
   Add(Register(15));
}

bool B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0::
is_relative_branch(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // relative: 'true'
  return true;
}

int32_t B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0::
branch_target_offset(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // relative_offset: "SignExtend(inst(23:0):'00'(1:0), 32) + 8"
  return (((((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) & 0x02000000)
       ? ((((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) | 0xFC000000)
       : ((((inst.Bits() & 0x00FFFFFF)) << 2) | (0 & 0x00000003))) + 8;
}

SafetyLevel B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Direct (relative) branches can represent a call. If so, they need
  // to be end-of-bundle aligned.
  violations = ViolationUnion(
      violations, get_call_position_violations(second, sfi));

  return violations;
}


// Bx_cccc000100101111111111110001mmmm_case_0:
//
//   {Pc: 15,
//    Rm: Rm(3:0),
//    arch: v4T,
//    cond: cond(31:28),
//    defs: {Pc},
//    fields: [cond(31:28), Rm(3:0)],
//    pattern: cccc000100101111111111110001mmmm,
//    rule: Bx,
//    safety: [Rm(3:0)=1111 => FORBIDDEN_OPERANDS],
//    target: Rm,
//    uses: {Rm},
//    violations: [implied by 'target']}
RegisterList Bx_cccc000100101111111111110001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{15}'
  return RegisterList().
   Add(Register(15));
}

SafetyLevel Bx_cccc000100101111111111110001mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(3:0)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000000F)  ==
          0x0000000F)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


Register Bx_cccc000100101111111111110001mmmm_case_0::
branch_target_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // target: 'inst(3:0)'
  return Register((inst.Bits() & 0x0000000F));
}

RegisterList Bx_cccc000100101111111111110001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet Bx_cccc000100101111111111110001mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Indirect branches (through a target register) need to be masked,
  // and if they represent a call, they need to be end-of-bundle aligned.
  violations = ViolationUnion(
      violations, get_branch_mask_violations(first, second, sfi, critical));
  violations = ViolationUnion(
      violations, get_call_position_violations(second, sfi));

  return violations;
}


// CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 11111110iiiiiiiiiiiiiiiiiii0iiii,
//    rule: CDP2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0:
//
//   {defs: {},
//    pattern: cccc1110oooonnnnddddccccooo0mmmm,
//    rule: CDP,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// CLREX_11110101011111111111000000011111_case_0:
//
//   {arch: V6K,
//    defs: {},
//    pattern: 11110101011111111111000000011111,
//    rule: CLREX,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList CLREX_11110101011111111111000000011111_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel CLREX_11110101011111111111000000011111_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList CLREX_11110101011111111111000000011111_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// CLZ_cccc000101101111dddd11110001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    arch: v5T,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), Rm(3:0)],
//    pattern: cccc000101101111dddd11110001mmmm,
//    rule: CLZ,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
RegisterList CLZ_cccc000101101111dddd11110001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel CLZ_cccc000101101111dddd11110001mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList CLZ_cccc000101101111dddd11110001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    Rn: Rn(19:16),
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm_C(imm12),
//    pattern: cccc00110111nnnn0000iiiiiiiiiiii,
//    rule: CMN_immediate,
//    uses: {Rn}}
RegisterList CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16}'
  return RegisterList().
   Add(Register(16));
}

SafetyLevel CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc00010111nnnn0000iiiiitt0mmmm,
//    rule: CMN_register,
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), Rs(11:8), type(6:5), Rm(3:0)],
//    pattern: cccc00010111nnnn0000ssss0tt1mmmm,
//    rule: CMN_register_shifted_register,
//    safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16}'
  return RegisterList().
   Add(Register(16));
}

SafetyLevel CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    Rn: Rn(19:16),
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm_C(imm12),
//    pattern: cccc00110101nnnn0000iiiiiiiiiiii,
//    rule: CMP_immediate,
//    uses: {Rn}}
RegisterList CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16}'
  return RegisterList().
   Add(Register(16));
}

SafetyLevel CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// CMP_register_cccc00010101nnnn0000iiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc00010101nnnn0000iiiiitt0mmmm,
//    rule: CMP_register,
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList CMP_register_cccc00010101nnnn0000iiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel CMP_register_cccc00010101nnnn0000iiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList CMP_register_cccc00010101nnnn0000iiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), Rs(11:8), type(6:5), Rm(3:0)],
//    pattern: cccc00010101nnnn0000ssss0tt1mmmm,
//    rule: CMP_register_shifted_register,
//    safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16}'
  return RegisterList().
   Add(Register(16));
}

SafetyLevel CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// CPS_111100010000iii00000000iii0iiiii_case_0:
//
//   {arch: v6,
//    defs: {},
//    pattern: 111100010000iii00000000iii0iiiii,
//    rule: CPS,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList CPS_111100010000iii00000000iii0iiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel CPS_111100010000iii00000000iii0iiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList CPS_111100010000iii00000000iii0iiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMDhp,
//    d: D:Vd,
//    defs: {},
//    elements: 4,
//    esize: 16,
//    fields: [D(22), size(19:18), Vd(15:12), op(8), M(5), Vm(3:0)],
//    half_to_single: op(8)=1,
//    m: M:Vm,
//    op: op(8),
//    pattern: 111100111d11ss10dddd011p00m0mmmm,
//    rule: CVT_between_half_precision_and_single_precision,
//    safety: [size(19:18)=~01 => UNDEFINED,
//      half_to_single &&
//         Vd(0)=1 => UNDEFINED,
//      not half_to_single &&
//         Vm(0)=1 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=~01 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00040000)
    return UNDEFINED;

  // inst(8)=1 &&
  //       inst(15:12)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000100)  ==
          0x00000100) &&
       ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  // not inst(8)=1 &&
  //       inst(3:0)(0)=1 => UNDEFINED
  if ((!((inst.Bits() & 0x00000100)  ==
          0x00000100)) &&
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// DBG_cccc001100100000111100001111iiii_case_0:
//
//   {arch: v7,
//    defs: {},
//    pattern: cccc001100100000111100001111iiii,
//    rule: DBG,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList DBG_cccc001100100000111100001111iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel DBG_cccc001100100000111100001111iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList DBG_cccc001100100000111100001111iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// DMB_1111010101111111111100000101xxxx_case_0:
//
//   {arch: v7,
//    defs: {},
//    fields: [option(3:0)],
//    option: option(3:0),
//    pattern: 1111010101111111111100000101xxxx,
//    rule: DMB,
//    safety: [not option in {'1111'(3:0), '1110'(3:0), '1011'(3:0), '1010'(3:0), '0111'(3:0), '0110'(3:0), '0011'(3:0), '0010'(3:0)} => FORBIDDEN_OPERANDS],
//    uses: {}}
RegisterList DMB_1111010101111111111100000101xxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel DMB_1111010101111111111100000101xxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // not '1111'(3:0)  ==
  //          inst(3:0) ||
  //       '1110'(3:0)  ==
  //          inst(3:0) ||
  //       '1011'(3:0)  ==
  //          inst(3:0) ||
  //       '1010'(3:0)  ==
  //          inst(3:0) ||
  //       '0111'(3:0)  ==
  //          inst(3:0) ||
  //       '0110'(3:0)  ==
  //          inst(3:0) ||
  //       '0011'(3:0)  ==
  //          inst(3:0) ||
  //       '0010'(3:0)  ==
  //          inst(3:0) => FORBIDDEN_OPERANDS
  if (!(((((inst.Bits() & 0x0000000F)) == ((15 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((14 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((11 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((10 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((7 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((6 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((3 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((2 & 0x0000000F))))))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList DMB_1111010101111111111100000101xxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// DSB_1111010101111111111100000100xxxx_case_0:
//
//   {arch: v6T2,
//    defs: {},
//    fields: [option(3:0)],
//    option: option(3:0),
//    pattern: 1111010101111111111100000100xxxx,
//    rule: DSB,
//    safety: [not option in {'1111'(3:0), '1110'(3:0), '1011'(3:0), '1010'(3:0), '0111'(3:0), '0110'(3:0), '0011'(3:0), '0010'(3:0)} => FORBIDDEN_OPERANDS],
//    uses: {}}
RegisterList DSB_1111010101111111111100000100xxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel DSB_1111010101111111111100000100xxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // not '1111'(3:0)  ==
  //          inst(3:0) ||
  //       '1110'(3:0)  ==
  //          inst(3:0) ||
  //       '1011'(3:0)  ==
  //          inst(3:0) ||
  //       '1010'(3:0)  ==
  //          inst(3:0) ||
  //       '0111'(3:0)  ==
  //          inst(3:0) ||
  //       '0110'(3:0)  ==
  //          inst(3:0) ||
  //       '0011'(3:0)  ==
  //          inst(3:0) ||
  //       '0010'(3:0)  ==
  //          inst(3:0) => FORBIDDEN_OPERANDS
  if (!(((((inst.Bits() & 0x0000000F)) == ((15 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((14 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((11 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((10 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((7 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((6 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((3 & 0x0000000F)))) ||
       ((((inst.Bits() & 0x0000000F)) == ((2 & 0x0000000F))))))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList DSB_1111010101111111111100000100xxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0010001snnnnddddiiiiiiiiiiii,
//    rule: EOR_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {Rn}}
RegisterList EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// EOR_register_cccc0000001snnnnddddiiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0000001snnnnddddiiiiitt0mmmm,
//    rule: EOR_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList EOR_register_cccc0000001snnnnddddiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel EOR_register_cccc0000001snnnnddddiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList EOR_register_cccc0000001snnnnddddiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      Rs(11:8),
//      type(6:5),
//      Rm(3:0)],
//    pattern: cccc0000001snnnnddddssss0tt1mmmm,
//    rule: EOR_register_shifted_register,
//    safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// ERET_cccc0001011000000000000001101110_case_0:
//
//   {arch: v7VE,
//    defs: {},
//    pattern: cccc0001011000000000000001101110,
//    rule: ERET,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList ERET_cccc0001011000000000000001101110_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel ERET_cccc0001011000000000000001101110_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList ERET_cccc0001011000000000000001101110_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// FICTITIOUS_FIRST_case_0:
//
//   {defs: {},
//    rule: FICTITIOUS_FIRST,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList FICTITIOUS_FIRST_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel FICTITIOUS_FIRST_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList FICTITIOUS_FIRST_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// HVC_cccc00010100iiiiiiiiiiii0111iiii_case_0:
//
//   {arch: v7VE,
//    defs: {},
//    pattern: cccc00010100iiiiiiiiiiii0111iiii,
//    rule: HVC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList HVC_cccc00010100iiiiiiiiiiii0111iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel HVC_cccc00010100iiiiiiiiiiii0111iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList HVC_cccc00010100iiiiiiiiiiii0111iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// ISB_1111010101111111111100000110xxxx_case_0:
//
//   {arch: v6T2,
//    defs: {},
//    fields: [option(3:0)],
//    option: option(3:0),
//    pattern: 1111010101111111111100000110xxxx,
//    rule: ISB,
//    safety: [option(3:0)=~1111 => FORBIDDEN_OPERANDS],
//    uses: {}}
RegisterList ISB_1111010101111111111100000110xxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel ISB_1111010101111111111100000110xxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(3:0)=~1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000000F)  !=
          0x0000000F)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList ISB_1111010101111111111100000110xxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 1111110pudw1nnnniiiiiiiiiiiiiiii,
//    rule: LDC2_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 1111110pudw11111iiiiiiiiiiiiiiii,
//    rule: LDC2_literal,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc110pudw1nnnnddddcccciiiiiiii,
//    rule: LDC_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc110pudw11111ddddcccciiiiiiii,
//    rule: LDC_literal,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0:
//
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    base: Rn,
//    cond: cond(31:28),
//    defs: Union({Rn
//         if wback
//         else None}, registers),
//    fields: [cond(31:28), W(21), Rn(19:16), register_list(15:0)],
//    pattern: cccc100000w1nnnnrrrrrrrrrrrrrrrr,
//    register_list: register_list(15:0),
//    registers: RegisterList(register_list),
//    rule: LDMDA_LDMFA,
//    safety: [Rn  ==
//            Pc ||
//         NumGPRs(registers)  <
//            1 => UNPREDICTABLE,
//      wback &&
//         Contains(registers, Rn) => UNKNOWN,
//      Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: W(21)=1}
Register LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: 'Union({inst(19:16)
  //       if inst(21)=1
  //       else 32}, RegisterList(inst(15:0)))'
  return nacl_arm_dec::Union(RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32))), nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)));
}

SafetyLevel LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       NumGPRs(RegisterList(inst(15:0)))  <
  //          1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((nacl_arm_dec::NumGPRs(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)))) < (1))))
    return UNPREDICTABLE;

  // inst(21)=1 &&
  //       Contains(RegisterList(inst(15:0)), inst(19:16)) => UNKNOWN
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(((inst.Bits() & 0x000F0000) >> 16)))))
    return UNKNOWN;

  // Contains(RegisterList(inst(15:0)), 15) => FORBIDDEN_OPERANDS
  if (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


bool LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0:
//
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    base: Rn,
//    cond: cond(31:28),
//    defs: Union({Rn
//         if wback
//         else None}, registers),
//    fields: [cond(31:28), W(21), Rn(19:16), register_list(15:0)],
//    pattern: cccc100100w1nnnnrrrrrrrrrrrrrrrr,
//    register_list: register_list(15:0),
//    registers: RegisterList(register_list),
//    rule: LDMDB_LDMEA,
//    safety: [Rn  ==
//            Pc ||
//         NumGPRs(registers)  <
//            1 => UNPREDICTABLE,
//      wback &&
//         Contains(registers, Rn) => UNKNOWN,
//      Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: W(21)=1}
Register LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: 'Union({inst(19:16)
  //       if inst(21)=1
  //       else 32}, RegisterList(inst(15:0)))'
  return nacl_arm_dec::Union(RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32))), nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)));
}

SafetyLevel LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       NumGPRs(RegisterList(inst(15:0)))  <
  //          1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((nacl_arm_dec::NumGPRs(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)))) < (1))))
    return UNPREDICTABLE;

  // inst(21)=1 &&
  //       Contains(RegisterList(inst(15:0)), inst(19:16)) => UNKNOWN
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(((inst.Bits() & 0x000F0000) >> 16)))))
    return UNKNOWN;

  // Contains(RegisterList(inst(15:0)), 15) => FORBIDDEN_OPERANDS
  if (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


bool LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0:
//
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    base: Rn,
//    cond: cond(31:28),
//    defs: Union({Rn
//         if wback
//         else None}, registers),
//    fields: [cond(31:28), W(21), Rn(19:16), register_list(15:0)],
//    pattern: cccc100110w1nnnnrrrrrrrrrrrrrrrr,
//    register_list: register_list(15:0),
//    registers: RegisterList(register_list),
//    rule: LDMIB_LDMED,
//    safety: [Rn  ==
//            Pc ||
//         NumGPRs(registers)  <
//            1 => UNPREDICTABLE,
//      wback &&
//         Contains(registers, Rn) => UNKNOWN,
//      Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: W(21)=1}
Register LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: 'Union({inst(19:16)
  //       if inst(21)=1
  //       else 32}, RegisterList(inst(15:0)))'
  return nacl_arm_dec::Union(RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32))), nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)));
}

SafetyLevel LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       NumGPRs(RegisterList(inst(15:0)))  <
  //          1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((nacl_arm_dec::NumGPRs(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)))) < (1))))
    return UNPREDICTABLE;

  // inst(21)=1 &&
  //       Contains(RegisterList(inst(15:0)), inst(19:16)) => UNKNOWN
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(((inst.Bits() & 0x000F0000) >> 16)))))
    return UNKNOWN;

  // Contains(RegisterList(inst(15:0)), 15) => FORBIDDEN_OPERANDS
  if (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


bool LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0:
//
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    base: Rn,
//    cond: cond(31:28),
//    defs: Union({Rn
//         if wback
//         else None}, registers),
//    fields: [cond(31:28), W(21), Rn(19:16), register_list(15:0)],
//    pattern: cccc100010w1nnnnrrrrrrrrrrrrrrrr,
//    register_list: register_list(15:0),
//    registers: RegisterList(register_list),
//    rule: LDM_LDMIA_LDMFD,
//    safety: [Rn  ==
//            Pc ||
//         NumGPRs(registers)  <
//            1 => UNPREDICTABLE,
//      wback &&
//         Contains(registers, Rn) => UNKNOWN,
//      Contains(registers, Pc) => FORBIDDEN_OPERANDS],
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: W(21)=1}
Register LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: 'Union({inst(19:16)
  //       if inst(21)=1
  //       else 32}, RegisterList(inst(15:0)))'
  return nacl_arm_dec::Union(RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32))), nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)));
}

SafetyLevel LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       NumGPRs(RegisterList(inst(15:0)))  <
  //          1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((nacl_arm_dec::NumGPRs(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)))) < (1))))
    return UNPREDICTABLE;

  // inst(21)=1 &&
  //       Contains(RegisterList(inst(15:0)), inst(19:16)) => UNKNOWN
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(((inst.Bits() & 0x000F0000) >> 16)))))
    return UNKNOWN;

  // Contains(RegisterList(inst(15:0)), 15) => FORBIDDEN_OPERANDS
  if (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


bool LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0:
//
//   {defs: {},
//    pattern: cccc100pu101nnnn0rrrrrrrrrrrrrrr,
//    rule: LDM_User_registers,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0:
//
//   {defs: {},
//    pattern: cccc100pu1w1nnnn1rrrrrrrrrrrrrrr,
//    rule: LDM_exception_return,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc0100u111nnnnttttiiiiiiiiiiii,
//    rule: LDRBT_A1,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0:
//
//   {defs: {},
//    pattern: cccc0110u111nnnnttttiiiiitt0mmmm,
//    rule: LDRBT_A2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ZeroExtend(imm12, 32),
//    index: P(24)=1,
//    pattern: cccc010pu1w1nnnnttttiiiiiiiiiiii,
//    rule: LDRB_immediate,
//    safety: [Rn  ==
//            Pc => DECODER_ERROR,
//      P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Rt  ==
//            Pc => UNPREDICTABLE,
//      wback &&
//         Rn  ==
//            Rt => UNPREDICTABLE],
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: P(24)=0 ||
//         W(21)=1}
Register LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) => DECODER_ERROR
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (15)))
    return DECODER_ERROR;

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(24)=0 ||
  //       inst(21)=1'
  return ((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000);
}

RegisterList LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0:
//
//   {Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    add: U(23)=1,
//    base: Pc,
//    cond: cond(31:28),
//    defs: {Rt},
//    fields: [cond(31:28), U(23), Rt(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ZeroExtend(imm12, 32),
//    is_literal_load: true,
//    pattern: cccc0101u1011111ttttiiiiiiiiiiii,
//    rule: LDRB_literal,
//    safety: [Rt  ==
//            Pc => UNPREDICTABLE],
//    true: true,
//    uses: {Pc},
//    violations: [implied by 'base']}
Register LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

bool LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    index: P(24)=1,
//    pattern: cccc011pu1w1nnnnttttiiiiitt0mmmm,
//    rule: LDRB_register,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Pc in {Rt, Rm} => UNPREDICTABLE,
//      wback &&
//         (Rn  ==
//            Pc ||
//         Rn  ==
//            Rt) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         wback &&
//         Rn  ==
//            Rm => UNPREDICTABLE,
//      index => FORBIDDEN],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rm, Rn},
//    violations: [implied by 'base'],
//    wback: P(24)=0 ||
//         W(21)=1}
Register LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    Rt2: Rt + 1,
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    address: offset_addr
//         if index
//         else Rn,
//    arch: v5TE,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, Rt2, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      imm4H(11:8),
//      imm4L(3:0)],
//    imm32: ZeroExtend(imm4H:imm4L, 32),
//    imm4H: imm4H(11:8),
//    imm4L: imm4L(3:0),
//    index: P(24)=1,
//    offset_addr: Rn + imm32
//         if add
//         else Rn - imm32,
//    pattern: cccc000pu1w0nnnnttttiiii1101iiii,
//    rule: LDRD_immediate,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR,
//      Rt(0)=1 => UNPREDICTABLE,
//      P(24)=0 &&
//         W(21)=1 => UNPREDICTABLE,
//      wback &&
//         (Rn  ==
//            Rt ||
//         Rn  ==
//            Rt2) => UNPREDICTABLE,
//      Rt2  ==
//            Pc => UNPREDICTABLE],
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
Register LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(15:12) + 1, inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1)).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // inst(15:12)(0)=1 => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNPREDICTABLE;

  // inst(24)=0 &&
  //       inst(21)=1 => UNPREDICTABLE
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12) + 1  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12) + 1))))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) + 1 => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12) + 1) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(inst(24)=0) ||
  //       (inst(21)=1)'
  return (((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000));
}

RegisterList LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0:
//
//   {P: P(24),
//    Pc: 15,
//    Rt: Rt(15:12),
//    Rt2: Rt + 1,
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    arch: v5TE,
//    base: Pc,
//    cond: cond(31:28),
//    defs: {Rt, Rt2},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rt(15:12),
//      imm4H(11:8),
//      imm4L(3:0)],
//    imm32: ZeroExtend(imm4H:imm4L, 32),
//    imm4H: imm4H(11:8),
//    imm4L: imm4L(3:0),
//    is_literal_load: true,
//    pattern: cccc0001u1001111ttttiiii1101iiii,
//    rule: LDRD_literal,
//    safety: [Rt(0)=1 => UNPREDICTABLE,
//      Rt2  ==
//            Pc => UNPREDICTABLE],
//    true: true,
//    uses: {Pc},
//    violations: [implied by 'base']}
Register LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(15:12) + 1}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1));
}

bool LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)(0)=1 => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) + 1 => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12) + 1) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    Rt2: Rt + 1,
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    arch: v5TE,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, Rt2, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      Rm(3:0)],
//    index: P(24)=1,
//    pattern: cccc000pu0w0nnnntttt00001101mmmm,
//    rule: LDRD_register,
//    safety: [Rt(0)=1 => UNPREDICTABLE,
//      P(24)=0 &&
//         W(21)=1 => UNPREDICTABLE,
//      Rt2  ==
//            Pc ||
//         Rm  ==
//            Pc ||
//         Rm  ==
//            Rt ||
//         Rm  ==
//            Rt2 => UNPREDICTABLE,
//      wback &&
//         (Rn  ==
//            Pc ||
//         Rn  ==
//            Rt ||
//         Rn  ==
//            Rt2) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         wback &&
//         Rm  ==
//            Rn => UNPREDICTABLE,
//      index => FORBIDDEN],
//    uses: {Rn, Rm},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
Register LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(15:12) + 1, inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1)).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)(0)=1 => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNPREDICTABLE;

  // inst(24)=0 &&
  //       inst(21)=1 => UNPREDICTABLE
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) + 1 ||
  //       15  ==
  //          inst(3:0) ||
  //       inst(15:12)  ==
  //          inst(3:0) ||
  //       inst(15:12) + 1  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12) + 1) == (15))) ||
       ((((inst.Bits() & 0x0000000F)) == (15))) ||
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x0000F000) >> 12) + 1))))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12) + 1  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12) + 1))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       ((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDREXB_cccc00011101nnnntttt111110011111_case_0:
//
//   {Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    arch: v6K,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt},
//    fields: [cond(31:28), Rn(19:16), Rt(15:12)],
//    imm32: Zeros((32)),
//    pattern: cccc00011101nnnntttt111110011111,
//    rule: LDREXB,
//    safety: [Pc in {Rt, Rn} => UNPREDICTABLE],
//    uses: {Rn},
//    violations: [implied by 'base']}
Register LDREXB_cccc00011101nnnntttt111110011111_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDREXB_cccc00011101nnnntttt111110011111_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel LDREXB_cccc00011101nnnntttt111110011111_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList LDREXB_cccc00011101nnnntttt111110011111_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDREXB_cccc00011101nnnntttt111110011111_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDREXD_cccc00011011nnnntttt111110011111_case_0:
//
//   {Lr: 14,
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    Rt2: Rt + 1,
//    arch: v6K,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, Rt2},
//    fields: [cond(31:28), Rn(19:16), Rt(15:12)],
//    imm32: Zeros((32)),
//    pattern: cccc00011011nnnntttt111110011111,
//    rule: LDREXD,
//    safety: [Rt(0)=1 ||
//         Rt  ==
//            Lr ||
//         Rn  ==
//            Pc => UNPREDICTABLE],
//    uses: {Rn},
//    violations: [implied by 'base']}
Register LDREXD_cccc00011011nnnntttt111110011111_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDREXD_cccc00011011nnnntttt111110011111_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(15:12) + 1}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1));
}

SafetyLevel LDREXD_cccc00011011nnnntttt111110011111_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)(0)=1 ||
  //       14  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == (14))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (15))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList LDREXD_cccc00011011nnnntttt111110011111_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDREXD_cccc00011011nnnntttt111110011111_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDREX_cccc00011001nnnntttt111110011111_case_0:
//
//   {Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    arch: v6,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt},
//    fields: [cond(31:28), Rn(19:16), Rt(15:12)],
//    imm32: Zeros((32)),
//    pattern: cccc00011001nnnntttt111110011111,
//    rule: LDREX,
//    safety: [Pc in {Rt, Rn} => UNPREDICTABLE],
//    uses: {Rn},
//    violations: [implied by 'base']}
Register LDREX_cccc00011001nnnntttt111110011111_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDREX_cccc00011001nnnntttt111110011111_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel LDREX_cccc00011001nnnntttt111110011111_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList LDREX_cccc00011001nnnntttt111110011111_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDREX_cccc00011001nnnntttt111110011111_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    address: offset_addr
//         if index
//         else Rn,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      imm4H(11:8),
//      imm4L(3:0)],
//    imm32: ZeroExtend(imm4H:imm4L, 32),
//    imm4H: imm4H(11:8),
//    imm4L: imm4L(3:0),
//    index: P(24)=1,
//    offset_addr: Rn + imm32
//         if add
//         else Rn - imm32,
//    pattern: cccc000pu1w1nnnnttttiiii1011iiii,
//    rule: LDRH_immediate,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR,
//      P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Rt  ==
//            Pc ||
//         (wback &&
//         Rn  ==
//            Rt) => UNPREDICTABLE,
//      Rt  ==
//            Pc => FORBIDDEN_OPERANDS],
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
Register LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       ((inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12)) == (15))) ||
       ((((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


bool LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(inst(24)=0) ||
  //       (inst(21)=1)'
  return (((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000));
}

RegisterList LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0:
//
//   {P: P(24),
//    Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    base: Pc,
//    cond: cond(31:28),
//    defs: {Rt},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rt(15:12),
//      imm4H(11:8),
//      imm4L(3:0)],
//    imm32: ZeroExtend(imm4H:imm4L, 32),
//    imm4H: imm4H(11:8),
//    imm4L: imm4L(3:0),
//    is_literal_load: true,
//    pattern: cccc000pu1w11111ttttiiii1011iiii,
//    rule: LDRH_literal,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      P  ==
//            W => UNPREDICTABLE,
//      Rt  ==
//            Pc => UNPREDICTABLE],
//    true: true,
//    uses: {Pc},
//    violations: [implied by 'base']}
Register LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

bool LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // inst(21)  ==
  //          inst(24) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000) >> 24)) == (((inst.Bits() & 0x00200000) >> 21))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      Rm(3:0)],
//    index: P(24)=1,
//    pattern: cccc000pu0w1nnnntttt00001011mmmm,
//    rule: LDRH_register,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Pc in {Rt, Rm} => UNPREDICTABLE,
//      wback &&
//         (Rn  ==
//            Pc ||
//         Rn  ==
//            Rt) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         wback &&
//         Rm  ==
//            Rn => UNPREDICTABLE,
//      index => FORBIDDEN],
//    shift_n: 0,
//    shift_t: SRType_LSL(),
//    uses: {Rn, Rm},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
Register LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       ((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    address: offset_addr
//         if index
//         else Rn,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      imm4H(11:8),
//      imm4L(3:0)],
//    imm32: ZeroExtend(imm4H:imm4L, 32),
//    imm4H: imm4H(11:8),
//    imm4L: imm4L(3:0),
//    index: P(24)=1,
//    offset_addr: Rn + imm32
//         if add
//         else Rn - imm32,
//    pattern: cccc000pu1w1nnnnttttiiii1101iiii,
//    rule: LDRSB_immediate,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR,
//      P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Rt  ==
//            Pc ||
//         (wback &&
//         Rn  ==
//            Rt) => UNPREDICTABLE,
//      Rt  ==
//            Pc => FORBIDDEN_OPERANDS],
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
Register LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       ((inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12)) == (15))) ||
       ((((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


bool LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(inst(24)=0) ||
  //       (inst(21)=1)'
  return (((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000));
}

RegisterList LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0:
//
//   {P: P(24),
//    Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    base: Pc,
//    cond: cond(31:28),
//    defs: {Rt},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rt(15:12),
//      imm4H(11:8),
//      imm4L(3:0)],
//    imm32: ZeroExtend(imm4H:imm4L, 32),
//    imm4H: imm4H(11:8),
//    imm4L: imm4L(3:0),
//    is_literal_load: true,
//    pattern: cccc0001u1011111ttttiiii1101iiii,
//    rule: LDRSB_literal,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      P  ==
//            W => UNPREDICTABLE,
//      Rt  ==
//            Pc => UNPREDICTABLE],
//    true: true,
//    uses: {Pc},
//    violations: [implied by 'base']}
Register LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

bool LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // inst(21)  ==
  //          inst(24) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000) >> 24)) == (((inst.Bits() & 0x00200000) >> 21))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      Rm(3:0)],
//    index: P(24)=1,
//    pattern: cccc000pu0w1nnnntttt00001101mmmm,
//    rule: LDRSB_register,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Pc in {Rt, Rm} => UNPREDICTABLE,
//      wback &&
//         (Rn  ==
//            Pc ||
//         Rn  ==
//            Rt) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         wback &&
//         Rm  ==
//            Rn => UNPREDICTABLE,
//      index => FORBIDDEN],
//    shift_n: 0,
//    shift_t: SRType_LSL(),
//    uses: {Rn, Rm},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
Register LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       ((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    address: offset_addr
//         if index
//         else Rn,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      imm4H(11:8),
//      imm4L(3:0)],
//    imm32: ZeroExtend(imm4H:imm4L, 32),
//    imm4H: imm4H(11:8),
//    imm4L: imm4L(3:0),
//    index: P(24)=1,
//    offset_addr: Rn + imm32
//         if add
//         else Rn - imm32,
//    pattern: cccc000pu1w1nnnnttttiiii1111iiii,
//    rule: LDRSH_immediate,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR,
//      P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Rt  ==
//            Pc ||
//         (wback &&
//         Rn  ==
//            Rt) => UNPREDICTABLE,
//      Rt  ==
//            Pc => FORBIDDEN_OPERANDS],
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
Register LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       ((inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12)) == (15))) ||
       ((((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


bool LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: '(inst(24)=0) ||
  //       (inst(21)=1)'
  return (((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000));
}

RegisterList LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0:
//
//   {P: P(24),
//    Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    base: Pc,
//    cond: cond(31:28),
//    defs: {Rt},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rt(15:12),
//      imm4H(11:8),
//      imm4L(3:0)],
//    imm32: ZeroExtend(imm4H:imm4L, 32),
//    imm4H: imm4H(11:8),
//    imm4L: imm4L(3:0),
//    is_literal_load: true,
//    pattern: cccc0001u1011111ttttiiii1111iiii,
//    rule: LDRSH_literal,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      P  ==
//            W => UNPREDICTABLE,
//      Rt  ==
//            Pc => UNPREDICTABLE],
//    true: true,
//    uses: {Pc},
//    violations: [implied by 'base']}
Register LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

bool LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // inst(21)  ==
  //          inst(24) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000) >> 24)) == (((inst.Bits() & 0x00200000) >> 21))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      Rm(3:0)],
//    index: P(24)=1,
//    pattern: cccc000pu0w1nnnntttt00001111mmmm,
//    rule: LDRSH_register,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Pc in {Rt, Rm} => UNPREDICTABLE,
//      wback &&
//         (Rn  ==
//            Pc ||
//         Rn  ==
//            Rt) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         wback &&
//         Rm  ==
//            Rn => UNPREDICTABLE,
//      index => FORBIDDEN],
//    shift_n: 0,
//    shift_t: SRType_LSL(),
//    uses: {Rn, Rm},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
Register LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if (((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       (inst(24)=0) ||
  //       (inst(21)=1) &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       ((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       ((((inst.Bits() & 0x0000000F)) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc0100u011nnnnttttiiiiiiiiiiii,
//    rule: LDRT_A1,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0:
//
//   {defs: {},
//    pattern: cccc0110u011nnnnttttiiiiitt0mmmm,
//    rule: LDRT_A2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    Tp: 9,
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ZeroExtend(imm12, 32),
//    index: P(24)=1,
//    is_load_tp: Rn  ==
//            Tp &&
//         index &&
//         not wback &&
//         add &&
//         imm12 in {0, 4},
//    pattern: cccc010pu0w1nnnnttttiiiiiiiiiiii,
//    rule: LDR_immediate,
//    safety: [Rn  ==
//            Pc => DECODER_ERROR,
//      P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      wback &&
//         Rn  ==
//            Rt => UNPREDICTABLE,
//      Rt  ==
//            Pc => FORBIDDEN_OPERANDS],
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    violations: [implied by 'base'],
//    wback: P(24)=0 ||
//         W(21)=1}
Register LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

bool LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0::
is_load_thread_address_pointer(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_load_tp: '9  ==
  //          inst(19:16) &&
  //       inst(24)=1 &&
  //       not inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(23)=1 &&
  //       0  ==
  //          inst(11:0) ||
  //       4  ==
  //          inst(11:0)'
  return (((((inst.Bits() & 0x000F0000) >> 16)) == (9))) &&
       ((inst.Bits() & 0x01000000)  ==
          0x01000000) &&
       (!(((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))) &&
       ((inst.Bits() & 0x00800000)  ==
          0x00800000) &&
       (((((inst.Bits() & 0x00000FFF)) == (0))) ||
       ((((inst.Bits() & 0x00000FFF)) == (4))));
}

SafetyLevel LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) => DECODER_ERROR
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (15)))
    return DECODER_ERROR;

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))
    return UNPREDICTABLE;

  // 15  ==
  //          inst(15:12) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


bool LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(24)=0 ||
  //       inst(21)=1'
  return ((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000);
}

RegisterList LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0:
//
//   {Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    add: U(23)=1,
//    base: Pc,
//    cond: cond(31:28),
//    defs: {Rt},
//    fields: [cond(31:28), U(23), Rt(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ZeroExtend(imm12, 32),
//    is_literal_load: true,
//    pattern: cccc0101u0011111ttttiiiiiiiiiiii,
//    rule: LDR_literal,
//    safety: [Rt  ==
//            Pc => FORBIDDEN_OPERANDS],
//    true: true,
//    uses: {Pc},
//    violations: [implied by 'base']}
Register LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

bool LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0:
//
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    add: U(23)=1,
//    base: Rn,
//    cond: cond(31:28),
//    defs: {Rt, base
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      W(21),
//      Rn(19:16),
//      Rt(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    index: P(24)=1,
//    pattern: cccc011pu0w1nnnnttttiiiiitt0mmmm,
//    rule: LDR_register,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Rm  ==
//            Pc => UNPREDICTABLE,
//      wback &&
//         (Rn  ==
//            Pc ||
//         Rn  ==
//            Rt) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         wback &&
//         Rn  ==
//            Rm => UNPREDICTABLE,
//      index => FORBIDDEN,
//      Rt  ==
//            Pc => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rm, Rn},
//    violations: [implied by 'base'],
//    wback: P(24)=0 ||
//         W(21)=1}
Register LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

  // 15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000000F)) == (15)))
    return UNPREDICTABLE;

  // inst(24)=0 ||
  //       inst(21)=1 &&
  //       (15  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(19:16)) => UNPREDICTABLE
  if ((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) ||
       (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12)))))))
    return UNPREDICTABLE;

  // ArchVersion()  <
  //          6 &&
  //       inst(24)=0 ||
  //       inst(21)=1 &&
  //       inst(19:16)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // inst(24)=1 => FORBIDDEN
  if ((inst.Bits() & 0x01000000)  ==
          0x01000000)
    return FORBIDDEN;

  // 15  ==
  //          inst(15:12) => FORBIDDEN_OPERANDS
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0001101s0000ddddiiiii000mmmm,
//    rule: LSL_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      imm5(11:7)=00000 => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rm}}
RegisterList LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(11:7)=00000 => DECODER_ERROR
  if ((inst.Bits() & 0x00000F80)  ==
          0x00000000)
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//    pattern: cccc0001101s0000ddddmmmm0001nnnn,
//    rule: LSL_register,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    uses: {Rn, Rm}}
RegisterList LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// LSR_immediate_cccc0001101s0000ddddiiiii010mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0001101s0000ddddiiiii010mmmm,
//    rule: LSR_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rm}}
RegisterList LSR_immediate_cccc0001101s0000ddddiiiii010mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel LSR_immediate_cccc0001101s0000ddddiiiii010mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList LSR_immediate_cccc0001101s0000ddddiiiii010mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//    pattern: cccc0001101s0000ddddmmmm0011nnnn,
//    rule: LSR_register,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    uses: {Rn, Rm}}
RegisterList LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 11111110iii0iiiittttiiiiiii1iiii,
//    rule: MCR2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0:
//
//   {arch: v6,
//    defs: {},
//    pattern: 111111000100ssssttttiiiiiiiiiiii,
//    rule: MCRR2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MCRR_cccc11000100ttttttttccccoooommmm_case_0:
//
//   {arch: v5TE,
//    defs: {},
//    pattern: cccc11000100ttttttttccccoooommmm,
//    rule: MCRR,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList MCRR_cccc11000100ttttttttccccoooommmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel MCRR_cccc11000100ttttttttccccoooommmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MCRR_cccc11000100ttttttttccccoooommmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0:
//
//   {defs: {},
//    diagnostics: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//     error('Consider using DSB (defined in ARMv7) for memory barrier')],
//    inst: inst,
//    pattern: cccc1110ooo0nnnnttttccccooo1mmmm,
//    rule: MCR,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {},
//    violations: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//     error('Consider using DSB (defined in ARMv7) for memory barrier')]}
RegisterList MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

void MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0::
generate_diagnostics(ViolationSet violations,
                     const nacl_arm_val::DecodedInstruction& first,
                     const nacl_arm_val::DecodedInstruction& second,
                     const nacl_arm_val::SfiValidator& sfi,
                     nacl_arm_val::ProblemSink* out) const {
  ClassDecoder::generate_diagnostics(violations, first, second, sfi, out);
  const Instruction& inst = second.inst();
  if (ContainsViolation(violations, OTHER_VIOLATION)) {

    // inst(31:0)=xxxx111000000111xxxx111110111010 =>
    //   error('Consider using DSB (defined in ARMv7) for memory barrier')
    if ((inst.Bits() & 0x0FFF0FFF)  ==
          0x0E070FBA) {
      out->ReportProblemDiagnostic(OTHER_VIOLATION, second.addr(),
         "Consider using DSB (defined in ARMv7) for memory barrier");
    }

  }
}


SafetyLevel MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

ViolationSet MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);
  const Instruction& inst = second.inst();

  // inst(31:0)=xxxx111000000111xxxx111110111010 =>
  //   error('Consider using DSB (defined in ARMv7) for memory barrier')
  if ((inst.Bits() & 0x0FFF0FFF)  ==
          0x0E070FBA)
     violations = ViolationUnion(violations, ViolationBit(OTHER_VIOLATION));

  return violations;
}


// MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rd(19:16),
//      Ra(15:12),
//      Rm(11:8),
//      Rn(3:0)],
//    pattern: cccc0000001sddddaaaammmm1001nnnn,
//    rule: MLA_A1,
//    safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE,
//      (ArchVersion()  <
//            6 &&
//         Rd  ==
//            Rn) => UNPREDICTABLE],
//    setflags: S(20)=1,
//    uses: {Rn, Rm, Ra}}
RegisterList MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) ||
  //       15  ==
  //          inst(15:12) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))))
    return UNPREDICTABLE;

  // (ArchVersion()  <
  //          6 &&
  //       inst(19:16)  ==
  //          inst(3:0)) => UNPREDICTABLE
  if (((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F))))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0:
//
//   {Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v6T2,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//    pattern: cccc00000110ddddaaaammmm1001nnnn,
//    rule: MLS_A1,
//    safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
RegisterList MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) ||
  //       15  ==
  //          inst(15:12) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_0:
//
//   {N: N(7),
//    Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    Vn: Vn(19:16),
//    advsimd: sel in bitset {'x1xxx', 'x0xx1'},
//    arch: ['VFPv2', 'AdvSIMD'],
//    cond: cond(31:28),
//    defs: {Rt},
//    esize: 8
//         if U:opc1:opc2(4:0)=x1xxx
//         else 16
//         if U:opc1:opc2(4:0)=x0xx1
//         else 32
//         if U:opc1:opc2(4:0)=00x00
//         else 0,
//    fields: [cond(31:28),
//      U(23),
//      opc1(22:21),
//      Vn(19:16),
//      Rt(15:12),
//      N(7),
//      opc2(6:5)],
//    index: opc1(0):opc2
//         if U:opc1:opc2(4:0)=x1xxx
//         else opc1(0):opc2(1)
//         if U:opc1:opc2(4:0)=x0xx1
//         else opc1(0)
//         if U:opc1:opc2(4:0)=00x00
//         else 0,
//    n: N:Vn,
//    opc1: opc1(22:21),
//    opc2: opc2(6:5),
//    pattern: cccc1110iii1nnnntttt1011nii10000,
//    rule: MOVE_scalar_to_ARM_core_register,
//    safety: [sel in bitset {'10x00', 'x0x10'} => UNDEFINED,
//      t  ==
//            Pc => UNPREDICTABLE],
//    sel: U:opc1:opc2,
//    t: Rt,
//    unsigned: U(23)=1}
RegisterList MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(23):inst(22:21):inst(6:5)(4:0)=10x00 ||
  //       inst(23):inst(22:21):inst(6:5)(4:0)=x0x10 => UNDEFINED
  if (((((((((((inst.Bits() & 0x00800000) >> 23)) << 2) | ((inst.Bits() & 0x00600000) >> 21))) << 2) | ((inst.Bits() & 0x00000060) >> 5)) & 0x0000001B)  ==
          0x00000010) ||
       ((((((((((inst.Bits() & 0x00800000) >> 23)) << 2) | ((inst.Bits() & 0x00600000) >> 21))) << 2) | ((inst.Bits() & 0x00000060) >> 5)) & 0x0000000B)  ==
          0x00000002))
    return UNDEFINED;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


// MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    S: S(20),
//    arch: v6T2,
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    dynamic_code_replace_immediates: {imm4, imm12},
//    fields: [cond(31:28), S(20), imm4(19:16), Rd(15:12), imm12(11:0)],
//    imm: imm4:imm12,
//    imm12: imm12(11:0),
//    imm4: imm4(19:16),
//    pattern: cccc00110100iiiiddddiiiiiiiiiiii,
//    rule: MOVT,
//    safety: [Rd(15:12)=1111 => UNPREDICTABLE],
//    uses: {}}
RegisterList MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

Instruction MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0::
dynamic_code_replacement_sentinel(
     Instruction inst) const {
  if (!defs(inst).ContainsAny(RegisterList::DynCodeReplaceFrozenRegs())) {
    // inst(19:16)
    inst.SetBits(19, 16, 0);
    // inst(11:0)
    inst.SetBits(11, 0, 0);
  }
  return inst;
}

SafetyLevel MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)=1111 => UNPREDICTABLE
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    S: S(20),
//    arch: v6T2,
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    dynamic_code_replace_immediates: {imm4, imm12},
//    fields: [cond(31:28), S(20), imm4(19:16), Rd(15:12), imm12(11:0)],
//    imm: imm4:imm12,
//    imm12: imm12(11:0),
//    imm4: imm4(19:16),
//    pattern: cccc00110000iiiiddddiiiiiiiiiiii,
//    rule: MOVW,
//    safety: [Rd(15:12)=1111 => UNPREDICTABLE],
//    uses: {}}
RegisterList MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

Instruction MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0::
dynamic_code_replacement_sentinel(
     Instruction inst) const {
  if (!defs(inst).ContainsAny(RegisterList::DynCodeReplaceFrozenRegs())) {
    // inst(19:16)
    inst.SetBits(19, 16, 0);
    // inst(11:0)
    inst.SetBits(11, 0, 0);
  }
  return inst;
}

SafetyLevel MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(15:12)=1111 => UNPREDICTABLE
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    dynamic_code_replace_immediates: {imm12},
//    fields: [cond(31:28), S(20), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0011101s0000ddddiiiiiiiiiiii,
//    rule: MOV_immediate_A1,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {}}
RegisterList MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

Instruction MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0::
dynamic_code_replacement_sentinel(
     Instruction inst) const {
  if (!defs(inst).ContainsAny(RegisterList::DynCodeReplaceFrozenRegs())) {
    // inst(11:0)
    inst.SetBits(11, 0, 0);
  }
  return inst;
}

SafetyLevel MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MOV_register_cccc0001101s0000dddd00000000mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28), S(20), Rd(15:12), Rm(3:0)],
//    pattern: cccc0001101s0000dddd00000000mmmm,
//    rule: MOV_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {Rm}}
RegisterList MOV_register_cccc0001101s0000dddd00000000mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel MOV_register_cccc0001101s0000dddd00000000mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList MOV_register_cccc0001101s0000dddd00000000mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 11111110iii1iiiittttiiiiiii1iiii,
//    rule: MRC2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0:
//
//   {defs: {},
//    pattern: cccc1110ooo1nnnnttttccccooo1mmmm,
//    rule: MRC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0:
//
//   {arch: v6,
//    defs: {},
//    pattern: 111111000101ssssttttiiiiiiiiiiii,
//    rule: MRRC2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MRRC_cccc11000101ttttttttccccoooommmm_case_0:
//
//   {arch: v5TE,
//    defs: {},
//    pattern: cccc11000101ttttttttccccoooommmm,
//    rule: MRRC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList MRRC_cccc11000101ttttttttccccoooommmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel MRRC_cccc11000101ttttttttccccoooommmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MRRC_cccc11000101ttttttttccccoooommmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MRS_Banked_register_cccc00010r00mmmmdddd001m00000000_case_0:
//
//   {arch: v7VE,
//    defs: {},
//    pattern: cccc00010r00mmmmdddd001m00000000,
//    rule: MRS_Banked_register,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList MRS_Banked_register_cccc00010r00mmmmdddd001m00000000_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel MRS_Banked_register_cccc00010r00mmmmdddd001m00000000_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MRS_Banked_register_cccc00010r00mmmmdddd001m00000000_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MRS_Banked_register_cccc00010r10mmmm1111001m0000nnnn_case_0:
//
//   {arch: v7VE,
//    defs: {},
//    pattern: cccc00010r10mmmm1111001m0000nnnn,
//    rule: MRS_Banked_register,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList MRS_Banked_register_cccc00010r10mmmm1111001m0000nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel MRS_Banked_register_cccc00010r10mmmm1111001m0000nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MRS_Banked_register_cccc00010r10mmmm1111001m0000nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MRS_cccc00010r001111dddd000000000000_case_0:
//
//   {R: R(22),
//    Rd: Rd(15:12),
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), R(22), Rd(15:12)],
//    pattern: cccc00010r001111dddd000000000000,
//    read_spsr: R(22)=1,
//    rule: MRS,
//    safety: [R(22)=1 => FORBIDDEN_OPERANDS,
//      Rd(15:12)=1111 => UNPREDICTABLE],
//    uses: {}}
RegisterList MRS_cccc00010r001111dddd000000000000_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel MRS_cccc00010r001111dddd000000000000_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(22)=1 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x00400000)  ==
          0x00400000)
    return FORBIDDEN_OPERANDS;

  // inst(15:12)=1111 => UNPREDICTABLE
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList MRS_cccc00010r001111dddd000000000000_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    cond: cond(31:28),
//    defs: {NZCV
//         if write_nzcvq
//         else None},
//    fields: [cond(31:28), mask(19:18), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    mask: mask(19:18),
//    pattern: cccc00110010mm001111iiiiiiiiiiii,
//    rule: MSR_immediate,
//    safety: [mask(19:18)=00 => DECODER_ERROR],
//    uses: {},
//    write_g: mask(0)=1,
//    write_nzcvq: mask(1)=1}
RegisterList MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16
  //       if inst(19:18)(1)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x000C0000) >> 18) & 0x00000002)  ==
          0x00000002
       ? 16
       : 32)));
}

SafetyLevel MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=00 => DECODER_ERROR
  if ((inst.Bits() & 0x000C0000)  ==
          0x00000000)
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//    rule: MSR_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MSR_register_cccc00010010mm00111100000000nnnn_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rn: Rn(3:0),
//    cond: cond(31:28),
//    defs: {NZCV
//         if write_nzcvq
//         else None},
//    fields: [cond(31:28), mask(19:18), Rn(3:0)],
//    mask: mask(19:18),
//    pattern: cccc00010010mm00111100000000nnnn,
//    rule: MSR_register,
//    safety: [mask(19:18)=00 => UNPREDICTABLE,
//      Rn  ==
//            Pc => UNPREDICTABLE],
//    uses: {Rn},
//    write_g: mask(0)=1,
//    write_nzcvq: mask(1)=1}
RegisterList MSR_register_cccc00010010mm00111100000000nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16
  //       if inst(19:18)(1)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x000C0000) >> 18) & 0x00000002)  ==
          0x00000002
       ? 16
       : 32)));
}

SafetyLevel MSR_register_cccc00010010mm00111100000000nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=00 => UNPREDICTABLE
  if ((inst.Bits() & 0x000C0000)  ==
          0x00000000)
    return UNPREDICTABLE;

  // 15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000000F)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList MSR_register_cccc00010010mm00111100000000nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// MSR_register_cccc00010r10mmmm111100000000nnnn_case_0:
//
//   {defs: {},
//    pattern: cccc00010r10mmmm111100000000nnnn,
//    rule: MSR_register,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList MSR_register_cccc00010r10mmmm111100000000nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel MSR_register_cccc00010r10mmmm111100000000nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList MSR_register_cccc00010r10mmmm111100000000nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rd(19:16), Rm(11:8), Rn(3:0)],
//    pattern: cccc0000000sdddd0000mmmm1001nnnn,
//    rule: MUL_A1,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE,
//      (ArchVersion()  <
//            6 &&
//         Rd  ==
//            Rn) => UNPREDICTABLE],
//    setflags: S(20)=1,
//    uses: {Rm, Rn}}
RegisterList MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  // (ArchVersion()  <
  //          6 &&
  //       inst(19:16)  ==
  //          inst(3:0)) => UNPREDICTABLE
  if (((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F))))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(11:8), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    dynamic_code_replace_immediates: {imm12},
//    fields: [cond(31:28), S(20), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0011111s0000ddddiiiiiiiiiiii,
//    rule: MVN_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {}}
RegisterList MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

Instruction MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0::
dynamic_code_replacement_sentinel(
     Instruction inst) const {
  if (!defs(inst).ContainsAny(RegisterList::DynCodeReplaceFrozenRegs())) {
    // inst(11:0)
    inst.SetBits(11, 0, 0);
  }
  return inst;
}

SafetyLevel MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// MVN_register_cccc0001111s0000ddddiiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0001111s0000ddddiiiiitt0mmmm,
//    rule: MVN_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rm}}
RegisterList MVN_register_cccc0001111s0000ddddiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel MVN_register_cccc0001111s0000ddddiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList MVN_register_cccc0001111s0000ddddiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rs: Rs(11:8),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rd(15:12),
//      Rs(11:8),
//      type(6:5),
//      Rm(3:0)],
//    pattern: cccc0001111s0000ddddssss0tt1mmmm,
//    rule: MVN_register_shifted_register,
//    safety: [Pc in {Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rm, Rs}}
RegisterList MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// NOP_cccc0011001000001111000000000000_case_0:
//
//   {arch: ['v6K', 'v6T2'],
//    defs: {},
//    pattern: cccc0011001000001111000000000000,
//    rule: NOP,
//    uses: {}}
RegisterList NOP_cccc0011001000001111000000000000_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel NOP_cccc0011001000001111000000000000_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList NOP_cccc0011001000001111000000000000_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// NOT_IMPLEMENTED_case_0:
//
//   {defs: {},
//    rule: NOT_IMPLEMENTED,
//    safety: [true => NOT_IMPLEMENTED],
//    true: true,
//    uses: {}}
RegisterList NOT_IMPLEMENTED_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel NOT_IMPLEMENTED_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => NOT_IMPLEMENTED
  if (true)
    return NOT_IMPLEMENTED;

  return MAY_BE_SAFE;
}


RegisterList NOT_IMPLEMENTED_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    dynamic_code_replace_immediates: {imm12},
//    fields: [cond(31:28), S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0011100snnnnddddiiiiiiiiiiii,
//    rule: ORR_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {Rn}}
RegisterList ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

Instruction ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0::
dynamic_code_replacement_sentinel(
     Instruction inst) const {
  if (!defs(inst).ContainsAny(RegisterList::DynCodeReplaceFrozenRegs())) {
    // inst(11:0)
    inst.SetBits(11, 0, 0);
  }
  return inst;
}

SafetyLevel ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// ORR_register_cccc0001100snnnnddddiiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0001100snnnnddddiiiiitt0mmmm,
//    rule: ORR_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList ORR_register_cccc0001100snnnnddddiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel ORR_register_cccc0001100snnnnddddiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList ORR_register_cccc0001100snnnnddddiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      Rs(11:8),
//      type(6:5),
//      Rm(3:0)],
//    pattern: cccc0001100snnnnddddssss0tt1mmmm,
//    rule: ORR_register_shifted_register,
//    safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28),
//      Rn(19:16),
//      Rd(15:12),
//      imm5(11:7),
//      tb(6),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc01101000nnnnddddiiiiit01mmmm,
//    rule: PKH,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    shift: DecodeImmShift(tb:'0'(0), imm5),
//    tb: tb(6),
//    tbform: tb(6)=1,
//    uses: {Rn, Rm}}
RegisterList PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0:
//
//   {Pc: 15,
//    R: R(22),
//    Rn: Rn(19:16),
//    U: U(23),
//    add: U(23)=1,
//    arch: MPExt,
//    base: Rn,
//    defs: {},
//    fields: [U(23), R(22), Rn(19:16), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ZeroExtend(imm12, 32),
//    is_literal_load: base  ==
//            Pc,
//    is_pldw: R(22)=0,
//    pattern: 11110101ur01nnnn1111iiiiiiiiiiii,
//    rule: PLD_PLDW_immediate,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR],
//    true: true,
//    uses: {Rn},
//    violations: [implied by 'base']}
Register PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

bool PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: '15  ==
  //          inst(19:16)'
  return ((((inst.Bits() & 0x000F0000) >> 16)) == (15));
}

SafetyLevel PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1:
//
//   {Pc: 15,
//    R: R(22),
//    Rn: Rn(19:16),
//    U: U(23),
//    add: U(23)=1,
//    arch: v5TE,
//    base: Rn,
//    defs: {},
//    fields: [U(23), R(22), Rn(19:16), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ZeroExtend(imm12, 32),
//    is_literal_load: base  ==
//            Pc,
//    is_pldw: R(22)=0,
//    pattern: 11110101ur01nnnn1111iiiiiiiiiiii,
//    rule: PLD_PLDW_immediate,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR],
//    true: true,
//    uses: {Rn},
//    violations: [implied by 'base']}
Register PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

bool PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: '15  ==
  //          inst(19:16)'
  return ((((inst.Bits() & 0x000F0000) >> 16)) == (15));
}

SafetyLevel PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0:
//
//   {Pc: 15,
//    R: R(22),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    U: U(23),
//    add: U(23)=1,
//    arch: MPExt,
//    base: Rn,
//    defs: {},
//    fields: [U(23), R(22), Rn(19:16), imm5(11:7), type(6:5), Rm(3:0)],
//    imm5: imm5(11:7),
//    is_pldw: R(22)=1,
//    pattern: 11110111u001nnnn1111iiiiitt0mmmm,
//    rule: PLD_PLDW_register,
//    safety: [Rm  ==
//            Pc ||
//         (Rn  ==
//            Pc &&
//         is_pldw) => UNPREDICTABLE,
//      true => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    true: true,
//    type: type(6:5),
//    uses: {Rm, Rn},
//    violations: [implied by 'base']}
Register PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(3:0) ||
  //       (15  ==
  //          inst(19:16) &&
  //       inst(22)=1) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000000F)) == (15))) ||
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) &&
       ((inst.Bits() & 0x00400000)  ==
          0x00400000))))
    return UNPREDICTABLE;

  // true => FORBIDDEN_OPERANDS
  if (true)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0:
//
//   {Pc: 15,
//    R: R(22),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    U: U(23),
//    add: U(23)=1,
//    arch: v5TE,
//    base: Rn,
//    defs: {},
//    fields: [U(23), R(22), Rn(19:16), imm5(11:7), type(6:5), Rm(3:0)],
//    imm5: imm5(11:7),
//    is_pldw: R(22)=1,
//    pattern: 11110111u101nnnn1111iiiiitt0mmmm,
//    rule: PLD_PLDW_register,
//    safety: [Rm  ==
//            Pc ||
//         (Rn  ==
//            Pc &&
//         is_pldw) => UNPREDICTABLE,
//      true => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    true: true,
//    type: type(6:5),
//    uses: {Rm, Rn},
//    violations: [implied by 'base']}
Register PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(3:0) ||
  //       (15  ==
  //          inst(19:16) &&
  //       inst(22)=1) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000000F)) == (15))) ||
       (((((((inst.Bits() & 0x000F0000) >> 16)) == (15))) &&
       ((inst.Bits() & 0x00400000)  ==
          0x00400000))))
    return UNPREDICTABLE;

  // true => FORBIDDEN_OPERANDS
  if (true)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0:
//
//   {Pc: 15,
//    U: U(23),
//    add: U(23)=1,
//    arch: v5TE,
//    base: Pc,
//    defs: {},
//    fields: [U(23), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ZeroExtend(imm12, 32),
//    is_literal_load: true,
//    pattern: 11110101u10111111111iiiiiiiiiiii,
//    rule: PLD_literal,
//    safety: [true => MAY_BE_SAFE],
//    true: true,
//    uses: {Pc},
//    violations: [implied by 'base']}
Register PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: '15'
  return Register(15);
}

RegisterList PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

bool PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: 'true'
  return true;
}

SafetyLevel PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{15}'
  return RegisterList().
   Add(Register(15));
}

ViolationSet PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0:
//
//   {Pc: 15,
//    Rn: Rn(19:16),
//    U: U(23),
//    add: U(23)=1,
//    arch: v7,
//    base: Rn,
//    defs: {},
//    fields: [U(23), Rn(19:16), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ZeroExtend(imm12, 32),
//    is_literal_load: Rn  ==
//            Pc,
//    pattern: 11110100u101nnnn1111iiiiiiiiiiii,
//    rule: PLI_immediate_literal,
//    safety: [true => MAY_BE_SAFE],
//    true: true,
//    uses: {Rn},
//    violations: [implied by 'base']}
Register PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

bool PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0::
is_literal_load(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // is_literal_load: '15  ==
  //          inst(19:16)'
  return ((((inst.Bits() & 0x000F0000) >> 16)) == (15));
}

SafetyLevel PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0:
//
//   {Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    U: U(23),
//    add: U(23)=1,
//    arch: v7,
//    base: Rn,
//    defs: {},
//    fields: [U(23), Rn(19:16), imm5(11:7), type(6:5), Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: 11110110u101nnnn1111iiiiitt0mmmm,
//    rule: PLI_register,
//    safety: [Rm  ==
//            Pc => UNPREDICTABLE,
//      true => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    true: true,
//    type: type(6:5),
//    uses: {Rm, Rn},
//    violations: [implied by 'base']}
Register PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((inst.Bits() & 0x0000000F)) == (15)))
    return UNPREDICTABLE;

  // true => FORBIDDEN_OPERANDS
  if (true)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0::
get_violations(const nacl_arm_val::DecodedInstruction& first,
               const nacl_arm_val::DecodedInstruction& second,
               const nacl_arm_val::SfiValidator& sfi,
               nacl_arm_val::AddressSet* branches,
               nacl_arm_val::AddressSet* critical,
               uint32_t* next_inst_addr) const {
  ViolationSet violations = ClassDecoder::get_violations(
      first, second, sfi, branches, critical, next_inst_addr);

  // Report unsafe loads/stores of a base address by the given instruction
  // pair.
  violations = ViolationUnion(
      violations, get_loadstore_violations(first, second, sfi, critical));

  return violations;
}


// QADD16_cccc01100010nnnndddd11110001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11110001mmmm,
//    rule: QADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList QADD16_cccc01100010nnnndddd11110001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel QADD16_cccc01100010nnnndddd11110001mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList QADD16_cccc01100010nnnndddd11110001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// QADD8_cccc01100010nnnndddd11111001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11111001mmmm,
//    rule: QADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList QADD8_cccc01100010nnnndddd11111001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel QADD8_cccc01100010nnnndddd11111001mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList QADD8_cccc01100010nnnndddd11111001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// QADD_cccc00010000nnnndddd00000101mmmm_case_0:
//
//   {Cond: Cond(31:28),
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v5TE,
//    defs: {Rd},
//    fields: [Cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc00010000nnnndddd00000101mmmm,
//    rule: QADD,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList QADD_cccc00010000nnnndddd00000101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel QADD_cccc00010000nnnndddd00000101mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList QADD_cccc00010000nnnndddd00000101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// QASX_cccc01100010nnnndddd11110011mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11110011mmmm,
//    rule: QASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList QASX_cccc01100010nnnndddd11110011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel QASX_cccc01100010nnnndddd11110011mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList QASX_cccc01100010nnnndddd11110011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// QDADD_cccc00010100nnnndddd00000101mmmm_case_0:
//
//   {Cond: Cond(31:28),
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v5TE,
//    defs: {Rd},
//    fields: [Cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc00010100nnnndddd00000101mmmm,
//    rule: QDADD,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList QDADD_cccc00010100nnnndddd00000101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel QDADD_cccc00010100nnnndddd00000101mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList QDADD_cccc00010100nnnndddd00000101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// QDSUB_cccc00010110nnnndddd00000101mmmm_case_0:
//
//   {Cond: Cond(31:28),
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v5TE,
//    defs: {Rd},
//    fields: [Cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc00010110nnnndddd00000101mmmm,
//    rule: QDSUB,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList QDSUB_cccc00010110nnnndddd00000101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel QDSUB_cccc00010110nnnndddd00000101mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList QDSUB_cccc00010110nnnndddd00000101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// QSAX_cccc01100010nnnndddd11110101mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11110101mmmm,
//    rule: QSAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList QSAX_cccc01100010nnnndddd11110101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel QSAX_cccc01100010nnnndddd11110101mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList QSAX_cccc01100010nnnndddd11110101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// QSUB16_cccc01100010nnnndddd11110111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11110111mmmm,
//    rule: QSUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList QSUB16_cccc01100010nnnndddd11110111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel QSUB16_cccc01100010nnnndddd11110111mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList QSUB16_cccc01100010nnnndddd11110111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// QSUB8_cccc01100010nnnndddd11111111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11111111mmmm,
//    rule: QSUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList QSUB8_cccc01100010nnnndddd11111111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel QSUB8_cccc01100010nnnndddd11111111mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList QSUB8_cccc01100010nnnndddd11111111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// QSUB_cccc00010010nnnndddd00000101mmmm_case_0:
//
//   {Cond: Cond(31:28),
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v5TE,
//    defs: {Rd},
//    fields: [Cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc00010010nnnndddd00000101mmmm,
//    rule: QSUB,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList QSUB_cccc00010010nnnndddd00000101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel QSUB_cccc00010010nnnndddd00000101mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList QSUB_cccc00010010nnnndddd00000101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// RBIT_cccc011011111111dddd11110011mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    arch: v6T2,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), Rm(3:0)],
//    pattern: cccc011011111111dddd11110011mmmm,
//    rule: RBIT,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
RegisterList RBIT_cccc011011111111dddd11110011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel RBIT_cccc011011111111dddd11110011mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList RBIT_cccc011011111111dddd11110011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// REV16_cccc011010111111dddd11111011mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), Rm(3:0)],
//    pattern: cccc011010111111dddd11111011mmmm,
//    rule: REV16,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
RegisterList REV16_cccc011010111111dddd11111011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel REV16_cccc011010111111dddd11111011mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList REV16_cccc011010111111dddd11111011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// REVSH_cccc011011111111dddd11111011mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), Rm(3:0)],
//    pattern: cccc011011111111dddd11111011mmmm,
//    rule: REVSH,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
RegisterList REVSH_cccc011011111111dddd11111011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel REVSH_cccc011011111111dddd11111011mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList REVSH_cccc011011111111dddd11111011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// REV_cccc011010111111dddd11110011mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), Rm(3:0)],
//    pattern: cccc011010111111dddd11110011mmmm,
//    rule: REV,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
RegisterList REV_cccc011010111111dddd11110011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel REV_cccc011010111111dddd11110011mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList REV_cccc011010111111dddd11110011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// RFE_1111100pu0w1nnnn0000101000000000_case_0:
//
//   {arch: v6,
//    defs: {},
//    pattern: 1111100pu0w1nnnn0000101000000000,
//    rule: RFE,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList RFE_1111100pu0w1nnnn0000101000000000_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel RFE_1111100pu0w1nnnn0000101000000000_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList RFE_1111100pu0w1nnnn0000101000000000_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// ROR_immediate_cccc0001101s0000ddddiiiii110mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0001101s0000ddddiiiii110mmmm,
//    rule: ROR_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      imm5(11:7)=00000 => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rm}}
RegisterList ROR_immediate_cccc0001101s0000ddddiiiii110mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel ROR_immediate_cccc0001101s0000ddddiiiii110mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(11:7)=00000 => DECODER_ERROR
  if ((inst.Bits() & 0x00000F80)  ==
          0x00000000)
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList ROR_immediate_cccc0001101s0000ddddiiiii110mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rd(15:12), Rm(11:8), Rn(3:0)],
//    pattern: cccc0001101s0000ddddmmmm0111nnnn,
//    rule: ROR_register,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    uses: {Rn, Rm}}
RegisterList ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// RRX_cccc0001101s0000dddd00000110mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28), S(20), Rd(15:12), Rm(3:0)],
//    pattern: cccc0001101s0000dddd00000110mmmm,
//    rule: RRX,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {Rm}}
RegisterList RRX_cccc0001101s0000dddd00000110mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel RRX_cccc0001101s0000dddd00000110mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList RRX_cccc0001101s0000dddd00000110mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0010011snnnnddddiiiiiiiiiiii,
//    rule: RSB_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {Rn}}
RegisterList RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// RSB_register_cccc0000011snnnnddddiiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0000011snnnnddddiiiiitt0mmmm,
//    rule: RSB_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList RSB_register_cccc0000011snnnnddddiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel RSB_register_cccc0000011snnnnddddiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList RSB_register_cccc0000011snnnnddddiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      Rs(11:8),
//      type(6:5),
//      Rm(3:0)],
//    pattern: cccc0000011snnnnddddssss0tt1mmmm,
//    rule: RSB_register_shfited_register,
//    safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0010111snnnnddddiiiiiiiiiiii,
//    rule: RSC_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {Rn}}
RegisterList RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// RSC_register_cccc0000111snnnnddddiiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0000111snnnnddddiiiiitt0mmmm,
//    rule: RSC_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList RSC_register_cccc0000111snnnnddddiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel RSC_register_cccc0000111snnnnddddiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList RSC_register_cccc0000111snnnnddddiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      Rs(11:8),
//      type(6:5),
//      Rm(3:0)],
//    pattern: cccc0000111snnnnddddssss0tt1mmmm,
//    rule: RSC_register_shifted_register,
//    safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// SADD16_cccc01100001nnnndddd11110001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11110001mmmm,
//    rule: SADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SADD16_cccc01100001nnnndddd11110001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SADD16_cccc01100001nnnndddd11110001mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SADD16_cccc01100001nnnndddd11110001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SADD8_cccc01100001nnnndddd11111001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11111001mmmm,
//    rule: SADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SADD8_cccc01100001nnnndddd11111001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SADD8_cccc01100001nnnndddd11111001mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SADD8_cccc01100001nnnndddd11111001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SASX_cccc01100001nnnndddd11110011mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11110011mmmm,
//    rule: SASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SASX_cccc01100001nnnndddd11110011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SASX_cccc01100001nnnndddd11110011mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SASX_cccc01100001nnnndddd11110011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0010110snnnnddddiiiiiiiiiiii,
//    rule: SBC_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {Rn}}
RegisterList SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// SBC_register_cccc0000110snnnnddddiiiiitt0mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if S
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      imm5(11:7),
//      type(6:5),
//      Rm(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0000110snnnnddddiiiiitt0mmmm,
//    rule: SBC_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList SBC_register_cccc0000110snnnnddddiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((((inst.Bits() & 0x00100000) >> 20) != 0)
       ? 16
       : 32)));
}

SafetyLevel SBC_register_cccc0000110snnnnddddiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // (inst(15:12)=1111 &&
  //       inst(20)=1) => DECODER_ERROR
  if ((((inst.Bits() & 0x0000F000)  ==
          0x0000F000) &&
       ((inst.Bits() & 0x00100000)  ==
          0x00100000)))
    return DECODER_ERROR;

  // inst(15:12)=1111 => FORBIDDEN_OPERANDS
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000)
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList SBC_register_cccc0000110snnnnddddiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      Rn(19:16),
//      Rd(15:12),
//      Rs(11:8),
//      type(6:5),
//      Rm(3:0)],
//    pattern: cccc0000110snnnnddddssss0tt1mmmm,
//    rule: SBC_register_shifted_register,
//    safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    arch: v6T2,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//    lsb: lsb(11:7),
//    pattern: cccc0111101wwwwwddddlllll101nnnn,
//    rule: SBFX,
//    safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//      lsb + widthm1  >
//            31 => UNPREDICTABLE],
//    uses: {Rn},
//    widthm1: widthm1(20:16)}
RegisterList SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  // 31  <=
  //          inst(11:7) + inst(20:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x00000F80) >> 7) + ((inst.Bits() & 0x001F0000) >> 16)) > (31)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0:
//
//   {M: M(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v7VEoptv7A_v7R,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110001dddd1111mmmm0001nnnn,
//    rule: SDIV,
//    safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//    uses: {Rm, Rn}}
RegisterList SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(11:8) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(11:8), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SEL_cccc01101000nnnndddd11111011mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01101000nnnndddd11111011mmmm,
//    rule: SEL,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SEL_cccc01101000nnnndddd11111011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SEL_cccc01101000nnnndddd11111011mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SEL_cccc01101000nnnndddd11111011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SETEND_1111000100000001000000i000000000_case_0:
//
//   {arch: v6,
//    defs: {},
//    pattern: 1111000100000001000000i000000000,
//    rule: SETEND,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList SETEND_1111000100000001000000i000000000_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel SETEND_1111000100000001000000i000000000_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList SETEND_1111000100000001000000i000000000_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// SEV_cccc0011001000001111000000000100_case_0:
//
//   {arch: v6K,
//    defs: {},
//    pattern: cccc0011001000001111000000000100,
//    rule: SEV,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList SEV_cccc0011001000001111000000000100_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel SEV_cccc0011001000001111000000000100_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList SEV_cccc0011001000001111000000000100_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// SHADD16_cccc01100011nnnndddd11110001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11110001mmmm,
//    rule: SHADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SHADD16_cccc01100011nnnndddd11110001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SHADD16_cccc01100011nnnndddd11110001mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SHADD16_cccc01100011nnnndddd11110001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SHADD8_cccc01100011nnnndddd11111001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11111001mmmm,
//    rule: SHADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SHADD8_cccc01100011nnnndddd11111001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SHADD8_cccc01100011nnnndddd11111001mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SHADD8_cccc01100011nnnndddd11111001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SHASX_cccc01100011nnnndddd11110011mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11110011mmmm,
//    rule: SHASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SHASX_cccc01100011nnnndddd11110011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SHASX_cccc01100011nnnndddd11110011mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SHASX_cccc01100011nnnndddd11110011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SHSAX_cccc01100011nnnndddd11110101mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11110101mmmm,
//    rule: SHSAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SHSAX_cccc01100011nnnndddd11110101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SHSAX_cccc01100011nnnndddd11110101mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SHSAX_cccc01100011nnnndddd11110101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11110111mmmm,
//    rule: SHSUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11111111mmmm,
//    rule: SHSUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SMC_cccc000101100000000000000111iiii_case_0:
//
//   {arch: SE,
//    defs: {},
//    pattern: cccc000101100000000000000111iiii,
//    rule: SMC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList SMC_cccc000101100000000000000111iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel SMC_cccc000101100000000000000111iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList SMC_cccc000101100000000000000111iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0:
//
//   {M: M(6),
//    N: N(5),
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v5TE,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28),
//      Rd(19:16),
//      Ra(15:12),
//      Rm(11:8),
//      M(6),
//      N(5),
//      Rn(3:0)],
//    m_high: M(6)=1,
//    n_high: N(5)=1,
//    pattern: cccc00010000ddddaaaammmm1xx0nnnn,
//    rule: SMLABB_SMLABT_SMLATB_SMLATT,
//    safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
RegisterList SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) ||
  //       15  ==
  //          inst(15:12) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0:
//
//   {M: M(5),
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Ra(15:12), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110000ddddaaaammmm00m1nnnn,
//    rule: SMLAD,
//    safety: [Ra  ==
//            Pc => DECODER_ERROR,
//      Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
RegisterList SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) => DECODER_ERROR
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return DECODER_ERROR;

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0:
//
//   {M: M(6),
//    N: N(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v5TE,
//    cond: cond(31:28),
//    defs: {RdLo, RdHi},
//    fields: [cond(31:28),
//      RdHi(19:16),
//      RdLo(15:12),
//      Rm(11:8),
//      M(6),
//      N(5),
//      Rn(3:0)],
//    m_high: M(6)=1,
//    n_high: N(5)=1,
//    pattern: cccc00010100hhhhllllmmmm1xx0nnnn,
//    rule: SMLALBB_SMLALBT_SMLALTB_SMLALTT,
//    safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//      RdHi  ==
//            RdLo => UNPREDICTABLE],
//    uses: {RdLo, RdHi, Rn, Rm}}
RegisterList SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0:
//
//   {M: M(5),
//    Pc: 15,
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {RdHi, RdLo},
//    fields: [cond(31:28),
//      RdHi(19:16),
//      RdLo(15:12),
//      Rm(11:8),
//      M(5),
//      Rn(3:0)],
//    pattern: cccc01110100hhhhllllmmmm00m1nnnn,
//    rule: SMLALD,
//    safety: [Pc in {RdHi, RdLo, Rn, Rm} => UNPREDICTABLE,
//      RdHi  ==
//            RdLo => UNPREDICTABLE],
//    uses: {RdHi, RdLo, Rm, Rn}}
RegisterList SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16), inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(15:12), inst(11:8), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0:
//
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    cond: cond(31:28),
//    defs: {RdLo, RdHi, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28),
//      S(20),
//      RdHi(19:16),
//      RdLo(15:12),
//      Rm(11:8),
//      Rn(3:0)],
//    pattern: cccc0000111shhhhllllmmmm1001nnnn,
//    rule: SMLAL_A1,
//    safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//      RdHi  ==
//            RdLo => UNPREDICTABLE,
//      (ArchVersion()  <
//            6 &&
//         (RdHi  ==
//            Rn ||
//         RdLo  ==
//            Rn)) => UNPREDICTABLE],
//    setflags: S(20)=1,
//    uses: {RdLo, RdHi, Rn, Rm}}
RegisterList SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16), 16
  //       if inst(20)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x00100000)  ==
          0x00100000
       ? 16
       : 32)));
}

SafetyLevel SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12))))
    return UNPREDICTABLE;

  // (ArchVersion()  <
  //          6 &&
  //       (inst(19:16)  ==
  //          inst(3:0) ||
  //       inst(15:12)  ==
  //          inst(3:0))) => UNPREDICTABLE
  if (((((nacl_arm_dec::ArchVersion()) < (6))) &&
       (((((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F))))))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0:
//
//   {M: M(6),
//    N: N(5),
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v5TE,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28),
//      Rd(19:16),
//      Ra(15:12),
//      Rm(11:8),
//      M(6),
//      N(5),
//      Rn(3:0)],
//    m_high: M(6)=1,
//    n_high: N(5)=1,
//    pattern: cccc00010010ddddaaaammmm1x00nnnn,
//    rule: SMLAWB_SMLAWT,
//    safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
RegisterList SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) ||
  //       15  ==
  //          inst(15:12) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0:
//
//   {M: M(5),
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Ra(15:12), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110000ddddaaaammmm01m1nnnn,
//    rule: SMLSD,
//    safety: [Ra  ==
//            Pc => DECODER_ERROR,
//      Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
RegisterList SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) => DECODER_ERROR
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return DECODER_ERROR;

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0:
//
//   {M: M(5),
//    Pc: 15,
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {RdHi, RdLo},
//    fields: [cond(31:28),
//      RdHi(19:16),
//      RdLo(15:12),
//      Rm(11:8),
//      M(5),
//      Rn(3:0)],
//    pattern: cccc01110100hhhhllllmmmm01m1nnnn,
//    rule: SMLSLD,
//    safety: [Pc in {RdHi, RdLo, Rn, Rm} => UNPREDICTABLE,
//      RdHi  ==
//            RdLo => UNPREDICTABLE],
//    uses: {RdHi, RdLo, Rm, Rn}}
RegisterList SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16), inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(19:16) ||
  //       15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(11:8) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) => UNPREDICTABLE
  if (((((inst.Bits() & 0x000F0000) >> 16)) == (((inst.Bits() & 0x0000F000) >> 12))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(15:12), inst(11:8), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

}  // namespace nacl_arm_dec
