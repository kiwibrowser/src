; Show that we know how to translate or.

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

define internal i32 @Or1WithR0(i32 %p) {
  %v = or i32 %p, 1
  ret i32 %v
}

; ASM-LABEL:Or1WithR0:
; ASM-NEXT:.LOr1WithR0$__0:
; ASM-NEXT:     orr     r0, r0, #1

; DIS-LABEL:00000000 <Or1WithR0>:
; DIS-NEXT:   0:        e3800001

; IASM-LABEL:Or1WithR0:
; IASM-NEXT:.LOr1WithR0$__0:
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x80
; IASM-NEXT:    .byte 0xe3

define internal i32 @Or2Regs(i32 %p1, i32 %p2) {
  %v = or i32 %p1, %p2
  ret i32 %v
}

; ASM-LABEL:Or2Regs:
; ASM-NEXT:.LOr2Regs$__0:
; ASM-NEXT:     orr     r0, r0, r1

; DIS-LABEL:00000010 <Or2Regs>:
; DIS-NEXT:  10:        e1800001

; IASM-LABEL:Or2Regs:
; IASM-NEXT:.LOr2Regs$__0:
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x80
; IASM-NEXT:    .byte 0xe1

define internal i64 @OrI64WithR0R1(i64 %p) {
  %v = or i64 %p, 1
  ret i64 %v
}

; ASM-LABEL:OrI64WithR0R1:
; ASM-NEXT:.LOrI64WithR0R1$__0:
; ASM-NEXT:     orr     r0, r0, #1
; ASM-NEXT:     orr     r1, r1, #0

; DIS-LABEL:00000020 <OrI64WithR0R1>:
; DIS-NEXT:  20:        e3800001
; DIS-NEXT:  24:        e3811000

; IASM-LABEL:OrI64WithR0R1:
; IASM-NEXT:.LOrI64WithR0R1$__0:
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x80
; IASM-NEXT:    .byte 0xe3
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x81
; IASM-NEXT:    .byte 0xe3


define internal i64 @OrI64Regs(i64 %p1, i64 %p2) {
  %v = or i64 %p1, %p2
  ret i64 %v
}

; ASM-LABEL:OrI64Regs:
; ASM-NEXT:.LOrI64Regs$__0:
; ASM-NEXT:     orr     r0, r0, r2
; ASM-NEXT:     orr     r1, r1, r3

; DIS-LABEL:00000030 <OrI64Regs>:
; DIS-NEXT:  30:        e1800002
; DIS-NEXT:  34:        e1811003

; IASM-LABEL:OrI64Regs:
; IASM-NEXT:.LOrI64Regs$__0:
; IASM-NEXT:    .byte 0x2
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x80
; IASM-NEXT:    .byte 0xe1
; IASM-NEXT:    .byte 0x3
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x81
; IASM-NEXT:    .byte 0xe1
