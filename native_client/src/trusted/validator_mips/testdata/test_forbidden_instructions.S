# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# Tests a few instructions that are always illegal in NaCl.
#
  .align 4
  .globl __start
__start:
  .set mips32r2

  eret               # Error, (forbidden) unsafe instruction.

  li        $v0, 1   # System call code for print_int.
  la        $a0, 20  # Integer for printing.
  syscall            # Error, this is forbidden in untrusted code.

  break              # Error, unsafe instruction.

  di                 # Error, unsafe instruction.
  ei                 # Error, unsafe instruction.
  wait               # Error, unsafe instruction.
  nop
  deret              # Error, unsafe instruction.

  teqi      $t1, 0
  tnei      $t1, 0
  tgei      $t1, 0
  tgeiu     $t1, 0
  tlti      $t1, 0
  tltiu     $t1, 0

  teq       $t1, $zero
  tne       $t1, $t2
  tge       $t1, $t2
  tgeu      $t1, $t2
  tlt       $t1, $t2
  tltu      $t1, $t2



end_of_code:
