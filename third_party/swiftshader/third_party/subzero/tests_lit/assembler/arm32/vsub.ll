; Show that we know how to translate vsub.

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

define internal float @testVsubFloat(float %v1, float %v2) {
; ASM-LABEL: testVsubFloat:
; DIS-LABEL: 00000000 <testVsubFloat>:
; IASM-LABEL: testVsubFloat:

entry:
; ASM-NEXT: .LtestVsubFloat$entry:
; IASM-NEXT: .LtestVsubFloat$entry:

  %res = fsub float %v1, %v2

; ASM-NEXT:     vsub.f32        s0, s0, s1
; DIS-NEXT:    0:       ee300a60
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xa
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0xee

  ret float %res
}

define internal double @testVsubDouble(double %v1, double %v2) {
; ASM-LABEL: testVsubDouble:
; DIS-LABEL: 00000010 <testVsubDouble>:
; IASM-LABEL: testVsubDouble:

entry:
; ASM-NEXT: .LtestVsubDouble$entry:
; IASM-NEXT: .LtestVsubDouble$entry:

  %res = fsub double %v1, %v2

; ASM-NEXT:     vsub.f64        d0, d0, d1
; DIS-NEXT:   10:       ee300b41
; IASM-NEXT: 	.byte 0x41
; IASM-NEXT: 	.byte 0xb
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0xee

  ret double %res
}
