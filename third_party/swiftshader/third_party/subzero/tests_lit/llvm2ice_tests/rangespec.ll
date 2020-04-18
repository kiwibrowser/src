; Tests basic functionality of RangeSpec matching.  Makes use of the fact that
; "-verbose=status" prints the sequence number, and "-test-status" can suppress
; this output.  Note that seq=2 is the first sequence number for functions.

; REQUIRES: allow_dump

define internal void @Func2() { ret void }
define internal void @Func3() { ret void }
define internal void @Func4() { ret void }
define internal void @Func5() { ret void }
define internal void @Func6() { ret void }
define internal void @Func7() { ret void }
define internal void @Func8() { ret void }
define internal void @Func9() { ret void }
define internal void @Func10() { ret void }
define internal void @Func11() { ret void }

; A few tests that include everything.
; RUN: %p2i -i %s -o /dev/null --args -verbose status -threads=0 \
; RUN:   -test-status=: \
; RUN:   | FileCheck %s --check-prefix=TEST1
; RUN: %p2i -i %s -o /dev/null --args -verbose status -threads=0 \
; RUN:   -test-status=2: \
; RUN:   | FileCheck %s --check-prefix=TEST1
; RUN: %p2i -i %s -o /dev/null --args -verbose status -threads=0 \
; RUN:   -test-status=:20 \
; RUN:   | FileCheck %s --check-prefix=TEST1
; RUN: %p2i -i %s -o /dev/null --args -verbose status -threads=0 \
; RUN:   -test-status=-xxx \
; RUN:   | FileCheck %s --check-prefix=TEST1
; TEST1: seq=2
; TEST1: seq=3
; TEST1: seq=4
; TEST1: seq=5
; TEST1: seq=6
; TEST1: seq=7
; TEST1: seq=8
; TEST1: seq=9
; TEST1: seq=10
; TEST1: seq=11

; Several ways of expressing 3+4+5+6
; RUN: %p2i -i %s -o /dev/null --args -verbose status -threads=0 \
; RUN:   -test-status=3:7 \
; RUN:   | FileCheck %s --check-prefix=TEST2
; RUN: %p2i -i %s -o /dev/null --args -verbose status -threads=0 \
; RUN:   -test-status=3:6,6 \
; RUN:   | FileCheck %s --check-prefix=TEST2
; RUN: %p2i -i %s -o /dev/null --args -verbose status -threads=0 \
; RUN:   -test-status=5,3:5,6 \
; RUN:   | FileCheck %s --check-prefix=TEST2
; RUN: %p2i -i %s -o /dev/null --args -verbose status -threads=0 \
; RUN:   -test-status=3:9,-7: \
; RUN:   | FileCheck %s --check-prefix=TEST2
; RUN: %p2i -i %s -o /dev/null --args -verbose status -threads=0 \
; RUN:   -test-status=3:9,-Func7,-Func8 \
; RUN:   | FileCheck %s --check-prefix=TEST2
; TEST2-NOT: seq=2
; TEST2: seq=3
; TEST2: seq=4
; TEST2: seq=5
; TEST2: seq=6
; TEST2-NOT: seq=7
; TEST2-NOT: seq=8
; TEST2-NOT: seq=9
; TEST2-NOT: seq=10
; TEST2-NOT: seq=11
; TEST2-NOT: seq=12
