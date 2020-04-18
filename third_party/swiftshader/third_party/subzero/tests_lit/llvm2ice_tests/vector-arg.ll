; This file checks that Subzero generates code in accordance with the
; calling convention for vectors.

; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:   -allow-externally-defined-symbols | FileCheck --check-prefix=OPTM1 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; The first five functions test that vectors are moved from their
; correct argument location to xmm0.

define internal <4 x float> @test_returning_arg0(
    <4 x float> %arg0, <4 x float> %arg1, <4 x float> %arg2, <4 x float> %arg3,
    <4 x float> %arg4, <4 x float> %arg5) {
entry:
  ret <4 x float> %arg0
; CHECK-LABEL: test_returning_arg0
; CHECK-NOT: mov
; CHECK: ret

; OPTM1-LABEL: test_returning_arg0
; OPTM1: movups XMMWORD PTR [[LOC:.*]],xmm0
; OPTM1: movups xmm0,XMMWORD PTR [[LOC]]
; OPTM1: ret
; MIPS32-LABEL: test_returning_arg0
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	lw	v1,{{.*}}(sp)
; MIPS32: 	move	a1,a0
; MIPS32: 	sw	a2,0(a1)
; MIPS32: 	sw	a3,4(a1)
; MIPS32: 	sw	v0,8(a1)
; MIPS32: 	sw	v1,12(a1)
; MIPS32: 	move	v0,a0
}

define internal <4 x float> @test_returning_arg1(
    <4 x float> %arg0, <4 x float> %arg1, <4 x float> %arg2, <4 x float> %arg3,
    <4 x float> %arg4, <4 x float> %arg5) {
entry:
  ret <4 x float> %arg1
; CHECK-LABEL: test_returning_arg1
; CHECK: movups xmm0,xmm1
; CHECK: ret

; OPTM1-LABEL: test_returning_arg1
; OPTM1: movups XMMWORD PTR [[LOC:.*]],xmm1
; OPTM1: movups xmm0,XMMWORD PTR [[LOC]]
; OPTM1: ret
; MIPS32-LABEL: test_returning_arg1
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	lw	v1,{{.*}}(sp)
; MIPS32: 	lw	a1,{{.*}}(sp)
; MIPS32: 	lw	a2,{{.*}}(sp)
; MIPS32: 	move	a3,a0
; MIPS32: 	sw	v0,0(a3)
; MIPS32: 	sw	v1,4(a3)
; MIPS32: 	sw	a1,8(a3)
; MIPS32: 	sw	a2,12(a3)
; MIPS32: 	move	v0,a0
}

define internal <4 x float> @test_returning_arg2(
    <4 x float> %arg0, <4 x float> %arg1, <4 x float> %arg2, <4 x float> %arg3,
    <4 x float> %arg4, <4 x float> %arg5) {
entry:
  ret <4 x float> %arg2
; CHECK-LABEL: test_returning_arg2
; CHECK: movups xmm0,xmm2
; CHECK: ret

; OPTM1-LABEL: test_returning_arg2
; OPTM1: movups XMMWORD PTR [[LOC:.*]],xmm2
; OPTM1: movups xmm0,XMMWORD PTR [[LOC]]
; OPTM1: ret
; MIPS32-LABEL: test_returning_arg2
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	lw	v1,{{.*}}(sp)
; MIPS32: 	lw	a1,{{.*}}(sp)
; MIPS32: 	lw	a2,{{.*}}(sp)
; MIPS32: 	move	a3,a0
; MIPS32: 	sw	v0,0(a3)
; MIPS32: 	sw	v1,4(a3)
; MIPS32: 	sw	a1,8(a3)
; MIPS32: 	sw	a2,12(a3)
; MIPS32: 	move	v0,a0
}

define internal <4 x float> @test_returning_arg3(
    <4 x float> %arg0, <4 x float> %arg1, <4 x float> %arg2, <4 x float> %arg3,
    <4 x float> %arg4, <4 x float> %arg5) {
entry:
  ret <4 x float> %arg3
; CHECK-LABEL: test_returning_arg3
; CHECK: movups xmm0,xmm3
; CHECK: ret

; OPTM1-LABEL: test_returning_arg3
; OPTM1: movups XMMWORD PTR [[LOC:.*]],xmm3
; OPTM1: movups xmm0,XMMWORD PTR [[LOC]]
; OPTM1: ret
; MIPS32-LABEL: test_returning_arg3
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	lw	v1,{{.*}}(sp)
; MIPS32: 	lw	a1,{{.*}}(sp)
; MIPS32: 	lw	a2,{{.*}}(sp)
; MIPS32: 	move	a3,a0
; MIPS32: 	sw	v0,0(a3)
; MIPS32: 	sw	v1,4(a3)
; MIPS32: 	sw	a1,8(a3)
; MIPS32: 	sw	a2,12(a3)
; MIPS32: 	move	v0,a0
}

define internal <4 x float> @test_returning_arg4(
    <4 x float> %arg0, <4 x float> %arg1, <4 x float> %arg2, <4 x float> %arg3,
    <4 x float> %arg4, <4 x float> %arg5) {
entry:
  ret <4 x float> %arg4
; CHECK-LABEL: test_returning_arg4
; CHECK: movups xmm0,XMMWORD PTR [esp+0x4]
; CHECK: ret

; OPTM1-LABEL: test_returning_arg4
; OPTM1: movups xmm0,XMMWORD PTR {{.*}}
; OPTM1: ret
; MIPS32-LABEL: test_returning_arg4
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	lw	v1,{{.*}}(sp)
; MIPS32: 	lw	a1,{{.*}}(sp)
; MIPS32: 	lw	a2,{{.*}}(sp)
; MIPS32: 	move	a3,a0
; MIPS32: 	sw	v0,0(a3)
; MIPS32: 	sw	v1,4(a3)
; MIPS32: 	sw	a1,8(a3)
; MIPS32: 	sw	a2,12(a3)
; MIPS32: 	move	v0,a0
}

; The next five functions check that xmm arguments are handled
; correctly when interspersed with stack arguments in the argument
; list.

define internal <4 x float> @test_returning_interspersed_arg0(
    i32 %i32arg0, double %doublearg0, <4 x float> %arg0, <4 x float> %arg1,
    i32 %i32arg1, <4 x float> %arg2, double %doublearg1, <4 x float> %arg3,
    i32 %i32arg2, double %doublearg2, float %floatarg0, <4 x float> %arg4,
     <4 x float> %arg5, float %floatarg1) {
entry:
  ret <4 x float> %arg0
; CHECK-LABEL: test_returning_interspersed_arg0
; CHECK-NOT: mov
; CHECK: ret

; OPTM1-LABEL: test_returning_interspersed_arg0
; OPTM1: movups XMMWORD PTR [[LOC:.*]],xmm0
; OPTM1: movups xmm0,XMMWORD PTR [[LOC]]
; OPTM1: ret
; MIPS32-LABEL: test_returning_interspersed_arg0
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	lw	v1,{{.*}}(sp)
; MIPS32: 	lw	a1,{{.*}}(sp)
; MIPS32: 	lw	a2,{{.*}}(sp)
; MIPS32: 	move	a3,a0
; MIPS32: 	sw	v0,0(a3)
; MIPS32: 	sw	v1,4(a3)
; MIPS32: 	sw	a1,8(a3)
; MIPS32: 	sw	a2,12(a3)
; MIPS32: 	move	v0,a0
}

