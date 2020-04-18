; Assembly test for simple arithmetic operations.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj --disassemble --target arm32 \
; RUN:   -i %s --args -O2 \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 --check-prefix ARM-OPT2 %s
; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj --disassemble --target arm32 \
; RUN:   -i %s --args -O2 --mattr=hwdiv-arm \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32HWDIV %s
; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj --disassemble --target arm32 \
; RUN:   -i %s --args -Om1 \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 --check-prefix ARM32-OPTM1 %s
;
; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target mips32\
; RUN:   -i %s --args -O2 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target mips32\
; RUN:   -i %s --args -Om1 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

define internal i32 @Add(i32 %a, i32 %b) {
entry:
  %add = add i32 %b, %a
  ret i32 %add
}
; CHECK-LABEL: Add
; CHECK: add e
; ARM32-LABEL: Add
; ARM32: add r
; MIPS32-LABEL: Add
; MIPS32: add

define internal i32 @And(i32 %a, i32 %b) {
entry:
  %and = and i32 %b, %a
  ret i32 %and
}
; CHECK-LABEL: And
; CHECK: and e
; ARM32-LABEL: And
; ARM32: and r
; MIPS32-LABEL: And
; MIPS32: and

define internal i32 @Or(i32 %a, i32 %b) {
entry:
  %or = or i32 %b, %a
  ret i32 %or
}
; CHECK-LABEL: Or
; CHECK: or e
; ARM32-LABEL: Or
; ARM32: orr r
; MIPS32-LABEL: Or
; MIPS32: or

define internal i32 @Xor(i32 %a, i32 %b) {
entry:
  %xor = xor i32 %b, %a
  ret i32 %xor
}
; CHECK-LABEL: Xor
; CHECK: xor e
; ARM32-LABEL: Xor
; ARM32: eor r
; MIPS32-LABEL: Xor
; MIPS32: xor

define internal i32 @Sub(i32 %a, i32 %b) {
entry:
  %sub = sub i32 %a, %b
  ret i32 %sub
}
; CHECK-LABEL: Sub
; CHECK: sub e
; ARM32-LABEL: Sub
; ARM32: sub r
; MIPS32-LABEL: Sub
; MIPS32: sub

define internal i32 @Mul(i32 %a, i32 %b) {
entry:
  %mul = mul i32 %b, %a
  ret i32 %mul
}
; CHECK-LABEL: Mul
; CHECK: imul e
; ARM32-LABEL: Mul
; ARM32: mul r
; MIPS32-LABEL: Mul
; MIPS32: mul

; Check for a valid ARM mul instruction where operands have to be registers.
; On the other hand x86-32 does allow an immediate.
define internal i32 @MulImm(i32 %a, i32 %b) {
entry:
  %mul = mul i32 %a, 99
  ret i32 %mul
}
; CHECK-LABEL: MulImm
; CHECK: imul e{{.*}},e{{.*}},0x63
; ARM32-LABEL: MulImm
; ARM32-OPTM1: mov {{.*}}, #99
; ARM32-OPTM1: mul r{{.*}}, r{{.*}}, r{{.*}}
; ARM32-OPT2: rsb [[T:r[0-9]+]], [[S:r[0-9]+]], [[S]], lsl #2
; ARM32-OPT2-DAG: add [[T]], [[T]], [[S]], lsl #7
; ARM32-OPT2-DAG: sub [[T]], [[T]], [[S]], lsl #5
; MIPS32-LABEL: MulImm
; MIPS32: mul

; Check for a valid addressing mode in the x86-32 mul instruction when
; the second source operand is an immediate.
define internal i64 @MulImm64(i64 %a) {
entry:
  %mul = mul i64 %a, 99
  ret i64 %mul
}
; NOTE: the lowering is currently a bit inefficient for small 64-bit constants.
; The top bits of the immediate are 0, but the instructions modeling that
; multiply by 0 are not eliminated (see expanded 64-bit ARM lowering).
; CHECK-LABEL: MulImm64
; CHECK: mov {{.*}},0x63
; CHECK: mov {{.*}},0x0
; CHECK-NOT: mul {{[0-9]+}}
;
; ARM32-LABEL: MulImm64
; ARM32: mov {{.*}}, #99
; ARM32: mov {{.*}}, #0
; ARM32: mul r
; ARM32: mla r
; ARM32: umull r
; ARM32: add r

; MIPS32-LABEL: MulImm64

define internal i32 @Sdiv(i32 %a, i32 %b) {
entry:
  %div = sdiv i32 %a, %b
  ret i32 %div
}
; CHECK-LABEL: Sdiv
; CHECK: cdq
; CHECK: idiv e
;
; ARM32-LABEL: Sdiv
; ARM32: tst [[DENOM:r.*]], [[DENOM]]
; ARM32: bne
; The following instruction is ".word 0xe7fedef0 = udf #60896 ; 0xede0".
; ARM32: e7fedef0
; ARM32: bl {{.*}} __divsi3
; ARM32HWDIV-LABEL: Sdiv
; ARM32HWDIV: tst
; ARM32HWDIV: bne
; ARM32HWDIV: sdiv

