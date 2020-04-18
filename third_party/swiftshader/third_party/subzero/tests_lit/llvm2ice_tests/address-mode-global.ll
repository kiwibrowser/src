; This file checks support for address mode optimization.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s

@bytes = internal global [1024 x i8] zeroinitializer

define internal i32 @load_global_direct() {
entry:
  %base = ptrtoint [1024 x i8]* @bytes to i32
  %addr_lo.int = add i32 0, %base
  %addr_hi.int = add i32 4, %base
  %addr_lo.ptr = inttoptr i32 %addr_lo.int to i32*
  %addr_hi.ptr = inttoptr i32 %addr_hi.int to i32*
  %addr_lo.load = load i32, i32* %addr_lo.ptr, align 1
  %addr_hi.load = load i32, i32* %addr_hi.ptr, align 1
  %result = add i32 %addr_lo.load, %addr_hi.load
  ret i32 %result
; CHECK-LABEL: load_global_direct
; CHECK-NEXT: mov eax,{{(DWORD PTR )?}}ds:0x0{{.*}}{{bytes|.bss}}
; CHECK-NEXT: add eax,DWORD PTR ds:0x4{{.*}}{{bytes|.bss}}
}

define internal i32 @load_global_indexed(i32 %arg) {
entry:
  %offset = shl i32 %arg, 3
  %base = ptrtoint [1024 x i8]* @bytes to i32
  %addr.int = add i32 %offset, %base
  %addr.ptr = inttoptr i32 %addr.int to i32*
  %addr.load = load i32, i32* %addr.ptr, align 1
  ret i32 %addr.load
; CHECK-LABEL: load_global_indexed
; CHECK-NEXT: mov eax,DWORD PTR [esp+0x4]
; CHECK-NEXT: mov eax,DWORD PTR [eax*8+0x0]
}
