; Show that we know how to translate (floating point) vstr.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   -reg-use r5,r6,s20,d20 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 \
; RUN:   -reg-use r5,r6,s20,d20 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   -reg-use r5,r6,s20,d20 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 \
; RUN:   -reg-use r5,r6,s20,d20 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal void @testFloat() {
; ASM-LABEL: testFloat:
; DIS-LABEL: 00000000 <testFloat>:
; IASM-LABEL: testFloat:

entry:
; ASM: .LtestFloat$entry:

  %vaddr = inttoptr i32 0 to float*
  store float 0.0, float* %vaddr, align 1

; ASM:  vstr    s20, [r5]
; DIS:  14:     ed96aa00
; IASM-NOT: vstr

  ret void
}

define internal void @testDouble() {
; ASM-LABEL: testDouble:
; DIS-LABEL: 00000030 <testDouble>:
; IASM-LABEL: testDouble:

entry:
; ASM: .LtestDouble$entry:

  %vaddr = inttoptr i32 0 to double*
  store double 0.0, double* %vaddr, align 1

; ASM:  vstr    d20, [r5]
; DIS:  3c:     edc54b00
; IASM-NOT: vstr

  ret void
}
