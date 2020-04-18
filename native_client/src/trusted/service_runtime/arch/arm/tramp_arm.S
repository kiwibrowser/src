/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/arch/arm/sel_ldr_arm.h"
#include "native_client/src/trusted/service_runtime/arch/arm/sel_rt.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

        /*
         * This is code, but it is not code that is actually part of the
         * program/library being linked.  Marking it as read-only data
         * instead ensures that nothing like linker code-rewriting will
         * be applied to this code.
         */
        NACL_RODATA

/*
 * Assembly code template.
 * This is linked into the service runtime but is unused as code -- it is used
 * as data to be patched into a NaCl app's address space.
 *
 * Trampoline to transfer control from native client module to
 * sel_ldr's NaClSyscallSeg residing in the service runtime portion of address
 * space. Trampolines are patched into nacl module's address space in the
 * trampoline region. They are patched by NaClLoadTrampoline() code (sel_ldr.c).
 * Each trampoline code segment corresponds to a system call, so the trampoline
 * region is full of identical trampoline code segments. Service runtime
 * distinguish which system call is requested using the address of an executed
 * trampoline (it is saved on stack in NaClSyscallSeg()).
 *
 * ARM passes parameters to a callee in registers r0-r3. If there are more
 * than 4 parameters, the first four are passed in registers and the rest are
 * placed on the stack. This code saves all parameters from registers into the
 * stack; thus, we keep all parameters on the stack as follows:
 * top - arg0, arg1, arg2, arg3 .... argN
 *
 * On top of that we save the return address, so we will know where to return
 * after the system call.
 *
 * At the point this code calls NaClSyscallSeg, the stack layout is as follows:
 *
 *   sp+0x04: argument 5 (if present)
 *   sp:      argument 4 (if present)
 *   sp-0x04: argument 3
 *   sp-0x08: argument 2
 *   sp-0x0c: argument 1
 *   sp-0x10: argument 0
 *   sp-0x14: return address to untrusted code
 *  (sp-0x18: return address to trampoline - saved later by NaClSyscallSeg)
 *
 * Usually, signal-safe code will not save data below the stack
 * pointer, but it is safe to do so here because the trusted signal
 * handler will never run on the untrusted stack.
 *
 * When service runtime serves a system call, it first creates a structure which
 * utilizes these arguments. The structure is created by Decoder functions in
 * nacl_syscall_handlers.c. (nacl_syscall_handlers.c is an automatically
 * generated file and placed in
 * scons-out//gen/native_client/src/trusted/service_runtime).
 */

DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_trampoline_seg_code):
  /*
   * Save first 4 syscall arguments below the stack pointer.  Any
   * further arguments are already on the stack.
   */
  stmfd sp, {r0, r1, r2, r3}
  /* Save the return address. */
  str lr, [sp, #-0x14]

  ldr r0, [r9, #NACL_R9_OFFSET(NACL_THREAD_CONTEXT_OFFSET_SYSCALL_ROUTINE)]
  /* NOTE: we use the blx instead of bx because the return addres */
  /*       is used to determine which trampoline was used */

  blx r0

  /* NORETURN */

  /* Fill second bundle: */
  .word NACL_INSTR_ARM_HALT_FILL
  .word NACL_INSTR_ARM_HALT_FILL
  .word NACL_INSTR_ARM_HALT_FILL
  .word NACL_INSTR_ARM_HALT_FILL

DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_trampoline_seg_end):
