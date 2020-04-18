; Test for insertion of redzones around local variables

; REQUIRES: allow_dump

; RUN: %p2i -i %s --args -verbose=inst -threads=0 -fsanitize-address \
; RUN:     -allow-externally-defined-symbols | FileCheck --check-prefix=DUMP %s

; Function with local variables to be instrumented
define internal void @func() {
  %local1 = alloca i8, i32 4, align 4
  %local2 = alloca i8, i32 32, align 1
  %local3 = alloca i8, i32 13, align 2
  %local4 = alloca i8, i32 75, align 4
  %local5 = alloca i8, i32 64, align 8
  %i1 = ptrtoint i8* %local1 to i32
  %i2 = ptrtoint i8* %local2 to i32
  %i3 = ptrtoint i8* %local3 to i32
  %i4 = ptrtoint i8* %local4 to i32
  %i5 = ptrtoint i8* %local5 to i32
  call void @foo(i32 %i1)
  call void @foo(i32 %i2)
  call void @foo(i32 %i3)
  call void @foo(i32 %i4)
  call void @foo(i32 %i5)
  ret void
}

declare external void @foo(i32)

; DUMP-LABEL: ================ Instrumented CFG ================
; DUMP-NEXT: define internal void @func() {
; DUMP-NEXT: __0:
; DUMP-NEXT:   %__$rz0 = alloca i8, i32 32, align 8
; DUMP-NEXT:   %local1 = alloca i8, i32 64, align 8
; DUMP-NEXT:   %local2 = alloca i8, i32 64, align 8
; DUMP-NEXT:   %local3 = alloca i8, i32 64, align 8
; DUMP-NEXT:   %local4 = alloca i8, i32 128, align 8
; DUMP-NEXT:   %local5 = alloca i8, i32 96, align 8
; DUMP-NEXT:   %shadowIndex = lshr i32 %__$rz0, 3
; DUMP-NEXT:   %firstShadowLoc = add i32 %shadowIndex, 536870912
; DUMP-NEXT:   %__8 = add i32 %firstShadowLoc, 0
; DUMP-NEXT:   store i32 -1, i32* %__8, align 1
; DUMP-NEXT:   %__9 = add i32 %firstShadowLoc, 4
; DUMP-NEXT:   store i32 -252, i32* %__9, align 1
; DUMP-NEXT:   %__10 = add i32 %firstShadowLoc, 8
; DUMP-NEXT:   store i32 -1, i32* %__10, align 1
; DUMP-NEXT:   %__11 = add i32 %firstShadowLoc, 16
; DUMP-NEXT:   store i32 -1, i32* %__11, align 1
; DUMP-NEXT:   %__12 = add i32 %firstShadowLoc, 20
; DUMP-NEXT:   store i32 -64256, i32* %__12, align 1
; DUMP-NEXT:   %__13 = add i32 %firstShadowLoc, 24
; DUMP-NEXT:   store i32 -1, i32* %__13, align 1
; DUMP-NEXT:   %__14 = add i32 %firstShadowLoc, 36
; DUMP-NEXT:   store i32 -64768, i32* %__14, align 1
; DUMP-NEXT:   %__15 = add i32 %firstShadowLoc, 40
; DUMP-NEXT:   store i32 -1, i32* %__15, align 1
; DUMP-NEXT:   %__16 = add i32 %firstShadowLoc, 52
; DUMP-NEXT:   store i32 -1, i32* %__16, align 1
; DUMP-NEXT:   call void @foo(i32 %local1)
; DUMP-NEXT:   call void @foo(i32 %local2)
; DUMP-NEXT:   call void @foo(i32 %local3)
; DUMP-NEXT:   call void @foo(i32 %local4)
; DUMP-NEXT:   call void @foo(i32 %local5)
; DUMP-NEXT:   store i32 0, i32* %__8, align 1
; DUMP-NEXT:   store i32 0, i32* %__9, align 1
; DUMP-NEXT:   store i32 0, i32* %__10, align 1
; DUMP-NEXT:   store i32 0, i32* %__11, align 1
; DUMP-NEXT:   store i32 0, i32* %__12, align 1
; DUMP-NEXT:   store i32 0, i32* %__13, align 1
; DUMP-NEXT:   store i32 0, i32* %__14, align 1
; DUMP-NEXT:   store i32 0, i32* %__15, align 1
; DUMP-NEXT:   store i32 0, i32* %__16, align 1
; DUMP-NEXT:   ret void
; DUMP-NEXT: }
