/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "nacl_x86_defs.h"

/*
 * jump_buf is organized as follows
 * rbx, rbp, rsp, r12, r13, r14, r15, pc
 */
    .text
    .p2align NACL_BLOCK_SHIFT
    .globl   setjmp
    .type    setjmp, @function

/* int setjmp(jmp_buf env) */
setjmp:
    /* edi has env argument*/
    pop     %rdx                         /* return address */

    movq    %rbx, %nacl:0(%rdi)     /* save rbx */
    movq    %rbp, %nacl:8(%rdi)     /* save rbp */
    movq    %rsp, %nacl:16(%rdi)    /* save rsp */
    movq    %r12, %nacl:24(%rdi)    /* save r12 */
    movq    %r13, %nacl:32(%rdi)    /* save r13 */
    movq    %r14, %nacl:40(%rdi)    /* save r14 */
    movq    %r15, %nacl:48(%rdi)    /* save r15 */
    movq    %rdx, %nacl:56(%rdi)    /* save pc */

    xorq    %rax, %rax
    nacljmp    %edx


    .text
    .p2align NACL_BLOCK_SHIFT
    .globl   longjmp
    .type    longjmp, @function

/* void longjmp(jmp_buf env, int val) */
longjmp:
    /* edi has env argument */
    /* esi has val argument */
    movl    %esi, %eax
    /* if val is zero, we must return 1 -- otherwise return val */
    testl   %eax, %eax
    jne     .skip
    movl    $1, %eax
.skip:
    movq    %nacl:0(%rdi), %rbx     /* restore rbx */
    movq    %nacl:8(%rdi), %rdx     /* restore rbp */
    naclrestbp    %edx
    movq    %nacl:16(%rdi), %rdx    /* restore rsp */
    naclrestsp    %edx
    movq    %nacl:24(%rdi), %r12    /* restore r12 */
    movq    %nacl:32(%rdi), %r13    /* restore r13 */
    movq    %nacl:40(%rdi), %r14    /* restore r14 */
    movq    %nacl:48(%rdi), %r15    /* restore r15 */
    movq    %nacl:56(%rdi), %rcx
    nacljmp    %ecx
    .p2align NACL_BLOCK_SHIFT

