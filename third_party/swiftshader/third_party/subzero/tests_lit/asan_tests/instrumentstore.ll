; Test for a call to __asan_check() preceding stores

; REQUIRES: allow_dump

; RUN: %p2i -i %s --args -verbose=inst -threads=0 -fsanitize-address \
; RUN:     | FileCheck --check-prefix=DUMP %s

; A function with a local variable that does the stores
define internal void @doStores(<4 x i32> %vecSrc, i32 %arg8, i32 %arg16,
                               i32 %arg32, i32 %arg64, i32 %arg128) {
  %destLocal8 = inttoptr i32 %arg8 to i8*
  %destLocal16 = inttoptr i32 %arg16 to i16*
  %destLocal32 = inttoptr i32 %arg32 to i32*
  %destLocal64 = inttoptr i32 %arg64 to i64*
  %destLocal128 = inttoptr i32 %arg128 to <4 x i32>*

  store i8 42, i8* %destLocal8, align 1
  store i16 42, i16* %destLocal16, align 1
  store i32 42, i32* %destLocal32, align 1
  store i64 42, i64* %destLocal64, align 1
  store <4 x i32> %vecSrc, <4 x i32>* %destLocal128, align 4

  ret void
}

; DUMP-LABEL: ================ Instrumented CFG ================
; DUMP-NEXT: define internal void @doStores(
; DUMP-NEXT: __0:
; DUMP-NEXT: call void @__asan_check_store(i32 %arg8, i32 1)
; DUMP-NEXT: store i8 42, i8* %arg8, align 1
; DUMP-NEXT: call void @__asan_check_store(i32 %arg16, i32 2)
; DUMP-NEXT: store i16 42, i16* %arg16, align 1
; DUMP-NEXT: call void @__asan_check_store(i32 %arg32, i32 4)
; DUMP-NEXT: store i32 42, i32* %arg32, align 1
; DUMP-NEXT: call void @__asan_check_store(i32 %arg64, i32 8)
; DUMP-NEXT: store i64 42, i64* %arg64, align 1
; DUMP-NEXT: call void @__asan_check_store(i32 %arg128, i32 16)
; DUMP-NEXT: store <4 x i32> %vecSrc, <4 x i32>* %arg128, align 4
; DUMP:      ret void
; DUMP-NEXT: }
