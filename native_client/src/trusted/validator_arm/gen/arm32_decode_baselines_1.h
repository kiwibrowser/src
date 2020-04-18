/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_BASELINES_1_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_BASELINES_1_H_

#include "native_client/src/trusted/validator_arm/arm_helpers.h"
#include "native_client/src/trusted/validator_arm/inst_classes.h"

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
class ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_0);
};

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
class ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_0);
};

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
class ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_0);
};

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
class ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_0);
};

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
class ADD_register_cccc0000100snnnnddddiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  ADD_register_cccc0000100snnnnddddiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ADD_register_cccc0000100snnnnddddiiiiitt0mmmm_case_0);
};

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
class ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ADD_register_shifted_register_cccc0000100snnnnddddssss0tt1mmmm_case_0);
};

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
class ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_0);
};

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
class ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ADR_A2_cccc001001001111ddddiiiiiiiiiiii_case_0);
};

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
class AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      AND_immediate_cccc0010000snnnnddddiiiiiiiiiiii_case_0);
};

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
class AND_register_cccc0000000snnnnddddiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  AND_register_cccc0000000snnnnddddiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      AND_register_cccc0000000snnnnddddiiiiitt0mmmm_case_0);
};

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
class AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      AND_register_shifted_register_cccc0000000snnnnddddssss0tt1mmmm_case_0);
};

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
class ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_0
     : public ClassDecoder {
 public:
  ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_0);
};

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
class ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0
     : public ClassDecoder {
 public:
  ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_0);
};

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
class BFC_cccc0111110mmmmmddddlllll0011111_case_0
     : public ClassDecoder {
 public:
  BFC_cccc0111110mmmmmddddlllll0011111_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      BFC_cccc0111110mmmmmddddlllll0011111_case_0);
};

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
class BFI_cccc0111110mmmmmddddlllll001nnnn_case_0
     : public ClassDecoder {
 public:
  BFI_cccc0111110mmmmmddddlllll001nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      BFI_cccc0111110mmmmmddddlllll001nnnn_case_0);
};

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
class BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual bool clears_bits(Instruction i, uint32_t clears_mask) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_0);
};

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
class BIC_register_cccc0001110snnnnddddiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  BIC_register_cccc0001110snnnnddddiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      BIC_register_cccc0001110snnnnddddiiiiitt0mmmm_case_0);
};

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
class BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      BIC_register_shifted_register_cccc0001110snnnnddddssss0tt1mmmm_case_0);
};

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
class BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_0
     : public ClassDecoder {
 public:
  BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_0()
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
      BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_0);
};

// BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 1111101hiiiiiiiiiiiiiiiiiiiiiiii,
//    rule: BLX_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_0);
};

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
class BLX_register_cccc000100101111111111110011mmmm_case_0
     : public ClassDecoder {
 public:
  BLX_register_cccc000100101111111111110011mmmm_case_0()
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
      BLX_register_cccc000100101111111111110011mmmm_case_0);
};

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
class BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0()
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
      BL_BLX_immediate_cccc1011iiiiiiiiiiiiiiiiiiiiiiii_case_0);
};

// BXJ_cccc000100101111111111110010mmmm_case_0:
//
//   {arch: v5TEJ,
//    defs: {},
//    pattern: cccc000100101111111111110010mmmm,
//    rule: BXJ,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class BXJ_cccc000100101111111111110010mmmm_case_0
     : public ClassDecoder {
 public:
  BXJ_cccc000100101111111111110010mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      BXJ_cccc000100101111111111110010mmmm_case_0);
};

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
class B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0()
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
      B_cccc1010iiiiiiiiiiiiiiiiiiiiiiii_case_0);
};

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
class Bx_cccc000100101111111111110001mmmm_case_0
     : public ClassDecoder {
 public:
  Bx_cccc000100101111111111110001mmmm_case_0()
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
      Bx_cccc000100101111111111110001mmmm_case_0);
};

// CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 11111110iiiiiiiiiiiiiiiiiii0iiii,
//    rule: CDP2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0
     : public ClassDecoder {
 public:
  CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CDP2_11111110iiiiiiiiiiiiiiiiiii0iiii_case_0);
};

// CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0:
//
//   {defs: {},
//    pattern: cccc1110oooonnnnddddccccooo0mmmm,
//    rule: CDP,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0
     : public ClassDecoder {
 public:
  CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CDP_cccc1110oooonnnnddddccccooo0mmmm_case_0);
};

// CLREX_11110101011111111111000000011111_case_0:
//
//   {arch: V6K,
//    defs: {},
//    pattern: 11110101011111111111000000011111,
//    rule: CLREX,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class CLREX_11110101011111111111000000011111_case_0
     : public ClassDecoder {
 public:
  CLREX_11110101011111111111000000011111_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CLREX_11110101011111111111000000011111_case_0);
};

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
class CLZ_cccc000101101111dddd11110001mmmm_case_0
     : public ClassDecoder {
 public:
  CLZ_cccc000101101111dddd11110001mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CLZ_cccc000101101111dddd11110001mmmm_case_0);
};

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
class CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_0);
};

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
class CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_0);
};

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
class CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_0);
};

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
class CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CMP_immediate_cccc00110101nnnn0000iiiiiiiiiiii_case_0);
};

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
class CMP_register_cccc00010101nnnn0000iiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  CMP_register_cccc00010101nnnn0000iiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CMP_register_cccc00010101nnnn0000iiiiitt0mmmm_case_0);
};

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
class CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CMP_register_shifted_register_cccc00010101nnnn0000ssss0tt1mmmm_case_0);
};

// CPS_111100010000iii00000000iii0iiiii_case_0:
//
//   {arch: v6,
//    defs: {},
//    pattern: 111100010000iii00000000iii0iiiii,
//    rule: CPS,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class CPS_111100010000iii00000000iii0iiiii_case_0
     : public ClassDecoder {
 public:
  CPS_111100010000iii00000000iii0iiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CPS_111100010000iii00000000iii0iiiii_case_0);
};

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
class CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0
     : public ClassDecoder {
 public:
  CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      CVT_between_half_precision_and_single_precision_111100111d11ss10dddd011p00m0mmmm_case_0);
};

// DBG_cccc001100100000111100001111iiii_case_0:
//
//   {arch: v7,
//    defs: {},
//    pattern: cccc001100100000111100001111iiii,
//    rule: DBG,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class DBG_cccc001100100000111100001111iiii_case_0
     : public ClassDecoder {
 public:
  DBG_cccc001100100000111100001111iiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      DBG_cccc001100100000111100001111iiii_case_0);
};

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
class DMB_1111010101111111111100000101xxxx_case_0
     : public ClassDecoder {
 public:
  DMB_1111010101111111111100000101xxxx_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      DMB_1111010101111111111100000101xxxx_case_0);
};

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
class DSB_1111010101111111111100000100xxxx_case_0
     : public ClassDecoder {
 public:
  DSB_1111010101111111111100000100xxxx_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      DSB_1111010101111111111100000100xxxx_case_0);
};

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
class EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      EOR_immediate_cccc0010001snnnnddddiiiiiiiiiiii_case_0);
};

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
class EOR_register_cccc0000001snnnnddddiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  EOR_register_cccc0000001snnnnddddiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      EOR_register_cccc0000001snnnnddddiiiiitt0mmmm_case_0);
};

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
class EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      EOR_register_shifted_register_cccc0000001snnnnddddssss0tt1mmmm_case_0);
};

// ERET_cccc0001011000000000000001101110_case_0:
//
//   {arch: v7VE,
//    defs: {},
//    pattern: cccc0001011000000000000001101110,
//    rule: ERET,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class ERET_cccc0001011000000000000001101110_case_0
     : public ClassDecoder {
 public:
  ERET_cccc0001011000000000000001101110_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ERET_cccc0001011000000000000001101110_case_0);
};

// FICTITIOUS_FIRST_case_0:
//
//   {defs: {},
//    rule: FICTITIOUS_FIRST,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class FICTITIOUS_FIRST_case_0
     : public ClassDecoder {
 public:
  FICTITIOUS_FIRST_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      FICTITIOUS_FIRST_case_0);
};

// HVC_cccc00010100iiiiiiiiiiii0111iiii_case_0:
//
//   {arch: v7VE,
//    defs: {},
//    pattern: cccc00010100iiiiiiiiiiii0111iiii,
//    rule: HVC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class HVC_cccc00010100iiiiiiiiiiii0111iiii_case_0
     : public ClassDecoder {
 public:
  HVC_cccc00010100iiiiiiiiiiii0111iiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      HVC_cccc00010100iiiiiiiiiiii0111iiii_case_0);
};

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
class ISB_1111010101111111111100000110xxxx_case_0
     : public ClassDecoder {
 public:
  ISB_1111010101111111111100000110xxxx_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ISB_1111010101111111111100000110xxxx_case_0);
};

// LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 1111110pudw1nnnniiiiiiiiiiiiiiii,
//    rule: LDC2_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LDC2_immediate_1111110pudw1nnnniiiiiiiiiiiiiiii_case_0);
};

// LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 1111110pudw11111iiiiiiiiiiiiiiii,
//    rule: LDC2_literal,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LDC2_literal_1111110pudw11111iiiiiiiiiiiiiiii_case_0);
};

// LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc110pudw1nnnnddddcccciiiiiiii,
//    rule: LDC_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0
     : public ClassDecoder {
 public:
  LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LDC_immediate_cccc110pudw1nnnnddddcccciiiiiiii_case_0);
};

// LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc110pudw11111ddddcccciiiiiiii,
//    rule: LDC_literal,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0
     : public ClassDecoder {
 public:
  LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LDC_literal_cccc110pudw11111ddddcccciiiiiiii_case_0);
};

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
class LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0
     : public ClassDecoder {
 public:
  LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0()
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
      LDMDA_LDMFA_cccc100000w1nnnnrrrrrrrrrrrrrrrr_case_0);
};

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
class LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0
     : public ClassDecoder {
 public:
  LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0()
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
      LDMDB_LDMEA_cccc100100w1nnnnrrrrrrrrrrrrrrrr_case_0);
};

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
class LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0
     : public ClassDecoder {
 public:
  LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0()
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
      LDMIB_LDMED_cccc100110w1nnnnrrrrrrrrrrrrrrrr_case_0);
};

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
class LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0
     : public ClassDecoder {
 public:
  LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0()
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
      LDM_LDMIA_LDMFD_cccc100010w1nnnnrrrrrrrrrrrrrrrr_case_0);
};

// LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0:
//
//   {defs: {},
//    pattern: cccc100pu101nnnn0rrrrrrrrrrrrrrr,
//    rule: LDM_User_registers,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0
     : public ClassDecoder {
 public:
  LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LDM_User_registers_cccc100pu101nnnn0rrrrrrrrrrrrrrr_case_0);
};

// LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0:
//
//   {defs: {},
//    pattern: cccc100pu1w1nnnn1rrrrrrrrrrrrrrr,
//    rule: LDM_exception_return,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0
     : public ClassDecoder {
 public:
  LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LDM_exception_return_cccc100pu1w1nnnn1rrrrrrrrrrrrrrr_case_0);
};

// LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc0100u111nnnnttttiiiiiiiiiiii,
//    rule: LDRBT_A1,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LDRBT_A1_cccc0100u111nnnnttttiiiiiiiiiiii_case_0);
};

// LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0:
//
//   {defs: {},
//    pattern: cccc0110u111nnnnttttiiiiitt0mmmm,
//    rule: LDRBT_A2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LDRBT_A2_cccc0110u111nnnnttttiiiiitt0mmmm_case_0);
};

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
class LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0()
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
      LDRB_immediate_cccc010pu1w1nnnnttttiiiiiiiiiiii_case_0);
};

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
class LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0()
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
      LDRB_literal_cccc0101u1011111ttttiiiiiiiiiiii_case_0);
};

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
class LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0()
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
      LDRB_register_cccc011pu1w1nnnnttttiiiiitt0mmmm_case_0);
};

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
class LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0
     : public ClassDecoder {
 public:
  LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0()
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
      LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_0);
};

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
class LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0
     : public ClassDecoder {
 public:
  LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0()
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
      LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_0);
};

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
class LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0
     : public ClassDecoder {
 public:
  LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0()
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
      LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_0);
};

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
class LDREXB_cccc00011101nnnntttt111110011111_case_0
     : public ClassDecoder {
 public:
  LDREXB_cccc00011101nnnntttt111110011111_case_0()
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
      LDREXB_cccc00011101nnnntttt111110011111_case_0);
};

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
class LDREXD_cccc00011011nnnntttt111110011111_case_0
     : public ClassDecoder {
 public:
  LDREXD_cccc00011011nnnntttt111110011111_case_0()
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
      LDREXD_cccc00011011nnnntttt111110011111_case_0);
};

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
class LDREX_cccc00011001nnnntttt111110011111_case_0
     : public ClassDecoder {
 public:
  LDREX_cccc00011001nnnntttt111110011111_case_0()
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
      LDREX_cccc00011001nnnntttt111110011111_case_0);
};

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
class LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0
     : public ClassDecoder {
 public:
  LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0()
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
      LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_0);
};

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
class LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0
     : public ClassDecoder {
 public:
  LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0()
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
      LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_0);
};

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
class LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0
     : public ClassDecoder {
 public:
  LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0()
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
      LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_0);
};

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
class LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0
     : public ClassDecoder {
 public:
  LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0()
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
      LDRSB_immediate_cccc000pu1w1nnnnttttiiii1101iiii_case_0);
};

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
class LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0
     : public ClassDecoder {
 public:
  LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0()
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
      LDRSB_literal_cccc0001u1011111ttttiiii1101iiii_case_0);
};

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
class LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0
     : public ClassDecoder {
 public:
  LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0()
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
      LDRSB_register_cccc000pu0w1nnnntttt00001101mmmm_case_0);
};

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
class LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0
     : public ClassDecoder {
 public:
  LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0()
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
      LDRSH_immediate_cccc000pu1w1nnnnttttiiii1111iiii_case_0);
};

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
class LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0
     : public ClassDecoder {
 public:
  LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0()
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
      LDRSH_literal_cccc0001u1011111ttttiiii1111iiii_case_0);
};

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
class LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0
     : public ClassDecoder {
 public:
  LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0()
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
      LDRSH_register_cccc000pu0w1nnnntttt00001111mmmm_case_0);
};

// LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc0100u011nnnnttttiiiiiiiiiiii,
//    rule: LDRT_A1,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LDRT_A1_cccc0100u011nnnnttttiiiiiiiiiiii_case_0);
};

// LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0:
//
//   {defs: {},
//    pattern: cccc0110u011nnnnttttiiiiitt0mmmm,
//    rule: LDRT_A2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LDRT_A2_cccc0110u011nnnnttttiiiiitt0mmmm_case_0);
};

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
class LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0()
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
      LDR_immediate_cccc010pu0w1nnnnttttiiiiiiiiiiii_case_0);
};

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
class LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0()
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
      LDR_literal_cccc0101u0011111ttttiiiiiiiiiiii_case_0);
};

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
class LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0()
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
      LDR_register_cccc011pu0w1nnnnttttiiiiitt0mmmm_case_0);
};

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
class LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_0
     : public ClassDecoder {
 public:
  LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_0);
};

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
class LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0
     : public ClassDecoder {
 public:
  LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LSL_register_cccc0001101s0000ddddmmmm0001nnnn_case_0);
};

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
class LSR_immediate_cccc0001101s0000ddddiiiii010mmmm_case_0
     : public ClassDecoder {
 public:
  LSR_immediate_cccc0001101s0000ddddiiiii010mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LSR_immediate_cccc0001101s0000ddddiiiii010mmmm_case_0);
};

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
class LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0
     : public ClassDecoder {
 public:
  LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      LSR_register_cccc0001101s0000ddddmmmm0011nnnn_case_0);
};

// MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 11111110iii0iiiittttiiiiiii1iiii,
//    rule: MCR2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0
     : public ClassDecoder {
 public:
  MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MCR2_11111110iii0iiiittttiiiiiii1iiii_case_0);
};

// MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0:
//
//   {arch: v6,
//    defs: {},
//    pattern: 111111000100ssssttttiiiiiiiiiiii,
//    rule: MCRR2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MCRR2_111111000100ssssttttiiiiiiiiiiii_case_0);
};

// MCRR_cccc11000100ttttttttccccoooommmm_case_0:
//
//   {arch: v5TE,
//    defs: {},
//    pattern: cccc11000100ttttttttccccoooommmm,
//    rule: MCRR,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class MCRR_cccc11000100ttttttttccccoooommmm_case_0
     : public ClassDecoder {
 public:
  MCRR_cccc11000100ttttttttccccoooommmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MCRR_cccc11000100ttttttttccccoooommmm_case_0);
};

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
class MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0
     : public ClassDecoder {
 public:
  MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0()
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
      MCR_cccc1110ooo0nnnnttttccccooo1mmmm_case_0);
};

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
class MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0
     : public ClassDecoder {
 public:
  MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_0);
};

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
class MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0
     : public ClassDecoder {
 public:
  MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_0);
};

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
class MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_0
     : public ClassDecoder {
 public:
  MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MOVE_scalar_to_ARM_core_register_cccc1110iii1nnnntttt1011nii10000_case_0);
};

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
class MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual Instruction dynamic_code_replacement_sentinel(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_0);
};

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
class MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual Instruction dynamic_code_replacement_sentinel(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MOVW_cccc00110000iiiiddddiiiiiiiiiiii_case_0);
};

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
class MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual Instruction dynamic_code_replacement_sentinel(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_0);
};

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
class MOV_register_cccc0001101s0000dddd00000000mmmm_case_0
     : public ClassDecoder {
 public:
  MOV_register_cccc0001101s0000dddd00000000mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MOV_register_cccc0001101s0000dddd00000000mmmm_case_0);
};

// MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 11111110iii1iiiittttiiiiiii1iiii,
//    rule: MRC2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0
     : public ClassDecoder {
 public:
  MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MRC2_11111110iii1iiiittttiiiiiii1iiii_case_0);
};

// MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0:
//
//   {defs: {},
//    pattern: cccc1110ooo1nnnnttttccccooo1mmmm,
//    rule: MRC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0
     : public ClassDecoder {
 public:
  MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MRC_cccc1110ooo1nnnnttttccccooo1mmmm_case_0);
};

// MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0:
//
//   {arch: v6,
//    defs: {},
//    pattern: 111111000101ssssttttiiiiiiiiiiii,
//    rule: MRRC2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MRRC2_111111000101ssssttttiiiiiiiiiiii_case_0);
};

// MRRC_cccc11000101ttttttttccccoooommmm_case_0:
//
//   {arch: v5TE,
//    defs: {},
//    pattern: cccc11000101ttttttttccccoooommmm,
//    rule: MRRC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class MRRC_cccc11000101ttttttttccccoooommmm_case_0
     : public ClassDecoder {
 public:
  MRRC_cccc11000101ttttttttccccoooommmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MRRC_cccc11000101ttttttttccccoooommmm_case_0);
};

// MRS_Banked_register_cccc00010r00mmmmdddd001m00000000_case_0:
//
//   {arch: v7VE,
//    defs: {},
//    pattern: cccc00010r00mmmmdddd001m00000000,
//    rule: MRS_Banked_register,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class MRS_Banked_register_cccc00010r00mmmmdddd001m00000000_case_0
     : public ClassDecoder {
 public:
  MRS_Banked_register_cccc00010r00mmmmdddd001m00000000_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MRS_Banked_register_cccc00010r00mmmmdddd001m00000000_case_0);
};

// MRS_Banked_register_cccc00010r10mmmm1111001m0000nnnn_case_0:
//
//   {arch: v7VE,
//    defs: {},
//    pattern: cccc00010r10mmmm1111001m0000nnnn,
//    rule: MRS_Banked_register,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class MRS_Banked_register_cccc00010r10mmmm1111001m0000nnnn_case_0
     : public ClassDecoder {
 public:
  MRS_Banked_register_cccc00010r10mmmm1111001m0000nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MRS_Banked_register_cccc00010r10mmmm1111001m0000nnnn_case_0);
};

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
class MRS_cccc00010r001111dddd000000000000_case_0
     : public ClassDecoder {
 public:
  MRS_cccc00010r001111dddd000000000000_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MRS_cccc00010r001111dddd000000000000_case_0);
};

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
class MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_0);
};

// MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc00110r10mmmm1111iiiiiiiiiiii,
//    rule: MSR_immediate,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MSR_immediate_cccc00110r10mmmm1111iiiiiiiiiiii_case_0);
};

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
class MSR_register_cccc00010010mm00111100000000nnnn_case_0
     : public ClassDecoder {
 public:
  MSR_register_cccc00010010mm00111100000000nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MSR_register_cccc00010010mm00111100000000nnnn_case_0);
};

// MSR_register_cccc00010r10mmmm111100000000nnnn_case_0:
//
//   {defs: {},
//    pattern: cccc00010r10mmmm111100000000nnnn,
//    rule: MSR_register,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class MSR_register_cccc00010r10mmmm111100000000nnnn_case_0
     : public ClassDecoder {
 public:
  MSR_register_cccc00010r10mmmm111100000000nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MSR_register_cccc00010r10mmmm111100000000nnnn_case_0);
};

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
class MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0
     : public ClassDecoder {
 public:
  MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_0);
};

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
class MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual Instruction dynamic_code_replacement_sentinel(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MVN_immediate_cccc0011111s0000ddddiiiiiiiiiiii_case_0);
};

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
class MVN_register_cccc0001111s0000ddddiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  MVN_register_cccc0001111s0000ddddiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MVN_register_cccc0001111s0000ddddiiiiitt0mmmm_case_0);
};

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
class MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      MVN_register_shifted_register_cccc0001111s0000ddddssss0tt1mmmm_case_0);
};

// NOP_cccc0011001000001111000000000000_case_0:
//
//   {arch: ['v6K', 'v6T2'],
//    defs: {},
//    pattern: cccc0011001000001111000000000000,
//    rule: NOP,
//    uses: {}}
class NOP_cccc0011001000001111000000000000_case_0
     : public ClassDecoder {
 public:
  NOP_cccc0011001000001111000000000000_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      NOP_cccc0011001000001111000000000000_case_0);
};

// NOT_IMPLEMENTED_case_0:
//
//   {defs: {},
//    rule: NOT_IMPLEMENTED,
//    safety: [true => NOT_IMPLEMENTED],
//    true: true,
//    uses: {}}
class NOT_IMPLEMENTED_case_0
     : public ClassDecoder {
 public:
  NOT_IMPLEMENTED_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      NOT_IMPLEMENTED_case_0);
};

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
class ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual Instruction dynamic_code_replacement_sentinel(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_0);
};

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
class ORR_register_cccc0001100snnnnddddiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  ORR_register_cccc0001100snnnnddddiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ORR_register_cccc0001100snnnnddddiiiiitt0mmmm_case_0);
};

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
class ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ORR_register_shifted_register_cccc0001100snnnnddddssss0tt1mmmm_case_0);
};

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
class PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0
     : public ClassDecoder {
 public:
  PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      PKH_cccc01101000nnnnddddiiiiit01mmmm_case_0);
};

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
class PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0()
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
      PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_0);
};

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
class PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1
     : public ClassDecoder {
 public:
  PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1()
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
      PLD_PLDW_immediate_11110101ur01nnnn1111iiiiiiiiiiii_case_1);
};

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
class PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0()
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
      PLD_PLDW_register_11110111u001nnnn1111iiiiitt0mmmm_case_0);
};

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
class PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0()
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
      PLD_PLDW_register_11110111u101nnnn1111iiiiitt0mmmm_case_0);
};

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
class PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0()
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
      PLD_literal_11110101u10111111111iiiiiiiiiiii_case_0);
};

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
class PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0()
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
      PLI_immediate_literal_11110100u101nnnn1111iiiiiiiiiiii_case_0);
};

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
class PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0()
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
      PLI_register_11110110u101nnnn1111iiiiitt0mmmm_case_0);
};

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
class QADD16_cccc01100010nnnndddd11110001mmmm_case_0
     : public ClassDecoder {
 public:
  QADD16_cccc01100010nnnndddd11110001mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      QADD16_cccc01100010nnnndddd11110001mmmm_case_0);
};

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
class QADD8_cccc01100010nnnndddd11111001mmmm_case_0
     : public ClassDecoder {
 public:
  QADD8_cccc01100010nnnndddd11111001mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      QADD8_cccc01100010nnnndddd11111001mmmm_case_0);
};

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
class QADD_cccc00010000nnnndddd00000101mmmm_case_0
     : public ClassDecoder {
 public:
  QADD_cccc00010000nnnndddd00000101mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      QADD_cccc00010000nnnndddd00000101mmmm_case_0);
};

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
class QASX_cccc01100010nnnndddd11110011mmmm_case_0
     : public ClassDecoder {
 public:
  QASX_cccc01100010nnnndddd11110011mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      QASX_cccc01100010nnnndddd11110011mmmm_case_0);
};

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
class QDADD_cccc00010100nnnndddd00000101mmmm_case_0
     : public ClassDecoder {
 public:
  QDADD_cccc00010100nnnndddd00000101mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      QDADD_cccc00010100nnnndddd00000101mmmm_case_0);
};

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
class QDSUB_cccc00010110nnnndddd00000101mmmm_case_0
     : public ClassDecoder {
 public:
  QDSUB_cccc00010110nnnndddd00000101mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      QDSUB_cccc00010110nnnndddd00000101mmmm_case_0);
};

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
class QSAX_cccc01100010nnnndddd11110101mmmm_case_0
     : public ClassDecoder {
 public:
  QSAX_cccc01100010nnnndddd11110101mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      QSAX_cccc01100010nnnndddd11110101mmmm_case_0);
};

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
class QSUB16_cccc01100010nnnndddd11110111mmmm_case_0
     : public ClassDecoder {
 public:
  QSUB16_cccc01100010nnnndddd11110111mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      QSUB16_cccc01100010nnnndddd11110111mmmm_case_0);
};

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
class QSUB8_cccc01100010nnnndddd11111111mmmm_case_0
     : public ClassDecoder {
 public:
  QSUB8_cccc01100010nnnndddd11111111mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      QSUB8_cccc01100010nnnndddd11111111mmmm_case_0);
};

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
class QSUB_cccc00010010nnnndddd00000101mmmm_case_0
     : public ClassDecoder {
 public:
  QSUB_cccc00010010nnnndddd00000101mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      QSUB_cccc00010010nnnndddd00000101mmmm_case_0);
};

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
class RBIT_cccc011011111111dddd11110011mmmm_case_0
     : public ClassDecoder {
 public:
  RBIT_cccc011011111111dddd11110011mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      RBIT_cccc011011111111dddd11110011mmmm_case_0);
};

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
class REV16_cccc011010111111dddd11111011mmmm_case_0
     : public ClassDecoder {
 public:
  REV16_cccc011010111111dddd11111011mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      REV16_cccc011010111111dddd11111011mmmm_case_0);
};

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
class REVSH_cccc011011111111dddd11111011mmmm_case_0
     : public ClassDecoder {
 public:
  REVSH_cccc011011111111dddd11111011mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      REVSH_cccc011011111111dddd11111011mmmm_case_0);
};

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
class REV_cccc011010111111dddd11110011mmmm_case_0
     : public ClassDecoder {
 public:
  REV_cccc011010111111dddd11110011mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      REV_cccc011010111111dddd11110011mmmm_case_0);
};

