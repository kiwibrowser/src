; Show that we know how to translate sub.

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

define internal i32 @Sub1FromR0(i32 %p) {
  %v = sub i32 %p, 1
  ret i32 %v
}

; ASM-LABEL: Sub1FromR0:
; ASM-NEXT:  .LSub1FromR0$__0:
; ASM-NEXT:     sub     r0, r0, #1
; ASM-NEXT:     bx      lr

; DIS-LABEL:00000000 <Sub1FromR0>:
; DIS-NEXT:   0:        e2400001
; DIS-NEXT:   4:        e12fff1e

; IASM-LABEL: Sub1FromR0:
; IASM-LABEL: .LSub1FromR0$__0:
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x40
; IASM-NEXT:    .byte 0xe2

; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1

define internal i32 @Sub2Regs(i32 %p1, i32 %p2) {
  %v = sub i32 %p1, %p2
  ret i32 %v
}

; ASM-LABEL: Sub2Regs:
; ASM-NEXT:  .LSub2Regs$__0:
; ASM-NEXT:     sub r0, r0, r1
; ASM-NEXT:     bx lr

; DIS-LABEL:00000010 <Sub2Regs>:
; DIS-NEXT:  10:        e0400001
; DIS-NEXT:  14:        e12fff1e

; IASM-LABEL: Sub2Regs:
; IASM-NEXT:  .LSub2Regs$__0:
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x40
; IASM-NEXT:    .byte 0xe0

; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1

define internal i64 @SubI64FromR0R1(i64 %p) {
  %v = sub i64 %p, 1
  ret i64 %v
}

; ASM-LABEL:SubI64FromR0R1:
; ASM-NEXT:.LSubI64FromR0R1$__0:
; ASM-NEXT:     subs    r0, r0, #1
; ASM-NEXT:     sbc     r1, r1, #0

; DIS-LABEL:00000020 <SubI64FromR0R1>:
; DIS-NEXT:  20:        e2500001
; DIS-NEXT:  24:        e2c11000

; IASM-LABEL:SubI64FromR0R1:
; IASM-NEXT:.LSubI64FromR0R1$__0:
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x50
; IASM-NEXT:    .byte 0xe2
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xc1
; IASM-NEXT:    .byte 0xe2

define internal i64 @SubI64Regs(i64 %p1, i64 %p2) {
  %v = sub i64 %p1, %p2
  ret i64 %v
}

; ASM-LABEL:SubI64Regs:
; ASM-NEXT:.LSubI64Regs$__0:
; ASM-NEXT:     subs    r0, r0, r2
; ASM-NEXT:     sbc     r1, r1, r3

; DIS-LABEL:00000030 <SubI64Regs>:
; DIS-NEXT:  30:	e0500002
; DIS-NEXT:  34:	e0c11003

; IASM-LABEL:SubI64Regs:
; IASM-NEXT:.LSubI64Regs$__0:
; IASM-NEXT:    .byte 0x2
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x50
; IASM-NEXT:    .byte 0xe0
; IASM-NEXT:    .byte 0x3
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xc1
; IASM-NEXT:    .byte 0xe0
