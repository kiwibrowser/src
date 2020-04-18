; This is a smoke test of randomized register allocation.  The output
; of this test will change with changes to the random number generator
; implementation.

; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 -sz-seed=1 \
; RUN:   -randomize-regalloc -split-local-vars=0 \
; RUN:   | FileCheck %s --check-prefix=CHECK_1
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 -sz-seed=1 \
; RUN:   -randomize-regalloc \
; RUN:   | FileCheck %s --check-prefix=OPTM1_1

; Same tests but with a different seed, just to verify randomness.
; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 -sz-seed=123 \
; RUN:   -randomize-regalloc -split-local-vars=0 \
; RUN:   | FileCheck %s --check-prefix=CHECK_123
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 -sz-seed=123 \
; RUN:   -randomize-regalloc \
; RUN:   | FileCheck %s --check-prefix=OPTM1_123

define internal <4 x i32> @mul_v4i32(<4 x i32> %a, <4 x i32> %b) {
entry:
  %res = mul <4 x i32> %a, %b
  ret <4 x i32> %res
; OPTM1_1-LABEL: mul_v4i32
; OPTM1_1: sub     esp,0x3c
; OPTM1_1-NEXT: movups  XMMWORD PTR [esp+0x20],xmm0
; OPTM1_1-NEXT: movups  XMMWORD PTR [esp+0x10],xmm1
; OPTM1_1-NEXT: movups  xmm0,XMMWORD PTR [esp+0x20]
; OPTM1_1-NEXT: pshufd  xmm6,XMMWORD PTR [esp+0x20],0x31
; OPTM1_1-NEXT: pshufd  xmm2,XMMWORD PTR [esp+0x10],0x31
; OPTM1_1-NEXT: pmuludq xmm0,XMMWORD PTR [esp+0x10]
; OPTM1_1-NEXT: pmuludq xmm6,xmm2
; OPTM1_1-NEXT: shufps  xmm0,xmm6,0x88
; OPTM1_1-NEXT: pshufd  xmm0,xmm0,0xd8
; OPTM1_1-NEXT: movups  XMMWORD PTR [esp],xmm0
; OPTM1_1-NEXT: movups  xmm0,XMMWORD PTR [esp]
; OPTM1_1-NEXT: add     esp,0x3c
; OPTM1_1-NEXT: ret

; CHECK_1-LABEL: mul_v4i32
; CHECK_1: movups  xmm7,xmm0
; CHECK_1-NEXT: pshufd  xmm0,xmm0,0x31
; CHECK_1-NEXT: pshufd  xmm5,xmm1,0x31
; CHECK_1-NEXT: pmuludq xmm7,xmm1
; CHECK_1-NEXT: pmuludq xmm0,xmm5
; CHECK_1-NEXT: shufps  xmm7,xmm0,0x88
; CHECK_1-NEXT: pshufd  xmm7,xmm7,0xd8
; CHECK_1-NEXT: movups  xmm0,xmm7
; CHECK_1-NEXT: ret

; OPTM1_123-LABEL: mul_v4i32
; OPTM1_123: sub     esp,0x3c
; OPTM1_123-NEXT: movups  XMMWORD PTR [esp+0x20],xmm0
; OPTM1_123-NEXT: movups  XMMWORD PTR [esp+0x10],xmm1
; OPTM1_123-NEXT: movups  xmm0,XMMWORD PTR [esp+0x20]
; OPTM1_123-NEXT: pshufd  xmm6,XMMWORD PTR [esp+0x20],0x31
; OPTM1_123-NEXT: pshufd  xmm2,XMMWORD PTR [esp+0x10],0x31
; OPTM1_123-NEXT: pmuludq xmm0,XMMWORD PTR [esp+0x10]
; OPTM1_123-NEXT: pmuludq xmm6,xmm2
; OPTM1_123-NEXT: shufps  xmm0,xmm6,0x88
; OPTM1_123-NEXT: pshufd  xmm0,xmm0,0xd8
; OPTM1_123-NEXT: movups  XMMWORD PTR [esp],xmm0
; OPTM1_123-NEXT: movups  xmm0,XMMWORD PTR [esp]
; OPTM1_123-NEXT: add     esp,0x3c
; OPTM1_123-NEXT: ret

; CHECK_123-LABEL: mul_v4i32
; CHECK_123: movups  xmm5,xmm0
; CHECK_123-NEXT: pshufd  xmm0,xmm0,0x31
; CHECK_123-NEXT: pshufd  xmm7,xmm1,0x31
; CHECK_123-NEXT: pmuludq xmm5,xmm1
; CHECK_123-NEXT: pmuludq xmm0,xmm7
; CHECK_123-NEXT: shufps  xmm5,xmm0,0x88
; CHECK_123-NEXT: pshufd  xmm5,xmm5,0xd8
; CHECK_123-NEXT: movups  xmm0,xmm5
; CHECK_123-NEXT: ret
}

; ERRORS-NOT: ICE translation error
; DUMP-NOT: SZ
