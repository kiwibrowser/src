# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Several test cases are implemented in this file:
#  1. Test if regular way to access thread pointers is allowed.
#  2. Test different random examples of forbidden stores through $t8.
#  3. Test different random examples of forbidden loads through $t8.


   .align 4
   .globl __start
__start:
   .set noreorder
   .set mips32r2

   # Test valid tp access
valid_load_access:
   lw      $a0, 0($t8)            # OK
   lw      $a0, 4($t8)            # OK
   nop
   nop

   # Test invalid tp access
invalid_store_access:
   sb      $a0, 4($t8)            # Error, forbidden thread pointer access.
   sc      $a0, 4($t8)            # Error, forbidden thread pointer access.
   sdc1    $0, 4($t8)             # Error, forbidden thread pointer access.
   sdc2    $8, 4($t8)             # Error, forbidden thread pointer access.

   sh      $a0, 4($t8)            # Error, forbidden thread pointer access.
   sw      $a0, 4($t8)            # Error, forbidden thread pointer access.
   swc1    $f2, 4($t8)            # Error, forbidden thread pointer access.
   swc2    $22, 4($t8)            # Error, forbidden thread pointer access.

   swl     $a0, 4($t8)            # Error, forbidden thread pointer access.
   swr     $a0, 4($t8)            # Error, forbidden thread pointer access.
   sdxc1   $f6, $a2($t8)          # Error, forbidden instruction.
   suxc1   $f4, $t8($t8)          # Error, forbidden instruction.

   swxc1   $f4, $t8($t8)          # Error, forbidden instruction.
   nop
   nop
   nop

invalid_load_access:
   lb      $a0, 4($t8)            # Error, forbidden thread pointer access.
   lw      $a0, 8($t8)            # Error, forbidden thread pointer access.
   lbu     $a0, 4($t8)            # Error, forbidden thread pointer access.
   ldc1    $f0, 4($t8)            # Error, forbidden thread pointer access.

   ldc2    $10, -24($t8)          # Error, forbidden thread pointer access.
   lh      $a0, 4($t8)            # Error, forbidden thread pointer access.
   lhu     $a0, 4($t8)            # Error, forbidden thread pointer access.
   ll      $a0, 4($t8)            # Error, forbidden thread pointer access.

   lwc1    $f0, 4($t8)            # Error, forbidden thread pointer access.
   lwc2    $10, 4($t8)            # Error, forbidden thread pointer access.
   lwl     $a0, 4($t8)            # Error, forbidden thread pointer access.
   lwr     $a0, 4($t8)            # Error, forbidden thread pointer access.

   ldxc1   $f0, $0($t8)           # Error, forbidden instruction.
   luxc1   $f0, $a2($t8)          # Error, forbidden instruction.
   lwxc1   $f6, $v0($t8)          # Error, forbidden instruction.
   nop

   .set dsp
   lbux    $a3, $t2($t8)          # Error, forbidden instruction.
   lhx     $a1, $a2($t8)          # Error, forbidden instruction.
   lwx     $t1, $t2($t8)          # Error, forbidden instruction.
   nop
end_of_code:
