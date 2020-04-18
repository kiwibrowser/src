; Show that we know how to translate vmov for casts.

; NOTE: Restricts S register to one that will better test S register encodings.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   -reg-use s20,s22,d20,d22 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   -reg-use s20,s22,d20,d22 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   -reg-use s20,s22,d20,d22 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   -reg-use s20,s22,d20,d22 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal float @castToFloat(i32 %a) {
; ASM-LABEL: castToFloat:
; DIS-LABEL: 00000000 <castToFloat>:
; IASM-LABEL: castToFloat:

entry:
; ASM: .LcastToFloat$entry:
; IASM: .LcastToFloat$entry:

  %0 = bitcast i32 %a to float

; ASM:          vmov    s20, r0
; DIS:  10:     ee0a0a10
; IASM-NOT:     vmov

  ret float %0
}
