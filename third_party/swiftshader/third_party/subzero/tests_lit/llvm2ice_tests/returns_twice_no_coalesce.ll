; This file checks that SimpleCoalescing of local stack slots is not done
; when calling a function with the "returns twice" attribute.

; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:      -allow-externally-defined-symbols | FileCheck %s

; Setjmp is a function with the "returns twice" attribute.
declare i32 @llvm.nacl.setjmp(i8*)

declare i32 @other(i32)
declare void @user(i32)

define internal i32 @call_returns_twice(i32 %iptr_jmpbuf, i32 %x) {
entry:
  %local = add i32 %x, 12345
  %jmpbuf = inttoptr i32 %iptr_jmpbuf to i8*
  %y = call i32 @llvm.nacl.setjmp(i8* %jmpbuf)
  call void @user(i32 %local)
  %cmp = icmp eq i32 %y, 0
  br i1 %cmp, label %Zero, label %NonZero
Zero:
  %other_local = add i32 %x, 54321
  call void @user(i32 %other_local)
  ret i32 %other_local
NonZero:
  ret i32 1
}

; CHECK-LABEL: call_returns_twice
; CHECK: add [[REG1:.*]],0x3039
; CHECK: mov DWORD PTR [esp+[[OFF:.*]]],[[REG1]]
; CHECK: add [[REG2:.*]],0xd431
; There should not be sharing of the stack slot.
; CHECK-NOT: mov DWORD PTR [esp + [[OFF]]], [[REG2]]

define internal i32 @no_call_returns_twice(i32 %iptr_jmpbuf, i32 %x) {
entry:
  %local = add i32 %x, 12345
  %y = call i32 @other(i32 %x)
  call void @user(i32 %local)
  %cmp = icmp eq i32 %y, 0
  br i1 %cmp, label %Zero, label %NonZero
Zero:
  %other_local = add i32 %x, 54321
  call void @user(i32 %other_local)
  ret i32 %other_local
NonZero:
  ret i32 1
}

; CHECK-LABEL: no_call_returns_twice
; CHECK: add [[REG1:.*]],0x3039
; CHECK: mov DWORD PTR [esp+[[OFF:.*]]],[[REG1]]
; CHECK: add [[REG2:.*]],0xd431
; Now there should be sharing of the stack slot (OFF is the same).
; Commenting out after disabling simple coalescing for -Om1.
; TODO(stichnot): Add it back if/when we add a flag to enable simple
; coalescing.
; xCHECK: mov DWORD PTR [esp + [[OFF]]], [[REG2]]
