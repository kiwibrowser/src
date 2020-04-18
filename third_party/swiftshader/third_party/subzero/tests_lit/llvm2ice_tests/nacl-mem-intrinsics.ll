; This tests the NaCl intrinsics memset, memcpy and memmove.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 --sandbox -i %s --args -O2 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s
; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 --sandbox -i %s --args -Om1 --fmem-intrin-opt \
; RUN:   | %if --need=target_X8632 --command FileCheck %s
; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 --sandbox -i %s --args -Om1 \
; RUN:   | %if --need=target_X8632 --command FileCheck --check-prefix OM1 %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj --disassemble --target arm32 \
; RUN:   -i %s --args -O2 \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target mips32\
; RUN:   -i %s --args -Om1 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

declare void @llvm.memcpy.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1)
declare void @llvm.memmove.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1)
declare void @llvm.memset.p0i8.i32(i8*, i8, i32, i32, i1)

define internal void @test_memcpy(i32 %iptr_dst, i32 %iptr_src, i32 %len) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 %len, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memcpy
; CHECK: call {{.*}} R_{{.*}} memcpy
; OM1-LABEL: test_memcpy
; OM1: call  {{.*}} memcpy
; ARM32-LABEL: test_memcpy
; ARM32: bl {{.*}} memcpy
; MIPS32-LABEL: test_memcpy
; MIPS32: jal {{.*}} memcpy