define internal <4 x float> @test_returning_interspersed_arg1(
    i32 %i32arg0, double %doublearg0, <4 x float> %arg0, <4 x float> %arg1,
    i32 %i32arg1, <4 x float> %arg2, double %doublearg1, <4 x float> %arg3,
    i32 %i32arg2, double %doublearg2, float %floatarg0, <4 x float> %arg4,
    <4 x float> %arg5, float %floatarg1) {
entry:
  ret <4 x float> %arg1
; CHECK-LABEL: test_returning_interspersed_arg1
; CHECK: movups xmm0,xmm1
; CHECK: ret

; OPTM1-LABEL: test_returning_interspersed_arg1
; OPTM1: movups XMMWORD PTR [[LOC:.*]],xmm1
; OPTM1: movups xmm0,XMMWORD PTR [[LOC]]
; OPTM1: ret
; MIPS32-LABEL: test_returning_interspersed_arg1
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	lw	v1,{{.*}}(sp)
; MIPS32: 	lw	a1,{{.*}}(sp)
; MIPS32: 	lw	a2,{{.*}}(sp)
; MIPS32: 	move	a3,a0
; MIPS32: 	sw	v0,0(a3)
; MIPS32: 	sw	v1,4(a3)
; MIPS32: 	sw	a1,8(a3)
; MIPS32: 	sw	a2,12(a3)
; MIPS32: 	move	v0,a0
}

define internal <4 x float> @test_returning_interspersed_arg2(
    i32 %i32arg0, double %doublearg0, <4 x float> %arg0, <4 x float> %arg1,
    i32 %i32arg1, <4 x float> %arg2, double %doublearg1, <4 x float> %arg3,
    i32 %i32arg2, double %doublearg2, float %floatarg0, <4 x float> %arg4,
    <4 x float> %arg5, float %floatarg1) {
entry:
  ret <4 x float> %arg2
; CHECK-LABEL: test_returning_interspersed_arg2
; CHECK: movups xmm0,xmm2
; CHECK: ret

; OPTM1-LABEL: test_returning_interspersed_arg2
; OPTM1: movups XMMWORD PTR [[LOC:.*]],xmm2
; OPTM1: movups xmm0,XMMWORD PTR [[LOC]]
; OPTM1: ret
; MIPS32-LABEL: test_returning_interspersed_arg2
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	lw	v1,{{.*}}(sp)
; MIPS32: 	lw	a1,{{.*}}(sp)
; MIPS32: 	lw	a2,{{.*}}(sp)
; MIPS32: 	move	a3,a0
; MIPS32: 	sw	v0,0(a3)
; MIPS32: 	sw	v1,4(a3)
; MIPS32: 	sw	a1,8(a3)
; MIPS32: 	sw	a2,12(a3)
; MIPS32: 	move	v0,a0
}

define internal <4 x float> @test_returning_interspersed_arg3(
    i32 %i32arg0, double %doublearg0, <4 x float> %arg0, <4 x float> %arg1,
    i32 %i32arg1, <4 x float> %arg2, double %doublearg1, <4 x float> %arg3,
    i32 %i32arg2, double %doublearg2, float %floatarg0, <4 x float> %arg4,
    <4 x float> %arg5, float %floatarg1) {
entry:
  ret <4 x float> %arg3
; CHECK-LABEL: test_returning_interspersed_arg3
; CHECK: movups xmm0,xmm3
; CHECK: ret

; OPTM1-LABEL: test_returning_interspersed_arg3
; OPTM1: movups XMMWORD PTR [[LOC:.*]],xmm3
; OPTM1: movups xmm0,XMMWORD PTR [[LOC]]
; OPTM1: ret
; MIPS32-LABEL: test_returning_interspersed_arg3
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	lw	v1,{{.*}}(sp)
; MIPS32: 	lw	a1,{{.*}}(sp)
; MIPS32: 	lw	a2,{{.*}}(sp)
; MIPS32: 	move	a3,a0
; MIPS32: 	sw	v0,0(a3)
; MIPS32: 	sw	v1,4(a3)
; MIPS32: 	sw	a1,8(a3)
; MIPS32: 	sw	a2,12(a3)
; MIPS32: 	move	v0,a0

}

define internal <4 x float> @test_returning_interspersed_arg4(
    i32 %i32arg0, double %doublearg0, <4 x float> %arg0, <4 x float> %arg1,
    i32 %i32arg1, <4 x float> %arg2, double %doublearg1, <4 x float> %arg3,
    i32 %i32arg2, double %doublearg2, float %floatarg0, <4 x float> %arg4,
    <4 x float> %arg5, float %floatarg1) {
entry:
  ret <4 x float> %arg4
; CHECK-LABEL: test_returning_interspersed_arg4
; CHECK: movups xmm0,XMMWORD PTR [esp+0x34]
; CHECK: ret

; OPTM1-LABEL: test_returning_interspersed_arg4
; OPTM1: movups xmm0,XMMWORD PTR {{.*}}
; OPTM1: ret
; MIPS32-LABEL: test_returning_interspersed_arg4
; MIPS32: 	lw	v0,1{{.*}}(sp)
; MIPS32: 	lw	v1,1{{.*}}(sp)
; MIPS32: 	lw	a1,1{{.*}}(sp)
; MIPS32: 	lw	a2,1{{.*}}(sp)
; MIPS32: 	move	a3,a0
; MIPS32: 	sw	v0,0(a3)
; MIPS32: 	sw	v1,4(a3)
; MIPS32: 	sw	a1,8(a3)
; MIPS32: 	sw	a2,12(a3)
; MIPS32: 	move	v0,a0
}

; Test that vectors are passed correctly as arguments to a function.

declare void @VectorArgs(<4 x float>, <4 x float>, <4 x float>, <4 x float>,
                         <4 x float>, <4 x float>)

declare void @killXmmRegisters()

