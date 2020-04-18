; This tries to be a comprehensive test of f32 and f64 compare operations.
; The CHECK lines are only checking for basic instruction patterns
; that should be present regardless of the optimization level, so
; there are no special OPTM1 match lines.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 | FileCheck %s
; RUN: %p2i --filetype=obj --disassemble -i %s --args -Om1 | FileCheck %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32O2 %s

define internal float @loadFloat(i32 %a) {
entry:
  %__1 = inttoptr i32 %a to float*
  %v0 = load float, float* %__1, align 4
  ret float %v0
}
; CHECK-LABEL: loadFloat
; CHECK: movss
; CHECK: fld

; MIPS32-LABEL: loadFloat
; MIPS32: lwc1 $f{{.*}},0{{.*}}
; MIPS32O2-LABEL: loadFloat
; MIPS32O2: lwc1 $f{{.*}},0{{.*}}

define internal double @loadDouble(i32 %a) {
entry:
  %__1 = inttoptr i32 %a to double*
  %v0 = load double, double* %__1, align 8
  ret double %v0
}
; CHECK-LABEL: loadDouble
; CHECK: movsd
; CHECK: fld

; MIPS32-LABEL: loadDouble
; MIPS32: ldc1 $f{{.*}},0{{.*}}
; MIPS32O2-LABEL: loadDouble
; MIPS32O2: ldc1 $f{{.*}},0{{.*}}

define internal void @storeFloat(i32 %a, float %value) {
entry:
  %__2 = inttoptr i32 %a to float*
  store float %value, float* %__2, align 4
  ret void
}
; CHECK-LABEL: storeFloat
; CHECK: movss
; CHECK: movss

; MIPS32-LABEL: storeFloat
; MIPS32: swc1 $f{{.*}},0{{.*}}
; MIPS32O2-LABEL: storeFloat
; MIPS32O2: mtc1 a1,$f{{.*}}
; MIPS32O2: swc1 $f{{.*}},0(a0)

define internal void @storeDouble(i32 %a, double %value) {
entry:
  %__2 = inttoptr i32 %a to double*
  store double %value, double* %__2, align 8
  ret void
}
; CHECK-LABEL: storeDouble
; CHECK: movsd
; CHECK: movsd

; MIPS32-LABEL: storeDouble
; MIPS32: sdc1 $f{{.*}},0{{.*}}
; MIPS32O2-LABEL: storeDouble
; MIPS32O2: mtc1 a2,$f{{.*}}
; MIPS32O2: mtc1 a3,$f{{.*}}
; MIPS32O2: sdc1 $f{{.*}},0(a0)

define internal void @storeFloatConst(i32 %a) {
entry:
  %a.asptr = inttoptr i32 %a to float*
  store float 0x3FF3AE1480000000, float* %a.asptr, align 4
  ret void
}
; CHECK-LABEL: storeFloatConst
; CHECK: movss
; CHECK: movss

; MIPS32-LABEL: storeFloatConst
; MIPS32: lui {{.*}},{{.*}}
; MIPS32: lwc1 $f{{.*}},{{.*}}
; MIPS32: swc1 $f{{.*}},0{{.*}}
; MIPS32O2-LABEL: storeFloatConst
; MIPS32O2: lui {{.*}},{{.*}}
; MIPS32O2: lwc1 $f{{.*}},{{.*}}
; MIPS32O2: swc1 $f{{.*}},0{{.*}}

define internal void @storeDoubleConst(i32 %a) {
entry:
  %a.asptr = inttoptr i32 %a to double*
  store double 1.230000e+00, double* %a.asptr, align 8
  ret void
}
; CHECK-LABEL: storeDoubleConst
; CHECK: movsd
; CHECK: movsd

; MIPS32-LABEL: storeDoubleConst
; MIPS32: lui {{.*}},{{.*}}
; MIPS32: ldc1 $f{{.*}},{{.*}}
; MIPS32: sdc1 $f{{.*}},0{{.*}}
; MIPS32O2-LABEL: storeDoubleConst
; MIPS32O2: lui {{.*}},{{.*}}
; MIPS32O2: ldc1 $f{{.*}},{{.*}}
; MIPS32O2: sdc1 $f{{.*}},0{{.*}}
