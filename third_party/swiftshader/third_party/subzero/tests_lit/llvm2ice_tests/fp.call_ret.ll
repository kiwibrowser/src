; This tries to be a comprehensive test of f32 and f64 call/return ops.
; The CHECK lines are only checking for basic instruction patterns
; that should be present regardless of the optimization level, so
; there are no special OPTM1 match lines.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_X8632 --command FileCheck %s
; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -Om1 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; Can't test on ARM yet. Need to use several vpush {contiguous FP regs},
; instead of push {any GPR list}.

define internal i32 @doubleArgs(double %a, i32 %b, double %c) {
entry:
  ret i32 %b
}
; CHECK-LABEL: doubleArgs
; CHECK:      mov eax,DWORD PTR [esp+0xc]
; CHECK-NEXT: ret
; ARM32-LABEL: doubleArgs
; MIPS32-LABEL: doubleArgs
; MIPS32: 	move	v0,a2
; MIPS32: 	jr	ra

define internal i32 @floatArgs(float %a, i32 %b, float %c) {
entry:
  ret i32 %b
}
; CHECK-LABEL: floatArgs
; CHECK:      mov eax,DWORD PTR [esp+0x8]
; CHECK-NEXT: ret
; MIPS32-LABEL: floatArgs
; MIPS32: 	move	v0,a1
; MIPS32: 	jr	ra

define internal i32 @passFpArgs(float %a, double %b, float %c, double %d, float %e, double %f) {
entry:
  %call = call i32 @ignoreFpArgsNoInline(float %a, i32 123, double %b)
  %call1 = call i32 @ignoreFpArgsNoInline(float %c, i32 123, double %d)
  %call2 = call i32 @ignoreFpArgsNoInline(float %e, i32 123, double %f)
  %add = add i32 %call1, %call
  %add3 = add i32 %add, %call2
  ret i32 %add3
}
; CHECK-LABEL: passFpArgs
; CHECK: mov DWORD PTR [esp+0x4],0x7b
; CHECK: call {{.*}} R_{{.*}} ignoreFpArgsNoInline
; CHECK: mov DWORD PTR [esp+0x4],0x7b
; CHECK: call {{.*}} R_{{.*}} ignoreFpArgsNoInline
; CHECK: mov DWORD PTR [esp+0x4],0x7b
; CHECK: call {{.*}} R_{{.*}} ignoreFpArgsNoInline
; MIPS32-LABEL: passFpArgs
; MIPS32: 	mfc1	a2,$f{{[0-9]+}}
; MIPS32: 	mfc1	a3,$f{{[0-9]+}}
; MIPS32: 	li	a1,123
; MIPS32: 	jal	{{.*}}	ignoreFpArgsNoInline
; MIPS32: 	mfc1	a2,$f{{[0-9]+}}
; MIPS32: 	mfc1	a3,$f{{[0-9]+}}
; MIPS32: 	li	a1,123
; MIPS32: 	jal	{{.*}}	ignoreFpArgsNoInline
; MIPS32: 	mfc1	a2,$f{{[0-9]+}}
; MIPS32: 	mfc1	a3,$f{{[0-9]+}}
; MIPS32: 	li	a1,123
; MIPS32: 	jal	{{.*}}	ignoreFpArgsNoInline

declare i32 @ignoreFpArgsNoInline(float %x, i32 %y, double %z)

define internal i32 @passFpConstArg(float %a, double %b) {
entry:
  %call = call i32 @ignoreFpArgsNoInline(float %a, i32 123, double 2.340000e+00)
  ret i32 %call
}
; CHECK-LABEL: passFpConstArg
; CHECK: mov DWORD PTR [esp+0x4],0x7b
; CHECK: call {{.*}} R_{{.*}} ignoreFpArgsNoInline
; MIPS32-LABEL: passFpConstArg
; MIPS32: 	mfc1	a2,$f{{[0-9]+}}
; MIPS32: 	mfc1	a3,$f{{[0-9]+}}
; MIPS32: 	li	a1,123
; MIPS32: 	jal	{{.*}}	ignoreFpArgsNoInline

define internal i32 @passFp32ConstArg(float %a) {
entry:
  %call = call i32 @ignoreFp32ArgsNoInline(float %a, i32 123, float 2.0)
  ret i32 %call
}
; CHECK-LABEL: passFp32ConstArg
; CHECK: mov DWORD PTR [esp+0x4],0x7b
; CHECK: movss DWORD PTR [esp+0x8]
; CHECK: call {{.*}} R_{{.*}} ignoreFp32ArgsNoInline
; MIPS32-LABEL: passFp32ConstArg
; MIPS32: 	mfc1	a2,$f0
; MIPS32: 	li	a1,123
; MIPS32: 	jal	{{.*}}	ignoreFp32ArgsNoInline

declare i32 @ignoreFp32ArgsNoInline(float %x, i32 %y, float %z)

define internal float @returnFloatArg(float %a) {
entry:
  ret float %a
}
; CHECK-LABEL: returnFloatArg
; CHECK: fld DWORD PTR [esp
; MIPS32-LABEL: returnFloatArg
; MIPS32: 	mov.s	$f0,$f12
; MIPS32: 	jr	ra

define internal double @returnDoubleArg(double %a) {
entry:
  ret double %a
}
; CHECK-LABEL: returnDoubleArg
; CHECK: fld QWORD PTR [esp
; MIPS32-LABEL: returnDoubleArg
; MIPS32: 	mov.d	$f0,$f12
; MIPS32: 	jr	ra

define internal float @returnFloatConst() {
entry:
  ret float 0x3FF3AE1480000000
}
; CHECK-LABEL: returnFloatConst
; CHECK: fld
; MIPS32-LABEL: returnFloatConst
; MIPS32: 	lui	v0,0x0    {{.*}} .L$float$3f9d70a4
; MIPS32: 	lwc1	$f0,0(v0) {{.*}} .L$float$3f9d70a4
; MIPS32: 	jr	ra

define internal double @returnDoubleConst() {
entry:
  ret double 1.230000e+00
}
; CHECK-LABEL: returnDoubleConst
; CHECK: fld
; MIPS32-LABEL: returnDoubleConst
; MIPS32: 	lui	v0,0x0	   {{.*}}  .L$double$3ff3ae147ae147ae
; MIPS32: 	ldc1	$f0,0(v0)  {{.*}}  .L$double$3ff3ae147ae147ae
; MIPS32: 	jr	ra
