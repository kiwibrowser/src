; Show that we know how to translate lsr.

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

define internal i32 @LshrAmt(i32 %a) {
; ASM-LABEL:LshrAmt:
; DIS-LABEL:00000000 <LshrAmt>:
; IASM-LABEL:LshrAmt:

entry:
; ASM-NEXT:.LLshrAmt$entry:
; IASM-NEXT:.LLshrAmt$entry:

  %v = lshr i32 %a, 23

; ASM-NEXT:     lsr     r0, r0, #23
; DIS-NEXT:   0:        e1a00ba0
; IASM-NOT:     lsr

  ret i32 %v
}

define internal i32 @LshrReg(i32 %a, i32 %b) {
; ASM-LABEL:LshrReg:
; DIS-LABEL:00000010 <LshrReg>:
; IASM-LABEL:LshrReg:

entry:
; ASM-NEXT:.LLshrReg$entry:
; IASM-NEXT:.LLshrReg$entry:

  %v = lshr i32 %a, %b

; ASM-NEXT:     lsr     r0, r0, r1
; DIS-NEXT:  10:        e1a00130
; IASM-NOT:     lsr

  ret i32 %v
}

define internal <4 x i32> @LshrVec(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:LshrVec:
; DIS-LABEL:00000020 <LshrVec>:
; IASM-LABEL:LshrVec:

entry:
; ASM-NEXT:.LLshrVec$entry:
; IASM-NEXT:.LLshrVec$entry:

  %v = lshr <4 x i32> %a, %b

; ASM:          vneg.s32  q1, q1
; ASM-NEXT:     vshl.u32 q0, q0, q1
; DIS:      20:          f3b923c2
; DIS:      24:          f3220440
; IASM-NOT:     vneg
; IASM-NOT:     vshl

  ret <4 x i32> %v
}

define internal <8 x i16> @LshrVeci16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:LshrVeci16:

entry:

  %v = lshr <8 x i16> %a, %b

; ASM:          vneg.s16  q1, q1
; ASM-NEXT:     vshl.u16 q0, q0, q1
; DIS:      30:          f3b523c2
; DIS:      34:          f3120440
; IASM-NOT:     vneg
; IASM-NOT:     vshl

  ret <8 x i16> %v
}

define internal <16 x i8> @LshrVeci8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:LshrVeci8:

entry:

  %v = lshr <16 x i8> %a, %b

; ASM:          vneg.s8  q1, q1
; ASM-NEXT:     vshl.u8 q0, q0, q1
; DIS:      40:         f3b123c2
; DIS:      44:         f3020440
; IASM-NOT:     vneg
; IASM-NOT:     vshl

  ret <16 x i8> %v
}
