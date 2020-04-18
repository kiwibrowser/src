; This file tests casting / conversion operations that apply to vector types.
; bitcast operations are in vector-bitcast.ll.

; RUN: %p2i -i %s --target=x8632 --filetype=obj --disassemble --args -O2 \
; RUN:     | FileCheck %s --check-prefix=X8632 --check-prefix=CHECK
; RUN: %p2i -i %s --target=x8632 --filetype=obj --disassemble --args -Om1 \
; RUN:     | FileCheck %s --check-prefix=X8632 --check-prefix=CHECK

; RUN: %p2i -i %s --target=arm32 --filetype=obj --disassemble --args -O2 \
; RUN:     | FileCheck %s --check-prefix=ARM32 --check-prefix=CHECK
; RUN: %p2i -i %s --target=arm32 --filetype=obj --disassemble --args -Om1 \
; RUN:     | FileCheck %s --check-prefix=ARM32 --check-prefix=CHECK

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; sext operations

define internal <16 x i8> @test_sext_v16i1_to_v16i8(<16 x i1> %arg) {
entry:
  %res = sext <16 x i1> %arg to <16 x i8>
  ret <16 x i8> %res

; CHECK-LABEL: test_sext_v16i1_to_v16i8
; X8632: pxor
; X8632: pcmpeqb
; X8632: psubb
; X8632: pand
; X8632: pxor
; X8632: pcmpgtb
; ARM32: vshl.s8
; ARM32-NEXT: vshr.s8
; MIPS32: 	move	t2,a0
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	andi	t2,t2,0x1
; MIPS32: 	sll	t2,t2,0x1f
; MIPS32: 	sra	t2,t2,0x1f
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	or	t2,t2,v0
; MIPS32: 	move	v0,a0
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	sll	v0,v0,0x1f
; MIPS32: 	sra	v0,v0,0x1f
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	lui	t3,0xffff
; MIPS32: 	ori	t3,t3,0xff
; MIPS32: 	and	t2,t2,t3
; MIPS32: 	or	v0,v0,t2
; MIPS32: 	move	t2,a0
; MIPS32: 	srl	t2,t2,0x10
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	andi	t2,t2,0x1
; MIPS32: 	sll	t2,t2,0x1f
; MIPS32: 	sra	t2,t2,0x1f
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	sll	t2,t2,0x10
; MIPS32: 	lui	t3,0xff00
; MIPS32: 	ori	t3,t3,0xffff
; MIPS32: 	and	v0,v0,t3
; MIPS32: 	or	t2,t2,v0
; MIPS32: 	srl	a0,a0,0x18
; MIPS32: 	andi	a0,a0,0x1
; MIPS32: 	sll	a0,a0,0x1f
; MIPS32: 	sra	a0,a0,0x1f
; MIPS32: 	sll	a0,a0,0x18
; MIPS32: 	sll	t2,t2,0x8
; MIPS32: 	srl	t2,t2,0x8
; MIPS32: 	or	a0,a0,t2
; MIPS32: 	move	v0,a1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	sll	v0,v0,0x1f
; MIPS32: 	sra	v0,v0,0x1f
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	move	v1,a1
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	sll	v1,v1,0x1f
; MIPS32: 	sra	v1,v1,0x1f
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	lui	t2,0xffff
; MIPS32: 	ori	t2,t2,0xff
; MIPS32: 	and	v0,v0,t2
; MIPS32: 	or	v1,v1,v0
; MIPS32: 	move	v0,a1
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	sll	v0,v0,0x1f
; MIPS32: 	sra	v0,v0,0x1f
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	lui	t2,0xff00
; MIPS32: 	ori	t2,t2,0xffff
; MIPS32: 	and	v1,v1,t2
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a1,a1,0x18
; MIPS32: 	andi	a1,a1,0x1
; MIPS32: 	sll	a1,a1,0x1f
; MIPS32: 	sra	a1,a1,0x1f
; MIPS32: 	sll	a1,a1,0x18
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	or	a1,a1,v0
; MIPS32: 	move	v0,a2
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	sll	v0,v0,0x1f
; MIPS32: 	sra	v0,v0,0x1f
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	srl	t0,t0,0x8
; MIPS32: 	sll	t0,t0,0x8
; MIPS32: 	or	v0,v0,t0
; MIPS32: 	move	v1,a2
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	sll	v1,v1,0x1f
; MIPS32: 	sra	v1,v1,0x1f
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	lui	t0,0xffff
; MIPS32: 	ori	t0,t0,0xff
; MIPS32: 	and	v0,v0,t0
; MIPS32: 	or	v1,v1,v0
; MIPS32: 	move	v0,a2
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	sll	v0,v0,0x1f
; MIPS32: 	sra	v0,v0,0x1f
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	lui	t0,0xff00
; MIPS32: 	ori	t0,t0,0xffff
; MIPS32: 	and	v1,v1,t0
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a2,a2,0x18
; MIPS32: 	andi	a2,a2,0x1
; MIPS32: 	sll	a2,a2,0x1f
; MIPS32: 	sra	a2,a2,0x1f
; MIPS32: 	sll	a2,a2,0x18
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	or	a2,a2,v0
; MIPS32: 	move	v0,a3
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	sll	v0,v0,0x1f
; MIPS32: 	sra	v0,v0,0x1f
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	srl	t1,t1,0x8
; MIPS32: 	sll	t1,t1,0x8
; MIPS32: 	or	v0,v0,t1
; MIPS32: 	move	v1,a3
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	sll	v1,v1,0x1f
; MIPS32: 	sra	v1,v1,0x1f
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	lui	t0,0xffff
; MIPS32: 	ori	t0,t0,0xff
; MIPS32: 	and	v0,v0,t0
; MIPS32: 	or	v1,v1,v0
; MIPS32: 	move	v0,a3
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	sll	v0,v0,0x1f
; MIPS32: 	sra	v0,v0,0x1f
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	lui	t0,0xff00
; MIPS32: 	ori	t0,t0,0xffff
; MIPS32: 	and	v1,v1,t0
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a3,a3,0x18
; MIPS32: 	andi	a3,a3,0x1
; MIPS32: 	sll	a3,a3,0x1f
; MIPS32: 	sra	a3,a3,0x1f
; MIPS32: 	sll	a3,a3,0x18
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	or	a3,a3,v0
}

define internal <8 x i16> @test_sext_v8i1_to_v8i16(<8 x i1> %arg) {
entry:
  %res = sext <8 x i1> %arg to <8 x i16>
  ret <8 x i16> %res

; CHECK-LABEL: test_sext_v8i1_to_v8i16
; X8632: psllw {{.*}},0xf
; X8632: psraw {{.*}},0xf
; ARM32: vshl.s16
; ARM32-NEXT: vshr.s16
; MIPS32: 	move	v0,zero
; MIPS32: 	move	v1,zero
; MIPS32: 	move	t0,zero
; MIPS32: 	move	t1,zero
; MIPS32: 	move	t2,a0
; MIPS32: 	andi	t2,t2,0xffff
; MIPS32: 	andi	t2,t2,0x1
; MIPS32: 	sll	t2,t2,0x1f
; MIPS32: 	sra	t2,t2,0x1f
; MIPS32: 	andi	t2,t2,0xffff
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	or	t2,t2,v0
; MIPS32: 	srl	a0,a0,0x10
; MIPS32: 	andi	a0,a0,0x1
; MIPS32: 	sll	a0,a0,0x1f
; MIPS32: 	sra	a0,a0,0x1f
; MIPS32: 	sll	a0,a0,0x10
; MIPS32: 	sll	t2,t2,0x10
; MIPS32: 	srl	t2,t2,0x10
; MIPS32: 	or	a0,a0,t2
; MIPS32: 	move	v0,a1
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	sll	v0,v0,0x1f
; MIPS32: 	sra	v0,v0,0x1f
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	srl	v1,v1,0x10
; MIPS32: 	sll	v1,v1,0x10
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a1,a1,0x10
; MIPS32: 	andi	a1,a1,0x1
; MIPS32: 	sll	a1,a1,0x1f
; MIPS32: 	sra	a1,a1,0x1f
; MIPS32: 	sll	a1,a1,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	or	a1,a1,v0
; MIPS32: 	move	v0,a2
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	sll	v0,v0,0x1f
; MIPS32: 	sra	v0,v0,0x1f
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	srl	t0,t0,0x10
; MIPS32: 	sll	t0,t0,0x10
; MIPS32: 	or	v0,v0,t0
; MIPS32: 	srl	a2,a2,0x10
; MIPS32: 	andi	a2,a2,0x1
; MIPS32: 	sll	a2,a2,0x1f
; MIPS32: 	sra	a2,a2,0x1f
; MIPS32: 	sll	a2,a2,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	or	a2,a2,v0
; MIPS32: 	move	v0,a3
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	sll	v0,v0,0x1f
; MIPS32: 	sra	v0,v0,0x1f
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	srl	t1,t1,0x10
; MIPS32: 	sll	t1,t1,0x10
; MIPS32: 	or	v0,v0,t1
; MIPS32: 	srl	a3,a3,0x10
; MIPS32: 	andi	a3,a3,0x1
; MIPS32: 	sll	a3,a3,0x1f
; MIPS32: 	sra	a3,a3,0x1f
; MIPS32: 	sll	a3,a3,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	or	a3,a3,v0
}