// RFE_1111100pu0w1nnnn0000101000000000_case_0:
//
//   {arch: v6,
//    defs: {},
//    pattern: 1111100pu0w1nnnn0000101000000000,
//    rule: RFE,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class RFE_1111100pu0w1nnnn0000101000000000_case_0
     : public ClassDecoder {
 public:
  RFE_1111100pu0w1nnnn0000101000000000_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      RFE_1111100pu0w1nnnn0000101000000000_case_0);
};

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
class ROR_immediate_cccc0001101s0000ddddiiiii110mmmm_case_0
     : public ClassDecoder {
 public:
  ROR_immediate_cccc0001101s0000ddddiiiii110mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ROR_immediate_cccc0001101s0000ddddiiiii110mmmm_case_0);
};

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
class ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0
     : public ClassDecoder {
 public:
  ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      ROR_register_cccc0001101s0000ddddmmmm0111nnnn_case_0);
};

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
class RRX_cccc0001101s0000dddd00000110mmmm_case_0
     : public ClassDecoder {
 public:
  RRX_cccc0001101s0000dddd00000110mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      RRX_cccc0001101s0000dddd00000110mmmm_case_0);
};

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
class RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      RSB_immediate_cccc0010011snnnnddddiiiiiiiiiiii_case_0);
};

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
class RSB_register_cccc0000011snnnnddddiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  RSB_register_cccc0000011snnnnddddiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      RSB_register_cccc0000011snnnnddddiiiiitt0mmmm_case_0);
};

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
class RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      RSB_register_shfited_register_cccc0000011snnnnddddssss0tt1mmmm_case_0);
};

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
class RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      RSC_immediate_cccc0010111snnnnddddiiiiiiiiiiii_case_0);
};

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
class RSC_register_cccc0000111snnnnddddiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  RSC_register_cccc0000111snnnnddddiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      RSC_register_cccc0000111snnnnddddiiiiitt0mmmm_case_0);
};

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
class RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      RSC_register_shifted_register_cccc0000111snnnnddddssss0tt1mmmm_case_0);
};

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
class SADD16_cccc01100001nnnndddd11110001mmmm_case_0
     : public ClassDecoder {
 public:
  SADD16_cccc01100001nnnndddd11110001mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SADD16_cccc01100001nnnndddd11110001mmmm_case_0);
};

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
class SADD8_cccc01100001nnnndddd11111001mmmm_case_0
     : public ClassDecoder {
 public:
  SADD8_cccc01100001nnnndddd11111001mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SADD8_cccc01100001nnnndddd11111001mmmm_case_0);
};

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
class SASX_cccc01100001nnnndddd11110011mmmm_case_0
     : public ClassDecoder {
 public:
  SASX_cccc01100001nnnndddd11110011mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SASX_cccc01100001nnnndddd11110011mmmm_case_0);
};

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
class SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0
     : public ClassDecoder {
 public:
  SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SBC_immediate_cccc0010110snnnnddddiiiiiiiiiiii_case_0);
};

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
class SBC_register_cccc0000110snnnnddddiiiiitt0mmmm_case_0
     : public ClassDecoder {
 public:
  SBC_register_cccc0000110snnnnddddiiiiitt0mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SBC_register_cccc0000110snnnnddddiiiiitt0mmmm_case_0);
};

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
class SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0
     : public ClassDecoder {
 public:
  SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SBC_register_shifted_register_cccc0000110snnnnddddssss0tt1mmmm_case_0);
};

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
class SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0
     : public ClassDecoder {
 public:
  SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0);
};

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
class SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0
     : public ClassDecoder {
 public:
  SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SDIV_cccc01110001dddd1111mmmm0001nnnn_case_0);
};

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
class SEL_cccc01101000nnnndddd11111011mmmm_case_0
     : public ClassDecoder {
 public:
  SEL_cccc01101000nnnndddd11111011mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SEL_cccc01101000nnnndddd11111011mmmm_case_0);
};

