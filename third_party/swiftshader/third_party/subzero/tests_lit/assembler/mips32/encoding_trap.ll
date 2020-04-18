; Test encoding of MIPS32 floating point arithmetic instructions

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=mips32 --args -O2 \
; RUN:   --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=mips32 --assemble --disassemble \
; RUN:   --args -O2 --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=mips32 --args -O2 \
; RUN:   --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=mips32 --assemble --disassemble \
; RUN:   --args -O2 --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal void @encTrap() {
  unreachable
}

; ASM-LABEL: encTrap
; ASM-NEXT: .LencTrap$__0:
; ASM: 	teq	$zero, $zero, 0

; DIS-LABEL: 00000000 <encTrap>:
; DIS-NEXT:    0:	00000034 	teq	zero,zero

; IASM-LABEL: encTrap:
; IASM-NEXT: .LencTrap$__0:
; IASM-NEXT: 	.byte 0x34
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
