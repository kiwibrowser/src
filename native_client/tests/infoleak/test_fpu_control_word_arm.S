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
 * r0: first argument, value for fpscr
 */
        .text
        .p2align 4
        .globl set_fpcw
set_fpcw:
        .word 0xeee10a10 /* vmsr fpscr, r0 */
.Lret:  bic lr, lr, #0xc000000f
        bx lr

/*
 * Returns:
 * r0: value of fpscr
 */
        .p2align 4
        .globl get_fpcw
get_fpcw:
        .word 0xeef10a10 /* vmrs r0, fpscr */
        b .Lret
