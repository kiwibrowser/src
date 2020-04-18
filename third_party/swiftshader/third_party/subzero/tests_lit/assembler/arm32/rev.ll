; Show that we know how to translate rev (used in bswap).

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

declare i16 @llvm.bswap.i16(i16)

define internal i32 @testRev(i32 %a) {
; ASM-LABEL:testRev:
; DIS-LABEL:00000000 <testRev>:
; IASM-LABEL:testRev:

entry:
; ASM-NEXT:.LtestRev$entry:
; IASM-NEXT:.LtestRev$entry:

  %a.arg_trunc = trunc i32 %a to i16
  %v = tail call i16 @llvm.bswap.i16(i16 %a.arg_trunc)

; ***** Example of rev instruction. *****
; ASM-NEXT:     rev     r0, r0
; DIS-NEXT:   0:        e6bf0f30
; IASM-NEXT:    .byte 0x30
; IASM-NEXT:    .byte 0xf
; IASM-NEXT:    .byte 0xbf
; IASM-NEXT:    .byte 0xe6

; ASM-NEXT:     lsr     r0, r0, #16

  %.ret_ext = zext i16 %v to i32
  ret i32 %.ret_ext
}
