/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
        .syntax unified
        .arm
        .fpu vfp

        .p2align 4
        .global try_operations_with_snans
try_operations_with_snans:
        /* Try a simple SNaN <op> SNaN w/ VFP ops. NEON doesn't trap. */
        movw r0, #0
        movt r0, #0x7fa0
        vmov.f32 s14, r0
        vmov.f32 s15, r0

        vadd.f32 s15, s14, s15
        /* Reload an argument in case it was converted to a QNaN. */
        vmov.f32 s15, r0
        vsub.f32 s15, s14, s15
        vmov.f32 s15, r0

        vmul.f32 s15, s14, s15
        vmov.f32 s15, r0
        vdiv.f32 s15, s14, s15
        nop

        bic lr, lr, #0xc000000f
        bx lr
