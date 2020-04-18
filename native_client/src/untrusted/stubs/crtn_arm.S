/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * The .init and .fini sections contain the _init and _fini functions,
 * respectively.  This file defines the epilogue of each function, and
 * crti_arm.S defines each prologue.
 * nacl-clang does not use the .init/.fini mechanism at all.
 */
#if !defined(__clang__)
        .section .init
        sfi_bx lr

        .section .fini
        sfi_bx lr
#endif