define internal <4 x i32> @test_sext_v4i1_to_v4i32(<4 x i1> %arg) {
entry:
  %res = sext <4 x i1> %arg to <4 x i32>
  ret <4 x i32> %res

; CHECK-LABEL: test_sext_v4i1_to_v4i32
; X8632: pslld {{.*}},0x1f
; X8632: psrad {{.*}},0x1f
; ARM32: vshl.s32
; ARM32-NEXT: vshr.s32
; MIPS32: 	andi	a0,a0,0x1
; MIPS32: 	sll	a0,a0,0x1f
; MIPS32: 	sra	a0,a0,0x1f
; MIPS32: 	andi	a1,a1,0x1
; MIPS32: 	sll	a1,a1,0x1f
; MIPS32: 	sra	a1,a1,0x1f
; MIPS32: 	andi	a2,a2,0x1
; MIPS32: 	sll	a2,a2,0x1f
; MIPS32: 	sra	a2,a2,0x1f
; MIPS32: 	andi	a3,a3,0x1
; MIPS32: 	sll	a3,a3,0x1f
; MIPS32: 	sra	a3,a3,0x1f
}

; zext operations

define internal <16 x i8> @test_zext_v16i1_to_v16i8(<16 x i1> %arg) {
entry:
  %res = zext <16 x i1> %arg to <16 x i8>
  ret <16 x i8> %res

; CHECK-LABEL: test_zext_v16i1_to_v16i8
; X8632: pxor
; X8632: pcmpeqb
; X8632: psubb
; X8632: pand
; ARM32: vmov.i8 [[S:.*]], #1
; ARM32-NEXT: vand {{.*}}, [[S]]
; MIPS32: 	move	t2,a0
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	andi	t2,t2,0x1
; MIPS32: 	andi	t2,t2,0x1
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	or	t2,t2,v0
; MIPS32: 	move	v0,a0
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	lui	t3,0xffff
; MIPS32: 	ori	t3,t3,0xff
; MIPS32: 	and	t2,t2,t3
; MIPS32: 	or	v0,v0,t2
; MIPS32: 	move	t2,a0
; MIPS32: 	srl	t2,t2,0x10
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	andi	t2,t2,0x1
; MIPS32: 	andi	t2,t2,0x1
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	sll	t2,t2,0x10
; MIPS32: 	lui	t3,0xff00
; MIPS32: 	ori	t3,t3,0xffff
; MIPS32: 	and	v0,v0,t3
; MIPS32: 	or	t2,t2,v0
; MIPS32: 	srl	a0,a0,0x18
; MIPS32: 	andi	a0,a0,0x1
; MIPS32: 	andi	a0,a0,0x1
; MIPS32: 	sll	a0,a0,0x18
; MIPS32: 	sll	t2,t2,0x8
; MIPS32: 	srl	t2,t2,0x8
; MIPS32: 	or	a0,a0,t2
; MIPS32: 	move	v0,a1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	move	v1,a1
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	lui	t2,0xffff
; MIPS32: 	ori	t2,t2,0xff
; MIPS32: 	and	v0,v0,t2
; MIPS32: 	or	v1,v1,v0
; MIPS32: 	move	v0,a1
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	lui	t2,0xff00
; MIPS32: 	ori	t2,t2,0xffff
; MIPS32: 	and	v1,v1,t2
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a1,a1,0x18
; MIPS32: 	andi	a1,a1,0x1
; MIPS32: 	andi	a1,a1,0x1
; MIPS32: 	sll	a1,a1,0x18
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	or	a1,a1,v0
; MIPS32: 	move	v0,a2
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	srl	t0,t0,0x8
; MIPS32: 	sll	t0,t0,0x8
; MIPS32: 	or	v0,v0,t0
; MIPS32: 	move	v1,a2
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	lui	t0,0xffff
; MIPS32: 	ori	t0,t0,0xff
; MIPS32: 	and	v0,v0,t0
; MIPS32: 	or	v1,v1,v0
; MIPS32: 	move	v0,a2
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	lui	t0,0xff00
; MIPS32: 	ori	t0,t0,0xffff
; MIPS32: 	and	v1,v1,t0
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a2,a2,0x18
; MIPS32: 	andi	a2,a2,0x1
; MIPS32: 	andi	a2,a2,0x1
; MIPS32: 	sll	a2,a2,0x18
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	or	a2,a2,v0
; MIPS32: 	move	v0,a3
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	srl	t1,t1,0x8
; MIPS32: 	sll	t1,t1,0x8
; MIPS32: 	or	v0,v0,t1
; MIPS32: 	move	v1,a3
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	lui	t0,0xffff
; MIPS32: 	ori	t0,t0,0xff
; MIPS32: 	and	v0,v0,t0
; MIPS32: 	or	v1,v1,v0
; MIPS32: 	move	v0,a3
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	lui	t0,0xff00
; MIPS32: 	ori	t0,t0,0xffff
; MIPS32: 	and	v1,v1,t0
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a3,a3,0x18
; MIPS32: 	andi	a3,a3,0x1
; MIPS32: 	andi	a3,a3,0x1
; MIPS32: 	sll	a3,a3,0x18
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	or	a3,a3,v0
}

