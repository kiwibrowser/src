/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

        .data
timebuf:
        .space 0x200 /* enough for a timeval */

        .text
        .globl SyscallReturnIsSandboxed
SyscallReturnIsSandboxed:
        movq $0, %rsi
        movq $timebuf, %rdi
        movl $1f, %ecx
        addq %r15, %rcx
        pushq %rcx

        .p2align 5
        movl $NACL_SYSCALL_ADDR(NACL_sys_gettimeofday), %ecx
        andl $0xffffffe0, %ecx
        addq %r15, %rcx
        jmp *%rcx

        .p2align 5
        /*
         * Here we have a whole bundle of halts, with the "return address"
         * we pushed being in the middle of the bundle.  The syscall return
         * should round up this misaligned address to the beginning of the
         * next bundle.  If it fails to do that, we'll execute hlt and crash.
         */
        hlt
1:      .p2align 5,0xf4
        /* Now we're at the next bundle, where the return is rounded to. */

        /*
         * Test the tls_get call as well, since it may have an alternate
         * entry/return path in the trampoline implementation.
         */
        leaq 2f(%r15), %rcx
        pushq %rcx

        movl $NACL_SYSCALL_ADDR(NACL_sys_tls_get), %ecx
        andl $0xffffffe0, %ecx
        addq %r15, %rcx
        jmp *%rcx

        .p2align 5
        hlt
2:      .p2align 5,0xf4
        /* Now we're at the next bundle, where the return is rounded to. */

        /*
         * Test the second_tls_get call as well, since it may have an
         * alternate entry/return path in the trampoline
         * implementation.
         */
        leaq 3f(%r15), %rcx
        pushq %rcx

        movl $NACL_SYSCALL_ADDR(NACL_sys_second_tls_get), %ecx
        andl $0xffffffe0, %ecx
        addq %r15, %rcx
        jmp *%rcx

        .p2align 5
        hlt
3:      .p2align 5,0xf4
        /* Now we're at the next bundle, where the return is rounded to. */

        movl $1, %eax
        naclret
        .p2align 5 /* -- pnacl toolchain leaves "..." without this */
