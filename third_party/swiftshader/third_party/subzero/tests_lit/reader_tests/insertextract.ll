; Tests insertelement and extractelement vector instructions.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN: %l2i -i %s --insts | %ifl FileCheck %s
; RUN: %lc2i -i %s --insts | %iflc FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

define internal void @ExtractV4xi1(<4 x i1> %v) {
entry:
  %e0 = extractelement <4 x i1> %v, i32 0
  %e1 = extractelement <4 x i1> %v, i32 1
  %e2 = extractelement <4 x i1> %v, i32 2
  %e3 = extractelement <4 x i1> %v, i32 3
  ret void
}

; CHECK:      define internal void @ExtractV4xi1(<4 x i1> %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e0 = extractelement <4 x i1> %v, i32 0
; CHECK-NEXT:   %e1 = extractelement <4 x i1> %v, i32 1
; CHECK-NEXT:   %e2 = extractelement <4 x i1> %v, i32 2
; CHECK-NEXT:   %e3 = extractelement <4 x i1> %v, i32 3
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @ExtractV8xi1(<8 x i1> %v) {
entry:
  %e0 = extractelement <8 x i1> %v, i32 0
  %e1 = extractelement <8 x i1> %v, i32 1
  %e2 = extractelement <8 x i1> %v, i32 2
  %e3 = extractelement <8 x i1> %v, i32 3
  %e4 = extractelement <8 x i1> %v, i32 4
  %e5 = extractelement <8 x i1> %v, i32 5
  %e6 = extractelement <8 x i1> %v, i32 6
  %e7 = extractelement <8 x i1> %v, i32 7
  ret void
}

; CHECK-NEXT: define internal void @ExtractV8xi1(<8 x i1> %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e0 = extractelement <8 x i1> %v, i32 0
; CHECK-NEXT:   %e1 = extractelement <8 x i1> %v, i32 1
; CHECK-NEXT:   %e2 = extractelement <8 x i1> %v, i32 2
; CHECK-NEXT:   %e3 = extractelement <8 x i1> %v, i32 3
; CHECK-NEXT:   %e4 = extractelement <8 x i1> %v, i32 4
; CHECK-NEXT:   %e5 = extractelement <8 x i1> %v, i32 5
; CHECK-NEXT:   %e6 = extractelement <8 x i1> %v, i32 6
; CHECK-NEXT:   %e7 = extractelement <8 x i1> %v, i32 7
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @ExtractV16xi1(<16 x i1> %v) {
entry:
  %e0 = extractelement <16 x i1> %v, i32 0
  %e1 = extractelement <16 x i1> %v, i32 1
  %e2 = extractelement <16 x i1> %v, i32 2
  %e3 = extractelement <16 x i1> %v, i32 3
  %e4 = extractelement <16 x i1> %v, i32 4
  %e5 = extractelement <16 x i1> %v, i32 5
  %e6 = extractelement <16 x i1> %v, i32 6
  %e7 = extractelement <16 x i1> %v, i32 7
  %e8 = extractelement <16 x i1> %v, i32 8
  %e9 = extractelement <16 x i1> %v, i32 9
  %e10 = extractelement <16 x i1> %v, i32 10
  %e11 = extractelement <16 x i1> %v, i32 11
  %e12 = extractelement <16 x i1> %v, i32 12
  %e13 = extractelement <16 x i1> %v, i32 13
  %e14 = extractelement <16 x i1> %v, i32 14
  %e15 = extractelement <16 x i1> %v, i32 15
  ret void
}

; CHECK-NEXT: define internal void @ExtractV16xi1(<16 x i1> %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e0 = extractelement <16 x i1> %v, i32 0
; CHECK-NEXT:   %e1 = extractelement <16 x i1> %v, i32 1
; CHECK-NEXT:   %e2 = extractelement <16 x i1> %v, i32 2
; CHECK-NEXT:   %e3 = extractelement <16 x i1> %v, i32 3
; CHECK-NEXT:   %e4 = extractelement <16 x i1> %v, i32 4
; CHECK-NEXT:   %e5 = extractelement <16 x i1> %v, i32 5
; CHECK-NEXT:   %e6 = extractelement <16 x i1> %v, i32 6
; CHECK-NEXT:   %e7 = extractelement <16 x i1> %v, i32 7
; CHECK-NEXT:   %e8 = extractelement <16 x i1> %v, i32 8
; CHECK-NEXT:   %e9 = extractelement <16 x i1> %v, i32 9
; CHECK-NEXT:   %e10 = extractelement <16 x i1> %v, i32 10
; CHECK-NEXT:   %e11 = extractelement <16 x i1> %v, i32 11
; CHECK-NEXT:   %e12 = extractelement <16 x i1> %v, i32 12
; CHECK-NEXT:   %e13 = extractelement <16 x i1> %v, i32 13
; CHECK-NEXT:   %e14 = extractelement <16 x i1> %v, i32 14
; CHECK-NEXT:   %e15 = extractelement <16 x i1> %v, i32 15
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @ExtractV16xi8(<16 x i8> %v, i32 %i) {
entry:
  %e0 = extractelement <16 x i8> %v, i32 0
  %e1 = extractelement <16 x i8> %v, i32 1
  %e2 = extractelement <16 x i8> %v, i32 2
  %e3 = extractelement <16 x i8> %v, i32 3
  %e4 = extractelement <16 x i8> %v, i32 4
  %e5 = extractelement <16 x i8> %v, i32 5
  %e6 = extractelement <16 x i8> %v, i32 6
  %e7 = extractelement <16 x i8> %v, i32 7
  %e8 = extractelement <16 x i8> %v, i32 8
  %e9 = extractelement <16 x i8> %v, i32 9
  %e10 = extractelement <16 x i8> %v, i32 10
  %e11 = extractelement <16 x i8> %v, i32 11
  %e12 = extractelement <16 x i8> %v, i32 12
  %e13 = extractelement <16 x i8> %v, i32 13
  %e14 = extractelement <16 x i8> %v, i32 14
  %e15 = extractelement <16 x i8> %v, i32 15
  ret void
}

; CHECK-NEXT: define internal void @ExtractV16xi8(<16 x i8> %v, i32 %i) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e0 = extractelement <16 x i8> %v, i32 0
; CHECK-NEXT:   %e1 = extractelement <16 x i8> %v, i32 1
; CHECK-NEXT:   %e2 = extractelement <16 x i8> %v, i32 2
; CHECK-NEXT:   %e3 = extractelement <16 x i8> %v, i32 3
; CHECK-NEXT:   %e4 = extractelement <16 x i8> %v, i32 4
; CHECK-NEXT:   %e5 = extractelement <16 x i8> %v, i32 5
; CHECK-NEXT:   %e6 = extractelement <16 x i8> %v, i32 6
; CHECK-NEXT:   %e7 = extractelement <16 x i8> %v, i32 7
; CHECK-NEXT:   %e8 = extractelement <16 x i8> %v, i32 8
; CHECK-NEXT:   %e9 = extractelement <16 x i8> %v, i32 9
; CHECK-NEXT:   %e10 = extractelement <16 x i8> %v, i32 10
; CHECK-NEXT:   %e11 = extractelement <16 x i8> %v, i32 11
; CHECK-NEXT:   %e12 = extractelement <16 x i8> %v, i32 12
; CHECK-NEXT:   %e13 = extractelement <16 x i8> %v, i32 13
; CHECK-NEXT:   %e14 = extractelement <16 x i8> %v, i32 14
; CHECK-NEXT:   %e15 = extractelement <16 x i8> %v, i32 15
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @ExtractV8xi16(<8 x i16> %v) {
entry:
  %e0 = extractelement <8 x i16> %v, i32 0
  %e1 = extractelement <8 x i16> %v, i32 1
  %e2 = extractelement <8 x i16> %v, i32 2
  %e3 = extractelement <8 x i16> %v, i32 3
  %e4 = extractelement <8 x i16> %v, i32 4
  %e5 = extractelement <8 x i16> %v, i32 5
  %e6 = extractelement <8 x i16> %v, i32 6
  %e7 = extractelement <8 x i16> %v, i32 7
  ret void
}

; CHECK-NEXT: define internal void @ExtractV8xi16(<8 x i16> %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e0 = extractelement <8 x i16> %v, i32 0
; CHECK-NEXT:   %e1 = extractelement <8 x i16> %v, i32 1
; CHECK-NEXT:   %e2 = extractelement <8 x i16> %v, i32 2
; CHECK-NEXT:   %e3 = extractelement <8 x i16> %v, i32 3
; CHECK-NEXT:   %e4 = extractelement <8 x i16> %v, i32 4
; CHECK-NEXT:   %e5 = extractelement <8 x i16> %v, i32 5
; CHECK-NEXT:   %e6 = extractelement <8 x i16> %v, i32 6
; CHECK-NEXT:   %e7 = extractelement <8 x i16> %v, i32 7
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal i32 @ExtractV4xi32(<4 x i32> %v) {
entry:
  %e0 = extractelement <4 x i32> %v, i32 0
  %e1 = extractelement <4 x i32> %v, i32 1
  %e2 = extractelement <4 x i32> %v, i32 2
  %e3 = extractelement <4 x i32> %v, i32 3
  ret i32 %e0
}

; CHECK-NEXT: define internal i32 @ExtractV4xi32(<4 x i32> %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e0 = extractelement <4 x i32> %v, i32 0
; CHECK-NEXT:   %e1 = extractelement <4 x i32> %v, i32 1
; CHECK-NEXT:   %e2 = extractelement <4 x i32> %v, i32 2
; CHECK-NEXT:   %e3 = extractelement <4 x i32> %v, i32 3
; CHECK-NEXT:   ret i32 %e0
; CHECK-NEXT: }

define internal float @ExtractV4xfloat(<4 x float> %v) {
entry:
  %e0 = extractelement <4 x float> %v, i32 0
  %e1 = extractelement <4 x float> %v, i32 1
  %e2 = extractelement <4 x float> %v, i32 2
  %e3 = extractelement <4 x float> %v, i32 3
  ret float %e0
}

; CHECK-NEXT: define internal float @ExtractV4xfloat(<4 x float> %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e0 = extractelement <4 x float> %v, i32 0
; CHECK-NEXT:   %e1 = extractelement <4 x float> %v, i32 1
; CHECK-NEXT:   %e2 = extractelement <4 x float> %v, i32 2
; CHECK-NEXT:   %e3 = extractelement <4 x float> %v, i32 3
; CHECK-NEXT:   ret float %e0
; CHECK-NEXT: }

define internal <4 x i1> @InsertV4xi1(<4 x i1> %v, i32 %pe) {
entry:
  %e = trunc i32 %pe to i1
  %r0 = insertelement <4 x i1> %v, i1 %e, i32 0
  %r1 = insertelement <4 x i1> %v, i1 %e, i32 1
  %r2 = insertelement <4 x i1> %v, i1 %e, i32 2
  %r3 = insertelement <4 x i1> %v, i1 %e, i32 3
  ret <4 x i1> %r3
}

; CHECK-NEXT: define internal <4 x i1> @InsertV4xi1(<4 x i1> %v, i32 %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e = trunc i32 %pe to i1
; CHECK-NEXT:   %r0 = insertelement <4 x i1> %v, i1 %e, i32 0
; CHECK-NEXT:   %r1 = insertelement <4 x i1> %v, i1 %e, i32 1
; CHECK-NEXT:   %r2 = insertelement <4 x i1> %v, i1 %e, i32 2
; CHECK-NEXT:   %r3 = insertelement <4 x i1> %v, i1 %e, i32 3
; CHECK-NEXT:   ret <4 x i1> %r3
; CHECK-NEXT: }

define internal <8 x i1> @InsertV8xi1(<8 x i1> %v, i32 %pe) {
entry:
  %e = trunc i32 %pe to i1
  %r0 = insertelement <8 x i1> %v, i1 %e, i32 0
  %r1 = insertelement <8 x i1> %v, i1 %e, i32 1
  %r2 = insertelement <8 x i1> %v, i1 %e, i32 2
  %r3 = insertelement <8 x i1> %v, i1 %e, i32 3
  %r4 = insertelement <8 x i1> %v, i1 %e, i32 4
  %r5 = insertelement <8 x i1> %v, i1 %e, i32 5
  %r6 = insertelement <8 x i1> %v, i1 %e, i32 6
  %r7 = insertelement <8 x i1> %v, i1 %e, i32 7
  ret <8 x i1> %r7
}

; CHECK-NEXT: define internal <8 x i1> @InsertV8xi1(<8 x i1> %v, i32 %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e = trunc i32 %pe to i1
; CHECK-NEXT:   %r0 = insertelement <8 x i1> %v, i1 %e, i32 0
; CHECK-NEXT:   %r1 = insertelement <8 x i1> %v, i1 %e, i32 1
; CHECK-NEXT:   %r2 = insertelement <8 x i1> %v, i1 %e, i32 2
; CHECK-NEXT:   %r3 = insertelement <8 x i1> %v, i1 %e, i32 3
; CHECK-NEXT:   %r4 = insertelement <8 x i1> %v, i1 %e, i32 4
; CHECK-NEXT:   %r5 = insertelement <8 x i1> %v, i1 %e, i32 5
; CHECK-NEXT:   %r6 = insertelement <8 x i1> %v, i1 %e, i32 6
; CHECK-NEXT:   %r7 = insertelement <8 x i1> %v, i1 %e, i32 7
; CHECK-NEXT:   ret <8 x i1> %r7
; CHECK-NEXT: }

define internal <16 x i1> @InsertV16xi1(<16 x i1> %v, i32 %pe) {
entry:
  %e = trunc i32 %pe to i1
  %r0 = insertelement <16 x i1> %v, i1 %e, i32 0
  %r1 = insertelement <16 x i1> %v, i1 %e, i32 1
  %r2 = insertelement <16 x i1> %v, i1 %e, i32 2
  %r3 = insertelement <16 x i1> %v, i1 %e, i32 3
  %r4 = insertelement <16 x i1> %v, i1 %e, i32 4
  %r5 = insertelement <16 x i1> %v, i1 %e, i32 5
  %r6 = insertelement <16 x i1> %v, i1 %e, i32 6
  %r7 = insertelement <16 x i1> %v, i1 %e, i32 7
  %r8 = insertelement <16 x i1> %v, i1 %e, i32 8
  %r9 = insertelement <16 x i1> %v, i1 %e, i32 9
  %r10 = insertelement <16 x i1> %v, i1 %e, i32 10
  %r11 = insertelement <16 x i1> %v, i1 %e, i32 11
  %r12 = insertelement <16 x i1> %v, i1 %e, i32 12
  %r13 = insertelement <16 x i1> %v, i1 %e, i32 13
  %r14 = insertelement <16 x i1> %v, i1 %e, i32 14
  %r15 = insertelement <16 x i1> %v, i1 %e, i32 15
  ret <16 x i1> %r15
}

; CHECK-NEXT: define internal <16 x i1> @InsertV16xi1(<16 x i1> %v, i32 %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e = trunc i32 %pe to i1
; CHECK-NEXT:   %r0 = insertelement <16 x i1> %v, i1 %e, i32 0
; CHECK-NEXT:   %r1 = insertelement <16 x i1> %v, i1 %e, i32 1
; CHECK-NEXT:   %r2 = insertelement <16 x i1> %v, i1 %e, i32 2
; CHECK-NEXT:   %r3 = insertelement <16 x i1> %v, i1 %e, i32 3
; CHECK-NEXT:   %r4 = insertelement <16 x i1> %v, i1 %e, i32 4
; CHECK-NEXT:   %r5 = insertelement <16 x i1> %v, i1 %e, i32 5
; CHECK-NEXT:   %r6 = insertelement <16 x i1> %v, i1 %e, i32 6
; CHECK-NEXT:   %r7 = insertelement <16 x i1> %v, i1 %e, i32 7
; CHECK-NEXT:   %r8 = insertelement <16 x i1> %v, i1 %e, i32 8
; CHECK-NEXT:   %r9 = insertelement <16 x i1> %v, i1 %e, i32 9
; CHECK-NEXT:   %r10 = insertelement <16 x i1> %v, i1 %e, i32 10
; CHECK-NEXT:   %r11 = insertelement <16 x i1> %v, i1 %e, i32 11
; CHECK-NEXT:   %r12 = insertelement <16 x i1> %v, i1 %e, i32 12
; CHECK-NEXT:   %r13 = insertelement <16 x i1> %v, i1 %e, i32 13
; CHECK-NEXT:   %r14 = insertelement <16 x i1> %v, i1 %e, i32 14
; CHECK-NEXT:   %r15 = insertelement <16 x i1> %v, i1 %e, i32 15
; CHECK-NEXT:   ret <16 x i1> %r15
; CHECK-NEXT: }

define internal <16 x i8> @InsertV16xi8(<16 x i8> %v, i32 %pe) {
entry:
  %e = trunc i32 %pe to i8
  %r0 = insertelement <16 x i8> %v, i8 %e, i32 0
  %r1 = insertelement <16 x i8> %v, i8 %e, i32 1
  %r2 = insertelement <16 x i8> %v, i8 %e, i32 2
  %r3 = insertelement <16 x i8> %v, i8 %e, i32 3
  %r4 = insertelement <16 x i8> %v, i8 %e, i32 4
  %r5 = insertelement <16 x i8> %v, i8 %e, i32 5
  %r6 = insertelement <16 x i8> %v, i8 %e, i32 6
  %r7 = insertelement <16 x i8> %v, i8 %e, i32 7
  ret <16 x i8> %r7
}

; CHECK-NEXT: define internal <16 x i8> @InsertV16xi8(<16 x i8> %v, i32 %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e = trunc i32 %pe to i8
; CHECK-NEXT:   %r0 = insertelement <16 x i8> %v, i8 %e, i32 0
; CHECK-NEXT:   %r1 = insertelement <16 x i8> %v, i8 %e, i32 1
; CHECK-NEXT:   %r2 = insertelement <16 x i8> %v, i8 %e, i32 2
; CHECK-NEXT:   %r3 = insertelement <16 x i8> %v, i8 %e, i32 3
; CHECK-NEXT:   %r4 = insertelement <16 x i8> %v, i8 %e, i32 4
; CHECK-NEXT:   %r5 = insertelement <16 x i8> %v, i8 %e, i32 5
; CHECK-NEXT:   %r6 = insertelement <16 x i8> %v, i8 %e, i32 6
; CHECK-NEXT:   %r7 = insertelement <16 x i8> %v, i8 %e, i32 7
; CHECK-NEXT:   ret <16 x i8> %r7
; CHECK-NEXT: }

define internal <8 x i16> @InsertV8xi16(<8 x i16> %v, i32 %pe) {
entry:
  %e = trunc i32 %pe to i16
  %r0 = insertelement <8 x i16> %v, i16 %e, i32 0
  %r1 = insertelement <8 x i16> %v, i16 %e, i32 1
  %r2 = insertelement <8 x i16> %v, i16 %e, i32 2
  %r3 = insertelement <8 x i16> %v, i16 %e, i32 3
  %r4 = insertelement <8 x i16> %v, i16 %e, i32 4
  %r5 = insertelement <8 x i16> %v, i16 %e, i32 5
  %r6 = insertelement <8 x i16> %v, i16 %e, i32 6
  %r7 = insertelement <8 x i16> %v, i16 %e, i32 7
  ret <8 x i16> %r7
}

; CHECK-NEXT: define internal <8 x i16> @InsertV8xi16(<8 x i16> %v, i32 %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %e = trunc i32 %pe to i16
; CHECK-NEXT:   %r0 = insertelement <8 x i16> %v, i16 %e, i32 0
; CHECK-NEXT:   %r1 = insertelement <8 x i16> %v, i16 %e, i32 1
; CHECK-NEXT:   %r2 = insertelement <8 x i16> %v, i16 %e, i32 2
; CHECK-NEXT:   %r3 = insertelement <8 x i16> %v, i16 %e, i32 3
; CHECK-NEXT:   %r4 = insertelement <8 x i16> %v, i16 %e, i32 4
; CHECK-NEXT:   %r5 = insertelement <8 x i16> %v, i16 %e, i32 5
; CHECK-NEXT:   %r6 = insertelement <8 x i16> %v, i16 %e, i32 6
; CHECK-NEXT:   %r7 = insertelement <8 x i16> %v, i16 %e, i32 7
; CHECK-NEXT:   ret <8 x i16> %r7
; CHECK-NEXT: }

define internal <4 x i32> @InsertV4xi32(<4 x i32> %v, i32 %e) {
entry:
  %r0 = insertelement <4 x i32> %v, i32 %e, i32 0
  %r1 = insertelement <4 x i32> %v, i32 %e, i32 1
  %r2 = insertelement <4 x i32> %v, i32 %e, i32 2
  %r3 = insertelement <4 x i32> %v, i32 %e, i32 3
  ret <4 x i32> %r3
}

; CHECK-NEXT: define internal <4 x i32> @InsertV4xi32(<4 x i32> %v, i32 %e) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r0 = insertelement <4 x i32> %v, i32 %e, i32 0
; CHECK-NEXT:   %r1 = insertelement <4 x i32> %v, i32 %e, i32 1
; CHECK-NEXT:   %r2 = insertelement <4 x i32> %v, i32 %e, i32 2
; CHECK-NEXT:   %r3 = insertelement <4 x i32> %v, i32 %e, i32 3
; CHECK-NEXT:   ret <4 x i32> %r3
; CHECK-NEXT: }

define internal <4 x float> @InsertV4xfloat(<4 x float> %v, float %e) {
entry:
  %r0 = insertelement <4 x float> %v, float %e, i32 0
  %r1 = insertelement <4 x float> %v, float %e, i32 1
  %r2 = insertelement <4 x float> %v, float %e, i32 2
  %r3 = insertelement <4 x float> %v, float %e, i32 3
  ret <4 x float> %r3
}

; CHECK-NEXT: define internal <4 x float> @InsertV4xfloat(<4 x float> %v, float %e) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r0 = insertelement <4 x float> %v, float %e, i32 0
; CHECK-NEXT:   %r1 = insertelement <4 x float> %v, float %e, i32 1
; CHECK-NEXT:   %r2 = insertelement <4 x float> %v, float %e, i32 2
; CHECK-NEXT:   %r3 = insertelement <4 x float> %v, float %e, i32 3
; CHECK-NEXT:   ret <4 x float> %r3
; CHECK-NEXT: }

; NOIR: Total across all functions
