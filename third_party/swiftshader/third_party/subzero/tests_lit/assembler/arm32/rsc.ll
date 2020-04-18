; Show that we know how to translate rsc

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

define internal i64 @NegateI64(i64 %a) {
; ASM-LABEL:NegateI64:
; DIS-LABEL:00000000 <NegateI64>:
; IASM-LABEL:NegateI64:

entry:
; ASM-NEXT:.LNegateI64$entry:
; IASM-NEXT:.LNegateI64$entry:

  %res = sub i64 0, %a

; ASM-NEXT:     rsbs    r0, r0, #0
; DIS-NEXT:   0:        e2700000
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x70
; IASM-NEXT:	.byte 0xe2

; ASM-NEXT:     rsc     r1, r1, #0
; DIS-NEXT:   4:        e2e11000
; IASM-NEXT:	.byte 0x0
; IASM-NEXT:	.byte 0x10
; IASM-NEXT:	.byte 0xe1
; IASM-NEXT:	.byte 0xe2

  ret i64 %res
}
