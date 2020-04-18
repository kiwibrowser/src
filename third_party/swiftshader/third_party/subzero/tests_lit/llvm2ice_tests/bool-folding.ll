; This tests the optimization where producers and consumers of i1 (bool)
; variables are combined to implicitly use flags instead of explicitly using
; stack or register variables.

; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s

; RUN: %if --need=target_ARM32 --command %p2i --filetype=obj \
; RUN:   --target arm32 -i %s --disassemble --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_ARM32 --command FileCheck %s \
; RUN:   --check-prefix=ARM32

declare void @use_value(i32)

; Basic cmp/branch folding.
define internal i32 @fold_cmp_br(i32 %arg1, i32 %arg2) {
entry:
  %cmp1 = icmp slt i32 %arg1, %arg2
  br i1 %cmp1, label %branch1, label %branch2
branch1:
  ret i32 1
branch2:
  ret i32 2
}

; CHECK-LABEL: fold_cmp_br
; CHECK: cmp
; CHECK: jge
; ARM32-LABEL: fold_cmp_br
; ARM32: cmp r0, r1
; ARM32: bge
; ARM32: mov r0, #1
; ARM32: bx lr
; ARM32: mov r0, #2
; ARM32: bx lr


; Cmp/branch folding with intervening instructions.
define internal i32 @fold_cmp_br_intervening_insts(i32 %arg1, i32 %arg2) {
entry:
  %cmp1 = icmp slt i32 %arg1, %arg2
  call void @use_value(i32 %arg1)
  br i1 %cmp1, label %branch1, label %branch2
branch1:
  ret i32 1
branch2:
  ret i32 2
}

; CHECK-LABEL: fold_cmp_br_intervening_insts
; CHECK-NOT: cmp
; CHECK: call
; CHECK: cmp
; CHECK: jge
; ARM32-LABEL: fold_cmp_br_intervening_insts
; ARM32: push {{[{].*[}]}}
; ARM32: bl{{.*}}use_value
; ARM32: cmp {{r[0-9]+}}, {{r[0-9]+}}
; ARM32: bge
; ARM32: mov r0, #1
; ARM32: bx lr
; ARM32: mov r0, #2
; ARM32: bx lr


; Cmp/branch non-folding because of live-out.
define internal i32 @no_fold_cmp_br_liveout(i32 %arg1, i32 %arg2) {
entry:
  %cmp1 = icmp slt i32 %arg1, %arg2
  br label %next
next:
  br i1 %cmp1, label %branch1, label %branch2
branch1:
  ret i32 1
branch2:
  ret i32 2
}

; CHECK-LABEL: no_fold_cmp_br_liveout
; CHECK: cmp
; CHECK: set
; CHECK: cmp
; CHECK: je
; ARM32-LABEL: no_fold_cmp_br_liveout
; ARM32: cmp
; ARM32: movlt [[REG:r[0-9]+]]
; ARM32: tst [[REG]], #1
; ARM32: beq


; Cmp/branch non-folding because of extra non-whitelisted uses.
define internal i32 @no_fold_cmp_br_non_whitelist(i32 %arg1, i32 %arg2) {
entry:
  %cmp1 = icmp slt i32 %arg1, %arg2
  %result = zext i1 %cmp1 to i32
  br i1 %cmp1, label %branch1, label %branch2
branch1:
  ret i32 %result
branch2:
  ret i32 2
}

; CHECK-LABEL: no_fold_cmp_br_non_whitelist
; CHECK: cmp
; CHECK: set
; CHECK: movzx
; CHECK: cmp
; CHECK: je
; ARM32-LABEL: no_fold_cmp_br_non_whitelist
; ARM32: mov [[R:r[0-9]+]], #0
; ARM32: cmp r0, r1
; ARM32: movlt [[R]], #1
; ARM32: tst [[R]], #1
; ARM32: beq
; ARM32: bx lr
; ARM32: mov r0, #2
; ARM32: bx lr


; Basic cmp/select folding.
define internal i32 @fold_cmp_select(i32 %arg1, i32 %arg2) {
entry:
  %cmp1 = icmp slt i32 %arg1, %arg2
  %result = select i1 %cmp1, i32 %arg1, i32 %arg2
  ret i32 %result
}

