; Show that we know how to translate converting float to signed integer.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   --reg-use=s20 | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 --reg-use=s20  | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   --reg-use=s20 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 --reg-use=s20 | FileCheck %s --check-prefix=DIS

define internal i32 @FloatToSignedInt() {
; ASM-LABEL: FloatToSignedInt:
; DIS-LABEL: 00000000 <FloatToSignedInt>:
; IASM-LABEL: FloatToSignedInt:

entry:
; ASM-NEXT: .LFloatToSignedInt$entry:
; IASM-NEXT: .LFloatToSignedInt$entry:

  %v = fptosi float 0.0 to i32

; ASM:  vcvt.s32.f32    s20, s20
; DIS:   14:    eebdaaca
; IASM-NOT: vcvt

  ret i32 %v
}

define internal <4 x i32> @FloatVecToIntVec(<4 x float> %a) {
; ASM-LABEL: FloatVecToIntVec:
; DIS-LABEL: 00000030 <FloatVecToIntVec>:
; IASM-LABEL: FloatVecToIntVec:

  %v = fptosi <4 x float> %a to <4 x i32>

; ASM:         vcvt.s32.f32    q0, q0
; DIS:     40: f3bb0740
; IASM-NOT:    vcvt

  ret <4 x i32> %v
}
