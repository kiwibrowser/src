; Tests various aspects of i1 related lowering.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -Om1 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; TODO(jpp): Switch to --filetype=obj when possible.
; RUN: %if --need=target_ARM32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target arm32 -i %s --args -O2 \
; RUN:   | %if --need=target_ARM32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix ARM32 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; Test that and with true uses immediate 1, not -1.
define internal i32 @testAndTrue(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result_i1 = and i1 %arg_i1, true
  %result = zext i1 %result_i1 to i32
  ret i32 %result
}
; CHECK-LABEL: testAndTrue
; CHECK: and {{.*}},0x1
; ARM32-LABEL: testAndTrue
; ARM32: and {{.*}}, #1
; MIPS32-LABEL: testAndTrue
; MIPS32: 	andi	{{.*}},0x1

; Test that or with true uses immediate 1, not -1.
define internal i32 @testOrTrue(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result_i1 = or i1 %arg_i1, true
  %result = zext i1 %result_i1 to i32
  ret i32 %result
}
; CHECK-LABEL: testOrTrue
; CHECK: or {{.*}},0x1
; ARM32-LABEL: testOrTrue
; ARM32: orr {{.*}}, #1
; MIPS32-LABEL: testOrTrue
; MIPS32: 	ori	{{.*}},0x1

; Test that xor with true uses immediate 1, not -1.
define internal i32 @testXorTrue(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result_i1 = xor i1 %arg_i1, true
  %result = zext i1 %result_i1 to i32
  ret i32 %result
}
; CHECK-LABEL: testXorTrue
; CHECK: xor {{.*}},0x1
; ARM32-LABEL: testXorTrue
; ARM32: eor {{.*}}, #1
; MIPS32-LABEL: testXorTrue
; MIPS32: 	xori	{{.*}},0x1

; Test that trunc to i1 masks correctly.
define internal i32 @testTrunc(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result = zext i1 %arg_i1 to i32
  ret i32 %result
}
; CHECK-LABEL: testTrunc
; CHECK: and {{.*}},0x1
; ARM32-LABEL: testTrunc
; ARM32: and {{.*}}, #1
; MIPS32-LABEL: testTrunc
; MIPS32: 	andi	{{.*}},0x1

; Test zext to i8.
define internal i32 @testZextI8(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result_i8 = zext i1 %arg_i1 to i8
  %result = zext i8 %result_i8 to i32
  ret i32 %result
}
; CHECK-LABEL: testZextI8
; match the trunc instruction
; CHECK: and {{.*}},0x1
; match the zext i1 instruction (NOTE: no mov need between i1 and i8).
; CHECK-NOT: and {{.*}},0x1
; ARM32-LABEL: testZextI8
; ARM32: {{.*}}, #1
; ARM32: uxtb
; MIPS32-LABEL: testZextI8
; MIPS32: 	andi	{{.*}},0x1
; MIPS32: 	andi	{{.*}},0xff

