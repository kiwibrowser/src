; Show that we know how to translate vmul vector instructions.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   -reg-use q10,q11 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 \
; RUN:   -reg-use q10,q11 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   -reg-use q10,q11 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 \
; RUN:   -reg-use q10,q11 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal <4 x float> @testVmulFloat4(<4 x float> %v1, <4 x float> %v2) {
; ASM-LABEL: testVmulFloat4:
; DIS-LABEL: 00000000 <testVmulFloat4>:
; IASM-LABEL: testVmulFloat4:

entry:
  %res = fmul <4 x float> %v1, %v2

; ASM:     vmul.f32        q10, q10, q11
; DIS:   8:       f3444df6
; IASM-NOT:     vmul.f32

  ret <4 x float> %res
}

define internal <4 x i32> @testVmul4i32(<4 x i32> %v1, <4 x i32> %v2) {
; ASM-LABEL: testVmul4i32:
; DIS-LABEL: 00000020 <testVmul4i32>:
; IASM-LABEL: testVmul4i32:

entry:
  %res = mul <4 x i32> %v1, %v2

; ASM:     vmul.i32        q10, q10, q11
; DIS:   28:       f26449f6
; IASM-NOT:     vmul.i32

  ret <4 x i32> %res
}

define internal <8 x i16> @testVmul8i16(<8 x i16> %v1, <8 x i16> %v2) {
; ASM-LABEL: testVmul8i16:
; DIS-LABEL: 00000040 <testVmul8i16>:
; IASM-LABEL: testVmul8i16:

entry:
  %res = mul <8 x i16> %v1, %v2

; ASM:     vmul.i16        q10, q10, q11
; DIS:   48:       f25449f6
; IASM-NOT:     vmul.i16

  ret <8 x i16> %res
}

define internal <16 x i8> @testVmul16i8(<16 x i8> %v1, <16 x i8> %v2) {
; ASM-LABEL: testVmul16i8:
; DIS-LABEL: 00000060 <testVmul16i8>:
; IASM-LABEL: testVmul16i8:

entry:
  %res = mul <16 x i8> %v1, %v2

; ASM:     vmul.i8        q10, q10, q11
; DIS:   68:       f24449f6
; IASM-NOT:     vmul.i8

  ret <16 x i8> %res
}
