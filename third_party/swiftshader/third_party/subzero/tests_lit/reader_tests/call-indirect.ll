; Test parsing indirect calls in Subzero.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN: %p2i -i %s --args -notranslate -timing | FileCheck --check-prefix=NOIR %s

define internal void @CallIndirectVoid(i32 %f_addr) {
entry:
  %f = inttoptr i32 %f_addr to void ()*
  call void %f()
  ret void
}

; CHECK:      define internal void @CallIndirectVoid(i32 %f_addr) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   call void %f_addr()
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal i32 @CallIndirectI32(i32 %f_addr) {
entry:
  %f = inttoptr i32 %f_addr to i32(i64, i32)*
  %r = call i32 %f(i64 1, i32 %f_addr)
  ret i32 %r
}

; CHECK-NEXT: define internal i32 @CallIndirectI32(i32 %f_addr) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call i32 %f_addr(i64 1, i32 %f_addr)
; CHECK-NEXT:   ret i32 %r
; CHECK-NEXT: }

; NOIR: Total across all functions