// SETEND_1111000100000001000000i000000000_case_0:
//
//   {arch: v6,
//    defs: {},
//    pattern: 1111000100000001000000i000000000,
//    rule: SETEND,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class SETEND_1111000100000001000000i000000000_case_0
     : public ClassDecoder {
 public:
  SETEND_1111000100000001000000i000000000_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SETEND_1111000100000001000000i000000000_case_0);
};

// SEV_cccc0011001000001111000000000100_case_0:
//
//   {arch: v6K,
//    defs: {},
//    pattern: cccc0011001000001111000000000100,
//    rule: SEV,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class SEV_cccc0011001000001111000000000100_case_0
     : public ClassDecoder {
 public:
  SEV_cccc0011001000001111000000000100_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SEV_cccc0011001000001111000000000100_case_0);
};

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
class SHADD16_cccc01100011nnnndddd11110001mmmm_case_0
     : public ClassDecoder {
 public:
  SHADD16_cccc01100011nnnndddd11110001mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SHADD16_cccc01100011nnnndddd11110001mmmm_case_0);
};

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
class SHADD8_cccc01100011nnnndddd11111001mmmm_case_0
     : public ClassDecoder {
 public:
  SHADD8_cccc01100011nnnndddd11111001mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SHADD8_cccc01100011nnnndddd11111001mmmm_case_0);
};

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
class SHASX_cccc01100011nnnndddd11110011mmmm_case_0
     : public ClassDecoder {
 public:
  SHASX_cccc01100011nnnndddd11110011mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SHASX_cccc01100011nnnndddd11110011mmmm_case_0);
};

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
class SHSAX_cccc01100011nnnndddd11110101mmmm_case_0
     : public ClassDecoder {
 public:
  SHSAX_cccc01100011nnnndddd11110101mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SHSAX_cccc01100011nnnndddd11110101mmmm_case_0);
};

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
class SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0
     : public ClassDecoder {
 public:
  SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SHSUB16_cccc01100011nnnndddd11110111mmmm_case_0);
};

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
class SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0
     : public ClassDecoder {
 public:
  SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SHSUB8_cccc01100011nnnndddd11111111mmmm_case_0);
};

// SMC_cccc000101100000000000000111iiii_case_0:
//
//   {arch: SE,
//    defs: {},
//    pattern: cccc000101100000000000000111iiii,
//    rule: SMC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
class SMC_cccc000101100000000000000111iiii_case_0
     : public ClassDecoder {
 public:
  SMC_cccc000101100000000000000111iiii_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SMC_cccc000101100000000000000111iiii_case_0);
};

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
class SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0
     : public ClassDecoder {
 public:
  SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SMLABB_SMLABT_SMLATB_SMLATT_cccc00010000ddddaaaammmm1xx0nnnn_case_0);
};

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
class SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0
     : public ClassDecoder {
 public:
  SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SMLAD_cccc01110000ddddaaaammmm00m1nnnn_case_0);
};

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
class SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0
     : public ClassDecoder {
 public:
  SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_0);
};

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
class SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0
     : public ClassDecoder {
 public:
  SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SMLALD_cccc01110100hhhhllllmmmm00m1nnnn_case_0);
};

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
class SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0
     : public ClassDecoder {
 public:
  SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_0);
};

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
class SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0
     : public ClassDecoder {
 public:
  SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SMLAWB_SMLAWT_cccc00010010ddddaaaammmm1x00nnnn_case_0);
};

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
class SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0
     : public ClassDecoder {
 public:
  SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SMLSD_cccc01110000ddddaaaammmm01m1nnnn_case_0);
};

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
class SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0
     : public ClassDecoder {
 public:
  SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      SMLSLD_cccc01110100hhhhllllmmmm01m1nnnn_case_0);
};

} // namespace nacl_arm_test

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_BASELINES_1_H_
