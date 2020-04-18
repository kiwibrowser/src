; Show that we can translate IR instruction "trap".

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=DIS

; testUnreachable generates a trap for the unreachable instruction.

define internal void @testUnreachable() {
; ASM-LABEL: testUnreachable:
; DIS-LABEL: 00000000 <testUnreachable>:

  unreachable

; ASM:  .long 0xe7fedef0
; DIS-NEXT:    0:       e7fedef0
; IASM-NOT:     .long 0xe7fedef0
}

; testTrap uses integer division to test this, since a trap is
; inserted if one divides by zero.

define internal i32 @testTrap(i32 %v1, i32 %v2) {
; ASM-LABEL: testTrap:
; DIS-LABEL: 00000010 <testTrap>:
; IASM-LABEL: testTrap:

  %res = udiv i32 %v1, %v2

; ASM:          bne
; DIS:  28:     1a000000
; IASM-NOT:     bne

; ASM-NEXT:     .long 0xe7fedef0
; DIS-NEXT:  2c:        e7fedef0
; IASM-NOT:     .long 0xe7fedef0

  ret i32 %res
}
