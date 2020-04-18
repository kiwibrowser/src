# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# Tests alteration of value stored in read-only registers.
#

#include "native_client/pnacl/support/nacl_mips_defs.h"

   .align 4
   .globl __start
__start:
   .set noreorder
   .set mips32r2
   .set TLS_REG, $t8

   # Testing instructions that alter t6 (JUMP_MASK).

bundle_altering_t6:
   addiu   JUMP_MASK, JUMP_MASK, 64       # Error, changing read-only register.
   and     JUMP_MASK, JUMP_MASK, $a2      # Error, changing read-only register.
   lui     JUMP_MASK, 0x21                # Error, changing read-only register.
   clo     JUMP_MASK, $a2                 # Error, changing read-only register.
   ins     JUMP_MASK, $s3, 2, 10          # Error, changing read-only register.
   lui     JUMP_MASK, 0x1f                # Error, changing read-only register.
   mfc1    JUMP_MASK, $f12                # Error, changing read-only register.
   mfhc1   JUMP_MASK, $f10                # Error, changing read-only register.
   mflo    JUMP_MASK                      # Error, changing read-only register.
   movn    JUMP_MASK, $v0, $v1            # Error, changing read-only register.
   movt    JUMP_MASK, $s4, $fcc4          # Error, changing read-only register.
   mul     JUMP_MASK, $s0, $s1            # Error, changing read-only register.

   # Testing instructions that alter t7 (STORE_MASK).

bundle_altering_t7:
   addiu   STORE_MASK, STORE_MASK, 64     # Error, changing read-only register.
   and     STORE_MASK, STORE_MASK, $a2    # Error, changing read-only register.
   lui     STORE_MASK, 0x21               # Error, changing read-only register.
   clo     STORE_MASK, $a2                # Error, changing read-only register.

   # Testing instructions that alter t8 (TLS index).

bundle_altering_t8:
   lui     TLS_REG, 0x1f                  # Error, changing read-only register.
   mfc1    TLS_REG, $f12                  # Error, changing read-only register.
   mfhc1   TLS_REG, $f10                  # Error, changing read-only register.
   mflo    TLS_REG                        # Error, changing read-only register.

bundle_jalr_sp_t9:
   nop
   and     $t9, $t9, JUMP_MASK
   jalr    STORE_MASK, $t9                # Error, changing read-only register.
    nop                                   # Instr in delay slot is indented.

   # Make sure $f14 is not interpreted
   # as $t6 (14), when in fp instruction.

bundle_fp_instr:
   add.s   $f14, $f4, $f2
   mov.d   $f14, $f0
   neg.d   $f14, $f4
   lwc1    $f14,24($sp)

   # A few instructions where $t6/$t7
   # is the first operand, but its value is not altered.

bundle_t6_t7_not_altered:
   mthi    JUMP_MASK                      # OK
   mult    STORE_MASK, $t9                # OK
   nop
   nop

end_of_code:
