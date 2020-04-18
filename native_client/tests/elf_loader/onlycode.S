/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * This is a tiny program that contains only a code section and no other
 * non-code sections at all.  It just exits successfully.
 */

        .text
.globl _start
_start:
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86

        xor %edi, %edi
        push %edi
        call NACL_SYSCALL_ADDR(NACL_sys_exit)
        hlt

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm

        movw    ip, #:lower16:(NACL_SYSCALL_ADDR(NACL_sys_exit))
        movt    ip, #:upper16:(NACL_SYSCALL_ADDR(NACL_sys_exit))
        mov     r0, #0
        sfi_blx ip
        .inst   NACL_HALT_OPCODE

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips

        lui   $t9, %hi(NACL_SYSCALL_ADDR(NACL_sys_exit))
        addiu $t9, $t9, %lo(NACL_SYSCALL_ADDR(NACL_sys_exit))
        addu  $a0, $zero, $s0
        nop

        nop
        and   $t9, $t9, $t6
        jalr  $t9
        nop

#else /* NACL_ARCH(NACL_BUILD_ARCH) */

# error Unknown platform!

#endif /* NACL_ARCH(NACL_BUILD_ARCH) */
