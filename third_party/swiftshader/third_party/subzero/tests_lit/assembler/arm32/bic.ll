; Show that we know how to translate bic.

; NOTE: We use -O2 to get rid of memory stores.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

define internal i32 @AllocBigAlign() {
  %addr = alloca i8, align 32
  %v = ptrtoint i8* %addr to i32
  ret i32 %v
}

; ASM-LABEL:AllocBigAlign:
; ASM-NEXT:.LAllocBigAlign$__0:
; DIS-LABEL:00000000 <AllocBigAlign>:
; IASM-LABEL:AllocBigAlign:
; IASM-NEXT:.LAllocBigAlign$__0:

; ASM-NEXT:  push    {fp}
; DIS-NEXT:   0:        e52db004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0xb0
; IASM-NEXT:    .byte 0x2d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:  mov     fp, sp
; DIS-NEXT:   4:        e1a0b00d
; IASM:         .byte 0xd
; IASM-NEXT:    .byte 0xb0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:  sub     sp, sp, #32
; DIS-NEXT:   8:        e24dd020
; IASM:         .byte 0x20
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:  bic     sp, sp, #31
; DIS-NEXT:   c:        e3cdd01f
; IASM:         .byte 0x1f
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:  # sp = def.pseudo

; ASM-NEXT:  add     r0, sp, #0
; DIS-NEXT:  10:        e28d0000
; IASM:         .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:  mov     sp, fp
; DIS-NEXT:  14:        e1a0d00b
; IASM:         .byte 0xb
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:  pop     {fp}
; DIS-NEXT:  18:        e49db004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0xb0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe4

; ASM-NEXT:  # fp = def.pseudo

; ASM-NEXT:  bx      lr
; DIS-NEXT:  1c:        e12fff1e
; IASM:         .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1
