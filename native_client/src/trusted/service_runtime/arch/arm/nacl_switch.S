/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/arch/arm/sel_rt.h"

.text
.fpu neon

/*
 * This trusted code is linked into the service_runtime and
 * executed when switching from the service runtime to a NaCl
 * module. This happens when a main NaCl thread is created and starting to
 * execute the NaCl code, or when NaCl module is returning from a system
 * call. This piece of code lives in a service runtime part of address space.
 * The one and only argument is in register r0.
 *
 * r0 -- address of thread context (struct NaClThreadContext)
 */

/*
 * Here we are starting a new untrusted thread.  The callee-saved VFP/NEON
 * registers have ambient values from trusted code, which must not leak
 * into untrusted code.  So clear them.
 */
DEFINE_GLOBAL_HIDDEN_FUNCTION(NaClStartSwitch):
  veor q4, q4, q4
  veor q5, q5, q5
  veor q6, q6, q6
  veor q7, q7, q7

/*
 * Execution falls through to here.  This is also the entry point for
 * returning from a syscall, which does not need to repeat the work of
 * clearing the callee-saved registers because the C frames for the call
 * to NaClSyscallCSegHook and its callees will save and restore their
 * incoming (i.e. the untrusted) values of those registers.
 */
DEFINE_GLOBAL_HIDDEN_FUNCTION(NaClSwitch):

/*
 * Load the untrusted code's value of the fpscr into a scratch register.
 */
  ldr ip, [r0, #NACL_THREAD_CONTEXT_OFFSET_FPSCR]

/*
 * We clear registers r2, r3, lr, flag and status fields in
 * CPSR (status register) to avoid information leaks.  The remaining
 * registers are overwritten by the code that follows after.
 */

  mov r2, #0
  mov r3, #0
  mov lr, #0
  msr cpsr_fs, #0

/*
 * Clear out the VFP/NEON state that otherwise might leak information.
 * Registers d8-d15 (aka q4-q7) are callee-saves, so the untrusted state
 * of those will have been saved and restored by whatever trusted code
 * paths might have touched them.  The other VFP/NEON registers are
 * call-clobbered, so trusted code might have left interesting bits
 * there and we must hide that information.
 */
  veor q0, q0, q0
  veor q1, q1, q1
  veor q2, q2, q2
  veor q3, q3, q3
  veor q8, q8, q8
  veor q9, q9, q9
  veor q10, q10, q10
  veor q11, q11, q11
  veor q12, q12, q12
  veor q13, q13, q13
  veor q14, q14, q14
  veor q15, q15, q15

/*
 * Restore the untrusted code's fpscr state.
 */
  fmxr fpscr, ip

/*
 * Restore register state from struct NaClThreadContext.
 *
 * TODO(mcgrathr): Perhaps drop callee-saved registers (r4-r8,r10-r11)
 * from this restore and instead just clear them in NaClStartSwitch.
 */
  ldmia r0, NACL_CALLEE_SAVE_LIST
  ldr sp, [r0, #NACL_THREAD_CONTEXT_OFFSET_STACK_PTR]
  /* Loads the "sysret" field into r0 and the "new_prog_ctr" field into r1. */
  ldrd r0, r1, [r0, #NACL_THREAD_CONTEXT_OFFSET_SYSRET]
  /* Transfer control to untrusted code */
  bx r1
