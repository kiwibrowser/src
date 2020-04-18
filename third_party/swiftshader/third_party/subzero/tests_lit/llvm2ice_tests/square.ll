; Test the a=b*b lowering sequence which can use a single temporary register
; instead of two registers.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 -mattr=sse4.1 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -Om1 -mattr=sse4.1 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

define internal float @Square_float(float %a) {
entry:
  %result = fmul float %a, %a
  ret float %result
}
; CHECK-LABEL: Square_float
; CHECK: mulss [[REG:xmm.]],[[REG]]
; MIPS32-LABEL: Square_float
; MIPS32: 	mov.s
; MIPS32: 	mul.s

define internal double @Square_double(double %a) {
entry:
  %result = fmul double %a, %a
  ret double %result
}
; CHECK-LABEL: Square_double
; CHECK: mulsd [[REG:xmm.]],[[REG]]
; MIPS32-LABEL: Square_double
; MIPS32: 	mov.d
; MIPS32: 	mul.d

define internal i32 @Square_i32(i32 %a) {
entry:
  %result = mul i32 %a, %a
  ret i32 %result
}
; CHECK-LABEL: Square_i32
; CHECK: imul [[REG:e..]],[[REG]]
; MIPS32-LABEL: Square_i32
; MIPS32: 	move
; MIPS32: 	mul

define internal i32 @Square_i16(i32 %a) {
entry:
  %a.16 = trunc i32 %a to i16
  %result = mul i16 %a.16, %a.16
  %result.i32 = sext i16 %result to i32
  ret i32 %result.i32
}
; CHECK-LABEL: Square_i16
; CHECK: imul [[REG:..]],[[REG]]
; MIPS32-LABEL: Square_i16
; MIPS32: 	move
; MIPS32: 	mul
; MIPS32: 	sll
; MIPS32: 	sra

define internal i32 @Square_i8(i32 %a) {
entry:
  %a.8 = trunc i32 %a to i8
  %result = mul i8 %a.8, %a.8
  %result.i32 = sext i8 %result to i32
  ret i32 %result.i32
}
; CHECK-LABEL: Square_i8
; CHECK: imul al
; MIPS32-LABEL: Square_i8
; MIPS32: 	move
; MIPS32: 	mul
; MIPS32: 	sll
; MIPS32: 	sra

define internal <4 x float> @Square_v4f32(<4 x float> %a) {
entry:
  %result = fmul <4 x float> %a, %a
  ret <4 x float> %result
}
; CHECK-LABEL: Square_v4f32
; CHECK: mulps [[REG:xmm.]],[[REG]]

define internal <4 x i32> @Square_v4i32(<4 x i32> %a) {
entry:
  %result = mul <4 x i32> %a, %a
  ret <4 x i32> %result
}
; CHECK-LABEL: Square_v4i32
; CHECK: pmulld [[REG:xmm.]],[[REG]]

define internal <8 x i16> @Square_v8i16(<8 x i16> %a) {
entry:
  %result = mul <8 x i16> %a, %a
  ret <8 x i16> %result
}
; CHECK-LABEL: Square_v8i16
; CHECK: pmullw [[REG:xmm.]],[[REG]]

define internal <16 x i8> @Square_v16i8(<16 x i8> %a) {
entry:
  %result = mul <16 x i8> %a, %a
  ret <16 x i8> %result
}
; CHECK-LABEL: Square_v16i8
; CHECK-NOT: pmul
