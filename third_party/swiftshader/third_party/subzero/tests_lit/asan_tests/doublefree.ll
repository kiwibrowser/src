; Test that double frees are detected

; REQUIRES: no_minimal_build

; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 2>&1 | FileCheck --check-prefix=ERR %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 2>&1 | FileCheck --check-prefix=ERR %s

declare external i32 @malloc(i32)
declare external void @free(i32)
declare external void @exit(i32)

define void @_start(i32 %arg) {
  %alloc = call i32 @malloc(i32 42)
  call void @free(i32 %alloc)
  call void @free(i32 %alloc)
  call void @exit(i32 1)
  ret void
}

; ERR: Double free of object at
; ERR-NEXT: address of __asan_error symbol is