; CHECK-LABEL: fold_cmp_select
; CHECK: cmp
; CHECK: cmovl
; ARM32-LABEL: fold_cmp_select
; ARM32: cmp r0, r1
; ARM32: movlt {{r[0-9]+}}, r0

; 64-bit cmp/select folding.
define internal i64 @fold_cmp_select_64(i64 %arg1, i64 %arg2) {
entry:
  %arg1_trunc = trunc i64 %arg1 to i32
  %arg2_trunc = trunc i64 %arg2 to i32
  %cmp1 = icmp slt i32 %arg1_trunc, %arg2_trunc
  %result = select i1 %cmp1, i64 %arg1, i64 %arg2
  ret i64 %result
}

; CHECK-LABEL: fold_cmp_select_64
; CHECK: cmp
; CHECK: cmovl
; CHECK: cmovl
; ARM32-LABEL: fold_cmp_select_64
; ARM32: cmp r0, r2
; ARM32: movlt [[LOW:r[0-9]+]], r0
; ARM32: movlt [[HIGH:r[0-9]+]], r1
; ARM32: mov r0, [[LOW]]
; ARM32: mov r1, [[HIGH]]
; ARM32: bx lr


define internal i64 @fold_cmp_select_64_undef(i64 %arg1) {
entry:
  %arg1_trunc = trunc i64 %arg1 to i32
  %cmp1 = icmp slt i32 undef, %arg1_trunc
  %result = select i1 %cmp1, i64 %arg1, i64 undef
  ret i64 %result
}
; CHECK-LABEL: fold_cmp_select_64_undef
; CHECK: cmp
; CHECK: cmovl
; CHECK: cmovl
; ARM32-LABEL: fold_cmp_select_64_undef
; ARM32: mov
; ARM32: rsbs r{{[0-9]+}}, r{{[0-9]+}}, #0
; ARM32: movlt
; ARM32: movlt
; ARM32: bx lr


; Cmp/select folding with intervening instructions.
define internal i32 @fold_cmp_select_intervening_insts(i32 %arg1, i32 %arg2) {
entry:
  %cmp1 = icmp slt i32 %arg1, %arg2
  call void @use_value(i32 %arg1)
  %result = select i1 %cmp1, i32 %arg1, i32 %arg2
  ret i32 %result
}

; CHECK-LABEL: fold_cmp_select_intervening_insts
; CHECK-NOT: cmp
; CHECK: call
; CHECK: cmp
; CHECK: cmovl
; ARM32-LABEL: fold_cmp_select_intervening_insts
; ARM32: bl{{.*}}use_value
; ARM32: cmp r{{[0-9]+}}, r{{[0-9]+}}
; ARM32: movlt
; ARM32: bx lr

; Cmp/multi-select folding.
define internal i32 @fold_cmp_select_multi(i32 %arg1, i32 %arg2) {
entry:
  %cmp1 = icmp slt i32 %arg1, %arg2
  %a = select i1 %cmp1, i32 %arg1, i32 %arg2
  %b = select i1 %cmp1, i32 %arg2, i32 %arg1
  %c = select i1 %cmp1, i32 123, i32 %arg1
  %partial = add i32 %a, %b
  %result = add i32 %partial, %c
  ret i32 %result
}

; CHECK-LABEL: fold_cmp_select_multi
; CHECK: cmp
; CHECK: cmovl
; CHECK: cmp
; CHECK: cmovl
; CHECK: cmp
; CHECK: cmovge
; CHECK: add
; CHECK: add
; ARM32-LABEL: fold_cmp_select_multi
; ARM32: mov
; ARM32: cmp
; ARM32: movlt {{.*}}, #1
; ARM32: mov
; ARM32: tst {{.*}}, #1
; ARM32: movne
; ARM32: mov
; ARM32: tst {{.*}}, #1
; ARM32: movne
; ARM32: tst {{.*}}, #1
; ARM32: movne {{.*}}, #123
; ARM32: bx lr


