; Show that we know how to translate rsb. Uses shl as example, because it
; uses rsb for type i64.

; Also shows an example of a register-shifted register (data) operation.

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

define internal i64 @shiftLeft(i64 %v, i64 %l) {
; ASM-LABEL:shiftLeft:
; DIS-LABEL:00000000 <shiftLeft>:
; IASM-LABEL:shiftLeft:

entry:
; ASM-NEXT:.LshiftLeft$entry:
; IASM-NEXT:.LshiftLeft$entry:

; ASM-NEXT:     sub     sp, sp, #24
; DIS-NEXT:   0:        e24dd018
; IASM-NEXT:    .byte 0x18
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     str     r0, [sp, #20]
; ASM-NEXT:     # [sp, #20] = def.pseudo
; DIS-NEXT:   4:        e58d0014
; IASM-NEXT:    .byte 0x14
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     mov     r0, r1
; DIS-NEXT:   8:        e1a00001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:     str     r0, [sp, #16]
; ASM-NEXT:     # [sp, #16] = def.pseudo
; DIS-NEXT:   c:        e58d0010
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     mov     r0, r2
; DIS-NEXT:  10:        e1a00002
; IASM-NEXT:    .byte 0x2
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:     str     r0, [sp, #12]
; ASM-NEXT:     # [sp, #12] = def.pseudo
; DIS-NEXT:  14:        e58d000c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     mov     r0, r3
; DIS-NEXT:  18:        e1a00003
; IASM-NEXT:    .byte 0x3
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:     str     r0, [sp, #8]
; ASM-NEXT:     # [sp, #8] = def.pseudo
; DIS-NEXT:  1c:        e58d0008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %result = shl i64 %v, %l

; ASM-NEXT:     ldr     r0, [sp, #20]
; DIS-NEXT:  20:        e59d0014
; IASM-NEXT:    .byte 0x14
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r1, [sp, #16]
; DIS-NEXT:  24:        e59d1010
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r2, [sp, #12]
; DIS-NEXT:  28:        e59d200c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ****** Here is the example of rsb *****
; ASM-NEXT:     rsb     r3, r2, #32
; DIS-NEXT:  2c:        e2623020
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0x30
; IASM-NEXT:    .byte 0x62
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:    lsr     r3, r0, r3
; DIS-NEXT:  30:        e1a03330
; IASM-NEXT:    .byte 0x30
; IASM-NEXT:    .byte 0x33
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

; ***** Here is an example of a register-shifted register *****
; ASM-NEXT:    orr     r1, r3, r1, lsl r2
; DIS-NEXT:  34:        e1831211
; IASM-NEXT:    .byte 0x11
; IASM-NEXT:    .byte 0x12
; IASM-NEXT:    .byte 0x83
; IASM-NEXT:    .byte 0xe1

  ret i64 %result
}
