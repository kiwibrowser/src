/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Tests masked and unmasked forms of vector loads.  Regression test for
 * http://code.google.com/p/nativeclient/issues/detail?id=2129
 */

.syntax unified
.arm
.fpu neon
.globl _start
_start:

  .align 4
bundle0:
  vld2.16 {d10[2],d12[2]}, [r0]     /* Unmasked, should fail. */

  bic r0, r0, #0xC0000000           /* r0 is masked here, so */
  vld1.16 {d10-d12}, [r0]           /* this should succeed. */

  nop

bundle1:
  bic r0, r0, #0xC0000000           /* r0 is masked here, so */
  vld1.8 {d0}, [r0, :64], r1        /* register postincrement should succeed */

  bic r0, r0, #0xC0000000           /* r0 is masked here, but */
  vld1.8 {d0}, [sp, :64], r1        /* SP+register should fail. */

bundle2:
  vld1.8 {d3-d6}, [sp]!             /* Pop vectors - should pass. */
  vst1.8 {d3-d6}, [sp]!             /* Push wrong way vectors - should pass. */

  vld1.8 {d3-d6}, [sp], r12         /* Attempt to update SP without mask - */
                                    /* ERROR. */
  vst1.8 {d3-d6}, [sp], r12         /* Same deal. */

bundle3:
  vld1.8 {d3-d6}, [sp], r12         /* Attempt to update SP, */
  bic sp, sp, #0xC0000000           /* with mask - should pass. */
  vst1.8 {d3-d6}, [sp], r12         /* Same deal, */
  bic sp, sp, #0xC0000000           /* with mask - should pass. */