; Cmp/multi-select non-folding because of live-out.
define internal i32 @no_fold_cmp_select_multi_liveout(i32 %arg1, i32 %arg2) {
entry:
  %cmp1 = icmp slt i32 %arg1, %arg2
  %a = select i1 %cmp1, i32 %arg1, i32 %arg2
  %b = select i1 %cmp1, i32 %arg2, i32 %arg1
  br label %next
next:
  %c = select i1 %cmp1, i32 123, i32 %arg1
  %partial = add i32 %a, %b
  %result = add i32 %partial, %c
  ret i32 %result
}

; CHECK-LABEL: no_fold_cmp_select_multi_liveout
; CHECK: set
; CHECK: cmp
; CHECK: cmovne
; CHECK: cmp
; CHECK: cmovne
; CHECK: cmp
; CHECK: cmove
; CHECK: add
; CHECK: add
; ARM32-LABEL: no_fold_cmp_select_multi_liveout
; ARM32: mov
; ARM32: cmp r0, r1
; ARM32: movlt
; ARM32: mov
; ARM32: tst
; ARM32: movne
; ARM32: mov
; ARM32: tst
; ARM32: movne
; ARM32: tst
; ARM32: movne
; ARM32: bx lr

; Cmp/branch non-folding due to load folding and intervening store.
define internal i32 @no_fold_cmp_br_store(i32 %arg2, i32 %argaddr) {
entry:
  %addr = inttoptr i32 %argaddr to i32*
  %arg1 = load i32, i32* %addr, align 1
  %cmp1 = icmp slt i32 %arg1, %arg2
  store i32 1, i32* %addr, align 1
  br i1 %cmp1, label %branch1, label %branch2
branch1:
  ret i32 1
branch2:
  ret i32 2
}

; CHECK-LABEL: no_fold_cmp_br_store
; CHECK: cmp
; CHECK: set
; CHECK: cmp

; Cmp/select non-folding due to load folding and intervening store.
define internal i32 @no_fold_cmp_select_store(i32 %arg1, i32 %argaddr) {
entry:
  %addr = inttoptr i32 %argaddr to i32*
  %arg2 = load i32, i32* %addr, align 1
  %cmp1 = icmp slt i32 %arg1, %arg2
  store i32 1, i32* %addr, align 1
  %result = select i1 %cmp1, i32 %arg1, i32 %argaddr
  ret i32 %result
}

; CHECK-LABEL: no_fold_cmp_select_store
; CHECK: cmp
; CHECK: setl
; CHECK: mov DWORD PTR
; CHECK: cmp
; CHECK: cmovne

; Cmp/select folding due to load folding and non-intervening store.
define internal i32 @fold_cmp_select_store(i32 %arg1, i32 %argaddr) {
entry:
  %addr = inttoptr i32 %argaddr to i32*
  %arg2 = load i32, i32* %addr, align 1
  %cmp1 = icmp slt i32 %arg1, %arg2
  %result = select i1 %cmp1, i32 %arg1, i32 %argaddr
  store i32 1, i32* %addr, align 1
  ret i32 %result
}

; CHECK-LABEL: fold_cmp_select_store
; CHECK: cmp {{.*}},DWORD PTR
; CHECK: cmovl

; Cmp/multi-select non-folding because of extra non-whitelisted uses.
define internal i32 @no_fold_cmp_select_multi_non_whitelist(i32 %arg1,
                                                            i32 %arg2) {
entry:
  %cmp1 = icmp slt i32 %arg1, %arg2
  %a = select i1 %cmp1, i32 %arg1, i32 %arg2
  %b = select i1 %cmp1, i32 %arg2, i32 %arg1
  %c = select i1 %cmp1, i32 123, i32 %arg1
  %ext = zext i1 %cmp1 to i32
  %partial1 = add i32 %a, %b
  %partial2 = add i32 %partial1, %c
  %result = add i32 %partial2, %ext
  ret i32 %result
}

