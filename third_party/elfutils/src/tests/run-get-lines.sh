#! /bin/sh
# Copyright (C) 1999, 2000, 2002, 2004, 2005, 2013 Red Hat, Inc.
# This file is part of elfutils.
# Written by Ulrich Drepper <drepper@redhat.com>, 1999.
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

testfiles testfile testfile2 testfilenolines

testrun_compare ${abs_builddir}/get-lines testfile testfile2 <<\EOF
cuhl = 11, o = 0, asz = 4, osz = 4, ncu = 191
 5 lines
804842c: /home/drepper/gnu/new-bu/build/ttt/m.c:5:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
8048432: /home/drepper/gnu/new-bu/build/ttt/m.c:6:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
804844d: /home/drepper/gnu/new-bu/build/ttt/m.c:7:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
8048458: /home/drepper/gnu/new-bu/build/ttt/m.c:8:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
804845a: /home/drepper/gnu/new-bu/build/ttt/m.c:8:0: is_stmt:yes, end_seq:yes, bb:no, prologue:no, epilogue:no
cuhl = 11, o = 114, asz = 4, osz = 4, ncu = 5617
 4 lines
804845c: /home/drepper/gnu/new-bu/build/ttt/b.c:4:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
804845f: /home/drepper/gnu/new-bu/build/ttt/b.c:5:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
8048464: /home/drepper/gnu/new-bu/build/ttt/b.c:6:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
8048466: /home/drepper/gnu/new-bu/build/ttt/b.c:6:0: is_stmt:yes, end_seq:yes, bb:no, prologue:no, epilogue:no
cuhl = 11, o = 412, asz = 4, osz = 4, ncu = 5752
 4 lines
8048468: /home/drepper/gnu/new-bu/build/ttt/f.c:3:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
804846b: /home/drepper/gnu/new-bu/build/ttt/f.c:4:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
8048470: /home/drepper/gnu/new-bu/build/ttt/f.c:5:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
8048472: /home/drepper/gnu/new-bu/build/ttt/f.c:5:0: is_stmt:yes, end_seq:yes, bb:no, prologue:no, epilogue:no
cuhl = 11, o = 0, asz = 4, osz = 4, ncu = 2418
 4 lines
10000470: /shoggoth/drepper/b.c:4:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
1000047c: /shoggoth/drepper/b.c:5:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
10000480: /shoggoth/drepper/b.c:6:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
10000490: /shoggoth/drepper/b.c:6:0: is_stmt:yes, end_seq:yes, bb:no, prologue:no, epilogue:no
cuhl = 11, o = 213, asz = 4, osz = 4, ncu = 2521
 4 lines
10000490: /shoggoth/drepper/f.c:3:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
1000049c: /shoggoth/drepper/f.c:4:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
100004a0: /shoggoth/drepper/f.c:5:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
100004b0: /shoggoth/drepper/f.c:5:0: is_stmt:yes, end_seq:yes, bb:no, prologue:no, epilogue:no
cuhl = 11, o = 267, asz = 4, osz = 4, ncu = 2680
 5 lines
100004b0: /shoggoth/drepper/m.c:5:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
100004cc: /shoggoth/drepper/m.c:6:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
100004e8: /shoggoth/drepper/m.c:7:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
100004f4: /shoggoth/drepper/m.c:8:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
10000514: /shoggoth/drepper/m.c:8:0: is_stmt:yes, end_seq:yes, bb:no, prologue:no, epilogue:no
EOF

# - lines.c
# int ft;
#
# int
# main (int argc, char **argv)
# {
#   return ft - 42;
# }
#
# - nolines.c
# int ft = 42;
#
# gcc -g -c lines.c
# gcc -g -c nolines.c
# gcc -g -o testfilenolines lines.o nolines.o

testrun_compare ${abs_builddir}/get-lines testfilenolines <<\EOF
cuhl = 11, o = 0, asz = 8, osz = 4, ncu = 169
 4 lines
400474: /home/mark/src/tests/lines.c:5:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
40047f: /home/mark/src/tests/lines.c:6:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
400488: /home/mark/src/tests/lines.c:7:0: is_stmt:yes, end_seq:no, bb:no, prologue:no, epilogue:no
40048a: /home/mark/src/tests/lines.c:7:0: is_stmt:yes, end_seq:yes, bb:no, prologue:no, epilogue:no
cuhl = 11, o = 125, asz = 8, osz = 4, ncu = 243
 0 lines
EOF

exit 0
