; Show that we know how to translate vand vector instructions.

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

define internal <4 x i32> @testVand4i32(<4 x i32> %v1, <4 x i32> %v2) {
; ASM-LABEL: testVand4i32:
; DIS-LABEL: 00000000 <testVand4i32>:
; IASM-LABEL: testVand4i32:

entry:
  %res = and <4 x i32> %v1, %v2

; ASM:     vand.i32        q0, q0, q1
; DIS:   0:       f2000152
; IASM-NOT:     vand

  ret <4 x i32> %res
}

define internal <8 x i16> @testVand8i16(<8 x i16> %v1, <8 x i16> %v2) {
; ASM-LABEL: testVand8i16:
; DIS-LABEL: 00000010 <testVand8i16>:
; IASM-LABEL: testVand8i16:

entry:
  %res = and <8 x i16> %v1, %v2

; ASM:     vand.i16        q0, q0, q1
; DIS:   10:       f2000152
; IASM-NOT:     vand

  ret <8 x i16> %res
}

define internal <16 x i8> @testVand16i8(<16 x i8> %v1, <16 x i8> %v2) {
; ASM-LABEL: testVand16i8:
; DIS-LABEL: 00000020 <testVand16i8>:
; IASM-LABEL: testVand16i8:

entry:
  %res = and <16 x i8> %v1, %v2

; ASM:     vand.i8        q0, q0, q1
; DIS:   20:       f2000152
; IASM-NOT:     vand

  ret <16 x i8> %res
}

;;
;; The following tests make sure logical and works on predicate vectors.
;;

define internal <4 x i1> @testVand4i1(<4 x i1> %v1, <4 x i1> %v2) {
; ASM-LABEL: testVand4i1:
; DIS-LABEL: 00000030 <testVand4i1>:
; IASM-LABEL: testVand4i1:

entry:
  %res = and <4 x i1> %v1, %v2

; ASM:     vand.i32        q0, q0, q1
; DIS:   30:       f2000152
; IASM-NOT:     vand

  ret <4 x i1> %res
}

define internal <8 x i1> @testVand8i1(<8 x i1> %v1, <8 x i1> %v2) {
; ASM-LABEL: testVand8i1:
; DIS-LABEL: 00000040 <testVand8i1>:
; IASM-LABEL: testVand8i1:

entry:
  %res = and <8 x i1> %v1, %v2

; ASM:     vand.i16        q0, q0, q1
; DIS:   40:       f2000152
; IASM-NOT:     vand

  ret <8 x i1> %res
}

define internal <16 x i1> @testVand16i1(<16 x i1> %v1, <16 x i1> %v2) {
; ASM-LABEL: testVand16i1:
; DIS-LABEL: 00000050 <testVand16i1>:
; IASM-LABEL: testVand16i1:

entry:
  %res = and <16 x i1> %v1, %v2

; ASM:     vand.i8        q0, q0, q1
; DIS:   50:       f2000152
; IASM-NOT:     vand

  ret <16 x i1> %res
}
