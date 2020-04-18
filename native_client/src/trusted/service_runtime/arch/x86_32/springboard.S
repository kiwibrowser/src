/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/arch/x86_32/sel_rt_32.h"

/*
 * Assembly code template.  This is linked into the service runtime
 * but is unused as code -- it is used as data to be patched into
 * a NaCl app's address space as a "hidden" part of its trampoline
 * region.
 */

        /*
         * This is code, but it is not code that is actually part of the
         * program/library being linked.  Marking it as read-only data
         * instead ensures that nothing like linker code-rewriting will
         * be applied to this code.
         */
        NACL_RODATA

/*
 * NaCl_springboard is used for syscall return and any time we want
 * to do an upcall into NaCl application.
 */

DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_springboard):
        hlt  /* one byte */
        /* Restore state from struct NaClThreadContext. */
        /* syscall return value, if any, from trusted stack */
        mov     NACL_THREAD_CONTEXT_OFFSET_SYSRET(%ecx), %eax
        lss     NACL_THREAD_CONTEXT_OFFSET_STACK_PTR(%ecx), %esp
        movw    NACL_THREAD_CONTEXT_OFFSET_DS(%ecx), %ds
        /* Set %ecx to zero to avoid leaking the NaClThreadContext address. */
        xorl    %ecx, %ecx
        jmp     *%edx
DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_springboard_end):


DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_springboard_all_regs):
        hlt  /* one byte */
        /* Restore state from struct NaClSwitchRemainingRegsState. */
        lss     0(%ecx), %esp /* stack_ptr */
        movw    0x10(%ecx), %ds /* ds */
        movl    %gs:0xc, %ecx /* new_ecx from NaClGsSegment */
        jmp     *%gs:8 /* new_prog_ctr from NaClGsSegment */
DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_springboard_all_regs_end):
