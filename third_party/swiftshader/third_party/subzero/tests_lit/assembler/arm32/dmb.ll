; Show that we know how to encode the dmb instruction.

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

declare i8 @llvm.nacl.atomic.load.i8(i8*, i32)

define internal i32 @test_atomic_load_8(i32 %iptr) {
; ASM-LABEL:test_atomic_load_8:
; DIS-LABEL:00000000 <test_atomic_load_8>:
; IASM-LABEL:test_atomic_load_8:

entry:
; ASM-NEXT:.Ltest_atomic_load_8$entry:
; IASM-NEXT:.Ltest_atomic_load_8$entry:

; ASM-NEXT:	sub	sp, sp, #12
; DIS-NEXT:   0:	e24dd00c
; IASM-NEXT:	.byte 0xc
; IASM-NEXT:	.byte 0xd0
; IASM-NEXT:	.byte 0x4d
; IASM-NEXT:	.byte 0xe2

; ASM-NEXT:	str	r0, [sp, #8]
; ASM-NEXT:	# [sp, #8] = def.pseudo
; DIS-NEXT:   4:	e58d0008
; IASM-NEXT:	.byte 0x8
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x8d
; IASM-NEXT:	.byte 0xe5

  %ptr = inttoptr i32 %iptr to i8*
  ; parameter value "6" is for the sequential consistency memory order.
  %i = call i8 @llvm.nacl.atomic.load.i8(i8* %ptr, i32 6)

; ASM-NEXT:	ldr	r0, [sp, #8]
; DIS-NEXT:   8:	e59d0008
; IASM-NEXT:	.byte 0x8
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x9d
; IASM-NEXT:	.byte 0xe5

; ASM-NEXT:	ldrb	r0, [r0]
; DIS-NEXT:   c:	e5d00000
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0xd0
; IASM-NEXT:	.byte 0xe5

; ASM-NEXT:	dmb	sy
; DIS-NEXT:  10:	f57ff05f
; IASM-NEXT:	.byte 0x5f
; IASM-NEXT:	.byte 0xf0
; IASM-NEXT:	.byte 0x7f
; IASM-NEXT:	.byte 0xf5

  %r = zext i8 %i to i32
  ret i32 %r
}
