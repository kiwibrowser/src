; Test if we can read store instructions.

; RUN: %p2i -i %s --insts --no-local-syms | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

define internal void @store_i8(i32 %addr) {
entry:
  %addr_i8 = inttoptr i32 %addr to i8*
  store i8 3, i8* %addr_i8, align 1
  ret void

; CHECK:      __0:
; CHECK-NEXT:   store i8 3, i8* %__0, align 1
; CHECK-NEXT:   ret void
}

define internal void @store_i16(i32 %addr) {
entry:
  %addr_i16 = inttoptr i32 %addr to i16*
  store i16 5, i16* %addr_i16, align 1
  ret void

; CHECK:      __0:
; CHECK-NEXT:   store i16 5, i16* %__0, align 1
; CHECK-NEXT:   ret void
}

define internal void @store_i32(i32 %addr, i32 %v) {
entry:
  %addr_i32 = inttoptr i32 %addr to i32*
  store i32 %v, i32* %addr_i32, align 1
  ret void

; CHECK:      __0:
; CHECK-NEXT:   store i32 %__1, i32* %__0, align 1
; CHECK-NEXT:   ret void
}

define internal void @store_i64(i32 %addr, i64 %v) {
entry:
  %addr_i64 = inttoptr i32 %addr to i64*
  store i64 %v, i64* %addr_i64, align 1
  ret void

; CHECK:      __0:
; CHECK-NEXT:   store i64 %__1, i64* %__0, align 1
; CHECK-NEXT:   ret void
}

define internal void @store_float_a1(i32 %addr, float %v) {
entry:
  %addr_float = inttoptr i32 %addr to float*
  store float %v, float* %addr_float, align 1
  ret void

; TODO(kschimpf) Fix store alignment in ICE to allow non-default.

; CHECK:      __0:
; CHECK-NEXT:   store float %__1, float* %__0, align 4
; CHECK-NEXT:   ret void
}

define internal void @store_float_a4(i32 %addr, float %v) {
entry:
  %addr_float = inttoptr i32 %addr to float*
  store float %v, float* %addr_float, align 4
  ret void

; CHECK:       __0:
; CHECK-NEXT:   store float %__1, float* %__0, align 4
; CHECK-NEXT:   ret void
}

define internal void @store_double_a1(i32 %addr, double %v) {
entry:
  %addr_double = inttoptr i32 %addr to double*
  store double %v, double* %addr_double, align 1
  ret void

; TODO(kschimpf) Fix store alignment in ICE to allow non-default.

; CHECK:      __0:
; CHECK-NEXT:   store double %__1, double* %__0, align 8
; CHECK-NEXT:   ret void
}

define internal void @store_double_a8(i32 %addr, double %v) {
entry:
  %addr_double = inttoptr i32 %addr to double*
  store double %v, double* %addr_double, align 8
  ret void

; CHECK:      __0:
; CHECK-NEXT:   store double %__1, double* %__0, align 8
; CHECK-NEXT:   ret void
}

define internal void @store_v16xI8(i32 %addr, <16 x i8> %v) {
  %addr_v16xI8 = inttoptr i32 %addr to <16 x i8>*
  store <16 x i8> %v, <16 x i8>* %addr_v16xI8, align 1
  ret void

; CHECK:      __0:
; CHECK-NEXT:   store <16 x i8> %__1, <16 x i8>* %__0, align 1
; CHECK-NEXT:   ret void
}

define internal void @store_v8xI16(i32 %addr, <8 x i16> %v) {
  %addr_v8xI16 = inttoptr i32 %addr to <8 x i16>*
  store <8 x i16> %v, <8 x i16>* %addr_v8xI16, align 2
  ret void

; CHECK:      __0:
; CHECK-NEXT:   store <8 x i16> %__1, <8 x i16>* %__0, align 2
; CHECK-NEXT:   ret void
}

define internal void @store_v4xI32(i32 %addr, <4 x i32> %v) {
  %addr_v4xI32 = inttoptr i32 %addr to <4 x i32>*
  store <4 x i32> %v, <4 x i32>* %addr_v4xI32, align 4
  ret void

; CHECK:      __0:
; CHECK-NEXT:   store <4 x i32> %__1, <4 x i32>* %__0, align 4
; CHECK-NEXT:   ret void
}

define internal void @store_v4xFloat(i32 %addr, <4 x float> %v) {
  %addr_v4xFloat = inttoptr i32 %addr to <4 x float>*
  store <4 x float> %v, <4 x float>* %addr_v4xFloat, align 4
  ret void

; CHECK:      __0:
; CHECK-NEXT:   store <4 x float> %__1, <4 x float>* %__0, align 4
; CHECK-NEXT:   ret void
}

; NOIR: Total across all functions
