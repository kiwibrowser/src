/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

        .p2align 5
        .global try_operations_with_snans
try_operations_with_snans:
#if defined(__x86_64__)
        mov $0x7fa00000, %eax
        mov %eax, -0x4(%rsp)
        movss -0x4(%rsp), %xmm0
        movss -0x4(%rsp), %xmm1
        addss %xmm1, %xmm0
        /* Reload an argument in case it was converted to a QNaN. */
        movss -0x4(%rsp), %xmm0
        subss %xmm1, %xmm0
        movss -0x4(%rsp), %xmm0
        mulss %xmm1, %xmm0
        movss -0x4(%rsp), %xmm0
        divss %xmm1, %xmm0
        naclret
#else
        sub $12, %esp
        mov $0x7fa00000,%eax
        mov %eax, 0(%esp)
        movss 0(%esp), %xmm0
        movss 0(%esp), %xmm1
        addss %xmm1, %xmm0
        movss 0(%esp), %xmm0
        subss %xmm1, %xmm0
        movss 0(%esp), %xmm0
        mulss %xmm1, %xmm0
        movss 0(%esp), %xmm0
        divss %xmm1, %xmm0
        /* Also try some x87 operations. */
        flds 0(%esp)
        flds 0(%esp)
        faddp %st, %st(1)
        flds 0(%esp)
        fsubp %st, %st(1)
        flds 0(%esp)
        fmulp %st, %st(1)
        flds 0(%esp)
        fdivp %st, %st(1)
        add $12, %esp
        naclret
#endif
