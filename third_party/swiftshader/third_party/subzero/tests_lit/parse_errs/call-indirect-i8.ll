; Tests that we don't allow illegal sized parameters on indirect calls.

; REQUIRES: no_minimal_build

; RUN: %p2i --expect-fail -i %s --insts | FileCheck %s

define internal void @CallIndirectI32(i32 %f_addr) {
entry:
  %f = inttoptr i32 %f_addr to i32(i8)*
  %r = call i32 %f(i8 1)
; CHECK: Argument 1 of function has invalid type: i8
  ret void
}
