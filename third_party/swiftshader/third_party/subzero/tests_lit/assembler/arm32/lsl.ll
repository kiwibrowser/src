; Show that we know how to translate lsl.

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

define internal i32 @ShlAmt(i32 %a) {
; ASM-LABEL:ShlAmt:
; DIS-LABEL:00000000 <ShlAmt>:
; IASM-LABEL:ShlAmt:

entry:
; ASM-NEXT:.LShlAmt$entry:
; IASM-NEXT:.LShlAmt$entry:

  %shl = shl i32 %a, 23

; ASM-NEXT:     lsl     r0, r0, #23
; DIS-NEXT:   0:        e1a00b80
; IASM-NOT:     lsl

  ret i32 %shl
}

define internal i32 @ShlReg(i32 %a, i32 %b) {
; ASM-LABEL:ShlReg:
; DIS-LABEL:00000010 <ShlReg>:
; IASM-LABEL:ShlReg:

entry:
; ASM-NEXT:.LShlReg$entry:
; IASM-NEXT:.LShlReg$entry:

  %shl = shl i32 %a, %b

; ASM-NEXT:     lsl     r0, r0, r1
; DIS-NEXT:  10:        e1a00110
; IASM-NOT:     lsl

  ret i32 %shl
}

define internal <4 x i32> @ShlVec(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:ShlVec:
; DIS-LABEL:00000020 <ShlVec>:
; IASM-LABEL:ShlVec:

entry:
; ASM-NEXT:.LShlVec$entry:
; IASM-NEXT:.LShlVec$entry:

  %shl = shl <4 x i32> %a, %b

; ASM:      vshl.u32     q0, q0, q1
; DIS:  20: f3220440
; IASM-NOT: vshl

  ret <4 x i32> %shl
}

define internal <8 x i16> @ShlVeci16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:ShlVeci16:

entry:

  %v = shl <8 x i16> %a, %b

; ASM:      vshl.u16     q0, q0, q1
; DIS:  30: f3120440
; IASM-NOT: vshl

  ret <8 x i16> %v
}

define internal <16 x i8> @ShlVeci8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:ShlVeci8:

entry:

  %v = shl <16 x i8> %a, %b

; ASM:      vshl.u8     q0, q0, q1
; DIS:  40: f3020440
; IASM-NOT: vshl

  ret <16 x i8> %v
}
