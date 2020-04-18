; Test branching instructions.
; TODO(kschimpf): Get this working.

; REQUIRES: allow_dump

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

define internal void @simple_uncond_branch() {
; DIS-LABEL: 00000000 <simple_uncond_branch>:
; ASM-LABEL: simple_uncond_branch:
; IASM-LABEL:simple_uncond_branch:

; ASM-NEXT:  .Lsimple_uncond_branch$__0:
; IASM-NEXT: .Lsimple_uncond_branch$__0:

  br label %l2
; ASM-NEXT:     b       .Lsimple_uncond_branch$l2
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xea
; DIS-NEXT:   0:   ea000000

l1:
; ASM-NEXT:  .Lsimple_uncond_branch$l1:
; IASM-NEXT: .Lsimple_uncond_branch$l1:

  br label %l3
; ASM-NEXT:     b       .Lsimple_uncond_branch$l3
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xea
; DIS-NEXT:   4:   ea000000

l2:
; ASM-NEXT:  .Lsimple_uncond_branch$l2:
; IASM-NEXT: .Lsimple_uncond_branch$l2:

  br label %l1
; ASM-NEXT:     b       .Lsimple_uncond_branch$l1
; IASM-NEXT:    .byte 0xfd
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0xea
; DIS-NEXT:   8:   eafffffd

l3:
; ASM-NEXT:  .Lsimple_uncond_branch$l3:
; IASM-NEXT: .Lsimple_uncond_branch$l3:

  ret void
; ASM-NEXT:     bx      lr
; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1
; DIS-NEXT:   c:   e12fff1e

}
