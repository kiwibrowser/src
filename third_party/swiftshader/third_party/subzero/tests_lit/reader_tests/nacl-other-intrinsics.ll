; This tests parsing NaCl intrinsics not related to atomic operations.

; RUN: %p2i -i %s --insts --args -allow-externally-defined-symbols \
; RUN: | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing \
; RUN:        -allow-externally-defined-symbols | \
; RUN:   FileCheck --check-prefix=NOIR %s

declare i8* @llvm.nacl.read.tp()
declare void @llvm.memcpy.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1)
declare void @llvm.memmove.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1)
declare void @llvm.memset.p0i8.i32(i8*, i8, i32, i32, i1)
declare void @llvm.nacl.longjmp(i8*, i32)
declare i32 @llvm.nacl.setjmp(i8*)
declare float @llvm.sqrt.f32(float)
declare double @llvm.sqrt.f64(double)
declare float @llvm.fabs.f32(float)
declare double @llvm.fabs.f64(double)
declare <4 x float> @llvm.fabs.v4f32(<4 x float>)
declare void @llvm.trap()
declare i16 @llvm.bswap.i16(i16)
declare i32 @llvm.bswap.i32(i32)
declare i64 @llvm.bswap.i64(i64)
declare i32 @llvm.ctlz.i32(i32, i1)
declare i64 @llvm.ctlz.i64(i64, i1)
declare i32 @llvm.cttz.i32(i32, i1)
declare i64 @llvm.cttz.i64(i64, i1)
declare i32 @llvm.ctpop.i32(i32)
declare i64 @llvm.ctpop.i64(i64)
declare i8* @llvm.stacksave()
declare void @llvm.stackrestore(i8*)

define internal i32 @test_nacl_read_tp() {
entry:
  %ptr = call i8* @llvm.nacl.read.tp()
  %__1 = ptrtoint i8* %ptr to i32
  ret i32 %__1
}

; CHECK:      define internal i32 @test_nacl_read_tp() {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %ptr = call i32 @llvm.nacl.read.tp()
; CHECK-NEXT:   ret i32 %ptr
; CHECK-NEXT: }

define internal void @test_memcpy(i32 %iptr_dst, i32 %iptr_src, i32 %len) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 %len, i32 1, i1 false)
  ret void
}

; CHECK-NEXT: define internal void @test_memcpy(i32 %iptr_dst, i32 %iptr_src, i32 %len) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   call void @llvm.memcpy.p0i8.p0i8.i32(i32 %iptr_dst, i32 %iptr_src, i32 %len, i32 1, i1 false)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @test_memmove(i32 %iptr_dst, i32 %iptr_src, i32 %len) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memmove.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                        i32 %len, i32 1, i1 false)
  ret void
}

; CHECK-NEXT: define internal void @test_memmove(i32 %iptr_dst, i32 %iptr_src, i32 %len) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   call void @llvm.memmove.p0i8.p0i8.i32(i32 %iptr_dst, i32 %iptr_src, i32 %len, i32 1, i1 false)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @test_memset(i32 %iptr_dst, i32 %wide_val, i32 %len) {
entry:
  %val = trunc i32 %wide_val to i8
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 %val,
                                  i32 %len, i32 1, i1 false)
  ret void
}

; CHECK-NEXT: define internal void @test_memset(i32 %iptr_dst, i32 %wide_val, i32 %len) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %val = trunc i32 %wide_val to i8
; CHECK-NEXT:   call void @llvm.memset.p0i8.i32(i32 %iptr_dst, i8 %val, i32 %len, i32 1, i1 false)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal i32 @test_setjmplongjmp(i32 %iptr_env) {
entry:
  %env = inttoptr i32 %iptr_env to i8*
  %i = call i32 @llvm.nacl.setjmp(i8* %env)
  %r1 = icmp eq i32 %i, 0
  br i1 %r1, label %Zero, label %NonZero
Zero:
  ; Redundant inttoptr, to make --pnacl cast-eliding/re-insertion happy.
  %env2 = inttoptr i32 %iptr_env to i8*
  call void @llvm.nacl.longjmp(i8* %env2, i32 1)
  ret i32 0
NonZero:
  ret i32 1
}

; CHECK-NEXT: define internal i32 @test_setjmplongjmp(i32 %iptr_env) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %i = call i32 @llvm.nacl.setjmp(i32 %iptr_env)
; CHECK-NEXT:   %r1 = icmp eq i32 %i, 0
; CHECK-NEXT:   br i1 %r1, label %Zero, label %NonZero
; CHECK-NEXT: Zero:
; CHECK-NEXT:   call void @llvm.nacl.longjmp(i32 %iptr_env, i32 1)
; CHECK-NEXT:   ret i32 0
; CHECK-NEXT: NonZero:
; CHECK-NEXT:   ret i32 1
; CHECK-NEXT: }

define internal float @test_sqrt_float(float %x, i32 %iptr) {
entry:
  %r = call float @llvm.sqrt.f32(float %x)
  %r2 = call float @llvm.sqrt.f32(float %r)
  %r3 = call float @llvm.sqrt.f32(float -0.0)
  %r4 = fadd float %r2, %r3
  ret float %r4
}

; CHECK-NEXT: define internal float @test_sqrt_float(float %x, i32 %iptr) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call float @llvm.sqrt.f32(float %x)
; CHECK-NEXT:   %r2 = call float @llvm.sqrt.f32(float %r)
; CHECK-NEXT:   %r3 = call float @llvm.sqrt.f32(float -0.000000e+00)
; CHECK-NEXT:   %r4 = fadd float %r2, %r3
; CHECK-NEXT:   ret float %r4
; CHECK-NEXT: }

define internal double @test_sqrt_double(double %x, i32 %iptr) {
entry:
  %r = call double @llvm.sqrt.f64(double %x)
  %r2 = call double @llvm.sqrt.f64(double %r)
  %r3 = call double @llvm.sqrt.f64(double -0.0)
  %r4 = fadd double %r2, %r3
  ret double %r4
}

; CHECK-NEXT: define internal double @test_sqrt_double(double %x, i32 %iptr) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call double @llvm.sqrt.f64(double %x)
; CHECK-NEXT:   %r2 = call double @llvm.sqrt.f64(double %r)
; CHECK-NEXT:   %r3 = call double @llvm.sqrt.f64(double -0.000000e+00)
; CHECK-NEXT:   %r4 = fadd double %r2, %r3
; CHECK-NEXT:   ret double %r4
; CHECK-NEXT: }

define internal float @test_fabs_float(float %x) {
entry:
  %r = call float @llvm.fabs.f32(float %x)
  %r2 = call float @llvm.fabs.f32(float %r)
  %r3 = call float @llvm.fabs.f32(float -0.0)
  %r4 = fadd float %r2, %r3
  ret float %r4
}

; CHECK-NEXT: define internal float @test_fabs_float(float %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call float @llvm.fabs.f32(float %x)
; CHECK-NEXT:   %r2 = call float @llvm.fabs.f32(float %r)
; CHECK-NEXT:   %r3 = call float @llvm.fabs.f32(float -0.000000e+00)
; CHECK-NEXT:   %r4 = fadd float %r2, %r3
; CHECK-NEXT:   ret float %r4
; CHECK-NEXT: }

define internal double @test_fabs_double(double %x) {
entry:
  %r = call double @llvm.fabs.f64(double %x)
  %r2 = call double @llvm.fabs.f64(double %r)
  %r3 = call double @llvm.fabs.f64(double -0.0)
  %r4 = fadd double %r2, %r3
  ret double %r4
}

; CHECK-NEXT: define internal double @test_fabs_double(double %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call double @llvm.fabs.f64(double %x)
; CHECK-NEXT:   %r2 = call double @llvm.fabs.f64(double %r)
; CHECK-NEXT:   %r3 = call double @llvm.fabs.f64(double -0.000000e+00)
; CHECK-NEXT:   %r4 = fadd double %r2, %r3
; CHECK-NEXT:   ret double %r4
; CHECK-NEXT: }

define internal <4 x float> @test_fabs_v4f32(<4 x float> %x) {
entry:
  %r = call <4 x float> @llvm.fabs.v4f32(<4 x float> %x)
  %r2 = call <4 x float> @llvm.fabs.v4f32(<4 x float> %r)
  %r3 = call <4 x float> @llvm.fabs.v4f32(<4 x float> undef)
  %r4 = fadd <4 x float> %r2, %r3
  ret <4 x float> %r4
}

; CHECK-NEXT: define internal <4 x float> @test_fabs_v4f32(<4 x float> %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call <4 x float> @llvm.fabs.v4f32(<4 x float> %x)
; CHECK-NEXT:   %r2 = call <4 x float> @llvm.fabs.v4f32(<4 x float> %r)
; CHECK-NEXT:   %r3 = call <4 x float> @llvm.fabs.v4f32(<4 x float> undef)
; CHECK-NEXT:   %r4 = fadd <4 x float> %r2, %r3
; CHECK-NEXT:   ret <4 x float> %r4
; CHECK-NEXT: }

define internal i32 @test_trap(i32 %br) {
entry:
  %r1 = icmp eq i32 %br, 0
  br i1 %r1, label %Zero, label %NonZero
Zero:
  call void @llvm.trap()
  unreachable
NonZero:
  ret i32 1
}

; CHECK-NEXT: define internal i32 @test_trap(i32 %br) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r1 = icmp eq i32 %br, 0
; CHECK-NEXT:   br i1 %r1, label %Zero, label %NonZero
; CHECK-NEXT: Zero:
; CHECK-NEXT:   call void @llvm.trap()
; CHECK-NEXT:   unreachable
; CHECK-NEXT: NonZero:
; CHECK-NEXT:   ret i32 1
; CHECK-NEXT: }

define internal i32 @test_bswap_16(i32 %x) {
entry:
  %x_trunc = trunc i32 %x to i16
  %r = call i16 @llvm.bswap.i16(i16 %x_trunc)
  %r_zext = zext i16 %r to i32
  ret i32 %r_zext
}

; CHECK-NEXT: define internal i32 @test_bswap_16(i32 %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %x_trunc = trunc i32 %x to i16
; CHECK-NEXT:   %r = call i16 @llvm.bswap.i16(i16 %x_trunc)
; CHECK-NEXT:   %r_zext = zext i16 %r to i32
; CHECK-NEXT:   ret i32 %r_zext
; CHECK-NEXT: }

define internal i32 @test_bswap_32(i32 %x) {
entry:
  %r = call i32 @llvm.bswap.i32(i32 %x)
  ret i32 %r
}

; CHECK-NEXT: define internal i32 @test_bswap_32(i32 %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call i32 @llvm.bswap.i32(i32 %x)
; CHECK-NEXT:   ret i32 %r
; CHECK-NEXT: }

define internal i64 @test_bswap_64(i64 %x) {
entry:
  %r = call i64 @llvm.bswap.i64(i64 %x)
  ret i64 %r
}

; CHECK-NEXT: define internal i64 @test_bswap_64(i64 %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call i64 @llvm.bswap.i64(i64 %x)
; CHECK-NEXT:   ret i64 %r
; CHECK-NEXT: }

define internal i32 @test_ctlz_32(i32 %x) {
entry:
  %r = call i32 @llvm.ctlz.i32(i32 %x, i1 false)
  ret i32 %r
}

; CHECK-NEXT: define internal i32 @test_ctlz_32(i32 %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call i32 @llvm.ctlz.i32(i32 %x, i1 false)
; CHECK-NEXT:   ret i32 %r
; CHECK-NEXT: }

define internal i64 @test_ctlz_64(i64 %x) {
entry:
  %r = call i64 @llvm.ctlz.i64(i64 %x, i1 false)
  ret i64 %r
}

; CHECK-NEXT: define internal i64 @test_ctlz_64(i64 %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call i64 @llvm.ctlz.i64(i64 %x, i1 false)
; CHECK-NEXT:   ret i64 %r
; CHECK-NEXT: }

