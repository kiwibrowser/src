# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Test cases for jumps/branches.
# There are several kinds of jumps/branches regarding destination address:
# 1. Jumps into 0-0x10000, null guard region, where every address is OK.
# 2. Jumps into 0x10000-0x20000, trampoline code section, destination address
#    needs to be bundle aligned.
# 3. Jumps into code section, all addresses are OK with additional check whether
#    a branch target address is inside of a pseudo-instruction.
# Besides destination address we also check the position of branch/jump-and-link
# instructions, which need to be at bundle offset +8.

   .align 4
   .globl __start
__start:
   .set noreorder

   # Branches to null guard region, everything is OK.

bundle_b_to_0_0x1000:
   b   __start - 0x10010   # Destination address is 0xfff0 - OK.
    nop                    # Instruction in the delay slot is always indented.
   b   __start - 0x1fff4   # Destination address is 0xc - OK.
    nop

   # Branches to trampoline code section, must be bundle aligned.

bundle_b_to_0x1000_0x20000:
   b   __start - 0x10000   # OK - trampoline start.
    nop
   b   __start - 0xfff0    # OK - bundle aligned jump into trampoline.
    nop
   b   __start - 0xfff4    # Error, destination address is 0x1000c, which is
    nop                    # is not bundle aligned.
   nop
   nop

   # Branches to code area.

bundle_b_to_code_area:
   b   __start + 0x1000    # OK
    nop
   b   __start + 0x1004    # OK
    nop
   b   __start + 0x10      # OK
    nop
   b   end_of_code         # OK
    nop

   # Both jumps are OK, because destinations are in guard region.

bundle_j_to_0_0x1000:
   j       0x0
    nop
   j       0x4
    nop

   # Direct jumps to trampoline area, must be bundle aligned.

bundle_j_trampoline_area:
   j       0x10000
    nop
   jal     0x10004         # Error - 0x10004 is not a trampoline start.
    nop

   nop
   nop
   jal     0x10010         # OK - not a trampoline start, but there is a halt.
    nop

   nop
   nop
   jal     0x10020         # OK
    nop

   # Jumps with negative argument.

bundle_j_negative:
   j       -0x4            # Dest. address 0xffffffc, inside code region - OK.
    nop
   j       -0x10           # Dest. address 0xffffff0, inside code region - OK.
    nop

bundle_1st:
   and     $a0, $a0, $t7
bundle_2nd:
   sw      $ra, 24($a0)
   addiu   $v0, $v0, 4
   addiu   $v0, $v1, 0

bl_check:
   nop
   nop
   bal     bundle_1st      # OK
    nop

   # Branch inside a pseudo-instruction.

bundle_check_b_dest_addr:
   b       bundle_1st      # OK
    nop
   b       bundle_2nd      # Error, branch at the middle of pseudo-instruction.
    nop

   # Test if branch and link is located at bundle offset 8.

   jal     0x10000         # Error, misaligned call.
    nop
   jal     0x10000         # OK
    nop

   # Test if jump and link is located at bundle offset 8.

   jal     0x10000         # Error, misaligned call.
    nop
   jal     0x3fffff0       # OK
    nop

   # Another branch instruction with the same target address.
   # Check if this one is also reported.

   b       bundle_2nd      # Error, branch at middle of pseudo-instruction.
    nop

   # Test forbidden instruction.

   jalx    0x3ffffff0      # Error, forbidden instruction.
   nop

end_of_code:
