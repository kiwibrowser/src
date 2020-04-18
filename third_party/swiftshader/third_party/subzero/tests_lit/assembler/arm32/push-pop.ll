; Show that we know how to translate push and pop.
; TODO(kschimpf) Translate pop instructions.

; NOTE: We use -O2 to get rid of memory stores.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 -allow-extern \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 -allow-extern | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   -allow-extern | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 -allow-extern | FileCheck %s --check-prefix=DIS

declare external void @DoSomething()

define internal void @SinglePushPop() {
  call void @DoSomething();
  ret void
}

; ASM-LABEL:SinglePushPop:
; ASM-NEXT:.LSinglePushPop$__0:
; ASM-NEXT:    push    {lr}
; ASM-NEXT:    sub     sp, sp, #12
; ASM-NEXT:    bl      DoSomething
; ASM-NEXT:    add     sp, sp, #12
; ASM-NEXT:    pop     {lr}
; ASM-NEXT:     # lr = def.pseudo
; ASM-NEXT:    bx      lr

; DIS-LABEL:00000000 <SinglePushPop>:
; DIS-NEXT:   0:        e52de004
; DIS-NEXT:   4:        e24dd00c
; DIS-NEXT:   8:        ebfffffe
; DIS-NEXT:   c:        e28dd00c
; DIS-NEXT:  10:        e49de004
; DIS-NEXT:  14:        e12fff1e

; IASM-LABEL:SinglePushPop:
; IASM-NEXT:.LSinglePushPop$__0:
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0xe0
; IASM-NEXT:    .byte 0x2d
; IASM-NEXT:    .byte 0xe5

; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2
; IASM-NEXT:    bl      DoSomething     @ .word ebfffffe
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe2

; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0xe0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe4

; IASM:         .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1

; This test is based on taking advantage of the over-eager -O2
; register allocator that puts V1 and V2 into callee-save registers,
; since the call instruction kills the scratch registers. This
; requires the callee-save registers to be pushed/popped in the
; prolog/epilog.
define internal i32 @MultPushPop(i32 %v1, i32 %v2) {
  call void @DoSomething();
  %v3 = add i32 %v1, %v2
  ret i32 %v3
}

; ASM-LABEL:MultPushPop:
; ASM-NEXT:.LMultPushPop$__0:
; ASM-NEXT:     push    {r4, r5, lr}
; ASM-NEXT:     sub     sp, sp, #4
; ASM-NEXT:     mov     r4, r0
; ASM-NEXT:     mov     r5, r1
; ASM-NEXT:     bl      DoSomething
; ASM-NEXT:     add     r4, r4, r5
; ASM-NEXT:     mov     r0, r4
; ASM-NEXT:     add     sp, sp, #4
; ASM-NEXT:     pop     {r4, r5, lr}
; ASM-NEXT:     # r4 = def.pseudo
; ASM-NEXT:     # r5 = def.pseudo
; ASM-NEXT:     # lr = def.pseudo
; ASM-NEXT:     bx      lr

; DIS-LABEL:00000020 <MultPushPop>:
; DIS-NEXT:  20:        e92d4030
; DIS-NEXT:  24:        e24dd004
; DIS-NEXT:  28:        e1a04000
; DIS-NEXT:  2c:        e1a05001
; DIS-NEXT:  30:        ebfffffe
; DIS-NEXT:  34:        e0844005
; DIS-NEXT:  38:        e1a00004
; DIS-NEXT:  3c:        e28dd004
; DIS-NEXT:  40:        e8bd4030
; DIS-NEXT:  44:        e12fff1e

; IASM-LABEL:MultPushPop:
; IASM-NEXT:.LMultPushPop$__0:
; IASM-NEXT:    .byte 0x30
; IASM-NEXT:    .byte 0x40
; IASM-NEXT:    .byte 0x2d
; IASM-NEXT:    .byte 0xe9

; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2

; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x40
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x50
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

; IASM-NEXT:    bl      DoSomething     @ .word ebfffffe
; IASM-NEXT:    .byte 0x5
; IASM-NEXT:    .byte 0x40
; IASM-NEXT:    .byte 0x84
; IASM-NEXT:    .byte 0xe0

; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe2

; IASM-NEXT:    .byte 0x30
; IASM-NEXT:    .byte 0x40
; IASM-NEXT:    .byte 0xbd
; IASM-NEXT:    .byte 0xe8

; IASM:         .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1
