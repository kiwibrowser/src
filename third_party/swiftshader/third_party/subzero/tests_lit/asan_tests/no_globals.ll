; Check that Subzero can instrument _start when there are no globals.
; Previously Subzero would deadlock when _start was the first function. Also
; test that instrumenting start does not deadlock waiting for nonexistent
; global initializers to be lowered.

; REQUIRES: no_minimal_build

; RUN: %p2i -i %s --args -verbose=inst -fsanitize-address \
; RUN:     | FileCheck --check-prefix=DUMP %s

; RUN: %p2i -i %s --args -verbose=inst -fsanitize-address -threads=0 \
; RUN:     | FileCheck --check-prefix=DUMP %s


define void @_start(i32 %arg) {
  ret void
}

; DUMP: __asan_init