define internal void @test_memcpy_long_const_len(i32 %iptr_dst, i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 4876, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memcpy_long_const_len
; CHECK: call {{.*}} R_{{.*}} memcpy
; OM1-LABEL: test_memcpy_long_const_len
; OM1: call {{.*}} memcpy
; ARM32-LABEL: test_memcpy_long_const_len
; ARM32: bl {{.*}} memcpy
; MIPS32-LABEL: test_memcpy_long_const_len
; MIPS32: jal {{.*}} memcpy

define internal void @test_memcpy_very_small_const_len(i32 %iptr_dst,
                                                       i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 2, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memcpy_very_small_const_len
; CHECK: mov [[REG:[^,]*]],WORD PTR [{{.*}}]
; CHECK-NEXT: mov WORD PTR [{{.*}}],[[REG]]
; CHECK-NOT: mov
; OM1-LABEL: test_memcpy_very_small_const_len
; OM1: call {{.*}} memcpy
; ARM32-LABEL: test_memcpy_very_small_const_len
; ARM32: bl {{.*}} memcpy
; MIPS32-LABEL: test_memcpy_very_small_const_len
; MIPS32: jal {{.*}} memcpy

define internal void @test_memcpy_const_len_3(i32 %iptr_dst, i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 3, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memcpy_const_len_3
; CHECK: mov [[REG:[^,]*]],WORD PTR [{{.*}}]
; CHECK-NEXT: mov WORD PTR [{{.*}}],[[REG]]
; CHECK-NEXT: mov [[REG:[^,]*]],BYTE PTR [{{.*}}+0x2]
; CHECK-NEXT: mov BYTE PTR [{{.*}}+0x2],[[REG]]
; CHECK-NOT: mov
; OM1-LABEL: test_memcpy_const_len_3
; OM1: call {{.*}} memcpy
; ARM32-LABEL: test_memcpy_const_len_3
; ARM32: bl {{.*}} memcpy
; MIPS32-LABEL: test_memcpy_const_len_3
; MIPS32: jal {{.*}} memcpy

define internal void @test_memcpy_mid_const_len(i32 %iptr_dst, i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 9, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memcpy_mid_const_len
; CHECK: movq [[REG:xmm[0-9]+]],QWORD PTR [{{.*}}]
; CHECK-NEXT: movq QWORD PTR [{{.*}}],[[REG]]
; CHECK-NEXT: mov [[REG:[^,]*]],BYTE PTR [{{.*}}+0x8]
; CHECK-NEXT: mov BYTE PTR [{{.*}}+0x8],[[REG]]
; CHECK-NOT: mov
; OM1-LABEL: test_memcpy_mid_const_len
; OM1: call {{.*}} memcpy
; ARM32-LABEL: test_memcpy_mid_const_len
; ARM32: bl {{.*}} memcpy
; MIPS32-LABEL: test_memcpy_mid_const_len
; MIPS32: jal {{.*}} memcpy

define internal void @test_memcpy_mid_const_len_overlap(i32 %iptr_dst,
                                                        i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 15, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memcpy_mid_const_len_overlap
; CHECK: movq [[REG:xmm[0-9]+]],QWORD PTR [{{.*}}]
; CHECK-NEXT: movq QWORD PTR [{{.*}}],[[REG]]
; CHECK-NEXT: movq [[REG:xmm[0-9]+]],QWORD PTR [{{.*}}+0x7]
; CHECK-NEXT: movq QWORD PTR [{{.*}}+0x7],[[REG]]
; CHECK-NOT: mov
; OM1-LABEL: test_memcpy_mid_const_len_overlap
; OM1: call {{.*}} memcpy
; ARM32-LABEL: test_memcpy_mid_const_len_overlap
; ARM32: bl {{.*}} memcpy
; MIPS32-LABEL: test_memcpy_mid_const_len_overlap
; MIPS32: jal {{.*}} memcpy

define internal void @test_memcpy_big_const_len_overlap(i32 %iptr_dst,
                                                        i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 30, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memcpy_big_const_len_overlap
; CHECK: movups [[REG:xmm[0-9]+]],XMMWORD PTR [{{.*}}]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}],[[REG]]
; CHECK-NEXT: movups [[REG:xmm[0-9]+]],XMMWORD PTR [{{.*}}+0xe]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}+0xe],[[REG]]
; CHECK-NOT: mov
; OM1-LABEL: test_memcpy_big_const_len_overlap
; OM1: call {{.*}} memcpy
; ARM32-LABEL: test_memcpy_big_const_len_overlap
; ARM32: bl {{.*}} memcpy
; MIPS32-LABEL: test_memcpy_big_const_len_overlap
; MIPS32: jal {{.*}} memcpy

define internal void @test_memcpy_large_const_len(i32 %iptr_dst,
                                                  i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 33, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memcpy_large_const_len
; CHECK: movups [[REG:xmm[0-9]+]],XMMWORD PTR [{{.*}}+0x10]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}+0x10],[[REG]]
; CHECK-NEXT: movups [[REG:xmm[0-9]+]],XMMWORD PTR [{{.*}}]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}],[[REG]]
; CHECK-NEXT: mov [[REG:[^,]*]],BYTE PTR [{{.*}}+0x20]
; CHECK-NEXT: mov BYTE PTR [{{.*}}+0x20],[[REG]]
; CHECK-NOT: mov
; OM1-LABEL: test_memcpy_large_const_len
; OM1: call {{.*}} memcpy
; ARM32-LABEL: test_memcpy_large_const_len
; ARM32: bl {{.*}} memcpy
; MIPS32-LABEL: test_memcpy_large_const_len
; MIPS32: jal {{.*}} memcpy

define internal void @test_memmove(i32 %iptr_dst, i32 %iptr_src, i32 %len) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memmove.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 %len, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memmove
; CHECK: call {{.*}} R_{{.*}} memmove
; OM1-LABEL: test_memmove
; OM1: call {{.*}} memmove
; ARM32-LABEL: test_memmove
; ARM32: bl {{.*}} memmove
; MIPS32-LABEL: test_memmove
; MIPS32: jal {{.*}} memmove

