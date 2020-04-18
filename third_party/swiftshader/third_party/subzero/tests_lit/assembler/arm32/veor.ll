; Show that we know how to translate veor. Does this by noting that
; loading a double 0.0 introduces a veor.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal double @testVeor() {
; ASM-LABEL: testVeor:
; DIS: 00000000 <testVeor>:

entry:
; ASM: .LtestVeor$entry:

  ret double 0.0

; ASM:  veor.f64        d0, d0, d0
; DIS:    0:    f3000110
; IASM-NOT: veor

}
