; This checks to ensure that Subzero aligns spill slots.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s
; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s

; The location of the stack slot for a variable is inferred from the
; return sequence.

; In this file, "global" refers to a variable with a live range across
; multiple basic blocks (not an LLVM global variable) and "local"
; refers to a variable that is live in only a single basic block.

define internal <4 x i32> @align_global_vector(i32 %arg) {
entry:
  %vec.global = insertelement <4 x i32> undef, i32 %arg, i32 0
  br label %block
block:
  call void @ForceXmmSpills()
  ret <4 x i32> %vec.global
; CHECK-LABEL: align_global_vector
; CHECK: movups xmm0,XMMWORD PTR [esp]
; CHECK-NEXT: add esp,0x1c
; CHECK-NEXT: ret
}

define internal <4 x i32> @align_local_vector(i32 %arg) {
entry:
  br label %block
block:
  %vec.local = insertelement <4 x i32> undef, i32 %arg, i32 0
  call void @ForceXmmSpills()
  ret <4 x i32> %vec.local
; CHECK-LABEL: align_local_vector
; CHECK: movups xmm0,XMMWORD PTR [esp]
; CHECK-NEXT: add esp,0x1c
; CHECK-NEXT: ret
}

declare void @ForceXmmSpills()

define internal <4 x i32> @align_global_vector_ebp_based(i32 %arg) {
entry:
  br label %eblock  ; Disable alloca optimization
eblock:
  %alloc = alloca i8, i32 1, align 1
  %vec.global = insertelement <4 x i32> undef, i32 %arg, i32 0
  br label %block
block:
  call void @ForceXmmSpillsAndUseAlloca(i8* %alloc)
  ret <4 x i32> %vec.global
; CHECK-LABEL: align_global_vector_ebp_based
; CHECK: movups xmm0,XMMWORD PTR [ebp-0x18]
; CHECK-NEXT: mov esp,ebp
; CHECK-NEXT: pop ebp
; CHECK: ret
}

define internal <4 x i32> @align_local_vector_ebp_based(i32 %arg) {
entry:
  br label %eblock  ; Disable alloca optimization
eblock:
  %alloc = alloca i8, i32 1, align 1
  %vec.local = insertelement <4 x i32> undef, i32 %arg, i32 0
  call void @ForceXmmSpillsAndUseAlloca(i8* %alloc)
  ret <4 x i32> %vec.local
; CHECK-LABEL: align_local_vector_ebp_based
; CHECK: movups xmm0,XMMWORD PTR [ebp-0x18]
; CHECK-NEXT: mov esp,ebp
; CHECK-NEXT: pop ebp
; CHECK: ret
}

define internal <4 x i32> @align_local_vector_and_global_float(i32 %arg) {
entry:
  %float.global = sitofp i32 %arg to float
  call void @ForceXmmSpillsAndUseFloat(float %float.global)
  br label %block
block:
  %vec.local = insertelement <4 x i32> undef, i32 undef, i32 0
  call void @ForceXmmSpillsAndUseFloat(float %float.global)
  ret <4 x i32> %vec.local
; CHECK-LABEL: align_local_vector_and_global_float
; CHECK: cvtsi2ss xmm0,eax
; CHECK-NEXT: movss DWORD PTR [esp+{{0x1c|0x2c}}],xmm0
; CHECK: movups xmm0,XMMWORD PTR [{{esp\+0x10|esp\+0x20}}]
; CHECK-NEXT: add esp,0x3c
; CHECK-NEXT: ret
}

declare void @ForceXmmSpillsAndUseAlloca(i8*)
declare void @ForceXmmSpillsAndUseFloat(float)
