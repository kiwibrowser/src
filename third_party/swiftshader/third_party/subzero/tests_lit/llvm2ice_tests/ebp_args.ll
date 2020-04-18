; This test originally exhibited a bug in ebp-based stack slots.  The
; problem was that during a function call push sequence, the esp
; adjustment was incorrectly added to the stack/frame offset for
; ebp-based frames.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s

declare i32 @memcpy_helper2(i32 %buf, i32 %buf2, i32 %n)

define internal i32 @memcpy_helper(i32 %buf, i32 %n) {
entry:
  br label %eblock  ; Disable alloca optimization
eblock:
  %buf2 = alloca i8, i32 128, align 4
  %n.arg_trunc = trunc i32 %n to i8
  %arg.ext = zext i8 %n.arg_trunc to i32
  %buf2.asint = ptrtoint i8* %buf2 to i32
  %call = call i32 @memcpy_helper2(i32 %buf, i32 %buf2.asint, i32 %arg.ext)
  ret i32 %call
}

; This check sequence is highly specific to the current Om1 lowering
; and stack slot assignment code, and may need to be relaxed if the
; lowering code changes.

; CHECK-LABEL: memcpy_helper
; CHECK:  push  ebp
; CHECK:  mov   ebp,esp
; CHECK:  sub   esp,0x28
; CHECK:  sub   esp,0x80
; CHECK:  lea   eax,[esp+0x10]
; CHECK:  mov   DWORD PTR [ebp-0x4],eax
; CHECK:  mov   eax,DWORD PTR [ebp+0xc]
; CHECK:  mov   BYTE PTR [ebp-0x8],al
; CHECK:  movzx eax,BYTE PTR [ebp-0x8]
; CHECK:  mov   DWORD PTR [ebp-0xc],eax
; CHECK:  mov   eax,DWORD PTR [ebp+0x8]
; CHECK:  mov   DWORD PTR [esp],eax
; CHECK:  mov   eax,DWORD PTR [ebp-0x4]
; CHECK:  mov   DWORD PTR [esp+0x4],eax
; CHECK:  mov   eax,DWORD PTR [ebp-0xc]
; CHECK:  mov   DWORD PTR [esp+0x8],eax
; CHECK:  call {{.*}} R_{{.*}} memcpy_helper2
