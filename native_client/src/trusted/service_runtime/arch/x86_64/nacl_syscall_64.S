/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This code gets executed when switching from a 64-bit nacl module to
 * the 64-bit service. NaClSyscallSeg is the lcall target from the
 * syscall trampoline code, and this code is responsible for figuring
 * out the identity of the thread, saving the user registers, finish
 * restoring the segment registers (and getting out of the sandbox),
 * and actually invoking the C system call handler code.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/service_runtime/arch/x86_64/sel_rt_64.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * This macro gets the NaClThreadContext from the nacl_current_thread
 * TLS variable in Windows or Linux, or the corresponding
 * pthread_getspecific(...) TSD data in OSX and puts it in %rdx.
 *
 * May clobber two registers, %rax and %rcx.
 *
 * If %rdx contains 0, then this is an invalid thread.
 */
.macro get_tls
        /*
         * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
         *
         * Open coded TLS (or TSD) access, for all of our target host
         * OS/toolchains.  If the compiler / runtime conventions for
         * how to access TLS or TSD changes, this code will break
         * mysteriously.
         *
         * Any changes/fixes for this must be mirrored in nacl_test_capture.S
         *
         * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
         */
#if NACL_LINUX
        /*
         * Linux uses TLS.
         * We use "@GOTTPOFF" rather than "@TPOFF" in order to be PIC-friendly.
         */
        movq    nacl_current_thread@GOTTPOFF(%rip), %rax
        movq    %fs:(%rax), %rdx
#elif NACL_OSX
        /*
         * This assumes a particular implementation of OS X's
         * pthread_getspecific(), which we check for in NaClTlsInit().
         */
        movl    _nacl_current_thread_tls_offset(%rip), %eax
        movq    %gs:(%rax), %rdx
#elif NACL_WINDOWS
        /*
         * NOTE: This code should match the code in platform/win/test_tls.S!
         * If you change this code, please update that file as well.
         *
         * The following two lines generate this instruction:
         *   ba XX XX XX XX    movl $nacl_current_thread@SECREL, %edx
         *
         * SECREL is a relocation type which gives the offset of
         * nacl_current_thread within the TLS template section.  GNU
         * binutils' assembler does not have a syntax for generating
         * this relocation as part of an instruction.  It only has the
         * .secrel32 syntax for generating the relocated value as a
         * 32-bit literal.  We use this here for generating the
         * instruction by hand.
         *
         * For background, see:
         * http://code.google.com/p/nativeclient/issues/detail?id=2155
         *
         * because of the need to do .byte, we cannot simply name the
         * scratch registers via macro arguments.
         */
        .byte 0xba
        .secrel32 nacl_current_thread

        mov _tls_index(%rip), %ecx
        movq %gs:0x58, %rax
        /* Get the address of this module (executable/DLL)'s TLS area. */
        movq (%rax,%rcx,8), %rax
        /* Get the value of nacl_current_thread from the TLS area. */
        movq (%rdx,%rax), %rdx
#else
# error "What OS/compiler is the service runtime being compiled with?"
#endif
.endm

/*
 * On untrusted stack:
 *      return-addr-to-caller-of-trampoline
 *      return-addr-to-trampoline (essentially syscall number)
 *
 * This code must save the syscall arguments so that they can be
 * accessed in an uniformed way, regardless of whether the service
 * runtime was compiled using gcc (NACL_LINUX and NACL_OSX) or
 * using MS Studio (NACL_WINDOWS).
 */
        .text
DEFINE_GLOBAL_HIDDEN_FUNCTION(NaClSyscallSeg):
        cld

        /* Save system call arguments on the untrusted stack. */
        movl %r9d, -0x0c(%rsp)
        movl %r8d, -0x10(%rsp)
        movl %ecx, -0x14(%rsp)
        movl %edx, -0x18(%rsp)
        movl %esi, -0x1c(%rsp)
        movl %edi, -0x20(%rsp)

        /* rax, rdi, rsi, rdx, rcx, r8, r9 are usable for scratch */

        get_tls
        /*
         * Code below will segfault if %rdx is the NULL pointer, since
         * the zero page is unmapped.
         */

        /* only save the callee saved registers */
DEFINE_GLOBAL_HIDDEN_LOCATION(NaClSyscallThreadCaptureFault):
        movq    %rbx, NACL_THREAD_CONTEXT_OFFSET_RBX(%rdx)
        movq    %rbp, NACL_THREAD_CONTEXT_OFFSET_RBP(%rdx)
        /*
         * Record the value of %rsp that we will restore when
         * returning to untrusted code from the syscall.
         */
        leaq    8(%rsp), %rcx
        movq    %rcx, NACL_THREAD_CONTEXT_OFFSET_RSP(%rdx)
        movq    %r12, NACL_THREAD_CONTEXT_OFFSET_R12(%rdx)
        movq    %r13, NACL_THREAD_CONTEXT_OFFSET_R13(%rdx)
        movq    %r14, NACL_THREAD_CONTEXT_OFFSET_R14(%rdx)
        /* r15 need not be saved, since it is immutable from user code */

        /*
         * Save the x87 FPU control word.  This is callee-saved,
         * while all other x87 state is caller-saved.  Then reload
         * the system default state to use while running trusted code.
         */
        fnstcw  NACL_THREAD_CONTEXT_OFFSET_FCW(%rdx)
        fldcw   NACL_THREAD_CONTEXT_OFFSET_SYS_FCW(%rdx)

        /*
         * Save the SSE control word.  Then reload the system default
         * state to use while running trusted code.
         */
        stmxcsr NACL_THREAD_CONTEXT_OFFSET_MXCSR(%rdx)
        ldmxcsr NACL_THREAD_CONTEXT_OFFSET_SYS_MXCSR(%rdx)
DEFINE_GLOBAL_HIDDEN_LOCATION(NaClSyscallSegRegsSaved):

        movq    NACL_THREAD_CONTEXT_OFFSET_TRUSTED_STACK_PTR(%rdx), %rsp
#if NACL_LINUX || NACL_OSX
        movq    %rdx, %rdi
#elif NACL_WINDOWS
        movq    %rdx, %rcx
#else
# error "What OS/compiler is the service runtime being compiled with?"
#endif
        /*
         * We want to make sure that the return address on the stack will
         * be handled properly by the host system's stack unwinder.
         *
         * For Linux, it is fine that the return address does not have
         * associated unwind info, because libgcc's stack unwinder
         * will stop unwinding when it reaches such an address.  Note
         * that pthread_exit() causes stack unwinding.
         *
         * We cannot do the same thing on Windows because its stack
         * unwinder does not have a similar safe default.  If it
         * encounters a return address without unwind info, it applies
         * the rule for leaf functions (even though this rule does not
         * make sense for return addresses in the Windows x86-64 ABI),
         * and it does so even for zero return addresses.  This can
         * cause it to read uninitialised memory.
         *
         * So, for Windows, the stack has already been set up by
         * NaClSwitchSavingStackPtr() with a return address with
         * associated unwind info.
         */
#if NACL_WINDOWS
        /* NaClSyscallCSegHook returns to NaClSwitchSavingStackPtr. */
        jmp     IDENTIFIER(NaClSyscallCSegHook)
#else
        call    IDENTIFIER(NaClSyscallCSegHook)
        /*
         * NaClSyscallCSegHook returned the struct NaClThreadContext
         * pointer (in %rax), which will be the argument (in %rdi) to
         * the NaClSwitch function.
         */
        movq    %rax, %rdi
        call    *IDENTIFIER(NaClSwitch)(%rip)
        hlt
        /* noret */
#endif

        /*
         * untrusted stack after call to NaClSyscallCSegHook:
         *
         * 0x20   0x8 return-addr-to-caller-of-trampoline
         * 0x18   0x0 return-addr-to-trampoline (essentially syscall number)
         * 0x14 -0x04 r9
         * 0x10 -0x08 r8
         * 0x0c -0x0c rcx
         * 0x08 -0x10 rdx
         * 0x04 -0x14 rsi
         * 0x00 -0x18 rdi
         */

.macro tls_syscall arg1, arg2, arg3, arg4
DEFINE_GLOBAL_HIDDEN_LOCATION(\arg1):  /* Entry */
        get_tls
        movl    \arg4(%rdx), %eax
        /* sandbox the return, straight off the untrusted stack */
        pop     %rcx
DEFINE_GLOBAL_HIDDEN_LOCATION(\arg2):  /* RspRestored */
        addl    $31, %ecx
        andl    $0xffffffe0, %ecx
        leaq    (%r15, %rcx), %r11
        /*
         * Set %rdx to zero so that we do not leak the address of the
         * NaClThreadContext struct.  This also resets flags.
         * Resetting flags to fixed values by doing this last
         * simplifies testing that we do not leak any information via
         * flags.
         */
        xorl    %edx, %edx
        jmp     *%r11
DEFINE_GLOBAL_HIDDEN_LOCATION(\arg3):  /* End */
.endm

        /*
         * Note that long lines are required here because "\" does not
         * work in the Windows build.
         */
        tls_syscall NaClGetTlsFastPath1, NaClGetTlsFastPath1RspRestored, NaClGetTlsFastPath1End, NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE1
        tls_syscall NaClGetTlsFastPath2, NaClGetTlsFastPath2RspRestored, NaClGetTlsFastPath2End, NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE2