define internal <8 x i16> @test_zext_v8i1_to_v8i16(<8 x i1> %arg) {
entry:
  %res = zext <8 x i1> %arg to <8 x i16>
  ret <8 x i16> %res

; CHECK-LABEL: test_zext_v8i1_to_v8i16
; X8632: pxor
; X8632: pcmpeqw
; X8632: psubw
; X8632: pand
; ARM32: vmov.i16 [[S:.*]], #1
; ARM32-NEXT: vand {{.*}}, [[S]]
; MIPS32: 	move	t2,a0
; MIPS32: 	andi	t2,t2,0xffff
; MIPS32: 	andi	t2,t2,0x1
; MIPS32: 	andi	t2,t2,0x1
; MIPS32: 	andi	t2,t2,0xffff
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	or	t2,t2,v0
; MIPS32: 	srl	a0,a0,0x10
; MIPS32: 	andi	a0,a0,0x1
; MIPS32: 	andi	a0,a0,0x1
; MIPS32: 	sll	a0,a0,0x10
; MIPS32: 	sll	t2,t2,0x10
; MIPS32: 	srl	t2,t2,0x10
; MIPS32: 	or	a0,a0,t2
; MIPS32: 	move	v0,a1
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	srl	v1,v1,0x10
; MIPS32: 	sll	v1,v1,0x10
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a1,a1,0x10
; MIPS32: 	andi	a1,a1,0x1
; MIPS32: 	andi	a1,a1,0x1
; MIPS32: 	sll	a1,a1,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	or	a1,a1,v0
; MIPS32: 	move	v0,a2
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	srl	t0,t0,0x10
; MIPS32: 	sll	t0,t0,0x10
; MIPS32: 	or	v0,v0,t0
; MIPS32: 	srl	a2,a2,0x10
; MIPS32: 	andi	a2,a2,0x1
; MIPS32: 	andi	a2,a2,0x1
; MIPS32: 	sll	a2,a2,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	or	a2,a2,v0
; MIPS32: 	move	v0,a3
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	srl	t1,t1,0x10
; MIPS32: 	sll	t1,t1,0x10
; MIPS32: 	or	v0,v0,t1
; MIPS32: 	srl	a3,a3,0x10
; MIPS32: 	andi	a3,a3,0x1
; MIPS32: 	andi	a3,a3,0x1
; MIPS32: 	sll	a3,a3,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	or	a3,a3,v0
}

