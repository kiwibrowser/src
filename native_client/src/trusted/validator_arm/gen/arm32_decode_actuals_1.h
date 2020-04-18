/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_ACTUALS_1_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_ACTUALS_1_H_

#include "native_client/src/trusted/validator_arm/inst_classes.h"
#include "native_client/src/trusted/validator_arm/arm_helpers.h"

namespace nacl_arm_dec {

// Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//    baseline: ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//    baseline: AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//    baseline: EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//    baseline: RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//    baseline: RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1,
//    baseline: SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0,
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
class Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1);
};

// Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16), inst(3:0)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1,
//    baseline: ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1,
//    baseline: ADD_register_cccc0000100snnnnddddiiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1,
//    baseline: AND_register_cccc0000000snnnnddddiiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1,
//    baseline: BIC_register_cccc0001110snnnnddddiiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1,
//    baseline: EOR_register_cccc0000001snnnnddddiiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1,
//    baseline: ORR_register_cccc0001100snnnnddddiiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1,
//    baseline: RSB_register_cccc0000011snnnnddddiiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1,
//    baseline: RSC_register_cccc0000111snnnnddddiiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1,
//    baseline: SBC_register_cccc0000110snnnnddddiiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1,
//    baseline: SUB_register_cccc0000010snnnnddddiiiiitt0mmmm_case_0,
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
//    pattern: cccc0000010snnnnddddiiiiitt0mmmm,
//    rule: SUB_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
class Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1);
};

// Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(19:16), inst(3:0), inst(11:8)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//    baseline: ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//    baseline: ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//    baseline: AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//    baseline: BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//    baseline: EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//    baseline: ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//    baseline: RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//    baseline: RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//    baseline: SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    S: S(20),
//    actual: Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1,
//    baseline: SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0,
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
//    pattern: cccc0000010snnnnddddssss0tt1mmmm,
//    rule: SUB_register_shifted_register,
//    safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
class Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1);
};

// Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      (inst(19:16)=1111 &&
//         inst(20)=0) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1,
//    baseline: ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1,
//    baseline: SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0,
//    cond: cond(31:28),
//    defs: {Rd, NZCV
//         if setflags
//         else None},
//    fields: [cond(31:28), S(20), Rn(19:16), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc0010010snnnnddddiiiiiiiiiiii,
//    rule: SUB_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      (Rn(19:16)=1111 &&
//         S(20)=0) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {Rn}}
class Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1);
};

// Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {15}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    actual: Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1,
//    baseline: ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc001010001111ddddiiiiiiiiiiii,
//    rule: ADR_A1,
//    safety: [Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {Pc}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    actual: Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1,
//    baseline: ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm(imm12),
//    pattern: cccc001001001111ddddiiiiiiiiiiii,
//    rule: ADR_A2,
//    safety: [Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {Pc}}
class Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1);
};

// Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(3:0)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    actual: Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1,
//    baseline: ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    actual: Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1,
//    baseline: LSR_immediate_cccc0001101s0000ddddiiiii010mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    actual: Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1,
//    baseline: MOV_register_cccc0001101s0000dddd00000000mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    actual: Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1,
//    baseline: MVN_register_cccc0001111s0000ddddiiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    actual: Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1,
//    baseline: RRX_cccc0001101s0000dddd00000110mmmm_case_0,
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
class Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1);
};

// Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//    baseline: ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//    baseline: LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//    baseline: LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rs: Rs(11:8),
//    S: S(20),
//    actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//    baseline: MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    actual: Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1,
//    baseline: ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0,
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
class Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1);
};

// Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(20:16)  <
//            inst(11:7) => UNPREDICTABLE],
//    uses: {inst(15:12)}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    actual: Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1,
//    arch: v6T2,
//    baseline: BFC_cccc0111110mmmmmddddlllll0011111_case_0,
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
class Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1
     : public ClassDecoder {
 public:
  Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_BFC_cccc0111110mmmmmddddlllll0011111_case_1);
};

// Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      15  ==
//            inst(3:0) => DECODER_ERROR,
//      inst(20:16)  <
//            inst(11:7) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(15:12)}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    actual: Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1,
//    arch: v6T2,
//    baseline: BFI_cccc0111110mmmmmddddlllll001nnnn_case_0,
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
class Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_BFI_cccc0111110mmmmmddddlllll001nnnn_case_1);
};

// Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1
//
// Actual:
//   {clears_bits: (ARMExpandImm(inst(11:0)) &&
//         clears_mask())  ==
//            clears_mask(),
//    defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1,
//    baseline: BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0,
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
class Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual bool clears_bits(Instruction i, uint32_t clears_mask) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1);
};

// Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1
//
// Actual:
//   {defs: {},
//    is_literal_pool_head: LiteralPoolHeadConstant()  ==
//            inst,
//    safety: [inst(31:28)=~1110 => UNPREDICTABLE,
//      not IsBreakPointAndConstantPoolHead(inst) => FORBIDDEN_OPERANDS],
//    uses: {},
//    violations: [implied by 'is_literal_pool_head']}
//
// Baseline:
//   {actual: Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1,
//    arch: v5T,
//    baseline: BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_0,
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
class Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1
     : public ClassDecoder {
 public:
  Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_pool_head(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1);
};

// Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => FORBIDDEN],
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v5,
//    baseline: BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: 1111101hiiiiiiiiiiiiiiiiiiiiiiii,
//    rule: BLX_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v5TEJ,
//    baseline: BXJ_cccc000100101111111111110010mmmm_case_0,
//    defs: {},
//    pattern: cccc000100101111111111110010mmmm,
//    rule: BXJ,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v5,
//    baseline: CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0,
//    defs: {},
//    pattern: 11111110iiiiiiiiiiiiiiiiiii0iiii,
//    rule: CDP2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0,
//    defs: {},
//    pattern: cccc1110oooonnnnddddccccooo0mmmm,
//    rule: CDP,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: V6K,
//    baseline: CLREX_11110101011111111111000000011111_case_0,
//    defs: {},
//    pattern: 11110101011111111111000000011111,
//    rule: CLREX,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v6,
//    baseline: CPS_111100010000iii00000000iii0iiiii_case_0,
//    defs: {},
//    pattern: 111100010000iii00000000iii0iiiii,
//    rule: CPS,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v7,
//    baseline: DBG_cccc001100100000111100001111iiii_case_0,
//    defs: {},
//    pattern: cccc001100100000111100001111iiii,
//    rule: DBG,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v7VE,
//    baseline: ERET_cccc0001011000000000000001101110_case_0,
//    defs: {},
//    pattern: cccc0001011000000000000001101110,
//    rule: ERET,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: FICTITIOUS_FIRST_case_0,
//    defs: {},
//    rule: FICTITIOUS_FIRST,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v7VE,
//    baseline: HVC_cccc00010100iiiiiiiiiiii0111iiii_case_0,
//    defs: {},
//    pattern: cccc00010100iiiiiiiiiiii0111iiii,
//    rule: HVC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v5,
//    baseline: LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: 1111110pudw1nnnniiiiiiiiiiiiiiii,
//    rule: LDC2_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v5,
//    baseline: LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: 1111110pudw11111iiiiiiiiiiiiiiii,
//    rule: LDC2_literal,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0,
//    defs: {},
//    pattern: cccc110pudw1nnnnddddcccciiiiiiii,
//    rule: LDC_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0,
//    defs: {},
//    pattern: cccc110pudw11111ddddcccciiiiiiii,
//    rule: LDC_literal,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0,
//    defs: {},
//    pattern: cccc100pu101nnnn0rrrrrrrrrrrrrrr,
//    rule: LDM_User_registers,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0,
//    defs: {},
//    pattern: cccc100pu1w1nnnn1rrrrrrrrrrrrrrr,
//    rule: LDM_exception_return,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: cccc0100u111nnnnttttiiiiiiiiiiii,
//    rule: LDRBT_A1,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0,
//    defs: {},
//    pattern: cccc0110u111nnnnttttiiiiitt0mmmm,
//    rule: LDRBT_A2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: cccc0100u011nnnnttttiiiiiiiiiiii,
//    rule: LDRT_A1,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0,
//    defs: {},
//    pattern: cccc0110u011nnnnttttiiiiitt0mmmm,
//    rule: LDRT_A2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v5,
//    baseline: MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0,
//    defs: {},
//    pattern: 11111110iii0iiiittttiiiiiii1iiii,
//    rule: MCR2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v6,
//    baseline: MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: 111111000100ssssttttiiiiiiiiiiii,
//    rule: MCRR2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v5TE,
//    baseline: MCRR_cccc11000100ttttttttccccoooommmm_case_0,
//    defs: {},
//    pattern: cccc11000100ttttttttccccoooommmm,
//    rule: MCRR,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v5,
//    baseline: MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0,
//    defs: {},
//    pattern: 11111110iii1iiiittttiiiiiii1iiii,
//    rule: MRC2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0,
//    defs: {},
//    pattern: cccc1110ooo1nnnnttttccccooo1mmmm,
//    rule: MRC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v6,
//    baseline: MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: 111111000101ssssttttiiiiiiiiiiii,
//    rule: MRRC2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v5TE,
//    baseline: MRRC_cccc11000101ttttttttccccoooommmm_case_0,
//    defs: {},
//    pattern: cccc11000101ttttttttccccoooommmm,
//    rule: MRRC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v7VE,
//    baseline: MRS_Banked_register_cccc00010r00mmmmdddd001m00000000_case_0,
//    defs: {},
//    pattern: cccc00010r00mmmmdddd001m00000000,
//    rule: MRS_Banked_register,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v7VE,
//    baseline: MRS_Banked_register_cccc00010r10mmmm1111001m0000nnnn_case_0,
//    defs: {},
//    pattern: cccc00010r10mmmm1111001m0000nnnn,
//    rule: MRS_Banked_register,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//    rule: MSR_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: MSR_register_cccc00010r10mmmm111100000000nnnn_case_0,
//    defs: {},
//    pattern: cccc00010r10mmmm111100000000nnnn,
//    rule: MSR_register,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v6,
//    baseline: RFE_1111100pu0w1nnnn0000101000000000_case_0,
//    defs: {},
//    pattern: 1111100pu0w1nnnn0000101000000000,
//    rule: RFE,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v6,
//    baseline: SETEND_1111000100000001000000i000000000_case_0,
//    defs: {},
//    pattern: 1111000100000001000000i000000000,
//    rule: SETEND,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v6K,
//    baseline: SEV_cccc0011001000001111000000000100_case_0,
//    defs: {},
//    pattern: cccc0011001000001111000000000100,
//    rule: SEV,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: SE,
//    baseline: SMC_cccc000101100000000000000111iiii_case_0,
//    defs: {},
//    pattern: cccc000101100000000000000111iiii,
//    rule: SMC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v6,
//    baseline: SRS_1111100pu1w0110100000101000iiiii_case_0,
//    defs: {},
//    pattern: 1111100pu1w0110100000101000iiiii,
//    rule: SRS,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v5,
//    baseline: STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: 1111110pudw0nnnniiiiiiiiiiiiiiii,
//    rule: STC2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0,
//    defs: {},
//    pattern: cccc110pudw0nnnnddddcccciiiiiiii,
//    rule: STC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0,
//    defs: {},
//    pattern: cccc100pu100nnnnrrrrrrrrrrrrrrrr,
//    rule: STM_User_registers,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: cccc0100u110nnnnttttiiiiiiiiiiii,
//    rule: STRBT_A1,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0,
//    defs: {},
//    pattern: cccc0110u110nnnnttttiiiiitt0mmmm,
//    rule: STRBT_A2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: cccc0100u010nnnnttttiiiiiiiiiiii,
//    rule: STRT_A1,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0,
//    defs: {},
//    pattern: cccc0110u010nnnnttttiiiiitt0mmmm,
//    rule: STRT_A2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0,
//    defs: {},
//    pattern: cccc1111iiiiiiiiiiiiiiiiiiiiiiii,
//    rule: SVC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: Unnamed_case_0,
//    defs: {},
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: MPExt,
//    baseline: Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0,
//    defs: {},
//    pattern: 11110100x001xxxxxxxxxxxxxxxxxxxx,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: MPExt,
//    baseline: Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0,
//    defs: {},
//    pattern: 11110110x001xxxxxxxxxxxxxxx0xxxx,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v6K,
//    baseline: WFE_cccc0011001000001111000000000010_case_0,
//    defs: {},
//    pattern: cccc0011001000001111000000000010,
//    rule: WFE,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    arch: v6K,
//    baseline: WFI_cccc0011001000001111000000000011_case_0,
//    defs: {},
//    pattern: cccc0011001000001111000000000011,
//    rule: WFI,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: extra_load_store_instructions_unpriviledged_cccc0000xx1xxxxxxxxxxxxx1xx1xxxx_case_0,
//    defs: {},
//    pattern: cccc0000xx1xxxxxxxxxxxxx1xx1xxxx,
//    rule: extra_load_store_instructions_unpriviledged,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1);
};

