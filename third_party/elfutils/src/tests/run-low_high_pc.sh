#! /bin/sh
# Copyright (C) 2005 Red Hat, Inc.
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

# int
# main (int argc, char **argv)
# {
#   return 0;
# }
# gcc -g -o main main.c
testfiles testfile_low_high_pc

testrun ${abs_builddir}/low_high_pc -e ./testfile_low_high_pc
testrun ${abs_builddir}/low_high_pc -e ${abs_builddir}/low_high_pc
testrun ${abs_builddir}/low_high_pc -e ${abs_top_builddir}/src/strip
testrun ${abs_builddir}/low_high_pc -e ${abs_top_builddir}/src/strip.o
testrun ${abs_builddir}/low_high_pc -e ${abs_top_builddir}/libelf/libelf.so

exit 0
