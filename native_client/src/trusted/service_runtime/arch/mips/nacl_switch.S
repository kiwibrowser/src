/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/arch/mips/sel_rt.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

.text

/*
 * This trusted code is linked into the service_runtime and
 * executed when switching from the service runtime to a nacl
 * module. This happens when a main nacl thread is created and starting to
 * execute the nacl code, or when nacl module is returning from a system
 * call. This piece of code lives in a service runtime part of address space.
 * When a new thread is started, we also clear callee-saved FPU registers
 * (f20-f31), while in the case of returning to a thread these registers are set
 * and restored by trusted code.
 * The one and only argument is in register a0
 *
 * a0 -- address of thread context (struct NaClThreadContext)
 */

DEFINE_GLOBAL_HIDDEN_FUNCTION(NaClStartSwitch):

  .set noreorder

  /*
   * A new untrusted thread is started, so we clear even callee saved
   * FPU registers.
   */

  mtc1  $zero, $f0
  mtc1  $zero, $f1
  mov.d $f20,  $f0
  mov.d $f22,  $f0
  mov.d $f24,  $f0
  mov.d $f26,  $f0
  mov.d $f28,  $f0
  mov.d $f30,  $f0

DEFINE_GLOBAL_HIDDEN_FUNCTION(NaClSwitch):

  /*
   * We clear the following registers to avoid information leaks. The
   * remaining registers are overwritten by the code that follows
   * after.
   */
  addu $at, $zero, $zero
  addu $v1, $zero, $zero
  addu $a1, $zero, $zero
  addu $a2, $zero, $zero
  addu $a3, $zero, $zero
  addu $t0, $zero, $zero
  addu $t1, $zero, $zero
  addu $t2, $zero, $zero
  addu $t3, $zero, $zero
  addu $t4, $zero, $zero
  addu $t5, $zero, $zero
  addu $gp, $zero, $zero
  addu $ra, $zero, $zero

  /*
   * We clear floating point registers f0-f19, while f20-f31 are callee-saved,
   * and those need clearing only when a new thread is started.
   */
  mtc1  $zero, $f0
  mtc1  $zero, $f1
  mov.d $f2,   $f0
  mov.d $f4,   $f0
  mov.d $f6,   $f0
  mov.d $f8,   $f0
  mov.d $f10,  $f0
  mov.d $f12,  $f0
  mov.d $f14,  $f0
  mov.d $f16,  $f0
  mov.d $f18,  $f0

  /* NACL_CALLEE_SAVE_LIST BEGIN */

  lui     $t6, %hi(NACL_CONTROL_FLOW_MASK)
  addiu   $t6, $t6, %lo(NACL_CONTROL_FLOW_MASK)
  lui     $t7, %hi(NACL_DATA_FLOW_MASK)
  addiu   $t7, $t7, %lo(NACL_DATA_FLOW_MASK)
  /*
   * TODO(mcgrathr): Perhaps drop callee-saved registers (s0-s8)
   * from this restore and instead just clear them at startup
   * (i.e. have NaClStartThreadInApp call a NaClStartSwitch that
   * clears them and calls NaClSwitch).
   */
  lw $s0, NACL_THREAD_CONTEXT_OFFSET_S0($a0)
  lw $s1, NACL_THREAD_CONTEXT_OFFSET_S1($a0)
  lw $s2, NACL_THREAD_CONTEXT_OFFSET_S2($a0)
  lw $s3, NACL_THREAD_CONTEXT_OFFSET_S3($a0)
  lw $s4, NACL_THREAD_CONTEXT_OFFSET_S4($a0)
  lw $s5, NACL_THREAD_CONTEXT_OFFSET_S5($a0)
  lw $s6, NACL_THREAD_CONTEXT_OFFSET_S6($a0)
  lw $s7, NACL_THREAD_CONTEXT_OFFSET_S7($a0)
  lw $t8, NACL_THREAD_CONTEXT_OFFSET_T8($a0)
  lw $sp, NACL_THREAD_CONTEXT_OFFSET_STACK_PTR($a0)
  lw $fp, NACL_THREAD_CONTEXT_OFFSET_FRAME_PTR($a0)

  /* NACL_CALLEE_SAVE_LIST END*/

  lw $v0, NACL_THREAD_CONTEXT_OFFSET_SYSRET($a0)
  lw $t9, NACL_THREAD_CONTEXT_OFFSET_NEW_PROG_CTR($a0)

  /* At startup, context->sysret contains not the the return value, but the
     first argument. Put it in a0. */
  addu $a0, $v0, $zero

  /*
   * Transfer control to untrusted code.
   *
   * We leave $t9 containing the address of the function being called,
   * as required by the MIPS ABI.
   */
  jr  $t9
  nop

  .set reorder
