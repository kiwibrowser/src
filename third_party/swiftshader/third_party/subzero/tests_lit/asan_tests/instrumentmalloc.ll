; Test that calls to malloc() and free() are replaced

; REQUIRES: allow_dump

; RUN: %p2i -i %s --args -verbose=inst -threads=0 -fsanitize-address \
; RUN:     --allow-externally-defined-symbols | FileCheck --check-prefix=DUMP %s

declare external i32 @malloc(i32)
declare external i32 @calloc(i32, i32)
declare external i32 @realloc(i32, i32)
declare external void @free(i32)

define internal void @func() {
  %ptr1 = call i32 @malloc(i32 42)
  %ptr2 = call i32 @calloc(i32 12, i32 42)
  %ptr3 = call i32 @realloc(i32 0, i32 100)
  call void @free(i32 %ptr1)
  ret void
}

; DUMP-LABEL: ================ Instrumented CFG ================
; DUMP-NEXT: define internal void @func() {
; DUMP-NEXT: __0:
; DUMP-NEXT: %ptr1 = call i32 @__asan_malloc(i32 42)
; DUMP-NEXT: %ptr2 = call i32 @__asan_calloc(i32 12, i32 42)
; DUMP-NEXT: %ptr3 = call i32 @__asan_realloc(i32 0, i32 100)
; DUMP-NEXT: call void @__asan_free(i32 %ptr1)
; DUMP-NEXT: ret void
; DUMP-NEXT: }
