/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Tests both legal and illegal variations on loads -- both loads that
 * require masking, and loads that are guaranteed sandboxed (i.e. through SP)
 */

/* we restrict loads to the lower 1GB of the address space */
#define MASK 0xc0000000

.syntax unified
.arm
.globl _start
_start:
  .align 4

bundle0:
  bic r1, r3, #MASK       /* Generating a confined address */
  ldr r0, [r1]            /* and storing to it is fine. */

  bic r1, r1, #MASK       /* Confining an address in place */
  ldr r0, [r1]            /* and storing to it is fine. */

bundle1:
  mov r1, r3              /* Just poking at the address */
  ldr r0, [r1]            /* and storing to it is an ERROR. */

  bic r1, r3, #0          /* Even if we use BIC, if the mask is wrong, */
  ldr r0, [r1]            /* still an ERROR. */

bundle2:
  nop
  nop
  nop
  bic r1, r3, #MASK       /* If the BIC is in a different bundle... */

bundle3:
  ldr r0, [r1]            /* ...then the loads is an ERROR. */
  nop

  biceq r2, r2, #0xC0000000 /* Mask a register and */
  ldrexeq r0, [r2]        /* use it in a load-exclusive.  Should pass. */

bundle4:
  bic r2, r2, #0          /* Mask incorrectly and */
  ldrex r0, [r2]          /* use it in a load-exclusive, for an ERROR. */

  nop
  nop

bundle5:
  bic r0, r0, #0xC0000000 /* Mask a register, and */
  ldr r1, [r0], r2        /* use it in register post-index load: should pass. */
  nop                     /* Don't mask, and */
  ldr r1, [r0], r2        /* use it in register post-index load: ERROR. */

bundle6:
  bic r1, r1, #MASK       /* Confining an address in place */
  ldr pc, [r1]            /* loading into pc is an ERROR */

  bic r1, r1, #MASK       /* Confining an address in place */
  ldr sp, [r1]            /* loading into sp is an ERROR without a mask */

bundle7:
  bic r1, r1, #MASK       /* Confining an address in place */
  ldr sp, [r1]            /* loading into sp is OK with a mask afterwards */
  bic sp, sp, #MASK       /* Confining an address in place */
  nop

bundle8:
  bic r1, r1, #MASK       /* Confining an address in place */
  ldr r1, [r1, r2]        /* loading with an index is an ERROR */

  nop
  nop

bundle9:
  ldr r1, [sp], #1024     /* post-indexing sp by a constant is fine. */
  ldr r1, [sp], r2        /* post-indexing sp by a register is an ERROR */
  nop
  nop
