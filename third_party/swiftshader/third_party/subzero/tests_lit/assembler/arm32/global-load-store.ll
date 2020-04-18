; TODO(kschimpf): Show that we can handle global variable loads/stores.

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

@filler = internal global [128 x i8] zeroinitializer, align 4

@global1 = internal global [4 x i8] zeroinitializer, align 4

; ASM-LABEL: global1:
; ASM-NEXT:    .zero   4
; ASM-NEXT:    .size   global1, 4
; ASM-NEXT:    .text
; ASM-NEXT:    .p2alignl 4,0xe7fedef0

; IASM-LABEL:global1:
; IASM-NEXT:    .zero   4
; IASM-NEXT:    .size   global1, 4
; IASM-NEXT:    .text
; IASM-NEXT:    .p2alignl 4,0xe7fedef0

define internal i32 @load() {
  %addr = bitcast [4 x i8]* @global1 to i32*
  %v = load i32, i32* %addr, align 1
  ret i32 %v
}

; ASM-LABEL: load:
; ASM-NEXT: .Lload$__0:
; ASM-NEXT:    movw    r0, #:lower16:global1
; ASM-NEXT:    movt    r0, #:upper16:global1
; ASM-NEXT:    ldr     r0, [r0]
; ASM-NEXT:    bx      lr

; DIS-LABEL:00000000 <load>:
; DIS-NEXT:   0:   e3000000
; DIS-NEXT:   4:   e3400000
; DIS-NEXT:   8:   e5900000
; DIS-NEXT:   c:   e12fff1e

; IASM-LABEL:load:
; IASM-NEXT: .Lload$__0:
; IASM-NEXT:    movw    r0, #:lower16:global1   @ .word e3000000
; IASM-NEXT:    movt    r0, #:upper16:global1   @ .word e3400000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x90
; IASM-NEXT:    .byte 0xe5
; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1

define internal void @store(i32 %v) {
  %addr = bitcast [4 x i8]* @global1 to i32*
  store i32 %v, i32* %addr, align 1
  ret void
}

; ASM-LABEL:store:
; ASM-NEXT: .Lstore$__0:
; ASM-NEXT:     movw    r1, #:lower16:global1
; ASM-NEXT:     movt    r1, #:upper16:global1
; ASM-NEXT:     str     r0, [r1]
; ASM-NEXT:     bx      lr

; DIS-LABEL:00000010 <store>:
; DIS-NEXT:  10:   e3001000
; DIS-NEXT:  14:   e3401000
; DIS-NEXT:  18:   e5810000
; DIS-NEXT:  1c:   e12fff1e

; IASM-LABEL:store:
; IASM-NEXT: .Lstore$__0:
; IASM-NEXT:    movw    r1, #:lower16:global1   @ .word e3001000
; IASM-NEXT:    movt    r1, #:upper16:global1   @ .word e3401000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x81
; IASM-NEXT:    .byte 0xe5
; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1
