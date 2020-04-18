; Show that we know how to translate veor vector instructions.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal <4 x i32> @testVxor4i32(<4 x i32> %v1, <4 x i32> %v2) {
; ASM-LABEL: testVxor4i32:
; DIS-LABEL: 00000000 <testVxor4i32>:
; IASM-LABEL: testVxor4i32:

entry:
  %res = xor <4 x i32> %v1, %v2

; ASM:     veor.i32        q0, q0, q1
; DIS:   0:       f3000152
; IASM-NOT:     veor.i32

  ret <4 x i32> %res
}

define internal <8 x i16> @testVxor8i16(<8 x i16> %v1, <8 x i16> %v2) {
; ASM-LABEL: testVxor8i16:
; DIS-LABEL: 00000010 <testVxor8i16>:
; IASM-LABEL: testVxor8i16:

entry:
  %res = xor <8 x i16> %v1, %v2

; ASM:     veor.i16        q0, q0, q1
; DIS:   10:       f3000152
; IASM-NOT:     veor.i16

  ret <8 x i16> %res
}

define internal <16 x i8> @testVxor16i8(<16 x i8> %v1, <16 x i8> %v2) {
; ASM-LABEL: testVxor16i8:
; DIS-LABEL: 00000020 <testVxor16i8>:
; IASM-LABEL: testVxor16i8:

entry:
  %res = xor <16 x i8> %v1, %v2

; ASM:     veor.i8        q0, q0, q1
; DIS:   20:       f3000152
; IASM-NOT:     veor.i8

  ret <16 x i8> %res
}

;;
;; The following tests make sure logical xor works on predicate vectors.
;;

define internal <4 x i1> @testVxor4i1(<4 x i1> %v1, <4 x i1> %v2) {
; ASM-LABEL: testVxor4i1:
; DIS-LABEL: 00000030 <testVxor4i1>:
; IASM-LABEL: testVxor4i1:

entry:
  %res = xor <4 x i1> %v1, %v2

; ASM:     veor.i32        q0, q0, q1
; DIS:   30:       f3000152
; IASM-NOT:     veor.i32

  ret <4 x i1> %res
}

define internal <8 x i1> @testVxor8i1(<8 x i1> %v1, <8 x i1> %v2) {
; ASM-LABEL: testVxor8i1:
; DIS-LABEL: 00000040 <testVxor8i1>:
; IASM-LABEL: testVxor8i1:

entry:
  %res = xor <8 x i1> %v1, %v2

; ASM:     veor.i16        q0, q0, q1
; DIS:   40:       f3000152
; IASM-NOT:     veor.i16

  ret <8 x i1> %res
}

define internal <16 x i1> @testVxor16i1(<16 x i1> %v1, <16 x i1> %v2) {
; ASM-LABEL: testVxor16i1:
; DIS-LABEL: 00000050 <testVxor16i1>:
; IASM-LABEL: testVxor16i1:

entry:
  %res = xor <16 x i1> %v1, %v2

; ASM:     veor.i8        q0, q0, q1
; DIS:   50:       f3000152
; IASM-NOT:     veor.i8

  ret <16 x i1> %res
}
