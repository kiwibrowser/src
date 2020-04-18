/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime context switch code.  NaClSyscallSeg is the
 * lcall target from the syscall trampoline code, and this code is
 * responsible for figuring out the identity of the thread, saving
 * the user registers, finish restoring the segment registers (and
 * getting out of the sandbox), and actually invoking the C system
 * call handler code.
 */

#include "native_client/src/trusted/service_runtime/arch/x86_32/sel_rt_32.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * on stack:
 *
 *  syscall-arg-N
 *  ...
 *  syscall-arg-1
 *  RA for caller to libc stub
 *  code seg from trampoline lcall
 *  RA for trampoline lcall (used to compute syscall number)
 */

        .text

        /*
         * The trampoline code calls NaClPcrelThunk using an lcall,
         * restoring %cs.  Next, NaClPcrelThunk invokes
         * NaClSyscallSeg, with nacl_user in %ecx and %ds restored.
         * The other segment registers remain to be restored.
         *
         * For accessing global variables, we avoid PIC address
         * synthesis using the usual sequence of
         *
         *     call 0f
         * 0:  pop %eax
         *
         * to get the address of the pop into a register.  Such
         * sequences are verboten in the context switch code, since we
         * are still using a stack in untrusted memory, and we need to
         * figure out the per-thread secure stack first.
         */
.macro syscallseg arg1, arg2, arg3, arg4
DEFINE_GLOBAL_HIDDEN_LOCATION(\arg1):
        cld
        xor     %eax, %eax
        movw    %gs, %ax
        /*
         * Check that the %gs value is really one that might have been
         * assigned to a NaCl thread.  This could only happen if an
         * attacker adaptively used a NaCl module to discover the
         * absolute addresses needed to make a syscall (%cs relative
         * is not enough, unless it is a zero-based code memory
         * sandbox), and use an independent code vulnerability (e.g.,
         * in an image processing library linked into the same
         * application embedding NaCl) to cause a trusted thread to
         * jump into this code.
         */
        movl    %eax, %edx
        andl    $7, %edx  /* extract TI and RPL fields */
        cmpl    $7, %edx  /* make sure it's LDT, ring 3 */
        jne     1f
        shr     $3, %eax
        test    %eax, %eax  /* zero is illegal (and default) */
        jz      1f

        movl    (%ecx,%eax,4), %edx
        test    %edx, %edx
        jz      1f

        movl    %ebx, NACL_THREAD_CONTEXT_OFFSET_EBX(%edx)
        movl    %esi, NACL_THREAD_CONTEXT_OFFSET_ESI(%edx)
        movl    %edi, NACL_THREAD_CONTEXT_OFFSET_EDI(%edx)
        movl    %ebp, NACL_THREAD_CONTEXT_OFFSET_FRAME_PTR(%edx)
        /*
         * Record the value of %esp that we will restore when
         * returning to untrusted code from the syscall.
         */
        leal    0xc(%esp), %ecx
        movl    %ecx, NACL_THREAD_CONTEXT_OFFSET_STACK_PTR(%edx)

        /*
         * Save the x87 FPU control word.  This is callee-saved,
         * while all other x87 state is caller-saved.  Then reload
         * the system default state to use while running trusted code.
         */
        fnstcw  NACL_THREAD_CONTEXT_OFFSET_FCW(%edx)
        fldcw   NACL_THREAD_CONTEXT_OFFSET_SYS_FCW(%edx)

.if \arg2
        /*
         * Save the SSE control word.  Then reload the system default
         * state to use while running trusted code.
         */
        stmxcsr NACL_THREAD_CONTEXT_OFFSET_MXCSR(%edx)
        ldmxcsr NACL_THREAD_CONTEXT_OFFSET_SYS_MXCSR(%edx)
.endif
DEFINE_GLOBAL_HIDDEN_LOCATION(\arg3): /* NaClSyscallSegRegsSaved */

        /*
         * We do not save segment registers, which untrusted code is
         * powerless to change.
         */

        /*
         * load only the system segments; called code do not depend on
         * any registers having particular values.  we will clear/discard
         * caller-saved registers at system call return.
         *
         * %cs and %ds already taken care of by NaCl_trampoline_seg_code
         *
         * %ebx need not be saved/restored even if -fpic were used, since
         * in that case %ebx is initialized on fn entry.
         */
        movw    NACL_THREAD_CONTEXT_OFFSET_TRUSTED_ES(%edx), %es
        movw    NACL_THREAD_CONTEXT_OFFSET_TRUSTED_FS(%edx), %fs
        movw    NACL_THREAD_CONTEXT_OFFSET_TRUSTED_GS(%edx), %gs

        /* 'lss' sets %ss as well as %esp */
        lss     NACL_THREAD_CONTEXT_OFFSET_TRUSTED_STACK_PTR(%edx), %esp
        push    %edx
        call    IDENTIFIER(NaClSyscallCSegHook)

        /* NaClSyscallCSegHook return value is argument to NaClSwitch. */
        push    %eax

        /*
         * Now switch back to untrusted code.  This does not return.
         * The first call is standard PIC magic.
         * See nacl_switch_32.S for the definition.
         */
        call    __x86.get_pc_thunk.ax
        call    *(IDENTIFIER(NaClSwitch)-.)(%eax)

        /*
         * If stack usage in the above code changes, modify initial %esp
         * computation -- see nacl_switch_to_app.c:NaClStartThreadInApp.
         */
DEFINE_GLOBAL_HIDDEN_LOCATION(\arg4): /* NaClSyscallThreadCaptureFault */
1:      hlt
        /* noret */
.endm

        /*
         * Note that long lines are required here because "\" does not
         * work in the Windows build.
         */
        syscallseg NaClSyscallSegNoSSE, 0, NaClSyscallSegRegsSavedNoSSE, NaClSyscallThreadCaptureFaultNoSSE
        syscallseg NaClSyscallSegSSE, 1, NaClSyscallSegRegsSavedSSE, NaClSyscallThreadCaptureFaultSSE

        /*
         * This is defined here rather than in C because the assembler for
         * MacOS cannot handle the PC-relative reference above unless the
         * symbol is defined in the same file.
         */
        .data
DEFINE_GLOBAL_HIDDEN_LOCATION(NaClSwitch):
        .long 0