// Actual_BLX_register_cccc000100101111111111110011mmmm_case_1
//
// Actual:
//   {defs: {15, 14},
//    safety: [inst(3:0)=1111 => FORBIDDEN_OPERANDS],
//    target: inst(3:0),
//    uses: {inst(3:0)},
//    violations: [implied by 'target']}
//
// Baseline:
//   {Lr: 14,
//    Pc: 15,
//    Rm: Rm(3:0),
//    actual: Actual_BLX_register_cccc000100101111111111110011mmmm_case_1,
//    arch: v5T,
//    baseline: BLX_register_cccc000100101111111111110011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Pc, Lr},
//    fields: [cond(31:28), Rm(3:0)],
//    pattern: cccc000100101111111111110011mmmm,
//    rule: BLX_register,
//    safety: [Rm(3:0)=1111 => FORBIDDEN_OPERANDS],
//    target: Rm,
//    uses: {Rm},
//    violations: [implied by 'target']}
class Actual_BLX_register_cccc000100101111111111110011mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_BLX_register_cccc000100101111111111110011mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual Register branch_target_register(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_BLX_register_cccc000100101111111111110011mmmm_case_1);
};

// Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {15, 14},
//    relative: true,
//    relative_offset: SignExtend(inst(23:0):'00'(1:0), 32) + 8,
//    safety: [true => MAY_BE_SAFE],
//    uses: {15},
//    violations: [implied by 'relative']}
//
// Baseline:
//   {Cond: Cond(31:28),
//    Lr: 14,
//    Pc: 15,
//    actual: Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0,
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
class Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_relative_branch(Instruction i) const;
  virtual int32_t branch_target_offset(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_1);
};

// Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {15},
//    relative: true,
//    relative_offset: SignExtend(inst(23:0):'00'(1:0), 32) + 8,
//    safety: [true => MAY_BE_SAFE],
//    uses: {15},
//    violations: [implied by 'relative']}
//
// Baseline:
//   {Cond: Cond(31:28),
//    Pc: 15,
//    actual: Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1,
//    baseline: B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0,
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
class Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_relative_branch(Instruction i) const;
  virtual int32_t branch_target_offset(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_1);
};

// Actual_Bx_cccc000100101111111111110001mmmm_case_1
//
// Actual:
//   {defs: {15},
//    safety: [inst(3:0)=1111 => FORBIDDEN_OPERANDS],
//    target: inst(3:0),
//    uses: {inst(3:0)},
//    violations: [implied by 'target']}
//
// Baseline:
//   {Pc: 15,
//    Rm: Rm(3:0),
//    actual: Actual_Bx_cccc000100101111111111110001mmmm_case_1,
//    arch: v4T,
//    baseline: Bx_cccc000100101111111111110001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Pc},
//    fields: [cond(31:28), Rm(3:0)],
//    pattern: cccc000100101111111111110001mmmm,
//    rule: Bx,
//    safety: [Rm(3:0)=1111 => FORBIDDEN_OPERANDS],
//    target: Rm,
//    uses: {Rm},
//    violations: [implied by 'target']}
class Actual_Bx_cccc000100101111111111110001mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_Bx_cccc000100101111111111110001mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual Register branch_target_register(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_Bx_cccc000100101111111111110001mmmm_case_1);
};

// Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE],
//    uses: {inst(3:0)}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v5T,
//    baseline: CLZ_cccc000101101111dddd11110001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), Rm(3:0)],
//    pattern: cccc000101101111dddd11110001mmmm,
//    rule: CLZ,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6T2,
//    baseline: RBIT_cccc011011111111dddd11110011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), Rm(3:0)],
//    pattern: cccc011011111111dddd11110011mmmm,
//    rule: RBIT,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: REV16_cccc011010111111dddd11111011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), Rm(3:0)],
//    pattern: cccc011010111111dddd11111011mmmm,
//    rule: REV16,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: REVSH_cccc011011111111dddd11111011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), Rm(3:0)],
//    pattern: cccc011011111111dddd11111011mmmm,
//    rule: REVSH,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: REV_cccc011010111111dddd11110011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), Rm(3:0)],
//    pattern: cccc011010111111dddd11110011mmmm,
//    rule: REV,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: SSAT16_cccc01101010iiiidddd11110011nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), sat_imm(19:16), Rd(15:12), Rn(3:0)],
//    pattern: cccc01101010iiiidddd11110011nnnn,
//    rule: SSAT16,
//    safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//    sat_imm: sat_imm(19:16),
//    saturate_to: sat_imm + 1,
//    uses: {Rn}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28),
//      sat_imm(20:16),
//      Rd(15:12),
//      imm5(11:7),
//      sh(6),
//      Rn(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0110101iiiiiddddiiiiis01nnnn,
//    rule: SSAT,
//    safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//    sat_imm: sat_imm(20:16),
//    saturate_to: sat_imm + 1,
//    sh: sh(6),
//    shift: DecodeImmShift(sh:'0'(0), imm5),
//    uses: {Rn}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: SXTB16_cccc011010001111ddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011010001111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: SXTB16,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: SXTB_cccc011010101111ddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011010101111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: SXTB,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: SXTH_cccc011010111111ddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011010111111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: SXTH,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: USAT16_cccc01101110iiiidddd11110011nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), sat_imm(19:16), Rd(15:12), Rn(3:0)],
//    pattern: cccc01101110iiiidddd11110011nnnn,
//    rule: USAT16,
//    safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//    sat_imm: sat_imm(19:16),
//    saturate_to: sat_imm,
//    uses: {Rn}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28),
//      sat_imm(20:16),
//      Rd(15:12),
//      imm5(11:7),
//      sh(6),
//      Rn(3:0)],
//    imm5: imm5(11:7),
//    pattern: cccc0110111iiiiiddddiiiiis01nnnn,
//    rule: USAT,
//    safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//    sat_imm: sat_imm(20:16),
//    saturate_to: sat_imm,
//    sh: sh(6),
//    shift: DecodeImmShift(sh:'0'(0), imm5),
//    uses: {Rn}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: UXTB16_cccc011011001111ddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011011001111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: UXTB16,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: UXTB_cccc011011101111ddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011011101111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: UXTB,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    actual: Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1,
//    arch: v6,
//    baseline: UXTH_cccc011011111111ddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011011111111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: UXTH,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
class Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1);
};

// Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {16},
//    uses: {inst(19:16)}}
//
// Baseline:
//   {NZCV: 16,
//    Rn: Rn(19:16),
//    actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//    baseline: CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0,
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm_C(imm12),
//    pattern: cccc00110111nnnn0000iiiiiiiiiiii,
//    rule: CMN_immediate,
//    uses: {Rn}}
//
// Baseline:
//   {NZCV: 16,
//    Rn: Rn(19:16),
//    actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//    baseline: CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0,
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm_C(imm12),
//    pattern: cccc00110101nnnn0000iiiiiiiiiiii,
//    rule: CMP_immediate,
//    uses: {Rn}}
//
// Baseline:
//   {NZCV: 16,
//    Rn: Rn(19:16),
//    actual: Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1,
//    baseline: TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0,
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm_C(imm12),
//    pattern: cccc00110011nnnn0000iiiiiiiiiiii,
//    rule: TEQ_immediate,
//    uses: {Rn}}
class Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1);
};

// Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1
//
// Actual:
//   {defs: {16
//         if inst(20)
//         else 32},
//    uses: {inst(19:16), inst(3:0)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1,
//    baseline: CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1,
//    baseline: CMP_register_cccc00010101nnnn0000iiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1,
//    baseline: TEQ_register_cccc00010011nnnn0000iiiiitt0mmmm_case_0,
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
//    pattern: cccc00010011nnnn0000iiiiitt0mmmm,
//    rule: TEQ_register,
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1,
//    baseline: TST_register_cccc00010001nnnn0000iiiiitt0mmmm_case_0,
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
//    pattern: cccc00010001nnnn0000iiiiitt0mmmm,
//    rule: TST_register,
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
class Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1);
};

// Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1
//
// Actual:
//   {defs: {16},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(19:16), inst(3:0), inst(11:8)}}
//
// Baseline:
//   {NZCV: 16,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//    baseline: CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0,
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), Rs(11:8), type(6:5), Rm(3:0)],
//    pattern: cccc00010111nnnn0000ssss0tt1mmmm,
//    rule: CMN_register_shifted_register,
//    safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
//
// Baseline:
//   {NZCV: 16,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//    baseline: CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0,
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), Rs(11:8), type(6:5), Rm(3:0)],
//    pattern: cccc00010101nnnn0000ssss0tt1mmmm,
//    rule: CMP_register_shifted_register,
//    safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
//
// Baseline:
//   {NZCV: 16,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//    baseline: TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0,
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), Rs(11:8), type(6:5), Rm(3:0)],
//    pattern: cccc00010011nnnn0000ssss0tt1mmmm,
//    rule: TEQ_register_shifted_register,
//    safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
//
// Baseline:
//   {NZCV: 16,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    actual: Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1,
//    baseline: TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0,
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), Rs(11:8), type(6:5), Rm(3:0)],
//    pattern: cccc00010001nnnn0000ssss0tt1mmmm,
//    rule: TST_register_shifted_register,
//    safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
class Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1);
};

// Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=~01 => UNDEFINED,
//      inst(8)=1 &&
//         inst(15:12)(0)=1 => UNDEFINED,
//      not inst(8)=1 &&
//         inst(3:0)(0)=1 => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    actual: Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1,
//    arch: ASIMDhp,
//    baseline: CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0,
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
class Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_1);
};

// Actual_DMB_1111010101111111111100000101xxxx_case_1
//
// Actual:
//   {defs: {},
//    safety: [not '1111'(3:0)  ==
//            inst(3:0) ||
//         '1110'(3:0)  ==
//            inst(3:0) ||
//         '1011'(3:0)  ==
//            inst(3:0) ||
//         '1010'(3:0)  ==
//            inst(3:0) ||
//         '0111'(3:0)  ==
//            inst(3:0) ||
//         '0110'(3:0)  ==
//            inst(3:0) ||
//         '0011'(3:0)  ==
//            inst(3:0) ||
//         '0010'(3:0)  ==
//            inst(3:0) => FORBIDDEN_OPERANDS],
//    uses: {}}
//
// Baseline:
//   {actual: Actual_DMB_1111010101111111111100000101xxxx_case_1,
//    arch: v7,
//    baseline: DMB_1111010101111111111100000101xxxx_case_0,
//    defs: {},
//    fields: [option(3:0)],
//    option: option(3:0),
//    pattern: 1111010101111111111100000101xxxx,
//    rule: DMB,
//    safety: [not option in {'1111'(3:0), '1110'(3:0), '1011'(3:0), '1010'(3:0), '0111'(3:0), '0110'(3:0), '0011'(3:0), '0010'(3:0)} => FORBIDDEN_OPERANDS],
//    uses: {}}
//
// Baseline:
//   {actual: Actual_DMB_1111010101111111111100000101xxxx_case_1,
//    arch: v6T2,
//    baseline: DSB_1111010101111111111100000100xxxx_case_0,
//    defs: {},
//    fields: [option(3:0)],
//    option: option(3:0),
//    pattern: 1111010101111111111100000100xxxx,
//    rule: DSB,
//    safety: [not option in {'1111'(3:0), '1110'(3:0), '1011'(3:0), '1010'(3:0), '0111'(3:0), '0110'(3:0), '0011'(3:0), '0010'(3:0)} => FORBIDDEN_OPERANDS],
//    uses: {}}
class Actual_DMB_1111010101111111111100000101xxxx_case_1
     : public ClassDecoder {
 public:
  Actual_DMB_1111010101111111111100000101xxxx_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_DMB_1111010101111111111100000101xxxx_case_1);
};

// Actual_ISB_1111010101111111111100000110xxxx_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(3:0)=~1111 => FORBIDDEN_OPERANDS],
//    uses: {}}
//
// Baseline:
//   {actual: Actual_ISB_1111010101111111111100000110xxxx_case_1,
//    arch: v6T2,
//    baseline: ISB_1111010101111111111100000110xxxx_case_0,
//    defs: {},
//    fields: [option(3:0)],
//    option: option(3:0),
//    pattern: 1111010101111111111100000110xxxx,
//    rule: ISB,
//    safety: [option(3:0)=~1111 => FORBIDDEN_OPERANDS],
//    uses: {}}
class Actual_ISB_1111010101111111111100000110xxxx_case_1
     : public ClassDecoder {
 public:
  Actual_ISB_1111010101111111111100000110xxxx_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_ISB_1111010101111111111100000110xxxx_case_1);
};

// Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: Union({inst(19:16)
//         if inst(21)=1
//         else 32}, RegisterList(inst(15:0))),
//    safety: [15  ==
//            inst(19:16) ||
//         NumGPRs(RegisterList(inst(15:0)))  <
//            1 => UNPREDICTABLE,
//      Contains(RegisterList(inst(15:0)), 15) => FORBIDDEN_OPERANDS,
//      inst(21)=1 &&
//         Contains(RegisterList(inst(15:0)), inst(19:16)) => UNKNOWN],
//    small_imm_base_wb: inst(21)=1,
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//    base: Rn,
//    baseline: LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0,
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
//
// Baseline:
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//    base: Rn,
//    baseline: LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0,
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
//
// Baseline:
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//    base: Rn,
//    baseline: LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0,
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
//
// Baseline:
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    actual: Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1,
//    base: Rn,
//    baseline: LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0,
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
class Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1
     : public ClassDecoder {
 public:
  Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_1);
};

// Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      15  ==
//            inst(19:16) => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    small_imm_base_wb: inst(24)=0 ||
//         inst(21)=1,
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0,
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
class Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_1);
};

// Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: 15,
//    defs: {inst(15:12)},
//    is_literal_load: true,
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE],
//    uses: {15},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    actual: Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1,
//    add: U(23)=1,
//    base: Pc,
//    baseline: LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0,
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
class Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_1);
};

// Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         inst(24)=0 ||
//         inst(21)=1 &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(3:0), inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0,
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
class Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_1);
};

// Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(15:12) + 1, inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12) + 1  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) + 1 => UNPREDICTABLE,
//      inst(15:12)(0)=1 => UNPREDICTABLE,
//      inst(19:16)=1111 => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(21)=1 => UNPREDICTABLE],
//    small_imm_base_wb: (inst(24)=0) ||
//         (inst(21)=1),
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    Rt2: Rt + 1,
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1,
//    add: U(23)=1,
//    address: offset_addr
//         if index
//         else Rn,
//    arch: v5TE,
//    base: Rn,
//    baseline: LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0,
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
class Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1
     : public ClassDecoder {
 public:
  Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1);
};

// Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1
//
// Actual:
//   {base: 15,
//    defs: {inst(15:12), inst(15:12) + 1},
//    is_literal_load: true,
//    safety: [15  ==
//            inst(15:12) + 1 => UNPREDICTABLE,
//      inst(15:12)(0)=1 => UNPREDICTABLE],
//    uses: {15},
//    violations: [implied by 'base']}
//
// Baseline:
//   {P: P(24),
//    Pc: 15,
//    Rt: Rt(15:12),
//    Rt2: Rt + 1,
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1,
//    add: U(23)=1,
//    arch: v5TE,
//    base: Pc,
//    baseline: LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0,
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
class Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1
     : public ClassDecoder {
 public:
  Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1);
};

// Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(15:12) + 1, inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12) + 1  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) + 1 ||
//         15  ==
//            inst(3:0) ||
//         inst(15:12)  ==
//            inst(3:0) ||
//         inst(15:12) + 1  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         (inst(24)=0) ||
//         (inst(21)=1) &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(15:12)(0)=1 => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(19:16), inst(3:0)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    Rt2: Rt + 1,
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1,
//    add: U(23)=1,
//    arch: v5TE,
//    base: Rn,
//    baseline: LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0,
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
class Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1);
};

// Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    actual: Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1,
//    arch: v6K,
//    base: Rn,
//    baseline: LDREXB_cccc00011101nnnntttt111110011111_case_0,
//    cond: cond(31:28),
//    defs: {Rt},
//    fields: [cond(31:28), Rn(19:16), Rt(15:12)],
//    imm32: Zeros((32)),
//    pattern: cccc00011101nnnntttt111110011111,
//    rule: LDREXB,
//    safety: [Pc in {Rt, Rn} => UNPREDICTABLE],
//    uses: {Rn},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    actual: Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1,
//    arch: v6,
//    base: Rn,
//    baseline: LDREX_cccc00011001nnnntttt111110011111_case_0,
//    cond: cond(31:28),
//    defs: {Rt},
//    fields: [cond(31:28), Rn(19:16), Rt(15:12)],
//    imm32: Zeros((32)),
//    pattern: cccc00011001nnnntttt111110011111,
//    rule: LDREX,
//    safety: [Pc in {Rt, Rn} => UNPREDICTABLE],
//    uses: {Rn},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    actual: Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1,
//    arch: v6K,
//    base: Rn,
//    baseline: STREXH_cccc00011111nnnntttt111110011111_case_0,
//    cond: cond(31:28),
//    defs: {Rt},
//    fields: [cond(31:28), Rn(19:16), Rt(15:12)],
//    imm32: Zeros((32)),
//    pattern: cccc00011111nnnntttt111110011111,
//    rule: STREXH,
//    safety: [Pc in {Rt, Rn} => UNPREDICTABLE],
//    uses: {Rn},
//    violations: [implied by 'base']}
class Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1
     : public ClassDecoder {
 public:
  Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1);
};

// Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(15:12) + 1},
//    safety: [inst(15:12)(0)=1 ||
//         14  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Lr: 14,
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    Rt2: Rt + 1,
//    actual: Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1,
//    arch: v6K,
//    base: Rn,
//    baseline: LDREXD_cccc00011011nnnntttt111110011111_case_0,
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
class Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1
     : public ClassDecoder {
 public:
  Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1);
};

// Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [15  ==
//            inst(15:12) => FORBIDDEN_OPERANDS,
//      15  ==
//            inst(15:12) ||
//         ((inst(24)=0) ||
//         (inst(21)=1) &&
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      inst(19:16)=1111 => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR],
//    small_imm_base_wb: (inst(24)=0) ||
//         (inst(21)=1),
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//    add: U(23)=1,
//    address: offset_addr
//         if index
//         else Rn,
//    base: Rn,
//    baseline: LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0,
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
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//    add: U(23)=1,
//    address: offset_addr
//         if index
//         else Rn,
//    base: Rn,
//    baseline: LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0,
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
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1,
//    add: U(23)=1,
//    address: offset_addr
//         if index
//         else Rn,
//    base: Rn,
//    baseline: LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0,
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
class Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1
     : public ClassDecoder {
 public:
  Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1);
};

// Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1
//
// Actual:
//   {base: 15,
//    defs: {inst(15:12)},
//    is_literal_load: true,
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(21)  ==
//            inst(24) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR],
//    uses: {15},
//    violations: [implied by 'base']}
//
// Baseline:
//   {P: P(24),
//    Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//    add: U(23)=1,
//    base: Pc,
//    baseline: LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0,
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
//
// Baseline:
//   {P: P(24),
//    Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//    add: U(23)=1,
//    base: Pc,
//    baseline: LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0,
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
//
// Baseline:
//   {P: P(24),
//    Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1,
//    add: U(23)=1,
//    base: Pc,
//    baseline: LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0,
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
class Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1
     : public ClassDecoder {
 public:
  Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1);
};

// Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         (inst(24)=0) ||
//         (inst(21)=1) &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(19:16), inst(3:0)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0,
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
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0,
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
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0,
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
class Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1);
};

// Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    is_load_tp: 9  ==
//            inst(19:16) &&
//         inst(24)=1 &&
//         not inst(24)=0 ||
//         inst(21)=1 &&
//         inst(23)=1 &&
//         0  ==
//            inst(11:0) ||
//         4  ==
//            inst(11:0),
//    safety: [15  ==
//            inst(15:12) => FORBIDDEN_OPERANDS,
//      15  ==
//            inst(19:16) => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    small_imm_base_wb: inst(24)=0 ||
//         inst(21)=1,
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    Tp: 9,
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0,
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
class Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_load_thread_address_pointer(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_1);
};

// Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: 15,
//    defs: {inst(15:12)},
//    is_literal_load: true,
//    safety: [15  ==
//            inst(15:12) => FORBIDDEN_OPERANDS],
//    uses: {15},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    actual: Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1,
//    add: U(23)=1,
//    base: Pc,
//    baseline: LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0,
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
class Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_1);
};

// Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12), inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) => FORBIDDEN_OPERANDS,
//      15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         inst(24)=0 ||
//         inst(21)=1 &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(3:0), inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0,
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
class Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_1);
};

// Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)
//         else 32},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(11:7)=00000 => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(3:0)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    actual: Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1,
//    baseline: LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    S: S(20),
//    actual: Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1,
//    baseline: ROR_immediate_cccc0001101s0000ddddiiiii110mmmm_case_0,
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
class Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1);
};

// Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1
//
// Actual:
//   {defs: {},
//    diagnostics: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//     error('Consider using DSB (defined in ARMv7) for memory barrier')],
//    safety: [true => FORBIDDEN],
//    uses: {},
//    violations: [inst(31:0)=xxxx111000000111xxxx111110111010 =>
//     error('Consider using DSB (defined in ARMv7) for memory barrier')]}
//
// Baseline:
//   {actual: Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1,
//    baseline: MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0,
//    defs: {},
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
class Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual void generate_diagnostics(
      ViolationSet violations,
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::ProblemSink* out) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_1);
};

// Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16), 16
//         if inst(20)=1
//         else 32},
//    safety: [(ArchVersion()  <
//            6 &&
//         inst(19:16)  ==
//            inst(3:0)) => UNPREDICTABLE,
//      15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) ||
//         15  ==
//            inst(15:12) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8), inst(15:12)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    actual: Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1,
//    baseline: MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0,
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
class Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1);
};

// Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16)},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) ||
//         15  ==
//            inst(15:12) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8), inst(15:12)}}
//
// Baseline:
//   {Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//    arch: v6T2,
//    baseline: MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//    pattern: cccc00000110ddddaaaammmm1001nnnn,
//    rule: MLS_A1,
//    safety: [Pc in {Rd, Rn, Rm, Ra} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
//
// Baseline:
//   {M: M(6),
//    N: N(5),
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//    arch: v5TE,
//    baseline: SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0,
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
//
// Baseline:
//   {M: M(6),
//    N: N(5),
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1,
//    arch: v5TE,
//    baseline: SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0,
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
class Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1);
};

// Actual_MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(23):inst(22:21):inst(6:5)(4:0)=10x00 ||
//         inst(23):inst(22:21):inst(6:5)(4:0)=x0x10 => UNDEFINED]}
//
// Baseline:
//   {N: N(7),
//    Pc: 15,
//    Rt: Rt(15:12),
//    U: U(23),
//    Vn: Vn(19:16),
//    actual: Actual_MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_1,
//    advsimd: sel in bitset {'x1xxx', 'x0xx1'},
//    arch: ['VFPv2', 'AdvSIMD'],
//    baseline: MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_0,
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
class Actual_MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_1
     : public ClassDecoder {
 public:
  Actual_MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_1);
};

// Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)
//         else 32},
//    dynamic_code_replace_immediates: {inst(19:16), inst(11:0)},
//    safety: [inst(15:12)=1111 => UNPREDICTABLE],
//    uses: {}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    S: S(20),
//    actual: Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1,
//    arch: v6T2,
//    baseline: MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    S: S(20),
//    actual: Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1,
//    arch: v6T2,
//    baseline: MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0,
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
class Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual Instruction dynamic_code_replacement_sentinel(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1);
};

// Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    dynamic_code_replace_immediates: {inst(11:0)},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    S: S(20),
//    actual: Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1,
//    baseline: MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    S: S(20),
//    actual: Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1,
//    baseline: MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0,
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
class Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual Instruction dynamic_code_replacement_sentinel(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1);
};

// Actual_MRS_cccc00010r001111dddd000000000000_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [inst(15:12)=1111 => UNPREDICTABLE,
//      inst(22)=1 => FORBIDDEN_OPERANDS],
//    uses: {}}
//
// Baseline:
//   {R: R(22),
//    Rd: Rd(15:12),
//    actual: Actual_MRS_cccc00010r001111dddd000000000000_case_1,
//    baseline: MRS_cccc00010r001111dddd000000000000_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), R(22), Rd(15:12)],
//    pattern: cccc00010r001111dddd000000000000,
//    read_spsr: R(22)=1,
//    rule: MRS,
//    safety: [R(22)=1 => FORBIDDEN_OPERANDS,
//      Rd(15:12)=1111 => UNPREDICTABLE],
//    uses: {}}
class Actual_MRS_cccc00010r001111dddd000000000000_case_1
     : public ClassDecoder {
 public:
  Actual_MRS_cccc00010r001111dddd000000000000_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_MRS_cccc00010r001111dddd000000000000_case_1);
};

// Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {16
//         if inst(19:18)(1)=1
//         else 32},
//    safety: [inst(19:18)=00 => DECODER_ERROR],
//    uses: {}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    actual: Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1,
//    baseline: MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0,
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
class Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1);
};

// Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1
//
// Actual:
//   {defs: {16
//         if inst(19:18)(1)=1
//         else 32},
//    safety: [15  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(19:18)=00 => UNPREDICTABLE],
//    uses: {inst(3:0)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rn: Rn(3:0),
//    actual: Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1,
//    baseline: MSR_register_cccc00010010mm00111100000000nnnn_case_0,
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
class Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1);
};

// Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16), 16
//         if inst(20)=1
//         else 32},
//    safety: [(ArchVersion()  <
//            6 &&
//         inst(19:16)  ==
//            inst(3:0)) => UNPREDICTABLE,
//      15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(11:8), inst(3:0)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    actual: Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1,
//    baseline: MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0,
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
class Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1);
};

// Actual_NOP_cccc0011001000001111000000000000_case_1
//
// Actual:
//   {defs: {},
//    uses: {}}
//
// Baseline:
//   {actual: Actual_NOP_cccc0011001000001111000000000000_case_1,
//    arch: ['v6K', 'v6T2'],
//    baseline: NOP_cccc0011001000001111000000000000_case_0,
//    defs: {},
//    pattern: cccc0011001000001111000000000000,
//    rule: NOP,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_NOP_cccc0011001000001111000000000000_case_1,
//    arch: v6K,
//    baseline: YIELD_cccc0011001000001111000000000001_case_0,
//    defs: {},
//    pattern: cccc0011001000001111000000000001,
//    rule: YIELD,
//    uses: {}}
class Actual_NOP_cccc0011001000001111000000000000_case_1
     : public ClassDecoder {
 public:
  Actual_NOP_cccc0011001000001111000000000000_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_NOP_cccc0011001000001111000000000000_case_1);
};

// Actual_NOT_IMPLEMENTED_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => NOT_IMPLEMENTED],
//    uses: {}}
//
// Baseline:
//   {actual: Actual_NOT_IMPLEMENTED_case_1,
//    baseline: NOT_IMPLEMENTED_case_0,
//    defs: {},
//    rule: NOT_IMPLEMENTED,
//    safety: [true => NOT_IMPLEMENTED],
//    true: true,
//    uses: {}}
class Actual_NOT_IMPLEMENTED_case_1
     : public ClassDecoder {
 public:
  Actual_NOT_IMPLEMENTED_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_NOT_IMPLEMENTED_case_1);
};

// Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {inst(15:12), 16
//         if inst(20)=1
//         else 32},
//    dynamic_code_replace_immediates: {inst(11:0)},
//    safety: [(inst(15:12)=1111 &&
//         inst(20)=1) => DECODER_ERROR,
//      inst(15:12)=1111 => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    S: S(20),
//    actual: Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1,
//    baseline: ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0,
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
class Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual Instruction dynamic_code_replacement_sentinel(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1);
};

// Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE],
//    uses: {inst(19:16), inst(3:0)}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0,
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
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: QADD16_cccc01100010nnnndddd11110001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11110001mmmm,
//    rule: QADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: QADD8_cccc01100010nnnndddd11111001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11111001mmmm,
//    rule: QADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Cond: Cond(31:28),
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v5TE,
//    baseline: QADD_cccc00010000nnnndddd00000101mmmm_case_0,
//    defs: {Rd},
//    fields: [Cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc00010000nnnndddd00000101mmmm,
//    rule: QADD,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: QASX_cccc01100010nnnndddd11110011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11110011mmmm,
//    rule: QASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Cond: Cond(31:28),
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v5TE,
//    baseline: QDADD_cccc00010100nnnndddd00000101mmmm_case_0,
//    defs: {Rd},
//    fields: [Cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc00010100nnnndddd00000101mmmm,
//    rule: QDADD,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Cond: Cond(31:28),
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v5TE,
//    baseline: QDSUB_cccc00010110nnnndddd00000101mmmm_case_0,
//    defs: {Rd},
//    fields: [Cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc00010110nnnndddd00000101mmmm,
//    rule: QDSUB,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: QSAX_cccc01100010nnnndddd11110101mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11110101mmmm,
//    rule: QSAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: QSUB16_cccc01100010nnnndddd11110111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11110111mmmm,
//    rule: QSUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: QSUB8_cccc01100010nnnndddd11111111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100010nnnndddd11111111mmmm,
//    rule: QSUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Cond: Cond(31:28),
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v5TE,
//    baseline: QSUB_cccc00010010nnnndddd00000101mmmm_case_0,
//    defs: {Rd},
//    fields: [Cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc00010010nnnndddd00000101mmmm,
//    rule: QSUB,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SADD16_cccc01100001nnnndddd11110001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11110001mmmm,
//    rule: SADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SADD8_cccc01100001nnnndddd11111001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11111001mmmm,
//    rule: SADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SASX_cccc01100001nnnndddd11110011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11110011mmmm,
//    rule: SASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SEL_cccc01101000nnnndddd11111011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01101000nnnndddd11111011mmmm,
//    rule: SEL,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SHADD16_cccc01100011nnnndddd11110001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11110001mmmm,
//    rule: SHADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SHADD8_cccc01100011nnnndddd11111001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11111001mmmm,
//    rule: SHADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SHASX_cccc01100011nnnndddd11110011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11110011mmmm,
//    rule: SHASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SHSAX_cccc01100011nnnndddd11110101mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11110101mmmm,
//    rule: SHSAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11110111mmmm,
//    rule: SHSUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100011nnnndddd11111111mmmm,
//    rule: SHSUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SSAX_cccc01100001nnnndddd11110101mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11110101mmmm,
//    rule: SSAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11110111mmmm,
//    rule: SSSUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: SSUB8_cccc01100001nnnndddd11111111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11111111mmmm,
//    rule: SSUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UADD16_cccc01100101nnnndddd11110001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11110001mmmm,
//    rule: UADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UADD8_cccc01100101nnnndddd11111001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11111001mmmm,
//    rule: UADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UASX_cccc01100101nnnndddd11110011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11110011mmmm,
//    rule: UASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UHADD16_cccc01100111nnnndddd11110001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11110001mmmm,
//    rule: UHADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UHADD8_cccc01100111nnnndddd11111001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11111001mmmm,
//    rule: UHADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UHASX_cccc01100111nnnndddd11110011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11110011mmmm,
//    rule: UHASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UHSAX_cccc01100111nnnndddd11110101mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11110101mmmm,
//    rule: UHSAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11110111mmmm,
//    rule: UHSUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11111111mmmm,
//    rule: UHSUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UQADD16_cccc01100110nnnndddd11110001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11110001mmmm,
//    rule: UQADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UQADD8_cccc01100110nnnndddd11111001mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11111001mmmm,
//    rule: UQADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UQASX_cccc01100110nnnndddd11110011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11110011mmmm,
//    rule: UQASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UQSAX_cccc01100110nnnndddd11110101mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11110101mmmm,
//    rule: UQSAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11110111mmmm,
//    rule: UQSUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11111111mmmm,
//    rule: UQSUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: USAX_cccc01100101nnnndddd11110101mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11110101mmmm,
//    rule: USAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: USUB16_cccc01100101nnnndddd11110111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11110111mmmm,
//    rule: USUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1,
//    arch: v6,
//    baseline: USUB8_cccc01100101nnnndddd11111111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11111111mmmm,
//    rule: USUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
class Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1);
};

// Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {},
//    is_literal_load: 15  ==
//            inst(19:16),
//    safety: [inst(19:16)=1111 => DECODER_ERROR],
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    R: R(22),
//    Rn: Rn(19:16),
//    U: U(23),
//    actual: Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
//    add: U(23)=1,
//    arch: MPExt,
//    base: Rn,
//    baseline: PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0,
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
//
// Baseline:
//   {Pc: 15,
//    R: R(22),
//    Rn: Rn(19:16),
//    U: U(23),
//    actual: Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
//    add: U(23)=1,
//    arch: v5TE,
//    base: Rn,
//    baseline: PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1,
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
class Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1);
};

// Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {},
//    safety: [15  ==
//            inst(3:0) ||
//         (15  ==
//            inst(19:16) &&
//         inst(22)=1) => UNPREDICTABLE,
//      true => FORBIDDEN_OPERANDS],
//    uses: {inst(3:0), inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    R: R(22),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    U: U(23),
//    actual: Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1,
//    add: U(23)=1,
//    arch: MPExt,
//    base: Rn,
//    baseline: PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0,
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
//
// Baseline:
//   {Pc: 15,
//    R: R(22),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    U: U(23),
//    actual: Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1,
//    add: U(23)=1,
//    arch: v5TE,
//    base: Rn,
//    baseline: PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0,
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
class Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_1);
};

// Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1
//
// Actual:
//   {base: 15,
//    defs: {},
//    is_literal_load: true,
//    safety: [true => MAY_BE_SAFE],
//    uses: {15},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    U: U(23),
//    actual: Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1,
//    add: U(23)=1,
//    arch: v5TE,
//    base: Pc,
//    baseline: PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0,
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
class Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_PLD_literal_11110101u10111111111iiiiiiiiiiii_case_1);
};

// Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {},
//    is_literal_load: 15  ==
//            inst(19:16),
//    safety: [true => MAY_BE_SAFE],
//    uses: {inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    Rn: Rn(19:16),
//    U: U(23),
//    actual: Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1,
//    add: U(23)=1,
//    arch: v7,
//    base: Rn,
//    baseline: PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0,
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
class Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_1);
};

// Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {},
//    safety: [15  ==
//            inst(3:0) => UNPREDICTABLE,
//      true => FORBIDDEN_OPERANDS],
//    uses: {inst(3:0), inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    U: U(23),
//    actual: Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1,
//    add: U(23)=1,
//    arch: v7,
//    base: Rn,
//    baseline: PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0,
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
class Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_1);
};

// Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      31  <=
//            inst(11:7) + inst(20:16) => UNPREDICTABLE],
//    uses: {inst(3:0)}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    actual: Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1,
//    arch: v6T2,
//    baseline: SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0,
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
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    actual: Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1,
//    arch: v6T2,
//    baseline: UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//    lsb: lsb(11:7),
//    pattern: cccc0111111mmmmmddddlllll101nnnn,
//    rule: UBFX,
//    safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//      lsb + widthm1  >
//            31 => UNPREDICTABLE],
//    uses: {Rn},
//    widthm1: widthm1(20:16)}
class Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_SBFX_cccc0111101wwwwwddddlllll101nnnn_case_1);
};

// Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16)},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(11:8) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE],
//    uses: {inst(11:8), inst(3:0)}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//    arch: v7VEoptv7A_v7R,
//    baseline: SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110001dddd1111mmmm0001nnnn,
//    rule: SDIV,
//    safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//    uses: {Rm, Rn}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//    arch: v6,
//    baseline: SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110101dddd1111mmmm00r1nnnn,
//    rule: SMMUL,
//    safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//    uses: {Rm, Rn}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//    arch: v6,
//    baseline: SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110000dddd1111mmmm00m1nnnn,
//    rule: SMUAD,
//    safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//    uses: {Rm, Rn}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//    arch: v6,
//    baseline: SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110000dddd1111mmmm01m1nnnn,
//    rule: SMUSD,
//    safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//    uses: {Rm, Rn}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1,
//    arch: v7VEoptv7A_v7R,
//    baseline: UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110011dddd1111mmmm0001nnnn,
//    rule: UDIV,
//    safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//    uses: {Rm, Rn}}
class Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_SDIV_cccc01110001dddd1111mmmm0001nnnn_case_1);
};

// Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16)},
//    safety: [15  ==
//            inst(15:12) => DECODER_ERROR,
//      15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8), inst(15:12)}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//    arch: v6,
//    baseline: SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Ra(15:12), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110000ddddaaaammmm00m1nnnn,
//    rule: SMLAD,
//    safety: [Ra  ==
//            Pc => DECODER_ERROR,
//      Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//    arch: v6,
//    baseline: SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Ra(15:12), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110000ddddaaaammmm01m1nnnn,
//    rule: SMLSD,
//    safety: [Ra  ==
//            Pc => DECODER_ERROR,
//      Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//    arch: v6,
//    baseline: SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Ra(15:12), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110101ddddaaaammmm00r1nnnn,
//    rule: SMMLA,
//    safety: [Ra  ==
//            Pc => DECODER_ERROR,
//      Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//    arch: v6,
//    baseline: SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Ra(15:12), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110101ddddaaaammmm11r1nnnn,
//    rule: SMMLS,
//    safety: [Ra  ==
//            Pc => DECODER_ERROR,
//      Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
//
// Baseline:
//   {Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1,
//    arch: v6,
//    baseline: USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//    pattern: cccc01111000ddddaaaammmm0001nnnn,
//    rule: USADA8,
//    safety: [Ra  ==
//            Pc => DECODER_ERROR,
//      Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
class Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_1);
};

// Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12), inst(19:16)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(15:12), inst(19:16), inst(3:0), inst(11:8)}}
//
// Baseline:
//   {M: M(6),
//    N: N(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1,
//    arch: v5TE,
//    baseline: SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0,
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
//
// Baseline:
//   {Pc: 15,
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1,
//    baseline: UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0,
//    cond: cond(31:28),
//    defs: {RdLo, RdHi},
//    fields: [cond(31:28), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//    pattern: cccc00000100hhhhllllmmmm1001nnnn,
//    rule: UMAAL_A1,
//    safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//      RdHi  ==
//            RdLo => UNPREDICTABLE],
//    uses: {RdLo, RdHi, Rn, Rm}}
class Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1);
};

// Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16), inst(15:12)},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(19:16), inst(15:12), inst(11:8), inst(3:0)}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1,
//    arch: v6,
//    baseline: SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0,
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
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1,
//    arch: v6,
//    baseline: SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0,
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
class Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_1);
};

// Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12), inst(19:16), 16
//         if inst(20)=1
//         else 32},
//    safety: [(ArchVersion()  <
//            6 &&
//         (inst(19:16)  ==
//            inst(3:0) ||
//         inst(15:12)  ==
//            inst(3:0))) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(15:12), inst(19:16), inst(3:0), inst(11:8)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    actual: Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1,
//    baseline: SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0,
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
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    actual: Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1,
//    baseline: UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0,
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
//    pattern: cccc0000101shhhhllllmmmm1001nnnn,
//    rule: UMLAL_A1,
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
class Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1);
};

// Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1
//
// Actual:
//   {defs: {inst(19:16)},
//    safety: [15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8)}}
//
// Baseline:
//   {M: M(6),
//    N: N(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//    arch: v5TE,
//    baseline: SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(6), N(5), Rn(3:0)],
//    m_high: M(6)=1,
//    n_high: N(5)=1,
//    pattern: cccc00010110dddd0000mmmm1xx0nnnn,
//    rule: SMULBB_SMULBT_SMULTB_SMULTT,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {M: M(6),
//    N: N(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//    arch: v5TE,
//    baseline: SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(6), N(5), Rn(3:0)],
//    m_high: M(6)=1,
//    n_high: N(5)=1,
//    pattern: cccc00010010dddd0000mmmm1x10nnnn,
//    rule: SMULWB_SMULWT,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    actual: Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1,
//    arch: v6,
//    baseline: USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), Rn(3:0)],
//    pattern: cccc01111000dddd1111mmmm0001nnnn,
//    rule: USAD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
class Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1);
};

// Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1
//
// Actual:
//   {defs: {inst(15:12), inst(19:16), 16
//         if inst(20)=1
//         else 32},
//    safety: [(ArchVersion()  <
//            6 &&
//         (inst(19:16)  ==
//            inst(3:0) ||
//         inst(15:12)  ==
//            inst(3:0))) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(11:8) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {inst(3:0), inst(11:8)}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    actual: Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1,
//    baseline: SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0,
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
//    pattern: cccc0000110shhhhllllmmmm1001nnnn,
//    rule: SMULL_A1,
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
//    uses: {Rn, Rm}}
//
// Baseline:
//   {NZCV: 16,
//    None: 32,
//    Pc: 15,
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    S: S(20),
//    actual: Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1,
//    baseline: UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0,
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
//    pattern: cccc0000100shhhhllllmmmm1001nnnn,
//    rule: UMULL_A1,
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
//    uses: {Rn, Rm}}
class Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1
     : public ClassDecoder {
 public:
  Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1);
};

// Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(19:16) ||
//         NumGPRs(RegisterList(inst(15:0)))  <
//            1 => UNPREDICTABLE,
//      inst(21)=1 &&
//         Contains(RegisterList(inst(15:0)), inst(19:16)) &&
//         SmallestGPR(RegisterList(inst(15:0)))  !=
//            inst(19:16) => UNKNOWN],
//    small_imm_base_wb: inst(21)=1,
//    uses: Union({inst(19:16)}, RegisterList(inst(15:0))),
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//    base: Rn,
//    baseline: STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0,
//    cond: cond(31:28),
//    defs: {Rn
//         if wback
//         else None},
//    fields: [cond(31:28), W(21), Rn(19:16), register_list(15:0)],
//    pattern: cccc100000w0nnnnrrrrrrrrrrrrrrrr,
//    register_list: register_list(15:0),
//    registers: RegisterList(register_list),
//    rule: STMDA_STMED,
//    safety: [Rn  ==
//            Pc ||
//         NumGPRs(registers)  <
//            1 => UNPREDICTABLE,
//      wback &&
//         Contains(registers, Rn) &&
//         Rn  !=
//            SmallestGPR(registers) => UNKNOWN],
//    small_imm_base_wb: wback,
//    uses: Union({Rn}, registers),
//    violations: [implied by 'base'],
//    wback: W(21)=1}
//
// Baseline:
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//    base: Rn,
//    baseline: STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0,
//    cond: cond(31:28),
//    defs: {Rn
//         if wback
//         else None},
//    fields: [cond(31:28), W(21), Rn(19:16), register_list(15:0)],
//    pattern: cccc100100w0nnnnrrrrrrrrrrrrrrrr,
//    register_list: register_list(15:0),
//    registers: RegisterList(register_list),
//    rule: STMDB_STMFD,
//    safety: [Rn  ==
//            Pc ||
//         NumGPRs(registers)  <
//            1 => UNPREDICTABLE,
//      wback &&
//         Contains(registers, Rn) &&
//         Rn  !=
//            SmallestGPR(registers) => UNKNOWN],
//    small_imm_base_wb: wback,
//    uses: Union({Rn}, registers),
//    violations: [implied by 'base'],
//    wback: W(21)=1}
//
// Baseline:
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//    base: Rn,
//    baseline: STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0,
//    cond: cond(31:28),
//    defs: {Rn
//         if wback
//         else None},
//    fields: [cond(31:28), W(21), Rn(19:16), register_list(15:0)],
//    pattern: cccc100110w0nnnnrrrrrrrrrrrrrrrr,
//    register_list: register_list(15:0),
//    registers: RegisterList(register_list),
//    rule: STMIB_STMFA,
//    safety: [Rn  ==
//            Pc ||
//         NumGPRs(registers)  <
//            1 => UNPREDICTABLE,
//      wback &&
//         Contains(registers, Rn) &&
//         Rn  !=
//            SmallestGPR(registers) => UNKNOWN],
//    small_imm_base_wb: wback,
//    uses: Union({Rn}, registers),
//    violations: [implied by 'base'],
//    wback: W(21)=1}
//
// Baseline:
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    actual: Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1,
//    base: Rn,
//    baseline: STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0,
//    cond: cond(31:28),
//    defs: {Rn
//         if wback
//         else None},
//    fields: [cond(31:28), W(21), Rn(19:16), register_list(15:0)],
//    pattern: cccc100010w0nnnnrrrrrrrrrrrrrrrr,
//    register_list: register_list(15:0),
//    registers: RegisterList(register_list),
//    rule: STM_STMIA_STMEA,
//    safety: [Rn  ==
//            Pc ||
//         NumGPRs(registers)  <
//            1 => UNPREDICTABLE,
//      wback &&
//         Contains(registers, Rn) &&
//         Rn  !=
//            SmallestGPR(registers) => UNKNOWN],
//    small_imm_base_wb: wback,
//    uses: Union({Rn}, registers),
//    violations: [implied by 'base'],
//    wback: W(21)=1}
class Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1
     : public ClassDecoder {
 public:
  Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_1);
};

// Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE],
//    small_imm_base_wb: inst(24)=0 ||
//         inst(21)=1,
//    uses: {inst(19:16), inst(15:12)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0,
//    cond: cond(31:28),
//    defs: {base
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
//    pattern: cccc010pu1w0nnnnttttiiiiiiiiiiii,
//    rule: STRB_immediate,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Rt  ==
//            Pc => UNPREDICTABLE,
//      wback &&
//         (Rn  ==
//            Pc ||
//         Rn  ==
//            Rt) => UNPREDICTABLE],
//    small_imm_base_wb: wback,
//    uses: {Rn, Rt},
//    violations: [implied by 'base'],
//    wback: P(24)=0 ||
//         W(21)=1}
class Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_1);
};

// Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(3:0) ||
//         15  ==
//            inst(15:12) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         inst(24)=0 ||
//         inst(21)=1 &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(3:0), inst(19:16), inst(15:12)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0,
//    cond: cond(31:28),
//    defs: {base
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
//    pattern: cccc011pu1w0nnnnttttiiiiitt0mmmm,
//    rule: STRB_register,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Pc in {Rm, Rt} => UNPREDICTABLE,
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
//    uses: {Rm, Rn, Rt},
//    violations: [implied by 'base'],
//    wback: P(24)=0 ||
//         W(21)=1}
class Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_1);
};

// Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12) + 1  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) + 1 => UNPREDICTABLE,
//      inst(15:12)(0)=1 => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => UNPREDICTABLE],
//    small_imm_base_wb: (inst(24)=0) ||
//         (inst(21)=1),
//    uses: {inst(15:12), inst(15:12) + 1, inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    Rt2: Rt + 1,
//    U: U(23),
//    W: W(21),
//    actual: Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1,
//    add: U(23)=1,
//    address: offset_addr
//         if index
//         else Rn,
//    base: Rn,
//    baseline: STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0,
//    cond: cond(31:28),
//    defs: {base
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
//    pattern: cccc000pu1w0nnnnttttiiii1111iiii,
//    rule: STRD_immediate,
//    safety: [Rt(0)=1 => UNPREDICTABLE,
//      P(24)=0 &&
//         W(21)=1 => UNPREDICTABLE,
//      wback &&
//         (Rn  ==
//            Pc ||
//         Rn  ==
//            Rt ||
//         Rn  ==
//            Rt2) => UNPREDICTABLE,
//      Rt2  ==
//            Pc => UNPREDICTABLE],
//    small_imm_base_wb: wback,
//    uses: {Rt, Rt2, Rn},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
class Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1
     : public ClassDecoder {
 public:
  Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1);
};

// Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12) + 1  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) + 1 ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         (inst(24)=0) ||
//         (inst(21)=1) &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(15:12)(0)=1 => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(15:12), inst(15:12) + 1, inst(19:16), inst(3:0)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    Rt2: Rt + 1,
//    U: U(23),
//    W: W(21),
//    actual: Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {base
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
//    pattern: cccc000pu0w0nnnntttt00001111mmmm,
//    rule: STRD_register,
//    safety: [Rt(0)=1 => UNPREDICTABLE,
//      P(24)=0 &&
//         W(21)=1 => UNPREDICTABLE,
//      Rt2  ==
//            Pc ||
//         Rm  ==
//            Pc => UNPREDICTABLE,
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
//    uses: {Rt, Rt2, Rn, Rm},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
class Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1);
};

// Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) ||
//         15  ==
//            inst(19:16) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(3:0) => UNPREDICTABLE],
//    uses: {inst(19:16), inst(3:0)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    Rt: Rt(3:0),
//    actual: Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1,
//    arch: v6K,
//    base: Rn,
//    baseline: STREXB_cccc00011100nnnndddd11111001tttt_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rt(3:0)],
//    imm32: Zeros((32)),
//    pattern: cccc00011100nnnndddd11111001tttt,
//    rule: STREXB,
//    safety: [Pc in {Rd, Rt, Rn} => UNPREDICTABLE,
//      Rd in {Rn, Rt} => UNPREDICTABLE],
//    uses: {Rn, Rt},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    Rt: Rt(3:0),
//    actual: Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1,
//    arch: v6K,
//    base: Rn,
//    baseline: STREXH_cccc00011110nnnndddd11111001tttt_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rt(3:0)],
//    imm32: Zeros((32)),
//    pattern: cccc00011110nnnndddd11111001tttt,
//    rule: STREXH,
//    safety: [Pc in {Rd, Rt, Rn} => UNPREDICTABLE,
//      Rd in {Rn, Rt} => UNPREDICTABLE],
//    uses: {Rn, Rt},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    Rt: Rt(3:0),
//    actual: Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1,
//    arch: v6,
//    base: Rn,
//    baseline: STREX_cccc00011000nnnndddd11111001tttt_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rt(3:0)],
//    imm32: Zeros((32)),
//    pattern: cccc00011000nnnndddd11111001tttt,
//    rule: STREX,
//    safety: [Pc in {Rd, Rt, Rn} => UNPREDICTABLE,
//      Rd in {Rn, Rt} => UNPREDICTABLE],
//    uses: {Rn, Rt},
//    violations: [implied by 'base']}
class Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1
     : public ClassDecoder {
 public:
  Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1);
};

// Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         inst(3:0)(0)=1 ||
//         14  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(15:12)  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(3:0) ||
//         inst(15:12)  ==
//            inst(3:0) + 1 => UNPREDICTABLE],
//    uses: {inst(19:16), inst(3:0), inst(3:0) + 1},
//    violations: [implied by 'base']}
//
// Baseline:
//   {Lr: 14,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    Rt: Rt(3:0),
//    Rt2: Rt + 1,
//    actual: Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1,
//    arch: v6K,
//    base: Rn,
//    baseline: STREXD_cccc00011010nnnndddd11111001tttt_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rt(3:0)],
//    imm32: Zeros((32)),
//    pattern: cccc00011010nnnndddd11111001tttt,
//    rule: STREXD,
//    safety: [Pc in {Rd, Rn} ||
//         Rt(0)=1 ||
//         Rt  ==
//            Lr => UNPREDICTABLE,
//      Rd in {Rn, Rt, Rt2} => UNPREDICTABLE],
//    uses: {Rn, Rt, Rt2},
//    violations: [implied by 'base']}
class Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1
     : public ClassDecoder {
 public:
  Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1);
};

// Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR],
//    small_imm_base_wb: (inst(24)=0) ||
//         (inst(21)=1),
//    uses: {inst(15:12), inst(19:16)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1,
//    add: U(23)=1,
//    address: offset_addr
//         if index
//         else Rn,
//    base: Rn,
//    baseline: STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0,
//    cond: cond(31:28),
//    defs: {base
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
//    pattern: cccc000pu1w0nnnnttttiiii1011iiii,
//    rule: STRH_immediate,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      Rt  ==
//            Pc => UNPREDICTABLE,
//      wback &&
//         (Rn  ==
//            Pc ||
//         Rn  ==
//            Rt) => UNPREDICTABLE],
//    small_imm_base_wb: wback,
//    uses: {Rt, Rn},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
class Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1
     : public ClassDecoder {
 public:
  Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1);
};

// Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if (inst(24)=0) ||
//         (inst(21)=1)
//         else 32},
//    safety: [(inst(24)=0) ||
//         (inst(21)=1) &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         (inst(24)=0) ||
//         (inst(21)=1) &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(15:12), inst(19:16), inst(3:0)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0,
//    cond: cond(31:28),
//    defs: {base
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
//    pattern: cccc000pu0w0nnnntttt00001011mmmm,
//    rule: STRH_register,
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
//    uses: {Rt, Rn, Rm},
//    violations: [implied by 'base'],
//    wback: (P(24)=0) ||
//         (W(21)=1)}
class Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1);
};

// Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE],
//    small_imm_base_wb: inst(24)=0 ||
//         inst(21)=1,
//    uses: {inst(19:16), inst(15:12)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0,
//    cond: cond(31:28),
//    defs: {base
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
//    pattern: cccc010pu0w0nnnnttttiiiiiiiiiiii,
//    rule: STR_immediate,
//    safety: [P(24)=0 &&
//         W(21)=1 => DECODER_ERROR,
//      wback &&
//         (Rn  ==
//            Pc ||
//         Rn  ==
//            Rt) => UNPREDICTABLE],
//    small_imm_base_wb: wback,
//    uses: {Rn, Rt},
//    violations: [implied by 'base'],
//    wback: P(24)=0 ||
//         W(21)=1}
class Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_1);
};

// Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(24)=0 ||
//         inst(21)=1
//         else 32},
//    safety: [15  ==
//            inst(3:0) => UNPREDICTABLE,
//      ArchVersion()  <
//            6 &&
//         inst(24)=0 ||
//         inst(21)=1 &&
//         inst(19:16)  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(24)=0 &&
//         inst(21)=1 => DECODER_ERROR,
//      inst(24)=0 ||
//         inst(21)=1 &&
//         (15  ==
//            inst(19:16) ||
//         inst(15:12)  ==
//            inst(19:16)) => UNPREDICTABLE,
//      inst(24)=1 => FORBIDDEN],
//    uses: {inst(3:0), inst(19:16), inst(15:12)},
//    violations: [implied by 'base']}
//
// Baseline:
//   {None: 32,
//    P: P(24),
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rt: Rt(15:12),
//    U: U(23),
//    W: W(21),
//    actual: Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1,
//    add: U(23)=1,
//    base: Rn,
//    baseline: STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0,
//    cond: cond(31:28),
//    defs: {base
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
//    pattern: cccc011pd0w0nnnnttttiiiiitt0mmmm,
//    rule: STR_register,
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
//      index => FORBIDDEN],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rm, Rn, Rt},
//    violations: [implied by 'base'],
//    wback: P(24)=0 ||
//         W(21)=1}
class Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
  virtual ViolationSet get_violations(
      const nacl_arm_val::DecodedInstruction& first,
      const nacl_arm_val::DecodedInstruction& second,
      const nacl_arm_val::SfiValidator& sfi,
      nacl_arm_val::AddressSet* branches,
      nacl_arm_val::AddressSet* critical,
      uint32_t* next_inst_addr) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_1);
};

// Actual_SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => DEPRECATED],
//    uses: {}}
//
// Baseline:
//   {actual: Actual_SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_1,
//    baseline: SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_0,
//    defs: {},
//    pattern: cccc00010b00nnnntttt00001001tttt,
//    rule: SWP_SWPB,
//    safety: [true => DEPRECATED],
//    true: true,
//    uses: {}}
class Actual_SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_1
     : public ClassDecoder {
 public:
  Actual_SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_1);
};

// Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12)},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(3:0) => UNPREDICTABLE,
//      inst(19:16)=1111 => DECODER_ERROR],
//    uses: {inst(19:16), inst(3:0)}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//    arch: v6,
//    baseline: SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc01101000nnnnddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: SXTAB16,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR,
//      Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//    arch: v6,
//    baseline: SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc01101010nnnnddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: SXTAB,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR,
//      Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//    arch: v6,
//    baseline: SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc01101011nnnnddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: SXTAH,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR,
//      Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//    arch: v6,
//    baseline: UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc01101100nnnnddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: UXTAB16,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR,
//      Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//    arch: v6,
//    baseline: UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc01101110nnnnddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: UXTAB,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR,
//      Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
//
// Baseline:
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    actual: Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1,
//    arch: v6,
//    baseline: UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc01101111nnnnddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: UXTAH,
//    safety: [Rn(19:16)=1111 => DECODER_ERROR,
//      Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
class Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_1);
};

// Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1
//
// Actual:
//   {defs: {16},
//    sets_Z_if_clear_bits: RegIndex(test_register())  ==
//            inst(19:16) &&
//         (ARMExpandImm_C(inst(11:0)) &&
//         clears_mask())  ==
//            clears_mask(),
//    uses: {inst(19:16)}}
//
// Baseline:
//   {NZCV: 16,
//    Rn: Rn(19:16),
//    actual: Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1,
//    baseline: TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0,
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm_C(imm12),
//    pattern: cccc00110001nnnn0000iiiiiiiiiiii,
//    rule: TST_immediate,
//    sets_Z_if_clear_bits: Rn  ==
//            RegIndex(test_register()) &&
//         (imm32 &&
//         clears_mask())  ==
//            clears_mask(),
//    uses: {Rn}}
class Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool sets_Z_if_bits_clear(Instruction i,
                                    Register test_register,
                                    uint32_t clears_mask) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1);
};

// Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1
//
// Actual:
//   {defs: {},
//    safety: [not IsUDFNaClSafe(inst) => FORBIDDEN_OPERANDS],
//    uses: {}}
//
// Baseline:
//   {actual: Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1,
//    baseline: UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0,
//    defs: {},
//    inst: inst,
//    pattern: cccc01111111iiiiiiiiiiii1111iiii,
//    rule: UDF,
//    safety: [not IsUDFNaClSafe(inst) => FORBIDDEN_OPERANDS],
//    uses: {}}
class Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1
     : public ClassDecoder {
 public:
  Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_UDF_cccc01111111iiiiiiiiiiii1111iiii_case_1);
};

// Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => UNPREDICTABLE],
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//    baseline: Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0,
//    defs: {},
//    pattern: 11110100xx11xxxxxxxxxxxxxxxxxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//    baseline: Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0,
//    defs: {},
//    pattern: 111101010011xxxxxxxxxxxxxxxxxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//    baseline: Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0,
//    defs: {},
//    pattern: 111101010111xxxxxxxxxxxx0000xxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//    baseline: Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0,
//    defs: {},
//    pattern: 111101010111xxxxxxxxxxxx001xxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//    baseline: Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0,
//    defs: {},
//    pattern: 111101010111xxxxxxxxxxxx0111xxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//    baseline: Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0,
//    defs: {},
//    pattern: 111101010111xxxxxxxxxxxx1xxxxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//    baseline: Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0,
//    defs: {},
//    pattern: 111101011x11xxxxxxxxxxxxxxxxxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//    baseline: Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0,
//    defs: {},
//    pattern: 11110101x001xxxxxxxxxxxxxxxxxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1,
//    baseline: Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0,
//    defs: {},
//    pattern: 1111011xxx11xxxxxxxxxxxxxxx0xxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
class Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1
     : public ClassDecoder {
 public:
  Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_1);
};

// Actual_Unnamed_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_case_1,
//    baseline: Unnamed_case_1,
//    defs: {},
//    safety: [true => UNDEFINED],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_case_1,
//    baseline: Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0,
//    defs: {},
//    pattern: cccc00000101xxxxxxxxxxxx1001xxxx,
//    safety: [true => UNDEFINED],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_case_1,
//    baseline: Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0,
//    defs: {},
//    pattern: cccc00000111xxxxxxxxxxxx1001xxxx,
//    safety: [true => UNDEFINED],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {actual: Actual_Unnamed_case_1,
//    baseline: Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0,
//    defs: {},
//    pattern: cccc1100000xnnnnxxxxccccxxxoxxxx,
//    safety: [true => UNDEFINED],
//    true: true,
//    uses: {}}
class Actual_Unnamed_case_1
     : public ClassDecoder {
 public:
  Actual_Unnamed_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_Unnamed_case_1);
};

// Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(15:12)(0)=1 => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//    baseline: VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd0101n0m0mmmm,
//    rule: VABAL_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//    baseline: VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd0111n0m0mmmm,
//    rule: VABDL_integer_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//    baseline: VMLAL_VMLSL_integer_A2_1111001u1dssnnnndddd10p0n0m0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd10p0n0m0mmmm,
//    rule: VMLAL_VMLSL_integer_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1,
//    baseline: VMULL_integer_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd11p0n0m0mmmm,
//    rule: VMULL_integer_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_1);
};

// Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)=11 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0111nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VABA,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0111nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VABD,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100110dssnnnndddd1000nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCEQ_register_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0011nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCGE_register_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0011nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCGT_register_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0000nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VHADD,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0010nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VHSUB,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VMAX_1111001u0dssnnnndddd0110nqm0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0110nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMAX,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VMIN_1111001u0dssnnnndddd0110nqm1mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0110nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMIN,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VMLA_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1001nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMLA_integer_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VMLS_integer_A1_1111001u0dssnnnndddd1001nqm0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1001nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMLS_integer_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VMUL_integer_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMUL_integer_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VRHADD_1111001u0dssnnnndddd0001nqm0mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0001nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRHADD,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    actual: Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1,
//    arch: ASIMD,
//    baseline: VTST_111100100dssnnnndddd1000nqm1mmmm_case_0,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100dssnnnndddd1000nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VTST,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(21:20)=11 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_1);
};

} // namespace nacl_arm_test

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_ACTUALS_1_H_
