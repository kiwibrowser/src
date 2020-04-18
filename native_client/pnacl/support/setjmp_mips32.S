/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "nacl_mips_defs.h"

  .text
  .p2align 4
  .align NACL_BLOCK_SHIFT
  .globl setjmp
  .ent setjmp
  .frame  $sp, 0, $31
  .set noreorder

/* int setjmp(jmp_buf env) */
setjmp:

  /* NACL_CALLEE_SAVE_LIST BEGIN */
  sw   $s0,  0($a0)
  sw   $s1,  4($a0)
  sw   $s2,  8($a0)
  sw   $s3,  12($a0)
  sw   $s4,  16($a0)
  sw   $s5,  20($a0)
  sw   $s6,  24($a0)
  sw   $s7,  28($a0)
  sw   $sp,  32($a0)
  sw   $fp,  36($a0)
  sw   $ra,  40($a0)
  swc1 $f20, 44($a0)
  swc1 $f21, 48($a0)
  swc1 $f22, 52($a0)
  swc1 $f23, 56($a0)
  swc1 $f24, 60($a0)
  swc1 $f25, 64($a0)
  swc1 $f26, 68($a0)
  swc1 $f27, 72($a0)
  swc1 $f28, 76($a0)
  swc1 $f29, 80($a0)
  swc1 $f30, 84($a0)
  swc1 $f31, 88($a0)
  /* NACL_CALLEE_SAVE_LIST END */
  move $v0,  $zero
  j    $ra
  nop  /* delay slot */

  .end setjmp

  .text
  .p2align 4
  .align NACL_BLOCK_SHIFT
  .globl longjmp
  .ent longjmp
  .set noreorder

/* void longjmp(jmp_buf env, int val) */
longjmp:

  /* NACL_CALLEE_LOAD_LIST BEGIN */
  lw   $s0,  0($a0)
  lw   $s1,  4($a0)
  lw   $s2,  8($a0)
  lw   $s3,  12($a0)
  lw   $s4,  16($a0)
  lw   $s5,  20($a0)
  lw   $s6,  24($a0)
  lw   $s7,  28($a0)
  lw   $sp,  32($a0)
  lw   $fp,  36($a0)
  lw   $ra,  40($a0)
  lwc1 $f20, 44($a0)
  lwc1 $f21, 48($a0)
  lwc1 $f22, 52($a0)
  lwc1 $f23, 56($a0)
  lwc1 $f24, 60($a0)
  lwc1 $f25, 64($a0)
  lwc1 $f26, 68($a0)
  lwc1 $f27, 72($a0)
  lwc1 $f28, 76($a0)
  lwc1 $f29, 80($a0)
  lwc1 $f30, 84($a0)
  lwc1 $f31, 88($a0)
  /* NACL_CALLEE_LOAD_LIST END */

  bne  $a1, $zero, 1f
  nop  /* delay slot */
  ori  $a1, $zero, 1
1:
  move $v0, $a1
  j    $ra
  nop  /* delay slot */

  .end longjmp

