; Show that we know how to translate converting unsigned integer to float.

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

define internal float @SignedIntToFloat() {
; ASM-LABEL: SignedIntToFloat:
; DIS-LABEL: 00000000 <SignedIntToFloat>:
; IASM-LABEL: SignedIntToFloat:

entry:
; ASM: .LSignedIntToFloat$entry:
; IASM: .LSignedIntToFloat$entry:

  %v = uitofp i32 17 to float

; ASM:  vcvt.f32.u32    s20, s20
; DIS:  10:     eeb8aa4a
; IASM-NOT: vcvt

  ret float %v
}

define internal <4 x float> @UIntVecToFloatVec(<4 x i32> %a) {
; ASM-LABEL: UIntVecToFloatVec:
; DIS-LABEL: 00000030 <UIntVecToFloatVec>:
; IASM-LABEL: UIntVecToFloatVec:

  %v = uitofp <4 x i32> %a to <4 x float>

; ASM:         vcvt.f32.u32    q0, q0
; DIS:     40: f3bb06c0
; IASM-NOT:    vcvt

  ret <4 x float> %v
}
