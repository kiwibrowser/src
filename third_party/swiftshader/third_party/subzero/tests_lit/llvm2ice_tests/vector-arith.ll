; This test checks support for vector arithmetic.

; RUN: %p2i -i %s --filetype=obj --disassemble -a -O2 \
; RUN:   | FileCheck %s
; RUN: %p2i -i %s --filetype=obj --disassemble -a -Om1 \
; RUN:   | FileCheck %s
; RUN: %p2i -i %s --filetype=obj --disassemble -a -O2 -mattr=sse4.1 \
; RUN:   | FileCheck --check-prefix=SSE41 %s
; RUN: %p2i -i %s --filetype=obj --disassemble -a -Om1 -mattr=sse4.1 \
; RUN:   | FileCheck --check-prefix=SSE41 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

define internal <4 x float> @test_fadd(<4 x float> %arg0, <4 x float> %arg1) {
entry:
  %res = fadd <4 x float> %arg0, %arg1
  ret <4 x float> %res
; CHECK-LABEL: test_fadd
; CHECK: addps
; MIPS32-LABEL: test_fadd
; MIPS32: 	add.s
; MIPS32: 	add.s
; MIPS32: 	add.s
; MIPS32: 	add.s
}

define internal <4 x float> @test_fsub(<4 x float> %arg0, <4 x float> %arg1) {
entry:
  %res = fsub <4 x float> %arg0, %arg1
  ret <4 x float> %res
; CHECK-LABEL: test_fsub
; CHECK: subps
; MIPS32-LABEL: test_fsub
; MIPS32: 	sub.s
; MIPS32: 	sub.s
; MIPS32: 	sub.s
; MIPS32: 	sub.s
}

define internal <4 x float> @test_fmul(<4 x float> %arg0, <4 x float> %arg1) {
entry:
  %res = fmul <4 x float> %arg0, %arg1
  ret <4 x float> %res
; CHECK-LABEL: test_fmul
; CHECK: mulps
; MIPS32-LABEL: test_fmul
; MIPS32: 	mul.s
; MIPS32: 	mul.s
; MIPS32: 	mul.s
; MIPS32: 	mul.s
}

define internal <4 x float> @test_fdiv(<4 x float> %arg0, <4 x float> %arg1) {
entry:
  %res = fdiv <4 x float> %arg0, %arg1
  ret <4 x float> %res
; CHECK-LABEL: test_fdiv
; CHECK: divps
; MIPS32-LABEL: test_fdiv
; MIPS32: 	div.s
; MIPS32: 	div.s
; MIPS32: 	div.s
; MIPS32: 	div.s
}

define internal <4 x float> @test_frem(<4 x float> %arg0, <4 x float> %arg1) {
entry:
  %res = frem <4 x float> %arg0, %arg1
  ret <4 x float> %res
; CHECK-LABEL: test_frem
; CHECK: fmodf
; CHECK: fmodf
; CHECK: fmodf
; CHECK: fmodf
}

define internal <16 x i8> @test_add_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = add <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_add_v16i8
; CHECK: paddb
; MIPS32-LABEL: test_add_v16i8
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
}

define internal <16 x i8> @test_and_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = and <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_and_v16i8
; CHECK: pand
; MIPS32-LABEL: test_and_v16i8
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
}

define internal <16 x i8> @test_or_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = or <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_or_v16i8
; CHECK: por
; MIPS32-LABEL: test_or_v16i8
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
}

define internal <16 x i8> @test_xor_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = xor <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_xor_v16i8
; CHECK: pxor
; MIPS32-LABEL: test_xor_v16i8
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
}

define internal <16 x i8> @test_sub_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = sub <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_sub_v16i8
; CHECK: psubb
; MIPS32-LABEL: test_sub_v16i8
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
}

define internal <16 x i8> @test_mul_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = mul <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_mul_v16i8
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; CHECK: imul
; MIPS32-LABEL: test_mul_v16i8
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
}

define internal <16 x i8> @test_shl_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = shl <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_shl_v16i8
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; MIPS32-LABEL: test_shl_v16i8
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
}

define internal <16 x i8> @test_lshr_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = lshr <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_lshr_v16i8
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; MIPS32-LABEL: test_lshr_v16i8
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
}

define internal <16 x i8> @test_ashr_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = ashr <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_ashr_v16i8
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; MIPS32-LABEL: test_ashr_v16i8
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
}

define internal <16 x i8> @test_udiv_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = udiv <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_udiv_v16i8
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; MIPS32-LABEL: test_udiv_v16i8
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
}

define internal <16 x i8> @test_sdiv_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = sdiv <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_sdiv_v16i8
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; MIPS32-LABEL: test_sdiv_v16i8
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
}

define internal <16 x i8> @test_urem_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = urem <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_urem_v16i8
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; MIPS32-LABEL: test_urem_v16i8
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
}

define internal <16 x i8> @test_srem_v16i8(<16 x i8> %arg0, <16 x i8> %arg1) {
entry:
  %res = srem <16 x i8> %arg0, %arg1
  ret <16 x i8> %res
; CHECK-LABEL: test_srem_v16i8
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; MIPS32-LABEL: test_srem_v16i8
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
}

