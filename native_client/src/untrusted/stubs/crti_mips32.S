/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

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
        jr $ra
        nop
        .p2align 4
        .globl _fini
        .type _fini,%function
_fini:
        jr $ra
        nop

