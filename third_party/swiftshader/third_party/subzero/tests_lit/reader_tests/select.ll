; Tests if we can read select instructions.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

define internal void @Seli1(i32 %p) {
entry:
  %vc = trunc i32 %p to i1
  %vt = trunc i32 %p to i1
  %ve = trunc i32 %p to i1
  %r = select i1 %vc, i1 %vt, i1 %ve
  ret void
}

; CHECK:      define internal void @Seli1(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %p to i1
; CHECK-NEXT:   %vt = trunc i32 %p to i1
; CHECK-NEXT:   %ve = trunc i32 %p to i1
; CHECK-NEXT:   %r = select i1 %vc, i1 %vt, i1 %ve
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @Seli8(i32 %p) {
entry:
  %vc = trunc i32 %p to i1
  %vt = trunc i32 %p to i8
  %ve = trunc i32 %p to i8
  %r = select i1 %vc, i8 %vt, i8 %ve
  ret void
}

; CHECK-NEXT: define internal void @Seli8(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %p to i1
; CHECK-NEXT:   %vt = trunc i32 %p to i8
; CHECK-NEXT:   %ve = trunc i32 %p to i8
; CHECK-NEXT:   %r = select i1 %vc, i8 %vt, i8 %ve
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @Seli16(i32 %p) {
entry:
  %vc = trunc i32 %p to i1
  %vt = trunc i32 %p to i16
  %ve = trunc i32 %p to i16
  %r = select i1 %vc, i16 %vt, i16 %ve
  ret void
}

; CHECK-NEXT: define internal void @Seli16(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %p to i1
; CHECK-NEXT:   %vt = trunc i32 %p to i16
; CHECK-NEXT:   %ve = trunc i32 %p to i16
; CHECK-NEXT:   %r = select i1 %vc, i16 %vt, i16 %ve
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal i32 @Seli32(i32 %pc, i32 %pt, i32 %pe) {
entry:
  %vc = trunc i32 %pc to i1
  %r = select i1 %vc, i32 %pt, i32 %pe
  ret i32 %r
}

; CHECK-NEXT: define internal i32 @Seli32(i32 %pc, i32 %pt, i32 %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %pc to i1
; CHECK-NEXT:   %r = select i1 %vc, i32 %pt, i32 %pe
; CHECK-NEXT:   ret i32 %r
; CHECK-NEXT: }

define internal i64 @Seli64(i64 %pc, i64 %pt, i64 %pe) {
entry:
  %vc = trunc i64 %pc to i1
  %r = select i1 %vc, i64 %pt, i64 %pe
  ret i64 %r
}

; CHECK-NEXT: define internal i64 @Seli64(i64 %pc, i64 %pt, i64 %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i64 %pc to i1
; CHECK-NEXT:   %r = select i1 %vc, i64 %pt, i64 %pe
; CHECK-NEXT:   ret i64 %r
; CHECK-NEXT: }

define internal float @SelFloat(i32 %pc, float %pt, float %pe) {
entry:
  %vc = trunc i32 %pc to i1
  %r = select i1 %vc, float %pt, float %pe
  ret float %r
}

; CHECK-NEXT: define internal float @SelFloat(i32 %pc, float %pt, float %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %pc to i1
; CHECK-NEXT:   %r = select i1 %vc, float %pt, float %pe
; CHECK-NEXT:   ret float %r
; CHECK-NEXT: }

define internal double @SelDouble(i32 %pc, double %pt, double %pe) {
entry:
  %vc = trunc i32 %pc to i1
  %r = select i1 %vc, double %pt, double %pe
  ret double %r
}

; CHECK-NEXT: define internal double @SelDouble(i32 %pc, double %pt, double %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %pc to i1
; CHECK-NEXT:   %r = select i1 %vc, double %pt, double %pe
; CHECK-NEXT:   ret double %r
; CHECK-NEXT: }

define internal <16 x i1> @SelV16x1(i32 %pc, <16 x i1> %pt, <16 x i1> %pe) {
entry:
  %vc = trunc i32 %pc to i1
  %r = select i1 %vc, <16 x i1> %pt, <16 x i1> %pe
  ret <16 x i1> %r
}

; CHECK-NEXT: define internal <16 x i1> @SelV16x1(i32 %pc, <16 x i1> %pt, <16 x i1> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %pc to i1
; CHECK-NEXT:   %r = select i1 %vc, <16 x i1> %pt, <16 x i1> %pe
; CHECK-NEXT:   ret <16 x i1> %r
; CHECK-NEXT: }

define internal <8 x i1> @SelV8x1(i32 %pc, <8 x i1> %pt, <8 x i1> %pe) {
entry:
  %vc = trunc i32 %pc to i1
  %r = select i1 %vc, <8 x i1> %pt, <8 x i1> %pe
  ret <8 x i1> %r
}

; CHECK-NEXT: define internal <8 x i1> @SelV8x1(i32 %pc, <8 x i1> %pt, <8 x i1> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %pc to i1
; CHECK-NEXT:   %r = select i1 %vc, <8 x i1> %pt, <8 x i1> %pe
; CHECK-NEXT:   ret <8 x i1> %r
; CHECK-NEXT: }

define internal <4 x i1> @SelV4x1(i32 %pc, <4 x i1> %pt, <4 x i1> %pe) {
entry:
  %vc = trunc i32 %pc to i1
  %r = select i1 %vc, <4 x i1> %pt, <4 x i1> %pe
  ret <4 x i1> %r
}

; CHECK-NEXT: define internal <4 x i1> @SelV4x1(i32 %pc, <4 x i1> %pt, <4 x i1> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %pc to i1
; CHECK-NEXT:   %r = select i1 %vc, <4 x i1> %pt, <4 x i1> %pe
; CHECK-NEXT:   ret <4 x i1> %r
; CHECK-NEXT: }

define internal <16 x i8> @SelV16x8(i32 %pc, <16 x i8> %pt, <16 x i8> %pe) {
entry:
  %vc = trunc i32 %pc to i1
  %r = select i1 %vc, <16 x i8> %pt, <16 x i8> %pe
  ret <16 x i8> %r
}

; CHECK-NEXT: define internal <16 x i8> @SelV16x8(i32 %pc, <16 x i8> %pt, <16 x i8> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %pc to i1
; CHECK-NEXT:   %r = select i1 %vc, <16 x i8> %pt, <16 x i8> %pe
; CHECK-NEXT:   ret <16 x i8> %r
; CHECK-NEXT: }

define internal <8 x i16> @SelV8x16(i32 %pc, <8 x i16> %pt, <8 x i16> %pe) {
entry:
  %vc = trunc i32 %pc to i1
  %r = select i1 %vc, <8 x i16> %pt, <8 x i16> %pe
  ret <8 x i16> %r
}

; CHECK-NEXT: define internal <8 x i16> @SelV8x16(i32 %pc, <8 x i16> %pt, <8 x i16> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %pc to i1
; CHECK-NEXT:   %r = select i1 %vc, <8 x i16> %pt, <8 x i16> %pe
; CHECK-NEXT:   ret <8 x i16> %r
; CHECK-NEXT: }

define internal <4 x i32> @SelV4x32(i32 %pc, <4 x i32> %pt, <4 x i32> %pe) {
entry:
  %vc = trunc i32 %pc to i1
  %r = select i1 %vc, <4 x i32> %pt, <4 x i32> %pe
  ret <4 x i32> %r
}

; CHECK-NEXT: define internal <4 x i32> @SelV4x32(i32 %pc, <4 x i32> %pt, <4 x i32> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %pc to i1
; CHECK-NEXT:   %r = select i1 %vc, <4 x i32> %pt, <4 x i32> %pe
; CHECK-NEXT:   ret <4 x i32> %r
; CHECK-NEXT: }

define internal <4 x float> @SelV4xfloat(i32 %pc, <4 x float> %pt, <4 x float> %pe) {
entry:
  %vc = trunc i32 %pc to i1
  %r = select i1 %vc, <4 x float> %pt, <4 x float> %pe
  ret <4 x float> %r
}

; CHECK-NEXT: define internal <4 x float> @SelV4xfloat(i32 %pc, <4 x float> %pt, <4 x float> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vc = trunc i32 %pc to i1
; CHECK-NEXT:   %r = select i1 %vc, <4 x float> %pt, <4 x float> %pe
; CHECK-NEXT:   ret <4 x float> %r
; CHECK-NEXT: }

define internal <16 x i1> @SelV16x1Vcond(<16 x i1> %pc, <16 x i1> %pt, <16 x i1> %pe) {
entry:
  %r = select <16 x i1> %pc, <16 x i1> %pt, <16 x i1> %pe
  ret <16 x i1> %r
}

; CHECK-NEXT: define internal <16 x i1> @SelV16x1Vcond(<16 x i1> %pc, <16 x i1> %pt, <16 x i1> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = select <16 x i1> %pc, <16 x i1> %pt, <16 x i1> %pe
; CHECK-NEXT:   ret <16 x i1> %r
; CHECK-NEXT: }

define internal <8 x i1> @SelV8x1Vcond(<8 x i1> %pc, <8 x i1> %pt, <8 x i1> %pe) {
entry:
  %r = select <8 x i1> %pc, <8 x i1> %pt, <8 x i1> %pe
  ret <8 x i1> %r
}

; CHECK-NEXT: define internal <8 x i1> @SelV8x1Vcond(<8 x i1> %pc, <8 x i1> %pt, <8 x i1> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = select <8 x i1> %pc, <8 x i1> %pt, <8 x i1> %pe
; CHECK-NEXT:   ret <8 x i1> %r
; CHECK-NEXT: }

define internal <4 x i1> @SelV4x1Vcond(<4 x i1> %pc, <4 x i1> %pt, <4 x i1> %pe) {
entry:
  %r = select <4 x i1> %pc, <4 x i1> %pt, <4 x i1> %pe
  ret <4 x i1> %r
}

; CHECK-NEXT: define internal <4 x i1> @SelV4x1Vcond(<4 x i1> %pc, <4 x i1> %pt, <4 x i1> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = select <4 x i1> %pc, <4 x i1> %pt, <4 x i1> %pe
; CHECK-NEXT:   ret <4 x i1> %r
; CHECK-NEXT: }

define internal <16 x i8> @SelV16x8Vcond(<16 x i1> %pc, <16 x i8> %pt, <16 x i8> %pe) {
entry:
  %r = select <16 x i1> %pc, <16 x i8> %pt, <16 x i8> %pe
  ret <16 x i8> %r
}

; CHECK-NEXT: define internal <16 x i8> @SelV16x8Vcond(<16 x i1> %pc, <16 x i8> %pt, <16 x i8> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = select <16 x i1> %pc, <16 x i8> %pt, <16 x i8> %pe
; CHECK-NEXT:   ret <16 x i8> %r
; CHECK-NEXT: }

define internal <8 x i16> @SelV8x16Vcond(<8 x i1> %pc, <8 x i16> %pt, <8 x i16> %pe) {
entry:
  %r = select <8 x i1> %pc, <8 x i16> %pt, <8 x i16> %pe
  ret <8 x i16> %r
}

; CHECK-NEXT: define internal <8 x i16> @SelV8x16Vcond(<8 x i1> %pc, <8 x i16> %pt, <8 x i16> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = select <8 x i1> %pc, <8 x i16> %pt, <8 x i16> %pe
; CHECK-NEXT:   ret <8 x i16> %r
; CHECK-NEXT: }

define internal <4 x i32> @SelV4x32Vcond(<4 x i1> %pc, <4 x i32> %pt, <4 x i32> %pe) {
entry:
  %r = select <4 x i1> %pc, <4 x i32> %pt, <4 x i32> %pe
  ret <4 x i32> %r
}

; CHECK-NEXT: define internal <4 x i32> @SelV4x32Vcond(<4 x i1> %pc, <4 x i32> %pt, <4 x i32> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = select <4 x i1> %pc, <4 x i32> %pt, <4 x i32> %pe
; CHECK-NEXT:   ret <4 x i32> %r
; CHECK-NEXT: }

define internal <4 x float> @SelV4xfloatVcond(<4 x i1> %pc, <4 x float> %pt, <4 x float> %pe) {
entry:
  %r = select <4 x i1> %pc, <4 x float> %pt, <4 x float> %pe
  ret <4 x float> %r
}

; CHECK-NEXT: define internal <4 x float> @SelV4xfloatVcond(<4 x i1> %pc, <4 x float> %pt, <4 x float> %pe) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = select <4 x i1> %pc, <4 x float> %pt, <4 x float> %pe
; CHECK-NEXT:   ret <4 x float> %r
; CHECK-NEXT: }

; NOIR: Total across all functions