; Test zext to i16.
define internal i32 @testZextI16(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result_i16 = zext i1 %arg_i1 to i16
  %result = zext i16 %result_i16 to i32
  ret i32 %result
}
; CHECK-LABEL: testZextI16
; match the trunc instruction
; CHECK: and {{.*}},0x1
; match the zext i1 instruction (note 32-bit reg is used because it's shorter).
; CHECK: movzx [[REG:e.*]],{{[a-d]l|BYTE PTR}}
; CHECK-NOT: and [[REG]],0x1

; ARM32-LABEL: testZextI16
; ARM32: and {{.*}}, #1
; ARM32: uxth

; MIPS32-LABEL: testZextI16
; MIPS32: 	andi	{{.*}},0x1
; MIPS32: 	andi	{{.*}},0xffff

; Test zext to i32.
define internal i32 @testZextI32(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result_i32 = zext i1 %arg_i1 to i32
  ret i32 %result_i32
}
; CHECK-LABEL: testZextI32
; match the trunc instruction
; CHECK: and {{.*}},0x1
; match the zext i1 instruction
; CHECK: movzx
; CHECK-NOT: and {{.*}},0x1
; ARM32-LABEL: testZextI32
; ARM32: and {{.*}}, #1
; MIPS32-LABEL: testZextI32
; MIPS32: 	andi	{{.*}},0x1

; Test zext to i64.
define internal i64 @testZextI64(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result_i64 = zext i1 %arg_i1 to i64
  ret i64 %result_i64
}
; CHECK-LABEL: testZextI64
; match the trunc instruction
; CHECK: and {{.*}},0x1
; match the zext i1 instruction
; CHECK: movzx
; CHECK: mov {{.*}},0x0
; ARM32-LABEL: testZextI64
; ARM32: and {{.*}}, #1
; ARM32: mov {{.*}}, #0
; MIPS32-LABEL: testZextI64
; MIPS32: 	andi	{{.*}},0x1
; MIPS32: 	li	{{.*}},0
; MIPS32: 	move
; MIPS32: 	move

; Test sext to i8.
define internal i32 @testSextI8(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result_i8 = sext i1 %arg_i1 to i8
  %result = sext i8 %result_i8 to i32
  ret i32 %result
}
; CHECK-LABEL: testSextI8
; match the trunc instruction
; CHECK: and {{.*}},0x1
; match the sext i1 instruction
; CHECK: shl [[REG:.*]],0x7
; CHECK-NEXT: sar [[REG]],0x7
;
; ARM32-LABEL: testSextI8
; ARM32: mov {{.*}}, #0
; ARM32: tst {{.*}}, #1
; ARM32: mvn {{.*}}, #0
; ARM32: movne
; ARM32: sxtb
;
; MIPS32-LABEL: testSextI8
; MIPS32: 	sll	{{.*}},0x1f
; MIPS32: 	sra	{{.*}},0x1f
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	sra	{{.*}},0x18

; Test sext to i16.
define internal i32 @testSextI16(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result_i16 = sext i1 %arg_i1 to i16
  %result = sext i16 %result_i16 to i32
  ret i32 %result
}
; CHECK-LABEL: testSextI16
; match the trunc instruction
; CHECK: and {{.*}},0x1
; match the sext i1 instruction
; CHECK: movzx {{e*}}[[REG:.*]],{{[a-d]l|BYTE PTR}}
; CHECK-NEXT: shl [[REG]],0xf
; CHECK-NEXT: sar [[REG]],0xf

; ARM32-LABEL: testSextI16
; ARM32: mov {{.*}}, #0
; ARM32: tst {{.*}}, #1
; ARM32: mvn {{.*}}, #0
; ARM32: movne
; ARM32: sxth

; MIPS32-LABEL: testSextI16
; MIPS32: 	sll	{{.*}},0x1f
; MIPS32: 	sra	{{.*}},0x1f
; MIPS32: 	sll	{{.*}},0x10
; MIPS32: 	sra	{{.*}},0x10

; Test sext to i32.
define internal i32 @testSextI32(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result_i32 = sext i1 %arg_i1 to i32
  ret i32 %result_i32
}
; CHECK-LABEL: testSextI32
; match the trunc instruction
; CHECK: and {{.*}},0x1
; match the sext i1 instruction
; CHECK: movzx [[REG:.*]],
; CHECK-NEXT: shl [[REG]],0x1f
; CHECK-NEXT: sar [[REG]],0x1f

; ARM32-LABEL: testSextI32
; ARM32: mov {{.*}}, #0
; ARM32: tst {{.*}}, #1
; ARM32: mvn {{.*}}, #0
; ARM32: movne

; MIPS32-LABEL: testSextI32
; MIPS32: 	sll	{{.*}},0x1f
; MIPS32: 	sra	{{.*}},0x1f

; Test sext to i64.
define internal i64 @testSextI64(i32 %arg) {
entry:
  %arg_i1 = trunc i32 %arg to i1
  %result_i64 = sext i1 %arg_i1 to i64
  ret i64 %result_i64
}
; CHECK-LABEL: testSextI64
; match the trunc instruction
; CHECK: and {{.*}},0x1
; match the sext i1 instruction
; CHECK: movzx [[REG:.*]],
; CHECK-NEXT: shl [[REG]],0x1f
; CHECK-NEXT: sar [[REG]],0x1f

; ARM32-LABEL: testSextI64
; ARM32: mov {{.*}}, #0
; ARM32: tst {{.*}}, #1
; ARM32: mvn {{.*}}, #0
; ARM32: movne [[REG:r[0-9]+]]
; ARM32: mov {{.*}}, [[REG]]

; MIPS32-LABEL: testSextI64
; MIPS32: 	sll	{{.*}},0x1f
; MIPS32: 	sra	{{.*}},0x1f
; MIPS32: 	move
; MIPS32: 	move

; Kind of like sext i1 to i32, but with an immediate source. On ARM,
; sxtb cannot take an immediate operand, so make sure it's using a reg.
; If we had optimized constants, this could just be mov dst, 0xffffffff
; or mvn dst, #0.
define internal i32 @testSextTrue() {
  %result = sext i1 true to i32
  ret i32 %result
}
; CHECK-LABEL: testSextTrue
; CHECK: movzx
; CHECK-NEXT: shl
; CHECK-NEXT: sar
; ARM32-LABEL: testSextTrue
; ARM32: mov {{.*}}, #0
; ARM32: tst {{.*}}, #1
; ARM32: mvn {{.*}}, #0
; ARM32: movne
; MIPS32-LABEL: testSextTrue
; MIPS32: 	li	{{.*}},1
; MIPS32: 	sll	{{.*}},0x1f
; MIPS32: 	sra	{{.*}},0x1f

define internal i32 @testZextTrue() {
  %result = zext i1 true to i32
  ret i32 %result
}
; CHECK-LABEL: testZextTrue
; CHECK: movzx
; CHECK-NOT: and {{.*}},0x1
; ARM32-LABEL: testZextTrue
; ARM32: mov{{.*}}, #1
; ARM32: and {{.*}}, #1
; MIPS32-LABEL: testZextTrue
; MIPS32: 	li	{{.*}},1
; MIPS32: 	andi	{{.*}},0x1

; Test fptosi float to i1.
define internal i32 @testFptosiFloat(float %arg) {
entry:
  %arg_i1 = fptosi float %arg to i1
  %result = sext i1 %arg_i1 to i32
  ret i32 %result
}
; CHECK-LABEL: testFptosiFloat
; CHECK: cvttss2si
; CHECK: and {{.*}},0x1
; CHECK: movzx [[REG:.*]],
; CHECK-NEXT: shl [[REG]],0x1f
; CHECK-NEXT: sar [[REG]],0x1f
; MIPS32-LABEL: testFptosiFloat
; MIPS32: 	trunc.w.s
; MIPS32: 	mfc1
; MIPS32: 	sll	{{.*}},0x1f
; MIPS32: 	sra	{{.*}},0x1f

; Test fptosi double to i1.
define internal i32 @testFptosiDouble(double %arg) {
entry:
  %arg_i1 = fptosi double %arg to i1
  %result = sext i1 %arg_i1 to i32
  ret i32 %result
}
; CHECK-LABEL: testFptosiDouble
; CHECK: cvttsd2si
; CHECK: and {{.*}},0x1
; CHECK: movzx [[REG:.*]],
; CHECK-NEXT: shl [[REG]],0x1f
; CHECK-NEXT: sar [[REG]],0x1f
; MIPS32-LABEL: testFptosiDouble
; MIPS32: 	trunc.w.d
; MIPS32: 	mfc1
; MIPS32: 	sll	{{.*}},0x1f
; MIPS32: 	sra	{{.*}},0x1f
