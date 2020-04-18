/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#include "native_client/src/trusted/validator_arm/gen/arm32_decode_baselines.h"
#include "native_client/src/trusted/validator_arm/inst_classes_inline.h"

namespace nacl_arm_dec {

// SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0:
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
//    pattern: cccc01110101ddddaaaammmm00r1nnnn,
//    rule: SMMLA,
//    safety: [Ra  ==
//            Pc => DECODER_ERROR,
//      Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
RegisterList SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0::
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


RegisterList SMMLA_cccc01110101ddddaaaammmm00r1nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0:
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
//    pattern: cccc01110101ddddaaaammmm11r1nnnn,
//    rule: SMMLS,
//    safety: [Ra  ==
//            Pc => DECODER_ERROR,
//      Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
RegisterList SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0::
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


RegisterList SMMLS_cccc01110101ddddaaaammmm11r1nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0:
//
//   {M: M(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110101dddd1111mmmm00r1nnnn,
//    rule: SMMUL,
//    safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//    uses: {Rm, Rn}}
RegisterList SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0::
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


RegisterList SMMUL_cccc01110101dddd1111mmmm00r1nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(11:8), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0:
//
//   {M: M(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110000dddd1111mmmm00m1nnnn,
//    rule: SMUAD,
//    safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//    uses: {Rm, Rn}}
RegisterList SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0::
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


RegisterList SMUAD_cccc01110000dddd1111mmmm00m1nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(11:8), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0:
//
//   {M: M(6),
//    N: N(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v5TE,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(6), N(5), Rn(3:0)],
//    m_high: M(6)=1,
//    n_high: N(5)=1,
//    pattern: cccc00010110dddd0000mmmm1xx0nnnn,
//    rule: SMULBB_SMULBT_SMULTB_SMULTT,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0::
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


RegisterList SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0:
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
RegisterList SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0::
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

SafetyLevel SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0::
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


RegisterList SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0:
//
//   {M: M(6),
//    N: N(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v5TE,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(6), N(5), Rn(3:0)],
//    m_high: M(6)=1,
//    n_high: N(5)=1,
//    pattern: cccc00010010dddd0000mmmm1x10nnnn,
//    rule: SMULWB_SMULWT,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0::
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


RegisterList SMULWB_SMULWT_cccc00010010dddd0000mmmm1x10nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0:
//
//   {M: M(5),
//    Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), M(5), Rn(3:0)],
//    pattern: cccc01110000dddd1111mmmm01m1nnnn,
//    rule: SMUSD,
//    safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//    uses: {Rm, Rn}}
RegisterList SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0::
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


RegisterList SMUSD_cccc01110000dddd1111mmmm01m1nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(11:8), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SRS_1111100pu1w0110100000101000iiiii_case_0:
//
//   {arch: v6,
//    defs: {},
//    pattern: 1111100pu1w0110100000101000iiiii,
//    rule: SRS,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList SRS_1111100pu1w0110100000101000iiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel SRS_1111100pu1w0110100000101000iiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList SRS_1111100pu1w0110100000101000iiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// SSAT16_cccc01101010iiiidddd11110011nnnn_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), sat_imm(19:16), Rd(15:12), Rn(3:0)],
//    pattern: cccc01101010iiiidddd11110011nnnn,
//    rule: SSAT16,
//    safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//    sat_imm: sat_imm(19:16),
//    saturate_to: sat_imm + 1,
//    uses: {Rn}}
RegisterList SSAT16_cccc01101010iiiidddd11110011nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SSAT16_cccc01101010iiiidddd11110011nnnn_case_0::
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


RegisterList SSAT16_cccc01101010iiiidddd11110011nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    arch: v6,
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
RegisterList SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0::
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


RegisterList SSAT_cccc0110101iiiiiddddiiiiis01nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SSAX_cccc01100001nnnndddd11110101mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11110101mmmm,
//    rule: SSAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SSAX_cccc01100001nnnndddd11110101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SSAX_cccc01100001nnnndddd11110101mmmm_case_0::
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


RegisterList SSAX_cccc01100001nnnndddd11110101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11110111mmmm,
//    rule: SSSUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0::
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


RegisterList SSSUB16_cccc01100001nnnndddd11110111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SSUB8_cccc01100001nnnndddd11111111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100001nnnndddd11111111mmmm,
//    rule: SSUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList SSUB8_cccc01100001nnnndddd11111111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SSUB8_cccc01100001nnnndddd11111111mmmm_case_0::
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


RegisterList SSUB8_cccc01100001nnnndddd11111111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0:
//
//   {arch: v5,
//    defs: {},
//    pattern: 1111110pudw0nnnniiiiiiiiiiiiiiii,
//    rule: STC2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList STC2_1111110pudw0nnnniiiiiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc110pudw0nnnnddddcccciiiiiiii,
//    rule: STC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList STC_cccc110pudw0nnnnddddcccciiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0:
//
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    base: Rn,
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
Register STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0::
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
  //       Contains(RegisterList(inst(15:0)), inst(19:16)) &&
  //       SmallestGPR(RegisterList(inst(15:0)))  !=
  //          inst(19:16) => UNKNOWN
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(((inst.Bits() & 0x000F0000) >> 16)))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) != (nacl_arm_dec::SmallestGPR(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)))))))
    return UNKNOWN;

  return MAY_BE_SAFE;
}


bool STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: 'Union({inst(19:16)}, RegisterList(inst(15:0)))'
  return nacl_arm_dec::Union(RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))), nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)));
}

ViolationSet STMDA_STMED_cccc100000w0nnnnrrrrrrrrrrrrrrrr_case_0::
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


// STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0:
//
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    base: Rn,
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
Register STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0::
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
  //       Contains(RegisterList(inst(15:0)), inst(19:16)) &&
  //       SmallestGPR(RegisterList(inst(15:0)))  !=
  //          inst(19:16) => UNKNOWN
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(((inst.Bits() & 0x000F0000) >> 16)))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) != (nacl_arm_dec::SmallestGPR(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)))))))
    return UNKNOWN;

  return MAY_BE_SAFE;
}


bool STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: 'Union({inst(19:16)}, RegisterList(inst(15:0)))'
  return nacl_arm_dec::Union(RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))), nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)));
}

ViolationSet STMDB_STMFD_cccc100100w0nnnnrrrrrrrrrrrrrrrr_case_0::
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


// STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0:
//
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    base: Rn,
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
Register STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0::
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
  //       Contains(RegisterList(inst(15:0)), inst(19:16)) &&
  //       SmallestGPR(RegisterList(inst(15:0)))  !=
  //          inst(19:16) => UNKNOWN
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(((inst.Bits() & 0x000F0000) >> 16)))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) != (nacl_arm_dec::SmallestGPR(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)))))))
    return UNKNOWN;

  return MAY_BE_SAFE;
}


bool STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: 'Union({inst(19:16)}, RegisterList(inst(15:0)))'
  return nacl_arm_dec::Union(RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))), nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)));
}

ViolationSet STMIB_STMFA_cccc100110w0nnnnrrrrrrrrrrrrrrrr_case_0::
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


// STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0:
//
//   {None: 32,
//    Pc: 15,
//    Rn: Rn(19:16),
//    W: W(21),
//    base: Rn,
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
Register STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00200000)  ==
          0x00200000
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0::
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
  //       Contains(RegisterList(inst(15:0)), inst(19:16)) &&
  //       SmallestGPR(RegisterList(inst(15:0)))  !=
  //          inst(19:16) => UNKNOWN
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       (nacl_arm_dec::Contains(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)), Register(((inst.Bits() & 0x000F0000) >> 16)))) &&
       (((((inst.Bits() & 0x000F0000) >> 16)) != (nacl_arm_dec::SmallestGPR(nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)))))))
    return UNKNOWN;

  return MAY_BE_SAFE;
}


bool STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0::
base_address_register_writeback_small_immediate(
      Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // small_imm_base_wb: 'inst(21)=1'
  return (inst.Bits() & 0x00200000)  ==
          0x00200000;
}

RegisterList STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: 'Union({inst(19:16)}, RegisterList(inst(15:0)))'
  return nacl_arm_dec::Union(RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))), nacl_arm_dec::RegisterList((inst.Bits() & 0x0000FFFF)));
}

ViolationSet STM_STMIA_STMEA_cccc100010w0nnnnrrrrrrrrrrrrrrrr_case_0::
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


// STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0:
//
//   {defs: {},
//    pattern: cccc100pu100nnnnrrrrrrrrrrrrrrrr,
//    rule: STM_User_registers,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList STM_User_registers_cccc100pu100nnnnrrrrrrrrrrrrrrrr_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc0100u110nnnnttttiiiiiiiiiiii,
//    rule: STRBT_A1,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList STRBT_A1_cccc0100u110nnnnttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0:
//
//   {defs: {},
//    pattern: cccc0110u110nnnnttttiiiiitt0mmmm,
//    rule: STRBT_A2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList STRBT_A2_cccc0110u110nnnnttttiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0:
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
Register STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0::
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
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
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

  return MAY_BE_SAFE;
}


bool STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0::
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

RegisterList STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

ViolationSet STRB_immediate_cccc010pu1w0nnnnttttiiiiiiiiiiii_case_0::
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


// STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0:
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
Register STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0::
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
  //          inst(3:0) ||
  //       15  ==
  //          inst(15:12) => UNPREDICTABLE
  if ((((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x0000F000) >> 12)))))
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


RegisterList STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

ViolationSet STRB_register_cccc011pu1w0nnnnttttiiiiitt0mmmm_case_0::
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


// STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0:
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
//    base: Rn,
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
Register STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0::
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

  // 15  ==
  //          inst(15:12) + 1 => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12) + 1) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


bool STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0::
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

RegisterList STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(15:12) + 1, inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1)).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_0::
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


// STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0:
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
//    base: Rn,
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
Register STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0::
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
  //          inst(3:0) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12) + 1) == (15))) ||
       ((((inst.Bits() & 0x0000000F)) == (15))))
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


RegisterList STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(15:12) + 1, inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12) + 1)).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_0::
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


// STREXB_cccc00011100nnnndddd11111001tttt_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    Rt: Rt(3:0),
//    arch: v6K,
//    base: Rn,
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
Register STREXB_cccc00011100nnnndddd11111001tttt_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STREXB_cccc00011100nnnndddd11111001tttt_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel STREXB_cccc00011100nnnndddd11111001tttt_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12)) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList STREXB_cccc00011100nnnndddd11111001tttt_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet STREXB_cccc00011100nnnndddd11111001tttt_case_0::
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


// STREXD_cccc00011010nnnndddd11111001tttt_case_0:
//
//   {Lr: 14,
//    Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    Rt: Rt(3:0),
//    Rt2: Rt + 1,
//    arch: v6K,
//    base: Rn,
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
Register STREXD_cccc00011010nnnndddd11111001tttt_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STREXD_cccc00011010nnnndddd11111001tttt_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel STREXD_cccc00011010nnnndddd11111001tttt_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(19:16) ||
  //       inst(3:0)(0)=1 ||
  //       14  ==
  //          inst(3:0) => UNPREDICTABLE
  if (((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16))))) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x0000000F)) == (14))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(3:0) ||
  //       inst(15:12)  ==
  //          inst(3:0) + 1 => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12)) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F)))) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F) + 1))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList STREXD_cccc00011010nnnndddd11111001tttt_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(3:0) + 1}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register((inst.Bits() & 0x0000000F) + 1));
}

ViolationSet STREXD_cccc00011010nnnndddd11111001tttt_case_0::
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


// STREXH_cccc00011110nnnndddd11111001tttt_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    Rt: Rt(3:0),
//    arch: v6K,
//    base: Rn,
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
Register STREXH_cccc00011110nnnndddd11111001tttt_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STREXH_cccc00011110nnnndddd11111001tttt_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel STREXH_cccc00011110nnnndddd11111001tttt_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12)) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList STREXH_cccc00011110nnnndddd11111001tttt_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet STREXH_cccc00011110nnnndddd11111001tttt_case_0::
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


// STREXH_cccc00011111nnnntttt111110011111_case_0:
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
//    pattern: cccc00011111nnnntttt111110011111,
//    rule: STREXH,
//    safety: [Pc in {Rt, Rn} => UNPREDICTABLE],
//    uses: {Rn},
//    violations: [implied by 'base']}
Register STREXH_cccc00011111nnnntttt111110011111_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STREXH_cccc00011111nnnntttt111110011111_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel STREXH_cccc00011111nnnntttt111110011111_case_0::
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


RegisterList STREXH_cccc00011111nnnntttt111110011111_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet STREXH_cccc00011111nnnntttt111110011111_case_0::
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


// STREX_cccc00011000nnnndddd11111001tttt_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(19:16),
//    Rt: Rt(3:0),
//    arch: v6,
//    base: Rn,
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
Register STREX_cccc00011000nnnndddd11111001tttt_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STREX_cccc00011000nnnndddd11111001tttt_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel STREX_cccc00011000nnnndddd11111001tttt_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) ||
  //       15  ==
  //          inst(19:16) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x000F0000) >> 16)))))
    return UNPREDICTABLE;

  // inst(15:12)  ==
  //          inst(19:16) ||
  //       inst(15:12)  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((((inst.Bits() & 0x0000F000) >> 12)) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList STREX_cccc00011000nnnndddd11111001tttt_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet STREX_cccc00011000nnnndddd11111001tttt_case_0::
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


// STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0:
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
Register STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0::
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
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
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

  return MAY_BE_SAFE;
}


bool STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0::
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

RegisterList STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

ViolationSet STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_0::
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


// STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0:
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
Register STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if (inst(24)=0) ||
  //       (inst(21)=1)
  //       else 32}'
  return RegisterList().
   Add(Register(((((inst.Bits() & 0x01000000)  ==
          0x00000000)) ||
       (((inst.Bits() & 0x00200000)  ==
          0x00200000))
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0::
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


RegisterList STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

ViolationSet STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_0::
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


// STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc0100u010nnnnttttiiiiiiiiiiii,
//    rule: STRT_A1,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList STRT_A1_cccc0100u010nnnnttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0:
//
//   {defs: {},
//    pattern: cccc0110u010nnnnttttiiiiitt0mmmm,
//    rule: STRT_A2,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList STRT_A2_cccc0110u010nnnnttttiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0:
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
Register STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(24)=0 &&
  //       inst(21)=1 => DECODER_ERROR
  if (((inst.Bits() & 0x01000000)  ==
          0x00000000) &&
       ((inst.Bits() & 0x00200000)  ==
          0x00200000))
    return DECODER_ERROR;

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

  return MAY_BE_SAFE;
}


bool STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0::
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

RegisterList STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

ViolationSet STR_immediate_cccc010pu0w0nnnnttttiiiiiiiiiiii_case_0::
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


// STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0:
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
Register STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0::
base_address_register(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // base: 'inst(19:16)'
  return Register(((inst.Bits() & 0x000F0000) >> 16));
}

RegisterList STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)
  //       if inst(24)=0 ||
  //       inst(21)=1
  //       else 32}'
  return RegisterList().
   Add(Register((((inst.Bits() & 0x01000000)  ==
          0x00000000) ||
       ((inst.Bits() & 0x00200000)  ==
          0x00200000)
       ? ((inst.Bits() & 0x000F0000) >> 16)
       : 32)));
}

SafetyLevel STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0::
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

  return MAY_BE_SAFE;
}


RegisterList STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(19:16), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

ViolationSet STR_register_cccc011pd0w0nnnnttttiiiiitt0mmmm_case_0::
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


// SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0:
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
//    pattern: cccc0010010snnnnddddiiiiiiiiiiii,
//    rule: SUB_immediate,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      (Rn(19:16)=1111 &&
//         S(20)=0) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    setflags: S(20)=1,
//    uses: {Rn}}
RegisterList SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0::
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

SafetyLevel SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0::
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


RegisterList SUB_immediate_cccc0010010snnnnddddiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// SUB_register_cccc0000010snnnnddddiiiiitt0mmmm_case_0:
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
//    pattern: cccc0000010snnnnddddiiiiitt0mmmm,
//    rule: SUB_register,
//    safety: [(Rd(15:12)=1111 &&
//         S(20)=1) => DECODER_ERROR,
//      Rd(15:12)=1111 => FORBIDDEN_OPERANDS],
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList SUB_register_cccc0000010snnnnddddiiiiitt0mmmm_case_0::
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

SafetyLevel SUB_register_cccc0000010snnnnddddiiiiitt0mmmm_case_0::
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


RegisterList SUB_register_cccc0000010snnnnddddiiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0:
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
//    pattern: cccc0000010snnnnddddssss0tt1mmmm,
//    rule: SUB_register_shifted_register,
//    safety: [Pc in {Rn, Rd, Rm, Rs} => UNPREDICTABLE],
//    setflags: S(20)=1,
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0::
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

SafetyLevel SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0::
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


RegisterList SUB_register_shifted_register_cccc0000010snnnnddddssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0:
//
//   {defs: {},
//    pattern: cccc1111iiiiiiiiiiiiiiiiiiiiiiii,
//    rule: SVC,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList SVC_cccc1111iiiiiiiiiiiiiiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_0:
//
//   {defs: {},
//    pattern: cccc00010b00nnnntttt00001001tttt,
//    rule: SWP_SWPB,
//    safety: [true => DEPRECATED],
//    true: true,
//    uses: {}}
RegisterList SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => DEPRECATED
  if (true)
    return DEPRECATED;

  return MAY_BE_SAFE;
}


RegisterList SWP_SWPB_cccc00010b00nnnntttt00001001tttt_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
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
RegisterList SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SXTAB16_cccc01101000nnnnddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
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
RegisterList SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SXTAB_cccc01101010nnnnddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
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
RegisterList SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList SXTAH_cccc01101011nnnnddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SXTB16_cccc011010001111ddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011010001111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: SXTB16,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
RegisterList SXTB16_cccc011010001111ddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SXTB16_cccc011010001111ddddrr000111mmmm_case_0::
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


RegisterList SXTB16_cccc011010001111ddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SXTB_cccc011010101111ddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011010101111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: SXTB,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
RegisterList SXTB_cccc011010101111ddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SXTB_cccc011010101111ddddrr000111mmmm_case_0::
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


RegisterList SXTB_cccc011010101111ddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// SXTH_cccc011010111111ddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011010111111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: SXTH,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
RegisterList SXTH_cccc011010111111ddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel SXTH_cccc011010111111ddddrr000111mmmm_case_0::
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


RegisterList SXTH_cccc011010111111ddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    Rn: Rn(19:16),
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), imm12(11:0)],
//    imm12: imm12(11:0),
//    imm32: ARMExpandImm_C(imm12),
//    pattern: cccc00110011nnnn0000iiiiiiiiiiii,
//    rule: TEQ_immediate,
//    uses: {Rn}}
RegisterList TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16}'
  return RegisterList().
   Add(Register(16));
}

SafetyLevel TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList TEQ_immediate_cccc00110011nnnn0000iiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// TEQ_register_cccc00010011nnnn0000iiiiitt0mmmm_case_0:
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
//    pattern: cccc00010011nnnn0000iiiiitt0mmmm,
//    rule: TEQ_register,
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList TEQ_register_cccc00010011nnnn0000iiiiitt0mmmm_case_0::
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

SafetyLevel TEQ_register_cccc00010011nnnn0000iiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList TEQ_register_cccc00010011nnnn0000iiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), Rs(11:8), type(6:5), Rm(3:0)],
//    pattern: cccc00010011nnnn0000ssss0tt1mmmm,
//    rule: TEQ_register_shifted_register,
//    safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16}'
  return RegisterList().
   Add(Register(16));
}

SafetyLevel TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0::
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


RegisterList TEQ_register_shifted_register_cccc00010011nnnn0000ssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0:
//
//   {NZCV: 16,
//    Rn: Rn(19:16),
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
RegisterList TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16}'
  return RegisterList().
   Add(Register(16));
}

SafetyLevel TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


bool TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0::
sets_Z_if_bits_clear(
      Instruction inst, Register test_register,
      uint32_t clears_mask) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // sets_Z_if_clear_bits: 'RegIndex(test_register())  ==
  //          inst(19:16) &&
  //       (ARMExpandImm_C(inst(11:0)) &&
  //       clears_mask())  ==
  //          clears_mask()'
  return (((((inst.Bits() & 0x000F0000) >> 16)) == (nacl_arm_dec::RegIndex(test_register)))) &&
       (((((nacl_arm_dec::ARMExpandImm_C((inst.Bits() & 0x00000FFF)) & clears_mask))) == (clears_mask)));
}

RegisterList TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

// TST_register_cccc00010001nnnn0000iiiiitt0mmmm_case_0:
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
//    pattern: cccc00010001nnnn0000iiiiitt0mmmm,
//    rule: TST_register,
//    shift: DecodeImmShift(type, imm5),
//    type: type(6:5),
//    uses: {Rn, Rm}}
RegisterList TST_register_cccc00010001nnnn0000iiiiitt0mmmm_case_0::
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

SafetyLevel TST_register_cccc00010001nnnn0000iiiiitt0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  return MAY_BE_SAFE;
}


