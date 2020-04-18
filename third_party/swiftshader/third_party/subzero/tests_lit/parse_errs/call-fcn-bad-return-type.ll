; Test that even if a call return type matches its declaration, it must still be
; a legal call return type (unless declaration is intrinsic).

; REQUIRES: no_minimal_build

; RUN: %p2i --expect-fail -i %s --insts --args \
; RUN:      -allow-externally-defined-symbols | FileCheck %s

declare i32 @f();

declare i64 @g();

define void @Test(i32 %ifcn) {
entry:
  %fcn =  inttoptr i32 %ifcn to i1()*
  %v = call i1 %fcn()
; CHECK: Return type of function is invalid: i1
  ret void
}
