; Test vldr{s,d} and vstr{s,d} when address is offset with an immediate.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   -reg-use d20 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   -reg-use d20 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   -reg-use d20 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   -reg-use d20 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal i32 @testFloatImm(float %f) {
; ASM-LABEL: testFloatImm:
; DIS-LABEL: 00000000 <testFloatImm>:
; IASM-LABEL: testFloatImm:

entry:
; ASM: .LtestFloatImm$entry:
; IASM: .LtestFloatImm$entry:

; ASM:  vstr    s0, [sp, #4]
; DIS:    4:    ed8d0a01
; IASM-NOT: vstr

  %v = bitcast float %f to i32

; ASM:  vldr    s0, [sp, #4]
; DIS:    8:    ed9d0a01
; IASM-NOT: vldr

  ret i32 %v
}

define internal i64 @testDoubleImm(double %d) {
; ASM-LABEL: testDoubleImm:
; DIS-LABEL: 00000020 <testDoubleImm>:
; IASM-LABEL: testDoubleImm:

entry:
; ASM: .LtestDoubleImm$entry:
; IASM: .LtestDoubleImm$entry:

; ASM:  vstr    d0, [sp, #8]
; DIS:   24:    ed8d0b02
; IASM-NOT: vstr

  %v = bitcast double %d to i64

; ASM:  vldr    d20, [sp, #8]
; DIS:   28:    eddd4b02
; IASM-NOT: vldr

  ret i64 %v
}
