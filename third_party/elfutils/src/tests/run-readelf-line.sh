#! /bin/sh
# Copyright (C) 2013 Red Hat, Inc.
# This file is part of elfutils.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# elfutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. $srcdir/test-subr.sh

# Tests readelf --debug-dump=line and --debug-dump=decodedline
# See run-readelf-aranges for testfiles.

testfiles testfilefoobarbaz

testrun_compare ${abs_top_builddir}/src/readelf --debug-dump=line testfilefoobarbaz <<EOF

DWARF section [30] '.debug_line' at offset 0x15f6:

Table at offset 0:

 Length:                     83
 DWARF version:              2
 Prologue length:            43
 Minimum instruction length: 1
 Maximum operations per instruction: 1
 Initial value if 'is_stmt': 1
 Line base:                  -5
 Line range:                 14
 Opcode base:                13

Opcodes:
  [ 1]  0 arguments
  [ 2]  1 argument
  [ 3]  1 argument
  [ 4]  1 argument
  [ 5]  1 argument
  [ 6]  0 arguments
  [ 7]  0 arguments
  [ 8]  0 arguments
  [ 9]  1 argument
  [10]  0 arguments
  [11]  0 arguments
  [12]  1 argument

Directory table:

File name table:
 Entry Dir   Time      Size      Name
 1     0     0         0         foo.c
 2     0     0         0         foobarbaz.h

Line number statements:
 [    35] extended opcode 2:  set address to 0x80482f0 <main>
 [    3c] advance line by constant 15 to 16
 [    3e] copy
 [    3f] special opcode 159: address+10 = 0x80482fa <main+0xa>, line+1 = 17
 [    40] special opcode 117: address+7 = 0x8048301 <main+0x11>, line+1 = 18
 [    41] advance line by constant -9 to 9
 [    43] special opcode 200: address+13 = 0x804830e <main+0x1e>, line+0 = 9
 [    44] special opcode 48: address+2 = 0x8048310 <main+0x20>, line+2 = 11
 [    45] special opcode 58: address+3 = 0x8048313 <main+0x23>, line-2 = 9
 [    46] special opcode 48: address+2 = 0x8048315 <main+0x25>, line+2 = 11
 [    47] special opcode 44: address+2 = 0x8048317 <main+0x27>, line-2 = 9
 [    48] advance line by constant 13 to 22
 [    4a] special opcode 46: address+2 = 0x8048319 <main+0x29>, line+0 = 22
 [    4b] advance line by constant -13 to 9
 [    4d] special opcode 60: address+3 = 0x804831c <main+0x2c>, line+0 = 9
 [    4e] advance line by constant 12 to 21
 [    50] special opcode 60: address+3 = 0x804831f <main+0x2f>, line+0 = 21
 [    51] special opcode 61: address+3 = 0x8048322 <main+0x32>, line+1 = 22
 [    52] advance address by 2 to 0x8048324
 [    54] extended opcode 1:  end of sequence

Table at offset 87:

 Length:                     72
 DWARF version:              2
 Prologue length:            28
 Minimum instruction length: 1
 Maximum operations per instruction: 1
 Initial value if 'is_stmt': 1
 Line base:                  -5
 Line range:                 14
 Opcode base:                13

Opcodes:
  [ 1]  0 arguments
  [ 2]  1 argument
  [ 3]  1 argument
  [ 4]  1 argument
  [ 5]  1 argument
  [ 6]  0 arguments
  [ 7]  0 arguments
  [ 8]  0 arguments
  [ 9]  1 argument
  [10]  0 arguments
  [11]  0 arguments
  [12]  1 argument

Directory table:

File name table:
 Entry Dir   Time      Size      Name
 1     0     0         0         bar.c

Line number statements:
 [    7d] extended opcode 2:  set address to 0x8048330 <nobar>
 [    84] advance line by constant 12 to 13
 [    86] copy
 [    87] special opcode 19: address+0 = 0x8048330 <nobar>, line+1 = 14
 [    88] advance address by 11 to 0x804833b
 [    8a] extended opcode 1:  end of sequence
 [    8d] extended opcode 2:  set address to 0x8048440 <bar>
 [    94] advance line by constant 18 to 19
 [    96] copy
 [    97] special opcode 19: address+0 = 0x8048440 <bar>, line+1 = 20
 [    98] advance line by constant -12 to 8
 [    9a] special opcode 200: address+13 = 0x804844d <bar+0xd>, line+0 = 8
 [    9b] advance line by constant 14 to 22
 [    9d] special opcode 74: address+4 = 0x8048451 <bar+0x11>, line+0 = 22
 [    9e] advance address by 1 to 0x8048452
 [    a0] extended opcode 1:  end of sequence

Table at offset 163:

 Length:                     106
 DWARF version:              2
 Prologue length:            43
 Minimum instruction length: 1
 Maximum operations per instruction: 1
 Initial value if 'is_stmt': 1
 Line base:                  -5
 Line range:                 14
 Opcode base:                13

Opcodes:
  [ 1]  0 arguments
  [ 2]  1 argument
  [ 3]  1 argument
  [ 4]  1 argument
  [ 5]  1 argument
  [ 6]  0 arguments
  [ 7]  0 arguments
  [ 8]  0 arguments
  [ 9]  1 argument
  [10]  0 arguments
  [11]  0 arguments
  [12]  1 argument

Directory table:

File name table:
 Entry Dir   Time      Size      Name
 1     0     0         0         baz.c
 2     0     0         0         foobarbaz.h

