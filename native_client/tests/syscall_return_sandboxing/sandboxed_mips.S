/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Assume 32-byte trampoline slots, 16-byte bundles.
 */
#define GETTIMEOFDAY_ADDR 0x10500
#define BREAK_INST .word 0xd

        .data
timebuf:
        .space 0x200 /* enough for a timeval */
        .text
        .globl SyscallReturnIsSandboxed
SyscallReturnIsSandboxed:
        .set noreorder

        addiu $sp, $sp, -24
        sw    $ra, 20($sp)
        lui   $a0, %hi(timebuf)
        addiu $a0, $a0, %lo(timebuf)
        addu  $a1, $zero, $zero
        lui   $ra, %hi(bad)
        addiu $ra, $ra, %lo(bad)
        lui   $a2, %hi(GETTIMEOFDAY_ADDR)
        addiu $a2, $a2, %lo(GETTIMEOFDAY_ADDR)
        jr    $a2
        nop
        .p2align 4
        /* Expected return address */
        b done
        nop
        /*
         * We don't expect the syscall to return to the following unaligned
         * address or to align up to the bundle boundary after it.
         */
bad:
        BREAK_INST  /* Abort */
        BREAK_INST
        BREAK_INST
        BREAK_INST
        BREAK_INST
        BREAK_INST
done:
        ori   $v0, $zero, 1 /* Indicate success */
        lw    $ra, 20($sp)
        addiu $sp, $sp, 24
        jr    $ra
        nop

