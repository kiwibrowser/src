; This is a smoke test of random number generator.
; The random number generators for different randomization passes should be
; decoupled. The random number used in one randomization pass should not be
; influenced by the existence of other randomization passes.

; REQUIRES: allow_dump, target_X8632

; Command for checking constant blinding (Need to turn off nop-insertion)
; RUN: %p2i --target x8632 -i %s --filetype=obj --disassemble --args -O2 \
; RUN:    -sz-seed=1 -randomize-pool-immediates=randomize \
; RUN:    -randomize-pool-threshold=0x1 \
; RUN:    -reorder-global-variables \
; RUN:    -reorder-basic-blocks \
; RUN:    -reorder-functions \
; RUN:    -randomize-regalloc \
; RUN:    -nop-insertion=0 \
; RUN:    -reorder-pooled-constants \
; RUN:    | FileCheck %s --check-prefix=BLINDINGO2

; Command for checking global variable reordering
; RUN: %p2i --target x8632 -i %s \
; RUN:    --filetype=obj --disassemble --dis-flags=-rD \
; RUN:    --args -O2 -sz-seed=1 \
; RUN:    -randomize-pool-immediates=randomize \
; RUN:    -randomize-pool-threshold=0x1 \
; RUN:    -reorder-global-variables \
; RUN:    -reorder-basic-blocks \
; RUN:    -reorder-functions \
; RUN:    -randomize-regalloc \
; RUN:    -nop-insertion \
; RUN:    -reorder-pooled-constants \
; RUN:    | FileCheck %s --check-prefix=GLOBALVARS

; Command for checking basic block reordering
; RUN: %p2i --target x8632 -i %s --filetype=asm --args -O2 -sz-seed=1\
; RUN:    -randomize-pool-immediates=randomize \
; RUN:    -randomize-pool-threshold=0x1 \
; RUN:    -reorder-global-variables \
; RUN:    -reorder-basic-blocks \
; RUN:    -reorder-functions \
; RUN:    -randomize-regalloc \
; RUN:    -nop-insertion \
; RUN:    -reorder-pooled-constants \
; RUN:    | FileCheck %s --check-prefix=BBREORDERING

; Command for checking function reordering
; RUN: %p2i --target x8632 -i %s --filetype=obj --disassemble --args -O2 \
; RUN:    -sz-seed=1 -randomize-pool-immediates=randomize \
; RUN:    -randomize-pool-threshold=0x1 \
; RUN:    -reorder-global-variables \
; RUN:    -reorder-basic-blocks \
; RUN:    -reorder-functions \
; RUN:    -randomize-regalloc \
; RUN:    -nop-insertion \
; RUN:    -reorder-pooled-constants \
; RUN:    | FileCheck %s --check-prefix=FUNCREORDERING

; Command for checking regalloc randomization
; RUN: %p2i --target x8632 -i %s --filetype=obj --disassemble --args -O2 \
; RUN:    -sz-seed=1 -randomize-pool-immediates=randomize \
; RUN:    -randomize-pool-threshold=0x1 \
; RUN:    -reorder-global-variables \
; RUN:    -reorder-basic-blocks \
; RUN:    -reorder-functions \
; RUN:    -randomize-regalloc \
; RUN:    -nop-insertion \
; RUN:    -reorder-pooled-constants \
; RUN:    -split-local-vars=0 \
; RUN:    | FileCheck %s --check-prefix=REGALLOC

; Command for checking nop insertion (Need to turn off randomize-regalloc)
; RUN: %p2i --target x8632 -i %s --filetype=asm --args \
; RUN:    -sz-seed=1 -randomize-pool-immediates=randomize \
; RUN:    -reorder-global-variables \
; RUN:    -reorder-basic-blocks \
; RUN:    -reorder-functions \
; RUN:    -randomize-regalloc=0 \
; RUN:    -nop-insertion -nop-insertion-percentage=50\
; RUN:    -reorder-pooled-constants \
; RUN:    | FileCheck %s --check-prefix=NOPINSERTION

; Command for checking pooled constants reordering
; RUN: %p2i --target x8632 -i %s --filetype=obj --disassemble --dis-flags=-s \
; RUN:    --args -O2 -sz-seed=1 \
; RUN:    -randomize-pool-immediates=randomize \
; RUN:    -randomize-pool-threshold=0x1 \
; RUN:    -reorder-global-variables \
; RUN:    -reorder-basic-blocks \
; RUN:    -reorder-functions \
; RUN:    -randomize-regalloc \
; RUN:    -nop-insertion \
; RUN:    -reorder-pooled-constants \
; RUN:    | FileCheck %s --check-prefix=POOLEDCONSTANTS


; Global variables copied from reorder-global-variables.ll
@PrimitiveInit = internal global [4 x i8] c"\1B\00\00\00", align 4
@PrimitiveInitConst = internal constant [4 x i8] c"\0D\00\00\00", align 4
@ArrayInit = internal global [20 x i8] c"\0A\00\00\00\14\00\00\00\1E\00\00\00(\00\00\002\00\00\00", align 4
@ArrayInitPartial = internal global [40 x i8] c"<\00\00\00F\00\00\00P\00\00\00Z\00\00\00d\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00", align 4
@PrimitiveInitStatic = internal global [4 x i8] zeroinitializer, align 4
@PrimitiveUninit = internal global [4 x i8] zeroinitializer, align 4
@ArrayUninit = internal global [20 x i8] zeroinitializer, align 4
@ArrayUninitConstDouble = internal constant [200 x i8] zeroinitializer, align 8
@ArrayUninitConstInt = internal constant [20 x i8] zeroinitializer, align 4


define internal <4 x i32> @func1(<4 x i32> %a, <4 x i32> %b) {
entry:
  %res = mul <4 x i32> %a, %b
  ret <4 x i32> %res

; NOPINSERTION-LABEL: func1
; NOPINSERTION: nop /* variant = 1 */
; NOPINSERTION: subl $60, %esp
; NOPINSERTION: nop /* variant = 3 */
; NOPINSERTION: movups %xmm0, 32(%esp)
; NOPINSERTION: movups %xmm1, 16(%esp)
; NOPINSERTION: movups 32(%esp), %xmm0
; NOPINSERTION: nop /* variant = 1 */
; NOPINSERTION: pshufd $49, 32(%esp), %xmm1
; NOPINSERTION: nop /* variant = 4 */
; NOPINSERTION: pshufd $49, 16(%esp), %xmm2
; NOPINSERTION: nop /* variant = 1 */
; NOPINSERTION: pmuludq 16(%esp), %xmm0
; NOPINSERTION: pmuludq %xmm2, %xmm1
; NOPINSERTION: nop /* variant = 0 */
; NOPINSERTION: shufps $136, %xmm1, %xmm0
; NOPINSERTION: nop /* variant = 3 */
; NOPINSERTION: pshufd $216, %xmm0, %xmm0
; NOPINSERTION: nop /* variant = 1 */
; NOPINSERTION: movups %xmm0, (%esp)
; NOPINSERTION: movups (%esp), %xmm0
; NOPINSERTION: addl $60, %esp
; NOPINSERTION: ret
}



define internal float @func2(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr.int = add i32 %arg.int, 200000
  %addr.ptr = inttoptr i32 %addr.int to float*
  %addr.load = load float, float* %addr.ptr, align 4
  ret float %addr.load

; BLINDINGO2-LABEL: func2
; BLINDINGO2: lea [[REG:e[a-z]*]],{{[[]}}{{e[a-z]*}}+0x69ed4ee7{{[]]}}
}