Line number statements:
 [    d8] extended opcode 2:  set address to 0x8048340 <nobaz>
 [    df] advance line by constant 12 to 13
 [    e1] copy
 [    e2] special opcode 19: address+0 = 0x8048340 <nobaz>, line+1 = 14
 [    e3] advance address by 11 to 0x804834b
 [    e5] extended opcode 1:  end of sequence
 [    e8] extended opcode 2:  set address to 0x8048460 <baz>
 [    ef] advance line by constant 18 to 19
 [    f1] copy
 [    f2] special opcode 74: address+4 = 0x8048464 <baz+0x4>, line+0 = 19
 [    f3] special opcode 75: address+4 = 0x8048468 <baz+0x8>, line+1 = 20
 [    f4] extended opcode 4:  set discriminator to 1
 [    f8] special opcode 78: address+4 = 0x804846c <baz+0xc>, line+4 = 24
 [    f9] special opcode 187: address+12 = 0x8048478 <baz+0x18>, line+1 = 25
 [    fa] special opcode 87: address+5 = 0x804847d <baz+0x1d>, line-1 = 24
 [    fb] special opcode 61: address+3 = 0x8048480 <baz+0x20>, line+1 = 25
 [    fc] special opcode 101: address+6 = 0x8048486 <baz+0x26>, line-1 = 24
 [    fd] special opcode 61: address+3 = 0x8048489 <baz+0x29>, line+1 = 25
 [    fe] special opcode 87: address+5 = 0x804848e <baz+0x2e>, line-1 = 24
 [    ff] advance line by constant -16 to 8
 [   101] special opcode 46: address+2 = 0x8048490 <baz+0x30>, line+0 = 8
 [   102] advance line by constant 20 to 28
 [   104] special opcode 186: address+12 = 0x804849c <baz+0x3c>, line+0 = 28
 [   105] advance line by constant -20 to 8
 [   107] special opcode 88: address+5 = 0x80484a1 <baz+0x41>, line+0 = 8
 [   108] advance line by constant 13 to 21
 [   10a] advance address by constant 17 to 0x80484b2 <baz+0x52>
 [   10b] special opcode 32: address+1 = 0x80484b3 <baz+0x53>, line+0 = 21
 [   10c] advance address by 9 to 0x80484bc
 [   10e] extended opcode 1:  end of sequence
EOF

testrun_compare ${abs_top_builddir}/src/readelf --debug-dump=decodedline testfilefoobarbaz <<\EOF

DWARF section [30] '.debug_line' at offset 0x15f6:

 CU [b] foo.c
  line:col SBPE* disc isa op address (Statement Block Prologue Epilogue *End)
  /home/mark/src/tests/foobarbaz/foo.c (mtime: 0, length: 0)
    16:0   S        0   0  0 0x080482f0 <main>
    17:0   S        0   0  0 0x080482fa <main+0xa>
    18:0   S        0   0  0 0x08048301 <main+0x11>
     9:0   S        0   0  0 0x0804830e <main+0x1e>
    11:0   S        0   0  0 0x08048310 <main+0x20>
     9:0   S        0   0  0 0x08048313 <main+0x23>
    11:0   S        0   0  0 0x08048315 <main+0x25>
     9:0   S        0   0  0 0x08048317 <main+0x27>
    22:0   S        0   0  0 0x08048319 <main+0x29>
     9:0   S        0   0  0 0x0804831c <main+0x2c>
    21:0   S        0   0  0 0x0804831f <main+0x2f>
    22:0   S        0   0  0 0x08048322 <main+0x32>
    22:0   S   *    0   0  0 0x08048323 <main+0x33>

 CU [141] bar.c
  line:col SBPE* disc isa op address (Statement Block Prologue Epilogue *End)
  /home/mark/src/tests/foobarbaz/bar.c (mtime: 0, length: 0)
    13:0   S        0   0  0 0x08048330 <nobar>
    14:0   S        0   0  0 0x08048330 <nobar>
    14:0   S   *    0   0  0 0x0804833a <nobar+0xa>

    19:0   S        0   0  0 0x08048440 <bar>
    20:0   S        0   0  0 0x08048440 <bar>
     8:0   S        0   0  0 0x0804844d <bar+0xd>
    22:0   S        0   0  0 0x08048451 <bar+0x11>
    22:0   S   *    0   0  0 0x08048451 <bar+0x11>

 CU [1dc] baz.c
  line:col SBPE* disc isa op address (Statement Block Prologue Epilogue *End)
  /home/mark/src/tests/foobarbaz/baz.c (mtime: 0, length: 0)
    13:0   S        0   0  0 0x08048340 <nobaz>
    14:0   S        0   0  0 0x08048340 <nobaz>
    14:0   S   *    0   0  0 0x0804834a <nobaz+0xa>

    19:0   S        0   0  0 0x08048460 <baz>
    19:0   S        0   0  0 0x08048464 <baz+0x4>
    20:0   S        0   0  0 0x08048468 <baz+0x8>
    24:0   S        1   0  0 0x0804846c <baz+0xc>
    25:0   S        0   0  0 0x08048478 <baz+0x18>
    24:0   S        0   0  0 0x0804847d <baz+0x1d>
    25:0   S        0   0  0 0x08048480 <baz+0x20>
    24:0   S        0   0  0 0x08048486 <baz+0x26>
    25:0   S        0   0  0 0x08048489 <baz+0x29>
    24:0   S        0   0  0 0x0804848e <baz+0x2e>
     8:0   S        0   0  0 0x08048490 <baz+0x30>
    28:0   S        0   0  0 0x0804849c <baz+0x3c>
     8:0   S        0   0  0 0x080484a1 <baz+0x41>
    21:0   S        0   0  0 0x080484b3 <baz+0x53>
    21:0   S   *    0   0  0 0x080484bb <baz+0x5b>

EOF

exit 0
