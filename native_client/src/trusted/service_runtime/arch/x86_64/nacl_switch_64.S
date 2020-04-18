/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This code gets executed when switching from the service
 * runtime to the NaCl module.  NaClSwitch has one parameter only,
 * which is a struct passed by reference.
 *
 * NB: this is not the only place where context switches from trusted
 * to untrusted code.  The NaClGetTlsFastPath* code (nacl_syscall_64.S)
 * also performs a partial switch (to and fro).
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/service_runtime/arch/x86_64/sel_rt_64.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

        /*
         * This function does not return.  Thus, we need to preserve
         * any callee-saved registers.
         */

        .text

.macro switcher arg1, arg2
DEFINE_GLOBAL_HIDDEN_FUNCTION(\arg1):
#if NACL_WINDOWS
        /* On Windows, 1st param is in %rcx. */
        mov     %rcx, %r11
#elif NACL_LINUX || NACL_OSX
        /* On Linux/OSX, 1st param is in %rdi. */
        mov     %rdi, %r11
#else
# error "What OS/compiler is the service runtime being compiled with?"
#endif

        /*
         * TODO(mcgrathr): Perhaps drop callee-saved registers (these six)
         * from this restore and instead just clear them at startup
         * (i.e. have NaClStartThreadInApp call a NaClStartSwitch that
         * clears them and calls NaClSwitch).
         */
        movq    NACL_THREAD_CONTEXT_OFFSET_RBX(%r11), %rbx
        movq    NACL_THREAD_CONTEXT_OFFSET_RBP(%r11), %rbp
        movq    NACL_THREAD_CONTEXT_OFFSET_R12(%r11), %r12
        movq    NACL_THREAD_CONTEXT_OFFSET_R13(%r11), %r13
        movq    NACL_THREAD_CONTEXT_OFFSET_R14(%r11), %r14
        movq    NACL_THREAD_CONTEXT_OFFSET_R15(%r11), %r15

        /* there is no springboard for x86_64 */
        movq    NACL_THREAD_CONTEXT_OFFSET_RSP(%r11), %rsp
        movq    NACL_THREAD_CONTEXT_OFFSET_SYSRET(%r11), %rax

        /*
         * %rdi is the first argument in the user calling convention.
         * When starting the initial thread, we are passing the address
         * of the parameter block here.  The initial stack pointer has
         * been adjusted to one word below there, to insert a dummy
         * return address for the user entry point function.
         */
        leal    8(%rsp), %edi

        /*
         * Zero all unused registers.  The 32-bit instructions
         * are a byte shorter than their 64-bit counterparts
         * when the target register is one of the first eight,
         * and they implicitly zero the high halves.
         *
         * The 'xorl' instruction also resets most flags to known
         * values.
         */
        xorl    %edx, %edx
        movl    %edx, %ecx
        movl    %edx, %esi
        movq    %rdx, %r8
        movq    %rdx, %r9
        movq    %rdx, %r10

        /*
         * Clear the x87, MMX, and SSE state.
         * Then restore the untrusted code's x87 and SSE control words.
         * We could roll them together by storing a 512-byte per-thread
         * buffer and setting the control words in that in NaClSyscallSeg.
         * But that would bloat struct NaClThreadContext by 504 bytes or so,
         * and the performance cost of these two instructions after fxrstor
         * seems to be immeasurably small.
         */
        fxrstor fxrstor_default_state(%rip)
        fldcw   NACL_THREAD_CONTEXT_OFFSET_FCW(%r11)
        ldmxcsr NACL_THREAD_CONTEXT_OFFSET_MXCSR(%r11)

.if \arg2
        /*
         * Clear the AVX state that the "fxrstor" instruction doesn't cover.
         * We could roll them together by using the "xrstor" instruction, but
         * that has a complicated protocol and this seems to perform fine.
         *
         * This is "vzeroupper".
         * Some assembler versions don't know the AVX instructions.
         */
        .byte   0xc5, 0xf8, 0x77
.endif

        /*
         * Load the return address into %r11 rather than doing
         * "jmp *XXX(%r11)" so that we do not leak the address of the
         * struct NaClThreadContext to untrusted code.  Knowing this
         * address would make bugs in the sandbox easier to exploit.
         */
        movq    NACL_THREAD_CONTEXT_OFFSET_NEW_PROG_CTR(%r11), %r11
        jmp     *%r11
.endm

        switcher NaClSwitchSSE, 0
        switcher NaClSwitchAVX, 1


        NACL_RODATA
        /*
         * This is the memory block for "fxrstor" to read.  The only
         * contents that matter are the fcw and mxcsr words, which we
         * store separately.  The mxcsr_mask word is ignored by the
         * hardware, so there is no need to get the hardware-supplied
         * value for that.  The hardware requires that this address be
         * aligned to 16 bytes.  Align it further to 64 bytes because
         * that is the usual size of a cache line; this might help
         * performance and is very unlikely to hurt it.
         */
        .balign 64
fxrstor_default_state:
        .space 512
