/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Secure Runtime
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/service_runtime/arch/x86_32/sel_rt_32.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * NB: why movl vs movw for moving to segment registers: movl is two
 * bytes, movw is three. ISA manual say these should be semantically
 * equivalent.  clang complains w/o explicit sizes.
 */

        .text

#define DO_NONE 0
#define DO_SSE 1
#define DO_AVX 2
#define DO_USING_ARG_IN_ECX 3

.macro switcher arg1, arg2
DEFINE_GLOBAL_HIDDEN_LOCATION(\arg1):
.if \arg2 != DO_USING_ARG_IN_ECX
        /*
         * For DO_USING_ARG_IN_ECX don't touch the stack and get the
         * thread context from ecx instead of the stack.
         */
        popl    %eax /* throw away the return addr */

        /* do not leak info to app */
        xorl    %ecx, %ecx
        /* xorl will leave eflags in a known state, so no info leaks */
        popl    %ecx
.endif
        movl    NACL_THREAD_CONTEXT_OFFSET_NEW_PROG_CTR(%ecx), %edx
        movl    NACL_THREAD_CONTEXT_OFFSET_FRAME_PTR(%ecx), %ebp
        movl    NACL_THREAD_CONTEXT_OFFSET_EDI(%ecx), %edi
        movl    NACL_THREAD_CONTEXT_OFFSET_ESI(%ecx), %esi
        movl    NACL_THREAD_CONTEXT_OFFSET_EBX(%ecx), %ebx

        movw    NACL_THREAD_CONTEXT_OFFSET_GS(%ecx), %gs
        movw    NACL_THREAD_CONTEXT_OFFSET_FS(%ecx), %fs
        movw    NACL_THREAD_CONTEXT_OFFSET_ES(%ecx), %es

        /*
         * None of the FPU state stuff matters for the DO_USING_ARG_IN_ECX
         * case, which is only used to transfer control back to untrusted
         * code.  As well as being unnecessary, it's unsafe to use it here
         * because the 'call' below might use an untrusted stack.
         */
.if \arg2 != DO_USING_ARG_IN_ECX
        /*
         * Clear the x87, MMX, and SSE state.
         * Then restore the untrusted code's x87 and SSE control words.
         * We could roll them together by storing a 512-byte per-thread
         * buffer and setting the control words in that in NaClSyscallSeg.
         * But that would bloat struct NaClThreadContext by 504 bytes or so,
         * and the performance cost of these two instructions after fxrstor64
         * seems to be immeasurably small.
         */
        call __x86.get_pc_thunk.ax /* PIC support */
.if \arg2 == DO_AVX || \arg2 == DO_SSE
        fxrstor fxrstor_default_state-.(%eax)
        ldmxcsr NACL_THREAD_CONTEXT_OFFSET_MXCSR(%ecx)
.else
        frstor frstor_default_state-.(%eax)
.endif
        fldcw   NACL_THREAD_CONTEXT_OFFSET_FCW(%ecx)

.if \arg2 == DO_AVX
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
.endif

        ljmp    *NACL_THREAD_CONTEXT_OFFSET_SPRING_ADDR(%ecx)
.endm

        switcher NaClSwitchNoSSE, DO_NONE
        switcher NaClSwitchSSE, DO_SSE
        switcher NaClSwitchAVX, DO_AVX

/* Generate register only version on OSX. */
#if NACL_OSX
        switcher NaClSwitchNoSSEViaECX, DO_USING_ARG_IN_ECX
#endif

#if NACL_LINUX
        /*
         * All this magic matches what the compiler would emit.
         * It arranges that this thunk is merged at link time with
         * any other duplicate copy of the same code.
         */
        .section .text.__x86.get_pc_thunk.ax,"axG",@progbits,__x86.get_pc_thunk.ax,comdat
        .globl  __x86.get_pc_thunk.ax
        .hidden __x86.get_pc_thunk.ax
        .type   __x86.get_pc_thunk.ax, @function
__x86.get_pc_thunk.ax:
        .cfi_startproc
        movl (%esp), %eax
        ret
        .cfi_endproc
#else
        .globl __x86.get_pc_thunk.ax
#if NACL_OSX
        /*
         * This is what the HIDDEN macro does, but that prepends an
         * underscore under NACL_OSX and that's not right for this case.
         * HIDDEN is empty for NACL_WINDOWS, and the NACL_LINUX case is
         * dealt with above.
         */
        .private_extern __x86.get_pc_thunk.ax
#endif
__x86.get_pc_thunk.ax:
        movl (%esp), %eax
        ret
#endif

        NACL_RODATA
        /*
         * This is the memory block for "fxrstor" to read.
         * The only contents that matter are the fcw and mxcsr words,
         * which we store separately.  The mxcsr_mask word is ignored by
         * the hardware, so there is no need to get the
         * hardware-supplied value for that.  The hardware requires that
         * this address be aligned to 16 bytes.  Align it further to 64
         * bytes because that is the usual size of a cache line; this
         * might help performance and is very unlikely to hurt it.
         *
         * Note: On Windows the assembler currently used (GAS 2.17.50) doesn't
         * request any alignment of the .rodata section, so the memory block
         * only gets the default alignment which is 16 bytes. If we upgrade to
         * GAS 2.20.1, we could request alignment in the .section directive.
         */
        .balign 64
fxrstor_default_state:
        .space 512
        /*
         * This is the memory block for "frstor" to read.  It matters that
         * some of these words have the expected nonzero values, so it's
         * not possible to reuse the fxrstor_default_state space for this
         * too.  The bit pattern below is the usual default state.
         */
frstor_default_state:
        .long 0xffff037f  /* x87 control word */
        .long 0xffff0000  /* x87 status word */
        .long 0xffffffff  /* x87 tag word */
        .long 0           /* x87 IP offset */
        .long 0           /* x87 IP selector */
        .long 0           /* x87 operand pointer offset */
        .long 0xffff0000  /* x87 operand pointer selector */
        /*
         * This is better written as: .space 108 - (. - frstor_default_state).
         * But the LLVM assembler (used on MacOS and in Clang builds on
         * Linux) is buggy and does not support that use of label arithmetic.
         * Reported as: http://llvm.org/bugs/show_bug.cgi?id=15315
         */
        .space 108 - (7 * 4)
