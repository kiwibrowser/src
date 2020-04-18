; Test moving constants into VPF registers.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 -reg-use=d21,s20 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 -reg-use=d21,s20 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 -reg-use=d21,s20 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 -reg-use=d21,s20 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal void @testMoveDouble() {
; ASM-LABEL: testMoveDouble:
; DIS-LABEL: 00000000 <testMoveDouble>:

entry:
  store double 1.5, double* undef, align 8

; ASM:  vmov.f64        d21, #1.500000e+00
; DIS:    4:    eef75b08
; IASM-NOT: vmov.f64

  ret void
}

define internal void @testMoveFloat() {
; ASM-LABEL: testMoveFloat:
; DIS-LABEL: 00000010 <testMoveFloat>:

entry:
  %addr = inttoptr i32 0 to float*
  store float 1.5, float* %addr, align 4

; ASM:  vmov.f32        s20, #1.500000e+00
; DIS:   18:    eeb7aa08
; IASM-NOT: vmov.f32

  ret void
}
