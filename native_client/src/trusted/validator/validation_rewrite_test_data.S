/*
 * Copyright 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_asm.h"

#define GLOBAL_SYM(name) .global IDENTIFIER(name); IDENTIFIER(name):

.data

#if NACL_BUILD_SUBARCH == 32

GLOBAL_SYM(no_rewrite_code)
    movntq %mm0, (%ebx)
GLOBAL_SYM(no_rewrite_code_end)

GLOBAL_SYM(no_rewrite_code_post_rewrite)
    movntq %mm0, (%ebx)
GLOBAL_SYM(no_rewrite_code_post_rewrite_end)

GLOBAL_SYM(movntq_code)
    movntq %mm0, (%ebx)
GLOBAL_SYM(movntq_code_end)

GLOBAL_SYM(movntq_code_post_rewrite)
    movq %mm0, (%ebx)
GLOBAL_SYM(movntq_code_post_rewrite_end)

GLOBAL_SYM(movntps_code)
    movntps %xmm0, (%ebx)
GLOBAL_SYM(movntps_code_end)

GLOBAL_SYM(movntps_code_post_rewrite)
    movaps %xmm0, (%ebx)
GLOBAL_SYM(movntps_code_post_rewrite_end)

GLOBAL_SYM(movntdq_code)
    movntdq %xmm0, (%edx)
GLOBAL_SYM(movntdq_code_end)

GLOBAL_SYM(movntdq_code_post_rewrite)
    movdqa %xmm0, (%edx)
GLOBAL_SYM(movntdq_code_post_rewrite_end)

GLOBAL_SYM(prefetchnta_code)
    prefetchnta (%eax)
GLOBAL_SYM(prefetchnta_code_end)

GLOBAL_SYM(prefetchnta_code_post_rewrite)
    nop
    nop
    nop
GLOBAL_SYM(prefetchnta_code_post_rewrite_end)

#else

GLOBAL_SYM(no_rewrite_code)
    mov %edi,%edi
    movnti %rax,0x68(%r15,%rdi,1)
GLOBAL_SYM(no_rewrite_code_end)

GLOBAL_SYM(no_rewrite_code_post_rewrite)
    mov %edi,%edi
    movnti %rax,0x68(%r15,%rdi,1)
GLOBAL_SYM(no_rewrite_code_post_rewrite_end)

GLOBAL_SYM(off_webstore_movnt_code)
    mov %ebx,%ebx
    movntq %mm0,(%rbx)
GLOBAL_SYM(off_webstore_movnt_code_end)

GLOBAL_SYM(off_webstore_movnt_code_post_rewrite)
    mov %ebx,%ebx
    movntq %mm0,(%rbx)
GLOBAL_SYM(off_webstore_movnt_code_post_rewrite_end)

GLOBAL_SYM(prefetchnta_code)
    mov %edi,%edi
    prefetchnta (%r15,%rdi,1)
GLOBAL_SYM(prefetchnta_code_end)

GLOBAL_SYM(prefetchnta_code_post_rewrite)
    mov %edi,%edi
    nop
    nop
    nop
    nop
    nop
GLOBAL_SYM(prefetchnta_code_post_rewrite_end)

GLOBAL_SYM(prefetchnta_rip_relative_code)
    mov %edi,%edi
    prefetchnta (%rip)
GLOBAL_SYM(prefetchnta_rip_relative_code_end)

GLOBAL_SYM(prefetchnta_rip_relative_code_post_rewrite)
    mov %edi,%edi
    nop
    nop
    nop
    nop
    nop
    nop
    nop
GLOBAL_SYM(prefetchnta_rip_relative_code_post_rewrite_end)

GLOBAL_SYM(movntq_code)
    mov %ebx,%ebx
    movntq %mm0,(%r15,%rbx,1)
GLOBAL_SYM(movntq_code_end)

GLOBAL_SYM(movntq_code_post_rewrite)
    mov %ebx,%ebx
    movq %mm0,(%r15,%rbx,1)
GLOBAL_SYM(movntq_code_post_rewrite_end)

GLOBAL_SYM(movntps_code)
    mov %ebx,%ebx
    movntps %xmm0,(%r15,%rbx,1)
GLOBAL_SYM(movntps_code_end)

GLOBAL_SYM(movntps_code_post_rewrite)
    mov %ebx,%ebx
    movaps %xmm0,(%r15,%rbx,1)
GLOBAL_SYM(movntps_code_post_rewrite_end)

GLOBAL_SYM(movnti_code)
    mov %edi,%edi
    movnti %rax,0x68(%r15,%rdi,1)
GLOBAL_SYM(movnti_code_end)

GLOBAL_SYM(movnti_code_post_rewrite)
    mov %edi,%edi
    mov %rax,0x68(%r15,%rdi,1)
    nop
GLOBAL_SYM(movnti_code_post_rewrite_end)

GLOBAL_SYM(movnti_code2)
    mov %edx,%edx
    movnti %r9d,(%r15,%rdx,1)
GLOBAL_SYM(movnti_code2_end)

GLOBAL_SYM(movnti_code2_post_rewrite)
    mov %edx,%edx
    mov %r9d,(%r15,%rdx,1)
    nop
GLOBAL_SYM(movnti_code2_post_rewrite_end)

GLOBAL_SYM(movnti_rip_relative_code)
    movnti %rax,0x20(%rip)
GLOBAL_SYM(movnti_rip_relative_code_end)

GLOBAL_SYM(movnti_rip_relative_code_post_rewrite)
    nop
    mov %rax,0x20(%rip)
GLOBAL_SYM(movnti_rip_relative_code_post_rewrite_end)

GLOBAL_SYM(movntdq_code)
    mov %edx,%edx
    movntdq %xmm0,0x10(%r15,%rdx,1)
GLOBAL_SYM(movntdq_code_end)

GLOBAL_SYM(movntdq_code_post_rewrite)
    mov %edx,%edx
    movdqa %xmm0,0x10(%r15,%rdx,1)
GLOBAL_SYM(movntdq_code_post_rewrite_end)

GLOBAL_SYM(movntdq_code2)
    mov %ecx,%ecx
    movntdq %xmm15,(%r15,%rcx,1)
GLOBAL_SYM(movntdq_code2_end)

GLOBAL_SYM(movntdq_code2_post_rewrite)
    mov %ecx,%ecx
    movdqa %xmm15,(%r15,%rcx,1)
GLOBAL_SYM(movntdq_code2_post_rewrite_end)

GLOBAL_SYM(multiple_movnt_code)
    mov %edi,%edi
    movnti %rax,0x68(%r15,%rdi,1)
    mov %edx,%edx
    movntdq %xmm0,0x10(%r15,%rdx,1)
GLOBAL_SYM(multiple_movnt_code_end)

GLOBAL_SYM(multiple_movnt_code_post_rewrite)
    mov %edi,%edi
    mov %rax,0x68(%r15,%rdi,1)
    nop
    mov %edx,%edx
    movdqa %xmm0,0x10(%r15,%rdx,1)
GLOBAL_SYM(multiple_movnt_code_post_rewrite_end)

    /* size = 32 bytes */