; CHECK-LABEL: no_fold_cmp_select_multi_non_whitelist
; CHECK: set
; CHECK: cmp
; CHECK: cmovne
; CHECK: cmp
; CHECK: cmovne
; CHECK: cmp
; CHECK: cmove
; CHECK: movzx
; CHECK: add
; CHECK: add
; CHECK: add
; ARM32-LABEL: no_fold_cmp_select_multi_non_whitelist
; ARM32: mov
; ARM32: cmp r0, r1
; ARM32: movlt
; ARM32: mov
; ARM32: tst
; ARM32: movne
; ARM32: mov
; ARM32: tst
; ARM32: movne
; ARM32: tst
; ARM32: movne
; ARM32: bx lr

define internal i32 @br_i1_folding2_and(i32 %arg1, i32 %arg2) {
  %t0 = trunc i32 %arg1 to i1
  %t1 = trunc i32 %arg2 to i1

  %t2 = and i1 %t0, %t1
  br i1 %t2, label %target_true, label %target_false

target_true:
  ret i32 1

target_false:
  ret i32 0
}
; ARM32-LABEL: br_i1_folding2_and
; ARM32: tst r0, #1
; ARM32: beq
; ARM32: tst r1, #1
; ARM32: beq

define internal i32 @br_i1_folding2_or(i32 %arg1, i32 %arg2) {
  %t0 = trunc i32 %arg1 to i1
  %t1 = trunc i32 %arg2 to i1

  %t2 = or i1 %t0, %t1
  br i1 %t2, label %target_true, label %target_false

target_true:
  ret i32 1

target_false:
  ret i32 0
}
; ARM32-LABEL: br_i1_folding2_or
; ARM32: tst r0, #1
; ARM32: bne
; ARM32: tst r1, #1
; ARM32: beq

define internal i32 @br_i1_folding3_and_or(i32 %arg1, i32 %arg2, i32 %arg3) {
  %t0 = trunc i32 %arg1 to i1
  %t1 = trunc i32 %arg2 to i1
  %t2 = trunc i32 %arg3 to i1

  %t3 = and i1 %t0, %t1
  %t4 = or i1 %t3, %t2

  br i1 %t4, label %target_true, label %target_false

target_true:
  ret i32 1

target_false:
  ret i32 0
}
; ARM32-LABEL: br_i1_folding3_and_or
; ARM32: tst r0, #1
; ARM32: beq
; ARM32: tst r1, #1
; ARM32: bne
; ARM32: tst r2, #1
; ARM32: beq

define internal i32 @br_i1_folding3_or_and(i32 %arg1, i32 %arg2, i32 %arg3) {
  %t0 = trunc i32 %arg1 to i1
  %t1 = trunc i32 %arg2 to i1
  %t2 = trunc i32 %arg3 to i1

  %t3 = or i1 %t0, %t1
  %t4 = and i1 %t3, %t2

  br i1 %t4, label %target_true, label %target_false

target_true:
  ret i32 1

target_false:
  ret i32 0
}
; ARM32-LABEL: br_i1_folding3_or_and
; ARM32: tst r0, #1
; ARM32: bne
; ARM32: tst r1, #1
; ARM32: beq
; ARM32: tst r2, #1
; ARM32: beq

define internal i32 @br_i1_folding4(i32 %arg1, i32 %arg2, i32 %arg3, i32 %arg4,
                                    i32 %arg5) {
  %t0 = trunc i32 %arg1 to i1
  %t1 = trunc i32 %arg2 to i1
  %t2 = trunc i32 %arg3 to i1
  %t3 = trunc i32 %arg4 to i1
  %t4 = trunc i32 %arg5 to i1

  %t5 = or i1 %t0, %t1
  %t6 = and i1 %t5, %t2
  %t7 = and i1 %t3, %t4
  %t8 = or i1 %t6, %t7
  br i1 %t8, label %target_true, label %target_false

target_true:
  ret i32 1

target_false:
  ret i32 0
}
; ARM32-LABEL: br_i1_folding4
; ARM32: tst r0, #1
; ARM32: bne
; ARM32: tst r1, #1
; ARM32: beq
; ARM32: tst r2, #1
; ARM32: bne
; ARM32: tst     r3, #1
; ARM32: beq     [[TARGET:.*]]
; ARM32: tst     r4, #1
; ARM32: beq     [[TARGET]]
