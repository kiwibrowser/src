; Show that we know how to translate udiv

; NOTE: We use -O2 to get rid of memory stores.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 -mattr=hwdiv-arm \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 -mattr=hwdiv-arm | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 -mattr=hwdiv-arm \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 -mattr=hwdiv-arm \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal i32 @UdivTwoRegs(i32 %a, i32 %b) {
  %v = udiv i32 %a, %b
  ret i32 %v
}

; ASM-LABEL:UdivTwoRegs:
; ASM-NEXT:.LUdivTwoRegs$__0:
; ASM-NEXT:     tst     r1, r1
; ASM-NEXT:     bne     .LUdivTwoRegs$local$__0
; ASM-NEXT:     .long 0xe7fedef0
; ASM-NEXT:.LUdivTwoRegs$local$__0:
; ASM-NEXT:     udiv    r0, r0, r1
; ASM-NEXT:     bx      lr

; DIS-LABEL:00000000 <UdivTwoRegs>:
; DIS-NEXT:   0:        e1110001
; DIS-NEXT:   4:        1a000000
; DIS-NEXT:   8:        e7fedef0
; DIS-NEXT:   c:        e730f110
; DIS-NEXT:  10:        e12fff1e

; IASM-LABEL:UdivTwoRegs:
; IASM-NEXT:.LUdivTwoRegs$__0:

; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x11
; IASM-NEXT:    .byte 0xe1

; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x1a

; IASM-NEXT:    .byte 0xf0
; IASM-NEXT:    .byte 0xde
; IASM-NEXT:    .byte 0xfe
; IASM-NEXT:    .byte 0xe7

; IASM-NEXT:.LUdivTwoRegs$local$__0:
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xf1
; IASM-NEXT:    .byte 0x30
; IASM-NEXT:    .byte 0xe7

; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1
