/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * This file supplies dead code.  All that matters is that it is valid code
 * and occupies at least 32MB.  32MB is the limit for direct branches on ARM.
 */

        .text
        .p2align 4
far_padding:
        .fill (32 << 20) / NACL_HALT_LEN, NACL_HALT_LEN, NACL_HALT_OPCODE
