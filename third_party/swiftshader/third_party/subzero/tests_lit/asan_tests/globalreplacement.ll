; Test that global pointers to allocation functions are replaced

; REQUIRES: allow_dump

; RUN: %p2i -i %s --args -verbose=global_init -threads=0 -fsanitize-address \
; RUN:     -allow-externally-defined-symbols | FileCheck --check-prefix=DUMP %s

declare external i32 @malloc(i32)
declare external i32 @realloc(i32, i32)
declare external i32 @calloc(i32, i32)
declare external void @free(i32)
declare external void @foo()

@global_malloc = internal global i32 ptrtoint (i32 (i32)* @malloc to i32)
@global_realloc = internal global i32 ptrtoint (i32 (i32, i32)* @realloc to i32)
@global_calloc = internal global i32 ptrtoint (i32 (i32, i32)* @calloc to i32)
@global_free = internal global i32 ptrtoint (void (i32)* @free to i32)
@global_foo = internal global i32 ptrtoint (void ()* @foo to i32)

@constant_malloc = internal constant i32 ptrtoint (i32 (i32)* @malloc to i32)
@constant_realloc = internal constant i32 ptrtoint (i32 (i32, i32)* @realloc to i32)
@constant_calloc = internal constant i32 ptrtoint (i32 (i32, i32)* @calloc to i32)
@constant_free = internal constant i32 ptrtoint (void (i32)* @free to i32)
@constant_foo = internal constant i32 ptrtoint (void ()* @foo to i32)

@multiple_initializers = internal global <{i32, i32}> <{i32 ptrtoint (i32 (i32)* @malloc to i32), i32 ptrtoint (void (i32)* @free to i32)}>

define void @func() {
  ret void
}

; DUMP: Instrumented Globals
; DUMP-NEXT: @__$rz_array
; DUMP-NEXT: @__$rz_sizes
; DUMP-NEXT: @__$rz0
; DUMP-NEXT: @global_malloc = internal global i32
; DUMP-SAME:   ptrtoint (i32 (i32)* @__asan_malloc to i32)
; DUMP-NEXT: @__$rz1
; DUMP-NEXT: @__$rz2
; DUMP-NEXT: @global_realloc = internal global i32
; DUMP-SAME:   ptrtoint (i32 (i32, i32)* @__asan_realloc to i32)
; DUMP-NEXT: @__$rz3
; DUMP-NEXT: @__$rz4
; DUMP-NEXT: @global_calloc = internal global i32
; DUMP-SAME:   ptrtoint (i32 (i32, i32)* @__asan_calloc to i32)
; DUMP-NEXT: @__$rz5
; DUMP-NEXT: @__$rz6
; DUMP-NEXT: @global_free = internal global i32
; DUMP-SAME:   ptrtoint (void (i32)* @__asan_free to i32)
; DUMP-NEXT: @__$rz7
; DUMP-NEXT: @__$rz8
; DUMP-NEXT: @global_foo = internal global i32
; DUMP-SAME:   ptrtoint (void ()* @foo to i32)
; DUMP-NEXT: @__$rz9
; DUMP-NEXT: @__$rz10
; DUMP-NEXT: @constant_malloc = internal constant i32
; DUMP-SAME:   ptrtoint (i32 (i32)* @__asan_malloc to i32)
; DUMP-NEXT: @__$rz11
; DUMP-NEXT: @__$rz12
; DUMP-NEXT: @constant_realloc = internal constant i32
; DUMP-SAME:   ptrtoint (i32 (i32, i32)* @__asan_realloc to i32)
; DUMP-NEXT: @__$rz13
; DUMP-NEXT: @__$rz14
; DUMP-NEXT: @constant_calloc = internal constant i32
; DUMP-SAME:   ptrtoint (i32 (i32, i32)* @__asan_calloc to i32)
; DUMP-NEXT: @__$rz15
; DUMP-NEXT: @__$rz16
; DUMP-NEXT: @constant_free = internal constant i32
; DUMP-SAME:   ptrtoint (void (i32)* @__asan_free to i32)
; DUMP-NEXT: @__$rz17
; DUMP-NEXT: @__$rz18
; DUMP-NEXT: @constant_foo = internal constant i32
; DUMP-SAME:   ptrtoint (void ()* @foo to i32)
; DUMP-NEXT: @__$rz19
; DUMP-NEXT: @__$rz20
; DUMP-NEXT: @multiple_initializers = internal global <{ i32, i32 }>
; DUMP-SAME:   <{ i32 ptrtoint (i32 (i32)* @__asan_malloc to i32),
; DUMP-SAME:      i32 ptrtoint (void (i32)* @__asan_free to i32) }>
; DUMP-NEXT: @__$rz21