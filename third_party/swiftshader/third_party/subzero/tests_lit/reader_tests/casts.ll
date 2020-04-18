; Tests if we can read cast operations.

; RUN: %p2i -i %s --insts --no-local-syms | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

; TODO(kschimpf) Find way to test pointer conversions (since they in general
; get removed by pnacl-freeze).

define internal i32 @TruncI64(i64 %v) {
  %v1 = trunc i64 %v to i1
  %v8 = trunc i64 %v to i8
  %v16 = trunc i64 %v to i16
  %v32 = trunc i64 %v to i32
  ret i32 %v32
}

; CHECK:      define internal i32 @TruncI64(i64 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i64 %__0 to i1
; CHECK-NEXT:   %__2 = trunc i64 %__0 to i8
; CHECK-NEXT:   %__3 = trunc i64 %__0 to i16
; CHECK-NEXT:   %__4 = trunc i64 %__0 to i32
; CHECK-NEXT:   ret i32 %__4
; CHECK-NEXT: }

define internal void @TruncI32(i32 %v) {
  %v1 = trunc i32 %v to i1
  %v8 = trunc i32 %v to i8
  %v16 = trunc i32 %v to i16
  ret void
}

; CHECK-NEXT: define internal void @TruncI32(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i1
; CHECK-NEXT:   %__2 = trunc i32 %__0 to i8
; CHECK-NEXT:   %__3 = trunc i32 %__0 to i16
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @TruncI16(i32 %p) {
  %v = trunc i32 %p to i16
  %v1 = trunc i16 %v to i1
  %v8 = trunc i16 %v to i8
  ret void
}

; CHECK-NEXT: define internal void @TruncI16(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i16
; CHECK-NEXT:   %__2 = trunc i16 %__1 to i1
; CHECK-NEXT:   %__3 = trunc i16 %__1 to i8
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @TruncI8(i32 %p) {
  %v = trunc i32 %p to i8
  %v1 = trunc i8 %v to i1
  ret void
}

; CHECK-NEXT: define internal void @TruncI8(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i8
; CHECK-NEXT:   %__2 = trunc i8 %__1 to i1
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal i64 @ZextI1(i32 %p) {
  %v = trunc i32 %p to i1
  %v8 = zext i1 %v to i8
  %v16 = zext i1 %v to i16
  %v32 = zext i1 %v to i32
  %v64 = zext i1 %v to i64
  ret i64 %v64
}

; CHECK-NEXT: define internal i64 @ZextI1(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i1
; CHECK-NEXT:   %__2 = zext i1 %__1 to i8
; CHECK-NEXT:   %__3 = zext i1 %__1 to i16
; CHECK-NEXT:   %__4 = zext i1 %__1 to i32
; CHECK-NEXT:   %__5 = zext i1 %__1 to i64
; CHECK-NEXT:   ret i64 %__5
; CHECK-NEXT: }

define internal i32 @ZextI8(i32 %p) {
  %v = trunc i32 %p to i8
  %v16 = zext i8 %v to i16
  %v32 = zext i8 %v to i32
  %v64 = zext i8 %v to i64
  ret i32 %v32
}

; CHECK-NEXT: define internal i32 @ZextI8(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i8
; CHECK-NEXT:   %__2 = zext i8 %__1 to i16
; CHECK-NEXT:   %__3 = zext i8 %__1 to i32
; CHECK-NEXT:   %__4 = zext i8 %__1 to i64
; CHECK-NEXT:   ret i32 %__3
; CHECK-NEXT: }

define internal i64 @ZextI16(i32 %p) {
  %v = trunc i32 %p to i16
  %v32 = zext i16 %v to i32
  %v64 = zext i16 %v to i64
  ret i64 %v64
}

; CHECK-NEXT: define internal i64 @ZextI16(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i16
; CHECK-NEXT:   %__2 = zext i16 %__1 to i32
; CHECK-NEXT:   %__3 = zext i16 %__1 to i64
; CHECK-NEXT:   ret i64 %__3
; CHECK-NEXT: }

define internal i64 @Zexti32(i32 %v) {
  %v64 = zext i32 %v to i64
  ret i64 %v64
}

; CHECK-NEXT: define internal i64 @Zexti32(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = zext i32 %__0 to i64
; CHECK-NEXT:   ret i64 %__1
; CHECK-NEXT: }

define internal i32 @SextI1(i32 %p) {
  %v = trunc i32 %p to i1
  %v8 = sext i1 %v to i8
  %v16 = sext i1 %v to i16
  %v32 = sext i1 %v to i32
  %v64 = sext i1 %v to i64
  ret i32 %v32
}

; CHECK-NEXT: define internal i32 @SextI1(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i1
; CHECK-NEXT:   %__2 = sext i1 %__1 to i8
; CHECK-NEXT:   %__3 = sext i1 %__1 to i16
; CHECK-NEXT:   %__4 = sext i1 %__1 to i32
; CHECK-NEXT:   %__5 = sext i1 %__1 to i64
; CHECK-NEXT:   ret i32 %__4
; CHECK-NEXT: }

define internal i64 @SextI8(i32 %p) {
  %v = trunc i32 %p to i8
  %v16 = sext i8 %v to i16
  %v32 = sext i8 %v to i32
  %v64 = sext i8 %v to i64
  ret i64 %v64
}

; CHECK-NEXT: define internal i64 @SextI8(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i8
; CHECK-NEXT:   %__2 = sext i8 %__1 to i16
; CHECK-NEXT:   %__3 = sext i8 %__1 to i32
; CHECK-NEXT:   %__4 = sext i8 %__1 to i64
; CHECK-NEXT:   ret i64 %__4
; CHECK-NEXT: }

define internal i32 @SextI16(i32 %p) {
  %v = trunc i32 %p to i16
  %v32 = sext i16 %v to i32
  %v64 = sext i16 %v to i64
  ret i32 %v32
}

; CHECK-NEXT: define internal i32 @SextI16(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i16
; CHECK-NEXT:   %__2 = sext i16 %__1 to i32
; CHECK-NEXT:   %__3 = sext i16 %__1 to i64
; CHECK-NEXT:   ret i32 %__2
; CHECK-NEXT: }

define internal i64 @Sexti32(i32 %v) {
  %v64 = sext i32 %v to i64
  ret i64 %v64
}

; CHECK-NEXT: define internal i64 @Sexti32(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = sext i32 %__0 to i64
; CHECK-NEXT:   ret i64 %__1
; CHECK-NEXT: }

define internal float @Fptrunc(double %v) {
  %vfloat = fptrunc double %v to float
  ret float %vfloat
}

; CHECK-NEXT: define internal float @Fptrunc(double %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = fptrunc double %__0 to float
; CHECK-NEXT:   ret float %__1
; CHECK-NEXT: }

define internal double @Fpext(float %v) {
  %vdouble = fpext float %v to double
  ret double %vdouble
}

; CHECK-NEXT: define internal double @Fpext(float %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = fpext float %__0 to double
; CHECK-NEXT:   ret double %__1
; CHECK-NEXT: }

define internal i32 @FptouiFloat(float %v) {
  %v1 = fptoui float %v to i1
  %v8 = fptoui float %v to i8
  %v16 = fptoui float %v to i16
  %v32 = fptoui float %v to i32
  %v64 = fptoui float %v to i64
  ret i32 %v32
}

; CHECK-NEXT: define internal i32 @FptouiFloat(float %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = fptoui float %__0 to i1
; CHECK-NEXT:   %__2 = fptoui float %__0 to i8
; CHECK-NEXT:   %__3 = fptoui float %__0 to i16
; CHECK-NEXT:   %__4 = fptoui float %__0 to i32
; CHECK-NEXT:   %__5 = fptoui float %__0 to i64
; CHECK-NEXT:   ret i32 %__4
; CHECK-NEXT: }

define internal i32 @FptouiDouble(double %v) {
  %v1 = fptoui double %v to i1
  %v8 = fptoui double %v to i8
  %v16 = fptoui double %v to i16
  %v32 = fptoui double %v to i32
  %v64 = fptoui double %v to i64
  ret i32 %v32
}

; CHECK-NEXT: define internal i32 @FptouiDouble(double %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = fptoui double %__0 to i1
; CHECK-NEXT:   %__2 = fptoui double %__0 to i8
; CHECK-NEXT:   %__3 = fptoui double %__0 to i16
; CHECK-NEXT:   %__4 = fptoui double %__0 to i32
; CHECK-NEXT:   %__5 = fptoui double %__0 to i64
; CHECK-NEXT:   ret i32 %__4
; CHECK-NEXT: }

define internal i32 @FptosiFloat(float %v) {
  %v1 = fptosi float %v to i1
  %v8 = fptosi float %v to i8
  %v16 = fptosi float %v to i16
  %v32 = fptosi float %v to i32
  %v64 = fptosi float %v to i64
  ret i32 %v32
}

; CHECK-NEXT: define internal i32 @FptosiFloat(float %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = fptosi float %__0 to i1
; CHECK-NEXT:   %__2 = fptosi float %__0 to i8
; CHECK-NEXT:   %__3 = fptosi float %__0 to i16
; CHECK-NEXT:   %__4 = fptosi float %__0 to i32
; CHECK-NEXT:   %__5 = fptosi float %__0 to i64
; CHECK-NEXT:   ret i32 %__4
; CHECK-NEXT: }

define internal i32 @FptosiDouble(double %v) {
  %v1 = fptosi double %v to i1
  %v8 = fptosi double %v to i8
  %v16 = fptosi double %v to i16
  %v32 = fptosi double %v to i32
  %v64 = fptosi double %v to i64
  ret i32 %v32
}

; CHECK-NEXT: define internal i32 @FptosiDouble(double %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = fptosi double %__0 to i1
; CHECK-NEXT:   %__2 = fptosi double %__0 to i8
; CHECK-NEXT:   %__3 = fptosi double %__0 to i16
; CHECK-NEXT:   %__4 = fptosi double %__0 to i32
; CHECK-NEXT:   %__5 = fptosi double %__0 to i64
; CHECK-NEXT:   ret i32 %__4
; CHECK-NEXT: }

define internal float @UitofpI1(i32 %p) {
  %v = trunc i32 %p to i1
  %vfloat = uitofp i1 %v to float
  %vdouble = uitofp i1 %v to double
  ret float %vfloat
}

; CHECK-NEXT: define internal float @UitofpI1(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i1
; CHECK-NEXT:   %__2 = uitofp i1 %__1 to float
; CHECK-NEXT:   %__3 = uitofp i1 %__1 to double
; CHECK-NEXT:   ret float %__2
; CHECK-NEXT: }

define internal float @UitofpI8(i32 %p) {
  %v = trunc i32 %p to i8
  %vfloat = uitofp i8 %v to float
  %vdouble = uitofp i8 %v to double
  ret float %vfloat
}

; CHECK-NEXT: define internal float @UitofpI8(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i8
; CHECK-NEXT:   %__2 = uitofp i8 %__1 to float
; CHECK-NEXT:   %__3 = uitofp i8 %__1 to double
; CHECK-NEXT:   ret float %__2
; CHECK-NEXT: }

define internal float @UitofpI16(i32 %p) {
  %v = trunc i32 %p to i16
  %vfloat = uitofp i16 %v to float
  %vdouble = uitofp i16 %v to double
  ret float %vfloat
}

; CHECK-NEXT: define internal float @UitofpI16(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i16
; CHECK-NEXT:   %__2 = uitofp i16 %__1 to float
; CHECK-NEXT:   %__3 = uitofp i16 %__1 to double
; CHECK-NEXT:   ret float %__2
; CHECK-NEXT: }

define internal float @UitofpI32(i32 %v) {
  %vfloat = uitofp i32 %v to float
  %vdouble = uitofp i32 %v to double
  ret float %vfloat
}

; CHECK-NEXT: define internal float @UitofpI32(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = uitofp i32 %__0 to float
; CHECK-NEXT:   %__2 = uitofp i32 %__0 to double
; CHECK-NEXT:   ret float %__1
; CHECK-NEXT: }

define internal float @UitofpI64(i64 %v) {
  %vfloat = uitofp i64 %v to float
  %vdouble = uitofp i64 %v to double
  ret float %vfloat
}

; CHECK-NEXT: define internal float @UitofpI64(i64 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = uitofp i64 %__0 to float
; CHECK-NEXT:   %__2 = uitofp i64 %__0 to double
; CHECK-NEXT:   ret float %__1
; CHECK-NEXT: }

define internal float @SitofpI1(i32 %p) {
  %v = trunc i32 %p to i1
  %vfloat = sitofp i1 %v to float
  %vdouble = sitofp i1 %v to double
  ret float %vfloat
}

; CHECK-NEXT: define internal float @SitofpI1(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i1
; CHECK-NEXT:   %__2 = sitofp i1 %__1 to float
; CHECK-NEXT:   %__3 = sitofp i1 %__1 to double
; CHECK-NEXT:   ret float %__2
; CHECK-NEXT: }

define internal float @SitofpI8(i32 %p) {
  %v = trunc i32 %p to i8
  %vfloat = sitofp i8 %v to float
  %vdouble = sitofp i8 %v to double
  ret float %vfloat
}

; CHECK-NEXT: define internal float @SitofpI8(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i8
; CHECK-NEXT:   %__2 = sitofp i8 %__1 to float
; CHECK-NEXT:   %__3 = sitofp i8 %__1 to double
; CHECK-NEXT:   ret float %__2
; CHECK-NEXT: }

define internal float @SitofpI16(i32 %p) {
  %v = trunc i32 %p to i16
  %vfloat = sitofp i16 %v to float
  %vdouble = sitofp i16 %v to double
  ret float %vfloat
}

; CHECK-NEXT: define internal float @SitofpI16(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = trunc i32 %__0 to i16
; CHECK-NEXT:   %__2 = sitofp i16 %__1 to float
; CHECK-NEXT:   %__3 = sitofp i16 %__1 to double
; CHECK-NEXT:   ret float %__2
; CHECK-NEXT: }

define internal float @SitofpI32(i32 %v) {
  %vfloat = sitofp i32 %v to float
  %vdouble = sitofp i32 %v to double
  ret float %vfloat
}

; CHECK-NEXT: define internal float @SitofpI32(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = sitofp i32 %__0 to float
; CHECK-NEXT:   %__2 = sitofp i32 %__0 to double
; CHECK-NEXT:   ret float %__1
; CHECK-NEXT: }

define internal float @SitofpI64(i64 %v) {
  %vfloat = sitofp i64 %v to float
  %vdouble = sitofp i64 %v to double
  ret float %vfloat
}

; CHECK-NEXT: define internal float @SitofpI64(i64 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = sitofp i64 %__0 to float
; CHECK-NEXT:   %__2 = sitofp i64 %__0 to double
; CHECK-NEXT:   ret float %__1
; CHECK-NEXT: }

define internal float @BitcastI32(i32 %v) {
  %vfloat = bitcast i32 %v to float
  ret float %vfloat
}

; CHECK-NEXT: define internal float @BitcastI32(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = bitcast i32 %__0 to float
; CHECK-NEXT:   ret float %__1
; CHECK-NEXT: }

define internal double @BitcastI64(i64 %v) {
  %vdouble = bitcast i64 %v to double
  ret double %vdouble
}

; CHECK-NEXT: define internal double @BitcastI64(i64 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = bitcast i64 %__0 to double
; CHECK-NEXT:   ret double %__1
; CHECK-NEXT: }

define internal i32 @BitcastFloat(float %v) {
  %vi32 = bitcast float %v to i32
  ret i32 %vi32
}

; CHECK-NEXT: define internal i32 @BitcastFloat(float %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = bitcast float %__0 to i32
; CHECK-NEXT:   ret i32 %__1
; CHECK-NEXT: }

define internal i64 @BitcastDouble(double %v) {
  %vi64 = bitcast double %v to i64
  ret i64 %vi64
}

; CHECK-NEXT: define internal i64 @BitcastDouble(double %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = bitcast double %__0 to i64
; CHECK-NEXT:   ret i64 %__1
; CHECK-NEXT: }

define internal void @BitcastV4xFloat(<4 x float> %v) {
  %v4xi32 = bitcast <4 x float> %v to <4 x i32>
  %v8xi16 = bitcast <4 x float> %v to <8 x i16>
  %v16xi8 = bitcast <4 x float> %v to <16 x i8>
  ret void
}

; CHECK-NEXT: define internal void @BitcastV4xFloat(<4 x float> %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = bitcast <4 x float> %__0 to <4 x i32>
; CHECK-NEXT:   %__2 = bitcast <4 x float> %__0 to <8 x i16>
; CHECK-NEXT:   %__3 = bitcast <4 x float> %__0 to <16 x i8>
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @BitcastV4xi32(<4 x i32> %v) {
  %v4xfloat = bitcast <4 x i32> %v to <4 x float>
  %v8xi16 = bitcast <4 x i32> %v to <8 x i16>
  %v16xi8 = bitcast <4 x i32> %v to <16 x i8>
  ret void
}

; CHECK-NEXT: define internal void @BitcastV4xi32(<4 x i32> %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = bitcast <4 x i32> %__0 to <4 x float>
; CHECK-NEXT:   %__2 = bitcast <4 x i32> %__0 to <8 x i16>
; CHECK-NEXT:   %__3 = bitcast <4 x i32> %__0 to <16 x i8>
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @BitcastV8xi16(<8 x i16> %v) {
  %v4xfloat = bitcast <8 x i16> %v to <4 x float>
  %v4xi32 = bitcast <8 x i16> %v to <4 x i32>
  %v16xi8 = bitcast <8 x i16> %v to <16 x i8>
  ret void
}

; CHECK-NEXT: define internal void @BitcastV8xi16(<8 x i16> %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = bitcast <8 x i16> %__0 to <4 x float>
; CHECK-NEXT:   %__2 = bitcast <8 x i16> %__0 to <4 x i32>
; CHECK-NEXT:   %__3 = bitcast <8 x i16> %__0 to <16 x i8>
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @BitcastV16xi8(<16 x i8> %v) {
  %v4xfloat = bitcast <16 x i8> %v to <4 x float>
  %v4xi32 = bitcast <16 x i8> %v to <4 x i32>
  %v8xi16 = bitcast <16 x i8> %v to <8 x i16>
  ret void
}

; CHECK-NEXT: define internal void @BitcastV16xi8(<16 x i8> %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   %__1 = bitcast <16 x i8> %__0 to <4 x float>
; CHECK-NEXT:   %__2 = bitcast <16 x i8> %__0 to <4 x i32>
; CHECK-NEXT:   %__3 = bitcast <16 x i8> %__0 to <8 x i16>
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; NOIR: Total across all functions
