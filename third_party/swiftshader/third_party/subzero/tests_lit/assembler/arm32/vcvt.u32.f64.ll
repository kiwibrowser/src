; Show that we know how to translate converting double to unsigned integer.

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

define internal i32 @DoubleToUnsignedInt() {
; ASM-LABEL: DoubleToUnsignedInt:
; DIS-LABEL: 00000000 <DoubleToUnsignedInt>:

entry:
; ASM: .LDoubleToUnsignedInt$entry:

  %v = fptoui double 0.0 to i32

; ASM:  vcvt.u32.f64    s20, d0
; DIS:    c:    eebcabc0
; IASM-NOT: vcvt

  ret i32 %v
}
