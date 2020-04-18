#! /bin/sh
# Copyright (C) 1999, 2000, 2002, 2004, 2005, 2007 Red Hat, Inc.
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

testfiles testfile testfile2

testrun_compare ${abs_builddir}/get-files testfile testfile2 <<\EOF
cuhl = 11, o = 0, asz = 4, osz = 4, ncu = 191
 dirs[0] = "/home/drepper/gnu/new-bu/build/ttt"
 file[0] = "???"
 file[1] = "/home/drepper/gnu/new-bu/build/ttt/m.c"
cuhl = 11, o = 114, asz = 4, osz = 4, ncu = 5617
 dirs[0] = "/home/drepper/gnu/new-bu/build/ttt"
 file[0] = "???"
 file[1] = "/home/drepper/gnu/new-bu/build/ttt/b.c"
 file[2] = "/usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h"
 file[3] = "/usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h"
 file[4] = "/usr/include/bits/types.h"
 file[5] = "/usr/include/bits/sched.h"
 file[6] = "/usr/include/bits/pthreadtypes.h"
 file[7] = "/usr/include/stdio.h"
 file[8] = "/usr/include/libio.h"
 file[9] = "/usr/include/wchar.h"
 file[10] = "/usr/include/_G_config.h"
 file[11] = "/usr/include/gconv.h"
cuhl = 11, o = 412, asz = 4, osz = 4, ncu = 5752
 dirs[0] = "/home/drepper/gnu/new-bu/build/ttt"
 file[0] = "???"
 file[1] = "/home/drepper/gnu/new-bu/build/ttt/f.c"
cuhl = 11, o = 0, asz = 4, osz = 4, ncu = 2418
 dirs[0] = "/shoggoth/drepper"
 file[0] = "???"
 file[1] = "/shoggoth/drepper/b.c"
 file[2] = "/home/geoffk/objs/laurel-000912-branch/lib/gcc-lib/powerpc-unknown-linux-gnu/2.96-laurel-000912/include/stddef.h"
 file[3] = "/home/geoffk/objs/laurel-000912-branch/lib/gcc-lib/powerpc-unknown-linux-gnu/2.96-laurel-000912/include/stdarg.h"
 file[4] = "/shoggoth/drepper/<built-in>"
 file[5] = "/usr/include/bits/types.h"
 file[6] = "/usr/include/stdio.h"
 file[7] = "/usr/include/libio.h"
 file[8] = "/usr/include/_G_config.h"
cuhl = 11, o = 213, asz = 4, osz = 4, ncu = 2521
 dirs[0] = "/shoggoth/drepper"
 file[0] = "???"
 file[1] = "/shoggoth/drepper/f.c"
cuhl = 11, o = 267, asz = 4, osz = 4, ncu = 2680
 dirs[0] = "/shoggoth/drepper"
 file[0] = "???"
 file[1] = "/shoggoth/drepper/m.c"
EOF

exit 0
