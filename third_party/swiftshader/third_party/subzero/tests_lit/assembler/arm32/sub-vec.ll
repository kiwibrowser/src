; Show that we know how to translate vsub vector instructions.

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

define internal <4 x float> @testVsubFloat4(<4 x float> %v1, <4 x float> %v2) {
; ASM-LABEL: testVsubFloat4:
; DIS-LABEL: 00000000 <testVsubFloat4>:
; IASM-LABEL: testVsubFloat4:

entry:
  %res = fsub <4 x float> %v1, %v2

; ASM:     vsub.f32        q10, q10, q11
; DIS:   8:       f2644de6
; IASM-NOT:     vsub.f32

  ret <4 x float> %res
}

define internal <4 x i32> @testVsub4i32(<4 x i32> %v1, <4 x i32> %v2) {
; ASM-LABEL: testVsub4i32:
; DIS-LABEL: 00000020 <testVsub4i32>:
; IASM-LABEL: testVsub4i32:

entry:
  %res = sub <4 x i32> %v1, %v2

; ASM:     vsub.i32        q10, q10, q11
; DIS:   28:       f36448e6
; IASM-NOT:     vsub.i32

  ret <4 x i32> %res
}

define internal <8 x i16> @testVsub8i16(<8 x i16> %v1, <8 x i16> %v2) {
; ASM-LABEL: testVsub8i16:
; DIS-LABEL: 00000040 <testVsub8i16>:
; IASM-LABEL: testVsub8i16:

entry:
  %res = sub <8 x i16> %v1, %v2

; ASM:     vsub.i16        q10, q10, q11
; DIS:   48:       f35448e6
; IASM-NOT:     vsub.i16

  ret <8 x i16> %res
}

define internal <16 x i8> @testVsub16i8(<16 x i8> %v1, <16 x i8> %v2) {
; ASM-LABEL: testVsub16i8:
; DIS-LABEL: 00000060 <testVsub16i8>:
; IASM-LABEL: testVsub16i8:

entry:
  %res = sub <16 x i8> %v1, %v2

; ASM:     vsub.i8        q10, q10, q11
; DIS:   68:       f34448e6
; IASM-NOT:     vsub.i8

  ret <16 x i8> %res
}
