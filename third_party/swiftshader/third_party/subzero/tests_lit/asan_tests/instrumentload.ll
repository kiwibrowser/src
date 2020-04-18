; Test for a call to __asan_check() preceding loads

; REQUIRES: allow_dump

; RUN: %p2i -i %s --args -verbose=inst -threads=0 -fsanitize-address \
; RUN:     | FileCheck --check-prefix=DUMP %s

; A function with a local variable that does the loads
define internal void @doLoads(i32 %arg8, i32 %arg16, i32 %arg32, i32 %arg64,
                              i32 %arg128) {
  %srcLocal8 = inttoptr i32 %arg8 to i8*
  %srcLocal16 = inttoptr i32 %arg16 to i16*
  %srcLocal32 = inttoptr i32 %arg32 to i32*
  %srcLocal64 = inttoptr i32 %arg64 to i64*
  %srcLocal128 = inttoptr i32 %arg128 to <4 x i32>*

  %dest11 = load i8, i8* %srcLocal8, align 1
  %dest12 = load i16, i16* %srcLocal16, align 1
  %dest13 = load i32, i32* %srcLocal32, align 1
  %dest14 = load i64, i64* %srcLocal64, align 1
  %dest15 = load <4 x i32>, <4 x i32>* %srcLocal128, align 4

  ret void
}

; DUMP-LABEL: ================ Instrumented CFG ================
; DUMP-NEXT: define internal void @doLoads(
; DUMP-NEXT: __0:
; DUMP-NEXT: call void @__asan_check_load(i32 %arg8, i32 1)
; DUMP-NEXT: %dest11 = load i8, i8* %arg8, align 1
; DUMP-NEXT: call void @__asan_check_load(i32 %arg16, i32 2)
; DUMP-NEXT: %dest12 = load i16, i16* %arg16, align 1
; DUMP-NEXT: call void @__asan_check_load(i32 %arg32, i32 4)
; DUMP-NEXT: %dest13 = load i32, i32* %arg32, align 1
; DUMP-NEXT: call void @__asan_check_load(i32 %arg64, i32 8)
; DUMP-NEXT: %dest14 = load i64, i64* %arg64, align 1
; DUMP-NEXT: call void @__asan_check_load(i32 %arg128, i32 16)
; DUMP-NEXT: %dest15 = load <4 x i32>, <4 x i32>* %arg128, align 4
; DUMP:      ret void
; DUMP-NEXT: }
