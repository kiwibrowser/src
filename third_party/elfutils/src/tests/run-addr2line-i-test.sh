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

# // g++ x.cpp -g -fPIC -olibx.so -shared -O3 -fvisibility=hidden
#
# void foobar()
# {
#   __asm__ ( "nop" ::: );
# }
#
# void fubar()
# {
#   __asm__ ( "nop" ::: );
# }
#
# void bar()
# {
#   foobar();
# }
#
# void baz()
# {
#   fubar();
# }
#
# void foo()
# {
#   bar();
#   baz();
# }
#
# void fu()
# {
#   __asm__ ( "nop" ::: );
#   fubar();
#   foobar();
# }

testfiles testfile-inlines

testrun_compare ${abs_top_builddir}/src/addr2line -i -e testfile-inlines 0x00000000000005a0 <<\EOF
/tmp/x.cpp:5
EOF

testrun_compare ${abs_top_builddir}/src/addr2line -i -e testfile-inlines 0x00000000000005a1 <<\EOF
/tmp/x.cpp:6
EOF

testrun_compare ${abs_top_builddir}/src/addr2line -i -e testfile-inlines 0x00000000000005b0 <<\EOF
/tmp/x.cpp:10
EOF

testrun_compare ${abs_top_builddir}/src/addr2line -i -e testfile-inlines 0x00000000000005b1 <<\EOF
/tmp/x.cpp:11
EOF

testrun_compare ${abs_top_builddir}/src/addr2line -i -e testfile-inlines 0x00000000000005c0 <<\EOF
/tmp/x.cpp:5
/tmp/x.cpp:15
EOF

testrun_compare ${abs_top_builddir}/src/addr2line -i -e testfile-inlines 0x00000000000005d0 <<\EOF
/tmp/x.cpp:10
/tmp/x.cpp:20
EOF

testrun_compare ${abs_top_builddir}/src/addr2line -i -e testfile-inlines 0x00000000000005e0 <<\EOF
/tmp/x.cpp:5
/tmp/x.cpp:15
/tmp/x.cpp:25
EOF

testrun_compare ${abs_top_builddir}/src/addr2line -i -e testfile-inlines 0x00000000000005e1 <<\EOF
/tmp/x.cpp:10
/tmp/x.cpp:20
/tmp/x.cpp:26
EOF

testrun_compare ${abs_top_builddir}/src/addr2line -i -e testfile-inlines 0x00000000000005f1 <<\EOF
/tmp/x.cpp:10
/tmp/x.cpp:32
EOF

testrun_compare ${abs_top_builddir}/src/addr2line -i -e testfile-inlines 0x00000000000005f2 <<\EOF
/tmp/x.cpp:5
/tmp/x.cpp:33
EOF

# All together now (plus function names).
testrun_compare ${abs_top_builddir}/src/addr2line -f -i -e testfile-inlines 0x00000000000005a0 0x00000000000005a1 0x00000000000005b0 0x00000000000005b1 0x00000000000005c0 0x00000000000005d0 0x00000000000005e0 0x00000000000005e1 0x00000000000005f1 0x00000000000005f2 <<\EOF
foobar
/tmp/x.cpp:5
foobar
/tmp/x.cpp:6
fubar
/tmp/x.cpp:10
fubar
/tmp/x.cpp:11
foobar inlined at /tmp/x.cpp:15 in _Z3barv
/tmp/x.cpp:5
bar
/tmp/x.cpp:15
fubar inlined at /tmp/x.cpp:20 in _Z3bazv
/tmp/x.cpp:10
baz
/tmp/x.cpp:20
foobar inlined at /tmp/x.cpp:15 in _Z3foov
/tmp/x.cpp:5
bar
/tmp/x.cpp:15
_Z3foov
/tmp/x.cpp:25
fubar inlined at /tmp/x.cpp:20 in _Z3foov
/tmp/x.cpp:10
baz
/tmp/x.cpp:20
_Z3foov
/tmp/x.cpp:26
fubar inlined at /tmp/x.cpp:32 in _Z2fuv
/tmp/x.cpp:10
_Z2fuv
/tmp/x.cpp:32
foobar inlined at /tmp/x.cpp:33 in _Z2fuv
/tmp/x.cpp:5
_Z2fuv
/tmp/x.cpp:33
EOF

exit 0