define internal i32 @test_cttz_32(i32 %x) {
entry:
  %r = call i32 @llvm.cttz.i32(i32 %x, i1 false)
  ret i32 %r
}

; CHECK-NEXT: define internal i32 @test_cttz_32(i32 %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call i32 @llvm.cttz.i32(i32 %x, i1 false)
; CHECK-NEXT:   ret i32 %r
; CHECK-NEXT: }

define internal i64 @test_cttz_64(i64 %x) {
entry:
  %r = call i64 @llvm.cttz.i64(i64 %x, i1 false)
  ret i64 %r
}

; CHECK-NEXT: define internal i64 @test_cttz_64(i64 %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call i64 @llvm.cttz.i64(i64 %x, i1 false)
; CHECK-NEXT:   ret i64 %r
; CHECK-NEXT: }

define internal i32 @test_popcount_32(i32 %x) {
entry:
  %r = call i32 @llvm.ctpop.i32(i32 %x)
  ret i32 %r
}

; CHECK-NEXT: define internal i32 @test_popcount_32(i32 %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call i32 @llvm.ctpop.i32(i32 %x)
; CHECK-NEXT:   ret i32 %r
; CHECK-NEXT: }

define internal i64 @test_popcount_64(i64 %x) {
entry:
  %r = call i64 @llvm.ctpop.i64(i64 %x)
  ret i64 %r
}

; CHECK-NEXT: define internal i64 @test_popcount_64(i64 %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call i64 @llvm.ctpop.i64(i64 %x)
; CHECK-NEXT:   ret i64 %r
; CHECK-NEXT: }

define internal void @test_stacksave_noalloca() {
entry:
  %sp = call i8* @llvm.stacksave()
  call void @llvm.stackrestore(i8* %sp)
  ret void
}

; CHECK-NEXT: define internal void @test_stacksave_noalloca() {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %sp = call i32 @llvm.stacksave()
; CHECK-NEXT:   call void @llvm.stackrestore(i32 %sp)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

declare i32 @foo(i32 %x)

define internal void @test_stacksave_multiple(i32 %x) {
entry:
  %x_4 = mul i32 %x, 4
  %sp1 = call i8* @llvm.stacksave()
  %tmp1 = alloca i8, i32 %x_4, align 4

  %sp2 = call i8* @llvm.stacksave()
  %tmp2 = alloca i8, i32 %x_4, align 4

  %y = call i32 @foo(i32 %x)

  %sp3 = call i8* @llvm.stacksave()
  %tmp3 = alloca i8, i32 %x_4, align 4

  %__9 = bitcast i8* %tmp1 to i32*
  store i32 %y, i32* %__9, align 1

  %__10 = bitcast i8* %tmp2 to i32*
  store i32 %x, i32* %__10, align 1

  %__11 = bitcast i8* %tmp3 to i32*
  store i32 %x, i32* %__11, align 1

  call void @llvm.stackrestore(i8* %sp1)
  ret void
}

; CHECK-NEXT: define internal void @test_stacksave_multiple(i32 %x) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %x_4 = mul i32 %x, 4
; CHECK-NEXT:   %sp1 = call i32 @llvm.stacksave()
; CHECK-NEXT:   %tmp1 = alloca i8, i32 %x_4, align 4
; CHECK-NEXT:   %sp2 = call i32 @llvm.stacksave()
; CHECK-NEXT:   %tmp2 = alloca i8, i32 %x_4, align 4
; CHECK-NEXT:   %y = call i32 @foo(i32 %x)
; CHECK-NEXT:   %sp3 = call i32 @llvm.stacksave()
; CHECK-NEXT:   %tmp3 = alloca i8, i32 %x_4, align 4
; CHECK-NEXT:   store i32 %y, i32* %tmp1, align 1
; CHECK-NEXT:   store i32 %x, i32* %tmp2, align 1
; CHECK-NEXT:   store i32 %x, i32* %tmp3, align 1
; CHECK-NEXT:   call void @llvm.stackrestore(i32 %sp1)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; NOIR: Total across all functions
