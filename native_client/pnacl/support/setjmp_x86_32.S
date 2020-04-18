/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "nacl_x86_defs.h"

#if defined(__native_client_nonsfi__)
# define NACLJMP(reg) jmp *reg
#else
# define NACLJMP(reg) nacljmp reg
#endif

/*
 * jump_buf is organized as follows
 * ebx, ebp, esp, esi, edi, pc
 * NOTE: eax, ecx, edx are NOT preserved
 */
    .text
    .p2align NACL_BLOCK_SHIFT
    .globl   setjmp
    .type    setjmp, @function

/* int setjmp(jmp_buf env) */
setjmp:
    pop     %edx              /* return address */

    movl    0(%esp), %ecx     /* ecx has env argument*/

    movl    %ebx, 0(%ecx)     /* save ebx */
    movl    %ebp, 4(%ecx)     /* save ebp */
    movl    %esp, 8(%ecx)     /* save esp */
    movl    %esi, 12(%ecx)    /* save esi */
    movl    %edi, 16(%ecx)    /* save edi */
    movl    %edx, 20(%ecx)    /* save pc */

    xor     %eax, %eax        /* return zero */
    NACLJMP(%edx)


    .text
    .p2align NACL_BLOCK_SHIFT
    .globl   longjmp
    .type    longjmp, @function

/* void longjmp(jmp_buf env, int val) */
longjmp:
    movl    4(%esp), %ecx     /* ecx has env argument */
    movl    8(%esp), %eax     /* eax has val argument */
    /* If val is zero, we must return 1 -- otherwise return val */
    testl   %eax, %eax
    jne     .skip
    movl    $1, %eax
.skip:
    movl    0(%ecx), %ebx     /* restore ebx */
    movl    4(%ecx), %ebp     /* restore ebp */
    movl    8(%ecx), %esp     /* restore esp */
    movl    12(%ecx), %esi    /* restore esi */
    movl    16(%ecx), %edi    /* restore edi */
    movl    20(%ecx), %ecx
    NACLJMP(%ecx)
    .p2align NACL_BLOCK_SHIFT

