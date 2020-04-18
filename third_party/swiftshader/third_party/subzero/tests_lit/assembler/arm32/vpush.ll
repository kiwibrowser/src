; Show that we know how to translate vpush and vpop.

; NOTE: We use -O2 because vpush/vpop only occur if optimized. Uses
; simple call with double parameters to cause the insertion of
; vpush/vpop.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 -reg-use=d9,d10 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 -reg-use=d9,d10 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 -reg-use=d9,d10 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 -reg-use=d9,d10 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal double @testVpushVpop(double %v1, double %v2) {
; ASM-LABEL: testVpushVpop:
; DIS-LABEL: 00000000 <testVpushVpop>:

; ASM:  vpush   {s18, s19, s20, s21}
; DIS:    0:    ed2d9a04
; IASM-NOT: vpush

  call void @foo()
  %res = fadd double %v1, %v2
  ret double %res

; ASM:  vpop    {s18, s19, s20, s21}
; DIS:   28:       ecbd9a04
; IASM-NOT: vpopd

}

define internal void @foo() {
  ret void
}