GLOBAL_SYM(one_bundle_movnt_code)
    mov %ecx,%ecx
    movntdq %xmm15,(%r15,%rcx,1)
    mov %ecx,%ecx
    movntdq %xmm15,(%r15,%rcx,1)
    mov %ecx,%ecx
    movntdq %xmm15,(%r15,%rcx,1)
    mov %ecx,%ecx
    movntdq %xmm15,(%r15,%rcx,1)
GLOBAL_SYM(one_bundle_movnt_code_end)

    /* size = 32 bytes */
GLOBAL_SYM(one_bundle_movnt_code_post_rewrite)
    mov %ecx,%ecx
    movdqa %xmm15,(%r15,%rcx,1)
    mov %ecx,%ecx
    movdqa %xmm15,(%r15,%rcx,1)
    mov %ecx,%ecx
    movdqa %xmm15,(%r15,%rcx,1)
    mov %ecx,%ecx
    movdqa %xmm15,(%r15,%rcx,1)
GLOBAL_SYM(one_bundle_movnt_code_post_rewrite_end)

GLOBAL_SYM(last_movnti_cross_bundle_by_one)
    mov %ecx,%ecx                 /* offset 0, length 2 */
    movntdq %xmm15,(%r15,%rcx,1)  /* offset 2, length 6 */
    mov %ecx,%ecx                 /* offset 8, length 2 */
    movntdq %xmm15,(%r15,%rcx,1)  /* offset 10, length 6 */
    mov %ecx,%ecx                 /* offset 16, length 2 */
    movntdq %xmm15,(%r15,%rcx,1)  /* offset 18, length 6 */
    nop                           /* offset 24, length 1 */
    mov %edx,%edx                 /* offset 25, length 2 */
    /* the last byte of the following instruction is at offset 32 */
    movnti %r9d,(%r15,%rdx,1)     /* offset 27, length 5 */
GLOBAL_SYM(last_movnti_cross_bundle_by_one_end)

GLOBAL_SYM(last_movnti_cross_bundle_by_one_post_rewrite)
    mov %ecx,%ecx                 /* offset 0, length 2 */
    movdqa %xmm15,(%r15,%rcx,1)   /* offset 2, length 6 */
    mov %ecx,%ecx                 /* offset 8, length 2 */
    movdqa %xmm15,(%r15,%rcx,1)   /* offset 10, length 6 */
    mov %ecx,%ecx                 /* offset 16, length 2 */
    movdqa %xmm15,(%r15,%rcx,1)   /* offset 18, length 6 */
    nop                           /* offset 24, length 1 */
    mov %edx,%edx                 /* offset 25, length 2 */
    mov %r9d,(%r15,%rdx,1)        /* offset 27, length 4 */
    nop                           /* offset 32, length 1 */
GLOBAL_SYM(last_movnti_cross_bundle_by_one_post_rewrite_end)

#endif
