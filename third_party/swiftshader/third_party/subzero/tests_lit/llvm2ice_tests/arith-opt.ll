; This is a very early test that just checks the representation of i32
; arithmetic instructions.  No assembly tests are done.

; REQUIRES: allow_dump

; RUN: %p2i -i %s --filetype=asm --args --verbose inst -threads=0 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s

define internal i32 @Add(i32 %a, i32 %b) {
; CHECK: define internal i32 @Add
entry:
  %add = add i32 %b, %a
; CHECK: add
  tail call void @Use(i32 %add)
; CHECK: call Use
  ret i32 %add
}

declare void @Use(i32)

define internal i32 @And(i32 %a, i32 %b) {
; CHECK: define internal i32 @And
entry:
  %and = and i32 %b, %a
; CHECK: and
  tail call void @Use(i32 %and)
; CHECK: call Use
  ret i32 %and
}

define internal i32 @Or(i32 %a, i32 %b) {
; CHECK: define internal i32 @Or
entry:
  %or = or i32 %b, %a
; CHECK: or
  tail call void @Use(i32 %or)
; CHECK: call Use
  ret i32 %or
}

define internal i32 @Xor(i32 %a, i32 %b) {
; CHECK: define internal i32 @Xor
entry:
  %xor = xor i32 %b, %a
; CHECK: xor
  tail call void @Use(i32 %xor)
; CHECK: call Use
  ret i32 %xor
}

define internal i32 @Sub(i32 %a, i32 %b) {
; CHECK: define internal i32 @Sub
entry:
  %sub = sub i32 %a, %b
; CHECK: sub
  tail call void @Use(i32 %sub)
; CHECK: call Use
  ret i32 %sub
}

define internal i32 @Mul(i32 %a, i32 %b) {
; CHECK: define internal i32 @Mul
entry:
  %mul = mul i32 %b, %a
; CHECK: imul
  tail call void @Use(i32 %mul)
; CHECK: call Use
  ret i32 %mul
}

define internal i32 @Sdiv(i32 %a, i32 %b) {
; CHECK: define internal i32 @Sdiv
entry:
  %div = sdiv i32 %a, %b
; CHECK: cdq
; CHECK: idiv
  tail call void @Use(i32 %div)
; CHECK: call Use
  ret i32 %div
}

define internal i32 @Srem(i32 %a, i32 %b) {
; CHECK: define internal i32 @Srem
entry:
  %rem = srem i32 %a, %b
; CHECK: cdq
; CHECK: idiv
  tail call void @Use(i32 %rem)
; CHECK: call Use
  ret i32 %rem
}

define internal i32 @Udiv(i32 %a, i32 %b) {
; CHECK: define internal i32 @Udiv
entry:
  %div = udiv i32 %a, %b
; CHECK: div
  tail call void @Use(i32 %div)
; CHECK: call Use
  ret i32 %div
}

define internal i32 @Urem(i32 %a, i32 %b) {
; CHECK: define internal i32 @Urem
entry:
  %rem = urem i32 %a, %b
; CHECK: div
  tail call void @Use(i32 %rem)
; CHECK: call Use
  ret i32 %rem
}

; Check for a valid addressing mode in the x86-32 mul instruction when
; the second source operand is an immediate.
define internal i64 @MulImm() {
entry:
  %mul = mul i64 3, 4
  ret i64 %mul
}
; CHECK-LABEL: MulImm
; CHECK-NOT: mul {{[0-9]+}}