define internal void @test_passing_vectors(
    <4 x float> %arg0, <4 x float> %arg1, <4 x float> %arg2, <4 x float> %arg3,
    <4 x float> %arg4, <4 x float> %arg5, <4 x float> %arg6, <4 x float> %arg7,
    <4 x float> %arg8, <4 x float> %arg9) {
entry:
  ; Kills XMM registers so that no in-arg lowering code interferes
  ; with the test.
  call void @killXmmRegisters()
  call void @VectorArgs(<4 x float> %arg9, <4 x float> %arg8, <4 x float> %arg7,
                        <4 x float> %arg6, <4 x float> %arg5, <4 x float> %arg4)
  ret void
; CHECK-LABEL: test_passing_vectors
; CHECK-NEXT: sub esp,0x2c
; CHECK: movups  [[ARG5:.*]],XMMWORD PTR [esp+0x40]
; CHECK: movups  XMMWORD PTR [esp],[[ARG5]]
; CHECK: movups  [[ARG6:.*]],XMMWORD PTR [esp+0x30]
; CHECK: movups  XMMWORD PTR [esp+0x10],[[ARG6]]
; CHECK: movups  xmm0,XMMWORD PTR [esp+0x80]
; CHECK: movups  xmm1,XMMWORD PTR [esp+0x70]
; CHECK: movups  xmm2,XMMWORD PTR [esp+0x60]
; CHECK: movups  xmm3,XMMWORD PTR [esp+0x50]
; CHECK: call {{.*}} R_{{.*}} VectorArgs
; CHECK-NEXT: add esp,0x2c

; OPTM1-LABEL: test_passing_vectors
; OPTM1: sub esp,0x6c
; OPTM1: movups  [[ARG5:.*]],XMMWORD PTR {{.*}}
; OPTM1: movups  XMMWORD PTR [esp],[[ARG5]]
; OPTM1: movups  [[ARG6:.*]],XMMWORD PTR {{.*}}
; OPTM1: movups  XMMWORD PTR [esp+0x10],[[ARG6]]
; OPTM1: movups  xmm0,XMMWORD PTR {{.*}}
; OPTM1: movups  xmm1,XMMWORD PTR {{.*}}
; OPTM1: movups  xmm2,XMMWORD PTR {{.*}}
; OPTM1: movups  xmm3,XMMWORD PTR {{.*}}
; OPTM1: call {{.*}} R_{{.*}} VectorArgs
; OPTM1-NEXT: add esp,0x6c
; MIPS32-LABEL: test_passing_vectors
; MIPS32: 	sw	s7,{{.*}}(sp)
; MIPS32: 	sw	s6,{{.*}}(sp)
; MIPS32: 	sw	s5,{{.*}}(sp)
; MIPS32: 	sw	s4,{{.*}}(sp)
; MIPS32: 	sw	s3,{{.*}}(sp)
; MIPS32: 	sw	s2,{{.*}}(sp)
; MIPS32: 	sw	s1,{{.*}}(sp)
; MIPS32: 	sw	s0,{{.*}}(sp)
; MIPS32: 	lw	s0,{{.*}}(sp)
; MIPS32: 	lw	s1,{{.*}}(sp)
; MIPS32: 	lw	s2,{{.*}}(sp)
; MIPS32: 	lw	s3,{{.*}}(sp)
; MIPS32: 	lw	s4,{{.*}}(sp)
; MIPS32: 	lw	s5,{{.*}}(sp)
; MIPS32: 	lw	s6,{{.*}}(sp)
; MIPS32: 	lw	s7,{{.*}}(sp)
; MIPS32: 	jal	0 <test_returning_arg0>	228: R_MIPS_26	killXmmRegisters
; MIPS32: 	nop
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	sw	s4,{{.*}}(sp)
; MIPS32: 	sw	s5,{{.*}}(sp)
; MIPS32: 	sw	s6,{{.*}}(sp)
; MIPS32: 	sw	s7,{{.*}}(sp)
; MIPS32: 	sw	s0,{{.*}}(sp)
; MIPS32: 	sw	s1,{{.*}}(sp)
; MIPS32: 	sw	s2,{{.*}}(sp)
; MIPS32: 	sw	s3,{{.*}}(sp)
; MIPS32: 	lw	a0,{{.*}}(sp)
; MIPS32: 	lw	a1,{{.*}}(sp)
; MIPS32: 	lw	a2,{{.*}}(sp)
; MIPS32: 	lw	a3,{{.*}}(sp)
; MIPS32: 	jal	0 <test_returning_arg0>	2c0: R_MIPS_26	VectorArgs
; MIPS32: 	nop
; MIPS32: 	lw	s0,{{.*}}(sp)
; MIPS32: 	lw	s1,{{.*}}(sp)
; MIPS32: 	lw	s2,{{.*}}(sp)
; MIPS32: 	lw	s3,{{.*}}(sp)
; MIPS32: 	lw	s4,{{.*}}(sp)
; MIPS32: 	lw	s5,{{.*}}(sp)
; MIPS32: 	lw	s6,{{.*}}(sp)
; MIPS32: 	lw	s7,{{.*}}(sp)
; MIPS32: 	lw	ra,{{.*}}(sp)

}

declare void @InterspersedVectorArgs(
    <4 x float>, i64, <4 x float>, i64, <4 x float>, float, <4 x float>,
    double, <4 x float>, i32, <4 x float>)