RegisterList TST_register_cccc00010001nnnn0000iiiiitt0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0:
//
//   {NZCV: 16,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Rs: Rs(11:8),
//    cond: cond(31:28),
//    defs: {NZCV},
//    fields: [cond(31:28), Rn(19:16), Rs(11:8), type(6:5), Rm(3:0)],
//    pattern: cccc00010001nnnn0000ssss0tt1mmmm,
//    rule: TST_register_shifted_register,
//    safety: [Pc in {Rn, Rm, Rs} => UNPREDICTABLE],
//    shift_t: DecodeRegShift(type),
//    type: type(6:5),
//    uses: {Rn, Rm, Rs}}
RegisterList TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{16}'
  return RegisterList().
   Add(Register(16));
}

SafetyLevel TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0::
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


RegisterList TST_register_shifted_register_cccc00010001nnnn0000ssss0tt1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// UADD16_cccc01100101nnnndddd11110001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11110001mmmm,
//    rule: UADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UADD16_cccc01100101nnnndddd11110001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UADD16_cccc01100101nnnndddd11110001mmmm_case_0::
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


RegisterList UADD16_cccc01100101nnnndddd11110001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UADD8_cccc01100101nnnndddd11111001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11111001mmmm,
//    rule: UADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UADD8_cccc01100101nnnndddd11111001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UADD8_cccc01100101nnnndddd11111001mmmm_case_0::
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


RegisterList UADD8_cccc01100101nnnndddd11111001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UASX_cccc01100101nnnndddd11110011mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11110011mmmm,
//    rule: UASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UASX_cccc01100101nnnndddd11110011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UASX_cccc01100101nnnndddd11110011mmmm_case_0::
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


RegisterList UASX_cccc01100101nnnndddd11110011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    arch: v6T2,
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
RegisterList UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0::
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


RegisterList UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0:
//
//   {defs: {},
//    inst: inst,
//    pattern: cccc01111111iiiiiiiiiiii1111iiii,
//    rule: UDF,
//    safety: [not IsUDFNaClSafe(inst) => FORBIDDEN_OPERANDS],
//    uses: {}}
RegisterList UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // not IsUDFNaClSafe(inst) => FORBIDDEN_OPERANDS
  if (!(nacl_arm_dec::IsUDFNaClSafe(inst.Bits())))
    return FORBIDDEN_OPERANDS;

  return MAY_BE_SAFE;
}


RegisterList UDF_cccc01111111iiiiiiiiiiii1111iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0:
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
//    pattern: cccc01110011dddd1111mmmm0001nnnn,
//    rule: UDIV,
//    safety: [Pc in {Rd, Rm, Rn} => UNPREDICTABLE],
//    uses: {Rm, Rn}}
RegisterList UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0::
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


RegisterList UDIV_cccc01110011dddd1111mmmm0001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(11:8), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UHADD16_cccc01100111nnnndddd11110001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11110001mmmm,
//    rule: UHADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UHADD16_cccc01100111nnnndddd11110001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UHADD16_cccc01100111nnnndddd11110001mmmm_case_0::
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


RegisterList UHADD16_cccc01100111nnnndddd11110001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UHADD8_cccc01100111nnnndddd11111001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11111001mmmm,
//    rule: UHADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UHADD8_cccc01100111nnnndddd11111001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UHADD8_cccc01100111nnnndddd11111001mmmm_case_0::
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


RegisterList UHADD8_cccc01100111nnnndddd11111001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UHASX_cccc01100111nnnndddd11110011mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11110011mmmm,
//    rule: UHASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UHASX_cccc01100111nnnndddd11110011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UHASX_cccc01100111nnnndddd11110011mmmm_case_0::
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


RegisterList UHASX_cccc01100111nnnndddd11110011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UHSAX_cccc01100111nnnndddd11110101mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11110101mmmm,
//    rule: UHSAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UHSAX_cccc01100111nnnndddd11110101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UHSAX_cccc01100111nnnndddd11110101mmmm_case_0::
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


RegisterList UHSAX_cccc01100111nnnndddd11110101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11110111mmmm,
//    rule: UHSUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0::
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


RegisterList UHSUB16_cccc01100111nnnndddd11110111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100111nnnndddd11111111mmmm,
//    rule: UHSUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0::
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


RegisterList UHSUB8_cccc01100111nnnndddd11111111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0:
//
//   {Pc: 15,
//    RdHi: RdHi(19:16),
//    RdLo: RdLo(15:12),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    cond: cond(31:28),
//    defs: {RdLo, RdHi},
//    fields: [cond(31:28), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//    pattern: cccc00000100hhhhllllmmmm1001nnnn,
//    rule: UMAAL_A1,
//    safety: [Pc in {RdLo, RdHi, Rn, Rm} => UNPREDICTABLE,
//      RdHi  ==
//            RdLo => UNPREDICTABLE],
//    uses: {RdLo, RdHi, Rn, Rm}}
RegisterList UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12), inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0::
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


RegisterList UMAAL_A1_cccc00000100hhhhllllmmmm1001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0:
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
RegisterList UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0::
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

SafetyLevel UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0::
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


RegisterList UMLAL_A1_cccc0000101shhhhllllmmmm1001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12), inst(19:16), inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12))).
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0:
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
RegisterList UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0::
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

SafetyLevel UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0::
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


RegisterList UMULL_A1_cccc0000100shhhhllllmmmm1001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// UQADD16_cccc01100110nnnndddd11110001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11110001mmmm,
//    rule: UQADD16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UQADD16_cccc01100110nnnndddd11110001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UQADD16_cccc01100110nnnndddd11110001mmmm_case_0::
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


RegisterList UQADD16_cccc01100110nnnndddd11110001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UQADD8_cccc01100110nnnndddd11111001mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11111001mmmm,
//    rule: UQADD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UQADD8_cccc01100110nnnndddd11111001mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UQADD8_cccc01100110nnnndddd11111001mmmm_case_0::
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


RegisterList UQADD8_cccc01100110nnnndddd11111001mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UQASX_cccc01100110nnnndddd11110011mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11110011mmmm,
//    rule: UQASX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UQASX_cccc01100110nnnndddd11110011mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UQASX_cccc01100110nnnndddd11110011mmmm_case_0::
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


RegisterList UQASX_cccc01100110nnnndddd11110011mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UQSAX_cccc01100110nnnndddd11110101mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11110101mmmm,
//    rule: UQSAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UQSAX_cccc01100110nnnndddd11110101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UQSAX_cccc01100110nnnndddd11110101mmmm_case_0::
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


RegisterList UQSAX_cccc01100110nnnndddd11110101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11110111mmmm,
//    rule: UQSUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0::
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


RegisterList UQSUB16_cccc01100110nnnndddd11110111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100110nnnndddd11111111mmmm,
//    rule: UQSUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0::
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


RegisterList UQSUB8_cccc01100110nnnndddd11111111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0:
//
//   {Pc: 15,
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Rm(11:8), Rn(3:0)],
//    pattern: cccc01111000dddd1111mmmm0001nnnn,
//    rule: USAD8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0::
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


RegisterList USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8)));
}

// USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0:
//
//   {Pc: 15,
//    Ra: Ra(15:12),
//    Rd: Rd(19:16),
//    Rm: Rm(11:8),
//    Rn: Rn(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//    pattern: cccc01111000ddddaaaammmm0001nnnn,
//    rule: USADA8,
//    safety: [Ra  ==
//            Pc => DECODER_ERROR,
//      Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm, Ra}}
RegisterList USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(19:16)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)));
}

SafetyLevel USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0::
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


RegisterList USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0), inst(11:8), inst(15:12)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F))).
   Add(Register(((inst.Bits() & 0x00000F00) >> 8))).
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// USAT16_cccc01101110iiiidddd11110011nnnn_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), sat_imm(19:16), Rd(15:12), Rn(3:0)],
//    pattern: cccc01101110iiiidddd11110011nnnn,
//    rule: USAT16,
//    safety: [Pc in {Rd, Rn} => UNPREDICTABLE],
//    sat_imm: sat_imm(19:16),
//    saturate_to: sat_imm,
//    uses: {Rn}}
RegisterList USAT16_cccc01101110iiiidddd11110011nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel USAT16_cccc01101110iiiidddd11110011nnnn_case_0::
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


RegisterList USAT16_cccc01101110iiiidddd11110011nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rn: Rn(3:0),
//    arch: v6,
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
RegisterList USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0::
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


RegisterList USAT_cccc0110111iiiiiddddiiiiis01nnnn_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// USAX_cccc01100101nnnndddd11110101mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11110101mmmm,
//    rule: USAX,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList USAX_cccc01100101nnnndddd11110101mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel USAX_cccc01100101nnnndddd11110101mmmm_case_0::
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


RegisterList USAX_cccc01100101nnnndddd11110101mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// USUB16_cccc01100101nnnndddd11110111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11110111mmmm,
//    rule: USUB16,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList USUB16_cccc01100101nnnndddd11110111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel USUB16_cccc01100101nnnndddd11110111mmmm_case_0::
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


RegisterList USUB16_cccc01100101nnnndddd11110111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// USUB8_cccc01100101nnnndddd11111111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rn(19:16), Rd(15:12), Rm(3:0)],
//    pattern: cccc01100101nnnndddd11111111mmmm,
//    rule: USUB8,
//    safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//    uses: {Rn, Rm}}
RegisterList USUB8_cccc01100101nnnndddd11111111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel USUB8_cccc01100101nnnndddd11111111mmmm_case_0::
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


RegisterList USUB8_cccc01100101nnnndddd11111111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
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
RegisterList UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList UXTAB16_cccc01101100nnnnddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
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
RegisterList UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList UXTAB_cccc01101110nnnnddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    arch: v6,
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
RegisterList UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0x000F0000)  ==
          0x000F0000)
    return DECODER_ERROR;

  // 15  ==
  //          inst(15:12) ||
  //       15  ==
  //          inst(3:0) => UNPREDICTABLE
  if ((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList UXTAH_cccc01101111nnnnddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(19:16), inst(3:0)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16))).
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UXTB16_cccc011011001111ddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011011001111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: UXTB16,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
RegisterList UXTB16_cccc011011001111ddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UXTB16_cccc011011001111ddddrr000111mmmm_case_0::
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


RegisterList UXTB16_cccc011011001111ddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UXTB_cccc011011101111ddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011011101111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: UXTB,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
RegisterList UXTB_cccc011011101111ddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UXTB_cccc011011101111ddddrr000111mmmm_case_0::
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


RegisterList UXTB_cccc011011101111ddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// UXTH_cccc011011111111ddddrr000111mmmm_case_0:
//
//   {Pc: 15,
//    Rd: Rd(15:12),
//    Rm: Rm(3:0),
//    arch: v6,
//    cond: cond(31:28),
//    defs: {Rd},
//    fields: [cond(31:28), Rd(15:12), rotate(11:10), Rm(3:0)],
//    pattern: cccc011011111111ddddrr000111mmmm,
//    rotate: rotate(11:10),
//    rotation: rotate:'000'(2:0),
//    rule: UXTH,
//    safety: [Pc in {Rd, Rm} => UNPREDICTABLE],
//    uses: {Rm}}
RegisterList UXTH_cccc011011111111ddddrr000111mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

SafetyLevel UXTH_cccc011011111111ddddrr000111mmmm_case_0::
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


RegisterList UXTH_cccc011011111111ddddrr000111mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(3:0)}'
  return RegisterList().
   Add(Register((inst.Bits() & 0x0000000F)));
}

// Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0:
//
//   {arch: MPExt,
//    defs: {},
//    pattern: 11110100x001xxxxxxxxxxxxxxxxxxxx,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_11110100x001xxxxxxxxxxxxxxxxxxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0:
//
//   {defs: {},
//    pattern: 11110100xx11xxxxxxxxxxxxxxxxxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
RegisterList Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNPREDICTABLE
  if (true)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_11110100xx11xxxxxxxxxxxxxxxxxxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0:
//
//   {defs: {},
//    pattern: 111101010011xxxxxxxxxxxxxxxxxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
RegisterList Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNPREDICTABLE
  if (true)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_111101010011xxxxxxxxxxxxxxxxxxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0:
//
//   {defs: {},
//    pattern: 111101010111xxxxxxxxxxxx0000xxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
RegisterList Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNPREDICTABLE
  if (true)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_111101010111xxxxxxxxxxxx0000xxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0:
//
//   {defs: {},
//    pattern: 111101010111xxxxxxxxxxxx001xxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
RegisterList Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNPREDICTABLE
  if (true)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_111101010111xxxxxxxxxxxx001xxxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0:
//
//   {defs: {},
//    pattern: 111101010111xxxxxxxxxxxx0111xxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
RegisterList Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNPREDICTABLE
  if (true)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_111101010111xxxxxxxxxxxx0111xxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0:
//
//   {defs: {},
//    pattern: 111101010111xxxxxxxxxxxx1xxxxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
RegisterList Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNPREDICTABLE
  if (true)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_111101010111xxxxxxxxxxxx1xxxxxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0:
//
//   {defs: {},
//    pattern: 111101011x11xxxxxxxxxxxxxxxxxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
RegisterList Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNPREDICTABLE
  if (true)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_111101011x11xxxxxxxxxxxxxxxxxxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0:
//
//   {defs: {},
//    pattern: 11110101x001xxxxxxxxxxxxxxxxxxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
RegisterList Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNPREDICTABLE
  if (true)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_11110101x001xxxxxxxxxxxxxxxxxxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0:
//
//   {arch: MPExt,
//    defs: {},
//    pattern: 11110110x001xxxxxxxxxxxxxxx0xxxx,
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_11110110x001xxxxxxxxxxxxxxx0xxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0:
//
//   {defs: {},
//    pattern: 1111011xxx11xxxxxxxxxxxxxxx0xxxx,
//    safety: [true => UNPREDICTABLE],
//    true: true,
//    uses: {}}
RegisterList Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNPREDICTABLE
  if (true)
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_1111011xxx11xxxxxxxxxxxxxxx0xxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_case_0:
//
//   {defs: {},
//    safety: [true => FORBIDDEN],
//    true: true,
//    uses: {}}
RegisterList Unnamed_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => FORBIDDEN
  if (true)
    return FORBIDDEN;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_case_1:
//
//   {defs: {},
//    safety: [true => UNDEFINED],
//    true: true,
//    uses: {}}
RegisterList Unnamed_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNDEFINED
  if (true)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0:
