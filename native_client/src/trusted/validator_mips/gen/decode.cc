/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#include <stdio.h>
#include "native_client/src/trusted/validator_mips/decode.h"

namespace nacl_mips_dec {

/*
 * This beast holds a bunch of pre-created ClassDecoder instances, which
 * we create in init_decode().  Because ClassDecoders are stateless, we
 * can freely reuse them -- even across threads -- and avoid allocating
 * in the inner decoder loop.
 */
struct DecoderState {
  const Load _Load_instance;
  const JalImm _JalImm_instance;
  const Branch _Branch_instance;
  const Arithm2 _Arithm2_instance;
  const Arithm3 _Arithm3_instance;
  const Forbidden _Forbidden_instance;
  const Safe _Safe_instance;
  const BranchAndLink _BranchAndLink_instance;
  const JalReg _JalReg_instance;
  const NaClHalt _NaClHalt_instance;
  const LoadWord _LoadWord_instance;
  const StoreConditional _StoreConditional_instance;
  const FPLoadStore _FPLoadStore_instance;
  const JmpImm _JmpImm_instance;
  const JmpReg _JmpReg_instance;
  const Store _Store_instance;
  const ExtIns _ExtIns_instance;
  DecoderState() :
  _Load_instance()
  , _JalImm_instance()
  , _Branch_instance()
  , _Arithm2_instance()
  , _Arithm3_instance()
  , _Forbidden_instance()
  , _Safe_instance()
  , _BranchAndLink_instance()
  , _JalReg_instance()
  , _NaClHalt_instance()
  , _LoadWord_instance()
  , _StoreConditional_instance()
  , _FPLoadStore_instance()
  , _JmpImm_instance()
  , _JmpReg_instance()
  , _Store_instance()
  , _ExtIns_instance()
  {}
};

/*
 * Prototypes for static table-matching functions.
 */
static inline const ClassDecoder
  &decode_MIPS32(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_special(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_regimm(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_special2(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_special3(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_movci(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_srl(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_srlv(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_bshfl(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_cop0(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_c0(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_cop1(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_c1(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_movcf(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_cop2(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_cop1x(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_branch_1(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_arithm2_1(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_arithm3_1(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_arithm3_2(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_jr(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_jalr(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_sync(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_mfhi(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_mthi(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_mult(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_arithm3_3(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_mfmc0(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_mfc1(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_mtc1(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_bc1(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_fp(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_bc2(const Instruction insn, const DecoderState *state);
static inline const ClassDecoder
  &decode_c_cond_fmt(const Instruction insn, const DecoderState *state);

/*
 * Table-matching function implementations.
 */

/*
 * Implementation of table MIPS32.
 * Specified by: See Table A.2.
 */
static inline const ClassDecoder
&decode_MIPS32(const Instruction insn, const DecoderState *state) {
  if (((insn & 0xFC000000) == 0x00000000)) {
    return decode_special(insn, state);
  }

  if (((insn & 0xFC000000) == 0x04000000)) {
    return decode_regimm(insn, state);
  }

  if (((insn & 0xFC000000) == 0x08000000)) {
    return state->_JmpImm_instance;
  }

  if (((insn & 0xFC000000) == 0x0C000000)) {
    return state->_JalImm_instance;
  }

  if (((insn & 0xFC000000) == 0x38000000)) {
    return state->_Arithm2_instance;
  }

  if (((insn & 0xFC000000) == 0x3C000000)) {
    return decode_arithm2_1(insn, state);
  }

  if (((insn & 0xFC000000) == 0x40000000)) {
    return decode_cop0(insn, state);
  }

  if (((insn & 0xFC000000) == 0x44000000)) {
    return decode_cop1(insn, state);
  }

  if (((insn & 0xFC000000) == 0x48000000)) {
    return decode_cop2(insn, state);
  }

  if (((insn & 0xFC000000) == 0x4C000000)) {
    return decode_cop1x(insn, state);
  }

  if (((insn & 0xFC000000) == 0x70000000)) {
    return decode_special2(insn, state);
  }

  if (((insn & 0xFC000000) == 0x74000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0xFC000000) == 0x78000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0xFC000000) == 0x7C000000)) {
    return decode_special3(insn, state);
  }

  if (((insn & 0xFC000000) == 0x8C000000)) {
    return state->_LoadWord_instance;
  }

  if (((insn & 0xFC000000) == 0xB8000000)) {
    return state->_Store_instance;
  }

  if (((insn & 0xFC000000) == 0xC0000000)) {
    return state->_Load_instance;
  }

  if (((insn & 0xFC000000) == 0xE0000000)) {
    return state->_StoreConditional_instance;
  }

  if (((insn & 0xDC000000) == 0x9C000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0xDC000000) == 0xD0000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0xF8000000) == 0x30000000)) {
    return state->_Arithm2_instance;
  }

  if (((insn & 0xEC000000) == 0x88000000)) {
    return state->_Load_instance;
  }

  if (((insn & 0xF8000000) == 0xB0000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0xB8000000) == 0x10000000)) {
    return state->_Branch_instance;
  }

  if (((insn & 0xB8000000) == 0x18000000)) {
    return decode_branch_1(insn, state);
  }

  if (((insn & 0xCC000000) == 0xC4000000)) {
    return state->_FPLoadStore_instance;
  }

  if (((insn & 0xCC000000) == 0xC8000000)) {
    return state->_FPLoadStore_instance;
  }

  if (((insn & 0xCC000000) == 0xCC000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0xF0000000) == 0x20000000)) {
    return state->_Arithm2_instance;
  }

  if (((insn & 0xF0000000) == 0x60000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0xE8000000) == 0x80000000)) {
    return state->_Load_instance;
  }

  if (((insn & 0xF0000000) == 0xA0000000)) {
    return state->_Store_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "MIPS32 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table special.
 * Specified by: See Table A.3.
 */
static inline const ClassDecoder
&decode_special(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x0000003F) == 0x00000000)) {
    return decode_arithm3_1(insn, state);
  }

  if (((insn & 0x0000003F) == 0x00000001)) {
    return decode_movci(insn, state);
  }

  if (((insn & 0x0000003F) == 0x00000002)) {
    return decode_srl(insn, state);
  }

  if (((insn & 0x0000003F) == 0x00000003)) {
    return decode_arithm3_1(insn, state);
  }

  if (((insn & 0x0000003F) == 0x00000004)) {
    return decode_arithm3_2(insn, state);
  }

  if (((insn & 0x0000003F) == 0x00000006)) {
    return decode_srlv(insn, state);
  }

  if (((insn & 0x0000003F) == 0x00000007)) {
    return decode_arithm3_2(insn, state);
  }

  if (((insn & 0x0000003F) == 0x00000008)) {
    return decode_jr(insn, state);
  }

  if (((insn & 0x0000003F) == 0x00000009)) {
    return decode_jalr(insn, state);
  }

  if (((insn & 0x0000003F) == 0x0000000D)) {
    return state->_NaClHalt_instance;
  }

  if (((insn & 0x0000003F) == 0x0000000F)) {
    return decode_sync(insn, state);
  }

  if (((insn & 0x0000003F) == 0x0000003F)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000001F) == 0x0000001E)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000002F) == 0x00000005)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000037) == 0x00000017)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003D) == 0x0000000C)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003D) == 0x00000010)) {
    return decode_mfhi(insn, state);
  }

  if (((insn & 0x0000003D) == 0x00000011)) {
    return decode_mthi(insn, state);
  }

  if (((insn & 0x0000003D) == 0x00000014)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003D) == 0x00000034)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x0000003D) == 0x00000035)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003E) == 0x0000000A)) {
    return decode_arithm3_2(insn, state);
  }

  if (((insn & 0x0000003E) == 0x0000002A)) {
    return decode_arithm3_3(insn, state);
  }

  if (((insn & 0x0000003E) == 0x0000002E)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000001E) == 0x0000001C)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003A) == 0x00000028)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003C) == 0x00000018)) {
    return decode_mult(insn, state);
  }

  if (((insn & 0x0000003C) == 0x00000030)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x0000003C) == 0x00000038)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000038) == 0x00000020)) {
    return decode_arithm3_3(insn, state);
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "special could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table regimm.
 * Specified by: See Table A.4.
 */
static inline const ClassDecoder
&decode_regimm(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x001D0000) == 0x000C0000)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x001D0000) == 0x000D0000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x001C0000) == 0x00000000)) {
    return state->_Branch_instance;
  }

  if (((insn & 0x001C0000) == 0x00080000)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x001C0000) == 0x00100000)) {
    return state->_BranchAndLink_instance;
  }

  if (((insn & 0x000C0000) == 0x00040000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00180000) == 0x00180000)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "regimm could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table special2.
 * Specified by: See Table A.5.
 */
