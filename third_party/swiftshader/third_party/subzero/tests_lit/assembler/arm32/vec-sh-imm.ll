; Show that we know how to translate vshl and vshr with immediate shift amounts.
; We abuse sign extension of vectors of i1 because that's the only way to force
; Subzero to emit these instructions.

; NOTE: We use -O2 to get rid of memory stores.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

define internal <4 x i32> @SextV4I1(<4 x i32> %a) {
; ASM-LABEL:SextV4I1
; DIS-LABEL:00000000 <SextV4I1>:
; IASM-LABEL:SextV4I1:

  %trunc = trunc <4 x i32> %a to <4 x i1>
  %sext = sext <4 x i1> %trunc to <4 x i32>
  ret <4 x i32> %sext
; ASM:         vshl.u32 {{.*}}, #31
; ASM-NEXT:    vshr.s32 {{.*}}, #31
; DIS:      0: f2bf0550
; DIS-NEXT: 4: f2a10050
; IASM-NOT:    vshl
; IASM-NOT:    vshr
}

define internal <8 x i16> @SextV8I1(<8 x i16> %a) {
; ASM-LABEL:SextV8I1
; DIS-LABEL:00000010 <SextV8I1>:
; IASM-LABEL:SextV8I1:

  %trunc = trunc <8 x i16> %a to <8 x i1>
  %sext = sext <8 x i1> %trunc to <8 x i16>
  ret <8 x i16> %sext
; ASM:          vshl.u16 {{.*}}, #15
; ASM-NEXT:     vshr.s16 {{.*}}, #15
; DIS:      10: f29f0550
; DIS-NEXT: 14: f2910050
; IASM-NOT:     vshl
; IASM-NOT:     vshr
}

define internal <16 x i8> @SextV16I1(<16 x i8> %a) {
; ASM-LABEL:SextV16I1
; DIS-LABEL:00000020 <SextV16I1>:
; IASM-LABEL:SextV16I1:

  %trunc = trunc <16 x i8> %a to <16 x i1>
  %sext = sext <16 x i1> %trunc to <16 x i8>
  ret <16 x i8> %sext
; ASM:          vshl.u8 {{.*}}, #7
; ASM-NEXT:     vshr.s8 {{.*}}, #7
; DIS:      20: f28f0550
; DIS-NEXT: 24: f2890050
; IASM-NOT:     vshl
; IASM-NOT:     vshr
}
