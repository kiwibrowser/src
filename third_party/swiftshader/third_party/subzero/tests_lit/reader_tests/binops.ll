; Tests if we can read binary operators.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN: %l2i -i %s --insts | %ifl FileCheck %s
; RUN: %lc2i -i %s --insts | %iflc FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

; TODO(kschimpf): add i8/i16. Needs bitcasts.

define internal i32 @AddI32(i32 %a, i32 %b) {
entry:
  %add = add i32 %b, %a
  ret i32 %add
}

; CHECK:      define internal i32 @AddI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %add = add i32 %b, %a
; CHECK-NEXT:   ret i32 %add
; CHECK-NEXT: }

define internal i64 @AddI64(i64 %a, i64 %b) {
entry:
  %add = add i64 %b, %a
  ret i64 %add
}

; CHECK-NEXT: define internal i64 @AddI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %add = add i64 %b, %a
; CHECK-NEXT:   ret i64 %add
; CHECK-NEXT: }

define internal <16 x i8> @AddV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %add = add <16 x i8> %b, %a
  ret <16 x i8> %add
}

; CHECK-NEXT: define internal <16 x i8> @AddV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %add = add <16 x i8> %b, %a
; CHECK-NEXT:   ret <16 x i8> %add
; CHECK-NEXT: }

define internal <8 x i16> @AddV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %add = add <8 x i16> %b, %a
  ret <8 x i16> %add
}

; CHECK-NEXT: define internal <8 x i16> @AddV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %add = add <8 x i16> %b, %a
; CHECK-NEXT:   ret <8 x i16> %add
; CHECK-NEXT: }

define internal <4 x i32> @AddV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %add = add <4 x i32> %b, %a
  ret <4 x i32> %add
}

; CHECK-NEXT: define internal <4 x i32> @AddV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %add = add <4 x i32> %b, %a
; CHECK-NEXT:   ret <4 x i32> %add
; CHECK-NEXT: }

define internal float @AddFloat(float %a, float %b) {
entry:
  %add = fadd float %b, %a
  ret float %add
}

; CHECK-NEXT: define internal float @AddFloat(float %a, float %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %add = fadd float %b, %a
; CHECK-NEXT:   ret float %add
; CHECK-NEXT: }

define internal double @AddDouble(double %a, double %b) {
entry:
  %add = fadd double %b, %a
  ret double %add
}

; CHECK-NEXT: define internal double @AddDouble(double %a, double %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %add = fadd double %b, %a
; CHECK-NEXT:   ret double %add
; CHECK-NEXT: }

define internal <4 x float> @AddV4Float(<4 x float> %a, <4 x float> %b) {
entry:
  %add = fadd <4 x float> %b, %a
  ret <4 x float> %add
}

; CHECK-NEXT: define internal <4 x float> @AddV4Float(<4 x float> %a, <4 x float> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %add = fadd <4 x float> %b, %a
; CHECK-NEXT:   ret <4 x float> %add
; CHECK-NEXT: }

; TODO(kschimpf): sub i8/i16. Needs bitcasts.

define internal i32 @SubI32(i32 %a, i32 %b) {
entry:
  %sub = sub i32 %a, %b
  ret i32 %sub
}

; CHECK-NEXT: define internal i32 @SubI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %sub = sub i32 %a, %b
; CHECK-NEXT:   ret i32 %sub
; CHECK-NEXT: }

define internal i64 @SubI64(i64 %a, i64 %b) {
entry:
  %sub = sub i64 %a, %b
  ret i64 %sub
}

; CHECK-NEXT: define internal i64 @SubI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %sub = sub i64 %a, %b
; CHECK-NEXT:   ret i64 %sub
; CHECK-NEXT: }

define internal <16 x i8> @SubV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %sub = sub <16 x i8> %a, %b
  ret <16 x i8> %sub
}

; CHECK-NEXT: define internal <16 x i8> @SubV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %sub = sub <16 x i8> %a, %b
; CHECK-NEXT:   ret <16 x i8> %sub
; CHECK-NEXT: }

define internal <8 x i16> @SubV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %sub = sub <8 x i16> %a, %b
  ret <8 x i16> %sub
}

; CHECK-NEXT: define internal <8 x i16> @SubV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %sub = sub <8 x i16> %a, %b
; CHECK-NEXT:   ret <8 x i16> %sub
; CHECK-NEXT: }

define internal <4 x i32> @SubV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %sub = sub <4 x i32> %a, %b
  ret <4 x i32> %sub
}

; CHECK-NEXT: define internal <4 x i32> @SubV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %sub = sub <4 x i32> %a, %b
; CHECK-NEXT:   ret <4 x i32> %sub
; CHECK-NEXT: }

define internal float @SubFloat(float %a, float %b) {
entry:
  %sub = fsub float %a, %b
  ret float %sub
}

; CHECK-NEXT: define internal float @SubFloat(float %a, float %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %sub = fsub float %a, %b
; CHECK-NEXT:   ret float %sub
; CHECK-NEXT: }

define internal double @SubDouble(double %a, double %b) {
entry:
  %sub = fsub double %a, %b
  ret double %sub
}

; CHECK-NEXT: define internal double @SubDouble(double %a, double %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %sub = fsub double %a, %b
; CHECK-NEXT:   ret double %sub
; CHECK-NEXT: }

define internal <4 x float> @SubV4Float(<4 x float> %a, <4 x float> %b) {
entry:
  %sub = fsub <4 x float> %a, %b
  ret <4 x float> %sub
}

; CHECK-NEXT: define internal <4 x float> @SubV4Float(<4 x float> %a, <4 x float> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %sub = fsub <4 x float> %a, %b
; CHECK-NEXT:   ret <4 x float> %sub
; CHECK-NEXT: }

; TODO(kschimpf): mul i8/i16. Needs bitcasts.

define internal i32 @MulI32(i32 %a, i32 %b) {
entry:
  %mul = mul i32 %b, %a
  ret i32 %mul
}

; CHECK-NEXT: define internal i32 @MulI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %mul = mul i32 %b, %a
; CHECK-NEXT:   ret i32 %mul
; CHECK-NEXT: }

define internal i64 @MulI64(i64 %a, i64 %b) {
entry:
  %mul = mul i64 %b, %a
  ret i64 %mul
}

; CHECK-NEXT: define internal i64 @MulI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %mul = mul i64 %b, %a
; CHECK-NEXT:   ret i64 %mul
; CHECK-NEXT: }

define internal <16 x i8> @MulV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %mul = mul <16 x i8> %b, %a
  ret <16 x i8> %mul
}

; CHECK-NEXT: define internal <16 x i8> @MulV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %mul = mul <16 x i8> %b, %a
; CHECK-NEXT:   ret <16 x i8> %mul
; CHECK-NEXT: }

define internal float @MulFloat(float %a, float %b) {
entry:
  %mul = fmul float %b, %a
  ret float %mul
}

; CHECK-NEXT: define internal float @MulFloat(float %a, float %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %mul = fmul float %b, %a
; CHECK-NEXT:   ret float %mul
; CHECK-NEXT: }

define internal double @MulDouble(double %a, double %b) {
entry:
  %mul = fmul double %b, %a
  ret double %mul
}

; CHECK-NEXT: define internal double @MulDouble(double %a, double %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %mul = fmul double %b, %a
; CHECK-NEXT:   ret double %mul
; CHECK-NEXT: }

define internal <4 x float> @MulV4Float(<4 x float> %a, <4 x float> %b) {
entry:
  %mul = fmul <4 x float> %b, %a
  ret <4 x float> %mul
}

; CHECK-NEXT: define internal <4 x float> @MulV4Float(<4 x float> %a, <4 x float> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %mul = fmul <4 x float> %b, %a
; CHECK-NEXT:   ret <4 x float> %mul
; CHECK-NEXT: }