define internal float @func3(i32 %arg, float %input) {
entry:
  switch i32 %arg, label %return [
    i32 0, label %sw.bb
    i32 1, label %sw.bb1
    i32 2, label %sw.bb2
    i32 3, label %sw.bb3
    i32 4, label %sw.bb4
  ]

sw.bb:
  %rbb = fadd float %input, 1.000000e+00
  br label %return

sw.bb1:
  %rbb1 = fadd float %input, 2.000000e+00
  br label %return

sw.bb2:
  %rbb2 = fadd float %input, 4.000000e+00
  br label %return

sw.bb3:
  %rbb3 = fadd float %input, 5.000000e-01
  br label %return

sw.bb4:
  %rbb4 = fadd float %input, 2.500000e-01
  br label %return

return:
  %retval.0 = phi float [ %rbb, %sw.bb ], [ %rbb1, %sw.bb1 ], [ %rbb2, %sw.bb2 ], [ %rbb3, %sw.bb3 ], [ %rbb4, %sw.bb4], [ 0.000000e+00, %entry ]
  ret float %retval.0
}

define internal <4 x i32> @func4(<4 x i32> %a, <4 x i32> %b) {
entry:
  %res = mul <4 x i32> %a, %b
  ret <4 x i32> %res

; REGALLOC-LABEL: func4
; REGALLOC: movups  xmm3,xmm0
; REGALLOC-NEXT: pshufd  xmm0,xmm0,0x31
; REGALLOC-NEXT: pshufd  xmm4,xmm1,0x31
; REGALLOC-NEXT: pmuludq xmm3,xmm1
; REGALLOC-NEXT: pmuludq xmm0,xmm4
; REGALLOC-NEXT: shufps  xmm3,xmm0,0x88
; REGALLOC-NEXT: pshufd  xmm3,xmm3,0xd8
; REGALLOC-NEXT: movups  xmm0,xmm3
; REGALLOC-NEXT: ret
}

define internal void @func5(i32 %foo, i32 %bar) {
entry:
  %r1 = icmp eq i32 %foo, %bar
  br i1 %r1, label %BB1, label %BB2
BB1:
  %r2 = icmp sgt i32 %foo, %bar
  br i1 %r2, label %BB3, label %BB4
BB2:
  %r3 = icmp slt i32 %foo, %bar
  br i1 %r3, label %BB3, label %BB4
BB3:
  ret void
BB4:
  ret void

; BBREORDERING-LABEL: func5:
; BBREORDERING: .Lfunc5$entry:
; BBREORDERING: .Lfunc5$BB1:
; BBREORDERING: .Lfunc5$BB2:
; BBREORDERING: .Lfunc5$BB4:
; BBREORDERING: .Lfunc5$BB3
}

define internal i32 @func6(i32 %arg) {
entry:
  %res = add i32 200000, %arg
  ret i32 %res

; BLINDINGO2-LABEL: func6
; BLINDINGO2: mov [[REG:e[a-z]*]],0x77254ee7
; BLINDINGO2-NEXT: lea [[REG]],{{[[]}}[[REG]]-0x772241a7{{[]]}}
}

; Check for function reordering
; FUNCREORDERING-LABEL: func1
; FUNCREORDERING-LABEL: func4
; FUNCREORDERING-LABEL: func5
; FUNCREORDERING-LABEL: func2
; FUNCREORDERING-LABEL: func6
; FUNCREORDERING-LABEL: func3

; Check for global variable reordering
; GLOBALVARS-LABEL: ArrayInit
; GLOBALVARS-LABEL: PrimitiveInit
; GLOBALVARS-LABEL: ArrayInitPartial
; GLOBALVARS-LABEL: PrimitiveUninit
; GLOBALVARS-LABEL: ArrayUninit
; GLOBALVARS-LABEL: PrimitiveInitStatic
; GLOBALVARS-LABEL: ArrayUninitConstDouble
; GLOBALVARS-LABEL: ArrayUninitConstInt
; GLOBALVARS-LABEL: PrimitiveInitConst

; Check for pooled constant reordering
; POOLEDCONSTANTS-LABEL: .rodata.cst4
; POOLEDCONSTANTS: 0000803e 0000803f 0000003f 00008040
; POOLEDCONSTANTS: 00000040