define internal void @test_passing_vectors_interspersed(
    <4 x float> %arg0, <4 x float> %arg1, <4 x float> %arg2, <4 x float> %arg3,
    <4 x float> %arg4, <4 x float> %arg5, <4 x float> %arg6, <4 x float> %arg7,
    <4 x float> %arg8, <4 x float> %arg9) {
entry:
  ; Kills XMM registers so that no in-arg lowering code interferes
  ; with the test.
  call void @killXmmRegisters()
  call void @InterspersedVectorArgs(<4 x float> %arg9, i64 0, <4 x float> %arg8,
                                    i64 1, <4 x float> %arg7, float 2.000000e+00,
                                    <4 x float> %arg6, double 3.000000e+00,
                                    <4 x float> %arg5, i32 4, <4 x float> %arg4)
  ret void
; CHECK-LABEL: test_passing_vectors_interspersed
; CHECK: sub esp,0x5c
; CHECK: movups  [[ARG9:.*]],XMMWORD PTR [esp+0x70]
; CHECK: movups  XMMWORD PTR [esp+0x20],[[ARG9]]
; CHECK: movups  [[ARG11:.*]],XMMWORD PTR [esp+0x60]
; CHECK: movups  XMMWORD PTR [esp+0x40],[[ARG11]]
; CHECK: movups  xmm0,XMMWORD PTR [esp+0xb0]
; CHECK: movups  xmm1,XMMWORD PTR [esp+0xa0]
; CHECK: movups  xmm2,XMMWORD PTR [esp+0x90]
; CHECK: movups  xmm3,XMMWORD PTR [esp+0x80]
; CHECK: call {{.*}} R_{{.*}} InterspersedVectorArgs
; CHECK-NEXT: add esp,0x5c
; CHECK: ret

; OPTM1-LABEL: test_passing_vectors_interspersed
; OPTM1: sub esp,0x9c
; OPTM1: movups  [[ARG9:.*]],XMMWORD PTR {{.*}}
; OPTM1: movups  XMMWORD PTR [esp+0x20],[[ARG9]]
; OPTM1: movups  [[ARG11:.*]],XMMWORD PTR {{.*}}
; OPTM1: movups  XMMWORD PTR [esp+0x40],[[ARG11]]
; OPTM1: movups  xmm0,XMMWORD PTR {{.*}}
; OPTM1: movups  xmm1,XMMWORD PTR {{.*}}
; OPTM1: movups  xmm2,XMMWORD PTR {{.*}}
; OPTM1: movups  xmm3,XMMWORD PTR {{.*}}
; OPTM1: call {{.*}} R_{{.*}} InterspersedVectorArgs
; OPTM1-NEXT: add esp,0x9c
; OPTM1: ret
; MIPS32-LABEL: test_passing_vectors_interspersed
; MIPS32: 	sw	s7,{{.*}}(sp)
; MIPS32: 	sw	s6,{{.*}}(sp)
; MIPS32: 	sw	s5,{{.*}}(sp)
; MIPS32: 	sw	s4,{{.*}}(sp)
; MIPS32: 	sw	s3,{{.*}}(sp)
; MIPS32: 	sw	s2,{{.*}}(sp)
; MIPS32: 	sw	s1,{{.*}}(sp)
; MIPS32: 	sw	s0,{{.*}}(sp)
; MIPS32: 	lw	s0,{{.*}}(sp)
; MIPS32: 	lw	s1,{{.*}}(sp)
; MIPS32: 	lw	s2,{{.*}}(sp)
; MIPS32: 	lw	s3,{{.*}}(sp)
; MIPS32: 	lw	s4,{{.*}}(sp)
; MIPS32: 	lw	s5,{{.*}}(sp)
; MIPS32: 	lw	s6,{{.*}}(sp)
; MIPS32: 	lw	s7,{{.*}}(sp)
; MIPS32: 	jal	0 <test_returning_arg0>	348: R_MIPS_26	killXmmRegisters
; MIPS32: 	nop
; MIPS32: 	li	v0,0
; MIPS32: 	li	v1,0
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	sw	v1,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	li	v0,0
; MIPS32: 	li	v1,1
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	sw	v1,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lui	v0,0x0	    3b0: R_MIPS_HI16	.L$float$40000000
; MIPS32: 	lwc1	$f0,0(v0)   3b4: R_MIPS_LO16	.L$float$40000000
; MIPS32: 	swc1	$f0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lw	v0,{{.*}}(sp)
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	lui	v0,0x0	    3dc: R_MIPS_HI16  .L$double$4008000000000000
; MIPS32: 	ldc1	$f0,0(v0)   3e0: R_MIPS_LO16  .L$double$4008000000000000
; MIPS32: 	sdc1	$f0,{{.*}}(sp)
; MIPS32: 	sw	s4,{{.*}}(sp)
; MIPS32: 	sw	s5,{{.*}}(sp)
; MIPS32: 	sw	s6,{{.*}}(sp)
; MIPS32: 	sw	s7,{{.*}}(sp)
; MIPS32: 	li	v0,4
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	sw	s0,{{.*}}(sp)
; MIPS32: 	sw	s1,{{.*}}(sp)
; MIPS32: 	sw	s2,{{.*}}(sp)
; MIPS32: 	sw	s3,{{.*}}(sp)
; MIPS32: 	lw	a0,{{.*}}(sp)
; MIPS32: 	lw	a1,{{.*}}(sp)
; MIPS32: 	lw	a2,{{.*}}(sp)
; MIPS32: 	lw	a3,{{.*}}(sp)
; MIPS32: 	jal	0 <test_returning_arg0>	420: R_MIPS_26	InterspersedVectorArgs
; MIPS32: 	nop
; MIPS32: 	lw	s0,{{.*}}(sp)
; MIPS32: 	lw	s1,{{.*}}(sp)
; MIPS32: 	lw	s2,{{.*}}(sp)
; MIPS32: 	lw	s3,{{.*}}(sp)
; MIPS32: 	lw	s4,{{.*}}(sp)
; MIPS32: 	lw	s5,{{.*}}(sp)
; MIPS32: 	lw	s6,{{.*}}(sp)
; MIPS32: 	lw	s7,{{.*}}(sp)
; MIPS32: 	lw	ra,{{.*}}(sp)
}

