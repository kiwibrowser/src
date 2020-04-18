; Test that shrinking an allocation updates the redzones

; REQUIRES: no_minimal_build

; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 2>&1 | FileCheck %s


declare external i32 @malloc(i32)
declare external i32 @realloc(i32, i32)
declare external void @free(i32)

define void @_start(i32 %arg) {
  %ptr16 = call i32 @malloc(i32 16)
  %off12a = add i32 %ptr16, 12
  %offptra = inttoptr i32 %off12a to i32*
  %resa = load i32, i32* %offptra, align 1
  %ptr8 = call i32 @realloc(i32 %ptr16, i32 8)
  %off12b = add i32 %ptr8, 12
  %offptrb = inttoptr i32 %off12b to i8*
  %resb = load i8, i8* %offptrb, align 1
  ret void
}

; CHECK: Illegal 1 byte load from heap object at