static inline const ClassDecoder
&decode_special2(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x0000003F) == 0x00000002)) {
    return decode_arithm3_2(insn, state);
  }

  if (((insn & 0x0000003F) == 0x0000003F)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000001F) == 0x0000001E)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000002F) == 0x0000000F)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000037) == 0x00000006)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003B) == 0x00000003)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003E) == 0x00000020)) {
    return decode_arithm3_2(insn, state);
  }

  if (((insn & 0x0000003E) == 0x0000002E)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003E) == 0x0000003C)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000002E) == 0x0000000C)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000036) == 0x00000024)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003A) == 0x00000000)) {
    return decode_mult(insn, state);
  }

  if (((insn & 0x0000003A) == 0x00000022)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000000C) == 0x00000008)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000018) == 0x00000010)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "special2 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table special3.
 * Specified by: See Table A.6.
 */
static inline const ClassDecoder
&decode_special3(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x0000003F) == 0x00000020)) {
    return decode_bshfl(insn, state);
  }

  if (((insn & 0x0000003F) == 0x0000003F)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000002F) == 0x0000002E)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000037) == 0x00000027)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003B) == 0x00000000)) {
    return state->_ExtIns_instance;
  }

  if (((insn & 0x0000003D) == 0x00000021)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003E) == 0x0000003C)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000001B) == 0x00000002)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000036) == 0x00000024)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000039) == 0x00000001)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000002C) == 0x00000028)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000018) == 0x00000010)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000028) == 0x00000008)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "special3 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table movci.
 * Specified by: See Table A.7.
 */
static inline const ClassDecoder
&decode_movci(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x00020000) == 0x00000000)
      && ((insn & 0x000007C0) == 0x00000000)) {
    return state->_Arithm3_instance;
  }

  if (((insn & 0x00020000) == 0x00020000)
      && ((insn & 0x000007C0) == 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if ((true)
      && ((insn & 0x000007C0) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "movci could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table srl.
 * Specified by: See Table A.8.
 */
static inline const ClassDecoder
&decode_srl(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x03C00000) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03C00000) == 0x00000000)) {
    return state->_Arithm3_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "srl could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table srlv.
 * Specified by: See Table A.9.
 */
static inline const ClassDecoder
&decode_srlv(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x00000780) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000780) == 0x00000000)) {
    return state->_Arithm3_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "srlv could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table bshfl.
 * Specified by: See Table A.10.
 */
static inline const ClassDecoder
&decode_bshfl(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x000007C0) == 0x00000080)) {
    return decode_arithm3_1(insn, state);
  }

  if (((insn & 0x000007C0) == 0x000005C0)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x000003C0) == 0x000003C0)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x000005C0) == 0x00000180)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x000005C0) == 0x00000400)) {
    return decode_arithm3_1(insn, state);
  }

  if (((insn & 0x000006C0) == 0x000000C0)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000780) == 0x00000500)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000380) == 0x00000300)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x000004C0) == 0x00000480)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000540) == 0x00000440)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000680) == 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000700) == 0x00000200)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "bshfl could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table cop0.
 * Specified by: See Table A.11.
 */
static inline const ClassDecoder
&decode_cop0(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x03E00000) == 0x01600000)) {
    return decode_mfmc0(insn, state);
  }

  if (((insn & 0x03E00000) == 0x02000000)) {
    return decode_c0(insn, state);
  }

  if (((insn & 0x03E00000) == 0x02E00000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03E00000) == 0x03C00000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x01E00000) == 0x01E00000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03600000) == 0x01400000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03600000) == 0x02400000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03A00000) == 0x02200000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x02C00000) == 0x02800000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03400000) == 0x01000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03800000) == 0x03000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03000000) == 0x00000000)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "cop0 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table c0.
 * Specified by: See Table A.12.
 */
