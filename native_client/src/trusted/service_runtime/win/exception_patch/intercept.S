/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * ntdll_patch.c patches KiUserExceptionDispatcher to jump to the
 * routine below.  To keep the patch instruction sequence small, we
 * use a 32-bit relative jump, which means this routine has to be
 * relocated to within +/-2GB of KiUserExceptionDispatcher.  Hence the
 * code below does not contain any %rip-relative instructions, since
 * these would require special handling when relocating the code.
 *
 * This routine checks whether the thread was executing untrusted
 * code.  If so, it terminates the process safely.  Otherwise, we
 * resume execution of KiUserExceptionDispatcher and any exception
 * handlers are run normally.
 */

        .globl IDENTIFIER(NaCl_exception_dispatcher_intercept)
IDENTIFIER(NaCl_exception_dispatcher_intercept):
        /*
         * Save registers, since we clobber these registers as part of
         * the TLS variable access below.  We do not know which
         * registers, if any, are used as part of the interface
         * between the kernel and KiUserExceptionDispatcher, but we
         * save them just in case.
         *
         * At this point, we do not know whether the exception was
         * triggered by untrusted code.  Writing to the stack is
         * mostly safe, but reading from the stack is not.  There is a
         * small risk that we could be writing sensitive values to the
         * untrusted stack that could read by untrusted code.
         */
        push %rax
        push %rcx
        push %rdx

        /*
         * The following two lines generate this instruction:
         *   ba XX XX XX XX    movl $nacl_on_safe_stack@SECREL, %edx
         */
        .byte 0xba
        .secrel32 nacl_on_safe_stack
        .globl IDENTIFIER(NaCl_exception_dispatcher_intercept_tls_index)
IDENTIFIER(NaCl_exception_dispatcher_intercept_tls_index):
        /*
         * The immediate value 0x12345678 is replaced with the value of
         * _tls_index when this code is copied.
         */
        movl $0x12345678, %ecx
        movq %gs:0x58, %rax
        /* Get the address of this module (executable/DLL)'s TLS area. */
        movq (%rax,%rcx,8), %rax
        movl (%rdx,%rax), %eax
        /* %eax now contains the thread-local value of nacl_on_safe_stack */

        test %eax, %eax
        jnz handle_trusted_fault

        /*
         * The fault was in untrusted code, so fault safely.  We use
         * the same code as for NaCl_exception_dispatcher_exit_fast.
         */
        xor %rsp, %rsp
        hlt

handle_trusted_fault:
        /*
         * Restore registers.  At this point we know that reading from
         * the stack is safe.
         */
        pop %rdx
        pop %rcx
        pop %rax
        /*
         * We fall through to instructions that have been relocated from
         * the start of KiUserExceptionDispatcher, followed by a jump into
         * the rest of KiUserExceptionDispatcher.
         */
        .globl IDENTIFIER(NaCl_exception_dispatcher_intercept_end)
IDENTIFIER(NaCl_exception_dispatcher_intercept_end):
