; Show that we can move between float (S) and integer (GPR) registers.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   --reg-use=s20,r5,r6 | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 --reg-use=s20,r5,r6  | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   --reg-use=s20,r5,r6 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 --reg-use=s20,r5,r6 | FileCheck %s --check-prefix=DIS

define internal void @FloatToI1() {
; ASM-LABEL: FloatToI1:
; DIS-LABEL: {{.+}} <FloatToI1>:

  %v = fptoui float 0.0 to i1

; ASM:  vmov    r5, s20
; DIS:   {{.+}}:   ee1a5a10
; IASM-NOT: vmov

  ret void
}

define internal void @FloatToI8() {
; ASM-LABEL: FloatToI8:
; DIS-LABEL: {{.+}} <FloatToI8>:

  %v = fptoui float 0.0 to i8

; ASM:  vmov    r5, s20
; DIS:   {{.+}}:   ee1a5a10
; IASM-NOT: vmov

  ret void
}

define internal void @FloatToI16() {
; ASM-LABEL: FloatToI16:
; DIS-LABEL: {{.+}} <FloatToI16>:

  %v = fptoui float 0.0 to i16

; ASM:  vmov    r5, s20
; DIS:   {{.+}}:   ee1a5a10
; IASM-NOT: vmov

  ret void
}

define internal void @FloatToI32() {
; ASM-LABEL: FloatToI32:
; DIS-LABEL: {{.+}} <FloatToI32>:

  %v = fptoui float 0.0 to i32

; ASM:  vmov    r5, s20
; DIS:   {{.+}}:   ee1a5a10
; IASM-NOT: vmov

  ret void
}

define internal float @I1ToFloat() {
; ASM-LABEL: I1ToFloat:
; DIS-LABEL: {{.+}} <I1ToFloat>:

  %v = uitofp i1 1 to float

; ASM:  vmov    s20, r5
; DIS:  {{.+}}:   ee0a5a10
; IASM-NOT: vmov

  ret float %v
}

define internal float @I8ToFloat() {
; ASM-LABEL: I8ToFloat:
; DIS-LABEL: {{.+}} <I8ToFloat>:

  %v = uitofp i8 1 to float

; ASM:  vmov    s20, r5
; DIS:  {{.+}}:   ee0a5a10
; IASM-NOT: vmov

  ret float %v
}

define internal float @I16ToFloat() {
; ASM-LABEL: I16ToFloat:
; DIS-LABEL: {{.+}} <I16ToFloat>:

  %v = uitofp i16 1 to float

; ASM:  vmov    s20, r5
; DIS:  {{.+}}:   ee0a5a10
; IASM-NOT: vmov

  ret float %v
}

define internal float @I32ToFloat() {
; ASM-LABEL: I32ToFloat:
; DIS-LABEL: {{.+}} <I32ToFloat>:

  %v = uitofp i32 17 to float

; ASM:  vmov    s20, r5
; DIS:  {{.+}}:   ee0a5a10
; IASM-NOT: vmov

  ret float %v
}
