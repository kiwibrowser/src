; This is a test of C-level conversion operations that clang lowers
; into pairs of shifts.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -Om1 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -O2 \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -Om1 \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -O2 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32-O2 --check-prefix MIPS32 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -Om1 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32-OM1 --check-prefix MIPS32 %s

@i1 = internal global [4 x i8] zeroinitializer, align 4
@i2 = internal global [4 x i8] zeroinitializer, align 4
@u1 = internal global [4 x i8] zeroinitializer, align 4

define internal void @conv1() {
entry:
  %__0 = bitcast [4 x i8]* @u1 to i32*
  %v0 = load i32, i32* %__0, align 1
  %sext = shl i32 %v0, 24
  %v1 = ashr i32 %sext, 24
  %__4 = bitcast [4 x i8]* @i1 to i32*
  store i32 %v1, i32* %__4, align 1
  ret void
}
; CHECK-LABEL: conv1
; CHECK: shl {{.*}},0x18
; CHECK: sar {{.*}},0x18

; ARM32-LABEL: conv1
; ARM32: lsl {{.*}}, #24
; ARM32: asr {{.*}}, #24

define internal void @conv2() {
entry:
  %__0 = bitcast [4 x i8]* @u1 to i32*
  %v0 = load i32, i32* %__0, align 1
  %sext1 = shl i32 %v0, 16
  %v1 = lshr i32 %sext1, 16
  %__4 = bitcast [4 x i8]* @i2 to i32*
  store i32 %v1, i32* %__4, align 1
  ret void
}
; CHECK-LABEL: conv2
; CHECK: shl {{.*}},0x10
; CHECK: shr {{.*}},0x10

; ARM32-LABEL: conv2
; ARM32: lsl {{.*}}, #16
; ARM32: lsr {{.*}}, #16

define internal i32 @shlImmLarge(i32 %val) {
entry:
  %result = shl i32 %val, 257
  ret i32 %result
}
; CHECK-LABEL: shlImmLarge
; CHECK: shl {{.*}},0x1

; MIPS32-LABEL: shlImmLarge
; MIPS32: sll

define internal i32 @shlImmNeg(i32 %val) {
entry:
  %result = shl i32 %val, -1
  ret i32 %result
}
; CHECK-LABEL: shlImmNeg
; CHECK: shl {{.*}},0xff

; MIPS32-LABEL: shlImmNeg
; MIPS32: sll

define internal i32 @lshrImmLarge(i32 %val) {
entry:
  %result = lshr i32 %val, 257
  ret i32 %result
}
; CHECK-LABEL: lshrImmLarge
; CHECK: shr {{.*}},0x1

; MIPS32-LABEL: lshrImmLarge
; MIPS32: srl

define internal i32 @lshrImmNeg(i32 %val) {
entry:
  %result = lshr i32 %val, -1
  ret i32 %result
}
; CHECK-LABEL: lshrImmNeg
; CHECK: shr {{.*}},0xff

; MIPS32-LABEL: lshrImmNeg
; MIPS32: srl

define internal i32 @ashrImmLarge(i32 %val) {
entry:
  %result = ashr i32 %val, 257
  ret i32 %result
}
; CHECK-LABEL: ashrImmLarge
; CHECK: sar {{.*}},0x1

; MIPS32-LABEL: ashrImmLarge
; MIPS32: sra

define internal i32 @ashrImmNeg(i32 %val) {
entry:
  %result = ashr i32 %val, -1
  ret i32 %result
}
; CHECK-LABEL: ashrImmNeg
; CHECK: sar {{.*}},0xff

; MIPS32-LABEL: ashrImmNeg
; MIPS32: sra

define internal i64 @shlImm64One(i64 %val) {
entry:
  %result = shl i64 %val, 1
  ret i64 %result
}
; CHECK-LABEL: shlImm64One
; CHECK: shl {{.*}},1
; MIPS32-LABEL: shlImm64One
; MIPS32: addu	[[T_LO:.*]],[[VAL_LO:.*]],[[VAL_LO]]
; MIPS32: sltu	[[T1:.*]],[[T_LO]],[[VAL_LO]]
; MIPS32: addu	[[T2:.*]],[[T1]],[[VAL_HI:.*]]
; MIPS32: addu	{{.*}},[[VAL_HI]],[[T2]]

define internal i64 @shlImm64LessThan32(i64 %val) {
entry:
  %result = shl i64 %val, 4
  ret i64 %result
}
; CHECK-LABEL: shlImm64LessThan32
; CHECK: shl {{.*}},0x4
; MIPS32-LABEL: shlImm64LessThan32
; MIPS32: srl	[[T1:.*]],[[VAL_LO:.*]],0x1c
; MIPS32: sll	[[T2:.*]],{{.*}},0x4
; MIPS32: or	{{.*}},[[T1]],[[T2]]
; MIPS32: sll	{{.*}},[[VAL_LO]],0x4

define internal i64 @shlImm64Equal32(i64 %val) {
entry:
  %result = shl i64 %val, 32
  ret i64 %result
}
; CHECK-LABEL: shlImm64Equal32
; CHECK-NOT: shl
; MIPS32-LABEL: shlImm64Equal32
; MIPS32: li	{{.*}},0
; MIPS32-O2:	move
; MIPS32-OM1:	sw
; MIPS32-OM1:	lw

