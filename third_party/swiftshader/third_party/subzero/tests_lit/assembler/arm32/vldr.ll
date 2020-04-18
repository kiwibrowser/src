; Show that we know how to translate (floating point) vldr.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   -reg-use r5,s20,d20 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 \
; RUN:   -reg-use r5,s20,d20 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   -reg-use r5,s20,d20 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 \
; RUN:   -reg-use r5,s20,d20 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal float @testFloat() {
; ASM-LABEL: testFloat:
; DIS-LABEL: 00000000 <testFloat>:

entry:
; ASM: .LtestFloat$entry:

  %vaddr = inttoptr i32 0 to float*
  %v = load float, float* %vaddr, align 1

; ASM:  vldr    s20, [r5]
; DIS:   c:    ed95aa00
; IASM-NOT: vldr

  ret float %v
}

define internal double @testDouble() {
; ASM-LABEL: testDouble:
; DIS-LABEL: 00000020 <testDouble>:

entry:
; ASM: .LtestDouble$entry:

;  %vaddr = bitcast [8 x i8]* @doubleVal to double*
  %vaddr = inttoptr i32 0 to double*
  %v = load double, double* %vaddr, align 1

; ASM:  vldr    d20, [r5]
; DIS:   28:    edd54b00
; IASM-NOT: vldr

  ret double %v
}
