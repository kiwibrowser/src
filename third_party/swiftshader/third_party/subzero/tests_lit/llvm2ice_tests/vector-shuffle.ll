; Some shufflevector optimized lowering. This list is by no means exhaustive. It
; is only a **basic** smoke test. the vector_ops crosstest has a broader range
; of test cases.

; RUN: %p2i -i %s --target=x8632 --filetype=obj --disassemble -a -O2 \
; RUN:     --allow-externally-defined-symbols | FileCheck %s --check-prefix=X86

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

declare void @useV4I32(<4 x i32> %t);

define internal void @shuffleV4I32(<4 x i32> %a, <4 x i32> %b) {
; X86-LABEL: shuffleV4I32
  %a_0 = extractelement <4 x i32> %a, i32 0
  %a_1 = extractelement <4 x i32> %a, i32 1
  %a_2 = extractelement <4 x i32> %a, i32 2
  %a_3 = extractelement <4 x i32> %a, i32 3

  %b_0 = extractelement <4 x i32> %b, i32 0
  %b_1 = extractelement <4 x i32> %b, i32 1
  %b_2 = extractelement <4 x i32> %b, i32 2
  %b_3 = extractelement <4 x i32> %b, i32 3

  %t0_0 = insertelement <4 x i32> undef, i32 %a_0, i32 0
  %t0_1 = insertelement <4 x i32> %t0_0, i32 %b_0, i32 1
  %t0_2 = insertelement <4 x i32> %t0_1, i32 %a_1, i32 2
  %t0   = insertelement <4 x i32> %t0_2, i32 %b_1, i32 3
; X86: punpckldq {{.*}}

  call void @useV4I32(<4 x i32> %t0)
; X86: call

  %t1_0 = insertelement <4 x i32> undef, i32 %a_0, i32 0
  %t1_1 = insertelement <4 x i32> %t1_0, i32 %b_1, i32 1
  %t1_2 = insertelement <4 x i32> %t1_1, i32 %b_1, i32 2
  %t1   = insertelement <4 x i32> %t1_2, i32 %a_0, i32 3
; X86: shufps [[T:xmm[0-9]+]],{{.*}},0x10
; X86: pshufd {{.*}},[[T]],0x28

  call void @useV4I32(<4 x i32> %t1)
; X86: call

  %t2_0 = insertelement <4 x i32> undef, i32 %a_0, i32 0
  %t2_1 = insertelement <4 x i32> %t2_0, i32 %b_3, i32 1
  %t2_2 = insertelement <4 x i32> %t2_1, i32 %a_2, i32 2
  %t2   = insertelement <4 x i32> %t2_2, i32 %b_2, i32 3
; X86: shufps {{.*}},0x30
; X86: shufps {{.*}},0x22
; X86: shufps {{.*}},0x88

  call void @useV4I32(<4 x i32> %t2)
; X86: call

  ret void
}
; MIPS32-LABEL: shuffleV4I32
; MIPS32: 	move
; MIPS32: 	move
; MIPS32: 	move
; MIPS32: 	move
; MIPS32: 	jal
; MIPS32: 	move
; MIPS32: 	move
; MIPS32: 	move
; MIPS32: 	move
; MIPS32: 	jal
; MIPS32: 	move
; MIPS32: 	move
; MIPS32: 	move
; MIPS32: 	move
; MIPS32: 	jal