define internal void @test_memmove_long_const_len(i32 %iptr_dst,
                                                  i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memmove.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 4876, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memmove_long_const_len
; CHECK: call {{.*}} R_{{.*}} memmove
; OM1-LABEL: test_memmove_long_const_len
; OM1: call {{.*}} memmove
; ARM32-LABEL: test_memmove_long_const_len
; ARM32: bl {{.*}} memmove
; MIPS32-LABEL: test_memmove_long_const_len
; MIPS32: jal {{.*}} memmove

define internal void @test_memmove_very_small_const_len(i32 %iptr_dst,
                                                        i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memmove.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 2, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memmove_very_small_const_len
; CHECK: mov [[REG:[^,]*]],WORD PTR [{{.*}}]
; CHECK-NEXT: mov WORD PTR [{{.*}}],[[REG]]
; CHECK-NOT: mov
; OM1-LABEL: test_memmove_very_small_const_len
; OM1: call {{.*}} memmove
; ARM32-LABEL: test_memmove_very_small_const_len
; ARM32: bl {{.*}} memmove
; MIPS32-LABEL: test_memmove_very_small_const_len
; MIPS32: jal {{.*}} memmove

define internal void @test_memmove_const_len_3(i32 %iptr_dst, i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memmove.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 3, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memmove_const_len_3
; CHECK: mov [[REG0:[^,]*]],WORD PTR [{{.*}}]
; CHECK-NEXT: mov [[REG1:[^,]*]],BYTE PTR [{{.*}}+0x2]
; CHECK-NEXT: mov WORD PTR [{{.*}}],[[REG0]]
; CHECK-NEXT: mov BYTE PTR [{{.*}}+0x2],[[REG1]]
; CHECK-NOT: mov
; OM1-LABEL: test_memmove_const_len_3
; OM1: call {{.*}} memmove
; ARM32-LABEL: test_memmove_const_len_3
; ARM32: bl {{.*}} memmove
; MIPS32-LABEL: test_memmove_const_len_3
; MIPS32: jal {{.*}} memmove

define internal void @test_memmove_mid_const_len(i32 %iptr_dst, i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memmove.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 9, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memmove_mid_const_len
; CHECK: movq [[REG0:xmm[0-9]+]],QWORD PTR [{{.*}}]
; CHECK-NEXT: mov [[REG1:[^,]*]],BYTE PTR [{{.*}}+0x8]
; CHECK-NEXT: movq QWORD PTR [{{.*}}],[[REG0]]
; CHECK-NEXT: mov BYTE PTR [{{.*}}+0x8],[[REG1]]
; CHECK-NOT: mov
; OM1-LABEL: test_memmove_mid_const_len
; OM1: call {{.*}} memmove
; ARM32-LABEL: test_memmove_mid_const_len
; ARM32: bl {{.*}} memmove
; MIPS32-LABEL: test_memmove_mid_const_len
; MIPS32: jal {{.*}} memmove

define internal void @test_memmove_mid_const_len_overlap(i32 %iptr_dst,
                                                         i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memmove.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 15, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memmove_mid_const_len_overlap
; CHECK: movq [[REG0:xmm[0-9]+]],QWORD PTR [{{.*}}]
; CHECK-NEXT: movq [[REG1:xmm[0-9]+]],QWORD PTR [{{.*}}+0x7]
; CHECK-NEXT: movq QWORD PTR [{{.*}}],[[REG0]]
; CHECK-NEXT: movq QWORD PTR [{{.*}}+0x7],[[REG1]]
; CHECK-NOT: mov
; OM1-LABEL: test_memmove_mid_const_len_overlap
; OM1: call {{.*}} memmove
; ARM32-LABEL: test_memmove_mid_const_len_overlap
; ARM32: bl {{.*}} memmove
; MIPS32-LABEL: test_memmove_mid_const_len_overlap
; MIPS32: jal {{.*}} memmove

define internal void @test_memmove_big_const_len_overlap(i32 %iptr_dst,
                                                         i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memmove.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 30, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memmove_big_const_len_overlap
; CHECK: movups [[REG0:xmm[0-9]+]],XMMWORD PTR [{{.*}}]
; CHECK-NEXT: movups [[REG1:xmm[0-9]+]],XMMWORD PTR [{{.*}}+0xe]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}],[[REG0]]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}+0xe],[[REG1]]
; CHECK-NOT: mov
; OM1-LABEL: test_memmove_big_const_len_overlap
; OM1: call {{.*}} memmove
; ARM32-LABEL: test_memmove_big_const_len_overlap
; ARM32: bl {{.*}} memmove
; MIPS32-LABEL: test_memmove_big_const_len_overlap
; MIPS32: jal {{.*}} memmove

define internal void @test_memmove_large_const_len(i32 %iptr_dst,
                                                   i32 %iptr_src) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = inttoptr i32 %iptr_src to i8*
  call void @llvm.memmove.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 33, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memmove_large_const_len
; CHECK: movups [[REG0:xmm[0-9]+]],XMMWORD PTR [{{.*}}+0x10]
; CHECK-NEXT: movups [[REG1:xmm[0-9]+]],XMMWORD PTR [{{.*}}]
; CHECK-NEXT: mov [[REG2:[^,]*]],BYTE PTR [{{.*}}+0x20]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}+0x10],[[REG0]]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}],[[REG1]]
; CHECK-NEXT: mov BYTE PTR [{{.*}}+0x20],[[REG2]]
; CHECK-NOT: mov
; OM1-LABEL: test_memmove_large_const_len
; OM1: call {{.*}} memmove
; ARM32-LABEL: test_memmove_large_const_len
; ARM32: bl {{.*}} memmove
; MIPS32-LABEL: test_memmove_large_const_len
; MIPS32: jal {{.*}} memmove