; TODO(kschimpf): sdiv i8/i16. Needs bitcasts.

define internal i32 @SdivI32(i32 %a, i32 %b) {
entry:
  %div = sdiv i32 %a, %b
  ret i32 %div
}

; CHECK-NEXT: define internal i32 @SdivI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = sdiv i32 %a, %b
; CHECK-NEXT:   ret i32 %div
; CHECK-NEXT: }

define internal i64 @SdivI64(i64 %a, i64 %b) {
entry:
  %div = sdiv i64 %a, %b
  ret i64 %div
}

; CHECK-NEXT: define internal i64 @SdivI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = sdiv i64 %a, %b
; CHECK-NEXT:   ret i64 %div
; CHECK-NEXT: }

define internal <16 x i8> @SdivV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %div = sdiv <16 x i8> %a, %b
  ret <16 x i8> %div
}

; CHECK-NEXT: define internal <16 x i8> @SdivV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = sdiv <16 x i8> %a, %b
; CHECK-NEXT:   ret <16 x i8> %div
; CHECK-NEXT: }

define internal <8 x i16> @SdivV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %div = sdiv <8 x i16> %a, %b
  ret <8 x i16> %div
}

; CHECK-NEXT: define internal <8 x i16> @SdivV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = sdiv <8 x i16> %a, %b
; CHECK-NEXT:   ret <8 x i16> %div
; CHECK-NEXT: }

define internal <4 x i32> @SdivV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %div = sdiv <4 x i32> %a, %b
  ret <4 x i32> %div
}

; CHECK-NEXT: define internal <4 x i32> @SdivV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = sdiv <4 x i32> %a, %b
; CHECK-NEXT:   ret <4 x i32> %div
; CHECK-NEXT: }

; TODO(kschimpf): srem i8/i16. Needs bitcasts.

define internal i32 @SremI32(i32 %a, i32 %b) {
entry:
  %rem = srem i32 %a, %b
  ret i32 %rem
}

; CHECK-NEXT: define internal i32 @SremI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = srem i32 %a, %b
; CHECK-NEXT:   ret i32 %rem
; CHECK-NEXT: }

define internal i64 @SremI64(i64 %a, i64 %b) {
entry:
  %rem = srem i64 %a, %b
  ret i64 %rem
}

; CHECK-NEXT: define internal i64 @SremI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = srem i64 %a, %b
; CHECK-NEXT:   ret i64 %rem
; CHECK-NEXT: }

define internal <16 x i8> @SremV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %rem = srem <16 x i8> %a, %b
  ret <16 x i8> %rem
}

; CHECK-NEXT: define internal <16 x i8> @SremV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = srem <16 x i8> %a, %b
; CHECK-NEXT:   ret <16 x i8> %rem
; CHECK-NEXT: }

define internal <8 x i16> @SremV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %rem = srem <8 x i16> %a, %b
  ret <8 x i16> %rem
}

; CHECK-NEXT: define internal <8 x i16> @SremV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = srem <8 x i16> %a, %b
; CHECK-NEXT:   ret <8 x i16> %rem
; CHECK-NEXT: }

define internal <4 x i32> @SremV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %rem = srem <4 x i32> %a, %b
  ret <4 x i32> %rem
}

; CHECK-NEXT: define internal <4 x i32> @SremV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = srem <4 x i32> %a, %b
; CHECK-NEXT:   ret <4 x i32> %rem
; CHECK-NEXT: }

; TODO(kschimpf): udiv i8/i16. Needs bitcasts.

define internal i32 @UdivI32(i32 %a, i32 %b) {
entry:
  %div = udiv i32 %a, %b
  ret i32 %div
}

; CHECK-NEXT: define internal i32 @UdivI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = udiv i32 %a, %b
; CHECK-NEXT:   ret i32 %div
; CHECK-NEXT: }

define internal i64 @UdivI64(i64 %a, i64 %b) {
entry:
  %div = udiv i64 %a, %b
  ret i64 %div
}

