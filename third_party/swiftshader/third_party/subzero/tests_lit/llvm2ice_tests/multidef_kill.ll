; This tests against a lowering error in a multiply instruction that produces
; results in a low and high register.  This is usually lowered as a mul
; instruction whose dest contains the low portion, and a FakeDef of the high
; portion.  The problem is that if the high portion is unused (e.g. the multiply
; is followed by a truncation), the FakeDef may be eliminated, and the register
; allocator may assign the high register to a variable that is live across the
; mul instruction.  This is incorrect because the mul instruction smashes the
; register.

; REQUIRES: allow_dump

; RUN: %p2i --target x8632 -i %s --filetype=asm --args -O2 -asm-verbose \
; RUN:   --split-local-vars=0 \
; RUN:   --reg-use=eax,edx -reg-reserve | FileCheck --check-prefix=X8632 %s
; RUN: %p2i --target arm32 -i %s --filetype=asm --args -O2 -asm-verbose \
; RUN:   | FileCheck --check-prefix=ARM32 %s

define internal i32 @mul(i64 %a, i64 %b, i32 %c) {
  ; Force an early use of %c.
  store i32 %c, i32* undef, align 1
  %m = mul i64 %a, %b
  %t = trunc i64 %m to i32
  ; Make many uses of %c to give it high weight.
  %t1 = add i32 %t, %c
  %t2 = add i32 %t1, %c
  %t3 = add i32 %t2, %c
  ret i32 %t3
}

; For x8632, we want asm-verbose to print the stack offset assignment for lv$c
; ("local variable 'c'") in the prolog, and then have at least one use of lv$c
; in the body, i.e. don't register-allocate edx to %c.

; X8632-LABEL: mul
; X8632: lv$c =
; X8632: lv$c

; For arm32, the failure would manifest as a translation error - no register
; being allocated to the high operand, so we just check for successful
; translation.

; ARM32-LABEL: mul
