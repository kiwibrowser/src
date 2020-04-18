; Test to ensure that blacklisted functions are not instrumented and others are.

; REQUIRES: allow_dump

; RUN: %p2i -i %s --args -verbose=inst -threads=0 -fsanitize-address \
; RUN:     -allow-externally-defined-symbols | FileCheck --check-prefix=DUMP %s

declare external i32 @malloc(i32)
declare external void @free(i32)

; A black listed function
define internal void @_Balloc() {
  %local = alloca i8, i32 4, align 4
  %heapvar = call i32 @malloc(i32 42)
  call void @free(i32 %heapvar)
  ret void
}

; DUMP-LABEL: ================ Instrumented CFG ================
; DUMP-NEXT: define internal void @_Balloc() {
; DUMP-NEXT: __0:
; DUMP-NEXT:   %local = alloca i8, i32 4, align 4
; DUMP-NEXT:   %heapvar = call i32 @malloc(i32 42)
; DUMP-NEXT:   call void @free(i32 %heapvar)
; DUMP-NEXT:   ret void
; DUMP-NEXT: }

; A non black listed function
define internal void @func() {
  %local = alloca i8, i32 4, align 4
  %heapvar = call i32 @malloc(i32 42)
  call void @free(i32 %heapvar)
  ret void
}

; DUMP-LABEL: ================ Instrumented CFG ================
; DUMP-NEXT: define internal void @func() {
; DUMP-NEXT: __0:
; DUMP-NEXT:   %__$rz0 = alloca i8, i32 32, align 8
; DUMP-NEXT:   %local = alloca i8, i32 64, align 8
; DUMP-NEXT:   %shadowIndex = lshr i32 %__$rz0, 3
; DUMP-NEXT:   %firstShadowLoc = add i32 %shadowIndex, 536870912
; DUMP-NEXT:   %__5 = add i32 %firstShadowLoc, 0
; DUMP-NEXT:   store i32 -1, i32* %__5, align 1
; DUMP-NEXT:   %__6 = add i32 %firstShadowLoc, 4
; DUMP-NEXT:   store i32 -252, i32* %__6, align 1
; DUMP-NEXT:   %__7 = add i32 %firstShadowLoc, 8
; DUMP-NEXT:   store i32 -1, i32* %__7, align 1
; DUMP-NEXT:   %heapvar = call i32 @__asan_malloc(i32 42)
; DUMP-NEXT:   call void @__asan_free(i32 %heapvar)
; DUMP-NEXT:   store i32 0, i32* %__5, align 1
; DUMP-NEXT:   store i32 0, i32* %__6, align 1
; DUMP-NEXT:   store i32 0, i32* %__7, align 1
; DUMP-NEXT:   ret void
; DUMP-NEXT: }