; CHECK-NEXT: define internal i64 @UdivI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = udiv i64 %a, %b
; CHECK-NEXT:   ret i64 %div
; CHECK-NEXT: }

define internal <16 x i8> @UdivV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %div = udiv <16 x i8> %a, %b
  ret <16 x i8> %div
}

; CHECK-NEXT: define internal <16 x i8> @UdivV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = udiv <16 x i8> %a, %b
; CHECK-NEXT:   ret <16 x i8> %div
; CHECK-NEXT: }

define internal <8 x i16> @UdivV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %div = udiv <8 x i16> %a, %b
  ret <8 x i16> %div
}

; CHECK-NEXT: define internal <8 x i16> @UdivV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = udiv <8 x i16> %a, %b
; CHECK-NEXT:   ret <8 x i16> %div
; CHECK-NEXT: }

define internal <4 x i32> @UdivV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %div = udiv <4 x i32> %a, %b
  ret <4 x i32> %div
}

; CHECK-NEXT: define internal <4 x i32> @UdivV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = udiv <4 x i32> %a, %b
; CHECK-NEXT:   ret <4 x i32> %div
; CHECK-NEXT: }

; TODO(kschimpf): urem i8/i16. Needs bitcasts.

define internal i32 @UremI32(i32 %a, i32 %b) {
entry:
  %rem = urem i32 %a, %b
  ret i32 %rem
}

; CHECK-NEXT: define internal i32 @UremI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = urem i32 %a, %b
; CHECK-NEXT:   ret i32 %rem
; CHECK-NEXT: }

define internal i64 @UremI64(i64 %a, i64 %b) {
entry:
  %rem = urem i64 %a, %b
  ret i64 %rem
}

; CHECK-NEXT: define internal i64 @UremI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = urem i64 %a, %b
; CHECK-NEXT:   ret i64 %rem
; CHECK-NEXT: }

define internal <16 x i8> @UremV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %rem = urem <16 x i8> %a, %b
  ret <16 x i8> %rem
}

; CHECK-NEXT: define internal <16 x i8> @UremV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = urem <16 x i8> %a, %b
; CHECK-NEXT:   ret <16 x i8> %rem
; CHECK-NEXT: }

define internal <8 x i16> @UremV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %rem = urem <8 x i16> %a, %b
  ret <8 x i16> %rem
}

; CHECK-NEXT: define internal <8 x i16> @UremV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = urem <8 x i16> %a, %b
; CHECK-NEXT:   ret <8 x i16> %rem
; CHECK-NEXT: }

define internal <4 x i32> @UremV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %rem = urem <4 x i32> %a, %b
  ret <4 x i32> %rem
}

; CHECK-NEXT: define internal <4 x i32> @UremV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = urem <4 x i32> %a, %b
; CHECK-NEXT:   ret <4 x i32> %rem
; CHECK-NEXT: }

define internal float @fdivFloat(float %a, float %b) {
entry:
  %div = fdiv float %a, %b
  ret float %div
}

; CHECK-NEXT: define internal float @fdivFloat(float %a, float %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = fdiv float %a, %b
; CHECK-NEXT:   ret float %div
; CHECK-NEXT: }

define internal double @fdivDouble(double %a, double %b) {
entry:
  %div = fdiv double %a, %b
  ret double %div
}

; CHECK-NEXT: define internal double @fdivDouble(double %a, double %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = fdiv double %a, %b
; CHECK-NEXT:   ret double %div
; CHECK-NEXT: }

define internal <4 x float> @fdivV4Float(<4 x float> %a, <4 x float> %b) {
entry:
  %div = fdiv <4 x float> %a, %b
  ret <4 x float> %div
}

; CHECK-NEXT: define internal <4 x float> @fdivV4Float(<4 x float> %a, <4 x float> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %div = fdiv <4 x float> %a, %b
; CHECK-NEXT:   ret <4 x float> %div
; CHECK-NEXT: }