define internal <8 x i16> @test_add_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = add <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_add_v8i16
; CHECK: paddw
; MIPS32-LABEL: test_add_v8i16
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
}

define internal <8 x i16> @test_and_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = and <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_and_v8i16
; CHECK: pand
; MIPS32-LABEL: test_and_v8i16
; MIPS32: 	andi
; MIPS32: 	andi
; MIPS32: 	andi
; MIPS32: 	andi
; MIPS32: 	andi
; MIPS32: 	andi
; MIPS32: 	andi
; MIPS32: 	andi
}

define internal <8 x i16> @test_or_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = or <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_or_v8i16
; CHECK: por
; MIPS32-LABEL: test_or_v8i16
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
}

define internal <8 x i16> @test_xor_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = xor <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_xor_v8i16
; CHECK: pxor
; MIPS32-LABEL: test_xor_v8i16
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
}

define internal <8 x i16> @test_sub_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = sub <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_sub_v8i16
; CHECK: psubw
; MIPS32-LABEL: test_sub_v8i16
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
}

define internal <8 x i16> @test_mul_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = mul <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_mul_v8i16
; CHECK: pmullw
; MIPS32-LABEL: test_mul_v8i16
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
}

define internal <8 x i16> @test_shl_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = shl <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_shl_v8i16
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl
; MIPS32-LABEL: test_shl_v8i16
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
}

define internal <8 x i16> @test_lshr_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = lshr <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_lshr_v8i16
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; MIPS32-LABEL: test_lshr_v8i16
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
}

define internal <8 x i16> @test_ashr_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = ashr <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_ashr_v8i16
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; MIPS32-LABEL: test_ashr_v8i16
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
}

define internal <8 x i16> @test_udiv_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = udiv <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_udiv_v8i16
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; MIPS32-LABEL: test_udiv_v8i16
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
}

define internal <8 x i16> @test_sdiv_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = sdiv <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_sdiv_v8i16
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; MIPS32-LABEL: test_sdiv_v8i16
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
}

define internal <8 x i16> @test_urem_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = urem <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_urem_v8i16
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; MIPS32-LABEL: test_urem_v8i16
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
}

define internal <8 x i16> @test_srem_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = srem <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_srem_v8i16
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; MIPS32-LABEL: test_srem_v8i16
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
}

define internal <4 x i32> @test_add_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = add <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_add_v4i32
; CHECK: paddd
; MIPS32-LABEL: test_add_v4i32
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
; MIPS32: 	addu
}

define internal <4 x i32> @test_and_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = and <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_and_v4i32
; CHECK: pand
; MIPS32-LABEL: test_and_v4i32
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
; MIPS32: 	and
}

define internal <4 x i32> @test_or_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = or <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_or_v4i32
; CHECK: por
; MIPS32-LABEL: test_or_v4i32
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
; MIPS32: 	or
}

define internal <4 x i32> @test_xor_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = xor <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_xor_v4i32
; CHECK: pxor
; MIPS32-LABEL: test_xor_v4i32
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
; MIPS32: 	xor
}

define internal <4 x i32> @test_sub_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = sub <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_sub_v4i32
; CHECK: psubd
; MIPS32-LABEL: test_sub_v4i32
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
; MIPS32: 	subu
}

define internal <4 x i32> @test_mul_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = mul <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_mul_v4i32
; CHECK: pmuludq
; CHECK: pmuludq
;
; SSE41-LABEL: test_mul_v4i32
; SSE41: pmulld
; MIPS32-LABEL: test_mul_v4i32
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
; MIPS32: 	mul
}

define internal <4 x i32> @test_shl_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = shl <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_shl_v4i32
; CHECK: shl
; CHECK: shl
; CHECK: shl
; CHECK: shl

; This line is to ensure that pmulld is generated in test_mul_v4i32 above.
; SSE41-LABEL: test_shl_v4i32
; MIPS32-LABEL: test_shl_v4i32
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
; MIPS32: 	sllv
}

define internal <4 x i32> @test_lshr_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = lshr <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_lshr_v4i32
; CHECK: shr
; CHECK: shr
; CHECK: shr
; CHECK: shr
; MIPS32-LABEL: test_lshr_v4i32
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
; MIPS32: 	srlv
}

define internal <4 x i32> @test_ashr_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = ashr <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_ashr_v4i32
; CHECK: sar
; CHECK: sar
; CHECK: sar
; CHECK: sar
; MIPS32-LABEL: test_ashr_v4i32
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
; MIPS32: 	srav
}

define internal <4 x i32> @test_udiv_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = udiv <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_udiv_v4i32
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; MIPS32-LABEL: test_udiv_v4i32
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
}

define internal <4 x i32> @test_sdiv_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = sdiv <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_sdiv_v4i32
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; MIPS32-LABEL: test_sdiv_v4i32
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
}

define internal <4 x i32> @test_urem_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = urem <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_urem_v4i32
; CHECK: div
; CHECK: div
; CHECK: div
; CHECK: div
; MIPS32-LABEL: test_urem_v4i32
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
; MIPS32: 	divu
}

define internal <4 x i32> @test_srem_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = srem <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_srem_v4i32
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; CHECK: idiv
; MIPS32-LABEL: test_srem_v4i32
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
; MIPS32: 	div
}
