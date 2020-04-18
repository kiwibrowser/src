; Verify that ASan properly catches and reports bugs

; REQUIRES: no_minimal_build

; check with a one off the end local load
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 2>&1 | FileCheck --check-prefix=LOCAL-LOAD %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 2>&1 | FileCheck --check-prefix=LOCAL-LOAD %s

; check with a many off the end local load
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2>&1 | FileCheck --check-prefix=LOCAL-LOAD %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2>&1 | FileCheck --check-prefix=LOCAL-LOAD %s

; check with a one before the front local load
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2 2>&1 | FileCheck --check-prefix=LOCAL-LOAD %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2\
; RUN:     %t.pexe -o %t && %t 1 2 2>&1 | FileCheck --check-prefix=LOCAL-LOAD %s

; check with a one off the end global load
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2 3 2>&1 | FileCheck \
; RUN:     --check-prefix=GLOBAL-LOAD %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2 3 2>&1 | FileCheck \
; RUN:     --check-prefix=GLOBAL-LOAD %s

; check with a many off the end global load
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 2>&1 | FileCheck \
; RUN:    --check-prefix=GLOBAL-LOAD %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 2>&1 | FileCheck \
; RUN:     --check-prefix=GLOBAL-LOAD %s

; check with a one before the front global load
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 2>&1 | FileCheck \
; RUN:     --check-prefix=GLOBAL-LOAD %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 2>&1 | FileCheck \
; RUN:     --check-prefix=GLOBAL-LOAD %s

; check with a one off the end local store
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 2>&1 | FileCheck \
; RUN:     --check-prefix=LOCAL-STORE %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 2>&1 | FileCheck \
; RUN:     --check-prefix=LOCAL-STORE %s

; check with a many off the end local store
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 7 2>&1 | FileCheck \
; RUN:     --check-prefix=LOCAL-STORE %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 7 2>&1 | FileCheck \
; RUN:     --check-prefix=LOCAL-STORE %s

; check with a one before the front local store
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 7 8 2>&1 | FileCheck \
; RUN:     --check-prefix=LOCAL-STORE %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 7 8 2>&1 | FileCheck \
; RUN:     --check-prefix=LOCAL-STORE %s

; check with a one off the end global store
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 7 8 9 2>&1 | FileCheck \
; RUN:     --check-prefix=GLOBAL-STORE %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 7 8 9 2>&1 | FileCheck \
; RUN:     --check-prefix=GLOBAL-STORE %s

; check with a many off the end global store
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 7 8 9 10 2>&1 | FileCheck \
; RUN:    --check-prefix=GLOBAL-STORE %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 7 8 9 10 2>&1 | FileCheck \
; RUN:    --check-prefix=GLOBAL-STORE %s

; check with a one before the front global store
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 7 8 9 10 11 2>&1 | FileCheck \
; RUN:     --check-prefix=GLOBAL-STORE %s
; RUN: llvm-as %s -o - | pnacl-freeze > %t.pexe && %S/../../pydir/szbuild.py \
; RUN:     --fsanitize-address --sz=-allow-externally-defined-symbols -O2 \
; RUN:     %t.pexe -o %t && %t 1 2 3 4 5 6 7 8 9 10 11 2>&1 | FileCheck \
; RUN:     --check-prefix=GLOBAL-STORE %s

declare external void @exit(i32)

; A global array
@array = internal constant [12 x i8] zeroinitializer

define void @access(i32 %is_local_i, i32 %is_load_i, i32 %err) {
  ; get the base pointer to either the local or global array
  %local = alloca i8, i32 12, align 1
  %global = bitcast [12 x i8]* @array to i8*
  %is_local = icmp ne i32 %is_local_i, 0
  %arr = select i1 %is_local, i8* %local, i8* %global

  ; determine the offset to access
  %err_offset = mul i32 %err, 4
  %pos_offset = add i32 %err_offset, 12
  %pos = icmp sge i32 %err_offset, 0
  %offset = select i1 %pos, i32 %pos_offset, i32 %err

  ; calculate the address to access
  %arraddr = ptrtoint i8* %arr to i32
  %badaddr = add i32 %arraddr, %offset
  %badptr = inttoptr i32 %badaddr to i8*

  ; determine load or store
  %is_load = icmp ne i32 %is_load_i, 0
  br i1 %is_load, label %bad_load, label %bad_store

bad_load:
  %result = load i8, i8* %badptr, align 1
  ret void

bad_store:
  store i8 42, i8* %badptr, align 1
  ret void
}

; use argc to determine which test routine to run
define void @_start(i32 %arg) {
  %argcaddr = add i32 %arg, 8
  %argcptr = inttoptr i32 %argcaddr to i32*
  %argc = load i32, i32* %argcptr, align 1
  switch i32 %argc, label %error [i32 1, label %one_local_load
                                  i32 2, label %many_local_load
                                  i32 3, label %neg_local_load
                                  i32 4, label %one_global_load
                                  i32 5, label %many_global_load
                                  i32 6, label %neg_global_load
                                  i32 7, label %one_local_store
                                  i32 8, label %many_local_store
                                  i32 9, label %neg_local_store
                                  i32 10, label %one_global_store
                                  i32 11, label %many_global_store
                                  i32 12, label %neg_global_store]
one_local_load:
  ; Access one past the end of a local
  call void @access(i32 1, i32 1, i32 0)
  br label %error
many_local_load:
  ; Access five past the end of a local
  call void @access(i32 1, i32 1, i32 4)
  br label %error
neg_local_load:
  ; Access one before the beginning of a local
  call void @access(i32 1, i32 1, i32 -1)
  br label %error
one_global_load:
  ; Access one past the end of a global
  call void @access(i32 0, i32 1, i32 0)
  br label %error
many_global_load:
  ; Access five past the end of a global
  call void @access(i32 0, i32 1, i32 4)
  br label %error
neg_global_load:
  ; Access one before the beginning of a global
  call void @access(i32 0, i32 1, i32 -1)
  br label %error
one_local_store:
  ; Access one past the end of a local
  call void @access(i32 1, i32 0, i32 0)
  br label %error
many_local_store:
  ; Access five past the end of a local
  call void @access(i32 1, i32 0, i32 4)
  br label %error
neg_local_store:
  ; Access one before the beginning of a local
  call void @access(i32 1, i32 0, i32 -1)
  br label %error
one_global_store:
  ; Access one past the end of a global
  call void @access(i32 0, i32 0, i32 0)
  br label %error
many_global_store:
  ; Access five past the end of a global
  call void @access(i32 0, i32 0, i32 4)
  br label %error
neg_global_store:
  ; Access one before the beginning of a global
  call void @access(i32 0, i32 0, i32 -1)
  br label %error
error:
  call void @exit(i32 1)
  unreachable
}

; LOCAL-LOAD: Illegal 1 byte load from stack object at
; LOCAL-LOAD-NEXT: address of __asan_error symbol is
; LOCAL-STORE: Illegal 1 byte store to stack object at
; LOCAL-STORE-NEXT: address of __asan_error symbol is
; GLOBAL-LOAD: Illegal 1 byte load from global object at
; GLOBAL-LOAD-NEXT: address of __asan_error symbol is
; GLOBAL-STORE: Illegal 1 byte store to global object at
; GLOBAL-STORE-NEXT: address of __asan_error symbol is
