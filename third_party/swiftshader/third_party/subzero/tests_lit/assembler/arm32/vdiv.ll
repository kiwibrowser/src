; Show that we know how to translate vdiv.

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

define internal float @testVdivFloat(float %v1, float %v2) {
; ASM-LABEL: testVdivFloat:
; DIS-LABEL: 00000000 <testVdivFloat>:
; IASM-LABEL: testVdivFloat:

entry:
; ASM-NEXT: .LtestVdivFloat$entry:
; IASM-NEXT: .LtestVdivFloat$entry:

  %res = fdiv float %v1, %v2

; ASM-NEXT:     vdiv.f32        s0, s0, s1
; DIS-NEXT:    0:       ee800a20
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0xa
; IASM-NEXT:    .byte 0x80
; IASM-NEXT:    .byte 0xee

  ret float %res
}

define internal double @testVdivDouble(double %v1, double %v2) {
; ASM-LABEL: testVdivDouble:
; DIS-LABEL: 00000010 <testVdivDouble>:
; IASM-LABEL: testVdivDouble:

entry:
; ASM-NEXT: .LtestVdivDouble$entry:
; IASM-NEXT: .LtestVdivDouble$entry:

  %res = fdiv double %v1, %v2

; ASM-NEXT:     vdiv.f64        d0, d0, d1
; DIS-NEXT:   10:       ee800b01
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0xb
; IASM-NEXT:    .byte 0x80
; IASM-NEXT:    .byte 0xee

  ret double %res
}
