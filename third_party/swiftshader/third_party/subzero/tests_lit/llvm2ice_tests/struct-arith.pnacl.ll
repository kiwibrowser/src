; This test is lowered from C code that does some simple aritmetic
; with struct members.

; REQUIRES: allow_dump
; RUN: %p2i -i %s --args --verbose inst -threads=0 | FileCheck %s

define internal i32 @compute_important_function(i32 %v1, i32 %v2) {
entry:
  %__2 = inttoptr i32 %v1 to i32*
  %_v0 = load i32, i32* %__2, align 1

; CHECK:        entry:
; CHECK-NEXT:       %_v0 = load i32, i32* {{.*}}, align 1

  %__4 = inttoptr i32 %v2 to i32*
  %_v1 = load i32, i32* %__4, align 1
  %gep = add i32 %v2, 12
  %__7 = inttoptr i32 %gep to i32*
  %_v2 = load i32, i32* %__7, align 1
  %mul = mul i32 %_v2, %_v1
  %gep6 = add i32 %v1, 4
  %__11 = inttoptr i32 %gep6 to i32*
  %_v3 = load i32, i32* %__11, align 1
  %gep8 = add i32 %v2, 8
  %__14 = inttoptr i32 %gep8 to i32*
  %_v4 = load i32, i32* %__14, align 1
  %gep10 = add i32 %v2, 4
  %__17 = inttoptr i32 %gep10 to i32*
  %_v5 = load i32, i32* %__17, align 1
  %mul3 = mul i32 %_v5, %_v4
  %gep12 = add i32 %v1, 8
  %__21 = inttoptr i32 %gep12 to i32*
  %_v6 = load i32, i32* %__21, align 1
  %mul7 = mul i32 %_v6, %_v3
  %mul9 = mul i32 %mul7, %_v6
  %gep14 = add i32 %v1, 12
  %__26 = inttoptr i32 %gep14 to i32*
  %_v7 = load i32, i32* %__26, align 1
  %mul11 = mul i32 %mul9, %_v7
  %add4.neg = add i32 %mul, %_v0
  %add = sub i32 %add4.neg, %_v3
  %sub = sub i32 %add, %mul3
  %sub12 = sub i32 %sub, %mul11
  ret i32 %sub12

; CHECK:        %sub12 = sub i32 %sub, %mul11
; CHECK-NEXT:       ret i32 %sub12
}
