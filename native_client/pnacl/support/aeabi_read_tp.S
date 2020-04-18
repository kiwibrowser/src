/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "nacl_arm_defs.h"

/* ====================================================================== */
/* Some runtime code for ARM */
/* NOTE: this code can only clobber r0 and hence is hand coded */
/* Access to tls. We also have an unused trampoline version of this */
/* ====================================================================== */
    .align NACL_BLOCK_SHIFT
    .global __aeabi_read_tp

__aeabi_read_tp:
    ldr r0, [r9]

    .align NACL_BLOCK_SHIFT
    bic lr, lr, #NACL_CONTROL_FLOW_MASK
    bx lr
