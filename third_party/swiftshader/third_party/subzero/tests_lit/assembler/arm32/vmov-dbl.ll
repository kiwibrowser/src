; Show that we can generate vmov for bitcasts between i64 and double.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   -allow-extern -reg-use r5,r10,d20 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   -allow-extern -reg-use r5,r10,d20 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   -allow-extern -reg-use r5,r10,d20 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   -allow-extern -reg-use r5,r10,d20 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal i64 @convertDoubleToI64(double %d) {
; ASM-LABEL: convertDoubleToI64:
; DIS-LABEL: {{.+}} <convertDoubleToI64>:

  %v = bitcast double %d to i64

; ASM:  vmov    r5, r10, d20
; DIS:   {{.+}}:    ec5a5b34
; IASM-NOT: vmov

  ret i64 %v
}

define internal double @convertI64ToDouble(i64 %i) {
; ASM-LABEL: convertI64ToDouble:
; DIS-LABEL: {{.+}} <convertI64ToDouble>:

  %v = bitcast i64 %i to double

; ASM:  vmov    d20, r5, r10
; DIS:   {{.+}}:    ec4a5b34
; IASM-NOT: vmov

  ; Note: This call is added to allow %v to be put into d20 (instead of
  ; return register d0).
  call void @ignore(double %v)

  ret double %v
}

declare external void @ignore(double)
