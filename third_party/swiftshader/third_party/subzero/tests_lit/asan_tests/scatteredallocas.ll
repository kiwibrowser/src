; Test that static allocas throughout the entry block are instrumented correctly

; REQUIRES: allow_dump

; RUN: %p2i -i %s --args -verbose=inst -threads=0 -fsanitize-address \
; RUN:     -allow-externally-defined-symbols | FileCheck --check-prefix=DUMP %s

declare external i32 @malloc(i32)
declare external void @free(i32)

define void @func() {
  %a = alloca i8, i32 4, align 4
  %m1 = call i32 @malloc(i32 42)
  %b = alloca i8, i32 16, align 4
  store i8 50, i8* %a, align 1
  %c = alloca i8, i32 8, align 8
  call void @free(i32 %m1)
  %d = alloca i8, i32 12, align 4
  ret void
}

; DUMP-LABEL: ================ Instrumented CFG ================
; DUMP-NEXT: define void @func() {
; DUMP-NEXT: __0:
; DUMP-NEXT:   %__$rz0 = alloca i8, i32 32, align 8
; DUMP-NEXT:   %a = alloca i8, i32 64, align 8
; DUMP-NEXT:   %b = alloca i8, i32 64, align 8
; DUMP-NEXT:   %c = alloca i8, i32 64, align 8
; DUMP-NEXT:   %d = alloca i8, i32 64, align 8
; DUMP-NEXT:   %shadowIndex = lshr i32 %__$rz0, 3
; DUMP-NEXT:   %firstShadowLoc = add i32 %shadowIndex, 536870912
; DUMP-NEXT:   %__8 = add i32 %firstShadowLoc, 0
; DUMP-NEXT:   store i32 -1, i32* %__8, align 1
; DUMP-NEXT:   %__9 = add i32 %firstShadowLoc, 4
; DUMP-NEXT:   store i32 -252, i32* %__9, align 1
; DUMP-NEXT:   %__10 = add i32 %firstShadowLoc, 8
; DUMP-NEXT:   store i32 -1, i32* %__10, align 1
; DUMP-NEXT:   %__11 = add i32 %firstShadowLoc, 12
; DUMP-NEXT:   store i32 -65536, i32* %__11, align 1
; DUMP-NEXT:   %__12 = add i32 %firstShadowLoc, 16
; DUMP-NEXT:   store i32 -1, i32* %__12, align 1
; DUMP-NEXT:   %__13 = add i32 %firstShadowLoc, 20
; DUMP-NEXT:   store i32 -256, i32* %__13, align 1
; DUMP-NEXT:   %__14 = add i32 %firstShadowLoc, 24
; DUMP-NEXT:   store i32 -1, i32* %__14, align 1
; DUMP-NEXT:   %__15 = add i32 %firstShadowLoc, 28
; DUMP-NEXT:   store i32 -64512, i32* %__15, align 1
; DUMP-NEXT:   %__16 = add i32 %firstShadowLoc, 32
; DUMP-NEXT:   store i32 -1, i32* %__16, align 1
; DUMP-NEXT:   %m1 = call i32 @__asan_malloc(i32 42)
; DUMP-NEXT:   store i8 50, i8* %a, align 1
; DUMP-NEXT:   call void @__asan_free(i32 %m1)
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