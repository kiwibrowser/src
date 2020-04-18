; This tries to be a comprehensive test of f32 and f64 compare operations.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s
; RUN: %p2i --filetype=obj --disassemble -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s \
; RUN:   --check-prefix=CHECK-OM1

; RUN: %if --need=target_ARM32 --command %p2i --filetype=obj --disassemble \
; RUN:   --target arm32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_ARM32 --command FileCheck %s \
; RUN:   --check-prefix=ARM32 --check-prefix=ARM32-O2

; RUN: %if --need=target_ARM32 --command %p2i --filetype=obj --disassemble \
; RUN:   --target arm32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_ARM32 --command FileCheck %s \
; RUN:   --check-prefix=ARM32 --check-prefix=ARM32-OM1

; RUN: %if --need=allow_dump --need=target_MIPS32 --command %p2i \
; RUN:   --filetype=asm --target mips32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=allow_dump --need=target_MIPS32 --command FileCheck %s \
; RUN:   --check-prefix=MIPS32

define internal void @fcmpEq(float %a, float %b, double %c, double %d) {
entry:
  %cmp = fcmp oeq float %a, %b
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = fcmp oeq double %c, %d
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.then2, %if.end
  ret void
}
; CHECK-LABEL: fcmpEq
; CHECK: ucomiss
; CHECK-NEXT: jne
; CHECK-NEXT: jp
; CHECK-NEXT: call {{.*}} R_{{.*}} func
; CHECK: ucomisd
; CHECK-NEXT: jne
; CHECK-NEXT: jp
; CHECK: call {{.*}} R_{{.*}} func
; CHECK-OM1-LABEL: fcmpEq
; CHECK-OM1: ucomiss
; CHECK-OM1: jne
; CHECK-OM1-NEXT: jp
; CHECK-OM1: call {{.*}} R_{{.*}} func
; CHECK-OM1: ucomisd
; CHECK-OM1: jne
; CHECK-NEXT-OM1: jp
; ARM32-LABEL: fcmpEq
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R0:r[0-9]+]], #0
; ARM32-OM1: moveq [[R0]], #1
; ARM32-O2: bne
; ARM32: bl{{.*}}func
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R1:r[0-9]+]], #0
; ARM32-OM1: moveq [[R1]], #1
; ARM32-O2: bne
; MIPS32-LABEL: fcmpEq
; MIPS32-LABEL: .LfcmpEq$entry
; MIPS32: c.eq.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}
; MIPS32-LABEL: .LfcmpEq$if.end
; MIPS32: c.eq.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

declare void @func()

