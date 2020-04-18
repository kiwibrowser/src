; Shows that the ARM integrated assembler can translate a trivial,
; bundle-aligned function.

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

define internal void @f() {
  ret void
}

; ASM-LABEL:f:
; ASM-NEXT: .Lf$__0:
; ASM-NEXT:     bx      lr

; DIS-LABEL:00000000 <f>:
; IASM-LABEL:f:
; IASM-NEXT:.Lf$__0:

; DIS-NEXT:   0:        e12fff1e
; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1

; DIS-NEXT:   4:        e7fedef0
; IASM-NEXT:    .byte 0xf0
; IASM-NEXT:    .byte 0xde
; IASM-NEXT:    .byte 0xfe
; IASM-NEXT:    .byte 0xe7

; DIS-NEXT:   8:        e7fedef0
; IASM-NEXT:    .byte 0xf0
; IASM-NEXT:    .byte 0xde
; IASM-NEXT:    .byte 0xfe
; IASM-NEXT:    .byte 0xe7

; DIS-NEXT:   c:        e7fedef0
; IASM-NEXT:    .byte 0xf0
; IASM-NEXT:    .byte 0xde
; IASM-NEXT:    .byte 0xfe
; IASM-NEXT:    .byte 0xe7

define internal void @ignore() {
  ret void
}

; ASM-LABEL:ignore:
; DIS-LABEL:00000010 <ignore>:
; IASM-LABEL:ignore:
