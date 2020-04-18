; Show that we know how to translate vcmp.

; REQUIRES: allow_dump

; TODO(kschimpf): Use include registers for compare instructions, so that the
; test is less brittle.

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 | FileCheck %s --check-prefix=DIS

define internal i32 @vcmpFloat(float %v1, float %v2) {
; ASM-LABEL: vcmpFloat:
; DIS-LABEL: 00000000 <vcmpFloat>:
; IASM-LABEL: vcmpFloat:

entry:
; ASM-NEXT: .LvcmpFloat$entry:
; IASM-NEXT: .LvcmpFloat$entry:

  %cmp = fcmp olt float %v1, %v2

; ASM:      vcmp.f32        s0, s1
; DIS:   14:      eeb40a60
; IASM-NOT: vcmp

  %res = zext i1 %cmp to i32
  ret i32 %res
}

define internal i32 @vcmpFloatToZero(float %v) {
; ASM-LABEL: vcmpFloatToZero:
; DIS-LABEL: 00000040 <vcmpFloatToZero>:
; IASM-LABEL: vcmpFloatToZero:

entry:
; ASM-NEXT: .LvcmpFloatToZero$entry:
; IASM-NEXT: .LvcmpFloatToZero$entry:

  %cmp = fcmp olt float %v, 0.0

; ASM:      vcmp.f32        s0, #0.0
; DIS:   4c:      eeb50a40
; IASM-NOT: vcmp

  %res = zext i1 %cmp to i32
  ret i32 %res
}

define internal i32 @vcmpDouble(double %v1, double %v2) {
; ASM-LABEL: vcmpDouble:
; DIS-LABEL: 00000080 <vcmpDouble>:
; IASM-LABEL: vcmpDouble:

entry:
; ASM-NEXT: .LvcmpDouble$entry:
; IASM-NEXT: .LvcmpDouble$entry:

  %cmp = fcmp olt double %v1, %v2

; ASM:      vcmp.f64        d0, d1
; DIS:   94:      eeb40b41
; IASM-NOT: vcmp

  %res = zext i1 %cmp to i32
  ret i32 %res
}

define internal i32 @vcmpDoubleToZero(double %v) {
; ASM-LABEL: vcmpDoubleToZero:
; DIS-LABEL: 000000c0 <vcmpDoubleToZero>:
; IASM-LABEL: vcmpDoubleToZero:

entry:
; ASM-NEXT: .LvcmpDoubleToZero$entry:
; IASM-NEXT: .LvcmpDoubleToZero$entry:

  %cmp = fcmp olt double %v, 0.0

; ASM:      vcmp.f64        d0, #0.0
; DIS:   cc:      eeb50b40
; IASM-NOT: vcmp

  %res = zext i1 %cmp to i32
  ret i32 %res
}
