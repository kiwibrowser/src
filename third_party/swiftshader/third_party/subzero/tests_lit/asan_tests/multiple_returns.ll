; Check that functions with multiple returns are correctly instrumented

; REQUIRES: allow_dump

; RUN: %p2i -i %s --args -verbose=inst -threads=0 -fsanitize-address \
; RUN:     | FileCheck --check-prefix=DUMP %s

define internal void @ret_twice(i32 %condarg) {
  %local1 = alloca i8, i32 4, align 4
  %local2 = alloca i8, i32 4, align 4
  %cond = icmp ne i32 %condarg, 0
  br i1 %cond, label %yes, label %no
yes:
  ret void
no:
  ret void
}

; DUMP-LABEL:================ Instrumented CFG ================
; DUMP-NEXT: define internal void @ret_twice(i32 %condarg) {
; DUMP-NEXT: __0:
; DUMP-NEXT:   %__$rz0 = alloca i8, i32 32, align 8
; DUMP-NEXT:   %local1 = alloca i8, i32 64, align 8
; DUMP-NEXT:   %local2 = alloca i8, i32 64, align 8
; DUMP-NEXT:   %shadowIndex = lshr i32 %__$rz0, 3
; DUMP-NEXT:   %firstShadowLoc = add i32 %shadowIndex, 53687091
; DUMP-NEXT:   %__7 = add i32 %firstShadowLoc, 0
; DUMP-NEXT:   store i32 -1, i32* %__7, align 1
; DUMP-NEXT:   %__8 = add i32 %firstShadowLoc, 4
; DUMP-NEXT:   store i32 -252, i32* %__8, align 1
; DUMP-NEXT:   %__9 = add i32 %firstShadowLoc, 8
; DUMP-NEXT:   store i32 -1, i32* %__9, align 1
; DUMP-NEXT:   %__10 = add i32 %firstShadowLoc, 12
; DUMP-NEXT:   store i32 -252, i32* %__10, align 1
; DUMP-NEXT:   %__11 = add i32 %firstShadowLoc, 16
; DUMP-NEXT:   store i32 -1, i32* %__11, align 1
; DUMP-NEXT:   %cond = icmp ne i32 %condarg, 0
; DUMP-NEXT:   br i1 %cond, label %yes, label %no
; DUMP-NEXT: yes:
; DUMP-NEXT:   store i32 0, i32* %__7, align 1
; DUMP-NEXT:   store i32 0, i32* %__8, align 1
; DUMP-NEXT:   store i32 0, i32* %__9, align 1
; DUMP-NEXT:   store i32 0, i32* %__10, align 1
; DUMP-NEXT:   store i32 0, i32* %__11, align 1
; DUMP-NEXT:   ret void
; DUMP-NEXT: no:
; DUMP-NEXT:   store i32 0, i32* %__7, align 1
; DUMP-NEXT:   store i32 0, i32* %__8, align 1
; DUMP-NEXT:   store i32 0, i32* %__9, align 1
; DUMP-NEXT:   store i32 0, i32* %__10, align 1
; DUMP-NEXT:   store i32 0, i32* %__11, align 1
; DUMP-NEXT:   ret void
; DUMP-NEXT: }
