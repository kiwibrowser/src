; Test that the quarantine for recently freed objects works

; REQUIRES: no_minimal_build

; Test with an illegal load from a freed block
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 2>&1 | FileCheck --check-prefix=LOAD %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 2>&1 | FileCheck --check-prefix=LOAD %s

; Test with an illegal store to a freed block
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2>&1 | FileCheck --check-prefix=STORE %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2>&1 | FileCheck --check-prefix=STORE %s

; Test that freed objects eventually get out of quarantine and are unpoisoned
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2 2>&1 | FileCheck --check-prefix=NONE %s \
; RUN:     --allow-empty
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2 2>&1 | FileCheck --check-prefix=NONE %s \
; RUN:     --allow-empty

declare external i32 @malloc(i32)
declare external void @free(i32)
declare external void @exit(i32)

; make three 100MB allocations
define void @_start(i32 %arg) {
  %argcaddr = add i32 %arg, 8
  %argcptr = inttoptr i32 %argcaddr to i32*
  %argc = load i32, i32* %argcptr, align 1
  %alloc1addr = call i32 @malloc(i32 104857600)
  %alloc2addr = call i32 @malloc(i32 104857600)
  %alloc3addr = call i32 @malloc(i32 104857600)
  %alloc1 = inttoptr i32 %alloc1addr to i32*
  %alloc2 = inttoptr i32 %alloc2addr to i32*
  %alloc3 = inttoptr i32 %alloc3addr to i32*
  call void @free(i32 %alloc1addr)
  call void @free(i32 %alloc2addr)
  call void @free(i32 %alloc3addr)
  switch i32 %argc, label %error [i32 1, label %bad_load
                                  i32 2, label %bad_store
                                  i32 3, label %no_err]
bad_load:
  %result_load = load i32, i32* %alloc2, align 1
  br label %error
bad_store:
  store i32 42, i32* %alloc3, align 1
  br label %error
no_err:
  %result_no_err = load i32, i32* %alloc1, align 1
  call void @exit(i32 0)
  unreachable
error:
  call void @exit(i32 1)
  unreachable
}

; LOAD: Illegal 4 byte load from freed object at
; STORE: Illegal 4 byte store to freed object at
; NONE-NOT: Illegal
