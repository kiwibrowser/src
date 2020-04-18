; Show that we know how to translate vadd vector instructions.

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

define internal <4 x float> @testVaddFloat4(<4 x float> %v1, <4 x float> %v2) {
; ASM-LABEL: testVaddFloat4:
; DIS-LABEL: 00000000 <testVaddFloat4>:
; IASM-LABEL: testVaddFloat4:

entry:
  %res = fadd <4 x float> %v1, %v2

; ASM:     vadd.f32        q10, q10, q11
; DIS:   8:       f2444de6
; IASM-NOT:     vadd.f32

  ret <4 x float> %res
}

define internal <4 x i32> @testVadd4i32(<4 x i32> %v1, <4 x i32> %v2) {
; ASM-LABEL: testVadd4i32:
; DIS-LABEL: 00000020 <testVadd4i32>:
; IASM-LABEL: testVadd4i32:

entry:
  %res = add <4 x i32> %v1, %v2

; ASM:     vadd.i32        q10, q10, q11
; DIS:   28:       f26448e6
; IASM-NOT:     vadd.i32

  ret <4 x i32> %res
}

define internal <8 x i16> @testVadd8i16(<8 x i16> %v1, <8 x i16> %v2) {
; ASM-LABEL: testVadd8i16:
; DIS-LABEL: 00000040 <testVadd8i16>:
; IASM-LABEL: testVadd8i16:

entry:
  %res = add <8 x i16> %v1, %v2

; ASM:     vadd.i16        q10, q10, q11
; DIS:   48:       f25448e6
; IASM-NOT:     vadd.i16

  ret <8 x i16> %res
}

define internal <16 x i8> @testVadd16i8(<16 x i8> %v1, <16 x i8> %v2) {
; ASM-LABEL: testVadd16i8:
; DIS-LABEL: 00000060 <testVadd16i8>:
; IASM-LABEL: testVadd16i8:

entry:
  %res = add <16 x i8> %v1, %v2

; ASM:     vadd.i8        q10, q10, q11
; DIS:   68:       f24448e6
; IASM-NOT:     vadd.i8

  ret <16 x i8> %res
}
