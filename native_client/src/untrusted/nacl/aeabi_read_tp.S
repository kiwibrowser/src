/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * The compiler generates calls to __aeabi_read_tp to retrieve the thread
 * pointer ($tp).  Note that the EABI requires that this function preserve
 * all registers except r0 (the return value register).
 */

        .globl __aeabi_read_tp
        .type __aeabi_read_tp,%function
        .p2align NACL_BLOCK_SHIFT
__aeabi_read_tp:
        ldr r0, [r9]
        sfi_bx lr