static inline const ClassDecoder
&decode_c0(const Instruction insn, const DecoderState *state) {
  if ((true)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "c0 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table cop1.
 * Specified by: See Table A.13.
 */
static inline const ClassDecoder
&decode_cop1(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x03E00000) == 0x00000000)) {
    return decode_mfc1(insn, state);
  }

  if (((insn & 0x03E00000) == 0x00600000)) {
    return decode_mfc1(insn, state);
  }

  if (((insn & 0x03E00000) == 0x00800000)) {
    return decode_mtc1(insn, state);
  }

  if (((insn & 0x03E00000) == 0x00E00000)) {
    return decode_mtc1(insn, state);
  }

  if (((insn & 0x03E00000) == 0x01000000)) {
    return decode_bc1(insn, state);
  }

  if (((insn & 0x03E00000) == 0x01E00000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03600000) == 0x00200000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03A00000) == 0x01200000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03C00000) == 0x01800000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03C00000) == 0x02400000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03C00000) == 0x03800000)) {
    return decode_c1(insn, state);
  }

  if (((insn & 0x02600000) == 0x00400000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x02C00000) == 0x02C00000)) {
    return decode_c1(insn, state);
  }

  if (((insn & 0x03400000) == 0x02000000)) {
    return decode_c1(insn, state);
  }

  if (((insn & 0x03800000) == 0x03000000)) {
    return decode_c1(insn, state);
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "cop1 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table c1.
 * Specified by: See Table A.14, A.15, A.16, A.17.
 */
static inline const ClassDecoder
&decode_c1(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x0000003F) == 0x00000011)) {
    return decode_movcf(insn, state);
  }

  if (((insn & 0x0000003F) == 0x00000015)) {
    return decode_fp(insn, state);
  }

  if (((insn & 0x0000003F) == 0x00000016)) {
    return decode_fp(insn, state);
  }

  if (((insn & 0x0000003F) == 0x0000001E)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003F) == 0x00000027)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003F) == 0x00000028)) {
    return decode_fp(insn, state);
  }

  if (((insn & 0x0000003F) == 0x0000002A)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003F) == 0x0000002F)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x00000037) == 0x00000017)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000037) == 0x00000026)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x0000003B) == 0x00000010)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003D) == 0x00000029)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003E) == 0x00000012)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x0000003E) == 0x0000001C)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003E) == 0x00000022)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003E) == 0x0000002C)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x0000003A) == 0x00000020)) {
    return decode_fp(insn, state);
  }

  if (((insn & 0x0000003C) == 0x00000000)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x0000003C) == 0x00000008)) {
    return decode_fp(insn, state);
  }

  if (((insn & 0x0000003C) == 0x00000018)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000034) == 0x00000004)) {
    return decode_fp(insn, state);
  }

  if (((insn & 0x00000030) == 0x00000030)) {
    return decode_c_cond_fmt(insn, state);
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "c1 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table movcf.
 * Specified by: See Table A.18.
 */
static inline const ClassDecoder
&decode_movcf(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x00020000) == 0x00000000)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x00020000) == 0x00020000)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "movcf could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table cop2.
 * Specified by: See Table A.19.
 */
static inline const ClassDecoder
&decode_cop2(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x03E00000) == 0x01000000)) {
    return decode_bc2(insn, state);
  }

  if (((insn & 0x03E00000) == 0x03C00000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x01E00000) == 0x01E00000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03600000) == 0x01400000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03A00000) == 0x01200000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x01C00000) == 0x01800000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03800000) == 0x03000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x01000000) == 0x00000000)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "cop2 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table cop1x.
 * Specified by: See Table A.20.
 */
static inline const ClassDecoder
&decode_cop1x(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x0000003F) == 0x00000036)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x0000003F) == 0x00000037)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000001F) == 0x0000001E)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x0000001F) == 0x0000001F)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000037) == 0x00000026)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x00000037) == 0x00000027)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003E) == 0x00000034)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000001E) == 0x0000001C)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000036) == 0x00000024)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000003C) == 0x00000018)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000026) == 0x00000020)) {
    return state->_Safe_instance;
  }

  if (((insn & 0x00000026) == 0x00000022)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000038) == 0x00000010)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x00000030) == 0x00000000)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "cop1x could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table branch_1.
 * Specified by: blez, bgtz, blezl, bgtzl.
 */
