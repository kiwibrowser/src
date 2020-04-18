/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

.syntax unified
.arm
.global _start
_start:

  /* We test bundle-aligned and bundle-unaligned versions of jumps. */

  /* Jump backwards to addresses that wrap around and have the top bit set.
   * Such destinations are outside the sandbox's address space and so
   * could be dangerous, although it's unlikely that anything is mapped
   * here in the range that an ARM jump instruction could reach.
   */
  b _start - 0x1000000   /* disallowed */
  b _start - 0x1000004   /* disallowed */

  /* Jump backwards to the syscall trampoline page. */
  b _start - 0x10000     /* allowed */
  b _start - 0x0fffc     /* disallowed */
  b _start - 0x0fff0     /* allowed (16-aligned but not 32-aligned) */
  b _start - 0x0ffe0     /* allowed (32-aligned) */
  b _start - 0x1000      /* allowed */
  b _start - 0x1004      /* disallowed */

  /* Jump forwards to addresses outside this chunk of code. */
  b _start + 0x1000     /* allowed */
  b _start + 0x1004     /* disallowed */

  /* Jump forwards further. */
  b _start + 0x1000000   /* allowed */
  b _start + 0x1000004   /* disallowed */

  /* Jump to zero page.
   * Assumes that _start == 0x20000.
   */
  b 0                    /* allowed */
  b _start - 0x20000     /* allowed */
  b _start - 0x1fffc     /* disallowed */

  b end_of_code          /* allowed */

.p2align 4
end_of_code:
