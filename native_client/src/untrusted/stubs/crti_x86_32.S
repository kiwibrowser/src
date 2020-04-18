/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#if !defined(__clang__)
    /*
     * The .init section contains one symbol, _init, which is the entry
     * of a function and hence needs to be aligned in accordance with
     * the NativeClient conventions.
     */
    .section .init
    .p2align NACLENTRYALIGN
    .global _init
_init:
    pushl  %ebp
    movl   %esp, %ebp

    /*
     * The code that follows will be appended by the linker, and will not
     * be aligned individually, so we need to align it.
     */
    .p2align NACLENTRYALIGN

    /*
     * The .fini section contains one symbol, _fini, which is the entry
     * of a function and hence needs to be aligned in accordance with
     * the NativeClient conventions.
     */
    .section .fini
    .p2align NACLENTRYALIGN
    .global _fini
_fini:
    pushl  %ebp
    movl   %esp, %ebp
#else
    /*
     * nacl-clang does not use the .init/.fini mechanism at all, but newlib
     * still calls _init and _fini, so define them as empty functions in the
     * text section.
     */
    .section .text
    .p2align NACLENTRYALIGN
    .global _init
_init:
     naclret

     .p2align NACLENTRYALIGN
     .global _fini
_fini:
     naclret
#endif
    /*
     * The code that follows will be appended by the linker, and will not
     * be aligned individually, so we need to align it.
     */
    .p2align NACLENTRYALIGN