define internal void @fcmpNe(float %a, float %b, double %c, double %d) {
entry:
  %cmp = fcmp une float %a, %b
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = fcmp une double %c, %d
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.then2, %if.end
  ret void
}
; CHECK-LABEL: fcmpNe
; CHECK: ucomiss
; CHECK-NEXT: jne
; CHECK-NEXT: jp
; CHECK-NEXT: jmp
; CHECK-NEXT: call {{.*}} R_{{.*}} func
; CHECK: ucomisd
; CHECK-NEXT: jne
; CHECK-NEXT: jp
; CHECK-NEXT: jmp
; CHECK-NEXT: call {{.*}} R_{{.*}} func
; CHECK-OM1-LABEL: fcmpNe
; CHECK-OM1: ucomiss
; CHECK-OM1: jne
; CHECK-OM1: jp
; CHECK-OM1: jmp
; CHECK-OM1: call {{.*}} R_{{.*}} func
; CHECK-OM1: ucomisd
; CHECK-OM1: jne
; CHECK-OM1: jp
; CHECK-OM1: jmp
; CHECK-OM1: call {{.*}} R_{{.*}} func
; ARM32-LABEL: fcmpNe
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R0:r[0-9]+]], #0
; ARM32-OM1: movne [[R0]], #1
; ARM32-O2: beq
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R1:r[0-9]+]], #0
; ARM32-OM1: movne [[R1]], #1
; ARM32-O2: beq
; MIPS32-LABEL: fcmpNe
; MIPS32-LABEL: .LfcmpNe$entry
; MIPS32: c.eq.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}
; MIPS32-LABEL: .LfcmpNe$if.end
; MIPS32: c.eq.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal void @fcmpGt(float %a, float %b, double %c, double %d) {
entry:
  %cmp = fcmp ogt float %a, %b
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = fcmp ogt double %c, %d
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.then2, %if.end
  ret void
}
; CHECK-LABEL: fcmpGt
; CHECK: ucomiss
; CHECK-NEXT: jbe
; CHECK-NEXT: call {{.*}} R_{{.*}} func
; CHECK: ucomisd
; CHECK-NEXT: jbe
; CHECK-NEXT: call {{.*}} R_{{.*}} func
; CHECK-OM1-LABEL: fcmpGt
; CHECK-OM1: ucomiss
; CHECK-OM1: seta
; CHECK-OM1: call {{.*}} R_{{.*}} func
; CHECK-OM1: ucomisd
; CHECK-OM1: seta
; CHECK-OM1: call {{.*}} R_{{.*}} func
; ARM32-LABEL: fcmpGt
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R0:r[0-9]+]], #0
; ARM32-OM1: movgt [[R0]], #1
; ARM32-O2: ble
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R1:r[0-9]+]], #0
; ARM32-OM1: movgt [[R1]], #1
; ARM32-O2: ble
; MIPS32-LABEL: fcmpGt
; MIPS32-LABEL: .LfcmpGt$entry
; MIPS32: c.ule.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}
; MIPS32-LABEL: .LfcmpGt$if.end
; MIPS32: c.ule.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal void @fcmpGe(float %a, float %b, double %c, double %d) {
entry:
  %cmp = fcmp ult float %a, %b
  br i1 %cmp, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %entry, %if.then
  %cmp1 = fcmp ult double %c, %d
  br i1 %cmp1, label %if.end3, label %if.then2

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.end, %if.then2
  ret void
}
; CHECK-LABEL: fcmpGe
; CHECK: ucomiss
; CHECK-NEXT: jb
; CHECK-NEXT: call {{.*}} R_{{.*}} func
; CHECK: ucomisd
; CHECK-NEXT: jb
; CHECK-NEXT: call {{.*}} R_{{.*}} func
; CHECK-OM1-LABEL: fcmpGe
; CHECK-OM1: ucomiss
; CHECK-OM1-NEXT: setb
; CHECK-OM1: call {{.*}} R_{{.*}} func
; CHECK-OM1: ucomisd
; CHECK-OM1-NEXT: setb
; CHECK-OM1: call {{.*}} R_{{.*}} func
; ARM32-LABEL: fcmpGe
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R0:r[0-9]+]], #0
; ARM32-OM1: movlt [[R0]], #1
; ARM32-O2: blt
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R1:r[0-9]+]], #0
; ARM32-OM1: movlt [[R1]], #1
; ARM32-O2: blt
; MIPS32-LABEL: fcmpGe
; MIPS32-LABEL: .LfcmpGe$entry
; MIPS32: c.ult.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}
; MIPS32-LABEL: .LfcmpGe$if.end
; MIPS32: c.ult.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal void @fcmpLt(float %a, float %b, double %c, double %d) {
entry:
  %cmp = fcmp olt float %a, %b
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = fcmp olt double %c, %d
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.then2, %if.end
  ret void
}
; CHECK-LABEL: fcmpLt
; CHECK: ucomiss
; CHECK-NEXT: jbe
; CHECK-NEXT: call {{.*}} R_{{.*}} func
; CHECK: ucomisd
; CHECK-NEXT: jbe
; CHECK-NEXT: call {{.*}} R_{{.*}} func
; CHECK-OM1-LABEL: fcmpLt
; CHECK-OM1: ucomiss
; CHECK-OM1-NEXT: seta
; CHECK-OM1: call {{.*}} R_{{.*}} func
; CHECK-OM1: ucomisd
; CHECK-OM1-NEXT: seta
; CHECK-OM1: call {{.*}} R_{{.*}} func
; ARM32-LABEL: fcmpLt
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R0:r[0-9]+]], #0
; ARM32-OM1: movmi [[R0]], #1
; ARM32-O2: bpl
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R1:r[0-9]+]], #0
; ARM32-OM1: movmi [[R1]], #1
; ARM32-O2: bpl
; MIPS32-LABEL: fcmpLt
; MIPS32-LABEL: .LfcmpLt$entry
; MIPS32: c.olt.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}
; MIPS32-LABEL: .LfcmpLt$if.end
; MIPS32: c.olt.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal void @fcmpLe(float %a, float %b, double %c, double %d) {
entry:
  %cmp = fcmp ugt float %a, %b
  br i1 %cmp, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %entry, %if.then
  %cmp1 = fcmp ugt double %c, %d
  br i1 %cmp1, label %if.end3, label %if.then2

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.end, %if.then2
  ret void
}
; CHECK-LABEL: fcmpLe
; CHECK: ucomiss
; CHECK-NEXT: jb
; CHECK-NEXT: call {{.*}} R_{{.*}} func
; CHECK: ucomisd
; CHECK-NEXT: jb
; CHECK-NEXT: call {{.*}} R_{{.*}} func
; CHECK-OM1-LABEL: fcmpLe
; CHECK-OM1: ucomiss
; CHECK-OM1-NEXT: setb
; CHECK-OM1: call {{.*}} R_{{.*}} func
; CHECK-OM1: ucomisd
; CHECK-OM1-NEXT: setb
; CHECK-OM1: call {{.*}} R_{{.*}} func
; ARM32-LABEL: fcmpLe
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R0:r[0-9]+]], #0
; ARM32-OM1: movhi [[R0]], #1
; ARM32-O2: bhi
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R1:r[0-9]+]], #0
; ARM32-OM1: movhi [[R1]], #1
; ARM32-O2: bhi
; MIPS32-LABEL: fcmpLe
; MIPS32-LABEL: .LfcmpLe$entry
; MIPS32: c.ole.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}
; MIPS32-LABEL: .LfcmpLe$if.end
; MIPS32: c.ole.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpFalseFloat(float %a, float %b) {
entry:
  %cmp = fcmp false float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpFalseFloat
; CHECK: mov {{.*}},0x0
; ARM32-LABEL: fcmpFalseFloat
; ARM32: mov [[R:r[0-9]+]], #0
; MIPS32-LABEL: fcmpFalseFloat
; MIPS32: addiu [[R:.*]], $zero, 0
; MIPS32: andi [[R]], [[R]], 1

define internal i32 @fcmpFalseDouble(double %a, double %b) {
entry:
  %cmp = fcmp false double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpFalseDouble
; CHECK: mov {{.*}},0x0
; ARM32-LABEL: fcmpFalseDouble
; ARM32: mov [[R:r[0-9]+]], #0
; MIPS32-LABEL: fcmpFalseDouble
; MIPS32: addiu [[R:.*]], $zero, 0
; MIPS32: andi [[R]], [[R]], 1

define internal i32 @fcmpOeqFloat(float %a, float %b) {
entry:
  %cmp = fcmp oeq float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOeqFloat
; CHECK: ucomiss
; CHECK: jne
; CHECK: jp
; ARM32-LABEL: fcmpOeqFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: moveq [[R]], #1
; MIPS32-LABEL: fcmpOeqFloat
; MIPS32: c.eq.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpOeqDouble(double %a, double %b) {
entry:
  %cmp = fcmp oeq double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOeqDouble
; CHECK: ucomisd
; CHECK: jne
; CHECK: jp
; ARM32-LABEL: fcmpOeqDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: moveq [[R]], #1
; MIPS32-LABEL: fcmpOeqDouble
; MIPS32: c.eq.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpOgtFloat(float %a, float %b) {
entry:
  %cmp = fcmp ogt float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOgtFloat
; CHECK: ucomiss
; CHECK: seta
; ARM32-LABEL: fcmpOgtFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movgt [[R]], #1
; MIPS32-LABEL: fcmpOgtFloat
; MIPS32: c.ule.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpOgtDouble(double %a, double %b) {
entry:
  %cmp = fcmp ogt double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOgtDouble
; CHECK: ucomisd
; CHECK: seta
; ARM32-LABEL: fcmpOgtDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movgt [[R]], #1
; MIPS32-LABEL: fcmpOgtDouble
; MIPS32: c.ule.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpOgeFloat(float %a, float %b) {
entry:
  %cmp = fcmp oge float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOgeFloat
; CHECK: ucomiss
; CHECK: setae
; ARM32-LABEL: fcmpOgeFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movge [[R]], #1
; MIPS32-LABEL: fcmpOgeFloat
; MIPS32: c.ult.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpOgeDouble(double %a, double %b) {
entry:
  %cmp = fcmp oge double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOgeDouble
; CHECK: ucomisd
; CHECK: setae
; ARM32-LABEL: fcmpOgeDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movge [[R]], #1
; MIPS32-LABEL: fcmpOgeDouble
; MIPS32: c.ult.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpOltFloat(float %a, float %b) {
entry:
  %cmp = fcmp olt float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOltFloat
; CHECK: ucomiss
; CHECK: seta
; ARM32-LABEL: fcmpOltFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movmi [[R]], #1
; MIPS32-LABEL: fcmpOltFloat
; MIPS32: c.olt.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpOltDouble(double %a, double %b) {
entry:
  %cmp = fcmp olt double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOltDouble
; CHECK: ucomisd
; CHECK: seta
; ARM32-LABEL: fcmpOltDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movmi [[R]], #1
; MIPS32-LABEL: fcmpOltDouble
; MIPS32: c.olt.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpOleFloat(float %a, float %b) {
entry:
  %cmp = fcmp ole float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOleFloat
; CHECK: ucomiss
; CHECK: setae
; ARM32-LABEL: fcmpOleFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movls [[R]], #1
; MIPS32-LABEL: fcmpOleFloat
; MIPS32: c.ole.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpOleDouble(double %a, double %b) {
entry:
  %cmp = fcmp ole double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOleDouble
; CHECK: ucomisd
; CHECK: setae
; ARM32-LABEL: fcmpOleDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movls [[R]], #1
; MIPS32-LABEL: fcmpOleDouble
; MIPS32: c.ole.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpOneFloat(float %a, float %b) {
entry:
  %cmp = fcmp one float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOneFloat
; CHECK: ucomiss
; CHECK: setne
; ARM32-LABEL: fcmpOneFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movmi [[R]], #1
; ARM32: movgt [[R]], #1
; MIPS32-LABEL: fcmpOneFloat
; MIPS32: c.ueq.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpOneDouble(double %a, double %b) {
entry:
  %cmp = fcmp one double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOneDouble
; CHECK: ucomisd
; CHECK: setne
; ARM32-LABEL: fcmpOneDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movmi [[R]], #1
; ARM32: movgt [[R]], #1
; MIPS32-LABEL: fcmpOneDouble
; MIPS32: c.ueq.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpOrdFloat(float %a, float %b) {
entry:
  %cmp = fcmp ord float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOrdFloat
; CHECK: ucomiss
; CHECK: setnp
; ARM32-LABEL: fcmpOrdFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movvc [[R]], #1
; MIPS32-LABEL: fcmpOrdFloat
; MIPS32: c.un.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpOrdDouble(double %a, double %b) {
entry:
  %cmp = fcmp ord double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpOrdDouble
; CHECK: ucomisd
; CHECK: setnp
; ARM32-LABEL: fcmpOrdDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movvc [[R]], #1
; MIPS32-LABEL: fcmpOrdDouble
; MIPS32: c.un.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpUeqFloat(float %a, float %b) {
entry:
  %cmp = fcmp ueq float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUeqFloat
; CHECK: ucomiss
; CHECK: sete
; ARM32-LABEL: fcmpUeqFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: moveq [[R]], #1
; ARM32: movvs [[R]], #1
; MIPS32-LABEL: fcmpUeqFloat
; MIPS32: c.ueq.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpUeqDouble(double %a, double %b) {
entry:
  %cmp = fcmp ueq double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUeqDouble
; CHECK: ucomisd
; CHECK: sete
; ARM32-LABEL: fcmpUeqDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: moveq [[R]], #1
; ARM32: movvs [[R]], #1
; MIPS32-LABEL: fcmpUeqDouble
; MIPS32: c.ueq.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpUgtFloat(float %a, float %b) {
entry:
  %cmp = fcmp ugt float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUgtFloat
; CHECK: ucomiss
; CHECK: setb
; ARM32-LABEL: fcmpUgtFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movhi [[R]], #1
; MIPS32-LABEL: fcmpUgtFloat
; MIPS32: c.ole.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpUgtDouble(double %a, double %b) {
entry:
  %cmp = fcmp ugt double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUgtDouble
; CHECK: ucomisd
; CHECK: setb
; ARM32-LABEL: fcmpUgtDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movhi [[R]], #1
; MIPS32-LABEL: fcmpUgtDouble
; MIPS32: c.ole.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpUgeFloat(float %a, float %b) {
entry:
  %cmp = fcmp uge float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUgeFloat
; CHECK: ucomiss
; CHECK: setbe
; ARM32-LABEL: fcmpUgeFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movpl [[R]], #1
; MIPS32-LABEL: fcmpUgeFloat
; MIPS32: c.olt.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpUgeDouble(double %a, double %b) {
entry:
  %cmp = fcmp uge double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUgeDouble
; CHECK: ucomisd
; CHECK: setbe
; ARM32-LABEL: fcmpUgeDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movpl [[R]], #1
; MIPS32-LABEL: fcmpUgeDouble
; MIPS32: c.olt.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpUltFloat(float %a, float %b) {
entry:
  %cmp = fcmp ult float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUltFloat
; CHECK: ucomiss
; CHECK: setb
; ARM32-LABEL: fcmpUltFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movlt [[R]], #1
; MIPS32-LABEL: fcmpUltFloat
; MIPS32: c.ult.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpUltDouble(double %a, double %b) {
entry:
  %cmp = fcmp ult double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUltDouble
; CHECK: ucomisd
; CHECK: setb
; ARM32-LABEL: fcmpUltDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movlt [[R]], #1
; MIPS32-LABEL: fcmpUltDouble
; MIPS32: c.ult.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpUleFloat(float %a, float %b) {
entry:
  %cmp = fcmp ule float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUleFloat
; CHECK: ucomiss
; CHECK: setbe
; ARM32-LABEL: fcmpUleFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movle [[R]], #1
; MIPS32-LABEL: fcmpUleFloat
; MIPS32: c.ule.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpUleDouble(double %a, double %b) {
entry:
  %cmp = fcmp ule double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUleDouble
; CHECK: ucomisd
; CHECK: setbe
; ARM32-LABEL: fcmpUleDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movle [[R]], #1
; MIPS32-LABEL: fcmpUleDouble
; MIPS32: c.ule.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpUneFloat(float %a, float %b) {
entry:
  %cmp = fcmp une float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUneFloat
; CHECK: ucomiss
; CHECK: jne
; CHECK: jp
; ARM32-LABEL: fcmpUneFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movne [[R]], #1
; MIPS32-LABEL: fcmpUneFloat
; MIPS32: c.eq.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpUneDouble(double %a, double %b) {
entry:
  %cmp = fcmp une double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUneDouble
; CHECK: ucomisd
; CHECK: jne
; CHECK: jp
; ARM32-LABEL: fcmpUneDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movne [[R]], #1
; MIPS32-LABEL: fcmpUneDouble
; MIPS32: c.eq.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movt [[REG]], $zero, {{.*}}

define internal i32 @fcmpUnoFloat(float %a, float %b) {
entry:
  %cmp = fcmp uno float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUnoFloat
; CHECK: ucomiss
; CHECK: setp
; ARM32-LABEL: fcmpUnoFloat
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f32
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movvs [[R]], #1
; MIPS32-LABEL: fcmpUnoFloat
; MIPS32: c.un.s
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpUnoDouble(double %a, double %b) {
entry:
  %cmp = fcmp uno double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpUnoDouble
; CHECK: ucomisd
; CHECK: setp
; ARM32-LABEL: fcmpUnoDouble
; ARM32-O2: mov [[R:r[0-9]+]], #0
; ARM32: vcmp.f64
; ARM32: vmrs
; ARM32-OM1: mov [[R:r[0-9]+]], #0
; ARM32: movvs [[R]], #1
; MIPS32-LABEL: fcmpUnoDouble
; MIPS32: c.un.d
; MIPS32: addiu [[REG:.*]], $zero, 1
; MIPS32: movf [[REG]], $zero, {{.*}}

define internal i32 @fcmpTrueFloat(float %a, float %b) {
entry:
  %cmp = fcmp true float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpTrueFloat
; CHECK: mov {{.*}},0x1
; ARM32-LABEL: fcmpTrueFloat
; ARM32: mov {{r[0-9]+}}, #1
; MIPS32-LABEL: fcmpTrueFloat
; MIPS32: addiu [[R:.*]], $zero, 1
; MIPS32: andi [[R]], [[R]], 1

define internal i32 @fcmpTrueDouble(double %a, double %b) {
entry:
  %cmp = fcmp true double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: fcmpTrueDouble
; CHECK: mov {{.*}},0x1
; ARM32-LABEL: fcmpTrueDouble
; ARM32: mov {{r[0-9]+}}, #1
; MIPS32-LABEL: fcmpTrueDouble
; MIPS32: addiu [[R:.*]], $zero, 1
; MIPS32: andi [[R]], [[R]], 1

define internal float @selectFloatVarVar(float %a, float %b) {
entry:
  %cmp = fcmp olt float %a, %b
  %cond = select i1 %cmp, float %a, float %b
  ret float %cond
}
; CHECK-LABEL: selectFloatVarVar
; CHECK: movss
; CHECK: minss
; ARM32-LABEL: selectFloatVarVar
; ARM32: vcmp.f32
; ARM32-OM1: vmovne.f32 s{{[0-9]+}}
; ARM32-O2: vmovmi.f32 s{{[0-9]+}}
; ARM32: bx
; MIPS32-LABEL: selectFloatVarVar
; MIPS32: movn.s {{.*}}

define internal double @selectDoubleVarVar(double %a, double %b) {
entry:
  %cmp = fcmp olt double %a, %b
  %cond = select i1 %cmp, double %a, double %b
  ret double %cond
}
; CHECK-LABEL: selectDoubleVarVar
; CHECK: movsd
; CHECK: minsd
; ARM32-LABEL: selectDoubleVarVar
; ARM32: vcmp.f64
; ARM32-OM1: vmovne.f64 d{{[0-9]+}}
; ARM32-O2: vmovmi.f64 d{{[0-9]+}}
; ARM32: bx
; MIPS32-LABEL: selectDoubleVarVar
; MIPS32: movn.d {{.*}}
