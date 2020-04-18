/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/arch/mips/sel_ldr_mips.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

.text

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
 * The trampoline code should push first 4 parameters of the system call to
 * the stack, and on top of that it should push the return address. On Mips the
 * trampoline is too small for all this, so the only thing that trampoline does
 * is call the function NaClSyscallSeg where the push is actually done.
 */

DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_trampoline_seg_code):

  .set noreorder

  /*
   * We don't actually load $t9 with zero, but we patch it with real address
   * of NaClSyscallSeg during placement of trampoline code.
   */

  lui   $t9, 0
  ori   $t9, $t9, 0
  jalr  $t5, $t9
  nop

  .word NACL_HALT_WORD
  .set reorder

DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_trampoline_seg_end):

