; Test if we can read load instructions.

; RUN: %p2i --no-local-syms -i %s --insts | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

define internal i32 @load_i8(i32 %addr) {
entry:
  %addr_i8 = inttoptr i32 %addr to i8*
  %v = load i8, i8* %addr_i8, align 1
  %r = sext i8 %v to i32
  ret i32 %r

; CHECK:      __0:
; CHECK-NEXT:   %__1 = load i8, i8* %__0, align 1
; CHECK-NEXT:   %__2 = sext i8 %__1 to i32
; CHECK-NEXT:   ret i32 %__2
}

define internal i32 @load_i16(i32 %addr) {
entry:
  %addr_i16 = inttoptr i32 %addr to i16*
  %v = load i16, i16* %addr_i16, align 1
  %r = sext i16 %v to i32
  ret i32 %r

; CHECK:      __0:
; CHECK-NEXT:   %__1 = load i16, i16* %__0, align 1
; CHECK-NEXT:   %__2 = sext i16 %__1 to i32
; CHECK-NEXT:   ret i32 %__2
}

define internal i32 @load_i32(i32 %addr) {
entry:
  %addr_i32 = inttoptr i32 %addr to i32*
  %v = load i32, i32* %addr_i32, align 1
  ret i32 %v

; CHECK:      __0:
; CHECK-NEXT:   %__1 = load i32, i32* %__0, align 1
; CHECK-NEXT:   ret i32 %__1
}

define internal i64 @load_i64(i32 %addr) {
entry:
  %addr_i64 = inttoptr i32 %addr to i64*
  %v = load i64, i64* %addr_i64, align 1
  ret i64 %v

; CHECK:      __0:
; CHECK-NEXT:   %__1 = load i64, i64* %__0, align 1
; CHECK-NEXT:   ret i64 %__1
}

define internal float @load_float_a1(i32 %addr) {
entry:
  %addr_float = inttoptr i32 %addr to float*
  %v = load float, float* %addr_float, align 1
  ret float %v

; TODO(kschimpf) Fix load alignment in ICE to allow non-default.

; CHECK:      __0:
; CHECK-NEXT:   %__1 = load float, float* %__0, align 4
; CHECK-NEXT:   ret float %__1
}


define internal float @load_float_a4(i32 %addr) {
entry:
  %addr_float = inttoptr i32 %addr to float*
  %v = load float, float* %addr_float, align 4
  ret float %v

; CHECK:       __0:
; CHECK-NEXT:   %__1 = load float, float* %__0, align 4
; CHECK-NEXT:   ret float %__1
}

define internal double @load_double_a1(i32 %addr) {
entry:
  %addr_double = inttoptr i32 %addr to double*
  %v = load double, double* %addr_double, align 1
  ret double %v

; TODO(kschimpf) Fix load alignment in ICE to allow non-default.

; CHECK:      __0:
; CHECK-NEXT:   %__1 = load double, double* %__0, align 8
; CHECK-NEXT:   ret double %__1
}


define internal double @load_double_a8(i32 %addr) {
entry:
  %addr_double = inttoptr i32 %addr to double*
  %v = load double, double* %addr_double, align 8
  ret double %v

; CHECK:      __0:
; CHECK-NEXT:   %__1 = load double, double* %__0, align 8
; CHECK-NEXT:   ret double %__1
}

define internal <16 x i8> @load_v16xI8(i32 %addr) {
entry:
  %addr_v16xI8 = inttoptr i32 %addr to <16 x i8>*
  %v = load <16 x i8>, <16 x i8>* %addr_v16xI8, align 1
  ret <16 x i8> %v

; CHECK:      __0:
; CHECK-NEXT:   %__1 = load <16 x i8>, <16 x i8>* %__0, align 1
; CHECK-NEXT:   ret <16 x i8> %__1
}

define internal <8 x i16> @load_v8xI16(i32 %addr) {
entry:
  %addr_v8xI16 = inttoptr i32 %addr to <8 x i16>*
  %v = load <8 x i16>, <8 x i16>* %addr_v8xI16, align 2
  ret <8 x i16> %v

; CHECK:      __0:
; CHECK-NEXT:   %__1 = load <8 x i16>, <8 x i16>* %__0, align 2
; CHECK-NEXT:   ret <8 x i16> %__1
}

define internal <4 x i32> @load_v4xI32(i32 %addr) {
entry:
  %addr_v4xI32 = inttoptr i32 %addr to <4 x i32>*
  %v = load <4 x i32>, <4 x i32>* %addr_v4xI32, align 4
  ret <4 x i32> %v

; CHECK:      __0:
; CHECK-NEXT:   %__1 = load <4 x i32>, <4 x i32>* %__0, align 4
; CHECK-NEXT:   ret <4 x i32> %__1
}

define internal <4 x float> @load_v4xFloat(i32 %addr) {
entry:
  %addr_v4xFloat = inttoptr i32 %addr to <4 x float>*
  %v = load <4 x float>, <4 x float>* %addr_v4xFloat, align 4
  ret <4 x float> %v

; CHECK:      __0:
; CHECK-NEXT:   %__1 = load <4 x float>, <4 x float>* %__0, align 4
; CHECK-NEXT:   ret <4 x float> %__1
}

; NOIR: Total across all functions
