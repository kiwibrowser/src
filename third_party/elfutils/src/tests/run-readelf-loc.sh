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

# - hello.c
# int say (const char *prefix);
#
# static char *
# subject (char *word, int count)
# {
#   return count > 0 ? word : (word + count);
# }
#
# int
# main (int argc, char **argv)
# {
#    return say (subject (argv[0], argc));
# }
#
# - world.c
# static int
# sad (char c)
# {
#   return c > 0 ? c : c + 1;
# }
#
# static int
# happy (const char *w)
# {
#   return sad (w[1]);
# }
#
# int
# say (const char *prefix)
# {
#   const char *world = "World";
#   return prefix ? sad (prefix[0]) : happy (world);
# }
#
# gcc -g -O2 -c hello.c
# gcc -g -O2 -c world.c
# gcc -g -o testfileloc hello.o world.o

testfiles testfileloc

# Process values as offsets from base addresses and resolve to symbols.
testrun_compare ${abs_top_builddir}/src/readelf --debug-dump=loc --debug-dump=ranges \
  testfileloc<<\EOF

DWARF section [33] '.debug_loc' at offset 0xd2a:
 [     0]  0x0000000000400480 <main>..0x000000000040048d <main+0xd> [   0] reg5
 [    23]  0x0000000000400485 <main+0x5>..0x000000000040048d <main+0xd> [   0] reg5
 [    46]  0x00000000004004b2 <say+0x12>..0x00000000004004ba <say+0x1a> [   0] breg5 0

DWARF section [34] '.debug_ranges' at offset 0xd94:
 [     0]  0x0000000000400480 <main>..0x0000000000400482 <main+0x2>
           0x0000000000400485 <main+0x5>..0x000000000040048d <main+0xd>
 [    30]  0x00000000004004ad <say+0xd>..0x00000000004004af <say+0xf>
           0x00000000004004b2 <say+0x12>..0x00000000004004ba <say+0x1a>
EOF

# Don't resolve addresses to symbols.
testrun_compare ${abs_top_builddir}/src/readelf -N --debug-dump=loc --debug-dump=ranges \
  testfileloc<<\EOF

DWARF section [33] '.debug_loc' at offset 0xd2a:
 [     0]  0x0000000000400480..0x000000000040048d [   0] reg5
 [    23]  0x0000000000400485..0x000000000040048d [   0] reg5
 [    46]  0x00000000004004b2..0x00000000004004ba [   0] breg5 0

DWARF section [34] '.debug_ranges' at offset 0xd94:
 [     0]  0x0000000000400480..0x0000000000400482
           0x0000000000400485..0x000000000040048d
 [    30]  0x00000000004004ad..0x00000000004004af
           0x00000000004004b2..0x00000000004004ba
EOF

# Produce "raw" unprocessed content.
testrun_compare ${abs_top_builddir}/src/readelf -U --debug-dump=loc --debug-dump=ranges \
  testfileloc<<\EOF

DWARF section [33] '.debug_loc' at offset 0xd2a:
 [     0]  000000000000000000..0x000000000000000d [   0] reg5
 [    23]  0x0000000000000005..0x000000000000000d [   0] reg5
 [    46]  0x0000000000000012..0x000000000000001a [   0] breg5 0

DWARF section [34] '.debug_ranges' at offset 0xd94:
 [     0]  000000000000000000..0x0000000000000002
           0x0000000000000005..0x000000000000000d
 [    30]  0x000000000000000d..0x000000000000000f
           0x0000000000000012..0x000000000000001a
EOF

exit 0
