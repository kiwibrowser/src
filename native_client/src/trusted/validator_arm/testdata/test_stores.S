/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Tests both legal and illegal variations on stores -- both stores that
 * require masking, and stores that are guaranteed sandboxed (i.e. through SP)
 */

/* we restrict store to the lower 1GB of the address space */
#define MASK 0xc0000000

.syntax unified
.arm
.globl _start
_start:
  .align 4

bundle0:
  bic r1, r3, #MASK       /* Generating a confined address */
  str r0, [r1]            /* and storing to it is fine. */

  bic r1, r1, #MASK       /* Confining an address in place */
  str r0, [r1]            /* and storing to it is fine. */

bundle1:
  mov r1, r3              /* Just poking at the address */
  str r0, [r1]            /* and storing to it is an ERROR. */

  bic r1, r3, #0          /* Even if we use BIC, if the mask is wrong, */
  str r0, [r1]            /* still an ERROR. */

bundle2:
  nop
  nop
  nop
  bic r1, r3, #MASK       /* If the BIC is in a different bundle... */

bundle3:
  str r0, [r1]            /* ...then the store is an ERROR. */
  nop

  biceq r2, r2, #0xC0000000 /* Mask a register and */
  strexeq r0, r1, [r2]    /* use it in a store-exclusive.  Should pass. */

bundle4:
  bic r2, r2, #0          /* Mask incorrectly and */
  strex r0, r1, [r2]      /* use it in a store-exclusive, for an ERROR. */

  nop
  nop

bundle5:
  bic r0, r0, #0xC0000000 /* Mask a register, and use */
  str r1, [r0], r2        /* it in register post-index store: should pass. */
  nop                     /* Don't mask, and */
  str r1, [r0], r2        /* use it in register post-index store: ERROR. */

bundle6:
  str r1, [sp], #1024     /* post-indexing sp by a constant is OK */
  str r1, [sp], r2        /* post-indexing sp by a register is an ERROR */
  bic r0, r0, #0xC0000000 /* Mask a register, and */
  str r1, [r0, r2]        /* use it in register pre-index store: ERROR */
