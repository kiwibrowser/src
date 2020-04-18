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

# struct s1
# {
#   char c;
#   short s;
#   int i;
#   long l;
#   float f;
#   double d;
# };
# 
# s1 S1;
# 
# int func (s1 *p)
# {
#   return p->i;
# }
# 
# int main()
# {
#   return func (&S1);
# }
#
# g++ -gdwarf-4 -g -fdebug-types-section

testfiles testfile59

testrun_compare ${abs_builddir}/typeiter testfile59 <<\EOF
ok
EOF

testrun_compare ${abs_builddir}/typeiter2 testfile59 <<\EOF
ok
EOF

exit 0
