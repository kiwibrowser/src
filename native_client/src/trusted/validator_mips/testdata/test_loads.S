# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Several test cases are implemented in this file:
# 1. Test if all load instructions are recognized as such.
# 2. Test if load from sp is allowed.
# 3. Test if mask and load instruction are split with delay slot.
# 4. Test if mask and load instruction are in the same bundle.

#include "native_client/pnacl/support/nacl_mips_defs.h"

   .align 4
   .globl __start
__start:
   .set noreorder
   .set mips32r2

   # Check masked load.

   and     $a0, $a0, STORE_MASK
   lw      $v0, 16($a0)               # OK
   nop
   nop

   # Test if all load instructions are recognized as such.

bundle_load_instrs:
   lb      $a0, 4($a1)                # Error, unsafe load, no mask.
   lw      $a0, 4($a1)                # Error, unsafe load, no mask.
   lbu     $a0, 4($a1)                # Error, unsafe load, no mask.
   ldc1    $f0, 4($a1)                # Error, unsafe load, no mask.

   ldc2    $10, -24($a1)              # Error, unsafe load, no mask.
   lh      $a0, 4($a1)                # Error, unsafe load, no mask.
   lhu     $a0, 4($a1)                # Error, unsafe load, no mask.
   ll      $a0, 4($a1)                # Error, unsafe load, no mask.

   lw      $a0, 4($a1)                # Error, unsafe load, no mask.
   lwc1    $f0, 4($a1)                # Error, unsafe load, no mask.
   lwc2    $10, 4($a1)                # Error, unsafe load, no mask.
   lwl     $a0, 4($a1)                # Error, unsafe load, no mask.

   lwr     $a0, 4($a1)                # Error, unsafe load, no mask.
   ldxc1   $f0, $0($a1)               # Forbidden instruction, unsafe load.
   luxc1   $f0, $a2($a1)              # Forbidden instruction, unsafe load.
   lwxc1   $f6, $v0($a1)              # Forbidden instruction, unsafe load.

   # Test if load from sp is allowed.

bundle_load_from_sp:
   lw      $v0, 16($sp)               # OK
   nop
   and     $t2, $t2, STORE_MASK
   lw      $t1, 16($t2)               # OK

   # Test when load pseudo-instruction is split with delay slot.

bundle_chk_delay_slot:
   b       bundle_chk_bundle_cross_1
    and    $s0, $s0, STORE_MASK       # OK
   lw      $a0, 16($s0)
   nop

   # Test when load pseudo-instruction crosses bundle.

bundle_chk_bundle_cross_1:
   nop
   nop
   nop
   and     $a3, $a3, STORE_MASK


bundle_chk_bundle_cross_2:
   lw      $a3, 16($a3)               # Error, pattern crosses bundle.
   nop
   nop
   nop

   .set dsp
   lbux    $a3, $t2($t3)              # Error, forbidden instruction.
   lhx     $a1, $a2($a3)              # Error, forbidden instruction.
   lwx     $t1, $t2($t3)              # Error, forbidden instruction.
   nop

end_of_code:
