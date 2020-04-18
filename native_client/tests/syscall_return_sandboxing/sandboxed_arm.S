/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * The assembly is correctly bundle-aligned and organized with whitespace
 * between each bundle, so the bundle_lock directives are not strictly
 * necessary, but they prevent clang's auto-sandboxer from redundantly
 * sandboxing the code.
 */

#include "native_client/src/include/arm_sandbox.h"

        /*
         * assume 32-byte trampoline slots, 16-byte bundles.
         */
#define GETTIMEOFDAY_ADDR 0x10500

        .data
timebuf:
        .space 0x200 /* enough for a timeval */

        .text
        .globl SyscallReturnIsSandboxed
        .p2align 4
SyscallReturnIsSandboxed:
        nop
        push {lr}
        ldr r0, addrOfTimeBuf
        mov r1, #0

        adr lr, bad
        ldr r2, addrOfGetTimeOfDaySyscallAddr
        .bundle_lock
        bic r2, r2, #0xf000000f
        bx  r2
        .bundle_unlock

        eor r0, r0, #1
bad:    pop {lr}
        .bundle_lock
        bic lr, lr, #0xf000000f
        bx lr
        .bundle_unlock

        .p2align 4
        .word NACL_INSTR_ARM_LITERAL_POOL_HEAD
        /*
         * Put a disallowed instruction here to ensure that the bkpt
         * data bundle marker works.  If the marker does not work, then
         * whether this code passes the validator is a matter of chance.
         */
        mov pc, r0
addrOfGetTimeOfDaySyscallAddr:
        .word GETTIMEOFDAY_ADDR
addrOfTimeBuf:
        .word timebuf
