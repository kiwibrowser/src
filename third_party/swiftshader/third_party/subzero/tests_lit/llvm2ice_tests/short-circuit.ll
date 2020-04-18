; Test on -enable-sc if basic blocks are split when short circuit evaluation
; is possible for boolean expressions

; REQUIRES: allow_dump

; RUN: %p2i -i %s --filetype=asm --target x8632 --args \
; RUN: -O2 -enable-sc | FileCheck %s --check-prefix=ENABLE \
; RUN: --check-prefix=CHECK

; RUN: %p2i -i %s --filetype=asm --target x8632 --args \
; RUN: -O2 | FileCheck %s --check-prefix=NOENABLE \
; RUN: --check-prefix=CHECK

define internal i32 @short_circuit(i32 %arg1, i32 %arg2, i32 %arg3, i32 %arg4,
                                   i32 %arg5) {
  %t0 = trunc i32 %arg1 to i1
  %t1 = trunc i32 %arg2 to i1
  %t2 = trunc i32 %arg3 to i1
  %t3 = trunc i32 %arg4 to i1
  %t4 = trunc i32 %arg5 to i1

  %t5 = or i1 %t0, %t1
  %t6 = and i1 %t5, %t2
  %t7 = and i1 %t3, %t4
  %t8 = or i1 %t6, %t7

  br i1 %t8, label %target_true, label %target_false

target_true:
  ret i32 1

target_false:
  ret i32 0
}

; CHECK-LABEL: short_circuit
; NOENABLE: .Lshort_circuit$__0:
; ENABLE: .Lshort_circuit$__0_1_1:
; ENABLE: .Lshort_circuit$__0_1_2:
; ENABLE: .Lshort_circuit$__0_2:
; CHECK: .Lshort_circuit$target_true:
; CHECK: .Lshort_circuit$target_false:
