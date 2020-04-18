; Test reading phi instructions.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

; TODO(kschimpf) Add forward reference examples.

define internal i32 @testPhi1(i32 %arg) {
entry:
  %cmp1 = icmp sgt i32 %arg, 0
  br i1 %cmp1, label %next, label %target
next:
  br label %target
target:
  %merge = phi i1 [ %cmp1, %entry ], [ false, %next ]
  %result = zext i1 %merge to i32
  ret i32 %result
}

; CHECK:      define internal i32 @testPhi1(i32 %arg) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %cmp1 = icmp sgt i32 %arg, 0
; CHECK-NEXT:   br i1 %cmp1, label %next, label %target
; CHECK-NEXT: next:
; CHECK-NEXT:   br label %target
; CHECK-NEXT: target:
; CHECK-NEXT:   %merge = phi i1 [ %cmp1, %entry ], [ false, %next ]
; CHECK-NEXT:   %result = zext i1 %merge to i32
; CHECK-NEXT:   ret i32 %result
; CHECK-NEXT: }

; NOIR: Total across all functions
