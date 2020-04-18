; Sample program that generates "str reg, [fp, #CCCC]", to show that we
; recognize that "fp" should be used instead of "sp".

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 | FileCheck %s --check-prefix=DIS

define internal void @test_vla_in_loop(i32 %n) {
; ASM-LABEL: test_vla_in_loop:
; DIS-LABEL: 00000000 <test_vla_in_loop>:
; IASM-LABEL: test_vla_in_loop:

entry:

; ASM-NEXT: .Ltest_vla_in_loop$entry:
; IASM-NEXT: .Ltest_vla_in_loop$entry:

; ASM-NEXT: 	push	{fp}
; DIS-NEXT:    0:	e52db004
; IASM-NEXT: 	.byte 0x4
; IASM-NEXT: 	.byte 0xb0
; IASM-NEXT: 	.byte 0x2d
; IASM-NEXT: 	.byte 0xe5

; ASM-NEXT: 	mov	fp, sp
; DIS-NEXT:    4:	e1a0b00d
; IASM-NEXT: 	.byte 0xd
; IASM-NEXT: 	.byte 0xb0
; IASM-NEXT: 	.byte 0xa0
; IASM-NEXT: 	.byte 0xe1

; ASM-NEXT: 	sub	sp, sp, #12
; DIS-NEXT:    8:	e24dd00c
; IASM-NEXT: 	.byte 0xc
; IASM-NEXT: 	.byte 0xd0
; IASM-NEXT: 	.byte 0x4d
; IASM-NEXT: 	.byte 0xe2

; **** Example of fixed instruction.
; ASM-NEXT: 	str	r0, [fp, #-4]
; DIS-NEXT:    c:	e50b0004
; IASM-NEXT: 	.byte 0x4
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xb
; IASM-NEXT: 	.byte 0xe5
; ASM-NEXT:     # [fp, #-4] = def.pseudo
  br label %next

; ASM-NEXT: 	b	.Ltest_vla_in_loop$next
; DIS-NEXT:   10:	eaffffff

; IASM-NEXT: 	.byte 0xff
; IASM-NEXT: 	.byte 0xff
; IASM-NEXT: 	.byte 0xff
; IASM-NEXT: 	.byte 0xea

; Put the variable-length alloca in a non-entry block, to reduce the
; chance the optimizer putting it before the regular frame creation.

next:
  %v = alloca i8, i32 %n, align 4
  ret void
}
