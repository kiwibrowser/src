; Tests various aspects of x86 opcode encodings. E.g., some opcodes like
; those for pmull vary more wildly depending on operand size (rather than
; follow a usual pattern).

; RUN: %p2i --filetype=obj --disassemble --sandbox -i %s --args -O2 \
; RUN:  -mattr=sse4.1 -split-local-vars=0 | FileCheck %s

define internal <8 x i16> @test_mul_v8i16(<8 x i16> %arg0, <8 x i16> %arg1) {
entry:
  %res = mul <8 x i16> %arg0, %arg1
  ret <8 x i16> %res
; CHECK-LABEL: test_mul_v8i16
; CHECK: 66 0f d5 c1 pmullw xmm0,xmm1
}

; Test register and address mode encoding.
define internal <8 x i16> @test_mul_v8i16_more_regs(
     <8 x i1> %cond, <8 x i16> %arg0, <8 x i16> %arg1, <8 x i16> %arg2,
     <8 x i16> %arg3, <8 x i16> %arg4, <8 x i16> %arg5, <8 x i16> %arg6,
     <8 x i16> %arg7, <8 x i16> %arg8) {
entry:
  %res1 = sub <8 x i16> %arg0, %arg1
  %res2 = sub <8 x i16> %arg0, %arg2
  %res3 = sub <8 x i16> %arg0, %arg3
  %res4 = sub <8 x i16> %arg0, %arg4
  %res5 = sub <8 x i16> %arg0, %arg5
  %res6 = sub <8 x i16> %arg0, %arg6
  %res7 = sub <8 x i16> %arg0, %arg7
  %res8 = sub <8 x i16> %arg0, %arg8
  %res_acc1 = select <8 x i1> %cond, <8 x i16> %res1, <8 x i16> %res2
  %res_acc2 = select <8 x i1> %cond, <8 x i16> %res3, <8 x i16> %res4
  %res_acc3 = select <8 x i1> %cond, <8 x i16> %res5, <8 x i16> %res6
  %res_acc4 = select <8 x i1> %cond, <8 x i16> %res7, <8 x i16> %res8
  %res_acc1_3 = select <8 x i1> %cond, <8 x i16> %res_acc1, <8 x i16> %res_acc3
  %res_acc2_4 = select <8 x i1> %cond, <8 x i16> %res_acc2, <8 x i16> %res_acc4
  %res = select <8 x i1> %cond, <8 x i16> %res_acc1_3, <8 x i16> %res_acc2_4
  ret <8 x i16> %res
; CHECK-LABEL: test_mul_v8i16_more_regs
; CHECK-DAG: psubw xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubw xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubw xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubw xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubw xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubw xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubw xmm0,XMMWORD PTR [esp
; CHECK-DAG: psubw xmm1,XMMWORD PTR [esp
}

define internal <4 x i32> @test_mul_v4i32(<4 x i32> %arg0, <4 x i32> %arg1) {
entry:
  %res = mul <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; CHECK-LABEL: test_mul_v4i32
; CHECK: 66 0f 38 40 c1  pmulld  xmm0,xmm1
}

define internal <4 x i32> @test_mul_v4i32_more_regs(
    <4 x i1> %cond, <4 x i32> %arg0, <4 x i32> %arg1, <4 x i32> %arg2,
    <4 x i32> %arg3, <4 x i32> %arg4, <4 x i32> %arg5, <4 x i32> %arg6,
    <4 x i32> %arg7, <4 x i32> %arg8) {
entry:
  %res1 = sub <4 x i32> %arg0, %arg1
  %res2 = sub <4 x i32> %arg0, %arg2
  %res3 = sub <4 x i32> %arg0, %arg3
  %res4 = sub <4 x i32> %arg0, %arg4
  %res5 = sub <4 x i32> %arg0, %arg5
  %res6 = sub <4 x i32> %arg0, %arg6
  %res7 = sub <4 x i32> %arg0, %arg7
  %res8 = sub <4 x i32> %arg0, %arg8
  %res_acc1 = select <4 x i1> %cond, <4 x i32> %res1, <4 x i32> %res2
  %res_acc2 = select <4 x i1> %cond, <4 x i32> %res3, <4 x i32> %res4
  %res_acc3 = select <4 x i1> %cond, <4 x i32> %res5, <4 x i32> %res6
  %res_acc4 = select <4 x i1> %cond, <4 x i32> %res7, <4 x i32> %res8
  %res_acc1_3 = select <4 x i1> %cond, <4 x i32> %res_acc1, <4 x i32> %res_acc3
  %res_acc2_4 = select <4 x i1> %cond, <4 x i32> %res_acc2, <4 x i32> %res_acc4
  %res = select <4 x i1> %cond, <4 x i32> %res_acc1_3, <4 x i32> %res_acc2_4
  ret <4 x i32> %res
; CHECK-LABEL: test_mul_v4i32_more_regs
; CHECK-DAG: psubd xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubd xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubd xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubd xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubd xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubd xmm0,{{xmm[0-7]|xmmword ptr\[esp}}
; CHECK-DAG: psubd xmm0,XMMWORD PTR [esp
; CHECK-DAG: psubd xmm1,XMMWORD PTR [esp
}

; Test movq, which is used by atomic stores.
declare void @llvm.nacl.atomic.store.i64(i64, i64*, i32)

define internal void @test_atomic_store_64(i32 %iptr, i32 %iptr2,
                                           i32 %iptr3, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %ptr2 = inttoptr i32 %iptr2 to i64*
  %ptr3 = inttoptr i32 %iptr3 to i64*
  call void @llvm.nacl.atomic.store.i64(i64 %v, i64* %ptr2, i32 6)
  call void @llvm.nacl.atomic.store.i64(i64 1234567891024, i64* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i64(i64 %v, i64* %ptr3, i32 6)
  ret void
}
; CHECK-LABEL: test_atomic_store_64
; CHECK-DAG: f3 0f 7e 04 24    movq xmm0,QWORD PTR [esp]
; CHECK-DAG: f3 0f 7e 44 24 08 movq xmm0,QWORD PTR [esp
; CHECK-DAG: 66 0f d6 0{{.*}}  movq QWORD PTR [e{{.*}}],xmm0

; Test "movups" via vector stores and loads.
define internal void @store_v16xI8(i32 %addr, i32 %addr2, i32 %addr3,
                                   <16 x i8> %v) {
  %addr_v16xI8 = inttoptr i32 %addr to <16 x i8>*
  %addr2_v16xI8 = inttoptr i32 %addr2 to <16 x i8>*
  %addr3_v16xI8 = inttoptr i32 %addr3 to <16 x i8>*
  store <16 x i8> %v, <16 x i8>* %addr2_v16xI8, align 1
  store <16 x i8> %v, <16 x i8>* %addr_v16xI8, align 1
  store <16 x i8> %v, <16 x i8>* %addr3_v16xI8, align 1
  ret void
}
; CHECK-LABEL: store_v16xI8
; CHECK: 0f 11 0{{.*}} movups XMMWORD PTR [e{{.*}}],xmm0

define internal <16 x i8> @load_v16xI8(i32 %addr, i32 %addr2, i32 %addr3) {
  %addr_v16xI8 = inttoptr i32 %addr to <16 x i8>*
  %addr2_v16xI8 = inttoptr i32 %addr2 to <16 x i8>*
  %addr3_v16xI8 = inttoptr i32 %addr3 to <16 x i8>*
  %res1 = load <16 x i8>, <16 x i8>* %addr2_v16xI8, align 1
  %res2 = load <16 x i8>, <16 x i8>* %addr_v16xI8, align 1
  %res3 = load <16 x i8>, <16 x i8>* %addr3_v16xI8, align 1
  %res12 = add <16 x i8> %res1, %res2
  %res123 = add <16 x i8> %res12, %res3
  ret <16 x i8> %res123
}
; CHECK-LABEL: load_v16xI8
; CHECK: 0f 10 0{{.*}} movups xmm0,XMMWORD PTR [e{{.*}}]

; Test segment override prefix. This happens w/ nacl.read.tp.
declare i8* @llvm.nacl.read.tp()

; Also test more address complex operands via address-mode-optimization.
define internal i32 @test_nacl_read_tp_more_addressing() {
entry:
  %ptr = call i8* @llvm.nacl.read.tp()
  %__1 = ptrtoint i8* %ptr to i32
  %x = add i32 %__1, %__1
  %__3 = inttoptr i32 %x to i32*
  %v = load i32, i32* %__3, align 1
  %v_add = add i32 %v, 1

  %ptr2 = call i8* @llvm.nacl.read.tp()
  %__6 = ptrtoint i8* %ptr2 to i32
  %y = add i32 %__6, -128
  %__8 = inttoptr i32 %y to i32*
  %v_add2 = add i32 %v, 4
  store i32 %v_add2, i32* %__8, align 1

  %z = add i32 %__6, 256
  %__9 = inttoptr i32 %z to i32*
  %v_add3 = add i32 %v, 91
  store i32 %v_add2, i32* %__9, align 1

  ret i32 %v
}
; CHECK-LABEL: test_nacl_read_tp_more_addressing
; CHECK: mov eax,{{(DWORD PTR )?}}gs:0x0
; CHECK: 8b 04 00              mov eax,DWORD PTR [eax+eax*1]
; CHECK: 65 8b 0d 00 00 00 00  mov ecx,DWORD PTR gs:0x0
; CHECK: 89 51 80              mov DWORD PTR [ecx-0x80],edx
; CHECK: 89 91 00 01 00 00     mov DWORD PTR [ecx+0x100],edx

; The 16-bit pinsrw/pextrw (SSE2) are quite different from
; the pinsr{b,d}/pextr{b,d} (SSE4.1).

define internal <4 x i32> @test_pinsrd(<4 x i32> %vec, i32 %elt1, i32 %elt2,
                                       i32 %elt3, i32 %elt4) {
entry:
  %elt12 = add i32 %elt1, %elt2
  %elt34 = add i32 %elt3, %elt4
  %res1 = insertelement <4 x i32> %vec, i32 %elt12, i32 1
  %res2 = insertelement <4 x i32> %res1, i32 %elt34, i32 2
  %res3 = insertelement <4 x i32> %res2, i32 %elt1, i32 3
  ret <4 x i32> %res3
}
; CHECK-LABEL: test_pinsrd
; CHECK-DAG: 66 0f 3a 22 c{{.*}} 01 pinsrd xmm0,e{{.*}}
; CHECK-DAG: 66 0f 3a 22 c{{.*}} 02 pinsrd xmm0,e{{.*}}
; CHECK-DAG: 66 0f 3a 22 c{{.*}} 03 pinsrd xmm0,e{{.*}}

define internal <16 x i8> @test_pinsrb(<16 x i8> %vec, i32 %elt1_w, i32 %elt2_w,
                                       i32 %elt3_w, i32 %elt4_w) {
entry:
  %elt1 = trunc i32 %elt1_w to i8
  %elt2 = trunc i32 %elt2_w to i8
  %elt3 = trunc i32 %elt3_w to i8
  %elt4 = trunc i32 %elt4_w to i8
  %elt12 = add i8 %elt1, %elt2
  %elt34 = add i8 %elt3, %elt4
  %res1 = insertelement <16 x i8> %vec, i8 %elt12, i32 1
  %res2 = insertelement <16 x i8> %res1, i8 %elt34, i32 7
  %res3 = insertelement <16 x i8> %res2, i8 %elt1, i32 15
  ret <16 x i8> %res3
}
; CHECK-LABEL: test_pinsrb
; CHECK-DAG: 66 0f 3a 20 c{{.*}} 01 pinsrb xmm0,e{{.*}}
; CHECK-DAG: 66 0f 3a 20 c{{.*}} 07 pinsrb xmm0,e{{.*}}
; CHECK-DAG: 66 0f 3a 20 c{{.*}} 0f pinsrb xmm0,e{{.*}}

define internal <8 x i16> @test_pinsrw(<8 x i16> %vec, i32 %elt1_w, i32 %elt2_w,
                                       i32 %elt3_w, i32 %elt4_w) {
entry:
  %elt1 = trunc i32 %elt1_w to i16
  %elt2 = trunc i32 %elt2_w to i16
  %elt3 = trunc i32 %elt3_w to i16
  %elt4 = trunc i32 %elt4_w to i16
  %elt12 = add i16 %elt1, %elt2
  %elt34 = add i16 %elt3, %elt4
  %res1 = insertelement <8 x i16> %vec, i16 %elt12, i32 1
  %res2 = insertelement <8 x i16> %res1, i16 %elt34, i32 4
  %res3 = insertelement <8 x i16> %res2, i16 %elt1, i32 7
  ret <8 x i16> %res3
}
; CHECK-LABEL: test_pinsrw
; CHECK-DAG: 66 0f c4 c{{.*}} 01 pinsrw xmm0,e{{.*}}
; CHECK-DAG: 66 0f c4 c{{.*}} 04 pinsrw xmm0,e{{.*}}
; CHECK-DAG: 66 0f c4 c{{.*}} 07 pinsrw xmm0,e{{.*}}

define internal i32 @test_pextrd(i32 %c, <4 x i32> %vec1, <4 x i32> %vec2,
                                 <4 x i32> %vec3, <4 x i32> %vec4) {
entry:
  switch i32 %c, label %three [i32 0, label %zero
                               i32 1, label %one
                               i32 2, label %two]
zero:
  %res0 = extractelement <4 x i32> %vec1, i32 0
  ret i32 %res0
one:
  %res1 = extractelement <4 x i32> %vec2, i32 1
  ret i32 %res1
two:
  %res2 = extractelement <4 x i32> %vec3, i32 2
  ret i32 %res2
three:
  %res3 = extractelement <4 x i32> %vec4, i32 3
  ret i32 %res3
}
; CHECK-LABEL: test_pextrd
; CHECK-DAG: 66 0f 3a 16 c0 00 pextrd eax,xmm0
; CHECK-DAG: 66 0f 3a 16 c8 01 pextrd eax,xmm1
; CHECK-DAG: 66 0f 3a 16 d0 02 pextrd eax,xmm2
; CHECK-DAG: 66 0f 3a 16 d8 03 pextrd eax,xmm3

define internal i32 @test_pextrb(i32 %c, <16 x i8> %vec1, <16 x i8> %vec2,
                                 <16 x i8> %vec3, <16 x i8> %vec4) {
entry:
  switch i32 %c, label %three [i32 0, label %zero
                               i32 1, label %one
                               i32 2, label %two]
zero:
  %res0 = extractelement <16 x i8> %vec1, i32 0
  %res0_ext = zext i8 %res0 to i32
  ret i32 %res0_ext
one:
  %res1 = extractelement <16 x i8> %vec2, i32 6
  %res1_ext = zext i8 %res1 to i32
  ret i32 %res1_ext
two:
  %res2 = extractelement <16 x i8> %vec3, i32 12
  %res2_ext = zext i8 %res2 to i32
  ret i32 %res2_ext
three:
  %res3 = extractelement <16 x i8> %vec4, i32 15
  %res3_ext = zext i8 %res3 to i32
  ret i32 %res3_ext
}
; CHECK-LABEL: test_pextrb
; CHECK-DAG: 66 0f 3a 14 c0 00 pextrb eax,xmm0
; CHECK-DAG: 66 0f 3a 14 c8 06 pextrb eax,xmm1
; CHECK-DAG: 66 0f 3a 14 d0 0c pextrb eax,xmm2
; CHECK-DAG: 66 0f 3a 14 d8 0f pextrb eax,xmm3

define internal i32 @test_pextrw(i32 %c, <8 x i16> %vec1, <8 x i16> %vec2,
                                 <8 x i16> %vec3, <8 x i16> %vec4) {
entry:
  switch i32 %c, label %three [i32 0, label %zero
                               i32 1, label %one
                               i32 2, label %two]
zero:
  %res0 = extractelement <8 x i16> %vec1, i32 0
  %res0_ext = zext i16 %res0 to i32
  ret i32 %res0_ext
one:
  %res1 = extractelement <8 x i16> %vec2, i32 2
  %res1_ext = zext i16 %res1 to i32
  ret i32 %res1_ext
two:
  %res2 = extractelement <8 x i16> %vec3, i32 5
  %res2_ext = zext i16 %res2 to i32
  ret i32 %res2_ext
three:
  %res3 = extractelement <8 x i16> %vec4, i32 7
  %res3_ext = zext i16 %res3 to i32
  ret i32 %res3_ext
}
; CHECK-LABEL: test_pextrw
; CHECK-DAG: 66 0f c5 c0 00 pextrw eax,xmm0
; CHECK-DAG: 66 0f c5 c1 02 pextrw eax,xmm1
; CHECK-DAG: 66 0f c5 c2 05 pextrw eax,xmm2
; CHECK-DAG: 66 0f c5 c3 07 pextrw eax,xmm3
