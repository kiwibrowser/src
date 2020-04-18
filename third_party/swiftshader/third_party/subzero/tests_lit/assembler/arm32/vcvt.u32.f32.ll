; Show that we know how to translate converting float to unsigned integer.

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

define internal i32 @FloatToUnsignedInt() {
; ASM-LABEL: FloatToUnsignedInt:
; DIS-LABEL: 00000000 <FloatToUnsignedInt>:
; IASM-LABEL: FloatToUnsignedInt:

entry:
; ASM-NEXT: .LFloatToUnsignedInt$entry:
; IASM-NEXT: .LFloatToUnsignedInt$entry:

  %v = fptoui float 0.0 to i32
; ASM:  vcvt.u32.f32    s20, s20
; DIS:   14:    eebcaaca
; IASM-NOT: vcvt

  ret i32 %v
}

define internal <4 x i32> @FloatVecToUIntVec(<4 x float> %a) {
; ASM-LABEL: FloatVecToUIntVec:
; DIS-LABEL: 00000030 <FloatVecToUIntVec>:
; IASM-LABEL: FloatVecToUIntVec:

  %v = fptoui <4 x float> %a to <4 x i32>

; ASM:         vcvt.u32.f32    q0, q0
; DIS:     40: f3bb07c0
; IASM-NOT:    vcvt

  ret <4 x i32> %v
}
