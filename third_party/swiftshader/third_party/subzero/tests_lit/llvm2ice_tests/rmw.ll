; This tests Read-Modify-Write (RMW) detection and lowering at the O2
; optimization level.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

define internal void @rmw_add_i32_var(i32 %addr_arg, i32 %var) {
entry:
  %addr = inttoptr i32 %addr_arg to i32*
  %val = load i32, i32* %addr, align 1
  %rmw = add i32 %val, %var
  store i32 %rmw, i32* %addr, align 1
  ret void
}
; Look for something like: add DWORD PTR [eax],ecx
; CHECK-LABEL: rmw_add_i32_var
; CHECK: add DWORD PTR [e{{ax|bx|cx|dx|bp|di|si}}],e{{ax|bx|cx|dx|bp|di|si}}

define internal void @rmw_add_i32_imm(i32 %addr_arg) {
entry:
  %addr = inttoptr i32 %addr_arg to i32*
  %val = load i32, i32* %addr, align 1
  %rmw = add i32 %val, 19
  store i32 %rmw, i32* %addr, align 1
  ret void
}
; Look for something like: add DWORD PTR [eax],0x13
; CHECK-LABEL: rmw_add_i32_imm
; CHECK: add DWORD PTR [e{{ax|bx|cx|dx|bp|di|si}}],0x13

define internal i32 @no_rmw_add_i32_var(i32 %addr_arg, i32 %var) {
entry:
  %addr = inttoptr i32 %addr_arg to i32*
  %val = load i32, i32* %addr, align 1
  %rmw = add i32 %val, %var
  store i32 %rmw, i32* %addr, align 1
  ret i32 %rmw
}
; CHECK-LABEL: no_rmw_add_i32_var
; CHECK: add e{{ax|bx|cx|dx|bp|di|si}},DWORD PTR [e{{ax|bx|cx|dx|bp|di|si}}]

define internal void @rmw_add_i16_var(i32 %addr_arg, i32 %var32) {
entry:
  %var = trunc i32 %var32 to i16
  %addr = inttoptr i32 %addr_arg to i16*
  %val = load i16, i16* %addr, align 1
  %rmw = add i16 %val, %var
  store i16 %rmw, i16* %addr, align 1
  ret void
}
; Look for something like: add WORD PTR [eax],cx
; CHECK-LABEL: rmw_add_i16_var
; CHECK: add WORD PTR [e{{ax|bx|cx|dx|bp|di|si}}],{{ax|bx|cx|dx|bp|di|si}}

define internal void @rmw_add_i16_imm(i32 %addr_arg) {
entry:
  %addr = inttoptr i32 %addr_arg to i16*
  %val = load i16, i16* %addr, align 1
  %rmw = add i16 %val, 19
  store i16 %rmw, i16* %addr, align 1
  ret void
}
; Look for something like: add WORD PTR [eax],0x13
; CHECK-LABEL: rmw_add_i16_imm
; CHECK: add WORD PTR [e{{ax|bx|cx|dx|bp|di|si}}],0x13

define internal void @rmw_add_i8_var(i32 %addr_arg, i32 %var32) {
entry:
  %var = trunc i32 %var32 to i8
  %addr = inttoptr i32 %addr_arg to i8*
  %val = load i8, i8* %addr, align 1
  %rmw = add i8 %val, %var
  store i8 %rmw, i8* %addr, align 1
  ret void
}
; Look for something like: add BYTE PTR [eax],cl
; CHECK-LABEL: rmw_add_i8_var
; CHECK: add BYTE PTR [e{{ax|bx|cx|dx|bp|di|si}}],{{al|bl|cl|dl}}

define internal void @rmw_add_i8_imm(i32 %addr_arg) {
entry:
  %addr = inttoptr i32 %addr_arg to i8*
  %val = load i8, i8* %addr, align 1
  %rmw = add i8 %val, 19
  store i8 %rmw, i8* %addr, align 1
  ret void
}
; Look for something like: add BYTE PTR [eax],0x13
; CHECK-LABEL: rmw_add_i8_imm
; CHECK: add BYTE PTR [e{{ax|bx|cx|dx|bp|di|si}}],0x13

define internal void @rmw_add_i32_var_addropt(i32 %addr_arg, i32 %var) {
entry:
  %addr_arg_plus_12 = add i32 %addr_arg, 12
  %var_times_4 = mul i32 %var, 4
  %addr_base = add i32 %addr_arg_plus_12 , %var_times_4
  %addr = inttoptr i32 %addr_base to i32*
  %val = load i32, i32* %addr, align 1
  %rmw = add i32 %val, %var
  store i32 %rmw, i32* %addr, align 1
  ret void
}
; Look for something like: add DWORD PTR [eax+ecx*4+12],ecx
; CHECK-LABEL: rmw_add_i32_var_addropt
; CHECK: add DWORD PTR [e{{..}}+e{{..}}*4+0xc],e{{ax|bx|cx|dx|bp|di|si}}

; Test for commutativity opportunities.  This is the same as rmw_add_i32_var
; except with the "add" operands reversed.
define internal void @rmw_add_i32_var_comm(i32 %addr_arg, i32 %var) {
entry:
  %addr = inttoptr i32 %addr_arg to i32*
  %val = load i32, i32* %addr, align 1
  %rmw = add i32 %var, %val
  store i32 %rmw, i32* %addr, align 1
  ret void
}
; Look for something like: add DWORD PTR [eax],ecx
; CHECK-LABEL: rmw_add_i32_var_comm
; CHECK: add DWORD PTR [e{{ax|bx|cx|dx|bp|di|si}}],e{{ax|bx|cx|dx|bp|di|si}}

; Test that commutativity isn't triggered for a non-commutative arithmetic
; operator (sub).  This is the same as rmw_add_i32_var_comm except with a
; "sub" operation.
define internal i32 @no_rmw_sub_i32_var(i32 %addr_arg, i32 %var) {
entry:
  %addr = inttoptr i32 %addr_arg to i32*
  %val = load i32, i32* %addr, align 1
  %rmw = sub i32 %var, %val
  store i32 %rmw, i32* %addr, align 1
  ret i32 %rmw
}
; CHECK-LABEL: no_rmw_sub_i32_var
; CHECK: sub e{{ax|bx|cx|dx|bp|di|si}},DWORD PTR [e{{ax|bx|cx|dx|bp|di|si}}]

define internal void @rmw_add_i64_undef(i32 %addr_arg) {
entry:
  %addr = inttoptr i32 %addr_arg to i64*
  %val = load i64, i64* %addr, align 1
  %rmw = add i64 %val, undef
  store i64 %rmw, i64* %addr, align 1
  ret void
}
; CHECK-LABEL: rmw_add_i64_undef
; CHECK: add DWORD PTR [e{{ax|bx|cx|dx|bp|di|si}}],0x0
; CHECK: adc DWORD PTR [e{{ax|bx|cx|dx|bp|di|si}}+0x4],0x0