define internal <4 x i32> @test_zext_v4i1_to_v4i32(<4 x i1> %arg) {
entry:
  %res = zext <4 x i1> %arg to <4 x i32>
  ret <4 x i32> %res

; CHECK-LABEL: test_zext_v4i1_to_v4i32
; X8632: pxor
; X8632: pcmpeqd
; X8632: psubd
; X8632: pand
; ARM32: vmov.i32 [[S:.*]], #1
; ARM32-NEXT: vand {{.*}}, [[S]]
; MIPS32: 	andi	a0,a0,0x1
; MIPS32: 	andi	a0,a0,0x1
; MIPS32: 	andi	a1,a1,0x1
; MIPS32: 	andi	a1,a1,0x1
; MIPS32: 	andi	a2,a2,0x1
; MIPS32: 	andi	a2,a2,0x1
; MIPS32: 	andi	a3,a3,0x1
; MIPS32: 	andi	a3,a3,0x1
}

; trunc operations

define internal <16 x i1> @test_trunc_v16i8_to_v16i1(<16 x i8> %arg) {
entry:
  %res = trunc <16 x i8> %arg to <16 x i1>
  ret <16 x i1> %res

; CHECK-LABEL: test_trunc_v16i8_to_v16i1
; X8632: pxor
; X8632: pcmpeqb
; X8632: psubb
; X8632: pand
; MIPS32: 	move	t2,a0
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	andi	t2,t2,0x1
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	or	t2,t2,v0
; MIPS32: 	move	v0,a0
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	lui	t3,0xffff
; MIPS32: 	ori	t3,t3,0xff
; MIPS32: 	and	t2,t2,t3
; MIPS32: 	or	v0,v0,t2
; MIPS32: 	move	t2,a0
; MIPS32: 	srl	t2,t2,0x10
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	andi	t2,t2,0x1
; MIPS32: 	andi	t2,t2,0xff
; MIPS32: 	sll	t2,t2,0x10
; MIPS32: 	lui	t3,0xff00
; MIPS32: 	ori	t3,t3,0xffff
; MIPS32: 	and	v0,v0,t3
; MIPS32: 	or	t2,t2,v0
; MIPS32: 	srl	a0,a0,0x18
; MIPS32: 	andi	a0,a0,0x1
; MIPS32: 	sll	a0,a0,0x18
; MIPS32: 	sll	t2,t2,0x8
; MIPS32: 	srl	t2,t2,0x8
; MIPS32: 	or	a0,a0,t2
; MIPS32: 	move	v0,a1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	move	v1,a1
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	lui	t2,0xffff
; MIPS32: 	ori	t2,t2,0xff
; MIPS32: 	and	v0,v0,t2
; MIPS32: 	or	v1,v1,v0
; MIPS32: 	move	v0,a1
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	lui	t2,0xff00
; MIPS32: 	ori	t2,t2,0xffff
; MIPS32: 	and	v1,v1,t2
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a1,a1,0x18
; MIPS32: 	andi	a1,a1,0x1
; MIPS32: 	sll	a1,a1,0x18
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	or	a1,a1,v0
; MIPS32: 	move	v0,a2
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	srl	t0,t0,0x8
; MIPS32: 	sll	t0,t0,0x8
; MIPS32: 	or	v0,v0,t0
; MIPS32: 	move	v1,a2
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	lui	t0,0xffff
; MIPS32: 	ori	t0,t0,0xff
; MIPS32: 	and	v0,v0,t0
; MIPS32: 	or	v1,v1,v0
; MIPS32: 	move	v0,a2
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	lui	t0,0xff00
; MIPS32: 	ori	t0,t0,0xffff
; MIPS32: 	and	v1,v1,t0
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a2,a2,0x18
; MIPS32: 	andi	a2,a2,0x1
; MIPS32: 	sll	a2,a2,0x18
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	or	a2,a2,v0
; MIPS32: 	move	v0,a3
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	srl	t1,t1,0x8
; MIPS32: 	sll	t1,t1,0x8
; MIPS32: 	or	v0,v0,t1
; MIPS32: 	move	v1,a3
; MIPS32: 	srl	v1,v1,0x8
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	andi	v1,v1,0x1
; MIPS32: 	andi	v1,v1,0xff
; MIPS32: 	sll	v1,v1,0x8
; MIPS32: 	lui	t0,0xffff
; MIPS32: 	ori	t0,t0,0xff
; MIPS32: 	and	v0,v0,t0
; MIPS32: 	or	v1,v1,v0
; MIPS32: 	move	v0,a3
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	andi	v0,v0,0x1
; MIPS32: 	andi	v0,v0,0xff
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	lui	t0,0xff00
; MIPS32: 	ori	t0,t0,0xffff
; MIPS32: 	and	v1,v1,t0
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a3,a3,0x18
; MIPS32: 	andi	a3,a3,0x1
; MIPS32: 	sll	a3,a3,0x18
; MIPS32: 	sll	v0,v0,0x8
; MIPS32: 	srl	v0,v0,0x8
; MIPS32: 	or	a3,a3,v0
}

