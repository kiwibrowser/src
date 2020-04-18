; Show that we know how to translate vadd.

; NOTE: Restricts S and D registers to ones that will better test S/D
; register encodings.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   -reg-use s20,s22,d20,d22 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   -reg-use s20,s22,d20,d22 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   -reg-use s20,s22,d20,d22 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   -reg-use s20,s22,d20,d22 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal float @testVaddFloat(float %v1, float %v2) {
; ASM-LABEL: testVaddFloat:
; DIS-LABEL: 00000000 <testVaddFloat>:
; IASM-LABEL: testVaddFloat:

entry:
  %res = fadd float %v1, %v2

; ASM:     vadd.f32        s20, s20, s22
; DIS:   1c:       ee3aaa0b
; IASM-NOT:     vadd

  ret float %res
}

define internal double @testVaddDouble(double %v1, double %v2) {
; ASM-LABEL: testVaddDouble:
; DIS-LABEL: 00000040 <testVaddDouble>:
; IASM-LABEL: .LtestVaddDouble$entry:

entry:
  %res = fadd double %v1, %v2

; ASM:        vadd.f64        d20, d20, d22
; DIS:      54:       ee744ba6
; IASM-NOT:   vadd

  ret double %res
}
