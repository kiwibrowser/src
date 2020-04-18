/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Receives no arguments, just clears registers.
 */
        .text
        .p2align 4
        .set noreorder
        .globl infoleak_clear_state
infoleak_clear_state:
        mtc1   $zero, $f0
        mtc1   $zero, $f1
        mov.d  $f2,   $f0
        mov.d  $f4,   $f0
        mov.d  $f6,   $f0
        mov.d  $f8,   $f0
        mov.d  $f10,  $f0
        mov.d  $f12,  $f0
        mov.d  $f14,  $f0
        mov.d  $f16,  $f0
        mov.d  $f18,  $f0
        mov.d  $f20,  $f0
        mov.d  $f22,  $f0
        mov.d  $f24,  $f0
        mov.d  $f26,  $f0
        mov.d  $f28,  $f0
        mov.d  $f30,  $f0
        jr     $ra
        nop
        .set reorder

/*
 * Receives:
 * a0: pointer to float_regs structure
 */

        .p2align 4
        .set noreorder
        .globl infoleak_fetch_regs
infoleak_fetch_regs:
        sdc1  $f0,  0($a0)
        sdc1  $f2,  8($a0)
        sdc1  $f4,  16($a0)
        sdc1  $f6,  24($a0)
        sdc1  $f8,  32($a0)
        sdc1  $f10, 40($a0)
        sdc1  $f12, 48($a0)
        sdc1  $f14, 56($a0)
        sdc1  $f16, 64($a0)
        sdc1  $f18, 72($a0)
        sdc1  $f20, 80($a0)
        sdc1  $f22, 88($a0)
        sdc1  $f24, 96($a0)
        sdc1  $f26, 104($a0)
        sdc1  $f28, 112($a0)
        sdc1  $f30, 120($a0)
        jr    $ra
        nop

        .set reorder