define internal <8 x i1> @test_trunc_v8i16_to_v8i1(<8 x i16> %arg) {
entry:
  %res = trunc <8 x i16> %arg to <8 x i1>
  ret <8 x i1> %res

; CHECK-LABEL: test_trunc_v8i16_to_v8i1
; X8632: pxor
; X8632: pcmpeqw
; X8632: psubw
; X8632: pand
; MIPS32: 	move	t2,a0
; MIPS32: 	andi	t2,t2,0xffff
; MIPS32: 	andi	t2,t2,0xffff
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	or	t2,t2,v0
; MIPS32: 	srl	a0,a0,0x10
; MIPS32: 	sll	a0,a0,0x10
; MIPS32: 	sll	t2,t2,0x10
; MIPS32: 	srl	t2,t2,0x10
; MIPS32: 	or	a0,a0,t2
; MIPS32: 	move	v0,a1
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	srl	v1,v1,0x10
; MIPS32: 	sll	v1,v1,0x10
; MIPS32: 	or	v0,v0,v1
; MIPS32: 	srl	a1,a1,0x10
; MIPS32: 	sll	a1,a1,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	or	a1,a1,v0
; MIPS32: 	move	v0,a2
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	srl	t0,t0,0x10
; MIPS32: 	sll	t0,t0,0x10
; MIPS32: 	or	v0,v0,t0
; MIPS32: 	srl	a2,a2,0x10
; MIPS32: 	sll	a2,a2,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	or	a2,a2,v0
; MIPS32: 	move	v0,a3
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	andi	v0,v0,0xffff
; MIPS32: 	srl	t1,t1,0x10
; MIPS32: 	sll	t1,t1,0x10
; MIPS32: 	or	v0,v0,t1
; MIPS32: 	srl	a3,a3,0x10
; MIPS32: 	sll	a3,a3,0x10
; MIPS32: 	sll	v0,v0,0x10
; MIPS32: 	srl	v0,v0,0x10
; MIPS32: 	or	a3,a3,v0
}