; MIPS32-LABEL: Sdiv
; MIPS32: div zero,{{.*}},[[REG:.*]]
; MIPS32: teq [[REG]],zero,0x7
; MIPS32: mflo

define internal i32 @SdivConst(i32 %a) {
entry:
  %div = sdiv i32 %a, 219
  ret i32 %div
}
; CHECK-LABEL: SdivConst
; CHECK: cdq
; CHECK: idiv e
;
; ARM32-LABEL: SdivConst
; ARM32-NOT: tst
; ARM32: bl {{.*}} __divsi3
; ARM32HWDIV-LABEL: SdivConst
; ARM32HWDIV-NOT: tst
; ARM32HWDIV: sdiv

; MIPS32-LABEL: SdivConst
; MIPS32: div zero,{{.*}},[[REG:.*]]
; MIPS32: teq [[REG]],zero,0x7
; MIPS32: mflo

define internal i32 @Srem(i32 %a, i32 %b) {
entry:
  %rem = srem i32 %a, %b
  ret i32 %rem
}
; CHECK-LABEL: Srem
; CHECK: cdq
; CHECK: idiv e
;
; ARM32-LABEL: Srem
; ARM32: tst [[DENOM:r.*]], [[DENOM]]
; ARM32: bne
; ARM32: bl {{.*}} __modsi3
; ARM32HWDIV-LABEL: Srem
; ARM32HWDIV: tst
; ARM32HWDIV: bne
; ARM32HWDIV: sdiv
; ARM32HWDIV: mls

; MIPS32-LABEL: Srem
; MIPS32: div zero,{{.*}},[[REG:.*]]
; MIPS32: teq [[REG]],zero,0x7
; MIPS32: mfhi

define internal i32 @Udiv(i32 %a, i32 %b) {
entry:
  %div = udiv i32 %a, %b
  ret i32 %div
}
; CHECK-LABEL: Udiv
; CHECK: div e
;
; ARM32-LABEL: Udiv
; ARM32: tst [[DENOM:r.*]], [[DENOM]]
; ARM32: bne
; ARM32: bl {{.*}} __udivsi3
; ARM32HWDIV-LABEL: Udiv
; ARM32HWDIV: tst
; ARM32HWDIV: bne
; ARM32HWDIV: udiv

; MIPS32-LABEL: Udiv
; MIPS32: divu zero,{{.*}},[[REG:.*]]
; MIPS32: teq [[REG]],zero,0x7
; MIPS32: mflo

define internal i32 @Urem(i32 %a, i32 %b) {
entry:
  %rem = urem i32 %a, %b
  ret i32 %rem
}
; CHECK-LABEL: Urem
; CHECK: div e
;
; ARM32-LABEL: Urem
; ARM32: tst [[DENOM:r.*]], [[DENOM]]
; ARM32: bne
; ARM32: bl {{.*}} __umodsi3
; ARM32HWDIV-LABEL: Urem
; ARM32HWDIV: tst
; ARM32HWDIV: bne
; ARM32HWDIV: udiv
; ARM32HWDIV: mls

; MIPS32-LABEL: Urem
; MIPS32: divu zero,{{.*}},[[REG:.*]]
; MIPS32: teq [[REG]],zero,0x7
; MIPS32: mfhi

; The following tests check that shift instructions don't try to use a
; ConstantRelocatable as an immediate operand.

@G = internal global [4 x i8] zeroinitializer, align 4

define internal i32 @ShlReloc(i32 %a) {
entry:
  %opnd = ptrtoint [4 x i8]* @G to i32
  %result = shl i32 %a, %opnd
  ret i32 %result
}
; CHECK-LABEL: ShlReloc
; CHECK: shl {{.*}},cl

; MIPS32-LABEL: ShlReloc
; MIPS32: lui [[REG:.*]],{{.*}} R_MIPS_HI16 G
; MIPS32: addiu [[REG]],[[REG]],{{.*}} R_MIPS_LO16 G
; MIPS32: sllv {{.*}},{{.*}},[[REG]]

define internal i32 @LshrReloc(i32 %a) {
entry:
  %opnd = ptrtoint [4 x i8]* @G to i32
  %result = lshr i32 %a, %opnd
  ret i32 %result
}
; CHECK-LABEL: LshrReloc
; CHECK: shr {{.*}},cl

; MIPS32-LABEL: LshrReloc
; MIPS32: lui [[REG:.*]],{{.*}} R_MIPS_HI16 G
; MIPS32: addiu [[REG]],[[REG]],{{.*}} R_MIPS_LO16 G
; MIPS32: srlv {{.*}},{{.*}},[[REG]]

define internal i32 @AshrReloc(i32 %a) {
entry:
  %opnd = ptrtoint [4 x i8]* @G to i32
  %result = ashr i32 %a, %opnd
  ret i32 %result
}
; CHECK-LABEL: AshrReloc
; CHECK: sar {{.*}},cl

; MIPS32-LABEL: AshrReloc
; MIPS32: lui [[REG:.*]],{{.*}} R_MIPS_HI16 G
; MIPS32: addiu [[REG]],[[REG]],{{.*}} R_MIPS_LO16 G
; MIPS32: srav {{.*}},{{.*}},[[REG]]
