; Show that we know how to translate vector load instructions.

; Note: Uses -O2 to remove unnecessary loads/stores, resulting in only one VLD1
; instruction per function.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   -reg-use=q11,r5 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 \
; RUN:   -reg-use=q11,r5 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   -reg-use=q11,r5 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 \
; RUN:   -reg-use=q11,r5 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal <4 x float> @testDerefFloat4(<4 x float> *%p) {
; ASM-LABEL: testDerefFloat4:
; DIS-LABEL: {{.+}} <testDerefFloat4>:
; IASM-LABEL: testDerefFloat4:

entry:
  %ret = load <4 x float>, <4 x float>* %p, align 4
; ASM:   vld1.32        q11, [r5]
; DIS:   {{.*}}:        f4656a8f
; IASM-NOT:  vld1.32

  ret <4 x float> %ret
}

define internal <4 x i32> @testDeref4i32(<4 x i32> *%p) {
; ASM-LABEL: testDeref4i32:
; DIS-LABEL: {{.+}} <testDeref4i32>:
; IASM-LABEL: testDeref4i32:

entry:
  %ret = load <4 x i32>, <4 x i32>* %p, align 4
; ASM:   vld1.32        q11, [r5]
; DIS:   {{.+}}:        f4656a8f
; IASM-NOT:  vld1.32

  ret <4 x i32> %ret
}

define internal <8 x i16> @testDeref8i16(<8 x i16> *%p) {
; ASM-LABEL: testDeref8i16:
; DIS-LABEL: {{.+}} <testDeref8i16>:
; IASM-LABEL: testDeref8i16:

entry:
  %ret = load <8 x i16>, <8 x i16>* %p, align 2
; ASM:   vld1.16        q11, [r5]
; DIS:   {{.+}}:        f4656a4f
; IASM-NOT:  vld1.16

  ret <8 x i16> %ret
}

define internal <16 x i8> @testDeref16i8(<16 x i8> *%p) {
; ASM-LABEL: testDeref16i8:
; DIS-LABEL: {{.+}} <testDeref16i8>:
; IASM-LABEL: testDeref16i8:

entry:
  %ret = load <16 x i8>, <16 x i8>* %p, align 1
; ASM:   vld1.8         q11, [r5]
; DIS:   {{.+}}:        f4656a0f
; IASM-NOT:  vld1.8

  ret <16 x i8> %ret
}
