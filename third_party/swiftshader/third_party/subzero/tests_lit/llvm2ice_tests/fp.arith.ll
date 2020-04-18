; This tries to be a comprehensive test of f32 and f64 arith operations.
; The CHECK lines are only checking for basic instruction patterns
; that should be present regardless of the optimization level, so
; there are no special OPTM1 match lines.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s
; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -Om1 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj --disassemble --target arm32 \
; RUN:   -i %s --args -O2 \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 %s
; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj --disassemble --target arm32 \
; RUN:   -i %s --args -Om1 \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s
; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -Om1 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

define internal float @addFloat(float %a, float %b) {
entry:
  %add = fadd float %a, %b
  ret float %add
}
; CHECK-LABEL: addFloat
; CHECK: addss
; CHECK: fld
; ARM32-LABEL: addFloat
; ARM32: vadd.f32 s{{[0-9]+}}, s
; MIPS32-LABEL: addFloat
; MIPS32: add.s

define internal double @addDouble(double %a, double %b) {
entry:
  %add = fadd double %a, %b
  ret double %add
}
; CHECK-LABEL: addDouble
; CHECK: addsd
; CHECK: fld
; ARM32-LABEL: addDouble
; ARM32: vadd.f64 d{{[0-9]+}}, d
; MIPS32-LABEL: addDouble
; MIPS32: add.d

define internal float @subFloat(float %a, float %b) {
entry:
  %sub = fsub float %a, %b
  ret float %sub
}
; CHECK-LABEL: subFloat
; CHECK: subss
; CHECK: fld
; ARM32-LABEL: subFloat
; ARM32: vsub.f32 s{{[0-9]+}}, s
; MIPS32-LABEL: subFloat
; MIPS32: sub.s

define internal double @subDouble(double %a, double %b) {
entry:
  %sub = fsub double %a, %b
  ret double %sub
}
; CHECK-LABEL: subDouble
; CHECK: subsd
; CHECK: fld
; ARM32-LABEL: subDouble
; ARM32: vsub.f64 d{{[0-9]+}}, d
; MIPS32-LABEL: subDouble
; MIPS32: sub.d

define internal float @mulFloat(float %a, float %b) {
entry:
  %mul = fmul float %a, %b
  ret float %mul
}
; CHECK-LABEL: mulFloat
; CHECK: mulss
; CHECK: fld
; ARM32-LABEL: mulFloat
; ARM32: vmul.f32 s{{[0-9]+}}, s
; MIPS32-LABEL: mulFloat
; MIPS32: mul.s

define internal double @mulDouble(double %a, double %b) {
entry:
  %mul = fmul double %a, %b
  ret double %mul
}
; CHECK-LABEL: mulDouble
; CHECK: mulsd
; CHECK: fld
; ARM32-LABEL: mulDouble
; ARM32: vmul.f64 d{{[0-9]+}}, d
; MIPS32-LABEL: mulDouble
; MIPS32: mul.d

define internal float @divFloat(float %a, float %b) {
entry:
  %div = fdiv float %a, %b
  ret float %div
}
; CHECK-LABEL: divFloat
; CHECK: divss
; CHECK: fld
; ARM32-LABEL: divFloat
; ARM32: vdiv.f32 s{{[0-9]+}}, s
; MIPS32-LABEL: divFloat
; MIPS32: div.s

define internal double @divDouble(double %a, double %b) {
entry:
  %div = fdiv double %a, %b
  ret double %div
}
; CHECK-LABEL: divDouble
; CHECK: divsd
; CHECK: fld
; ARM32-LABEL: divDouble
; ARM32: vdiv.f64 d{{[0-9]+}}, d
; MIPS32-LABEL: divDouble
; MIPS32: div.d

define internal float @remFloat(float %a, float %b) {
entry:
  %div = frem float %a, %b
  ret float %div
}
; CHECK-LABEL: remFloat
; CHECK: call {{.*}} R_{{.*}} fmodf
; ARM32-LABEL: remFloat
; ARM32: bl {{.*}} fmodf
; MIPS32-LABEL: remFloat
; MIPS32: jal {{.*}} fmodf

define internal double @remDouble(double %a, double %b) {
entry:
  %div = frem double %a, %b
  ret double %div
}
; CHECK-LABEL: remDouble
; CHECK: call {{.*}} R_{{.*}} fmod
; ARM32-LABEL: remDouble
; ARM32: bl {{.*}} fmod
; MIPS32-LABEL: remDouble
; MIPS32: jal {{.*}} fmod
