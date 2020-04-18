; This tests a switch statement, including multiple branches to the
; same label which also results in phi instructions with multiple
; entries for the same incoming edge.

; For x86 see adv-switch-opt.ll

; TODO(jvoung): Update to -02 once the phi assignments is done for ARM
; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj --disassemble \
; RUN:   --target arm32 -i %s --args -Om1 \
; RUN:   | %if --need=target_ARM32 \
; RUN:     --command FileCheck --check-prefix ARM32 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble \
; RUN:   --target mips32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:     --command FileCheck --check-prefix MIPS32 %s

define internal i32 @testSwitch(i32 %a) {
entry:
  switch i32 %a, label %sw.default [
    i32 1, label %sw.epilog
    i32 2, label %sw.epilog
    i32 3, label %sw.epilog
    i32 7, label %sw.bb1
    i32 8, label %sw.bb1
    i32 15, label %sw.bb2
    i32 14, label %sw.bb2
  ]

sw.default:                                       ; preds = %entry
  %add = add i32 %a, 27
  br label %sw.epilog

sw.bb1:                                           ; preds = %entry, %entry
  %phitmp = sub i32 21, %a
  br label %sw.bb2

sw.bb2:                                           ; preds = %sw.bb1, %entry, %entry
  %result.0 = phi i32 [ 1, %entry ], [ 1, %entry ], [ %phitmp, %sw.bb1 ]
  br label %sw.epilog

sw.epilog:                                        ; preds = %sw.bb2, %sw.default, %entry, %entry, %entry
  %result.1 = phi i32 [ %add, %sw.default ], [ %result.0, %sw.bb2 ], [ 17, %entry ], [ 17, %entry ], [ 17, %entry ]
  ret i32 %result.1
}

; MIPS32-LABEL: testSwitch
; MIPS32: li	{{.*}},1
; MIPS32: li	{{.*}},17
; MIPS32: li	{{.*}},1
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <[[SW_EPILOG:.*]]>
; MIPS32: li	{{.*}},2
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <[[SW_EPILOG]]>
; MIPS32: li	{{.*}},3
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <[[SW_EPILOG]]>
; MIPS32: li	{{.*}},7
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <[[SW_BB1:.*]]>
; MIPS32: li	{{.*}},8
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <[[SW_BB1]]>
; MIPS32: li	{{.*}},15
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <[[SW_BB2:.*]]>
; MIPS32: li	{{.*}},14
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <[[SW_BB2]]>
; MIPS32: b	{{.*}} <[[SW_DEFAULT:.*]]>
; MIPS32: <[[SW_DEFAULT]]>
; MIPS32: addiu	{{.*}},27
; MIPS32: b	{{.*}} <[[SW_EPILOG]]>
; MIPS32: <[[SW_BB1]]>
; MIPS32: li	{{.*}},21
; MIPS32: b	{{.*}} <[[SW_BB2]]>
; MIPS32: <[[SW_BB2]]>
; MIPS32: b	{{.*}} <[[SW_EPILOG]]>
; MIPS32: <[[SW_EPILOG]]>
; MIPS32: jr	ra

; Check for a valid addressing mode when the switch operand is an
; immediate.  It's important that there is exactly one case, because
; for two or more cases the source operand is legalized into a
; register.
define internal i32 @testSwitchImm() {
entry:
  switch i32 10, label %sw.default [
    i32 1, label %sw.default
  ]

sw.default:
  ret i32 20
}
; ARM32-LABEL: testSwitchImm
; ARM32: cmp {{r[0-9]+}}, #1
; ARM32-NEXT: beq
; ARM32-NEXT: b

; MIPS32-LABEL: testSwitchImm
; MIPS32: li	{{.*}},10
; MIPS32: li	{{.*}},1
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <.LtestSwitchImm$sw.default>
; MIPS32: .LtestSwitchImm$sw.default
; MIPS32: li	v0,20
; MIPS32: jr	ra

; Test for correct 64-bit lowering.
define internal i32 @testSwitch64(i64 %a) {
entry:
  switch i64 %a, label %sw.default [
    i64 123, label %return
    i64 234, label %sw.bb1
    i64 345, label %sw.bb2
    i64 78187493520, label %sw.bb3
  ]

sw.bb1:                                           ; preds = %entry
  br label %return

sw.bb2:                                           ; preds = %entry
  br label %return

sw.bb3:                                           ; preds = %entry
  br label %return

sw.default:                                       ; preds = %entry
  br label %return

return:                                           ; preds = %sw.default, %sw.bb3, %sw.bb2, %sw.bb1, %entry
  %retval.0 = phi i32 [ 5, %sw.default ], [ 4, %sw.bb3 ], [ 3, %sw.bb2 ], [ 2, %sw.bb1 ], [ 1, %entry ]
  ret i32 %retval.0
}
; ARM32-LABEL: testSwitch64
; ARM32: cmp {{r[0-9]+}}, #123
; ARM32-NEXT: cmpeq {{r[0-9]+}}, #0
; ARM32-NEXT: beq
; ARM32: cmp {{r[0-9]+}}, #234
; ARM32-NEXT: cmpeq {{r[0-9]+}}, #0
; ARM32-NEXT: beq
; ARM32: movw [[REG:r[0-9]+]], #345
; ARM32-NEXT: cmp {{r[0-9]+}}, [[REG]]
; ARM32-NEXT: cmpeq {{r[0-9]+}}, #0
; ARM32-NEXT: beq
; ARM32: movw [[REG:r[0-9]+]], #30864
; ARM32-NEXT: movt [[REG]], #13398
; ARM32-NEXT: cmp {{r[0-9]+}}, [[REG]]
; ARM32-NEXT: cmpeq {{r[0-9]+}}, #18
; ARM32-NEXT: beq
; ARM32-NEXT: b

