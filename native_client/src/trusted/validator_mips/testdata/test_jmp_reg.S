# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# Test jump register instructions (jr and jalr).
#

#include "native_client/pnacl/support/nacl_mips_defs.h"

   .align 4
   .globl __start
__start:
   .set noreorder

bundle_8mod16_jr:
   nop
   and    $t1,$t1,JUMP_MASK
   jr     $t1                    # OK
    nop                          # Instruction in the delay slot is indented.

bundle_4mod16_jr:
   and    $t1,$t1,JUMP_MASK
   jr     $t1                    # OK
    nop
   nop

bundle_8mod16_jalr:
   nop
   and    $t1,$t1,JUMP_MASK
   jalr   $t1                    # OK
    nop

   # Jalr instruction not at bundle offset 8.

bundle_4mod16_jalr:
   and    $t1,$t1,JUMP_MASK
   jalr   $t1                    # Error, misaligned call.
    nop
   and    $a1,$a1,JUMP_MASK

cross_bundle:
   jr     $a1                    # Error, pseudo-instruction crosses bundle.
    nop
   nop
   nop

bundle_bad_mask:
   addiu  $a0, $zero, 16
   and    $a1, $a1, STORE_MASK
   jalr   $a1                    # Error, bad jump mask.
    nop

bundle_no_mask:
   addiu  $a0, $zero, 16
   lw     $a2, 16($sp)
   jalr   $a1                    # Error, no jump mask.
    nop

bundle_jalr_t0_t9:
   addu   $s0, $a0, $a1
   and    $v0, $v0, JUMP_MASK
   jalr   $t0, $v0 # ok
    nop

bundle_jalr_t7_t9:
   addu   $s0, $a0, $a1
   and    $v0, $v0, JUMP_MASK
   jalr   STORE_MASK, $v0        # Error, alteration of read only register.
    nop

bundle_jalr_t7_t92:
   addu   $s0, $a0, $a1
   and    $v0, $v0, JUMP_MASK
   jalr   $sp, $v0               # Error, alteration of sp register.
    nop

bundle_jal_delay_slot:
   b      bundle_jalr_t7_t92
    and    $v0, $v0, JUMP_MASK
   jalr   $t0, $v0               # OK
    nop

end_of_code:
