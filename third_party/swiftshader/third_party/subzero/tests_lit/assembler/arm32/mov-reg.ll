; Show that we know how to translate mov (shifted register), which
; are pseudo instructions for ASR, LSR, ROR, and RRX.

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

define internal i64 @testMovWithAsr(i32 %a) {
; ASM-LABEL:testMovWithAsr:
; DIS-LABEL:00000000 <testMovWithAsr>:
; IASM-LABEL:testMovWithAsr:

entry:
; ASM-NEXT:.LtestMovWithAsr$entry:
; IASM-NEXT:.LtestMovWithAsr$entry:

  %a.arg_trunc = trunc i32 %a to i8
  %conv = sext i8 %a.arg_trunc to i64
  ret i64 %conv

; ASM-NEXT:     sxtb    r0, r0
; DIS-NEXT:   0:        e6af0070
; IASM-NEXT:    .byte 0x70
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xaf
; IASM-NEXT:    .byte 0xe6

; ***** Example of mov pseudo instruction.
; ASM-NEXT:     mov     r1, r0, asr #31
; DIS-NEXT:   4:        e1a01fc0
; IASM-NEXT:    .byte 0xc0
; IASM-NEXT:    .byte 0x1f
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:     bx      lr

}
