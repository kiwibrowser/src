; Tests that -asm-verbose doesn't fail liveness validation because of
; callee-save pushes/pops in a single-basic-block function.

; REQUIRES: allow_dump
; RUN: %p2i --target x8632 -i %s --filetype=asm --args -O2 -asm-verbose \
; RUN:   | FileCheck %s
; TODO(stichnot,jpp): Enable for x8664.
; RUIN: %p2i --target x8664 -i %s --filetype=asm --args -O2 -asm-verbose \
; RUIN:   | FileCheck %s
; RUN: %p2i --target arm32 -i %s --filetype=asm --args -O2 -asm-verbose \
; RUN:   | FileCheck %s

define internal i32 @single_bb(i32 %arg0, i32 %arg1, i32 %arg2, i32 %arg3,
                               i32 %arg4, i32 %arg5, i32 %arg6, i32 %arg7) {
b1:
  %t1 = add i32 %arg0, %arg1
  %t2 = add i32 %t1, %arg2
  %t3 = add i32 %t2, %arg3
  %t4 = add i32 %t3, %arg4
  %t5 = add i32 %t4, %arg5
  %t6 = add i32 %t5, %arg6
  %t7 = add i32 %t6, %arg7
  ret i32 %t7
}

; CHECK-LABEL: single_bb
