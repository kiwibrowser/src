; Test that calls made through pointers are unchanged by ASan

; REQUIRES: allow_dump

; RUN: %p2i -i %s --args -verbose=inst -threads=0 -fsanitize-address \
; RUN:     | FileCheck --check-prefix=DUMP %s

define internal i32 @caller(i32 %callee_addr, i32 %arg) {
  %callee = inttoptr i32 %callee_addr to i32 (i32)*
  %result = call i32 %callee(i32 %arg)
  ret i32 %result
}

; DUMP-LABEL: ================ Initial CFG ================
; DUMP-NEXT: define internal i32 @caller(i32 %callee_addr, i32 %arg) {
; DUMP-NEXT: __0:
; DUMP-NEXT:   %result = call i32 %callee_addr(i32 %arg)
; DUMP-NEXT:   ret i32 %result
; DUMP-NEXT: }
; DUMP-LABEL: ================ Instrumented CFG ================
; DUMP-NEXT: define internal i32 @caller(i32 %callee_addr, i32 %arg) {
; DUMP-NEXT: __0:
; DUMP-NEXT:   %result = call i32 %callee_addr(i32 %arg)
; DUMP-NEXT:   ret i32 %result
; DUMP-NEXT: }
