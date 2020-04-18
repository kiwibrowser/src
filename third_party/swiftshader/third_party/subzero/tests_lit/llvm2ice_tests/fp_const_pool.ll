; This tests that different floating point constants (such as 0.0 and -0.0)
; remain distinct even when they sort of look equal, and also that different
; instances of the same floating point constant (such as NaN and NaN) get the
; same constant pool entry even when "a==a" would suggest they are different.

; REQUIRES: allow_dump

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble \
; RUN:   --target mips32 -i %s --args -O2 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

define internal void @consume_float(float %f) {
  ret void
}

define internal void @consume_double(double %d) {
  ret void
}

define internal void @test_zeros() {
entry:
  call void @consume_float(float 0.0)
  call void @consume_float(float -0.0)
  call void @consume_double(double 0.0)
  call void @consume_double(double -0.0)
  ret void
}

; MIPS32-LABEL: test_zeros
; MIPS32: mtc1 zero,[[REG:.*]]
; MIPS32: mov.s {{.*}},[[REG]]
; MIPS32: lui [[REG:.*]],{{.*}}: R_MIPS_HI16 .L$float$80000000
; MIPS32: lwc1 {{.*}},0([[REG]]) {{.*}}: R_MIPS_LO16 .L$float$80000000
; MIPS32: mtc1 zero,[[REGLo:.*]]
; MIPS32: mtc1 zero,[[REGHi:.*]]
; MIPS32: mov.d {{.*}},[[REGLo]]
; MIPS32: lui [[REG:.*]],{{.*}}: R_MIPS_HI16 .L$double$8000000000000000
; MIPS32: ldc1 {{.*}},0([[REG]]) {{.*}}: R_MIPS_LO16 .L$double$8000000000000000

; Parse the function, dump the bitcode back out, and stop without translating.
; This tests that +0.0 and -0.0 aren't accidentally merged into a single
; zero-valued constant pool entry.
;
; RUN: %p2i -i %s --insts | FileCheck --check-prefix=ZERO %s
; ZERO: test_zeros
; ZERO-NEXT: entry:
; ZERO-NEXT: call void @consume_float(float 0.0
; ZERO-NEXT: call void @consume_float(float -0.0
; ZERO-NEXT: call void @consume_double(double 0.0
; ZERO-NEXT: call void @consume_double(double -0.0


define internal void @test_nans() {
entry:
  call void @consume_float(float 0x7FF8000000000000)
  call void @consume_float(float 0x7FF8000000000000)
  call void @consume_float(float 0xFFF8000000000000)
  call void @consume_float(float 0xFFF8000000000000)
  call void @consume_double(double 0x7FF8000000000000)
  call void @consume_double(double 0x7FF8000000000000)
  call void @consume_double(double 0xFFF8000000000000)
  call void @consume_double(double 0xFFF8000000000000)
  ret void
}

; MIPS32-LABEL: test_nans
; MIPS32: lui [[REG:.*]],{{.*}}: R_MIPS_HI16 .L$float$7fc00000
; MIPS32: lwc1 {{.*}},0([[REG]]) {{.*}}: R_MIPS_LO16 .L$float$7fc00000
; MIPS32: lui [[REG:.*]],{{.*}}: R_MIPS_HI16 .L$float$7fc00000
; MIPS32: lwc1 {{.*}},0([[REG]]) {{.*}}: R_MIPS_LO16 .L$float$7fc00000
; MIPS32: lui [[REG:.*]],{{.*}}: R_MIPS_HI16 .L$float$ffc00000
; MIPS32: lwc1 {{.*}},0([[REG]]) {{.*}}: R_MIPS_LO16 .L$float$ffc00000
; MIPS32: lui [[REG:.*]],{{.*}}: R_MIPS_HI16 .L$float$ffc00000
; MIPS32: lwc1 {{.*}},0([[REG]]) {{.*}}: R_MIPS_LO16 .L$float$ffc00000
; MIPS32: lui [[REG:.*]],{{.*}}: R_MIPS_HI16 .L$double$7ff8000000000000
; MIPS32: ldc1 {{.*}},0([[REG]]) {{.*}}: R_MIPS_LO16 .L$double$7ff8000000000000
; MIPS32: lui [[REG:.*]],{{.*}}: R_MIPS_HI16 .L$double$7ff8000000000000
; MIPS32: ldc1 {{.*}},0([[REG]]) {{.*}}: R_MIPS_LO16 .L$double$7ff8000000000000
; MIPS32: lui [[REG:.*]],{{.*}}: R_MIPS_HI16 .L$double$fff8000000000000
; MIPS32: ldc1 {{.*}},0([[REG]]) {{.*}}: R_MIPS_LO16 .L$double$fff8000000000000
; MIPS32: lui [[REG:.*]],{{.*}}: R_MIPS_HI16 .L$double$fff8000000000000
; MIPS32: ldc1 {{.*}},0([[REG]]) {{.*}}: R_MIPS_LO16 .L$double$fff8000000000000
; MIPS32: jr ra

; The following tests check the emitted constant pool entries and make sure
; there is at most one entry for each NaN value.  We have to run a separate test
; for each NaN because the constant pool entries may be emitted in any order.
;
; RUN: %p2i -i %s --filetype=asm --llvm-source \
; RUN:   | FileCheck --check-prefix=NANS1 %s
; NANS1: float nan
; NANS1-NOT: float nan
;
; RUN: %p2i -i %s --filetype=asm --llvm-source \
; RUN:   | FileCheck --check-prefix=NANS2 %s
; NANS2: float -nan
; NANS2-NOT: float -nan
;
; RUN: %p2i -i %s --filetype=asm --llvm-source \
; RUN:   | FileCheck --check-prefix=NANS3 %s
; NANS3: double nan
; NANS3-NOT: double nan
;
; RUN: %p2i -i %s --filetype=asm --llvm-source \
; RUN:   | FileCheck --check-prefix=NANS4 %s
; NANS4: double -nan
; NANS4-NOT: double -nan

; MIPS32 constant pool
; RUN: %if --need=target_MIPS32 --command %p2i \
; RUN:   --target mips32 -i %s --filetype=asm --llvm-source \
; RUN:   --args -O2 \
; RUN:   | %if --need=target_MIPS32 --command FileCheck \
; RUN:   --check-prefix=MIPS32CP %s
; MIPS32CP-LABEL: .L$float$7fc00000:
; MIPS32CP: .word 0x7fc00000 /* f32 nan */
; MIPS32CP-LABEL: .L$float$80000000
; MIPS32CP: .word 0x80000000 /* f32 -0.000000e+00 */
; MIPS32CP-LABEL: .L$float$ffc00000
; MIPS32CP: .word 0xffc00000 /* f32 -nan */
; MIPS32CP-LABEL: .L$double$7ff8000000000000
; MIPS32CP: .quad 0x7ff8000000000000 /* f64 nan */
; MIPS32CP-LABEL: .L$double$8000000000000000
; MIPS32CP: .quad 0x8000000000000000 /* f64 -0.000000e+00 */
; MIPS32CP-LABEL: .L$double$fff8000000000000
; MIPS32CP: .quad 0xfff8000000000000 /* f64 -nan */
