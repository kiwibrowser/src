; Show that we know how to translate vorr vector instructions.

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

define internal <4 x i32> @testVor4i32(<4 x i32> %v1, <4 x i32> %v2) {
; ASM-LABEL: testVor4i32:
; DIS-LABEL: 00000000 <testVor4i32>:
; IASM-LABEL: testVor4i32:

entry:
  %res = or <4 x i32> %v1, %v2

; ASM:     vorr.i32        q0, q0, q1
; DIS:   0:       f2200152
; IASM-NOT:     vorr.i32

  ret <4 x i32> %res
}

define internal <8 x i16> @testVor8i16(<8 x i16> %v1, <8 x i16> %v2) {
; ASM-LABEL: testVor8i16:
; DIS-LABEL: 00000010 <testVor8i16>:
; IASM-LABEL: testVor8i16:

entry:
  %res = or <8 x i16> %v1, %v2

; ASM:     vorr.i16        q0, q0, q1
; DIS:   10:       f2200152
; IASM-NOT:     vorr.i16

  ret <8 x i16> %res
}

define internal <16 x i8> @testVor16i8(<16 x i8> %v1, <16 x i8> %v2) {
; ASM-LABEL: testVor16i8:
; DIS-LABEL: 00000020 <testVor16i8>:
; IASM-LABEL: testVor16i8:

entry:
  %res = or <16 x i8> %v1, %v2

; ASM:     vorr.i8        q0, q0, q1
; DIS:   20:       f2200152
; IASM-NOT:     vorr.i8

  ret <16 x i8> %res
}

;;
;; The following tests make sure logical or works on predicate vectors.
;;

define internal <4 x i1> @testVor4i1(<4 x i1> %v1, <4 x i1> %v2) {
; ASM-LABEL: testVor4i1:
; DIS-LABEL: 00000030 <testVor4i1>:
; IASM-LABEL: testVor4i1:

entry:
  %res = or <4 x i1> %v1, %v2

; ASM:     vorr.i32        q0, q0, q1
; DIS:   30:       f2200152
; IASM-NOT:     vorr.i32

  ret <4 x i1> %res
}

define internal <8 x i1> @testVor8i1(<8 x i1> %v1, <8 x i1> %v2) {
; ASM-LABEL: testVor8i1:
; DIS-LABEL: 00000040 <testVor8i1>:
; IASM-LABEL: testVor8i1:

entry:
  %res = or <8 x i1> %v1, %v2

; ASM:     vorr.i16        q0, q0, q1
; DIS:   40:       f2200152
; IASM-NOT:     vorr.i16

  ret <8 x i1> %res
}

define internal <16 x i1> @testVor16i1(<16 x i1> %v1, <16 x i1> %v2) {
; ASM-LABEL: testVor16i1:
; DIS-LABEL: 00000050 <testVor16i1>:
; IASM-LABEL: testVor16i1:

entry:
  %res = or <16 x i1> %v1, %v2

; ASM:     vorr.i8        q0, q0, q1
; DIS:   50:       f2200152
; IASM-NOT:     vorr.i8

  ret <16 x i1> %res
}
