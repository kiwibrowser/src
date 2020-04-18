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

# See the source files testfile_const_type.c testfile_implicit_value.c
# testfile_entry_value.c testfile_parameter_ref.c testfile_implicit_pointer.c
# how to regenerate the test files (needs GCC 4.8+).

testfiles testfile_const_type testfile_implicit_value testfile_entry_value
testfiles testfile_parameter_ref testfile_implicit_pointer

testrun_compare ${abs_top_builddir}/tests/varlocs -e testfile_const_type <<\EOF
module 'testfile_const_type'
[b] CU 'const_type.c'@0
  [33] function 'f1'@80483f0
    frame_base: {call_frame_cfa {bregx(4,4)}}
    [4b] parameter 'd'
      [80483f0,804841b) {fbreg(0)}
    [57] variable 'w'
      [80483f0,804841b) {fbreg(0), GNU_deref_type(8){long long int,signed,64@[25]}, GNU_const_type{long long int,signed,64@[25]}(8)[0000806745230100], div, GNU_convert{long long unsigned int,unsigned,64@[2c]}, stack_value}
  [7d] function 'main'@80482f0
    frame_base: {call_frame_cfa {bregx(4,4)}}
EOF

testrun_compare ${abs_top_builddir}/tests/varlocs -e testfile_implicit_value <<\EOF
module 'testfile_implicit_value'
[b] CU 'implicit_value.c'@0
  [25] function 'foo'@80483f0
    frame_base: {call_frame_cfa {bregx(4,4)}}
    [3e] variable 'a'
      [80483f0,80483f6) {implicit_value(8){0200000000000000}, piece(8), implicit_value(8){1500000000000000}, piece(8)}
  [86] function 'main'@80482f0
    frame_base: {call_frame_cfa {bregx(4,4)}}
EOF

testrun_compare ${abs_top_builddir}/tests/varlocs -e testfile_entry_value <<\EOF
module 'testfile_entry_value'
[b] CU 'entry_value.c'@0
  [29] function 'foo'@400500
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [4a] parameter 'x'
      [400500,400504) {reg5}
    [55] parameter 'y'
      [400500,400504) {reg4}
  [68] function 'bar'@400510
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [89] parameter 'x'
      [400510,40051c) {reg5}
      [40051c,40052b) {reg6}
      [40052b,400531) {GNU_entry_value(1) {reg5}, stack_value}
    [96] parameter 'y'
      [400510,40051c) {reg4}
      [40051c,40052a) {reg3}
      [40052a,400531) {GNU_entry_value(1) {reg4}, stack_value}
    [a3] variable 'z'
      [400524,400528) {reg0}
      [400528,400529) {reg12}
      [400529,40052e) {breg0(0), breg12(0), plus, stack_value}
      [40052e,400531) {reg0}
  [e9] function 'main'@400400
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [10a] parameter 'argc'
      [400400,400406) {reg5}
      [400406,40040a) {breg5(-1), stack_value}
      [40040a,40040b) {GNU_entry_value(1) {reg5}, stack_value}
    [119] parameter 'argv'
      [400400,400403) {reg4}
      [400403,40040b) {GNU_entry_value(1) {reg4}, stack_value}
EOF

testrun_compare ${abs_top_builddir}/tests/varlocs -e testfile_parameter_ref <<\EOF
module 'testfile_parameter_ref'
[b] CU 'parameter_ref.c'@0
  [77] function 'foo'@400510
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [92] parameter 'x'
      [400510,400523) {reg5}
    [99] parameter 'y'
      [400510,400523) {GNU_parameter_ref[42], stack_value}
    [a5] variable 'a'
      [400510,400523) {breg5(0), lit1, shl, stack_value}
    [b0] variable 'b'
      [400510,400523) {GNU_parameter_ref[42], lit1, shl, stack_value}
    [be] variable 'c'
      <constant value>
    [c4] parameter 'z'
      <constant value>
  [cb] function 'main'@400400
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [ec] parameter 'x'
      [400400,400408) {reg5}
      [400408,400421) {reg3}
      [400421,400423) {GNU_entry_value(1) {reg5}, stack_value}
    [f9] parameter 'argv'
      [400400,400408) {reg4}
      [400408,400423) {GNU_entry_value(1) {reg4}, stack_value}
EOF

testrun_compare ${abs_top_builddir}/tests/varlocs -e testfile_implicit_pointer <<\EOF
module 'testfile_implicit_pointer'
[b] CU 'implicit_pointer.c'@0
  [29] function 'foo'@400500
    frame_base: {call_frame_cfa {bregx(7,8)}}
    [4a] parameter 'i'
      [400500,400503) {reg5}
    [55] variable 'p'
      [400500,400503) {GNU_implicit_pointer([4a],0) {reg5}}
  [73] function 'main'@400400
    frame_base: {call_frame_cfa {bregx(7,8)}}
EOF


exit 0