define internal <4 x i1> @test_trunc_v4i32_to_v4i1(<4 x i32> %arg) {
entry:
  %res = trunc <4 x i32> %arg to <4 x i1>
  ret <4 x i1> %res

; CHECK-LABEL: test_trunc_v4i32_to_v4i1
; X8632: pxor
; X8632: pcmpeqd
; X8632: psubd
; X8632: pand
; MIPS32: 	move	v0,a0
; MIPS32: 	move	v1,a1
; MIPS32: 	move	a0,a2
; MIPS32: 	move	a1,a3
}

; fpto[us]i operations

define internal <4 x i32> @test_fptosi_v4f32_to_v4i32(<4 x float> %arg) {
entry:
  %res = fptosi <4 x float> %arg to <4 x i32>
  ret <4 x i32> %res

; CHECK-LABEL: test_fptosi_v4f32_to_v4i32
; X8632: cvttps2dq
; ARM32: vcvt.s32.f32
; MIPS32: 	trunc.w.s	$f0,$f0
; MIPS32: 	trunc.w.s	$f0,$f0
; MIPS32: 	trunc.w.s	$f0,$f0
; MIPS32: 	trunc.w.s	$f0,$f0
}

define internal <4 x i32> @test_fptoui_v4f32_to_v4i32(<4 x float> %arg) {
entry:
  %res = fptoui <4 x float> %arg to <4 x i32>
  ret <4 x i32> %res

; CHECK-LABEL: test_fptoui_v4f32_to_v4i32
; X8632: call {{.*}} R_{{.*}} __Sz_fptoui_4xi32_f32
; ARM32: vcvt.u32.f32
; MIPS32: 	trunc.w.s	$f0,$f0
; MIPS32: 	trunc.w.s	$f0,$f0
; MIPS32: 	trunc.w.s	$f0,$f0
; MIPS32: 	trunc.w.s	$f0,$f0
}

; [su]itofp operations

define internal <4 x float> @test_sitofp_v4i32_to_v4f32(<4 x i32> %arg) {
entry:
  %res = sitofp <4 x i32> %arg to <4 x float>
  ret <4 x float> %res

; CHECK-LABEL: test_sitofp_v4i32_to_v4f32
; X8632: cvtdq2ps
; ARM32: vcvt.f32.s32
; MIPS32: 	cvt.s.w	$f0,$f0
; MIPS32: 	cvt.s.w	$f0,$f0
; MIPS32: 	cvt.s.w	$f0,$f0
; MIPS32: 	cvt.s.w	$f0,$f0

}

define internal <4 x float> @test_uitofp_v4i32_to_v4f32(<4 x i32> %arg) {
entry:
  %res = uitofp <4 x i32> %arg to <4 x float>
  ret <4 x float> %res

; CHECK-LABEL: test_uitofp_v4i32_to_v4f32
; X8632: call {{.*}} R_{{.*}} __Sz_uitofp_4xi32_4xf32
; ARM32: vcvt.f32.u32
; MIPS32: 	cvt.s.w	$f0,$f0
; MIPS32: 	cvt.s.w	$f0,$f0
; MIPS32: 	cvt.s.w	$f0,$f0
; MIPS32: 	cvt.s.w	$f0,$f0
}
