/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

        .syntax unified
        .arm
/*
 * TODO(mcgrathr): The LLVM assembler doesn't grok '.fpu vfp'
 * and then it complains about using VFP2 instructions.
 * This is fixed by passing -mattr=+vfp2 on the llvm-mc command line.
 * But the pnacl-clang driver script doesn't have a way to do that.
 * We can fix the driver to respect -Wa,... and then change nacl.scons
 * to pass -Wa,-mattr=+vfp2; or we can fix the assembler to accept the
 * canonical ARM directives like .fpu; or we can fix the driver or the
 * LLVM build or whatever so that +vfp2 is on by default.
 * In the meantime, we just use hex constants for the VFP instructions below.
 */
#ifndef __clang__
        .fpu vfp
#endif

/*
 * Receives:
 * r0: first argument, pointer to double[16]
 * r1: second argument, value for fpscr
 */
        .text
        .p2align 4
        .globl infoleak_store_state
infoleak_store_state:
        bic r0, r0, #0xc0000000
        .word 0xec900b20 /* vldm r0, {d0-d15} */
        .word 0xeee11a10 /* vmsr fpscr, r1 */
        nop
.Lret:  bic lr, lr, #0xc000000f
        bx lr

/*
 * Receives:
 * r0: first argument, pointer to double[16]
 * Returns:
 * r0: value of fpscr
 */
        .p2align 4
        .globl infoleak_fetch_state
infoleak_fetch_state:
        bic r0, r0, #0xc0000000
        .word 0xec800b20 /* vstm r0, {d0-d15} */
        .word 0xeef10a10 /* vmrs r0, fpscr */
        b .Lret