; Test that a vector returned from a function is recognized to be in
; xmm0.

declare <4 x float> @VectorReturn(<4 x float> %arg0)

define internal void @test_receiving_vectors(<4 x float> %arg0) {
entry:
  %result = call <4 x float> @VectorReturn(<4 x float> %arg0)
  %result2 = call <4 x float> @VectorReturn(<4 x float> %result)
  ret void
; CHECK-LABEL: test_receiving_vectors
; CHECK: call {{.*}} R_{{.*}} VectorReturn
; CHECK-NOT: movups xmm0
; CHECK: call {{.*}} R_{{.*}} VectorReturn
; CHECK: ret

; OPTM1-LABEL: test_receiving_vectors
; OPTM1: call {{.*}} R_{{.*}} VectorReturn
; OPTM1: movups {{.*}},xmm0
; OPTM1: movups xmm0,{{.*}}
; OPTM1: call {{.*}} R_{{.*}} VectorReturn
; OPTM1: ret
; MIPS32-LABEL: test_receiving_vectors
; MIPS32: 	sw	s8,{{.*}}(sp)
; MIPS32: 	sw	s0,{{.*}}(sp)
; MIPS32: 	move	s8,sp
; MIPS32: 	move	v0,a0
; MIPS32: 	addiu	v1,sp,32
; MIPS32: 	move	s0,v1
; MIPS32: 	move	a0,s0
; MIPS32: 	sw	a2,{{.*}}(sp)
; MIPS32: 	sw	a3,{{.*}}(sp)
; MIPS32: 	move	a2,v0
; MIPS32: 	move	a3,a1
; MIPS32: 	jal	0 <test_returning_arg0>	{{.*}} R_MIPS_26	VectorReturn
; MIPS32: 	nop
; MIPS32: 	lw	v0,0(s0)
; MIPS32: 	lw	v1,4(s0)
; MIPS32: 	lw	a0,8(s0)
; MIPS32: 	move	a1,a0
; MIPS32: 	lw	s0,12(s0)
; MIPS32: 	addiu	a0,sp,48
; MIPS32: 	sw	a1,{{.*}}(sp)
; MIPS32: 	sw	s0,{{.*}}(sp)
; MIPS32: 	move	a2,v0
; MIPS32: 	move	a3,v1
; MIPS32: 	jal	0 <test_returning_arg0>	{{.*}} R_MIPS_26	VectorReturn
; MIPS32: 	nop
; MIPS32: 	move	sp,s8
; MIPS32: 	lw	s0,{{.*}}(sp)
; MIPS32: 	lw	s8,{{.*}}(sp)
; MIPS32: 	lw	ra,{{.*}}(sp)
}
