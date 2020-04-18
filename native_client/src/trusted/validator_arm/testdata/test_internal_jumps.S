/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

.syntax unified
.arm
.global _start
_start:

/* Test forwards jumps */
.p2align 4
  b bundle_good
  b middle_good
  b middle_bad

.p2align 4
bundle_good:
  nop
middle_good:
  bic lr, lr, #0xf000000f
middle_bad:
  bx lr

/* Test backwards jumps */
.p2align 4
  b bundle_good
  b middle_good
  b middle_bad

/* Test similar jumps with variations on the mask */
.p2align 4
  bic lr, lr, #0xc000000f  /* 1GiB mask */
  bx lr                    /* should succeed */

  bic lr, lr, #0x8000000f  /* 2GiB mask */
  bx lr                    /* must fail */

  bic lr, lr, #0xe000000f  /* in between mask */
  bx lr                    /* also succeeds */
