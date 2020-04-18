; Test that direct loads and stores of local variables are not checked.
; Also test that redundant checks of the same variable are elided.

; REQUIRES: allow_dump

; RUN: %p2i -i %s --args -verbose=inst -threads=0 -fsanitize-address \
; RUN:     | FileCheck --check-prefix=DUMP %s

define internal void @foo() {
  %ptr8 = alloca i8, i32 1, align 4
  %ptr16 = alloca i8, i32 2, align 4
  %ptr32 = alloca i8, i32 4, align 4
  %ptr64 = alloca i8, i32 8, align 4
  %ptr128 = alloca i8, i32 16, align 4

  %target8 = bitcast i8* %ptr8 to i8*
  %target16 = bitcast i8* %ptr16 to i16*
  %target32 = bitcast i8* %ptr32 to i32*
  %target64 = bitcast i8* %ptr64 to i64*
  %target128 = bitcast i8* %ptr128 to <4 x i32>*

  ; unchecked loads
  %loaded8 = load i8, i8* %target8, align 1
  %loaded16 = load i16, i16* %target16, align 1
  %loaded32 = load i32, i32* %target32, align 1
  %loaded64 = load i64, i64* %target64, align 1
  %loaded128 = load <4 x i32>, <4 x i32>* %target128, align 4

  ; unchecked stores
  store i8 %loaded8, i8* %target8, align 1
  store i16 %loaded16, i16* %target16, align 1
  store i32 %loaded32, i32* %target32, align 1
  store i64 %loaded64, i64* %target64, align 1
  store <4 x i32> %loaded128, <4 x i32>* %target128, align 4

  %addr8 = ptrtoint i8* %ptr8 to i32
  %addr16 = ptrtoint i8* %ptr16 to i32
  %addr32 = ptrtoint i8* %ptr32 to i32
  %addr64 = ptrtoint i8* %ptr64 to i32
  %addr128 = ptrtoint i8* %ptr128 to i32

  %off8 = add i32 %addr8, -1
  %off16 = add i32 %addr16, -1
  %off32 = add i32 %addr32, -1
  %off64 = add i32 %addr64, -1
  %off128 = add i32 %addr128, -1

  %offtarget8 = inttoptr i32 %off8 to i8*
  %offtarget16 = inttoptr i32 %off16 to i16*
  %offtarget32 = inttoptr i32 %off32 to i32*
  %offtarget64 = inttoptr i32 %off64 to i64*
  %offtarget128 = inttoptr i32 %off128 to <4 x i32>*

  ; checked stores
  store i8 42, i8* %offtarget8, align 1
  store i16 42, i16* %offtarget16, align 1
  store i32 42, i32* %offtarget32, align 1

  ; checked loads
  %offloaded64 = load i64, i64* %offtarget64, align 1
  %offloaded128 = load <4 x i32>, <4 x i32>* %offtarget128, align 4

  ; loads and stores with elided redundant checks
  %offloaded8 = load i8, i8* %offtarget8, align 1
  %offloaded16 = load i16, i16* %offtarget16, align 1
  %offloaded32 = load i32, i32* %offtarget32, align 1
  store i64 %offloaded64, i64* %offtarget64, align 1
  store <4 x i32> %offloaded128, <4 x i32>* %offtarget128, align 4

  ret void
}

; DUMP-LABEL: ================ Instrumented CFG ================
; DUMP-NEXT: define internal void @foo() {

; Direct unchecked loads and stores
; DUMP: %loaded8 = load i8, i8* %ptr8, align 1
; DUMP-NEXT: %loaded16 = load i16, i16* %ptr16, align 1
; DUMP-NEXT: %loaded32 = load i32, i32* %ptr32, align 1
; DUMP-NEXT: %loaded64 = load i64, i64* %ptr64, align 1
; DUMP-NEXT: %loaded128 = load <4 x i32>, <4 x i32>* %ptr128, align 4
; DUMP-NEXT: store i8 %loaded8, i8* %ptr8, align 1
; DUMP-NEXT: store i16 %loaded16, i16* %ptr16, align 1
; DUMP-NEXT: store i32 %loaded32, i32* %ptr32, align 1
; DUMP-NEXT: store i64 %loaded64, i64* %ptr64, align 1
; DUMP-NEXT: store <4 x i32> %loaded128, <4 x i32>* %ptr128, align 4

; Checked stores
; DUMP: call void @__asan_check_store(i32 %off8, i32 1)
; DUMP-NEXT: store i8 42, i8* %off8, align 1
; DUMP-NEXT: call void @__asan_check_store(i32 %off16, i32 2)
; DUMP-NEXT: store i16 42, i16* %off16, align 1
; DUMP-NEXT: call void @__asan_check_store(i32 %off32, i32 4)
; DUMP-NEXT: store i32 42, i32* %off32, align 1

; Checked loads
; DUMP-NEXT: call void @__asan_check_load(i32 %off64, i32 8)
; DUMP-NEXT: %offloaded64 = load i64, i64* %off64, align 1
; DUMP-NEXT: call void @__asan_check_load(i32 %off128, i32 16)
; DUMP-NEXT: %offloaded128 = load <4 x i32>, <4 x i32>* %off128, align 4

; Loads and stores with elided redundant checks
; DUMP-NEXT: %offloaded8 = load i8, i8* %off8, align 1
; DUMP-NEXT: %offloaded16 = load i16, i16* %off16, align 1
; DUMP-NEXT: %offloaded32 = load i32, i32* %off32, align 1
; DUMP-NEXT: store i64 %offloaded64, i64* %off64, align 1, beacon %offloaded64
; DUMP-NEXT: store <4 x i32> %offloaded128, <4 x i32>* %off128, align 4, beacon %offloaded128
