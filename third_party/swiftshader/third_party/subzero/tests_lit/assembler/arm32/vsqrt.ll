; Show that we can translate intrinsic vsqrt into a binary instruction.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 -allow-extern \
; RUN:   -reg-use s20,d20 | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 -allow-extern -reg-use s20,d20 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 -allow-extern \
; RUN:   -reg-use s20,d20 | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 -allow-extern -reg-use s20,d20 \
; RUN:   | FileCheck %s --check-prefix=DIS

declare float @llvm.sqrt.f32(float)
declare double @llvm.sqrt.f64(double)

define internal float @sqrtFloat() {
; ASM-LABEL: sqrtFloat:
; DIS-LABEL: 00000000 <sqrtFloat>:
; IASM-LABEL: sqrtFloat:

  %v = call float @llvm.sqrt.f32(float 0.5);

; ASM:  vsqrt.f32       s20, s20
; DIS:    c:    eeb1aaca
; IASM-NOT: vsqrt.f32

  ret float %v
}

define internal double @sqrtDouble() {
; ASM-LABEL: sqrtDouble:
; DIS-LABEL: 00000030 <sqrtDouble>:
; IASM-LABEL: sqrtDouble:

  %v = call double @llvm.sqrt.f64(double 0.5);

; ASM:  vsqrt.f64       d20, d20
; DIS:   38:    eef14be4
; IASM-NOT: vsqrt.f64

  ret double %v
}
