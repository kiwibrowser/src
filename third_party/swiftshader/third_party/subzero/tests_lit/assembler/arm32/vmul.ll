; Show that we know how to translate vmul.

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

define internal float @testVmulFloat(float %a, float %b) {
; ASM-LABEL: testVmulFloat:
; DIS-LABEL: 00000000 <testVmulFloat>:
; IASM-LABEL: testVmulFloat:

entry:
; ASM-NEXT: .LtestVmulFloat$entry:
; IASM-NEXT: .LtestVmulFloat$entry:

  %mul = fmul float %a, %b

; ASM-NEXT:    vmul.f32        s0, s0, s1
; DIS-NEXT:    0:       ee200a20
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0xa
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0xee

  ret float %mul
}

define internal double @testVmulDouble(double %a, double %b) {
; ASM-LABEL: testVmulDouble:
; DIS-LABEL: 00000010 <testVmulDouble>:
; IASM-LABEL: testVmulDouble:

entry:
; ASM-NEXT: .LtestVmulDouble$entry:
; IASM-NEXT: .LtestVmulDouble$entry:

  %mul = fmul double %a, %b

; ASM-NEXT:    vmul.f64        d0, d0, d1
; DIS-NEXT:   10:       ee200b01
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0xb
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0xee

  ret double %mul
}