define internal i64 @shlImm64GreaterThan32(i64 %val) {
entry:
  %result = shl i64 %val, 40
  ret i64 %result
}
; CHECK-LABEL: shlImm64GreaterThan32
; CHECK: shl {{.*}},0x8
; MIPS32-LABEL: shlImm64GreaterThan32
; MIPS32: sll	{{.*}},{{.*}},0x8
; MIPS32: li	{{.*}},0

define internal i64 @lshrImm64One(i64 %val) {
entry:
  %result = lshr i64 %val, 1
  ret i64 %result
}
; CHECK-LABEL: lshrImm64One
; CHECK: shr {{.*}},1
; MIPS32-LABEL: lshrImm64One
; MIPS32: sll	[[T1:.*]],[[VAL_HI:.*]],0x1f
; MIPS32: srl	[[T2:.*]],{{.*}},0x1
; MIPS32: or	{{.*}},[[T1]],[[T2]]
; MIPS32: srl	{{.*}},[[VAL_HI]],0x1

define internal i64 @lshrImm64LessThan32(i64 %val) {
entry:
  %result = lshr i64 %val, 4
  ret i64 %result
}
; CHECK-LABEL: lshrImm64LessThan32
; CHECK: shrd {{.*}},0x4
; CHECK: shr {{.*}},0x4
; MIPS32-LABEL: lshrImm64LessThan32
; MIPS32: sll	[[T1:.*]],[[VAL_HI:.*]],0x1c
; MIPS32: srl	[[T2:.*]],{{.*}},0x4
; MIPS32: or	{{.*}},[[T1]],[[T2]]
; MIPS32: srl	{{.*}},[[VAL_HI]],0x4

define internal i64 @lshrImm64Equal32(i64 %val) {
entry:
  %result = lshr i64 %val, 32
  ret i64 %result
}
; CHECK-LABEL: lshrImm64Equal32
; CHECK-NOT: shr
; MIPS32-LABEL: lshrImm64Equal32
; MIPS32: li	{{.*}},0
; MIPS32-O2: move
; MIPS32-OM1: sw
; MIPS32-OM1: lw

define internal i64 @lshrImm64GreaterThan32(i64 %val) {
entry:
  %result = lshr i64 %val, 40
  ret i64 %result
}
; CHECK-LABEL: lshrImm64GreaterThan32
; CHECK-NOT: shrd
; CHECK: shr {{.*}},0x8
; MIPS32-LABEL: lshrImm64GreaterThan32
; MIPS32: srl	{{.*}},{{.*}},0x8
; MIPS32: li	{{.*}},0

define internal i64 @ashrImm64One(i64 %val) {
entry:
  %result = ashr i64 %val, 1
  ret i64 %result
}
; CHECK-LABEL: ashrImm64One
; CHECK: shrd {{.*}},0x1
; CHECK: sar {{.*}},1
; MIPS32-LABEL: ashrImm64One
; MIPS32: sll	[[T1:.*]],[[VAL_HI:.*]],0x1f
; MIPS32: srl	[[T2:.*]],{{.*}},0x1
; MIPS32: or	{{.*}},[[T1]],[[T2]]
; MIPS32: sra	{{.*}},[[VAL_HI]],0x1

define internal i64 @ashrImm64LessThan32(i64 %val) {
entry:
  %result = ashr i64 %val, 4
  ret i64 %result
}
; CHECK-LABEL: ashrImm64LessThan32
; CHECK: shrd {{.*}},0x4
; CHECK: sar {{.*}},0x4
; MIPS32-LABEL: ashrImm64LessThan32
; MIPS32: sll   [[T1:.*]],[[VAL_HI:.*]],0x1c
; MIPS32: srl   [[T2:.*]],{{.*}},0x4
; MIPS32: or    {{.*}},[[T1]],[[T2]]
; MIPS32: sra   {{.*}},[[VAL_HI]],0x4

define internal i64 @ashrImm64Equal32(i64 %val) {
entry:
  %result = ashr i64 %val, 32
  ret i64 %result
}
; CHECK-LABEL: ashrImm64Equal32
; CHECK: sar {{.*}},0x1f
; CHECK-NOT: shrd
; MIPS32-LABEL: ashrImm64Equal32
; MIPS32: sra	{{.*}},[[VAL_HI:.*]],0x1f
; MIPS32-O2: move	{{.*}},[[VAL_HI]]
; MIPS32-OM1:	sw [[VAL_HI]],{{.*}}
; MIPS32-OM1:   lw {{.*}},{{.*}}

define internal i64 @ashrImm64GreaterThan32(i64 %val) {
entry:
  %result = ashr i64 %val, 40
  ret i64 %result
}
; CHECK-LABEL: ashrImm64GreaterThan32
; CHECK: sar {{.*}},0x1f
; CHECK: shrd {{.*}},0x8
; MIPS32-LABEL: ashrImm64GreaterThan32
; MIPS32: sra	{{.*}},[[VAL_HI:.*]],0x8
; MIPS32: sra	{{.*}},[[VAL_HI]],0x1f
