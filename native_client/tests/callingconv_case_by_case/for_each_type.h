/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Repeat some macros for all types (that make sense) in "useful_structs.h".
 * This doesn't have an include guard since it can be included more than once.
 */

#if !defined(DO_FOR_TYPE)
#error "MUST define DO_FOR_TYPE when including for_each_type.inc!"
#endif

/* Skip EMPTY struct for now -- may want to just test that with inline code
 * since there are no real values to check. The only thing to check is that
 * it didn't take up a register or stack slot (or if our calling convention
 * is dumb and uses a slot, check that it consistently takes up a slot).
 */

DO_FOR_TYPE(CHAR_I32)
DO_FOR_TYPE(I32_I32)
DO_FOR_TYPE(CHAR_I64)
DO_FOR_TYPE(I64_I64)

DO_FOR_TYPE(I64_STRUCT)
DO_FOR_TYPE(I64_NON_STRUCT)
DO_FOR_TYPE(I32_STRUCT)
DO_FOR_TYPE(I32_NON_STRUCT)
DO_FOR_TYPE(I16_STRUCT)
DO_FOR_TYPE(I16_NON_STRUCT)
DO_FOR_TYPE(DOUBLE_STRUCT)
DO_FOR_TYPE(DOUBLE_NON_STRUCT)
DO_FOR_TYPE(FLOAT_STRUCT)
DO_FOR_TYPE(FLOAT_NON_STRUCT)

DO_FOR_TYPE(I32_FLOAT)
DO_FOR_TYPE(FLOAT_FLOAT)
DO_FOR_TYPE(STRUCT_STRUCT)
DO_FOR_TYPE(PTR_EMPTYSTRUCT_PTR)
DO_FOR_TYPE(DOUBLE_DOUBLE)
DO_FOR_TYPE(CHAR_BOOL_I32_BOOL)
/*
 * pnacl-clang does not align struct arguments correctly when the
 * struct type is declared with __attribute__((aligned)).  This causes
 * this test to fault on an unaligned "movaps" instruction on x86.
 * See https://code.google.com/p/nativeclient/issues/detail?id=3403
 */
/* DO_FOR_TYPE(I32_ALIGN16) */
DO_FOR_TYPE(I32_CHAR12)
DO_FOR_TYPE(ARRAY_I32_4)
DO_FOR_TYPE(ARRAY_FLOAT_4)
DO_FOR_TYPE(DOUBLE_DOUBLE_DOUBLE)
DO_FOR_TYPE(CHAR_I64_I32)

DO_FOR_TYPE(BITFIELD_STRADDLE)
DO_FOR_TYPE(NONBITFIELD_STRADDLE)

/*
 * pnacl-clang does not align __attribute__((aligned)) structs
 * correctly.  See above.
 */
/* DO_FOR_TYPE(I32_CHAR_ALIGN32) */

DO_FOR_TYPE(U_I64_DOUBLE)
DO_FOR_TYPE(U_DOUBLE_I64)
DO_FOR_TYPE(U_DOUBLE_ARRAY_I16_4)
DO_FOR_TYPE(U_ARRAY_I16_4_DOUBLE)
DO_FOR_TYPE(U_STRADDLE_BF_NONBF)
DO_FOR_TYPE(U_I16_STRUCT)


DO_FOR_TYPE(ENUM1)
DO_FOR_TYPE(ENUM1_PACKED8)
DO_FOR_TYPE(ENUM1_PACKED16)
DO_FOR_TYPE(ENUM1_PACKED24)
DO_FOR_TYPE(ENUM1_PACKED32)

DO_FOR_TYPE(CLASS_I32_I32)
DO_FOR_TYPE(CLASS_DOUBLE_DOUBLE)
DO_FOR_TYPE(U_CLASS_INT_CLASS)

DO_FOR_TYPE(NONTRIV_CLASS_I32_I32)
DO_FOR_TYPE(NONTRIV_CLASS_DOUBLE_DOUBLE)

DO_FOR_TYPE(MEMBER_PTRS)
/*
 * NOTE: C++ method pointers are represented differently on x86 and ARM.
 * PNaCl has changed to use the ARM representation, but only for le32.
 * So, this should still work for native nacl-clang, but beware
 * if testing le32 against anything else.
 * See https://code.google.com/p/nativeclient/issues/detail?id=3450
 */
DO_FOR_TYPE(MEMBER_FUN_PTRS)
DO_FOR_TYPE(I32_MEMBER_FUN_PTR)
DO_FOR_TYPE(I32_MEMBER_FUN_PTR_I32)

/*
 * Vector arguments currently have different name mangling (gcc < 4.5 bug).
 * http://code.google.com/p/nativeclient/issues/detail?id=2399
 *
 * pnacl: error: undefined reference to 'mod2a___m128(float __vector(4))'
 * _Z12mod1b___m128Dv4_f (clang default)
 * vs
 * nnacl: error: undefined reference to 'mod1b___m128(float __vector)'
 * _Z12mod1b___m128U8__vectorf (gcc default)
 *
 * If these were exported as C, it would be okay. Set extern "C" for now.
 * Be careful not to do extern "C" in contexts which are not declarations
 * or definitions.
 */
#if !defined(NOT_DECLARING_DEFINING) && defined(__cplusplus)
extern "C" {
#endif

/*
 * TODO(mcgrathr): __m128 is not really supported by arm-nacl-gcc yet.
 */
#if !defined(__arm__)
DO_FOR_TYPE(__m128)
#endif

#if !defined(NOT_DECLARING_DEFINING) && defined(__cplusplus)
}
#endif
