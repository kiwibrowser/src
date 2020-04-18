; Trivial smoke test of bitcast between integer and FP types.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 | FileCheck %s
; RUN: %p2i --filetype=obj --disassemble -i %s --args -Om1 | FileCheck %s

; RUN: %if --need=target_ARM32 --command %p2i --filetype=obj --disassemble \
; RUN:   --target arm32 -i %s --args -O2 \
; RUN:   | %if --need=target_ARM32 --command FileCheck %s \
; RUN:   --check-prefix=ARM32

; RUN: %if --need=target_ARM32 --command %p2i --filetype=obj --disassemble \
; RUN:   --target arm32 -i %s --args -Om1 \
; RUN:   | %if --need=target_ARM32 --command FileCheck %s \
; RUN:   --check-prefix=ARM32

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm \
; RUN:   --target mips32 -i %s --args -O2 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck %s \
; RUN:   --check-prefix=MIPS32 --check-prefix=MIPS32-O2

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm \
; RUN:   --target mips32 -i %s --args -Om1 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck %s \
; RUN:   --check-prefix=MIPS32 --check-prefix=MIPS32-OM1

define internal i32 @cast_f2i(float %f) {
entry:
  %v0 = bitcast float %f to i32
  ret i32 %v0
}
; CHECK-LABEL: cast_f2i
; CHECK: movd eax
; ARM32-LABEL: cast_f2i
; ARM32: vmov r{{[0-9]+}}, s{{[0-9]+}}
; MIPS32-LABEL: cast_f2i
; MIPS32-O2: mfc1 $v0, $f{{[0-9]+}}
; MIPS32-OM1: swc1
; MIPS32-OM1: lw

define internal float @cast_i2f(i32 %i) {
entry:
  %v0 = bitcast i32 %i to float
  ret float %v0
}
; CHECK-LABEL: cast_i2f
; CHECK: fld DWORD PTR
; ARM32-LABEL: cast_i2f
; ARM32: vmov s{{[0-9]+}}, r{{[0-9]+}}
; MIPS32-LABEL: cast_i2f
; MIPS32-O2: mtc1 $a0, $f{{[0-9]+}}
; MIPS32-OM1: sw
; MIPS32-OM1: lwc1

define internal i64 @cast_d2ll(double %d) {
entry:
  %v0 = bitcast double %d to i64
  ret i64 %v0
}
; CHECK-LABEL: cast_d2ll
; CHECK: mov edx
; ARM32-LABEL: cast_d2ll
; ARM32: vmov r{{[0-9]+}}, r{{[0-9]+}}, d{{[0-9]+}}
; MIPS32-LABEL: cast_d2ll
; MIPS32-O2: swc1 $f13, {{.*}}
; MIPS32-O2: swc1 $f12, {{.*}}
; MIPS32-O2: lw $v0, {{.*}}
; MIPS32-O2: lw $v1, {{.*}}
; MIPS32-OM1: sdc1
; MIPS32-OM1: lw
; MIPS32-OM1: lw

define internal i64 @cast_d2ll_const() {
entry:
  %v0 = bitcast double 0x12345678901234 to i64
  ret i64 %v0
}
; CHECK-LABEL: cast_d2ll_const
; CHECK: mov e{{..}},{{(DWORD PTR )?}}ds:0x0 {{.*}} {{.*}}0012345678901234
; CHECK: mov e{{..}},{{(DWORD PTR )?}}ds:0x4 {{.*}} {{.*}}0012345678901234
; ARM32-LABEL: cast_d2ll_const
; ARM32-DAG: movw [[ADDR:r[0-9]+]], #{{.*_MOVW_}}
; ARM32-DAG: movt [[ADDR]], #{{.*_MOVT_}}
; ARM32-DAG: vldr [[DREG:d[0-9]+]], {{\[}}[[ADDR]]{{\]}}
; ARM32: vmov r{{[0-9]+}}, r{{[0-9]+}}, [[DREG]]
; MIPS32-LABEL: cast_d2ll_const
; MIPS32: lui {{.*}}, %hi(.L$double$0012345678901234)
; MIPS32: ldc1 {{.*}}, %lo(.L$double$0012345678901234)({{.*}})
; MIPS32: swc1 $f{{[0-9]+}}, {{.*}}
; MIPS32: swc1 $f{{[0-9]+}}, {{.*}}
; MIPS32: lw $v0, {{.*}}
; MIPS32: lw $v1, {{.*}}

define internal double @cast_ll2d(i64 %ll) {
entry:
  %v0 = bitcast i64 %ll to double
  ret double %v0
}
; CHECK-LABEL: cast_ll2d
; CHECK: fld QWORD PTR
; ARM32-LABEL: cast_ll2d
; ARM32: vmov d{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}
; MIPS32-LABEL: cast_ll2d
; MIPS32-O2: mtc1 $a0, $f{{[0-9]+}}
; MIPS32-O2: mtc1 $a1, $f{{[0-9]+}}
; MIPS32-OM1: sw
; MIPS32-OM1: sw
; MIPS32-OM1: ldc1

define internal double @cast_ll2d_const() {
entry:
  %v0 = bitcast i64 12345678901234 to double
  ret double %v0
}
; CHECK-LABEL: cast_ll2d_const
; CHECK: mov {{.*}},0x73ce2ff2
; CHECK: mov {{.*}},0xb3a
; CHECK: fld QWORD PTR
; ARM32-LABEL: cast_ll2d_const
; ARM32-DAG: movw [[REG0:r[0-9]+]], #12274
; ARM32-DAG: movt [[REG0:r[0-9]+]], #29646
; ARM32-DAG: movw [[REG1:r[0-9]+]], #2874
; ARM32: vmov d{{[0-9]+}}, [[REG0]], [[REG1]]
; MIPS32-LABEL: cast_ll2d_const
; MIPS32: lui {{.*}}, 29646
; MIPS32: ori {{.*}}, {{.*}}, 12274
; MIPS32: addiu	{{.*}}, $zero, 2874
; MIPS32-O2: mtc1 {{.*}}, $f{{[0-9]+}}
; MIPS32-O2: mtc1 {{.*}}, $f{{[0-9]+}}
; MIPS32-OM1: sw
; MIPS32-OM1: sw
; MIPS32-OM1: ldc1
