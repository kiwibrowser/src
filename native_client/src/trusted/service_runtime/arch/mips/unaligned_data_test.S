/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/pnacl/support/nacl_mips_defs.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

    .section .rodata
    exit_addr: .word NACL_SYSCALL_ADDR(NACL_sys_exit)
    .section .data
    exit_status: .word 0

    .text
    .globl _start
    .align 4
_start:
    .set noreorder

    lui     $t9, %hi(exit_addr)
    ori     $t9, $t9, %lo(exit_addr)
    and     $t9, $t9, STORE_MASK
    lw      $t9, 0x0($t9)

    lui     $a0, %hi(exit_status)
    ori     $a0, $a0, %lo(exit_status)
    and     $a0, $a0, STORE_MASK
    lw      $a0, 0x0($a0)

    nop
    and     $t9, $t9, JUMP_MASK
    jr      $t9
    nop
