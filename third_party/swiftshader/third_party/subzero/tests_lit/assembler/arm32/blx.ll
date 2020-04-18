; Show that we know how to translate blx.

; NOTE: We use -O2 to get rid of memory stores.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

define internal void @callIndirect(i32 %addr) {
; ASM-LABEL:callIndirect:
; DIS-LABEL:00000000 <callIndirect>:
; IASM-LABEL:callIndirect:

entry:
; ASM-NEXT:.LcallIndirect$entry:
; IASM-NEXT:.LcallIndirect$entry:
; ASM-NEXT:     push    {lr}
; DIS-NEXT:   0:        e52de004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0xe0
; IASM-NEXT:    .byte 0x2d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     sub     sp, sp, #12
; DIS-NEXT:   4:        e24dd00c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2

   %calladdr = inttoptr i32 %addr to void (i32)*

; ASM-NEXT:     mov     r1, r0
; DIS-NEXT:   8:        e1a01000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

   call void %calladdr(i32 %addr)

; ASM-NEXT:     blx     r1
; DIS-NEXT:   c:        e12fff31
; IASM-NEXT:    .byte 0x31
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1

   ret void
}
