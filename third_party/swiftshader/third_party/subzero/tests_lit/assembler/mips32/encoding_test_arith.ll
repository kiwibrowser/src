; Test encoding of MIPS32 arithmetic instructions

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=mips32 --args -O2 \
; RUN:   --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=mips32 --assemble --disassemble \
; RUN:   --args -O2 --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=mips32 --args -O2 \
; RUN:   --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=mips32 --assemble --disassemble \
; RUN:   --args -O2 --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal i32 @test_01(i32 %a) {
  %v = add i32 %a, 1
  %v1 = and i32 %v, 1
  %v2 = or i32 %v1, 1
  %v3 = xor i32 %v2, 1
  ret i32 %v3
}

; ASM-LABEL: test_01:
; ASM-NEXT: .Ltest_01$__0:
; ASM-NEXT:	addiu	$a0, $a0, 1
; ASM-NEXT:	andi	$a0, $a0, 1
; ASM-NEXT:	ori	$a0, $a0, 1
; ASM-NEXT:	xori	$a0, $a0, 1
; ASM-NEXT:	move	$v0, $a0
; ASM-NEXT:	jr	$ra

; DIS-LABEL:00000000 <test_01>:
; DIS-NEXT:   0:	24840001 	addiu	a0,a0,1
; DIS-NEXT:   4:	30840001 	andi	a0,a0,0x1
; DIS-NEXT:   8:	34840001 	ori	a0,a0,0x1
; DIS-NEXT:   c:	38840001 	xori	a0,a0,0x1
; DIS-NEXT:  10:	00801021 	move	v0,a0
; DIS-NEXT:  14:	03e00008 	jr	ra
; DIS-NEXT:  18:	00000000 	nop

; IASM-LABEL: test_01:
; IASM-LABEL: .Ltest_01$__0:
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x84
; IASM-NEXT:	.byte 0x24
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x84
; IASM-NEXT:	.byte 0x30
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x84
; IASM-NEXT:	.byte 0x34
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x84
; IASM-NEXT:	.byte 0x38
; IASM-NEXT:	.byte 0x21
; IASM-NEXT:	.byte 0x10
; IASM-NEXT:	.byte 0x80
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x8
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0xe0
; IASM-NEXT:	.byte 0x3
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0

define internal i32 @test_02(i32 %a) {
  %cmp = icmp eq i32 %a, 9
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: test_02:
; ASM-NEXT: .Ltest_02$__0:
; ASM-NEXT:	# $zero = def.pseudo
; ASM-NEXT:	addiu	$v0, $zero, 9
; ASM-NEXT:	xor	$a0, $a0, $v0
; ASM-NEXT:	sltiu	$a0, $a0, 1
; ASM-NEXT:	andi	$a0, $a0, 1
; ASM-NEXT:	move	$v0, $a0
; ASM-NEXT:	jr	$ra

; DIS-LABEL:00000020 <test_02>:
; DIS-NEXT:   20:	24020009 	li	v0,9
; DIS-NEXT:   24:	00822026 	xor	a0,a0,v0
; DIS-NEXT:   28:	2c840001 	sltiu	a0,a0,1
; DIS-NEXT:   2c:	30840001 	andi	a0,a0,0x1
; DIS-NEXT:   30:	00801021 	move	v0,a0
; DIS-NEXT:   34:	03e00008 	jr	ra
; DIS-NEXT:   38:	00000000 	nop

; IASM-LABEL: test_02:
; IASM-LABEL: .Ltest_02$__0:
; IASM-NEXT:	.byte 0x9
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x2
; IASM-NEXT:	.byte 0x24
; IASM-NEXT:	.byte 0x26
; IASM-NEXT:	.byte 0x20
; IASM-NEXT:	.byte 0x82
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x84
; IASM-NEXT:	.byte 0x2c
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x84
; IASM-NEXT:	.byte 0x30
; IASM-NEXT:	.byte 0x21
; IASM-NEXT:	.byte 0x10
; IASM-NEXT:	.byte 0x80
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x8
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0xe0
; IASM-NEXT:	.byte 0x3
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0

define internal i32 @ashrImm(i32 %val, i32 %shift) {
entry:
  %result = ashr i32 %val, %shift
  ret i32 %result
}

; ASM-LABEL: ashrImm:
; ASM-NEXT: .LashrImm$entry:
; ASM-NEXT: 	srav	$a0, $a0, $a1
; ASM-NEXT: 	move	$v0, $a0
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: <ashrImm>:
; DIS-NEXT:  00a42007 	srav	a0,a0,a1
; DIS-NEXT:  00801021 	move	v0,a0
; DIS-NEXT:  03e00008 	jr	ra

; IASM-LABEL: ashrImm:
; IASM-NEXT: .LashrImm$entry:
; IASM-NEXT:	.byte 0x7
; IASM-NEXT:	.byte 0x20
; IASM-NEXT:	.byte 0xa4
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x21
; IASM-NEXT:	.byte 0x10
; IASM-NEXT:	.byte 0x80
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x8
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0xe0
; IASM-NEXT:	.byte 0x3
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
