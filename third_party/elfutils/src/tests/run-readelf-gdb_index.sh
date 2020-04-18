#! /bin/sh
# Copyright (C) 2012 Red Hat, Inc.
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

# common.h
# struct foo
# {
#   const char *bar;
# };
#
# extern char *global;
# int say (struct foo *prefix);

# hello.c
# #include "common.h"
#
# static char *hello = "Hello";
#
# int
# main (int argc, char **argv)
# {
#   struct foo baz;
#   global = hello;
#   baz.bar = global;
#   return say(&baz);
# }

# world.c
# #include "common.h"
#
# char *global;
#
# static int hello (const char *bar)
# {
#   return bar == global;
# }
#
# int
# say (struct foo *prefix)
# {
#   return hello (prefix->bar);
# }

# gcc -g -fdebug-types-section -c hello.c
# gcc -g -fdebug-types-section -c world.c
# gcc -g -fdebug-types-section -o testfilegdbindex7 hello.o world.o
# gdb testfilegdbindex7
# (gdb) save gdb-index .
# objcopy --add-section .gdb_index=testfilegdbindex7.gdb-index --set-section-flags .gdb_index=readonly testfilegdbindex7 testfilegdbindex7

testfiles testfilegdbindex5 testfilegdbindex7

testrun_compare ${abs_top_builddir}/src/readelf --debug-dump=gdb_index testfilegdbindex5 <<\EOF

GDB section [33] '.gdb_index' at offset 0xe76 contains 8383 bytes :
 Version:         5
 CU offset:       0x18
 TU offset:       0x38
 address offset:  0x50
 symbol offset:   0x78
 constant offset: 0x2078

 CU list at offset 0x18 contains 2 entries:
 [   0] start: 00000000, length:   184
 [   1] start: 0x0000b8, length:   204

 TU list at offset 0x38 contains 1 entries:
 [   0] CU offset:     0, type offset:    29, signature: 0x87e03f92cc37cdf0

 Address list at offset 0x50 contains 2 entries:
 [   0] 0x000000000040049c <main>..0x00000000004004d1 <main+0x35>, CU index:     0
 [   1] 0x00000000004004d4 <hello>..0x000000000040050b <say+0x1c>, CU index:     1

 Symbol table at offset 0x50 contains 1024 slots:
 [ 123] symbol: global, CUs: 1
 [ 489] symbol: main, CUs: 0
 [ 518] symbol: char, CUs: 0
 [ 661] symbol: foo, CUs: 0T
 [ 741] symbol: hello, CUs: 0, 1
 [ 746] symbol: say, CUs: 1
 [ 754] symbol: int, CUs: 0
EOF

testrun_compare ${abs_top_builddir}/src/readelf --debug-dump=gdb_index testfilegdbindex7 <<\EOF

GDB section [33] '.gdb_index' at offset 0xe76 contains 8399 bytes :
 Version:         7
 CU offset:       0x18
 TU offset:       0x38
 address offset:  0x50
 symbol offset:   0x78
 constant offset: 0x2078

 CU list at offset 0x18 contains 2 entries:
 [   0] start: 00000000, length:   184
 [   1] start: 0x0000b8, length:   204

 TU list at offset 0x38 contains 1 entries:
 [   0] CU offset:     0, type offset:    29, signature: 0x87e03f92cc37cdf0

 Address list at offset 0x50 contains 2 entries:
 [   0] 0x000000000040049c <main>..0x00000000004004d1 <main+0x35>, CU index:     0
 [   1] 0x00000000004004d4 <hello>..0x000000000040050b <say+0x1c>, CU index:     1

 Symbol table at offset 0x50 contains 1024 slots:
 [ 123] symbol: global, CUs: 1 (var:G)
 [ 489] symbol: main, CUs: 0 (func:G)
 [ 518] symbol: char, CUs: 0 (type:S)
 [ 661] symbol: foo, CUs: 0T (type:S)
 [ 741] symbol: hello, CUs: 0 (var:S), 1 (func:S)
 [ 746] symbol: say, CUs: 1 (func:G)
 [ 754] symbol: int, CUs: 0 (type:S)
EOF

exit 0
