; Show that we know how to move between floating point registers.

; NOTE: We use the select instruction to fire this in -Om1, since a
; vmovne is generated (after a branch) to (conditionally) assign the
; else value.

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

define internal float @moveFloat() {
; ASM-LABEL: moveFloat:
; DIS-LABEL: 00000000 <moveFloat>:
; IASM-LABEL: moveFloat:

  %v = select i1 true, float 0.5, float 1.5

; ASM:  vmovne.f32      s20, s22
; DIS:   1c:    1eb0aa4b
; IASM-NOT: vmovnew.f32

  ret float %v
}

define internal double @moveDouble() {
; ASM-LABEL: moveDouble:
; DIS-LABEL: 00000040 <moveDouble>:
; IASM-LABEL: moveDouble:

  %v = select i1 true, double 0.5, double 1.5

; ASM:  vmovne.f64      d20, d22
; DIS:   54:    1ef04b66
; IASM-NOT: vmovne.f64

  ret double %v
}
