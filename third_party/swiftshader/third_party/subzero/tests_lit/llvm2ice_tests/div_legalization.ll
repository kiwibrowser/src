; This is a regression test that idiv and div operands are legalized
; (they cannot be constants and can only be reg/mem for x86).

; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 | FileCheck %s
; RUN: %p2i --filetype=obj --disassemble -i %s --args -Om1 | FileCheck %s

define internal i32 @Sdiv_const8_b(i32 %a32) {
; CHECK-LABEL: Sdiv_const8_b
entry:
  %a = trunc i32 %a32 to i8
  %div = sdiv i8 %a, 12
; CHECK: mov {{.*}},0xc
; CHECK-NOT: idiv 0xc
  %div_ext = sext i8 %div to i32
  ret i32 %div_ext
}

define internal i32 @Sdiv_const16_b(i32 %a32) {
; CHECK-LABEL: Sdiv_const16_b
entry:
  %a = trunc i32 %a32 to i16
  %div = sdiv i16 %a, 1234
; CHECK: mov {{.*}},0x4d2
; CHECK-NOT: idiv 0x4d2
  %div_ext = sext i16 %div to i32
  ret i32 %div_ext
}

define internal i32 @Sdiv_const32_b(i32 %a) {
; CHECK-LABEL: Sdiv_const32_b
entry:
  %div = sdiv i32 %a, 1234
; CHECK: mov {{.*}},0x4d2
; CHECK-NOT: idiv 0x4d2
  ret i32 %div
}

define internal i32 @Srem_const_b(i32 %a) {
; CHECK-LABEL: Srem_const_b
entry:
  %rem = srem i32 %a, 2345
; CHECK: mov {{.*}},0x929
; CHECK-NOT: idiv 0x929
  ret i32 %rem
}

define internal i32 @Udiv_const_b(i32 %a) {
; CHECK-LABEL: Udiv_const_b
entry:
  %div = udiv i32 %a, 3456
; CHECK: mov {{.*}},0xd80
; CHECK-NOT: div 0xd80
  ret i32 %div
}

define internal i32 @Urem_const_b(i32 %a) {
; CHECK-LABEL: Urem_const_b
entry:
  %rem = urem i32 %a, 4567
; CHECK: mov {{.*}},0x11d7
; CHECK-NOT: div 0x11d7
  ret i32 %rem
}
