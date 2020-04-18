; Show that we can handle variable (i.e. stack) spills.

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

define internal i32 @add1ToR0(i32 %p) {
  %v = add i32 %p, 1
  ret i32 %v
}

; ASM-LABEL: add1ToR0:
; IASM-LABEL: add1ToR0:
; DIS-LABEL:00000000 <add1ToR0>:

; ASM:          sub     sp, sp, #8
; DIS-NEXT:   0:        e24dd008
; IASM:         .byte 0x8
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     str     r0, [sp, #4]
; ASM-NEXT:     # [sp, #4] = def.pseudo
; DIS-NEXT:   4:        e58d0004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r0, [sp, #4]
; DIS-NEXT:   8:        e59d0004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     add     r0, r0, #1
; DIS-NEXT:   c:        e2800001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x80
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     str     r0, [sp]
; ASM-NEXT:     # [sp] = def.pseudo
; DIS-NEXT:  10:        e58d0000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r0, [sp]
; DIS-NEXT:  14:        e59d0000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     add     sp, sp, #8
; DIS-NEXT:  18:        e28dd008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     bx      lr
; DIS-NEXT:  1c:        e12fff1e
; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1
