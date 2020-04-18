; Show that we know how to translate clz.

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

declare i32 @llvm.ctlz.i32(i32, i1)

define internal i32 @testClz(i32 %a) {
; ASM-LABEL:testClz:
; DIS-LABEL:00000000 <testClz>:
; IASM-LABEL:testClz:

entry:
; ASM-NEXT:.LtestClz$entry:
; IASM-NEXT:.LtestClz$entry:

; ASM-NEXT:     sub     sp, sp, #8
; DIS-NEXT:   0:        e24dd008
; IASM-NEXT:	.byte 0x8
; IASM-NEXT:	.byte 0xd0
; IASM-NEXT:	.byte 0x4d
; IASM-NEXT:	.byte 0xe2

; ASM-NEXT:     str     r0, [sp, #4]
; ASM-NEXT:     # [sp, #4] = def.pseudo
; DIS-NEXT:   4:        e58d0004
; IASM-NEXT:	.byte 0x4
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x8d
; IASM-NEXT:	.byte 0xe5

  %x = call i32 @llvm.ctlz.i32(i32 %a, i1 0)

; ASM-NEXT:     ldr     r0, [sp, #4]
; DIS-NEXT:   8:        e59d0004
; IASM-NEXT:	.byte 0x4
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x9d
; IASM-NEXT:	.byte 0xe5

; ASM-NEXT:     clz     r0, r0
; DIS-NEXT:   c:        e16f0f10
; IASM-NEXT:	.byte 0x10
; IASM-NEXT:	.byte 0xf
; IASM-NEXT:	.byte 0x6f
; IASM-NEXT:	.byte 0xe1

  ret i32 %x
}
