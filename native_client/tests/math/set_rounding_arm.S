/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
        .syntax unified
        .arm
        .fpu vfp

/*
 * Rounding mode control field is bits [23:22] in FPSCR.
 * This macro takes the pre-shifted two-bit value and sets bits [23:22].
 *
 * The bit patterns for ARM are:
 * 0b00 Round to Nearest (RN) mode
 * 0b01 Round towards Plus Infinity (RP) mode
 * 0b10 Round towards Minus Infinity (RM) mode
 * 0b11 Round towards Zero (RZ) mode
 */
.macro set_rounding_control_field value
  vmrs r1, fpscr
  bic r1, r1, #0x00c00000
  orr r1, r1, \value
  vmsr fpscr, r1

  bic lr, lr, #0xc000000f
  bx lr
.endm

        .p2align 4
        .global set_round_toward_nearest
set_round_toward_nearest:
        set_rounding_control_field #0

        .p2align 4
        .global set_round_toward_plus_infinity
set_round_toward_plus_infinity:
        set_rounding_control_field #0x400000

        .p2align 4
        .global set_round_toward_minus_infinity
set_round_toward_minus_infinity:
        set_rounding_control_field #0x800000

        .p2align 4
        .global set_round_toward_zero
set_round_toward_zero:
        set_rounding_control_field #0xc00000