define internal float @fremFloat(float %a, float %b) {
entry:
  %rem = frem float %a, %b
  ret float %rem
}

; CHECK-NEXT: define internal float @fremFloat(float %a, float %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = frem float %a, %b
; CHECK-NEXT:   ret float %rem
; CHECK-NEXT: }

define internal double @fremDouble(double %a, double %b) {
entry:
  %rem = frem double %a, %b
  ret double %rem
}

; CHECK-NEXT: define internal double @fremDouble(double %a, double %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = frem double %a, %b
; CHECK-NEXT:   ret double %rem
; CHECK-NEXT: }

define internal <4 x float> @fremV4Float(<4 x float> %a, <4 x float> %b) {
entry:
  %rem = frem <4 x float> %a, %b
  ret <4 x float> %rem
}

; CHECK-NEXT: define internal <4 x float> @fremV4Float(<4 x float> %a, <4 x float> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %rem = frem <4 x float> %a, %b
; CHECK-NEXT:   ret <4 x float> %rem
; CHECK-NEXT: }

; TODO(kschimpf): and i1/i8/i16. Needs bitcasts.

define internal i32 @AndI32(i32 %a, i32 %b) {
entry:
  %and = and i32 %b, %a
  ret i32 %and
}

; CHECK-NEXT: define internal i32 @AndI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %and = and i32 %b, %a
; CHECK-NEXT:   ret i32 %and
; CHECK-NEXT: }

define internal i64 @AndI64(i64 %a, i64 %b) {
entry:
  %and = and i64 %b, %a
  ret i64 %and
}

; CHECK-NEXT: define internal i64 @AndI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %and = and i64 %b, %a
; CHECK-NEXT:   ret i64 %and
; CHECK-NEXT: }

define internal <16 x i8> @AndV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %and = and <16 x i8> %b, %a
  ret <16 x i8> %and
}

; CHECK-NEXT: define internal <16 x i8> @AndV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %and = and <16 x i8> %b, %a
; CHECK-NEXT:   ret <16 x i8> %and
; CHECK-NEXT: }

define internal <8 x i16> @AndV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %and = and <8 x i16> %b, %a
  ret <8 x i16> %and
}

; CHECK-NEXT: define internal <8 x i16> @AndV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %and = and <8 x i16> %b, %a
; CHECK-NEXT:   ret <8 x i16> %and
; CHECK-NEXT: }

define internal <4 x i32> @AndV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %and = and <4 x i32> %b, %a
  ret <4 x i32> %and
}

; CHECK-NEXT: define internal <4 x i32> @AndV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %and = and <4 x i32> %b, %a
; CHECK-NEXT:   ret <4 x i32> %and
; CHECK-NEXT: }

; TODO(kschimpf): or i1/i8/i16. Needs bitcasts.

define internal i32 @OrI32(i32 %a, i32 %b) {
entry:
  %or = or i32 %b, %a
  ret i32 %or
}

; CHECK-NEXT: define internal i32 @OrI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %or = or i32 %b, %a
; CHECK-NEXT:   ret i32 %or
; CHECK-NEXT: }

define internal i64 @OrI64(i64 %a, i64 %b) {
entry:
  %or = or i64 %b, %a
  ret i64 %or
}

; CHECK-NEXT: define internal i64 @OrI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %or = or i64 %b, %a
; CHECK-NEXT:   ret i64 %or
; CHECK-NEXT: }

define internal <16 x i8> @OrV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %or = or <16 x i8> %b, %a
  ret <16 x i8> %or
}

; CHECK-NEXT: define internal <16 x i8> @OrV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %or = or <16 x i8> %b, %a
; CHECK-NEXT:   ret <16 x i8> %or
; CHECK-NEXT: }

define internal <8 x i16> @OrV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %or = or <8 x i16> %b, %a
  ret <8 x i16> %or
}

; CHECK-NEXT: define internal <8 x i16> @OrV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %or = or <8 x i16> %b, %a
; CHECK-NEXT:   ret <8 x i16> %or
; CHECK-NEXT: }

