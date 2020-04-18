; Show that we know how to translate vector store instructions.

; Note: Uses -O2 to remove unnecessary loads/stores, resulting in only one VST1
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

define internal void @testDerefFloat4(<4 x float>* %p, <4 x float> %v) {
; ASM-LABEL: testDerefFloat4:
; DIS-LABEL: {{.+}} <testDerefFloat4>:

entry:
  store <4 x float> %v, <4 x float>* %p, align 4
; ASM:    vst1.32   q11, [r5]
; DIS:    {{.+}}:   f4456a8f
; IASM-NOT:   vst1.32

  ret void
}

define internal void @testDeref4i32(<4 x i32> *%p, <4 x i32> %v) {
; ASM-LABEL: testDeref4i32:
; DIS-LABEL: {{.+}} <testDeref4i32>:

entry:
  store <4 x i32> %v, <4 x i32>* %p, align 4
; ASM:   vst1.32  q11, [r5]
; DIS:   {{.+}}:  f4456a8f
; IASM-NOT:   vst1.32

  ret void
}

define internal void @testDeref8i16(<8 x i16> *%p, <8 x i16> %v) {
; ASM-LABEl: testDeref8i16:
; DIS-LABEL: {{.+}} <testDeref8i16>:

  store <8 x i16> %v, <8 x i16>* %p, align 2
; ASM:   vst1.16  q11, [r5]
; DIS:   {{.+}}:  f4456a4f
; IASM-NOT:   vst1.16

  ret void
}

define internal void @testDeref16i8(<16 x i8> *%p, <16 x i8> %v) {
; ASM-LABEL: testDeref16i8:
; DIS-LABEL: {{.+}} <testDeref16i8>:

  store <16 x i8> %v, <16 x i8>* %p, align 1
; ASM:   vst1.8   q11, [r5]
; DIS:   {{.+}}:  f4456a0f
; IASM-NOT:   vst1.8

  ret void
}