//
//   {defs: {},
//    pattern: cccc00000101xxxxxxxxxxxx1001xxxx,
//    safety: [true => UNDEFINED],
//    true: true,
//    uses: {}}
RegisterList Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNDEFINED
  if (true)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_cccc00000101xxxxxxxxxxxx1001xxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0:
//
//   {defs: {},
//    pattern: cccc00000111xxxxxxxxxxxx1001xxxx,
//    safety: [true => UNDEFINED],
//    true: true,
//    uses: {}}
RegisterList Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNDEFINED
  if (true)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_cccc00000111xxxxxxxxxxxx1001xxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0:
//
//   {defs: {},
//    pattern: cccc1100000xnnnnxxxxccccxxxoxxxx,
//    safety: [true => UNDEFINED],
//    true: true,
//    uses: {}}
RegisterList Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => UNDEFINED
  if (true)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList Unnamed_cccc1100000xnnnnxxxxccccxxxoxxxx_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
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
RegisterList VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // inst(15:12)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VABAL_A2_1111001u1dssnnnndddd0101n0m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
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
RegisterList VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)=11 => UNDEFINED
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VABA_1111001u0dssnnnndddd0111nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
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
RegisterList VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // inst(15:12)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VABDL_integer_A2_1111001u1dssnnnndddd0111n0m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
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
RegisterList VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)=11 => UNDEFINED
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VABD_1111001u0dssnnnndddd0111nqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
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
//    op1_neg: size(1),
//    pattern: 111100110d1snnnndddd1101nqm0mmmm,
//    rule: VABD_floating_point,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
RegisterList VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x00300000) >> 20) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f110qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VABS_A1,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=11 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  ==
          0x000C0000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f110qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VABS_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(19:18)=~10 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00080000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VABS_cccc11101d110000dddd101s11m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    advsimd: false,
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), M(5), Vm(3:0)],
//    m: Vm:D
//         if sz(8)=0
//         else M:Vm,
//    pattern: cccc11101d110000dddd101s11m0mmmm,
//    rule: VABS,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {}}
RegisterList VABS_cccc11101d110000dddd101s11m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VABS_cccc11101d110000dddd101s11m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList VABS_cccc11101d110000dddd101s11m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
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
//    op1_neg: size(1),
//    pattern: 111100110dssnnnndddd1110nqm1mmmm,
//    rule: VACGE,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
RegisterList VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x00300000) >> 20) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
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
//    op1_neg: size(1),
//    pattern: 111100110dssnnnndddd1110nqm1mmmm,
//    rule: VACGT,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
RegisterList VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x00300000) >> 20) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
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
//    pattern: 111100101dssnnnndddd0100n0m0mmmm,
//    rule: VADDHN,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vn(0)=1 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
RegisterList VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1 => UNDEFINED
  if (((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
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
//    is_vaddw: U(24)=1,
//    is_w: op(8)=1,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd000pn0m0mmmm,
//    rule: VADDL_VADDW,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vd(0)=1 ||
//         (op(8)=1 &&
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
RegisterList VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:20)=11 => DECODER_ERROR
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return DECODER_ERROR;

  // inst(15:12)(0)=1 ||
  //       (inst(8)=1 &&
  //       inst(19:16)(0)=1) => UNDEFINED
  if (((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x00000100)  ==
          0x00000100) &&
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
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
//    op1_neg: size(1),
//    pattern: 111100100d0snnnndddd1101nqm0mmmm,
//    rule: VADD_floating_point_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
RegisterList VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x00300000) >> 20) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    advsimd: false,
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    pattern: cccc11100d11nnnndddd101sn0m0mmmm,
//    rule: VADD_floating_point,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
RegisterList VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(31:28)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0xF0000000)  ==
          0xF0000000)
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
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
//    pattern: 111100100dssnnnndddd1000nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VADD_integer,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
RegisterList VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
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
//    pattern: 111100100d00nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VAND_register,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
RegisterList VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0:
//
//   {D: D(22),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    arch: ASIMD,
//    cmode: cmode(11:8),
//    d: D:Vd,
//    defs: {},
//    fields: [i(24),
//      D(22),
//      imm3(18:16),
//      Vd(15:12),
//      cmode(11:8),
//      Q(6),
//      op(5),
//      imm4(3:0)],
//    i: i(24),
//    imm3: imm3(18:16),
//    imm4: imm4(3:0),
//    imm64: AdvSIMDExpandImm(op, cmode, i:imm3:imm4),
//    op: op(5),
//    pattern: 1111001i1d000mmmddddcccc0q11mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VBIC_immediate,
//    safety: [cmode(0)=0 ||
//         cmode(3:2)=11 => DECODER_ERROR,
//      Q(6)=1 &&
//         Vd(0)=1 => UNDEFINED],
//    uses: {}}
RegisterList VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(11:8)(0)=0 ||
  //       inst(11:8)(3:2)=11 => DECODER_ERROR
  if (((((inst.Bits() & 0x00000F00) >> 8) & 0x00000001)  ==
          0x00000000) ||
       ((((inst.Bits() & 0x00000F00) >> 8) & 0x0000000C)  ==
          0x0000000C))
    return DECODER_ERROR;

  // inst(6)=1 &&
  //       inst(15:12)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
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
//    pattern: 111100100d01nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VBIC_register,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
RegisterList VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
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
//    pattern: 111100110d11nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VBIF,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
RegisterList VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
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
//    pattern: 111100110d10nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VBIT,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
RegisterList VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
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
//    pattern: 111100110d01nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VBSL,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
RegisterList VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f010qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCEQ_immediate_0,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=11 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  ==
          0x000C0000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f010qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCEQ_immediate_0,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(19:18)=~10 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00080000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
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
RegisterList VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)=11 => UNDEFINED
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCEQ_register_A1_111100110dssnnnndddd1000nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
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
//    op1_neg: size(1),
//    pattern: 111100100d0snnnndddd1110nqm0mmmm,
//    rule: VCEQ_register_A2,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
RegisterList VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x00300000) >> 20) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f001qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCGE_immediate_0,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=11 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  ==
          0x000C0000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f001qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCGE_immediate_0,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(19:18)=~10 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00080000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
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
RegisterList VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)=11 => UNDEFINED
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCGE_register_A1_1111001u0dssnnnndddd0011nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
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
//    op1_neg: size(1),
//    pattern: 111100110d0snnnndddd1110nqm0mmmm,
//    rule: VCGE_register_A2,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
RegisterList VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x00300000) >> 20) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f000qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCGT_immediate_0,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=11 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  ==
          0x000C0000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f000qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCGT_immediate_0,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(19:18)=~10 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00080000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
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
RegisterList VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)=11 => UNDEFINED
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCGT_register_A1_1111001u0dssnnnndddd0011nqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
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
//    op1_neg: size(1),
//    pattern: 111100110d1snnnndddd1110nqm0mmmm,
//    rule: VCGT_register_A2,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
RegisterList VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x00300000) >> 20) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f011qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLE_immediate_0,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=11 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  ==
          0x000C0000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f011qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLE_immediate_0,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(19:18)=~10 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00080000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCLS_111100111d11ss00dddd01000qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01000qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLS,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCLS_111100111d11ss00dddd01000qm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCLS_111100111d11ss00dddd01000qm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=11 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  ==
          0x000C0000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCLS_111100111d11ss00dddd01000qm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f100qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLT_immediate_0,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=11 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  ==
          0x000C0000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f100qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLT_immediate_0,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(19:18)=~10 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00080000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01001qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLZ,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=11 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  ==
          0x000C0000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0:
//
//   {D: D(22),
//    E: E(7),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), E(7), M(5), Vm(3:0)],
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    pattern: cccc11101d110100dddd101se1m0mmmm,
//    quiet_nan_exc: E(7)=1,
//    rule: VCMP_VCMPE,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {},
//    with_zero: false}
RegisterList VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0:
//
//   {D: D(22),
//    E: E(7),
//    Vd: Vd(15:12),
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sz(8)=1,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), E(7)],
//    pattern: cccc11101d110101dddd101se1000000,
//    quiet_nan_exc: E(7)=1,
//    rule: VCMP_VCMPE,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {},
//    with_zero: true}
RegisterList VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCNT_111100111d11ss00dddd01010qm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01010qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCNT,
//    safety: [size(19:18)=~00 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
RegisterList VCNT_111100111d11ss00dddd01010qm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCNT_111100111d11ss00dddd01010qm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:18)=~00 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00000000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCNT_111100111d11ss00dddd01010qm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    T: T(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: VFPv3HP,
//    cond: cond(31:28),
//    d: Vd:D,
//    defs: {},
//    fields: [cond(31:28), D(22), op(16), Vd(15:12), T(7), M(5), Vm(3:0)],
//    half_to_single: op(16)=0,
//    lowbit: 16
//         if T(7)=1
//         else 0,
//    m: Vm:M,
//    op: op(16),
//    pattern: cccc11101d11001odddd1010t1m0mmmm,
//    rule: VCVTB_VCVTT,
//    safety: [true => MAY_BE_SAFE],
//    true: true,
//    uses: {}}
RegisterList VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0:
//
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss11dddd011ppqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCVT,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    to_integer: op(1)=1,
//    unsigned: op(0)=1,
//    uses: {}}
RegisterList VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(19:18)=~10 => UNDEFINED
  if ((inst.Bits() & 0x000C0000)  !=
          0x00080000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: Vd:D
//         if to_integer
//         else D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    fields: [cond(31:28),
//      D(22),
//      opc2(18:16),
//      Vd(15:12),
//      sz(8),
//      op(7),
//      M(5),
//      Vm(3:0)],
//    m: Vm:M
//         if not to_integer
//         else M:Vm
//         if dp_operation
//         else Vm:M,
//    op: op(7),
//    opc2: opc2(18:16),
//    pattern: cccc11101d111ooodddd101sp1m0mmmm,
//    round_nearest: not to_integer,
//    round_zero: to_integer &&
//         op(7)=1,
//    rule: VCVT_VCVTR_between_floating_point_and_integer_Floating_point,
//    safety: [opc2(18:16)=~000 &&
//         opc2(18:16)=~10x => DECODER_ERROR],
//    sz: sz(8),
//    to_integer: opc2(2)=1,
//    true: true,
//    unsigned: opc2(0)=0
//         if to_integer
//         else op(7)=0,
//    uses: {}}
RegisterList VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(18:16)=~000 &&
  //       inst(18:16)=~10x => DECODER_ERROR
  if (((inst.Bits() & 0x00070000)  !=
          0x00000000) &&
       ((inst.Bits() & 0x00060000)  !=
          0x00040000))
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    double_to_single: sz(8)=1,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), M(5), Vm(3:0)],
//    m: Vm:D
//         if sz(8)=0
//         else M:Vm,
//    pattern: cccc11101d110111dddd101s11m0mmmm,
//    rule: VCVT_between_double_precision_and_single_precision,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {}}
RegisterList VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // true => MAY_BE_SAFE
  if (true)
    return MAY_BE_SAFE;

  return MAY_BE_SAFE;
}


RegisterList VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0:
//
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    frac_bits: 64 - imm6,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd111p0qm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCVT_between_floating_point_and_fixed_point,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      imm6(21:16)=0xxxxx => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    to_fixed: op(8)=1,
//    unsigned: U(24)=1,
//    uses: {}}
RegisterList VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(21:16)=000xxx => DECODER_ERROR
  if ((inst.Bits() & 0x00380000)  ==
          0x00000000)
    return DECODER_ERROR;

  // inst(21:16)=0xxxxx => UNDEFINED
  if ((inst.Bits() & 0x00200000)  ==
          0x00000000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0:
//
//   {D: D(22),
//    U: U(16),
//    Vd: Vd(12),
//    arch: VFPv3,
//    cond: cond(31:28),
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sf(8)=1,
//    fields: [cond(31:28),
//      D(22),
//      op(18),
//      U(16),
//      Vd(12),
//      sf(8),
//      sx(7),
//      i(5),
//      imm4(3:0)],
//    frac_bits: size - imm4:i,
//    i: i(5),
//    imm4: imm4(3:0),
//    op: op(18),
//    pattern: cccc11101d111o1udddd101fx1i0iiii,
//    round_nearest: not to_fixed,
//    round_zero: to_fixed,
//    rule: VCVT_between_floating_point_and_fixed_point_Floating_point,
//    safety: [frac_bits  <
//            0 => UNPREDICTABLE],
//    sf: sf(8),
//    size: 16
//         if sx(7)=0
//         else 32,
//    sx: sx(7),
//    sz: sz(8),
//    to_fixed: op(18)=1,
//    true: true,
//    unsigned: U(16)=1,
//    uses: {}}
RegisterList VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 16
  //       if inst(7)=0
  //       else 32 - inst(3:0):inst(5)  <
  //          0 => UNPREDICTABLE
  if (((static_cast<int32_t>(((inst.Bits() & 0x00000080)  ==
          0x00000000
       ? 16
       : 32)) - static_cast<int32_t>(((((inst.Bits() & 0x0000000F)) << 1) | ((inst.Bits() & 0x00000020) >> 5)))) < (0)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    advsimd: false,
//    arch: VFPv2,
//    cond: cond(31:28),
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    pattern: cccc11101d00nnnndddd101sn0m0mmmm,
//    rule: VDIV,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
RegisterList VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(31:28)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0xF0000000)  ==
          0xF0000000)
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VDUP_ARM_core_register_cccc11101bq0ddddtttt1011d0e10000_case_0:
//
//   {B: B(22),
//    D: D(7),
//    E: E(5),
//    Pc: 15,
//    Q: Q(21),
//    Rt: Rt(15:12),
//    Vd: Vd(19:16),
//    arch: AdvSIMD,
//    cond: cond(31:28),
//    cond_AL: 14,
//    d: D:Vd,
//    defs: {},
//    elements: 2
//         if B:E(1:0)=00
//         else 4
//         if B:E(1:0)=01
//         else 8
//         if B:E(1:0)=10
//         else 0,
//    esize: 32
//         if B:E(1:0)=00
//         else 16
//         if B:E(1:0)=01
//         else 8
//         if B:E(1:0)=10
//         else 0,
//    fields: [cond(31:28),
//      B(22),
//      Q(21),
//      Vd(19:16),
//      Rt(15:12),
//      D(7),
//      E(5)],
//    pattern: cccc11101bq0ddddtttt1011d0e10000,
//    regs: 1
//         if Q(21)=0
//         else 2,
//    rule: VDUP_ARM_core_register,
//    safety: [cond  !=
//            cond_AL => DEPRECATED,
//      Q(21)=1 &&
//         Vd(0)=1 => UNDEFINED,
//      B:E(1:0)=11 => UNDEFINED,
//      t  ==
//            Pc => UNPREDICTABLE],
//    sel: B:E,
//    t: Rt,
//    uses: {Rt}}
RegisterList VDUP_ARM_core_register_cccc11101bq0ddddtttt1011d0e10000_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VDUP_ARM_core_register_cccc11101bq0ddddtttt1011d0e10000_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // 14  !=
  //          inst(31:28) => DEPRECATED
  if (((((inst.Bits() & 0xF0000000) >> 28)) != (14)))
    return DEPRECATED;

  // inst(21)=1 &&
  //       inst(19:16)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x00200000)  ==
          0x00200000) &&
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  // inst(22):inst(5)(1:0)=11 => UNDEFINED
  if (((((((inst.Bits() & 0x00400000) >> 22)) << 1) | ((inst.Bits() & 0x00000020) >> 5)) & 0x00000003)  ==
          0x00000003)
    return UNDEFINED;

  // 15  ==
  //          inst(15:12) => UNPREDICTABLE
  if (((((inst.Bits() & 0x0000F000) >> 12)) == (15)))
    return UNPREDICTABLE;

  return MAY_BE_SAFE;
}