define internal <4 x i32> @OrV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %or = or <4 x i32> %b, %a
  ret <4 x i32> %or
}

; CHECK-NEXT: define internal <4 x i32> @OrV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %or = or <4 x i32> %b, %a
; CHECK-NEXT:   ret <4 x i32> %or
; CHECK-NEXT: }

; TODO(kschimpf): xor i1/i8/i16. Needs bitcasts.

define internal i32 @XorI32(i32 %a, i32 %b) {
entry:
  %xor = xor i32 %b, %a
  ret i32 %xor
}

; CHECK-NEXT: define internal i32 @XorI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %xor = xor i32 %b, %a
; CHECK-NEXT:   ret i32 %xor
; CHECK-NEXT: }

define internal i64 @XorI64(i64 %a, i64 %b) {
entry:
  %xor = xor i64 %b, %a
  ret i64 %xor
}

; CHECK-NEXT: define internal i64 @XorI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %xor = xor i64 %b, %a
; CHECK-NEXT:   ret i64 %xor
; CHECK-NEXT: }

define internal <16 x i8> @XorV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %xor = xor <16 x i8> %b, %a
  ret <16 x i8> %xor
}

; CHECK-NEXT: define internal <16 x i8> @XorV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %xor = xor <16 x i8> %b, %a
; CHECK-NEXT:   ret <16 x i8> %xor
; CHECK-NEXT: }

define internal <8 x i16> @XorV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %xor = xor <8 x i16> %b, %a
  ret <8 x i16> %xor
}

; CHECK-NEXT: define internal <8 x i16> @XorV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %xor = xor <8 x i16> %b, %a
; CHECK-NEXT:   ret <8 x i16> %xor
; CHECK-NEXT: }

define internal <4 x i32> @XorV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %xor = xor <4 x i32> %b, %a
  ret <4 x i32> %xor
}

; CHECK-NEXT: define internal <4 x i32> @XorV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %xor = xor <4 x i32> %b, %a
; CHECK-NEXT:   ret <4 x i32> %xor
; CHECK-NEXT: }

; TODO(kschimpf): shl i8/i16. Needs bitcasts.

define internal i32 @ShlI32(i32 %a, i32 %b) {
entry:
  %shl = shl i32 %b, %a
  ret i32 %shl
}

; CHECK-NEXT: define internal i32 @ShlI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %shl = shl i32 %b, %a
; CHECK-NEXT:   ret i32 %shl
; CHECK-NEXT: }

define internal i64 @ShlI64(i64 %a, i64 %b) {
entry:
  %shl = shl i64 %b, %a
  ret i64 %shl
}

; CHECK-NEXT: define internal i64 @ShlI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %shl = shl i64 %b, %a
; CHECK-NEXT:   ret i64 %shl
; CHECK-NEXT: }

define internal <16 x i8> @ShlV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %shl = shl <16 x i8> %b, %a
  ret <16 x i8> %shl
}

; CHECK-NEXT: define internal <16 x i8> @ShlV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %shl = shl <16 x i8> %b, %a
; CHECK-NEXT:   ret <16 x i8> %shl
; CHECK-NEXT: }

define internal <8 x i16> @ShlV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %shl = shl <8 x i16> %b, %a
  ret <8 x i16> %shl
}

; CHECK-NEXT: define internal <8 x i16> @ShlV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %shl = shl <8 x i16> %b, %a
; CHECK-NEXT:   ret <8 x i16> %shl
; CHECK-NEXT: }

define internal <4 x i32> @ShlV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %shl = shl <4 x i32> %b, %a
  ret <4 x i32> %shl
}

; CHECK-NEXT: define internal <4 x i32> @ShlV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %shl = shl <4 x i32> %b, %a
; CHECK-NEXT:   ret <4 x i32> %shl
; CHECK-NEXT: }

; TODO(kschimpf): ashr i8/i16. Needs bitcasts.

