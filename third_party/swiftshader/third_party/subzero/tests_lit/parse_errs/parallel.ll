; Test that we report a bug at the same place, independent of a parallel parse.

; REQUIRES: no_minimal_build

; RUN: %p2i --expect-fail -i %s --args -threads=0 -parse-parallel=0 \
; RUN:      -allow-externally-defined-symbols | FileCheck %s

; RUN: %p2i --expect-fail -i %s --args -threads=1 -parse-parallel=1 \
; RUN:      -allow-externally-defined-symbols | FileCheck %s

declare i32 @f();

declare i64 @g();

define void @Test(i32 %ifcn) {
entry:
  %fcn =  inttoptr i32 %ifcn to i1()*
  %v = call i1 %fcn()

; CHECK: Error(222:6): Return type of function is invalid: i1

  ret void
}
