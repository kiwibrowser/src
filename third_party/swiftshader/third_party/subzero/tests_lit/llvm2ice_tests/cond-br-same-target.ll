; Test conditional branch where targets are the same.
; Tests issue: https://code.google.com/p/nativeclient/issues/detail?id=4212
; REQUIRES: allow_dump

; RUN: %p2i -i %s --insts | FileCheck %s

define internal void @f(i32 %foo, i32 %bar) {
entry:
  %c = icmp ult i32 %foo, %bar
  br i1 %c, label %block, label %block
block:
  ret void
}

; Note that the branch is converted to an unconditional branch.

; CHECK:      define internal void @f(i32 %foo, i32 %bar) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %c = icmp ult i32 %foo, %bar
; CHECK-NEXT:   br label %block
; CHECK-NEXT: block:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }
