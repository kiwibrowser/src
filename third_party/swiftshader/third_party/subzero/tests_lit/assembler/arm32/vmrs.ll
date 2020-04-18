; Test the "vmrs APSR_nzcv, FPSCR" form of the VMRS instruction.

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

define internal i32 @testVmrsASPR_nzcv() {
; ASM-LABEL: testVmrsASPR_nzcv:
; DIS-LABEL: 00000000 <testVmrsASPR_nzcv>:

entry:
; ASM: .LtestVmrsASPR_nzcv$entry:

  %test = fcmp olt float 0.0, 0.0

; ASM:  vmrs    APSR_nzcv, FPSCR
; DIS:   14:    eef1fa10
; IASM-NOT: vmrs

  %result = zext i1 %test to i32
  ret i32 %result
}