RegisterList VDUP_ARM_core_register_cccc11101bq0ddddtttt1011d0e10000_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{inst(15:12)}'
  return RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)));
}

// VDUP_scalar_111100111d11iiiidddd11000qm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if imm4(19:16)=xxx1
//         else 4
//         if imm4(19:16)=xx10
//         else 2
//         if imm4(19:16)=x100
//         else 0,
//    esize: 8
//         if imm4(19:16)=xxx1
//         else 16
//         if imm4(19:16)=xx10
//         else 32
//         if imm4(19:16)=x100
//         else 0,
//    fields: [D(22), imm4(19:16), Vd(15:12), Q(6), M(5), Vm(3:0)],
//    imm4: imm4(19:16),
//    index: imm4(3:1)
//         if imm4(19:16)=xxx1
//         else imm4(3:2)
//         if imm4(19:16)=xx10
//         else imm4(3)
//         if imm4(19:16)=x100
//         else 0,
//    m: M:Vm,
//    pattern: 111100111d11iiiidddd11000qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VDUP_scalar,
//    safety: [imm4(19:16)=x000 => UNDEFINED,
//      Q(6)=1 &&
//         Vd(0)=1 => UNDEFINED],
//    uses: {}}
RegisterList VDUP_scalar_111100111d11iiiidddd11000qm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VDUP_scalar_111100111d11iiiidddd11000qm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(19:16)=x000 => UNDEFINED
  if ((inst.Bits() & 0x00070000)  ==
          0x00000000)
    return UNDEFINED;

  // inst(6)=1 &&
  //       inst(15:12)(0)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VDUP_scalar_111100111d11iiiidddd11000qm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    d: D:Vd,
//    defs: {},
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
//    pattern: 111100110d00nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VEOR,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
RegisterList VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VEXT_111100101d11nnnnddddiiiinqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    d: D:Vd,
//    defs: {},
//    fields: [D(22),
//      Vn(19:16),
//      Vd(15:12),
//      imm4(11:8),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    imm4: imm4(11:8),
//    m: M:Vm,
//    n: N:Vn,
//    pattern: 111100101d11nnnnddddiiiinqm0mmmm,
//    position: 8 * imm4,
//    quadword_operation: Q(6)=1,
//    rule: VEXT,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      Q(6)=0 &&
//         imm4(3)=1 => UNDEFINED],
//    uses: {}}
RegisterList VEXT_111100101d11nnnnddddiiiinqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VEXT_111100101d11nnnnddddiiiinqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(6)=0 &&
  //       inst(11:8)(3)=1 => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000000) &&
       ((((inst.Bits() & 0x00000F00) >> 8) & 0x00000008)  ==
          0x00000008))
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VEXT_111100101d11nnnnddddiiiinqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMDv2,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
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
//    op1_neg: size(1),
//    pattern: 111100100d00nnnndddd1100nqm1mmmm,
//    rule: VFMA_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
RegisterList VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x00300000) >> 20) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    advsimd: false,
//    arch: VFPv4,
//    cond: cond(31:28),
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      op(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    op: op(6),
//    op1_neg: op(6)=1,
//    pattern: cccc11101d10nnnndddd101snom0mmmm,
//    rule: VFMA_VFMS,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
RegisterList VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(31:28)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0xF0000000)  ==
          0xF0000000)
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMDv2,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
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
//    op1_neg: size(1),
//    pattern: 111100100d10nnnndddd1100nqm1mmmm,
//    rule: VFMS_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
RegisterList VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)(0)=1 => UNDEFINED
  if ((((inst.Bits() & 0x00300000) >> 20) & 0x00000001)  ==
          0x00000001)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: VFPv4,
//    cond: cond(31:28),
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      op(6),
//      M(5),
//      Vm(3:0)],
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    op: op(6),
//    op1_neg: op(6)=1,
//    pattern: cccc11101d01nnnndddd101snom0mmmm,
//    rule: VFNMA_VFNMS,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
RegisterList VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(31:28)=1111 => DECODER_ERROR
  if ((inst.Bits() & 0xF0000000)  ==
          0xF0000000)
    return DECODER_ERROR;

  return MAY_BE_SAFE;
}


RegisterList VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
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
RegisterList VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)=11 => UNDEFINED
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VHADD_1111001u0dssnnnndddd0000nqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

// VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0:
//
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
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
RegisterList VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0::
defs(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // defs: '{}'
  return RegisterList();
}

SafetyLevel VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0::
safety(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.

  // inst(6)=1 &&
  //       (inst(15:12)(0)=1 ||
  //       inst(19:16)(0)=1 ||
  //       inst(3:0)(0)=1) => UNDEFINED
  if (((inst.Bits() & 0x00000040)  ==
          0x00000040) &&
       ((((((inst.Bits() & 0x0000F000) >> 12) & 0x00000001)  ==
          0x00000001) ||
       ((((inst.Bits() & 0x000F0000) >> 16) & 0x00000001)  ==
          0x00000001) ||
       (((inst.Bits() & 0x0000000F) & 0x00000001)  ==
          0x00000001))))
    return UNDEFINED;

  // inst(21:20)=11 => UNDEFINED
  if ((inst.Bits() & 0x00300000)  ==
          0x00300000)
    return UNDEFINED;

  return MAY_BE_SAFE;
}


RegisterList VHSUB_1111001u0dssnnnndddd0010nqm0mmmm_case_0::
uses(Instruction inst) const {
  UNREFERENCED_PARAMETER(inst);  // To silence compiler.
  // uses: '{}'
  return RegisterList();
}

}  // namespace nacl_arm_dec
