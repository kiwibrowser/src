; Show that we know how to translate the fabs intrinsic on float vectors.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   -reg-use q5 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   -reg-use q5 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   -reg-use q5 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   -reg-use q5 \
; RUN:   | FileCheck %s --check-prefix=DIS

declare <4 x float> @llvm.fabs.v4f32(<4 x float>)

define internal <4 x float> @_Z6myFabsDv4_f(<4 x float> %a) {
; ASM-LABEL: _Z6myFabsDv4_f:
; DIS-LABEL: {{.+}} <_Z6myFabsDv4_f>:
; IASM-LABEL: _Z6myFabsDv4_f:

  %x = call <4 x float> @llvm.fabs.v4f32(<4 x float> %a)

; ASM:            vabs.f32    q5, q5
; DIS: {{.+}}:    f3b9a74a
; IASM-NOT:       vabs.f32

  ret <4 x float> %x
}
