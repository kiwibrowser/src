# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# Tests both legal and illegal variations on SP updates.
#

#include "native_client/pnacl/support/nacl_mips_defs.h"

  .align 4
  .globl __start
__start:
   .set noreorder
   .set mips32r2

   # Test sp alteration without the appropriate mask.

bundle_sp_dest_reg:
   addiu   $sp, $sp, 64             # Error, no mask after sp alteration.
   and     $sp, $a0, $a1            # Error, no mask after sp alteration.
   mfhi    $sp                      # Error, no mask after sp alteration.
   clo     $sp, $a2                 # Error, no mask after sp alteration.
   ins     $sp, $s3, 2, 10          # Error, no mask after sp alteration.
   lui     $sp, 0x1f                # Error, no mask after sp alteration.
   mfc1    $sp, $f12                # Error, no mask after sp alteration.
   mfhc1   $sp, $f10                # Error, no mask after sp alteration.
   mflo    $sp                      # Error, no mask after sp alteration.
   movn    $sp, $v0, $v1            # Error, no mask after sp alteration.
   movt    $sp, $s4, $fcc4          # Error, no mask after sp alteration.
   mul     $sp, $s0, $s1            # Error, no mask after sp alteration.

   # Modify sp with lw instruction.

bundle_load_into_sp:
   lw      $sp, 16($a0)             # Error, no mask after sp alteration.
   nop
   lw      $sp, 16($a0)
   and     $sp, $sp, STORE_MASK     # Error, a0 register is not masked.

   # Alteration of sp with mask doesn't need a mask.

bundle_mask_doesnt_need_mask:
   nop
   and     $sp, $sp, STORE_MASK     # OK
   nop
   nop

   # A few instructions where sp is the first operand, but is not altered.

bundle_sp_not_altered:
   mthi    $sp                      # OK
   mult    $sp, $t9                 # OK
   nop
   nop

   # Alteration of sp with jalr instruction, not allowed.

bundle_jalr_sp_t9:
   nop
   and     $t9, $t9, $t6
   jalr    $sp, $t9                 # Error, alteration of sp register.
    nop                             # Instruction in delay slot is indented.

   # Changing value of sp in delay slot, not allowed.

bundle_chk_delay_slot:
   b       bundle_chk_cross_1
    addiu   $sp, $sp, -32           # Error, delay slot splits sp alteration.
   and     $sp, $sp, STORE_MASK
   nop

   # Pseudo-instruction with sp modification crosses bundle.
   # We allow this, because the potential jump at the start of bundle will
   # cause executing only mask instruction for sp, which is OK.

bundle_chk_cross_1:
   nop
   nop
   nop
   addiu   $sp, $sp, -32            # OK

bundle_chk_cross_2:
   and     $sp, $sp, STORE_MASK
   nop
   nop
   nop

   # Correct sp changes.

bundle_OK:
   add     $sp, $sp, 64             # OK
   and     $sp, $sp, STORE_MASK
   sub     $sp, $sp, 56             # OK
   and     $sp, $sp, STORE_MASK

end_of_code:
