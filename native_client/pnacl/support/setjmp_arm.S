/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#if defined(__native_client_nonsfi__)

    .text
    .globl setjmp

/* int setjmp(jmp_buf env) */
setjmp:
    stmia   r0!, {r4-r11, sp, lr}
    vstmia  r0!, {d8-d15}
    mov     r0, #0
    bx      lr

    .text
    .globl longjmp

/* void longjmp(jmp_buf env, int val) */
longjmp:
    ldmia   r0!, {r4-r11, sp, lr}
    vldmia  r0!, {d8-d15}
    /* Return the result argument if nonzero or 1 otherwise */
    /* Note: move updates condition codes */
    movs    r0, r1
    moveq   r0, #1
    bx      lr

#else

#include "nacl_arm_defs.h"

/*
 * jump_buf is organized as follows
 *
 * r4, r5, r6, r7, r8, r10 (sl), r11 (fp), r12 (ip/pad), r13 (sp), r14 (lr),
 * d8-d15.
 *
 * Note: We do not save/restore r9 - which pnacl uses for thread accounting.
 * r12 is spilled to pad out the jump buf so that d8-d15 are stored at an
 * 8-byte aligned location.
 *
 * This also assumes that the minimal ARM hardware has d8-d15 registers,
 * which is checked: src/trusted/platform_qualify/arch/arm/nacl_qualify_fpu.c
 *
 * The assembly is correctly bundle-aligned and organized with whitespace
 * between each bundle, so the bundle_lock directives are not strictly
 * necessary, but they prevent clang's auto-sandboxer from redundantly
 * sandboxing the code.
 */

    .text
    .align NACL_BLOCK_SHIFT
    .globl setjmp

/* int setjmp(jmp_buf env) */
setjmp:
    .bundle_lock
    bic     r0, r0, #NACL_DATA_FLOW_MASK
    stmia   r0!, {r4, r5, r6, r7, r8, r10, r11, r12, sp, lr}
    .bundle_unlock
    .bundle_lock
    bic     r0, r0, #NACL_DATA_FLOW_MASK
    vstmia  r0!, {d8-d15}
    .bundle_unlock

    mov     r0, #0
    .bundle_lock
    bic     lr, lr, #NACL_CONTROL_FLOW_MASK
    bx      lr
    .bundle_unlock

    .text
    .align NACL_BLOCK_SHIFT
    .globl longjmp

/* void longjmp(jmp_buf env, int val) */
longjmp:
    .bundle_lock
    bic     r0, r0, #NACL_DATA_FLOW_MASK
    ldmia   r0!, {r4, r5, r6, r7, r8, r10, r11, r12, sp, lr}
    bic     sp, sp, #NACL_DATA_FLOW_MASK
    .bundle_unlock
    nop

    .bundle_lock
    bic     r0, r0, #NACL_DATA_FLOW_MASK
    vldmia  r0!, {d8-d15}
    .bundle_unlock
    /* Return the result argument if nonzero or 1 otherwise */
    /* Note: move updates condition codes */
    movs    r0, r1
    moveq   r0, #1

    .bundle_lock
    bic     lr, lr, #NACL_CONTROL_FLOW_MASK
    bx      lr
    .bundle_unlock

#endif
