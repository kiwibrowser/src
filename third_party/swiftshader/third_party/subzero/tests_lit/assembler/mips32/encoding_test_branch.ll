; Test encoding of MIPS32 branch instructions

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

define internal void @test_01(i32 %a) {
  %cmp = icmp eq i32 %a, 1
  br i1 %cmp, label %then, label %else
then:
  br label %end
else:
  br label %end
end:
  ret void
}

; ASM-LABEL: test_01:
; ASM-LABEL: .Ltest_01$__0:
; ASM-NEXT:	# $zero = def.pseudo
; ASM-NEXT:	addiu	$v0, $zero, 1
; ASM-NEXT:	bne	$a0, $v0, .Ltest_01$end
; ASM-LABEL: .Ltest_01$end:
; ASM-NEXT:	jr	$ra
; ASM-LABEL: .Ltest_01$then:
; ASM-LABEL: .Ltest_01$else:

; DIS-LABEL:00000000 <test_01>:
; DIS-NEXT:   0:   24020001	li	v0,1
; DIS-NEXT:   4:   14820001	bne	a0,v0,c <.Ltest_01$end>
; DIS-NEXT:   8:   00000000	nop
; DIS-LABEL:0000000c <.Ltest_01$end>:
; DIS-NEXT:   c:   03e00008	jr	ra
; DIS-NEXT:  10:   00000000	nop

; IASM-LABEL: test_01:
; IASM-LABEL: .Ltest_01$__0:
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x2
; IASM-NEXT:	.byte 0x24
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x82
; IASM-NEXT:	.byte 0x14
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-LABEL: .Ltest_01$end:
; IASM-NEXT:	.byte 0x8
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0xe0
; IASM-NEXT:	.byte 0x3
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-LABEL: .Ltest_01$then:
; IASM-LABEL: .Ltest_01$else:

define internal void @test_02(i32 %a) {
  %cmp = icmp ne i32 %a, 2
  br i1 %cmp, label %then, label %else
then:
  br label %end
else:
  br label %end
end:
  ret void
}

; ASM-LABEL: test_02:
; ASM-LABEL: .Ltest_02$__0:
; ASM-NEXT:	# $zero = def.pseudo
; ASM-NEXT:	addiu	$v0, $zero, 2
; ASM-NEXT:	beq	$a0, $v0, .Ltest_02$end
; ASM-LABEL: .Ltest_02$end:
; ASM-NEXT:	jr	$ra
; ASM-LABEL: .Ltest_02$then:
; ASM-LABEL: .Ltest_02$else:

; DIS-LABEL:00000020 <test_02>:
; DIS-NEXT:  20:   24020002	li	v0,2
; DIS-NEXT:  24:   10820001	beq	a0,v0,2c <.Ltest_02$end>
; DIS-NEXT:  28:   00000000	nop
; DIS-LABEL:0000002c <.Ltest_02$end>:
; DIS-NEXT:  2c:   03e00008	jr	ra
; DIS-NEXT:  30:   00000000	nop

; IASM-LABEL: test_02:
; IASM-LABEL: .Ltest_02$__0:
; IASM-NEXT:	.byte 0x2
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x2
; IASM-NEXT:	.byte 0x24
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x82
; IASM-NEXT:	.byte 0x10
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-LABEL: .Ltest_02$end:
; IASM-NEXT:	.byte 0x8
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0xe0
; IASM-NEXT:	.byte 0x3
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-LABEL: .Ltest_02$then:
; IASM-LABEL: .Ltest_02$else:

define internal void @test_03(i32 %a) {
  %cmp = icmp ugt i32 %a, 3
  br i1 %cmp, label %then, label %else
then:
  br label %end
else:
  br label %end
end:
  ret void
}

; ASM-LABEL: test_03:
; ASM-LABEL: .Ltest_03$__0:
; ASM-NEXT:	# $zero = def.pseudo
; ASM-NEXT:	addiu	$v0, $zero, 3
; ASM-NEXT:	sltu	$v0, $v0, $a0
; ASM-NEXT:	beqz	$v0, .Ltest_03$end
; ASM-LABEL: .Ltest_03$end:
; ASM-NEXT:	jr	$ra
; ASM-LABEL: .Ltest_03$then:
; ASM-LABEL: .Ltest_03$else:

; DIS-LABEL:00000040 <test_03>:
; DIS-NEXT:  40:   24020003	li	v0,3
; DIS-NEXT:  44:   0044102b	sltu	v0,v0,a0
; DIS-NEXT:  48:   10400001	beqz	v0,50 <.Ltest_03$end>
; DIS-NEXT:  4c:   00000000	nop
; DIS-LABEL:00000050 <.Ltest_03$end>:
; DIS-NEXT:  50:   03e00008	jr	ra
; DIS-NEXT:  54:   00000000	nop

; IASM-LABEL: test_03:
; IASM-LABEL: .Ltest_03$__0:
; IASM-NEXT:	.byte 0x3
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x2
; IASM-NEXT:	.byte 0x24
; IASM-NEXT:	.byte 0x2b
; IASM-NEXT:	.byte 0x10
; IASM-NEXT:	.byte 0x44
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x40
; IASM-NEXT:	.byte 0x10
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-LABEL: .Ltest_03$end:
; IASM-NEXT:	.byte 0x8
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0xe0
; IASM-NEXT:	.byte 0x3
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-LABEL: .Ltest_03$then:
; IASM-LABEL: .Ltest_03$else:

define internal void @test_04(i32 %a) {
  %cmp = icmp uge i32 %a, 4
  br i1 %cmp, label %then, label %else
then:
  br label %end
else:
  br label %end
end:
  ret void
}

; ASM-LABEL: test_04:
; ASM-LABEL: .Ltest_04$__0:
; ASM-NEXT:	# $zero = def.pseudo
; ASM-NEXT:	addiu	$v0, $zero, 4
; ASM-NEXT:	sltu	$a0, $a0, $v0
; ASM-NEXT:	bnez	$a0, .Ltest_04$end
; ASM-LABEL: .Ltest_04$end:
; ASM-NEXT:	jr	$ra
; ASM-LABEL: .Ltest_04$then:
; ASM-LABEL: .Ltest_04$else:

; DIS-LABEL:00000060 <test_04>:
; DIS-NEXT:  60:   24020004	li	v0,4
; DIS-NEXT:  64:   0082202b	sltu	a0,a0,v0
; DIS-NEXT:  68:   14800001	bnez	a0,70 <.Ltest_04$end>
; DIS-NEXT:  6c:   00000000	nop
; DIS-LABEL:00000070 <.Ltest_04$end>:
; DIS-NEXT:  70:   03e00008	jr	ra
; DIS-NEXT:  74:   00000000	nop

; IASM-LABEL: test_04:
; IASM-LABEL: .Ltest_04$__0:
; IASM-NEXT:	.byte 0x4
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x2
; IASM-NEXT:	.byte 0x24
; IASM-NEXT:	.byte 0x2b
; IASM-NEXT:	.byte 0x20
; IASM-NEXT:	.byte 0x82
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x80
; IASM-NEXT:	.byte 0x14
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-LABEL: .Ltest_04$end:
; IASM-NEXT:	.byte 0x8
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0xe0
; IASM-NEXT:	.byte 0x3
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-LABEL: .Ltest_04$then:
; IASM-LABEL: .Ltest_04$else:
