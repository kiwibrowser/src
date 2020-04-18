; Simple test that returns various immediates. For fixed-width instruction
; sets, some immediates are more complicated than others.
; For x86-32, it shouldn't be a problem.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 \
; RUN:      -allow-externally-defined-symbols | FileCheck %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -O2 \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 %s

; TODO: Switch to --filetype=obj when possible.
; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -O2 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; Test 8-bits of all ones rotated right by various amounts (even vs odd).
; ARM has a shifter that allows encoding 8-bits rotated right by even amounts.
; The first few "rotate right" test cases are expressed as shift-left.

define internal i32 @ret_8bits_shift_left0() {
  ret i32 255
}
; CHECK-LABEL: ret_8bits_shift_left0
; CHECK-NEXT: mov eax,0xff
; ARM32-LABEL: ret_8bits_shift_left0
; ARM32-NEXT: mov r0, #255
; MIPS32-LABEL: ret_8bits_shift_left0
; MIPS32-NEXT: li	v0,255

define internal i32 @ret_8bits_shift_left1() {
  ret i32 510
}
; CHECK-LABEL: ret_8bits_shift_left1
; CHECK-NEXT: mov eax,0x1fe
; ARM32-LABEL: ret_8bits_shift_left1
; ARM32-NEXT: movw r0, #510
; MIPS32-LABEL: ret_8bits_shift_left1
; MIPS32-NEXT: li	v0,510

define internal i32 @ret_8bits_shift_left2() {
  ret i32 1020
}
; CHECK-LABEL: ret_8bits_shift_left2
; CHECK-NEXT: mov eax,0x3fc
; ARM32-LABEL: ret_8bits_shift_left2
; ARM32-NEXT: mov r0, #1020
; MIPS32-LABEL: ret_8bits_shift_left2
; MIPS32-NEXT: li	v0,1020

define internal i32 @ret_8bits_shift_left4() {
  ret i32 4080
}
; CHECK-LABEL: ret_8bits_shift_left4
; CHECK-NEXT: mov eax,0xff0
; ARM32-LABEL: ret_8bits_shift_left4
; ARM32-NEXT: mov r0, #4080
; MIPS32-LABEL: ret_8bits_shift_left4
; MIPS32-NEXT: li	v0,4080

define internal i32 @ret_8bits_shift_left14() {
  ret i32 4177920
}
; CHECK-LABEL: ret_8bits_shift_left14
; CHECK-NEXT: mov eax,0x3fc000
; ARM32-LABEL: ret_8bits_shift_left14
; ARM32-NEXT: mov r0, #4177920
; MIPS32-LABEL: ret_8bits_shift_left14
; MIPS32-NEXT: lui	v0,0x3f
; MIPS32-NEXT: ori	v0,v0,0xc000

define internal i32 @ret_8bits_shift_left15() {
  ret i32 8355840
}
; CHECK-LABEL: ret_8bits_shift_left15
; CHECK-NEXT: mov eax,0x7f8000
; ARM32-LABEL: ret_8bits_shift_left15
; ARM32-NEXT: movw r0, #32768
; ARM32-NEXT: movt r0, #127
; MIPS32-LABEL: ret_8bits_shift_left15
; MIPS32-NEXT: lui	v0,0x7f
; MIPS32-NEXT: ori	v0,v0,0x8000

; Shift 8 bits left by 24 to the i32 limit. This is also ror by 8 bits.

define internal i32 @ret_8bits_shift_left24() {
  ret i32 4278190080
}
; CHECK-LABEL: ret_8bits_shift_left24
; CHECK-NEXT: mov eax,0xff000000
; ARM32-LABEL: ret_8bits_shift_left24
; ARM32-NEXT: mov r0, #-16777216
; ARM32-NEXT: bx lr
; MIPS32-LABEL: ret_8bits_shift_left24
; MIPS32-NEXT: lui	v0,0xff00

; The next few cases wrap around and actually demonstrate the rotation.

define internal i32 @ret_8bits_ror7() {
  ret i32 4261412865
}
; CHECK-LABEL: ret_8bits_ror7
; CHECK-NEXT: mov eax,0xfe000001
; ARM32-LABEL: ret_8bits_ror7
; ARM32-NEXT: movw r0, #1
; ARM32-NEXT: movt r0, #65024
; MIPS32-LABEL: ret_8bits_ror7
; MIPS32-NEXT: lui	v0,0xfe00
; MIPS32-NEXT: ori	v0,v0,0x1

define internal i32 @ret_8bits_ror6() {
  ret i32 4227858435
}
; CHECK-LABEL: ret_8bits_ror6
; CHECK-NEXT: mov eax,0xfc000003
; ARM32-LABEL: ret_8bits_ror6
; ARM32-NEXT: mov r0, #-67108861
; ARM32-NEXT: bx lr
; MIPS32-LABEL: ret_8bits_ror6
; MIPS32-NEXT: lui	v0,0xfc00
; MIPS32-NEXT: ori	v0,v0,0x3

define internal i32 @ret_8bits_ror5() {
  ret i32 4160749575
}
; CHECK-LABEL: ret_8bits_ror5
; CHECK-NEXT: mov eax,0xf8000007
; ARM32-LABEL: ret_8bits_ror5
; ARM32-NEXT: movw r0, #7
; ARM32-NEXT: movt r0, #63488
; MIPS32-LABEL: ret_8bits_ror5
; MIPS32-NEXT: lui	v0,0xf800
; MIPS32-NEXT: ori	v0,v0,0x7

define internal i32 @ret_8bits_ror4() {
  ret i32 4026531855
}
; CHECK-LABEL: ret_8bits_ror4
; CHECK-NEXT: mov eax,0xf000000f
; ARM32-LABEL: ret_8bits_ror4
; ARM32-NEXT: mov r0, #-268435441
; ARM32-NEXT: bx lr
; MIPS32-LABEL: ret_8bits_ror4
; MIPS32-NEXT: lui	v0,0xf000
; MIPS32-NEXT: ori	v0,v0,0xf

define internal i32 @ret_8bits_ror3() {
  ret i32 3758096415
}
; CHECK-LABEL: ret_8bits_ror3
; CHECK-NEXT: mov eax,0xe000001f
; ARM32-LABEL: ret_8bits_ror3
; ARM32-NEXT: movw r0, #31
; ARM32-NEXT: movt r0, #57344
; MIPS32-LABEL: ret_8bits_ror3
; MIPS32-NEXT: lui	v0,0xe000
; MIPS32-NEXT: ori	v0,v0,0x1f


define internal i32 @ret_8bits_ror2() {
  ret i32 3221225535
}
; CHECK-LABEL: ret_8bits_ror2
; CHECK-NEXT: mov eax,0xc000003f
; ARM32-LABEL: ret_8bits_ror2
; ARM32-NEXT: mov r0, #-1073741761
; ARM32-NEXT: bx lr
; MIPS32-LABEL: ret_8bits_ror2
; MIPS32-NEXT: lui	v0,0xc000
; MIPS32-NEXT: ori	v0,v0,0x3f

define internal i32 @ret_8bits_ror1() {
  ret i32 2147483775
}
; CHECK-LABEL: ret_8bits_ror1
; CHECK-NEXT: mov eax,0x8000007f
; ARM32-LABEL: ret_8bits_ror1
; ARM32-NEXT: movw r0, #127
; ARM32-NEXT: movt r0, #32768
; MIPS32-LABEL: ret_8bits_ror1
; MIPS32-NEXT: lui	v0,0x8000
; MIPS32-NEXT: ori	v0,v0,0x7f

; Some architectures can handle 16-bits at a time efficiently,
; so also test those.

define internal i32 @ret_16bits_lower() {
  ret i32 65535
}
; CHECK-LABEL: ret_16bits_lower
; CHECK-NEXT: mov eax,0xffff
; ARM32-LABEL: ret_16bits_lower
; ARM32-NEXT: movw r0, #65535
; ARM32-NEXT: bx lr
; MIPS32-LABEL: ret_16bits_lower
; MIPS32-NEXT: lui	v0,0x0
; MIPS32-NEXT: ori	v0,v0,0xffff

define internal i32 @ret_17bits_lower() {
  ret i32 131071
}
; CHECK-LABEL: ret_17bits_lower
; CHECK-NEXT: mov eax,0x1ffff
; ARM32-LABEL: ret_17bits_lower
; ARM32-NEXT: movw r0, #65535
; ARM32-NEXT: movt r0, #1
; MIPS32-LABEL: ret_17bits_lower
; MIPS32-NEXT: lui	v0,0x1
; MIPS32-NEXT: ori	v0,v0,0xffff


define internal i32 @ret_16bits_upper() {
  ret i32 4294901760
}
; CHECK-LABEL: ret_16bits_upper
; CHECK-NEXT: mov eax,0xffff0000
; ARM32-LABEL: ret_16bits_upper
; ARM32-NEXT: movw r0, #0
; ARM32-NEXT: movt r0, #65535
; MIPS32-LABEL: ret_16bits_upper
; MIPS32-NEXT: lui	v0,0xffff


; Some 32-bit immediates can be inverted, and moved in a single instruction.

define internal i32 @ret_8bits_inverted_shift_left0() {
  ret i32 4294967040
}
; CHECK-LABEL: ret_8bits_inverted_shift_left0
; CHECK-NEXT: mov eax,0xffffff00
; ARM32-LABEL: ret_8bits_inverted_shift_left0
; ARM32-NEXT: mvn r0, #255
; ARM32-NEXT: bx lr
; MIPS32-LABEL: ret_8bits_inverted_shift_left0
; MIPS32-NEXT: li	v0,-256

define internal i32 @ret_8bits_inverted_shift_left24() {
  ret i32 16777215
}
; CHECK-LABEL: ret_8bits_inverted_shift_left24
; CHECK-NEXT: mov eax,0xffffff
; ARM32-LABEL: ret_8bits_inverted_shift_left24
; ARM32-NEXT: mvn r0, #-16777216
; ARM32-NEXT: bx lr
; MIPS32-LABEL: ret_8bits_inverted_shift_left24
; MIPS32-NEXT: lui	v0,0xff
; MIPS32-NEXT: ori	v0,v0,0xffff

define internal i32 @ret_8bits_inverted_ror2() {
  ret i32 1073741760
}
; CHECK-LABEL: ret_8bits_inverted_ror2
; CHECK-NEXT: mov eax,0x3fffffc0
; ARM32-LABEL: ret_8bits_inverted_ror2
; ARM32-NEXT: mvn r0, #-1073741761
; ARM32-NEXT: bx lr
; MIPS32-LABEL: ret_8bits_inverted_ror2
; MIPS32-NEXT: lui	v0,0x3fff
; MIPS32-NEXT: ori	v0,v0,0xffc0

define internal i32 @ret_8bits_inverted_ror6() {
  ret i32 67108860
}
; CHECK-LABEL: ret_8bits_inverted_ror6
; CHECK-NEXT: mov eax,0x3fffffc
; ARM32-LABEL: ret_8bits_inverted_ror6
; ARM32-NEXT: mvn r0, #-67108861
; ARM32-NEXT: bx lr
; MIPS32-LABEL: ret_8bits_inverted_ror6
; MIPS32-NEXT: lui	v0,0x3ff
; MIPS32-NEXT: ori	v0,v0,0xfffc


define internal i32 @ret_8bits_inverted_ror7() {
  ret i32 33554430
}
; CHECK-LABEL: ret_8bits_inverted_ror7
; CHECK-NEXT: mov eax,0x1fffffe
; ARM32-LABEL: ret_8bits_inverted_ror7
; ARM32-NEXT: movw r0, #65534
; ARM32-NEXT: movt r0, #511
; MIPS32-LABEL: ret_8bits_inverted_ror7
; MIPS32-NEXT: lui	v0,0x1ff
; MIPS32-NEXT: ori	v0,v0,0xfffe

; 64-bit immediates.

define internal i64 @ret_64bits_shift_left0() {
  ret i64 1095216660735
}
; CHECK-LABEL: ret_64bits_shift_left0
; CHECK-NEXT: mov eax,0xff
; CHECK-NEXT: mov edx,0xff
; ARM32-LABEL: ret_64bits_shift_left0
; ARM32-NEXT: mov r0, #255
; ARM32-NEXT: mov r1, #255
; MIPS32-LABEL: ret_64bits_shift_left0
; MIPS32-NEXT: li	v0,255
; MIPS32-NEXT: li	v1,255


; A relocatable constant is assumed to require 32-bits along with
; relocation directives.

declare void @_start()

define internal i32 @ret_addr() {
  %ptr = ptrtoint void ()* @_start to i32
  ret i32 %ptr
}
; CHECK-LABEL: ret_addr
; CHECK-NEXT: mov eax,0x0 {{.*}} R_386_32 _start
; ARM32-LABEL: ret_addr
; ARM32-NEXT: movw r0, #0 {{.*}} R_ARM_MOVW_ABS_NC _start
; ARM32-NEXT: movt r0, #0 {{.*}} R_ARM_MOVT_ABS    _start
; TODO(RKotler) emitting proper li but in disassembly
; it shows up only in the relocation records. Should emit
; without the macro but we still need to add GOT implementation
; to finish this case
;
