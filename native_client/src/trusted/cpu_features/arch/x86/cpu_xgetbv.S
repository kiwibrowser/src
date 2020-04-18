/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This exists only because Windows 64 doesn't support trivial inline assembly.
 * Visual Studio 2010 SP1 has an intrinsic _xgetbv() for this.
 * But earlier versions do not.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

        .text

DEFINE_GLOBAL_HIDDEN_FUNCTION(NaClXGETBV):
#if NACL_BUILD_SUBARCH == 32
        movl 4(%esp), %ecx
#elif !NACL_WINDOWS
        movl %edi, %ecx
#endif
        /*
         * This is "xgetbv" (no operands).  Some assemblers don't know it yet.
         */
        .byte 0x0f, 0x01, 0xd0
#if NACL_BUILD_SUBARCH == 64
        shlq $32, %rdx
        orq %rdx, %rax
#endif
        ret
