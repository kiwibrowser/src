; Show that we know how to translate rbit.

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

declare i32 @llvm.cttz.i32(i32, i1)

define internal i32 @testRbit(i32 %a) {
; ASM-LABEL: testRbit:
; DIS-LABEL: 00000000 <testRbit>:
; IASM-LABEL: testRbit:

entry:
; ASM-NEXT: .LtestRbit$entry:
; IASM-NEXT: .LtestRbit$entry:

  %x = call i32 @llvm.cttz.i32(i32 %a, i1 0)

; ASM-NEXT:     rbit    r0, r0
; DIS-NEXT:    0:       e6ff0f30
; IASM-NEXT:    .byte 0x30
; IASM-NEXT:    .byte 0xf
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0xe6

  ret i32 %x
}
