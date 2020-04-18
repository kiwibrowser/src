/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * The .init and .fini sections contain the _init and _fini functions,
 * respectively.  This file defines the prologue of each function, and
 * crtn_arm.S defines each epilogue.
 */
#if !defined(__clang__)
        .section .init
        .p2align 4
        .globl _init
        .type _init,%function
_init:

        .section .fini
        .p2align 4
        .globl _fini
        .type _fini,%function
_fini:
#else
/*
 * nacl-clang does not use the .init/.fini mechanism at all, but newlib
 * still calls _init and _fini, so define them as empty functions in the
 * text section.
 */
        .section .text
        .p2align 4
        .globl _init
        .type _init,%function
_init:
        sfi_bx lr

        .p2align 4
        .globl _fini
        .type _fini,%function
_fini:
        sfi_bx lr
#endif