static inline const ClassDecoder
&decode_branch_1(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x001F0000) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x001F0000) == 0x00000000)) {
    return state->_Branch_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "branch_1 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table arithm2_1.
 * Specified by: lui.
 */
static inline const ClassDecoder
&decode_arithm2_1(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x03E00000) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03E00000) == 0x00000000)) {
    return state->_Arithm2_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "arithm2_1 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table arithm3_1.
 * Specified by: sll, sra, wsbh, seb, seh.
 */
static inline const ClassDecoder
&decode_arithm3_1(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x03E00000) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03E00000) == 0x00000000)) {
    return state->_Arithm3_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "arithm3_1 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table arithm3_2.
 * Specified by: sllv, srav, movz, movn, mul, clz, clo.
 */
static inline const ClassDecoder
&decode_arithm3_2(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x000007C0) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x000007C0) == 0x00000000)) {
    return state->_Arithm3_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "arithm3_2 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table jr.
 * Specified by: jr.
 */
static inline const ClassDecoder
&decode_jr(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x001FFFC0) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x001FFFC0) == 0x00000000)) {
    return state->_JmpReg_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "jr could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table jalr.
 * Specified by: jalr.
 */
static inline const ClassDecoder
&decode_jalr(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x001F0000) != 0x00000000)
      && ((insn & 0x000007C0) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x001F0000) == 0x00000000)
      && ((insn & 0x000007C0) == 0x00000000)) {
    return state->_JalReg_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "jalr could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table sync.
 * Specified by: sync.
 */
static inline const ClassDecoder
&decode_sync(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x03FFFFC0) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03FFFFC0) == 0x00000000)) {
    return state->_Safe_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "sync could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table mfhi.
 * Specified by: mfhi, mflo.
 */
static inline const ClassDecoder
&decode_mfhi(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x03FF0000) != 0x00000000)
      && ((insn & 0x000007C0) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x03FF0000) == 0x00000000)
      && ((insn & 0x000007C0) == 0x00000000)) {
    return state->_Arithm3_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "mfhi could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table mthi.
 * Specified by: mthi, mtlo.
 */
static inline const ClassDecoder
&decode_mthi(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x001FFFC0) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x001FFFC0) == 0x00000000)) {
    return state->_Safe_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "mthi could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table mult.
 * Specified by: mult, multu, div, divu, madd, maddu, msub, msubu.
 */
static inline const ClassDecoder
&decode_mult(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x0000FFC0) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x0000FFC0) == 0x00000000)) {
    return state->_Safe_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "mult could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table arithm3_3.
 * Specified by: add, addu, sub, subu, and, or, xor, nor, slt, sltu.
 */
static inline const ClassDecoder
&decode_arithm3_3(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x000007C0) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x000007C0) == 0x00000000)) {
    return state->_Arithm3_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "arithm3_3 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table mfmc0.
 * Specified by: di, ei.
 */
static inline const ClassDecoder
&decode_mfmc0(const Instruction insn, const DecoderState *state) {
  if ((true)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "mfmc0 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table mfc1.
 * Specified by: mfc1, mfhc1.
 */
static inline const ClassDecoder
&decode_mfc1(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x000007FF) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x000007FF) == 0x00000000)) {
    return state->_Arithm2_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "mfc1 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table mtc1.
 * Specified by: mtc1, mthc1.
 */
static inline const ClassDecoder
&decode_mtc1(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x000007FF) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x000007FF) == 0x00000000)) {
    return state->_Safe_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "mtc1 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table bc1.
 * Specified by: bc1f, bc1t, bc1fl, bc1tl.
 */
static inline const ClassDecoder
&decode_bc1(const Instruction insn, const DecoderState *state) {
  if ((true)) {
    return state->_Branch_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "bc1 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table fp.
 * Specified by: sqrt.fmt, abs.fmt, mov.fmt, neg.fmt, round.l.fmt, trunc.l.fmt, ceil.l.fmt, floor.l.fmt, round.w.fmt, trunc.w.fmt, ceil.w.fmt, floor.w.fmt, recip.fmt, rsqrt.fmt, cvt.s.fmt, cvt.d.fmt, cvt.w.fmt, cvt.l.fmt, cvt.s.pl.
 */
static inline const ClassDecoder
&decode_fp(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x001F0000) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x001F0000) == 0x00000000)) {
    return state->_Safe_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "fp could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table bc2.
 * Specified by: bc2f, bc2t, bc2fl, bc2tl.
 */
static inline const ClassDecoder
&decode_bc2(const Instruction insn, const DecoderState *state) {
  if ((true)) {
    return state->_Forbidden_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "bc2 could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

/*
 * Implementation of table c_cond_fmt.
 * Specified by: c.cond.fmt.
 */
static inline const ClassDecoder
&decode_c_cond_fmt(const Instruction insn, const DecoderState *state) {
  if (((insn & 0x000000C0) != 0x00000000)) {
    return state->_Forbidden_instance;
  }

  if (((insn & 0x000000C0) == 0x00000000)) {
    return state->_Safe_instance;
  }

  // Catch any attempt to fall through...
  fprintf(stderr, "TABLE IS INCOMPLETE: "
          "c_cond_fmt could not parse %08X", insn.Bits(31, 0));
  return state->_Forbidden_instance;
}

const DecoderState *init_decode() {
  return new DecoderState;
}
void delete_state(const DecoderState *state) {
  delete state;
}

const ClassDecoder &decode(const Instruction insn, const DecoderState *state) {
  return decode_MIPS32(insn, state);
}

}  // namespace nacl_mips_dec
