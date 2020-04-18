; Show that we know how to translate and.

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

define internal i32 @And1WithR0(i32 %p) {
  %v = and i32 %p, 1
  ret i32 %v
}

; ASM-LABEL:And1WithR0:
; ASM-NEXT:.LAnd1WithR0$__0:
; ASM-NEXT:     and     r0, r0, #1

; DIS-LABEL:00000000 <And1WithR0>:
; DIS-NEXT:   0:        e2000001

; IASM-LABEL:And1WithR0:
; IASM-NEXT:.LAnd1WithR0$__0:
; IASM-NEXT:     .byte 0x1
; IASM-NEXT:     .byte 0x0
; IASM-NEXT:     .byte 0x0
; IASM-NEXT:     .byte 0xe2

define internal i32 @And2Regs(i32 %p1, i32 %p2) {
  %v = and i32 %p1, %p2
  ret i32 %v
}

; ASM-LABEL:And2Regs:
; ASM-NEXT:.LAnd2Regs$__0:
; ASM-NEXT:     and     r0, r0, r1

; DIS-LABEL:00000010 <And2Regs>:
; DIS-NEXT:  10:        e0000001

; IASM-LABEL:And2Regs:
; IASM-NEXT:.LAnd2Regs$__0:
; IASM-NEXT:     .byte 0x1
; IASM-NEXT:     .byte 0x0
; IASM-NEXT:     .byte 0x0
; IASM-NEXT:     .byte 0xe0

define internal i64 @AndI64WithR0R1(i64 %p) {
  %v = and i64 %p, 1
  ret i64 %v
}

; ASM-LABEL:AndI64WithR0R1:
; ASM-NEXT:.LAndI64WithR0R1$__0:
; ASM-NEXT:     and     r0, r0, #1
; ASM-NEXT:     and     r1, r1, #0

; DIS-LABEL:00000020 <AndI64WithR0R1>:
; DIS-NEXT:  20:        e2000001
; DIS-NEXT:  24:        e2011000

; IASM-LABEL:AndI64WithR0R1:
; IASM-NEXT:.LAndI64WithR0R1$__0:
; IASM-NEXT:     .byte 0x1
; IASM-NEXT:     .byte 0x0
; IASM-NEXT:     .byte 0x0
; IASM-NEXT:     .byte 0xe2
; IASM-NEXT:     .byte 0x0
; IASM-NEXT:     .byte 0x10
; IASM-NEXT:     .byte 0x1
; IASM-NEXT:     .byte 0xe2

define internal i64 @AndI64Regs(i64 %p1, i64 %p2) {
  %v = and i64 %p1, %p2
  ret i64 %v
}

; ASM-LABEL:AndI64Regs:
; ASM-NEXT:.LAndI64Regs$__0:
; ASM-NEXT:     and     r0, r0, r2
; ASM-NEXT:     and     r1, r1, r3

; DIS-LABEL:00000030 <AndI64Regs>:
; DIS-NEXT:  30:        e0000002
; DIS-NEXT:  34:        e0011003

; IASM-LABEL:AndI64Regs:
; IASM-NEXT:.LAndI64Regs$__0:
; IASM-NEXT:     .byte 0x2
; IASM-NEXT:     .byte 0x0
; IASM-NEXT:     .byte 0x0
; IASM-NEXT:     .byte 0xe0
; IASM-NEXT:     .byte 0x3
; IASM-NEXT:     .byte 0x10
; IASM-NEXT:     .byte 0x1
; IASM-NEXT:     .byte 0xe0
