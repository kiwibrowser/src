/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/arch/mips/sel_ldr_mips.h"
#include "native_client/src/trusted/service_runtime/arch/mips/sel_rt.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

.text


/*
 * This trusted code is linked into the service runtime. It is executed when a
 * nacl module performs a system call via a service runtime interface. The nacl
 * module jumps to the trampoline corresponding to the syscall and then here.
 * This code switches the execution contexts (registers and stack) from
 * untrusted to trusted.
 *
 * on stack:
 *
 * top
 * =====
 * a0 - alignment for NaClSyscallCSegHook
 * ra - return address
 * arg 1
 * arg 2
 * .....
 * arg N
 */

DEFINE_GLOBAL_HIDDEN_FUNCTION(NaClSyscallSeg):

  .set noreorder

/*
 * The following code (next 6 instructions) does what the trampoline code should
 * do. It places the first 4 parameters of the system call to the stack, and on
 * top of that it places the return address to the user module. On Mips the
 * trampoline is too small for all this, so the code is moved here.
 *
 * Mips passes parameters to a callee in registers a0-a3. If there are more
 * than 4 parameters, the first four args are passed in registers and the rest
 * are placed on the stack. Regardless of the number of arguments passed, Mips
 * o32 ABI requires that the caller should always reserve 16 bytes on stack that
 * correspond to registers a0-a3.
 *
 * This code pushes all parameters from registers into the stack; thus, we keep
 * all parameters on the stack as follows:
 * top - arg0, arg1, arg2, arg3 .... argN
 *
 * On top of that we push the return address, so we will know where to return
 * after the system call.
 *
 * After this code the stack layout must look like:
 *
 * sp+0: retaddr to user module
 * sp+4: arg0 (system call argument)
 * sp+8: arg1
 * ..........
 * sp+X: argN
 *
 * When service runtime serves a system call, it first creates a structure which
 * utilizes these arguments. The structure is created by Decoder functions in
 * nacl_syscall_handlers.c. (nacl_syscall_handlers.c is an automatically
 * generated file and placed in
 * scons-out//gen/native_client/src/trusted/service_runtime).
 */

  /*
   * .cpload will expand into the three instructions function prologue that sets
   * up the $gp register to the trusted value.
   */
  .cpload $t9

  /*
   * Place a0-a3 argument registers in the reserved 16-byte region in caller's
   * stack frame (o32 ABI), and place return address on top of that.
   */
  sw    $a3, 12($sp)
  sw    $a2,  8($sp)
  sw    $a1,  4($sp)
  sw    $a0,  0($sp)
  /* Save return address for returning to untrusted code. */
  sw    $ra, -4($sp)
  /* Save return address that indicates which trampoline was called. */
  sw    $t5, -8($sp)
  add   $a0, $t8, -NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE1

  lw    $t2, NACL_THREAD_CONTEXT_OFFSET_GUARD_TOKEN($a0)
  la    $t3, nacl_guard_token
  lw    $t3, 0($t3)
  bne   $t2, $t3, NaClSyscallThreadCaptureFault
  nop

  sw $s0, NACL_THREAD_CONTEXT_OFFSET_S0($a0)
  sw $s1, NACL_THREAD_CONTEXT_OFFSET_S1($a0)
  sw $s2, NACL_THREAD_CONTEXT_OFFSET_S2($a0)
  sw $s3, NACL_THREAD_CONTEXT_OFFSET_S3($a0)
  sw $s4, NACL_THREAD_CONTEXT_OFFSET_S4($a0)
  sw $s5, NACL_THREAD_CONTEXT_OFFSET_S5($a0)
  sw $s6, NACL_THREAD_CONTEXT_OFFSET_S6($a0)
  sw $s7, NACL_THREAD_CONTEXT_OFFSET_S7($a0)
  sw $t8, NACL_THREAD_CONTEXT_OFFSET_T8($a0)
  sw $sp, NACL_THREAD_CONTEXT_OFFSET_STACK_PTR($a0)
  sw $fp, NACL_THREAD_CONTEXT_OFFSET_FRAME_PTR($a0)
DEFINE_GLOBAL_HIDDEN_LOCATION(NaClSyscallSegRegsSaved):

  /* Restore the trusted stack */
  lw $sp, NACL_THREAD_CONTEXT_OFFSET_TRUSTED_STACK_PTR($a0)

  move  $s0, $gp
  lw    $t9,%call16(NaClSyscallCSegHook)($gp)
  jalr  $t9
  nop

  /*
   * NaClSyscallCSegHook returned the NaClThreadContext pointer in $v0.
   * Make that the argument (in $a0) to NaClSwitch.
   */
  lw    $t9,%call16(NaClSwitch)($s0)
  jr    $t9
  move  $a0, $v0 /* delay slot */

  /* NOTREACHED */

DEFINE_GLOBAL_HIDDEN_LOCATION(NaClSyscallSegEnd):
  /*
   * If the thread returns, which must not happen, it will be halted
   * by the following instruction.
   */
DEFINE_GLOBAL_HIDDEN_LOCATION(NaClSyscallThreadCaptureFault):
  .word NACL_HALT_WORD
