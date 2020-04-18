; Show that we know how to translate move (immediate) ARM instruction.

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

define internal i32 @Imm1() {
  ret i32 1
}

; ASM-LABEL: Imm1:
; ASM: mov      r0, #1

; DIS-LABEL:00000000 <Imm1>:
; DIS-NEXT:   0:        e3a00001

; IASM-LABEL: Imm1:
; IASM: .byte 0x1
; IASM: .byte 0x0
; IASM: .byte 0xa0
; IASM: .byte 0xe3


define internal i32 @rotateFImmAA() {
  ; immediate = 0x000002a8 = b 0000 0000 0000 0000 0000 0010 1010 1000
  ret i32 680
}

; ASM-LABEL: rotateFImmAA:
; ASM: mov      r0, #680

; DIS-LABEL:00000010 <rotateFImmAA>:
; DIS-NEXT:  10:        e3a00faa

; IASM-LABEL: rotateFImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0xf
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotateEImmAA() {
 ; immediate = 0x00000aa0 = b 0000 0000 0000 0000 0000 1010 1010 0000
  ret i32 2720
}

; ASM-LABEL: rotateEImmAA:
; ASM: mov      r0, #2720

; DIS-LABEL:00000020 <rotateEImmAA>:
; DIS-NEXT:  20:        e3a00eaa

; IASM-LABEL: rotateEImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0xe
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotateDImmAA() {
  ; immediate = 0x00002a80 = b 0000 0000 0000 0000 0010 1010 1000 0000
  ret i32 10880
}

; ASM-LABEL: rotateDImmAA:
; ASM: mov      r0, #10880

; DIS-LABEL:00000030 <rotateDImmAA>:
; DIS-NEXT:  30:        e3a00daa

; IASM-LABEL: rotateDImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0xd
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotateCImmAA() {
  ; immediate = 0x0000aa00 = b 0000 0000 0000 0000 1010 1010 0000 0000
  ret i32 43520
}

; ASM-LABEL: rotateCImmAA:
; ASM: mov      r0, #43520

; DIS-LABEL:00000040 <rotateCImmAA>:
; DIS-NEXT:  40:        e3a00caa

; IASM-LABEL: rotateCImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0xc
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotateBImmAA() {
  ; immediate = 0x0002a800 = b 0000 0000 0000 0010 1010 1000 0000 0000
  ret i32 174080
}

; ASM-LABEL: rotateBImmAA:
; ASM: mov      r0, #174080

; DIS-LABEL:00000050 <rotateBImmAA>:
; DIS-NEXT:  50:        e3a00baa

; IASM-LABEL: rotateBImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0xb
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotateAImmAA() {
  ; immediate = 0x000aa000 = b 0000 0000 0000 1010 1010 0000 0000 0000
  ret i32 696320
}

; ASM-LABEL: rotateAImmAA:
; ASM: mov      r0, #696320

; DIS-LABEL:00000060 <rotateAImmAA>:
; DIS-NEXT:  60:        e3a00aaa

; IASM-LABEL: rotateAImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0xa
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotate9ImmAA() {
  ; immediate = 0x002a8000 = b 0000 0000 0010 1010 1000 0000 0000 0000
  ret i32 2785280
}

; ASM-LABEL: rotate9ImmAA:
; ASM: mov      r0, #2785280

; DIS-LABEL:00000070 <rotate9ImmAA>:
; DIS-NEXT:  70:        e3a009aa

; IASM-LABEL: rotate9ImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0x9
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotate8ImmAA() {
  ; immediate = 0x00aa0000 = b 0000 0000 1010 1010 0000 0000 0000 0000
  ret i32 11141120
}

; ASM-LABEL: rotate8ImmAA:
; ASM: mov      r0, #11141120

; DIS-LABEL:00000080 <rotate8ImmAA>:
; DIS-NEXT:  80:        e3a008aa

; IASM-LABEL: rotate8ImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0x8
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotate7ImmAA() {
  ; immediate = 0x02a80000 = b 0000 0010 1010 1000 0000 0000 0000 0000
  ret i32 44564480
}

; ASM-LABEL: rotate7ImmAA:
; ASM:  mov     r0, #44564480

; DIS-LABEL:00000090 <rotate7ImmAA>:
; DIS-NEXT:  90:        e3a007aa

; IASM-LABEL: rotate7ImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0x7
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotate6ImmAA() {
  ; immediate = 0x0aa00000 = b 0000 1010 1010 0000 0000 0000 0000 0000
  ret i32 178257920
}

; ASM-LABEL: rotate6ImmAA:
; ASM:  mov     r0, #178257920

; DIS-LABEL:000000a0 <rotate6ImmAA>:
; DIS-NEXT:  a0:        e3a006aa

; IASM-LABEL: rotate6ImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0x6
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotate5ImmAA() {
  ; immediate = 0x2a800000 = b 0010 1010 1000 0000 0000 0000 0000 0000
  ret i32 713031680
}

; ASM-LABEL: rotate5ImmAA:
; ASM:  mov     r0, #713031680

; DIS-LABEL:000000b0 <rotate5ImmAA>:
; DIS-NEXT:  b0:        e3a005aa

; IASM-LABEL: rotate5ImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0x5
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotate4ImmAA() {
  ; immediate = 0xaa000000 = b 1010 1010 0000 0000 0000 0000 0000 0000
  ret i32 2852126720
}

; ASM-LABEL: rotate4ImmAA:
; ASM: mov      r0, #2852126720

; DIS-LABEL:000000c0 <rotate4ImmAA>:
; DIS-NEXT:  c0:        e3a004aa

; IASM-LABEL: rotate4ImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0x4
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotate3ImmAA() {
  ; immediate = 0xa8000002 = b 1010 1000 0000 0000 0000 0000 0000 0010
  ret i32 2818572290
}

; ASM-LABEL: rotate3ImmAA:
; ASM: mov      r0, #2818572290

; DIS-LABEL:000000d0 <rotate3ImmAA>:
; DIS-NEXT:  d0:        e3a003aa

; IASM-LABEL: rotate3ImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0x3
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotate2ImmAA() {
  ; immediate = 0xa000000a = b 1010 0000 0000 0000 0000 0000 0000 1010
  ret i32 2684354570
}

; ASM-LABEL: rotate2ImmAA:
; ASM:  mov     r0, #2684354570

; DIS-LABEL:000000e0 <rotate2ImmAA>:
; DIS-NEXT:  e0:        e3a002aa

; IASM-LABEL: rotate2ImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0x2
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotate1ImmAA() {
  ; immediate = 0x8000002a = b 1000 1000 0000 0000 0000 0000 0010 1010
  ret i32 2147483690
}

; ASM-LABEL: rotate1ImmAA:
; ASM: mov      r0, #2147483690

; DIS-LABEL:000000f0 <rotate1ImmAA>:
; DIS-NEXT:  f0:        e3a001aa

; IASM-LABEL: rotate1ImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0x1
; IASM: .byte 0xa0
; IASM: .byte 0xe3

define internal i32 @rotate0ImmAA() {
  ; immediate = 0x000000aa = b 0000 0000 0000 0000 0000 0000 1010 1010
  ret i32 170
}

; ASM-LABEL: rotate0ImmAA:
; ASM: mov      r0, #170

; DIS-LABEL:00000100 <rotate0ImmAA>:
; DIS-NEXT: 100:        e3a000aa

; IASM-LABEL: rotate0ImmAA:
; IASM: .byte 0xaa
; IASM: .byte 0x0
; IASM: .byte 0xa0
; IASM: .byte 0xe3
