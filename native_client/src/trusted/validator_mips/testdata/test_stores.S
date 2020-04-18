# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Several test cases are implemented in this file:
#  1. Test if all store instructions are recognized as such.
#  2. Test if store from sp is allowed.
#  3. Test if mask and instruction are split with delay slot.
#  4. Test if mask and instruction are in the same bundle.

#include "native_client/pnacl/support/nacl_mips_defs.h"

  .align 4
  .globl __start
__start:
  .set noreorder
  .set mips32r2

  # Check store with mask.

bundle_store_mask:
  and     $a3, $a3, STORE_MASK
  sw      $s1, 128($a3)          # OK
  and     $a3, $a3, $t8
  sw      $s1, 128($a3)          # Error, incorrect mask.

  # Store instructions without store mask, should report error.

bundle_store_list:
  sb      $a0, 16($a1)           # Error, unsafe store, no mask.
  sc      $a0, 16($a1)           # Error, unsafe store, no mask.
  sdc1    $0, 16($a1)            # Error, unsafe store, no mask.
  sdc2    $8, 16($a1)            # Error, unsafe store, no mask.

  sh      $a0, 16($a1)           # Error, unsafe store, no mask.
  sw      $a0, 16($a1)           # Error, unsafe store, no mask.
  swc1    $f2, 16($a1)           # Error, unsafe store, no mask.
  swc2    $22, 16($a1)           # Error, unsafe store, no mask.

  swl     $a0, 16($a1)           # Error, unsafe store, no mask.
  swr     $a0, 16($a1)           # Error, unsafe store, no mask.
  nop
  nop

  # A few forbidden instructions (indexed store).

bundle_indexed_store:
  sdxc1   $f6, $a2($a0)          # Error, forbidden instruction.
  suxc1   $f4, $t8($a1)          # Error, forbidden instruction.
  swxc1   $f4, $t8($a1)          # Error, forbidden instruction.
  nop

bundle_store_from_sp:
  sw      $a0, 16($sp)           # OK - store from sp doesn't need a mask.
  nop
  nop
  nop

  # Test when store pseudo-instruction is split with delay slot.

bundle_chk_delay_slot:
  b       bundle_chk_bundle_cross_1
   and    $s0, $s0, STORE_MASK
  sw      $v0, 16($s0)           # OK
  nop

  # Test when store pseudo-instruction crosses bundle.

bundle_chk_bundle_cross_1:
  nop
  nop
  nop
  and     $a3, $a3, STORE_MASK

bundle_chk_bundle_cross_2:
  sw      $a3, 16($a3)           # Error, pseudo-instruction crosses bundle.
  nop
  nop
  nop

  # Test indexed store with store mask.

bundle_indexed_store_index_not_zero:
  and     $a0, $a0, STORE_MASK
  sdxc1   $f6, $a2($a0)          # Error, forbidden instruction.
  nop
  nop

  # Test if base register is properly recognized.

bundle_sp_1:
  sw      $sp,0($t1)             # Error, no store mask for t1.
  and     $sp,$sp,STORE_MASK
  nop
  nop

end_of_code:
