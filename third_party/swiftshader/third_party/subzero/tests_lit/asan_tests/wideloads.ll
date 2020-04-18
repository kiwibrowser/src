; Test that potentially widened loads to not trigger an error report

; REQUIRES: no_minimal_build

; check for wide load exception
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t | FileCheck %s --check-prefix=WIDE --allow-empty

; check for error reporting
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2>&1 | FileCheck %s --check-prefix=NOWIDE


declare external void @exit(i32)

define internal void @wide_load() {
  %str = alloca i8, i32 1, align 1
  %str4 = bitcast i8* %str to i32*
  %contents = load i32, i32* %str4, align 1
  call void @exit(i32 0)
  unreachable
}

define internal void @no_wide_load() {
  %str = alloca i8, i32 1, align 1
  %straddr = ptrtoint i8* %str to i32
  %off1addr = add i32 %straddr, 1
  %off1 = inttoptr i32 %off1addr to i8*
  %contents = load i8, i8* %off1, align 1
  call void @exit(i32 1) 
  unreachable
}

; WIDE-NOT: Illegal
; NOWIDE: Illegal 1 byte load from stack object at

; use argc to determine which test routine to run
define void @_start(i32 %arg) {
  %argcaddr = add i32 %arg, 8
  %argcptr = inttoptr i32 %argcaddr to i32*
  %argc = load i32, i32* %argcptr, align 1
  switch i32 %argc, label %error [i32 1, label %wide_load
                                  i32 2, label %no_wide_load]
wide_load:
  call void @wide_load()
  br label %error
no_wide_load:
  call void @no_wide_load()
  br label %error
error:
  call void @exit(i32 1)
  unreachable
}