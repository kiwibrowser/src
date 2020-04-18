/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"

        /*
         * This is code, but it is not code that is actually part of the
         * program/library being linked.  Marking it as read-only data
         * instead ensures that nothing like linker code-rewriting will
         * be applied to this code.
         */
        NACL_RODATA

DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_trampoline_code):
        /*
         * Write a value onto the stack which is used to calculate the
         * syscall number.
         */
        movl $0xdeadbeef, -8(%rsp)
DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_trampoline_tramp_addr):
        mov $0xdeadbeefcafebabe, %rax
DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_trampoline_call_target):
        /*
         * Fetch the address of NaClSyscallSeg (or other syscall
         * routine) and jump to it.  We store the address in a
         * location outside the sandbox so as not to defeat ASLR by
         * leaking address space layout to untrusted code.
         */
        jmp *(%rax)
        hlt
DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_trampoline_code_end):
