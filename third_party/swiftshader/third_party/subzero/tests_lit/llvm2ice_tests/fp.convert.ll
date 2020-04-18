; This tries to be a comprehensive test of f32 and f64 convert operations.
; The CHECK lines are only checking for basic instruction patterns
; that should be present regardless of the optimization level, so
; there are no special OPTM1 match lines.

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

; RUN: %if --need=allow_dump --need=target_MIPS32 --command %p2i \
; RUN:   --filetype=asm --target mips32 -i %s --args -Om1 \
; RUN:   | %if --need=allow_dump --need=target_MIPS32 --command FileCheck %s \
; RUN:   --check-prefix=MIPS32

; RUN: %if --need=allow_dump --need=target_MIPS32 --command %p2i \
; RUN:   --filetype=asm --target mips32 -i %s --args -O2 \
; RUN:   | %if --need=allow_dump --need=target_MIPS32 --command FileCheck %s \
; RUN:   --check-prefix=MIPS32O2

define internal float @fptrunc(double %a) {
entry:
  %conv = fptrunc double %a to float
  ret float %conv
}
; CHECK-LABEL: fptrunc
; CHECK: cvtsd2ss
; CHECK: fld
; ARM32-LABEL: fptrunc
; ARM32: vcvt.f32.f64 {{s[0-9]+}}, {{d[0-9]+}}
; MIPS32-LABEL: fptrunc
; MIPS32: cvt.s.d
; MIPS32O2-LABEL: fptrunc
; MIPS32O2: cvt.s.d

define internal double @fpext(float %a) {
entry:
  %conv = fpext float %a to double
  ret double %conv
}
; CHECK-LABEL: fpext
; CHECK: cvtss2sd
; CHECK: fld
; ARM32-LABEL: fpext
; ARM32: vcvt.f64.f32 {{d[0-9]+}}, {{s[0-9]+}}
; MIPS32-LABEL: fpext
; MIPS32: cvt.d.s
; MIPS32O2-LABEL: fpext
; MIPS32O2: cvt.d.s

define internal i64 @doubleToSigned64(double %a) {
entry:
  %conv = fptosi double %a to i64
  ret i64 %conv
}
; CHECK-LABEL: doubleToSigned64
; CHECK: call {{.*}} R_{{.*}} __Sz_fptosi_f64_i64
; ARM32-LABEL: doubleToSigned64
; TODO(jpp): implement this test.
; MIPS32-LABEL: doubleToSigned64
; MIPS32: jal __Sz_fptosi_f64_i64
; MIPS32O2-LABEL: doubleToSigned64
; MIPS32O2: jal __Sz_fptosi_f64_i64

define internal i64 @floatToSigned64(float %a) {
entry:
  %conv = fptosi float %a to i64
  ret i64 %conv
}
; CHECK-LABEL: floatToSigned64
; CHECK: call {{.*}} R_{{.*}} __Sz_fptosi_f32_i64
; ARM32-LABEL: floatToSigned64
; TODO(jpp): implement this test.
; MIPS32-LABEL: floatToSigned64
; MIPS32: jal __Sz_fptosi_f32_i64
; MIPS32O2-LABEL: floatToSigned64
; MIPS32O2: jal __Sz_fptosi_f32_i64

define internal i64 @doubleToUnsigned64(double %a) {
entry:
  %conv = fptoui double %a to i64
  ret i64 %conv
}
; CHECK-LABEL: doubleToUnsigned64
; CHECK: call {{.*}} R_{{.*}} __Sz_fptoui_f64_i64
; ARM32-LABEL: doubleToUnsigned64
; TODO(jpp): implement this test.
; MIPS32-LABEL: doubleToUnsigned64
; MIPS32: jal __Sz_fptoui_f64_i64
; MIPS32O2-LABEL: doubleToUnsigned64
; MIPS32O2: jal __Sz_fptoui_f64_i64

