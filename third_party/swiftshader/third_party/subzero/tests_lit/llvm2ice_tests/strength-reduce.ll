; This tests various strength reduction operations.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

define internal i32 @mul_i32_arg_5(i32 %arg) {
  %result = mul i32 %arg, 5
  ret i32 %result
}
; CHECK-LABEL: mul_i32_arg_5
; CHECK: lea [[REG:e..]],{{\[}}[[REG]]+[[REG]]*4]

define internal i32 @mul_i32_5_arg(i32 %arg) {
  %result = mul i32 5, %arg
  ret i32 %result
}
; CHECK-LABEL: mul_i32_5_arg
; CHECK: lea [[REG:e..]],{{\[}}[[REG]]+[[REG]]*4]

define internal i32 @mul_i32_arg_18(i32 %arg) {
  %result = mul i32 %arg, 18
  ret i32 %result
}
; CHECK-LABEL: mul_i32_arg_18
; CHECK-DAG: lea [[REG:e..]],{{\[}}[[REG]]+[[REG]]*8]
; CHECK-DAG: shl [[REG]],1

define internal i32 @mul_i32_arg_27(i32 %arg) {
  %result = mul i32 %arg, 27
  ret i32 %result
}
; CHECK-LABEL: mul_i32_arg_27
; CHECK-DAG: lea [[REG:e..]],{{\[}}[[REG]]+[[REG]]*2]
; CHECK-DAG: lea [[REG]],{{\[}}[[REG]]+[[REG]]*8]

define internal i32 @mul_i32_arg_m45(i32 %arg) {
  %result = mul i32 %arg, -45
  ret i32 %result
}
; CHECK-LABEL: mul_i32_arg_m45
; CHECK-DAG: lea [[REG:e..]],{{\[}}[[REG]]+[[REG]]*8]
; CHECK-DAG: lea [[REG]],{{\[}}[[REG]]+[[REG]]*4]
; CHECK: neg [[REG]]

define internal i32 @mul_i16_arg_18(i32 %arg) {
  %arg.16 = trunc i32 %arg to i16
  %result = mul i16 %arg.16, 18
  %result.i32 = zext i16 %result to i32
  ret i32 %result.i32
}
; CHECK-LABEL: mul_i16_arg_18
; CHECK: imul

define internal i32 @mul_i8_arg_16(i32 %arg) {
  %arg.8 = trunc i32 %arg to i8
  %result = mul i8 %arg.8, 16
  %result.i32 = zext i8 %result to i32
  ret i32 %result.i32
}
; CHECK-LABEL: mul_i8_arg_16
; CHECK: shl {{.*}},0x4

define internal i32 @mul_i8_arg_18(i32 %arg) {
  %arg.8 = trunc i32 %arg to i8
  %result = mul i8 %arg.8, 18
  %result.i32 = zext i8 %result to i32
  ret i32 %result.i32
}
; CHECK-LABEL: mul_i8_arg_18
; CHECK: imul
