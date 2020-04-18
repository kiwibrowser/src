; Show that we know how to translate mul.

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

define internal i32 @MulTwoRegs(i32 %a, i32 %b) {
  %v = mul i32 %a, %b
  ret i32 %v
}

; ASM-LABEL:MulTwoRegs:
; DIS-LABEL:<MulTwoRegs>:
; IASM-LABEL:MulTwoRegs:

; ASM:          mul     r0, r0, r1
; DIS:          e0000190
; IASM-NOT:     mul

define internal i64 @MulTwoI64Regs(i64 %a, i64 %b) {
  %v = mul i64 %a, %b
  ret i64 %v
}

; ASM-LABEL:MulTwoI64Regs:
; DIS-LABEL:<MulTwoI64Regs>:
; IASM-LABEL:MulTwoI64Regs:

; ASM:          mul     r3, r0, r3
; ASM-NEXT:     mla     r1, r2, r1, r3
; ASM-NEXT:     # r3 = def.pseudo
; ASM-NEXT:     umull   r0, r3, r0, r2
; ASM-NEXT:     # r3 = def.pseudo r0
; ASM-NEXT:     add     r3, r3, r1

; DIS:          e0030390
; DIS-NEXT:     e0213192
; DIS-NEXT:     e0830290
; DIS-NEXT:     e0833001

; IASM-NOT:     mul
; IASM-NOT:     mla
; IASM-NOT:     umull
; IASM-NOT:     add
