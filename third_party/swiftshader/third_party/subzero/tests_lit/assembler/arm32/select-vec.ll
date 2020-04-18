; Test that we handle select on vectors.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 --reg-use=s20  | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   --reg-use=s20 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 --reg-use=s20 | FileCheck %s --check-prefix=DIS

define internal <4 x float> @select4float(<4 x i1> %s, <4 x float> %a,
                                          <4 x float> %b) {
; ASM-LABEL:select4float:
; DIS-LABEL:00000000 <select4float>:
; IASM-LABEL:select4float:

entry:
  %res = select <4 x i1> %s, <4 x float> %a, <4 x float> %b

; ASM:          vshl.u32 [[M:.*]], {{.*}}, #31
; ASM-NEXT:     vshr.s32 [[M:.*]], {{.*}}, #31
; ASM-NEXT:     vbsl.i32 [[M]], {{.*}}
; DIS:       0: f2bf0550
; DIS-NEXT:  4: f2a10050
; DIS-NEXT:  8: f3120154
; IASM-NOT:     vshl
; IASM-NOT:     vshr
; IASM-NOT:     vbsl

  ret <4 x float> %res
}

define internal <4 x i32> @select4i32(<4 x i1> %s, <4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:select4i32:
; DIS-LABEL:00000010 <select4i32>:
; IASM-LABEL:select4i32:

entry:
  %res = select <4 x i1> %s, <4 x i32> %a, <4 x i32> %b

; ASM:          vshl.u32 [[M:.*]], {{.*}}, #31
; ASM-NEXT:     vshr.s32 [[M:.*]], {{.*}}, #31
; ASM-NEXT:     vbsl.i32 [[M]], {{.*}}
; DIS:      10: f2bf0550
; DIS-NEXT: 14: f2a10050
; DIS_NEXT: 18: f3120154
; IASM-NOT:     vshl
; IASM-NOT:     vshr
; IASM-NOT:     vbsl

  ret <4 x i32> %res
}

define internal <8 x i16> @select8i16(<8 x i1> %s, <8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:select8i16:
; DIS-LABEL:00000020 <select8i16>:
; IASM-LABEL:select8i16:

entry:
  %res = select <8 x i1> %s, <8 x i16> %a, <8 x i16> %b

; ASM:          vshl.u16 [[M:.*]], {{.*}}, #15
; ASM-NEXT:     vshr.s16 [[M:.*]], {{.*}}, #15
; ASM-NEXT:     vbsl.i16 [[M]], {{.*}}
; DIS:      20: f29f0550
; DIS-NEXT: 24: f2910050
; DIS-NEXT: 28: f3120154
; IASM-NOT:     vshl
; IASM-NOT:     vshr
; IASM-NOT:     vbsl

  ret <8 x i16> %res
}

define internal <16 x i8> @select16i8(<16 x i1> %s, <16 x i8> %a,
                                      <16 x i8> %b) {
; ASM-LABEL:select16i8:
; DIS-LABEL:00000030 <select16i8>:
; IASM-LABEL:select16i8:

entry:
  %res = select <16 x i1> %s, <16 x i8> %a, <16 x i8> %b

; ASM:          vshl.u8 [[M:.*]], {{.*}}, #7
; ASM-NEXT:     vshr.s8 [[M:.*]], {{.*}}, #7
; ASM-NEXT:     vbsl.i8 [[M]], {{.*}}
; DIS:      30: f28f0550
; DIS-NEXT: 34: f2890050
; DIS-NEXT: 38: f3120154
; IASM-NOT:     vshl
; IASM-NOT:     vshr
; IASM-NOT:     vbsl

  ret <16 x i8> %res
}