; MIPS32-LABEL: testSwitch64
; MIPS32: bne	{{.*}},{{.*}},{{.*}} <.LtestSwitch64$local$__0>
; MIPS32: li	{{.*}},123
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <.LtestSwitch64$return>
; MIPS32: .LtestSwitch64$local$__0
; MIPS32: li	{{.*}},0
; MIPS32: bne	{{.*}},{{.*}},{{.*}} <.LtestSwitch64$local$__1>
; MIPS32: li	{{.*}},234
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <.LtestSwitch64$sw.bb1>
; MIPS32: .LtestSwitch64$local$__1
; MIPS32: li	{{.*}},0
; MIPS32: bne	{{.*}},{{.*}},{{.*}} <.LtestSwitch64$local$__2>
; MIPS32: li	{{.*}},345
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <.LtestSwitch64$sw.bb2>
; MIPS32: .LtestSwitch64$local$__2
; MIPS32: li	{{.*}},18
; MIPS32: bne	{{.*}},{{.*}},{{.*}} <.LtestSwitch64$local$__3>
; MIPS32: lui	{{.*}},0x3456
; MIPS32: ori	{{.*}},{{.*}},0x7890
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <.LtestSwitch64$sw.bb3>
; MIPS32: .LtestSwitch64$local$__3
; MIPS32: b	{{.*}} <.LtestSwitch64$sw.default>
; MIPS32: .LtestSwitch64$sw.bb1
; MIPS32: li	{{.*}},2
; MIPS32: b	{{.*}} <.LtestSwitch64$return>
; MIPS32: .LtestSwitch64$sw.bb2
; MIPS32: li	{{.*}},3
; MIPS32: b	{{.*}} <.LtestSwitch64$return>
; MIPS32: .LtestSwitch64$sw.bb3
; MIPS32: li	{{.*}},4
; MIPS32: b	{{.*}} <.LtestSwitch64$return>
; MIPS32: .LtestSwitch64$sw.default
; MIPS32: li	{{.*}},5
; MIPS32: b	{{.*}} <.LtestSwitch64$return>
; MIPS32: .LtestSwitch64$return
; MIPS32: jr	ra

; Similar to testSwitchImm, make sure proper addressing modes are
; used.  In reality, this is tested by running the output through the
; assembler.
define internal i32 @testSwitchImm64() {
entry:
  switch i64 10, label %sw.default [
    i64 1, label %sw.default
  ]

sw.default:
  ret i32 20
}
; ARM32-LABEL: testSwitchImm64
; ARM32: cmp {{r[0-9]+}}, #1
; ARM32-NEXT: cmpeq {{r[0-9]+}}, #0
; ARM32-NEXT: beq [[ADDR:[0-9a-f]+]]
; ARM32-NEXT: b [[ADDR]]

; MIPS32-LABEL: testSwitchImm64
; MIPS32: li	{{.*}},10
; MIPS32: li	{{.*}},0
; MIPS32: li	{{.*}},0
; MIPS32: bne	{{.*}},{{.*}},{{.*}} <.LtestSwitchImm64$local$__0>
; MIPS32: li	{{.*}},1
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <.LtestSwitchImm64$sw.default>
; MIPS32: .LtestSwitchImm64$local$__0
; MIPS32: b	{{.*}} <.LtestSwitchImm64$sw.default>
; MIPS32: .LtestSwitchImm64$sw.default
; MIPS32: li	{{.*}},20
; MIPS32: jr	ra

define internal i32 @testSwitchUndef64() {
entry:
  switch i64 undef, label %sw.default [
    i64 1, label %sw.default
  ]

sw.default:
  ret i32 20
}
; ARM32-LABEL: testSwitchUndef64
; ARM32: mov {{.*}}, #0
; ARM32: mov {{.*}}, #0

; MIPS32-LABEL: testSwitchUndef64
; MIPS32: li	{{.*}},0
; MIPS32: li	{{.*}},0
; MIPS32: li	{{.*}},0
; MIPS32: bne	{{.*}},{{.*}},{{.*}} <.LtestSwitchUndef64$local$__0>
; MIPS32: li	{{.*}},1
; MIPS32: beq	{{.*}},{{.*}},{{.*}} <.LtestSwitchUndef64$sw.default>
; MIPS32: .LtestSwitchUndef64$local$__0
; MIPS32: b	{{.*}} <.LtestSwitchUndef64$sw.default>
; MIPS32: .LtestSwitchUndef64$sw.default
; MIPS32: li	{{.*}},20
; MIPS32: jr	ra
