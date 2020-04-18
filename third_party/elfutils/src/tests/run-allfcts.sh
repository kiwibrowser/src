#! /bin/sh
# Copyright (C) 2005, 2013 Red Hat, Inc.
# This file is part of elfutils.
# Written by Ulrich Drepper <drepper@redhat.com>, 2005.
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

testfiles testfile testfile2 testfile8

testrun_compare ${abs_builddir}/allfcts testfile testfile2 testfile8 <<\EOF
/home/drepper/gnu/new-bu/build/ttt/m.c:5:main
/home/drepper/gnu/new-bu/build/ttt/b.c:4:bar
/home/drepper/gnu/new-bu/build/ttt/f.c:3:foo
/shoggoth/drepper/b.c:4:bar
/shoggoth/drepper/f.c:3:foo
/shoggoth/drepper/m.c:5:main
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:107:main
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:159:print_version
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:173:parse_opt
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:201:more_help
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:217:process_file
/usr/include/sys/stat.h:375:stat64
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:291:crc32_file
/home/drepper/gnu/elfutils/build/src/../../src/strip.c:313:handle_elf
EOF

# = nested_funcs.c =
#
# static int
# foo (int x)
# {
#   int bar (int y)
#   {
#     return x - y;
#   }
# 
#   return bar (x * 2);
# }
#
# int
# main (int argc, char ** argv)
# {
#   return foo (argc);
# }
#
# gcc -g -o nested_funcs nested_funcs.c

# = class_func.cxx =
#
# namespace foobar
# {
#   class Foo
#   {
#   public:
#     int bar(int x);
#   };
#
#   int Foo::bar(int x) { return x - 42; }
# };
#
# int
# main (int argc, char **argv)
# {
#   foobar::Foo foo;
#
#   return foo.bar (42);
# }
#
# clang++ -g -o class_func class_func.cxx

testfiles testfile_nested_funcs testfile_class_func

testrun_compare ${abs_builddir}/allfcts testfile_nested_funcs testfile_class_func <<\EOF
/home/mark/src/tests/nested/nested_funcs.c:2:foo
/home/mark/src/tests/nested/nested_funcs.c:4:bar
/home/mark/src/tests/nested/nested_funcs.c:13:main
/home/mark/src/tests/nested/class_func.cxx:6:bar
/home/mark/src/tests/nested/class_func.cxx:13:main
EOF

exit 0