define internal i64 @floatToUnsigned64(float %a) {
entry:
  %conv = fptoui float %a to i64
  ret i64 %conv
}
; CHECK-LABEL: floatToUnsigned64
; CHECK: call {{.*}} R_{{.*}} __Sz_fptoui_f32_i64
; ARM32-LABEL: floatToUnsigned64
; TODO(jpp): implement this test.
; MIPS32-LABEL: floatToUnsigned64
; MIPS32: jal __Sz_fptoui_f32_i64
; MIPS32O2-LABEL: floatToUnsigned64
; MIPS32O2: jal __Sz_fptoui_f32_i64

define internal i32 @doubleToSigned32(double %a) {
entry:
  %conv = fptosi double %a to i32
  ret i32 %conv
}
; CHECK-LABEL: doubleToSigned32
; CHECK: cvttsd2si
; ARM32-LABEL: doubleToSigned32
; ARM32-DAG: vcvt.s32.f64 [[REG:s[0-9]*]], {{d[0-9]*}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; MIPS32-LABEL: doubleToSigned32
; MIPS32: trunc.w.d
; MIPS32O2-LABEL: doubleToSigned32
; MIPS32O2: trunc.w.d

define internal i32 @doubleToSigned32Const() {
entry:
  %conv = fptosi double 867.5309 to i32
  ret i32 %conv
}
; CHECK-LABEL: doubleToSigned32Const
; CHECK: cvttsd2si
; ARM32-LABEL: doubleToSigned32Const
; ARM32-DAG: movw [[ADDR:r[0-9]+]], #{{.*_MOVW_}}
; ARM32-DAG: movt [[ADDR]], #{{.*_MOVT_}}
; ARM32-DAG: vldr [[DREG:d[0-9]+]], {{\[}}[[ADDR]]{{\]}}
; ARM32-DAG: vcvt.s32.f64 [[REG:s[0-9]+]], [[DREG]]
; ARM32-DAF: vmov {{r[0-9]+}}, [[REG]]
; MIPS32-LABEL: doubleToSigned32Const
; MIPS32: lui
; MIPS32: ldc1
; MIPS32: trunc.w.d
; MIPS32O2-LABEL: doubleToSigned32Const
; MIPS32O2: lui
; MIPS32O2: ldc1
; MIPS32O2: trunc.w.d

define internal i32 @floatToSigned32(float %a) {
entry:
  %conv = fptosi float %a to i32
  ret i32 %conv
}
; CHECK-LABEL: floatToSigned32
; CHECK: cvttss2si
; ARM32-LABEL: floatToSigned32
; ARM32-DAG: vcvt.s32.f32 [[REG:s[0-9]+]], {{s[0-9]+}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; MIPS32-LABEL: floatToSigned32
; MIPS32: trunc.w.s $f{{.*}}, $f{{.*}}
; MIPS32O2-LABEL: floatToSigned32
; MIPS32O2: trunc.w.s $[[REG:f[0-9]+]], $f{{.*}}
; MIPS32O2: mfc1 $v0, $[[REG]]

define internal i32 @doubleToUnsigned32(double %a) {
entry:
  %conv = fptoui double %a to i32
  ret i32 %conv
}
; CHECK-LABEL: doubleToUnsigned32
; CHECK: call {{.*}} R_{{.*}} __Sz_fptoui_f64_i32
; ARM32-LABEL: doubleToUnsigned32
; ARM32-DAG: vcvt.u32.f64 [[REG:s[0-9]+]], {{d[0-9]+}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; MIPS32-LABEL: doubleToUnsigned32
; MIPS32: jal __Sz_fptoui_f64_i32
; MIPS32O2-LABEL: doubleToUnsigned32
; MIPS32O2: jal __Sz_fptoui_f64_i32

define internal i32 @floatToUnsigned32(float %a) {
entry:
  %conv = fptoui float %a to i32
  ret i32 %conv
}
; CHECK-LABEL: floatToUnsigned32
; CHECK: call {{.*}} R_{{.*}} __Sz_fptoui_f32_i32
; ARM32-LABEL: floatToUnsigned32
; ARM32-DAG: vcvt.u32.f32 [[REG:s[0-9]+]], {{s[0-9]+}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; MIPS32-LABEL: floatToUnsigned32
; MIPS32: jal __Sz_fptoui_f32_i32
; MIPS32O2-LABEL: floatToUnsigned32
; MIPS32O2: jal __Sz_fptoui_f32_i32

define internal i32 @doubleToSigned16(double %a) {
entry:
  %conv = fptosi double %a to i16
  %conv.ret_ext = sext i16 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: doubleToSigned16
; CHECK: cvttsd2si
; CHECK: movsx
; ARM32-LABEL: doubleToSigned16
; ARM32-DAG: vcvt.s32.f64 [[REG:s[0-9]*]], {{d[0-9]*}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; ARM32: sxth
; MIPS32-LABEL: doubleToSigned16
; MIPS32: trunc.w.d
; MIPS32O2-LABEL: doubleToSigned16
; MIPS32O2: trunc.w.d

define internal i32 @floatToSigned16(float %a) {
entry:
  %conv = fptosi float %a to i16
  %conv.ret_ext = sext i16 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: floatToSigned16
; CHECK: cvttss2si
; CHECK: movsx
; ARM32-LABEL: floatToSigned16
; ARM32-DAG: vcvt.s32.f32 [[REG:s[0-9]*]], {{s[0-9]*}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; ARM32: sxth
; MIPS32-LABEL: floatToSigned16
; MIPS32: trunc.w.s
; MIPS32O2-LABEL: floatToSigned16
; MIPS32O2: trunc.w.s

define internal i32 @doubleToUnsigned16(double %a) {
entry:
  %conv = fptoui double %a to i16
  %conv.ret_ext = zext i16 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: doubleToUnsigned16
; CHECK: cvttsd2si
; CHECK: movzx
; ARM32-LABEL: doubleToUnsigned16
; ARM32-DAG: vcvt.u32.f64 [[REG:s[0-9]*]], {{d[0-9]*}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; ARM32: uxth
; MIPS32-LABEL: doubleToUnsigned16
; MIPS32: trunc.w.d
; MIPS32O2-LABEL: doubleToUnsigned16
; MIPS32O2: trunc.w.d

define internal i32 @floatToUnsigned16(float %a) {
entry:
  %conv = fptoui float %a to i16
  %conv.ret_ext = zext i16 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: floatToUnsigned16
; CHECK: cvttss2si
; CHECK: movzx
; ARM32-LABEL: floatToUnsigned16
; ARM32-DAG: vcvt.u32.f32 [[REG:s[0-9]*]], {{s[0-9]*}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; ARM32: uxth
; MIPS32-LABEL: floatToUnsigned16
; MIPS32: trunc.w.s
; MIPS32O2-LABEL: floatToUnsigned16
; MIPS32O2: trunc.w.s

define internal i32 @doubleToSigned8(double %a) {
entry:
  %conv = fptosi double %a to i8
  %conv.ret_ext = sext i8 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: doubleToSigned8
; CHECK: cvttsd2si
; CHECK: movsx
; ARM32-LABEL: doubleToSigned8
; ARM32-DAG: vcvt.s32.f64 [[REG:s[0-9]*]], {{d[0-9]*}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; ARM32: sxtb
; MIPS32-LABEL: doubleToSigned8
; MIPS32: trunc.w.d
; MIPS32O2-LABEL: doubleToSigned8
; MIPS32O2: trunc.w.d

define internal i32 @floatToSigned8(float %a) {
entry:
  %conv = fptosi float %a to i8
  %conv.ret_ext = sext i8 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: floatToSigned8
; CHECK: cvttss2si
; CHECK: movsx
; ARM32-LABEL: floatToSigned8
; ARM32-DAG: vcvt.s32.f32 [[REG:s[0-9]*]], {{s[0-9]*}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; ARM32: sxtb
; MIPS32-LABEL: floatToSigned8
; MIPS32: trunc.w.s
; MIPS32O2-LABEL: floatToSigned8
; MIPS32O2: trunc.w.s

define internal i32 @doubleToUnsigned8(double %a) {
entry:
  %conv = fptoui double %a to i8
  %conv.ret_ext = zext i8 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: doubleToUnsigned8
; CHECK: cvttsd2si
; CHECK: movzx
; ARM32-LABEL: doubleToUnsigned8
; ARM32-DAG: vcvt.u32.f64 [[REG:s[0-9]*]], {{d[0-9]*}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; ARM32: uxtb
; MIPS32-LABEL: doubleToUnsigned8
; MIPS32: trunc.w.d
; MIPS32O2-LABEL: doubleToUnsigned8
; MIPS32O2: trunc.w.d

define internal i32 @floatToUnsigned8(float %a) {
entry:
  %conv = fptoui float %a to i8
  %conv.ret_ext = zext i8 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: floatToUnsigned8
; CHECK: cvttss2si
; CHECK: movzx
; ARM32-LABEL: floatToUnsigned8
; ARM32-DAG: vcvt.u32.f32 [[REG:s[0-9]*]], {{s[0-9]*}}
; ARM32-DAG: vmov {{r[0-9]+}}, [[REG]]
; ARM32: uxtb
; MIPS32-LABEL: floatToUnsigned8
; MIPS32: trunc.w.s
; MIPS32O2-LABEL: floatToUnsigned8
; MIPS32O2: trunc.w.s

define internal i32 @doubleToUnsigned1(double %a) {
entry:
  %tobool = fptoui double %a to i1
  %tobool.ret_ext = zext i1 %tobool to i32
  ret i32 %tobool.ret_ext
}
; CHECK-LABEL: doubleToUnsigned1
; CHECK: cvttsd2si
; CHECK-NOT: and eax,0x1
; ARM32-LABEL: doubleToUnsigned1
; ARM32-DAG: vcvt.u32.f64 [[REG:s[0-9]*]], {{d[0-9]*}}
; ARM32-DAG: vmov [[RES:r[0-9]+]], [[REG]]
; ARM32-DAG: and {{r[0-9]+}}, [[RES]], #1
; ARM32-NOT: uxth
; ARM32-NOT: uxtb
; MIPS32-LABEL: doubleToUnsigned1
; MIPS32: trunc.w.d
; MIPS32O2-LABEL: doubleToUnsigned1
; MIPS32O2: trunc.w.d

define internal i32 @floatToUnsigned1(float %a) {
entry:
  %tobool = fptoui float %a to i1
  %tobool.ret_ext = zext i1 %tobool to i32
  ret i32 %tobool.ret_ext
}
; CHECK-LABEL: floatToUnsigned1
; CHECK: cvttss2si
; CHECK-NOT: and eax,0x1
; ARM32-LABEL: floatToUnsigned1
; ARM32-DAG: vcvt.u32.f32 [[REG:s[0-9]*]], {{s[0-9]*}}
; ARM32-DAG: vmov [[RES:r[0-9]+]], [[REG]]
; ARM32-DAG: and {{r[0-9]+}}, [[RES]], #1
; ARM32-NOT: uxth
; ARM32-NOT: uxtb
; MIPS32-LABEL: floatToUnsigned1
; MIPS32: trunc.w.s
; MIPS32O2-LABEL: floatToUnsigned1
; MIPS32O2: trunc.w.s

define internal double @signed64ToDouble(i64 %a) {
entry:
  %conv = sitofp i64 %a to double
  ret double %conv
}
; CHECK-LABEL: signed64ToDouble
; CHECK: call {{.*}} R_{{.*}} __Sz_sitofp_i64_f64
; CHECK: fstp QWORD
; ARM32-LABEL: signed64ToDouble
; TODO(jpp): implement this test.
; MIPS32-LABEL: signed64ToDouble
; MIPS32: jal __Sz_sitofp_i64_f64
; MIPS32O2-LABEL: signed64ToDouble
; MIPS32O2: jal __Sz_sitofp_i64_f64

define internal float @signed64ToFloat(i64 %a) {
entry:
  %conv = sitofp i64 %a to float
  ret float %conv
}
; CHECK-LABEL: signed64ToFloat
; CHECK: call {{.*}} R_{{.*}} __Sz_sitofp_i64_f32
; CHECK: fstp DWORD
; ARM32-LABEL: signed64ToFloat
; TODO(jpp): implement this test.
; MIPS32-LABEL: signed64ToFloat
; MIPS32: jal __Sz_sitofp_i64_f32
; MIPS32O2-LABEL: signed64ToFloat
; MIPS32O2: jal __Sz_sitofp_i64_f32

define internal double @unsigned64ToDouble(i64 %a) {
entry:
  %conv = uitofp i64 %a to double
  ret double %conv
}
; CHECK-LABEL: unsigned64ToDouble
; CHECK: call {{.*}} R_{{.*}} __Sz_uitofp_i64_f64
; CHECK: fstp
; ARM32-LABEL: unsigned64ToDouble
; TODO(jpp): implement this test.
; MIPS32-LABEL: unsigned64ToDouble
; MIPS32: jal __Sz_uitofp_i64_f64
; MIPS32O2-LABEL: unsigned64ToDouble
; MIPS32O2: jal __Sz_uitofp_i64_f64

define internal float @unsigned64ToFloat(i64 %a) {
entry:
  %conv = uitofp i64 %a to float
  ret float %conv
}
; CHECK-LABEL: unsigned64ToFloat
; CHECK: call {{.*}} R_{{.*}} __Sz_uitofp_i64_f32
; CHECK: fstp
; ARM32-LABEL: unsigned64ToFloat
; TODO(jpp): implement this test.
; MIPS32-LABEL: unsigned64ToFloat
; MIPS32: jal __Sz_uitofp_i64_f32
; MIPS32O2-LABEL: unsigned64ToFloat
; MIPS32O2: jal __Sz_uitofp_i64_f32

define internal double @unsigned64ToDoubleConst() {
entry:
  %conv = uitofp i64 12345678901234 to double
  ret double %conv
}
; CHECK-LABEL: unsigned64ToDoubleConst
; CHECK: mov DWORD PTR [esp+0x4],0xb3a
; CHECK: mov DWORD PTR [esp],0x73ce2ff2
; CHECK: call {{.*}} R_{{.*}} __Sz_uitofp_i64_f64
; CHECK: fstp
; ARM32-LABEL: unsigned64ToDoubleConst
; TODO(jpp): implement this test.
; MIPS32-LABEL: unsigned64ToDoubleConst
; MIPS32: jal __Sz_uitofp_i64_f64
; MIPS32O2-LABEL: unsigned64ToDoubleConst
; MIPS32O2: jal __Sz_uitofp_i64_f64

define internal double @signed32ToDouble(i32 %a) {
entry:
  %conv = sitofp i32 %a to double
  ret double %conv
}
; CHECK-LABEL: signed32ToDouble
; CHECK: cvtsi2sd
; CHECK: fld
; ARM32-LABEL: signed32ToDouble
; ARM32-DAG: vmov [[SRC:s[0-9]+]], {{r[0-9]+}}
; ARM32-DAG: vcvt.f64.s32 {{d[0-9]+}}, [[SRC]]
; MIPS32-LABEL: signed32ToDouble
; MIPS32: cvt.d.w
; MIPS32O2-LABEL: signed32ToDouble
; MIPS32O2: cvt.d.w

define internal double @signed32ToDoubleConst() {
entry:
  %conv = sitofp i32 123 to double
  ret double %conv
}
; CHECK-LABEL: signed32ToDoubleConst
; CHECK: cvtsi2sd {{.*[^1]}}
; CHECK: fld
; ARM32-LABEL: signed32ToDoubleConst
; ARM32-DAG: mov [[CONST:r[0-9]+]], #123
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[CONST]]
; ARM32-DAG: vcvt.f64.s32 {{d[0-9]+}}, [[SRC]]
; MIPS32-LABEL: signed32ToDoubleConst
; MIPS32: cvt.d.w
; MIPS32O2-LABEL: signed32ToDoubleConst
; MIPS32O2: cvt.d.w

define internal float @signed32ToFloat(i32 %a) {
entry:
  %conv = sitofp i32 %a to float
  ret float %conv
}
; CHECK-LABEL: signed32ToFloat
; CHECK: cvtsi2ss
; CHECK: fld
; ARM32-LABEL: signed32ToFloat
; ARM32-DAG: vmov [[SRC:s[0-9]+]], {{r[0-9]+}}
; ARM32-DAG: vcvt.f32.s32 {{s[0-9]+}}, [[SRC]]
; MIPS32-LABEL: signed32ToFloat
; MIPS32: cvt.s.w $f{{.*}}, $f{{.*}}
; MIPS32O2-LABEL: signed32ToFloat
; MIPS32O2: mtc1 $a0, $[[REG:f[0-9]+]]
; MIPS32O2: cvt.s.w $f{{.*}}, $[[REG]]

define internal double @unsigned32ToDouble(i32 %a) {
entry:
  %conv = uitofp i32 %a to double
  ret double %conv
}
; CHECK-LABEL: unsigned32ToDouble
; CHECK: call {{.*}} R_{{.*}} __Sz_uitofp_i32_f64
; CHECK: fstp QWORD
; ARM32-LABEL: unsigned32ToDouble
; ARM32-DAG: vmov [[SRC:s[0-9]+]], {{r[0-9]+}}
; ARM32-DAG: vcvt.f64.u32 {{d[0-9]+}}, [[SRC]]
; MIPS32-LABEL: unsigned32ToDouble
; MIPS32: jal __Sz_uitofp_i32_f64
; MIPS32O2-LABEL: unsigned32ToDouble
; MIPS32O2: jal __Sz_uitofp_i32_f64

define internal float @unsigned32ToFloat(i32 %a) {
entry:
  %conv = uitofp i32 %a to float
  ret float %conv
}
; CHECK-LABEL: unsigned32ToFloat
; CHECK: call {{.*}} R_{{.*}} __Sz_uitofp_i32_f32
; CHECK: fstp DWORD
; ARM32-LABEL: unsigned32ToFloat
; ARM32-DAG: vmov [[SRC:s[0-9]+]], {{r[0-9]+}}
; ARM32-DAG: vcvt.f32.u32 {{s[0-9]+}}, [[SRC]]
; MIPS32-LABEL: unsigned32ToFloat
; MIPS32: jal __Sz_uitofp_i32_f32
; MIPS32O2-LABEL: unsigned32ToFloat
; MIPS32O2: jal __Sz_uitofp_i32_f32

define internal double @signed16ToDouble(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i16
  %conv = sitofp i16 %a.arg_trunc to double
  ret double %conv
}
; CHECK-LABEL: signed16ToDouble
; CHECK: cvtsi2sd
; CHECK: fld QWORD
; ARM32-LABEL: signed16ToDouble
; ARM32-DAG: sxth [[INT:r[0-9]+]]
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[INT]]
; ARM32-DAG: vcvt.f64.s32 {{d[0-9]+}}, [[SRC]]
; MIPS32-LABEL: signed16ToDouble
; MIPS32: cvt.d.w
; MIPS32O2-LABEL: signed16ToDouble
; MIPS32O2: cvt.d.w

define internal float @signed16ToFloat(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i16
  %conv = sitofp i16 %a.arg_trunc to float
  ret float %conv
}
; CHECK-LABEL: signed16ToFloat
; CHECK: cvtsi2ss
; CHECK: fld DWORD
; ARM32-LABEL: signed16ToFloat
; ARM32-DAG: sxth [[INT:r[0-9]+]]
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[INT]]
; ARM32-DAG: vcvt.f32.s32 {{s[0-9]+}}, [[SRC]]
; MIPS32-LABEL: signed16ToFloat
; MIPS32: cvt.s.w
; MIPS32O2-LABEL: signed16ToFloat
; MIPS32O2: cvt.s.w

define internal double @unsigned16ToDouble(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i16
  %conv = uitofp i16 %a.arg_trunc to double
  ret double %conv
}
; CHECK-LABEL: unsigned16ToDouble
; CHECK: cvtsi2sd
; CHECK: fld
; ARM32-LABEL: unsigned16ToDouble
; ARM32-DAG: uxth [[INT:r[0-9]+]]
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[INT]]
; ARM32-DAG: vcvt.f64.u32 {{d[0-9]+}}, [[SRC]]
; MIPS32-LABEL: unsigned16ToDouble
; MIPS32: cvt.d.w
; MIPS32O2-LABEL: unsigned16ToDouble
; MIPS32O2: cvt.d.w

define internal double @unsigned16ToDoubleConst() {
entry:
  %conv = uitofp i16 12345 to double
  ret double %conv
}
; CHECK-LABEL: unsigned16ToDoubleConst
; CHECK: cvtsi2sd
; CHECK: fld
; ARM32-LABEL: unsigned16ToDoubleConst
; ARM32-DAG: movw [[INT:r[0-9]+]], #12345
; ARM32-DAG: uxth [[INT]]
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[INT]]
; ARM32-DAG: vcvt.f64.u32 {{d[0-9]+}}, [[SRC]]
; MIPS32-LABEL: unsigned16ToDoubleConst
; MIPS32: cvt.d.w
; MIPS32O2-LABEL: unsigned16ToDoubleConst
; MIPS32O2: cvt.d.w

define internal float @unsigned16ToFloat(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i16
  %conv = uitofp i16 %a.arg_trunc to float
  ret float %conv
}
; CHECK-LABEL: unsigned16ToFloat
; CHECK: cvtsi2ss
; CHECK: fld
; ARM32-LABEL: unsigned16ToFloat
; ARM32-DAG: uxth [[INT:r[0-9]+]]
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[INT]]
; ARM32-DAG: vcvt.f32.u32 {{s[0-9]+}}, [[SRC]]
; MIPS32-LABEL: unsigned16ToFloat
; MIPS32: cvt.s.w
; MIPS32O2-LABEL: unsigned16ToFloat
; MIPS32O2: cvt.s.w

define internal double @signed8ToDouble(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i8
  %conv = sitofp i8 %a.arg_trunc to double
  ret double %conv
}
; CHECK-LABEL: signed8ToDouble
; CHECK: cvtsi2sd
; CHECK: fld
; ARM32-LABEL: signed8ToDouble
; ARM32-DAG: sxtb [[INT:r[0-9]+]]
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[INT]]
; ARM32-DAG: vcvt.f64.s32 {{d[0-9]+}}, [[SRC]]
; MIPS32-LABEL: signed8ToDouble
; MIPS32: cvt.d.w
; MIPS32O2-LABEL: signed8ToDouble
; MIPS32O2: cvt.d.w

define internal float @signed8ToFloat(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i8
  %conv = sitofp i8 %a.arg_trunc to float
  ret float %conv
}
; CHECK-LABEL: signed8ToFloat
; CHECK: cvtsi2ss
; CHECK: fld
; ARM32-LABEL: signed8ToFloat
; ARM32-DAG: sxtb [[INT:r[0-9]+]]
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[INT]]
; ARM32-DAG: vcvt.f32.s32 {{s[0-9]+}}, [[SRC]]
; MIPS32-LABEL: signed8ToFloat
; MIPS32: cvt.s.w
; MIPS32O2-LABEL: signed8ToFloat
; MIPS32O2: cvt.s.w

define internal double @unsigned8ToDouble(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i8
  %conv = uitofp i8 %a.arg_trunc to double
  ret double %conv
}
; CHECK-LABEL: unsigned8ToDouble
; CHECK: cvtsi2sd
; CHECK: fld
; ARM32-LABEL: unsigned8ToDouble
; ARM32-DAG: uxtb [[INT:r[0-9]+]]
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[INT]]
; ARM32-DAG: vcvt.f64.u32 {{d[0-9]+}}, [[SRC]]
; MIPS32-LABEL: unsigned8ToDouble
; MIPS32: cvt.d.w
; MIPS32O2-LABEL: unsigned8ToDouble
; MIPS32O2: cvt.d.w

define internal float @unsigned8ToFloat(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i8
  %conv = uitofp i8 %a.arg_trunc to float
  ret float %conv
}
; CHECK-LABEL: unsigned8ToFloat
; CHECK: cvtsi2ss
; CHECK: fld
; ARM32-LABEL: unsigned8ToFloat
; ARM32-DAG: uxtb [[INT:r[0-9]+]]
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[INT]]
; ARM32-DAG: vcvt.f32.u32 {{s[0-9]+}}, [[SRC]]
; MIPS32-LABEL: unsigned8ToFloat
; MIPS32: cvt.s.w
; MIPS32O2-LABEL: unsigned8ToFloat
; MIPS32O2: cvt.s.w

define internal double @unsigned1ToDouble(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i1
  %conv = uitofp i1 %a.arg_trunc to double
  ret double %conv
}
; CHECK-LABEL: unsigned1ToDouble
; CHECK: cvtsi2sd
; CHECK: fld
; ARM32-LABEL: unsigned1ToDouble
; ARM32-DAG: and [[INT:r[0-9]+]], {{r[0-9]+}}, #1
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[INT]]
; ARM32-DAG: vcvt.f64.u32 {{d[0-9]+}}, [[SRC]]
; MIPS32-LABEL: unsigned1ToDouble
; MIPS32: cvt.d.w
; MIPS32O2-LABEL: unsigned1ToDouble
; MIPS32O2: cvt.d.w

define internal float @unsigned1ToFloat(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i1
  %conv = uitofp i1 %a.arg_trunc to float
  ret float %conv
}
; CHECK-LABEL: unsigned1ToFloat
; CHECK: cvtsi2ss
; CHECK: fld
; ARM32-LABEL: unsigned1ToFloat
; ARM32-DAG: and [[INT:r[0-9]+]], {{r[0-9]+}}, #1
; ARM32-DAG: vmov [[SRC:s[0-9]+]], [[INT]]
; ARM32-DAG: vcvt.f32.u32 {{s[0-9]+}}, [[SRC]]
; MIPS32-LABEL: unsigned1ToFloat
; MIPS32: cvt.s.w
; MIPS32O2-LABEL: unsigned1ToFloat
; MIPS32O2: cvt.s.w

define internal float @int32BitcastToFloat(i32 %a) {
entry:
  %conv = bitcast i32 %a to float
  ret float %conv
}
; CHECK-LABEL: int32BitcastToFloat
; CHECK: mov
; ARM32-LABEL: int32BitcastToFloat
; ARM32: vmov s{{[0-9]+}}, r{{[0-9]+}}
; MIPS32-LABEL: int32BitcastToFloat
; MIPS32: sw
; MIPS32: lwc1
; MIPS32O2-LABEL: int32BitcastToFloat

define internal float @int32BitcastToFloatConst() {
entry:
  %conv = bitcast i32 8675309 to float
  ret float %conv
}
; CHECK-LABEL: int32BitcastToFloatConst
; CHECK: mov
; ARM32-LABEL: int32BitcastToFloatConst
; ARM32-DAG: movw [[REG:r[0-9]+]], #24557
; ARM32-DAG: movt [[REG]], #132
; ARM32: vmov s{{[0-9]+}}, [[REG]]
; MIPS32-LABEL: int32BitcastToFloatConst
; MIPS32: lwc1
; MIPS32O2-LABEL: int32BitcastToFloatConst

define internal double @int64BitcastToDouble(i64 %a) {
entry:
  %conv = bitcast i64 %a to double
  ret double %conv
}
; CHECK-LABEL: int64BitcastToDouble
; CHECK: mov
; ARM32-LABEL: int64BitcastToDouble
; ARM32: vmov d{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}
; MIPS32-LABEL: int64BitcastToDouble
; MIPS32: sw
; MIPS32: sw
; MIPS32: ldc1
; MIPS32O2-LABEL: int64BitcastToDouble

define internal double @int64BitcastToDoubleConst() {
entry:
  %conv = bitcast i64 9035768 to double
  ret double %conv
}
; CHECK-LABEL: int64BitcastToDoubleConst
; CHECK: mov
; ARM32-LABEL: int64BitcastToDoubleConst
; ARM32-DAG: movw [[REG0:r[0-9]+]], #57336
; ARM32-DAG: movt [[REG0]], #137
; ARM32-DAG: mov [[REG1:r[0-9]+]], #0
; ARM32-DAG: vmov d{{[0-9]+}}, [[REG0]], [[REG1]]
; MIPS32-LABEL: int64BitcastToDoubleConst
; MIPS32: ldc1
; MIPS32O2-LABEL: int64BitcastToDoubleConst
