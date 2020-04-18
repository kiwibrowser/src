; Test the lowering sequence for commutative operations.  If there is a source
; operand whose lifetime ends in an operation, it should be the first operand,
; eliminating the need for a move to start the new lifetime.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

define internal i32 @integerAddLeft(i32 %a, i32 %b) {
entry:
  %tmp = add i32 %a, %b
  %result = add i32 %a, %tmp
  ret i32 %result
}
; CHECK-LABEL: integerAddLeft
; CHECK-NEXT: mov {{e..}},DWORD PTR
; CHECK-NEXT: mov {{e..}},DWORD PTR
; CHECK-NEXT: add {{e..}},{{e..}}
; CHECK-NEXT: add {{e..}},{{e..}}
; MIPS32-LABEL: integerAddLeft
; MIPS32: 	move	v0,a0
; MIPS32: 	addu	v0,v0,a1
; MIPS32: 	addu	a0,a0,v0

define internal i32 @integerAddRight(i32 %a, i32 %b) {
entry:
  %tmp = add i32 %a, %b
  %result = add i32 %b, %tmp
  ret i32 %result
}
; CHECK-LABEL: integerAddRight
; CHECK-NEXT: mov {{e..}},DWORD PTR
; CHECK-NEXT: mov {{e..}},DWORD PTR
; CHECK-NEXT: add {{e..}},{{e..}}
; CHECK-NEXT: add {{e..}},{{e..}}
; MIPS32-LABEL: integerAddRight
; MIPS32: 	move	v0,a1
; MIPS32: 	addu	a0,a0,v0
; MIPS32: 	addu	a1,a1,a0

define internal i32 @integerMultiplyLeft(i32 %a, i32 %b) {
entry:
  %tmp = mul i32 %a, %b
  %result = mul i32 %a, %tmp
  ret i32 %result
}
; CHECK-LABEL: integerMultiplyLeft
; CHECK-NEXT: mov {{e..}},DWORD PTR
; CHECK-NEXT: mov {{e..}},DWORD PTR
; CHECK-NEXT: imul {{e..}},{{e..}}
; CHECK-NEXT: imul {{e..}},{{e..}}
; MIPS32-LABEL: integerMultiplyLeft
; MIPS32: 	move	v0,a0
; MIPS32: 	mul	v0,v0,a1
; MIPS32: 	mul	a0,a0,v0

define internal i32 @integerMultiplyRight(i32 %a, i32 %b) {
entry:
  %tmp = mul i32 %a, %b
  %result = mul i32 %b, %tmp
  ret i32 %result
}
; CHECK-LABEL: integerMultiplyRight
; CHECK-NEXT: mov {{e..}},DWORD PTR
; CHECK-NEXT: mov {{e..}},DWORD PTR
; CHECK-NEXT: imul {{e..}},{{e..}}
; CHECK-NEXT: imul {{e..}},{{e..}}
; MIPS32-LABEL: integerMultiplyRight
; MIPS32: 	move	v0,a1
; MIPS32: 	mul	a0,a0,v0
; MIPS32: 	mul	a1,a1,a0

define internal float @floatAddLeft(float %a, float %b) {
entry:
  %tmp = fadd float %a, %b
  %result = fadd float %a, %tmp
  ret float %result
}
; CHECK-LABEL: floatAddLeft
; CHECK-NEXT: sub esp,0x1c
; CHECK-NEXT: movss xmm0,DWORD PTR
; CHECK-NEXT: movss xmm1,DWORD PTR
; CHECK-NEXT: addss xmm1,xmm0
; CHECK-NEXT: addss xmm0,xmm1
; MIPS32-LABEL: floatAddLeft
; MIPS32: 	mov.s	$f0,$f12
; MIPS32: 	add.s	$f0,$f0,$f14
; MIPS32: 	add.s	$f12,$f12,$f0

define internal float @floatAddRight(float %a, float %b) {
entry:
  %tmp = fadd float %a, %b
  %result = fadd float %b, %tmp
  ret float %result
}
; CHECK-LABEL: floatAddRight
; CHECK-NEXT: sub esp,0x1c
; CHECK-NEXT: movss xmm0,DWORD PTR
; CHECK-NEXT: movss xmm1,DWORD PTR
; CHECK-NEXT: addss xmm0,xmm1
; CHECK-NEXT: addss xmm1,xmm0
; MIPS32-LABEL: floatAddRight
; MIPS32: 	mov.s	$f0,$f14
; MIPS32: 	add.s	$f12,$f12,$f0
; MIPS32: 	add.s	$f14,$f14,$f12

define internal float @floatMultiplyLeft(float %a, float %b) {
entry:
  %tmp = fmul float %a, %b
  %result = fmul float %a, %tmp
  ret float %result
}
; CHECK-LABEL: floatMultiplyLeft
; CHECK-NEXT: sub esp,0x1c
; CHECK-NEXT: movss xmm0,DWORD PTR
; CHECK-NEXT: movss xmm1,DWORD PTR
; CHECK-NEXT: mulss xmm1,xmm0
; CHECK-NEXT: mulss xmm0,xmm1
; MIPS32-LABEL: floatMultiplyLeft
; MIPS32: 	mov.s	$f0,$f12
; MIPS32: 	mul.s	$f0,$f0,$f14
; MIPS32: 	mul.s	$f12,$f12,$f0

define internal float @floatMultiplyRight(float %a, float %b) {
entry:
  %tmp = fmul float %a, %b
  %result = fmul float %b, %tmp
  ret float %result
}
; CHECK-LABEL: floatMultiplyRight
; CHECK-NEXT: sub esp,0x1c
; CHECK-NEXT: movss xmm0,DWORD PTR
; CHECK-NEXT: movss xmm1,DWORD PTR
; CHECK-NEXT: mulss xmm0,xmm1
; CHECK-NEXT: mulss xmm1,xmm0
; MIPS32-LABEL: floatMultiplyRight
; MIPS32: 	mov.s	$f0,$f14
; MIPS32: 	mul.s	$f12,$f12,$f0
; MIPS32: 	mul.s	$f14,$f14,$f12
