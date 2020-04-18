; Simple tests for icmp with i8, i16, i32 operands.

; RUN: %if --need=allow_dump --need=target_MIPS32 --command %p2i \
; RUN:   --filetype=asm --target mips32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=allow_dump --need=target_MIPS32 --command FileCheck %s \
; RUN:   --check-prefix=COMMON --check-prefix=MIPS32
; RUN: %if --need=allow_dump --need=target_MIPS32 --command %p2i \
; RUN:   --filetype=asm --target mips32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=allow_dump --need=target_MIPS32 --command FileCheck %s \
; RUN:   --check-prefix=COMMON --check-prefix=MIPS32

define internal i32 @icmpEq32(i32 %a, i32 %b) {
entry:
  %cmp = icmp eq i32 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; MIPS32-LABEL: icmpEq32
; MIPS32: xor
; MIPS32: sltiu {{.*}}, {{.*}}, 1

define internal i32 @icmpNe32(i32 %a, i32 %b) {
entry:
  %cmp = icmp ne i32 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; MIPS32-LABEL: icmpNe32
; MIPS32: xor
; MIPS32: sltu {{.*}}, $zero, {{.*}}

define internal i32 @icmpSgt32(i32 %a, i32 %b) {
entry:
  %cmp = icmp sgt i32 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; MIPS32-LABEL: icmpSgt32
; MIPS32: slt

define internal i32 @icmpUgt32(i32 %a, i32 %b) {
entry:
  %cmp = icmp ugt i32 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; MIPS32-LABEL: icmpUgt32
; MIPS32: sltu

define internal i32 @icmpSge32(i32 %a, i32 %b) {
entry:
  %cmp = icmp sge i32 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; MIPS32-LABEL: icmpSge32
; MIPS32: slt
; MIPS32: xori {{.*}}, {{.*}}, 1

define internal i32 @icmpUge32(i32 %a, i32 %b) {
entry:
  %cmp = icmp uge i32 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; MIPS32-LABEL: icmpUge32
; MIPS32: sltu
; MIPS32: xori {{.*}}, {{.*}}, 1

define internal i32 @icmpSlt32(i32 %a, i32 %b) {
entry:
  %cmp = icmp slt i32 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; MIPS32-LABEL: icmpSlt32
; MIPS32: slt

define internal i32 @icmpUlt32(i32 %a, i32 %b) {
entry:
  %cmp = icmp ult i32 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; MIPS32-LABEL: icmpUlt32
; MIPS32: sltu

define internal i32 @icmpSle32(i32 %a, i32 %b) {
entry:
  %cmp = icmp sle i32 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; MIPS32-LABEL: icmpSle32
; MIPS32: slt
; MIPS32: xori {{.*}}, {{.*}}, 1

define internal i32 @icmpUle32(i32 %a, i32 %b) {
entry:
  %cmp = icmp ule i32 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; MIPS32-LABEL: icmpUle32
; MIPS32: sltu
; MIPS32: xori {{.*}}, {{.*}}, 1

define internal i32 @icmpEq8(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %icmp = icmp eq i8 %b_8, %a_8
  %ret = zext i1 %icmp to i32
  ret i32 %ret
}
; MIPS32-LABEL: icmpEq8
; MIPS32: sll {{.*}}, {{.*}}, 24
; MIPS32: sll {{.*}}, {{.*}}, 24
; MIPS32: xor
; MIPS32: sltiu	{{.*}}, {{.*}}, 1

define internal i32 @icmpSgt8(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %icmp = icmp sgt i8 %b_8, %a_8
  %ret = zext i1 %icmp to i32
  ret i32 %ret
}
; MIPS32-LABEL: icmpSgt8
; MIPS32: sll {{.*}}, {{.*}}, 24
; MIPS32: sll {{.*}}, {{.*}}, 24
; MIPS32: slt

define internal i32 @icmpUgt8(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %icmp = icmp ugt i8 %b_8, %a_8
  %ret = zext i1 %icmp to i32
  ret i32 %ret
}
; MIPS32-LABEL: icmpUgt8
; MIPS32: sll {{.*}}, {{.*}}, 24
; MIPS32: sll {{.*}}, {{.*}}, 24
; MIPS32: sltu

define internal i32 @icmpSgt16(i32 %a, i32 %b) {
entry:
  %a_16 = trunc i32 %a to i16
  %b_16 = trunc i32 %b to i16
  %icmp = icmp sgt i16 %b_16, %a_16
  %ret = zext i1 %icmp to i32
  ret i32 %ret
}
; MIPS32-LABEL: icmpSgt16
; MIPS32: sll {{.*}}, {{.*}}, 16
; MIPS32: sll {{.*}}, {{.*}}, 16
; MIPS32: slt

define internal i32 @icmpUgt16(i32 %a, i32 %b) {
entry:
  %a_16 = trunc i32 %a to i16
  %b_16 = trunc i32 %b to i16
  %icmp = icmp ugt i16 %b_16, %a_16
  %ret = zext i1 %icmp to i32
  ret i32 %ret
}
; MIPS32-LABEL: icmpUgt16
; MIPS32: sll {{.*}}, {{.*}}, 16
; MIPS32: sll {{.*}}, {{.*}}, 16
; MIPS32: sltu
