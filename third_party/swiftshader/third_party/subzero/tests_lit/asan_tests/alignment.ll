; Translate with -fsanitize-address and -O2 to test alignment and ordering of
; redzones when allocas are coalesced.

; REQUIRES: no_minimal_build

; RUN: %p2i --filetype=obj --disassemble --target x8632 -i %s --args -O2 \
; RUN:     -allow-externally-defined-symbols -fsanitize-address | FileCheck %s

define internal i32 @func(i32 %arg1, i32 %arg2) {
  %l1 = alloca i8, i32 4, align 4
  %l2 = alloca i8, i32 5, align 1
  ret i32 42
}

; CHECK: func
; CHECK-NEXT: sub    esp,0xac
; CHECK-NEXT: lea    eax,[esp]
; CHECK-NEXT: shr    eax,0x3
; CHECK-NEXT: mov    DWORD PTR [eax+0x20000000],0xffffffff
; CHECK-NEXT: mov    DWORD PTR [eax+0x20000004],0xffffff04
; CHECK-NEXT: mov    DWORD PTR [eax+0x20000008],0xffffffff
; CHECK-NEXT: mov    DWORD PTR [eax+0x2000000c],0xffffff05
; CHECK-NEXT: mov    DWORD PTR [eax+0x20000010],0xffffffff
; CHECK-NEXT: mov    DWORD PTR [eax+0x20000000],0x0
; CHECK-NEXT: mov    DWORD PTR [eax+0x20000004],0x0
; CHECK-NEXT: mov    DWORD PTR [eax+0x20000008],0x0
; CHECK-NEXT: mov    DWORD PTR [eax+0x2000000c],0x0
; CHECK-NEXT: mov    DWORD PTR [eax+0x20000010],0x0
; CHECK-NEXT: mov    eax,0x2a
; CHECK-NEXT: add    esp,0xac
; CHECK-NEXT: ret