define internal i32 @ashrI32(i32 %a, i32 %b) {
entry:
  %ashr = ashr i32 %b, %a
  ret i32 %ashr
}

; CHECK-NEXT: define internal i32 @ashrI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %ashr = ashr i32 %b, %a
; CHECK-NEXT:   ret i32 %ashr
; CHECK-NEXT: }

define internal i64 @AshrI64(i64 %a, i64 %b) {
entry:
  %ashr = ashr i64 %b, %a
  ret i64 %ashr
}

; CHECK-NEXT: define internal i64 @AshrI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %ashr = ashr i64 %b, %a
; CHECK-NEXT:   ret i64 %ashr
; CHECK-NEXT: }

define internal <16 x i8> @AshrV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %ashr = ashr <16 x i8> %b, %a
  ret <16 x i8> %ashr
}

; CHECK-NEXT: define internal <16 x i8> @AshrV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %ashr = ashr <16 x i8> %b, %a
; CHECK-NEXT:   ret <16 x i8> %ashr
; CHECK-NEXT: }

define internal <8 x i16> @AshrV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %ashr = ashr <8 x i16> %b, %a
  ret <8 x i16> %ashr
}

; CHECK-NEXT: define internal <8 x i16> @AshrV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %ashr = ashr <8 x i16> %b, %a
; CHECK-NEXT:   ret <8 x i16> %ashr
; CHECK-NEXT: }

define internal <4 x i32> @AshrV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %ashr = ashr <4 x i32> %b, %a
  ret <4 x i32> %ashr
}

; CHECK-NEXT: define internal <4 x i32> @AshrV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %ashr = ashr <4 x i32> %b, %a
; CHECK-NEXT:   ret <4 x i32> %ashr
; CHECK-NEXT: }

; TODO(kschimpf): lshr i8/i16. Needs bitcasts.

define internal i32 @lshrI32(i32 %a, i32 %b) {
entry:
  %lshr = lshr i32 %b, %a
  ret i32 %lshr
}

; CHECK-NEXT: define internal i32 @lshrI32(i32 %a, i32 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %lshr = lshr i32 %b, %a
; CHECK-NEXT:   ret i32 %lshr
; CHECK-NEXT: }

define internal i64 @LshrI64(i64 %a, i64 %b) {
entry:
  %lshr = lshr i64 %b, %a
  ret i64 %lshr
}

; CHECK-NEXT: define internal i64 @LshrI64(i64 %a, i64 %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %lshr = lshr i64 %b, %a
; CHECK-NEXT:   ret i64 %lshr
; CHECK-NEXT: }

define internal <16 x i8> @LshrV16I8(<16 x i8> %a, <16 x i8> %b) {
entry:
  %lshr = lshr <16 x i8> %b, %a
  ret <16 x i8> %lshr
}

; CHECK-NEXT: define internal <16 x i8> @LshrV16I8(<16 x i8> %a, <16 x i8> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %lshr = lshr <16 x i8> %b, %a
; CHECK-NEXT:   ret <16 x i8> %lshr
; CHECK-NEXT: }

define internal <8 x i16> @LshrV8I16(<8 x i16> %a, <8 x i16> %b) {
entry:
  %lshr = lshr <8 x i16> %b, %a
  ret <8 x i16> %lshr
}

; CHECK-NEXT: define internal <8 x i16> @LshrV8I16(<8 x i16> %a, <8 x i16> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %lshr = lshr <8 x i16> %b, %a
; CHECK-NEXT:   ret <8 x i16> %lshr
; CHECK-NEXT: }

define internal <4 x i32> @LshrV4I32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %lshr = lshr <4 x i32> %b, %a
  ret <4 x i32> %lshr
}

; CHECK-NEXT: define internal <4 x i32> @LshrV4I32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %lshr = lshr <4 x i32> %b, %a
; CHECK-NEXT:   ret <4 x i32> %lshr
; CHECK-NEXT: }

; NOIR: Total across all functions
