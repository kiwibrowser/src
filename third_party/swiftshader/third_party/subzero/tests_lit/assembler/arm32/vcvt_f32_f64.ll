; Show that we know how to translate vcvt between float and double.

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

define internal double @testVcvtFloatToDouble(float %v) {
; ASM-LABEL: testVcvtFloatToDouble:
; DIS-LABEL: 00000000 <testVcvtFloatToDouble>:
; IASM-LABEL: testVcvtFloatToDouble:

entry:
; ASM-NEXT: .LtestVcvtFloatToDouble$entry:
; IASM-NEXT: .LtestVcvtFloatToDouble$entry:

  %res = fpext float %v to double

; ASM-NEXT:    vcvt.f64.f32    d0, s0
; DIS-NEXT:    0:	eeb70ac0
; IASM-NEXT: 	.byte 0xc0
; IASM-NEXT: 	.byte 0xa
; IASM-NEXT: 	.byte 0xb7
; IASM-NEXT: 	.byte 0xee

  ret double %res
}

define internal float @testVcvtDoubleToFloat(double %v) {
; ASM-LABEL: testVcvtDoubleToFloat:
; DIS-LABEL: 00000010 <testVcvtDoubleToFloat>:
; IASM-LABEL: testVcvtDoubleToFloat:

entry:
; ASM-NEXT: .LtestVcvtDoubleToFloat$entry:
; IASM-NEXT: .LtestVcvtDoubleToFloat$entry:

  %res = fptrunc double %v to float
; ASM-NEXT:    vcvt.f32.f64    s0, d0
; DIS-NEXT:   10:	eeb70bc0
; IASM-NEXT: 	.byte 0xc0
; IASM-NEXT: 	.byte 0xb
; IASM-NEXT: 	.byte 0xb7
; IASM-NEXT: 	.byte 0xee

  ret float %res
}
