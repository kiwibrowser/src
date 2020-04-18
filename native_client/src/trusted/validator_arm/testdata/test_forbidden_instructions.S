/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Tests a few instructions that are always illegal in NaCl.
 */
.syntax unified
.arm
.globl _start
_start:
  swi 6     /* SWI illegal in untrusted code */

  adr r0, foo
  bxj r0    /* BXJ illegal in NaCl */

foo:
  b foo
