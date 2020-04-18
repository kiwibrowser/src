/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Tests if added 'violations' rule for mcr (in armv7.table) works. */

/* we restrict loads to the lower 1GB of the address space */
#define MASK 0xc0000000

.syntax unified
.arm
.globl _start
_start:
  .align 4

bundle0:
  mcr p15, 0, r2, c7, c10, 5
  nop
  nop
  nop
