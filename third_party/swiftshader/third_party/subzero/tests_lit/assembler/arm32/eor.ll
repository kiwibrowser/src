; Show that we know how to translate eor.

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

define internal i32 @Eor1WithR0(i32 %p) {
  %v = xor i32 %p, 1
  ret i32 %v
}

; ASM-LABEL:Eor1WithR0:
; ASM-NEXT:.LEor1WithR0$__0:
; ASM-NEXT:     eor     r0, r0, #1

; DIS-LABEL:00000000 <Eor1WithR0>:
; DIS-NEXT:   0:        e2200001

; IASM-LABEL:Eor1WithR0:
; IASM-NEXT:.LEor1WithR0$__0:
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x20
; IASM-NEXT:	.byte 0xe2

define internal i32 @Eor2Regs(i32 %p1, i32 %p2) {
  %v = xor i32 %p1, %p2
  ret i32 %v
}

; ASM-LABEL:Eor2Regs:
; ASM-NEXT:.LEor2Regs$__0:
; ASM-NEXT:     eor     r0, r0, r1

; DIS-LABEL:00000010 <Eor2Regs>:
; DIS-NEXT:  10:        e0200001

; IASM-LABEL:Eor2Regs:
; IASM-NEXT:.LEor2Regs$__0:
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x20
; IASM-NEXT:	.byte 0xe0

define internal i64 @EorI64WithR0R1(i64 %p) {
  %v = xor i64 %p, 1
  ret i64 %v
}

; ASM-LABEL:EorI64WithR0R1:
; ASM-NEXT:.LEorI64WithR0R1$__0:
; ASM-NEXT:     eor     r0, r0, #1
; ASM-NEXT:     eor     r1, r1, #0

; DIS-LABEL:00000020 <EorI64WithR0R1>:
; DIS-NEXT:  20:        e2200001
; DIS-NEXT:  24:        e2211000

; IASM-LABEL:EorI64WithR0R1:
; IASM-NEXT:.LEorI64WithR0R1$__0:
; IASM-NEXT:	.byte 0x1
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x20
; IASM-NEXT:	.byte 0xe2
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x10
; IASM-NEXT:	.byte 0x21
; IASM-NEXT:	.byte 0xe2

define internal i64 @EorI64Regs(i64 %p1, i64 %p2) {
  %v = xor i64 %p1, %p2
  ret i64 %v
}

; ASM-LABEL:EorI64Regs:
; ASM-NEXT:.LEorI64Regs$__0:
; ASM-NEXT:     eor     r0, r0, r2
; ASM-NEXT:     eor     r1, r1, r3

; DIS-LABEL:00000030 <EorI64Regs>:
; DIS-NEXT:  30:        e0200002
; DIS-NEXT:  34:        e0211003

; IASM-LABEL:EorI64Regs:
; IASM-NEXT:.LEorI64Regs$__0:
; IASM-NEXT:	.byte 0x2
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x20
; IASM-NEXT:	.byte 0xe0
; IASM-NEXT:	.byte 0x3
; IASM-NEXT:	.byte 0x10
; IASM-NEXT:	.byte 0x21
; IASM-NEXT:	.byte 0xe0