define internal void @test_memset(i32 %iptr_dst, i32 %wide_val, i32 %len) {
entry:
  %val = trunc i32 %wide_val to i8
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 %val,
                                  i32 %len, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset
; CHECK: movzx
; CHECK: call {{.*}} R_{{.*}} memset
; OM1-LABEL: test_memset
; OM1: movzx
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset
; MIPS32: jal {{.*}} memset

define internal void @test_memset_const_len_align(i32 %iptr_dst,
                                                  i32 %wide_val) {
entry:
  %val = trunc i32 %wide_val to i8
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 %val,
                                  i32 32, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset_const_len_align
; CHECK: movzx
; CHECK: call {{.*}} R_{{.*}} memset
; OM1-LABEL: test_memset_const_len_align
; OM1: movzx
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset_const_len_align
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset_const_len_align
; MIPS32: jal {{.*}} memset

define internal void @test_memset_long_const_len_zero_val_align(
    i32 %iptr_dst) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 0,
                                  i32 4876, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset_long_const_len_zero_val_align
; CHECK: call {{.*}} R_{{.*}} memset
; OM1-LABEL: test_memset_long_const_len_zero_val_align
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset_long_const_len_zero_val_align
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset_long_const_len_zero_val_align
; MIPS32: jal {{.*}} memset

define internal void @test_memset_const_val(i32 %iptr_dst, i32 %len) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 0, i32 %len, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset_const_val
; CHECK-NOT: movzx
; CHECK: call {{.*}} R_{{.*}} memset
; OM1-LABEL: test_memset_const_val
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset_const_val
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset_const_val
; MIPS32: jal {{.*}} memset

define internal void @test_memset_const_val_len_very_small(i32 %iptr_dst) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 10, i32 2, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset_const_val_len_very_small
; CHECK: mov WORD PTR [{{.*}}],0xa0a
; CHECK-NOT: mov
; OM1-LABEL: test_memset_const_val_len_very_small
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset_const_val_len_very_small
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset_const_val_len_very_small
; MIPS32: jal {{.*}} memset

define internal void @test_memset_const_val_len_3(i32 %iptr_dst) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 16, i32 3, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset_const_val_len_3
; CHECK: mov WORD PTR [{{.*}}],0x1010
; CHECK-NEXT: mov BYTE PTR [{{.*}}+0x2],0x10
; CHECK-NOT: mov
; OM1-LABEL: test_memset_const_val_len_3
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset_const_val_len_3
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset_const_val_len_3
; MIPS32: jal {{.*}} memset

define internal void @test_memset_const_val_len_mid(i32 %iptr_dst) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 32, i32 9, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset_const_val_len_mid
; CHECK: mov DWORD PTR [{{.*}}+0x4],0x20202020
; CHECK: mov DWORD PTR [{{.*}}],0x20202020
; CHECK-NEXT: mov BYTE PTR [{{.*}}+0x8],0x20
; CHECK-NOT: mov
; OM1-LABEL: test_memset_const_val_len_mid
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset_const_val_len_mid
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset_const_val_len_mid
; MIPS32: jal {{.*}} memset

; Same as above, but with a negative value.
define internal void @test_memset_const_neg_val_len_mid(i32 %iptr_dst) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 -128, i32 9, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset_const_neg_val_len_mid
; CHECK: mov DWORD PTR [{{.*}}+0x4],0x80808080
; CHECK: mov DWORD PTR [{{.*}}],0x80808080
; CHECK-NEXT: mov BYTE PTR [{{.*}}+0x8],0x80
; CHECK-NOT: mov
; OM1-LABEL: test_memset_const_neg_val_len_mid
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset_const_neg_val_len_mid
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset_const_neg_val_len_mid
; MIPS32: jal {{.*}} memset

define internal void @test_memset_zero_const_len_small(i32 %iptr_dst) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 0, i32 12, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset_zero_const_len_small
; CHECK: pxor [[ZERO:xmm[0-9]+]],[[ZERO]]
; CHECK-NEXT: movq QWORD PTR [{{.*}}],[[ZERO]]
; CHECK-NEXT: mov DWORD PTR [{{.*}}+0x8],0x0
; CHECK-NOT: mov
; OM1-LABEL: test_memset_zero_const_len_small
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset_zero_const_len_small
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset_zero_const_len_small
; MIPS32: jal {{.*}} memset

define internal void @test_memset_zero_const_len_small_overlap(i32 %iptr_dst) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 0, i32 15, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset_zero_const_len_small_overlap
; CHECK: pxor [[ZERO:xmm[0-9]+]],[[ZERO]]
; CHECK-NEXT: movq QWORD PTR [{{.*}}],[[ZERO]]
; CHECK-NEXT: movq QWORD PTR [{{.*}}+0x7],[[ZERO]]
; CHECK-NOT: mov
; OM1-LABEL: test_memset_zero_const_len_small_overlap
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset_zero_const_len_small_overlap
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset_zero_const_len_small_overlap
; MIPS32: jal {{.*}} memset

define internal void @test_memset_zero_const_len_big_overlap(i32 %iptr_dst) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 0, i32 30, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset_zero_const_len_big_overlap
; CHECK: pxor [[ZERO:xmm[0-9]+]],[[ZERO]]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}],[[ZERO]]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}+0xe],[[ZERO]]
; CHECK-NOT: mov
; OM1-LABEL: test_memset_zero_const_len_big_overlap
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset_zero_const_len_big_overlap
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset_zero_const_len_big_overlap
; MIPS32: jal {{.*}} memset

define internal void @test_memset_zero_const_len_large(i32 %iptr_dst) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 0, i32 33, i32 1, i1 false)
  ret void
}
; CHECK-LABEL: test_memset_zero_const_len_large
; CHECK: pxor [[ZERO:xmm[0-9]+]],[[ZERO]]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}+0x10],[[ZERO]]
; CHECK-NEXT: movups XMMWORD PTR [{{.*}}],[[ZERO]]
; CHECK-NEXT: mov BYTE PTR [{{.*}}+0x20],0x0
; CHECK-NOT: mov
; OM1-LABEL: test_memset_zero_const_len_large
; OM1: call {{.*}} R_{{.*}} memset
; ARM32-LABEL: test_memset_zero_const_len_large
; ARM32: uxtb
; ARM32: bl {{.*}} memset
; MIPS32-LABEL: test_memset_zero_const_len_large
; MIPS32: jal {{.*}} memset
