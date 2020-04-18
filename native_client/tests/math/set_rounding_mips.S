/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Rounding mode control field is bits [1:0] in FCSR.
 * This function takes the two-bit value and sets bits [1:0].
 *
 * The bit patterns for MIPS are:
 * 0b00 Round to Nearest (RN) mode
 * 0b10 Round towards Plus Infinity (RP) mode
 * 0b11 Round towards Minus Infinity (RM) mode
 * 0b01 Round towards Zero (RZ) mode
 */
        .text
        .p2align 4
set_rounding_control_field:
        /* LLVM's MipsAsmParser can't parse cfc1 $v0, $31,
         * So try to use .word.  Using .word we can encode
         * the instruction, but the validator rejects this
         * instruction. Eventually we may make a syscall to enable
         * setting the control word, so that the untrusted code
         * won't be responsible for this.
         */
#if defined(__clang__)
#        .word 0x4442f800
#else
        cfc1 $v0, $31 /* fcsr */
#endif
        li $t1, 0xfffffffc
        and $v0, $v0, $t1
        or $v0, $v0, $a0
#if defined(__clang__)
#        .word 0x44c2f800
#else
        ctc1 $v0, $31 /* fcsr */
#endif
        nop /* Force return sequence to next bundle. */

        jr $ra
        nop

        .p2align 4
        .globl set_round_toward_nearest
set_round_toward_nearest:
        li $a0, 0
        j set_rounding_control_field

        .p2align 4
        .globl set_round_toward_plus_infinity
set_round_toward_plus_infinity:
        li $a0, 2
        j set_rounding_control_field

        .p2align 4
        .globl set_round_toward_minus_infinity
set_round_toward_minus_infinity:
        li $a0, 3
        j set_rounding_control_field

        .p2align 4
        .globl set_round_toward_zero
set_round_toward_zero:
        li $a0, 1
        j set_rounding_control_field
