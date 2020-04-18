; This tries to be a comprehensive test of i8 operations.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s
; RUN: %p2i --filetype=obj --disassemble -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s

; The following tests i8 srem/urem lowering on x86-64, specifically that the %ah
; result gets copied into %al/%bl/%cl/%dl before moved into its final register.
; This extra copy is forced by excluding al/bl/cl/dl by default (-reg-exclude),
; but allowing them to be used if absolutely necessary (-reg-reserve).

; RUN: %p2i --target=x8664 --filetype=obj --disassemble -i %s --args -O2 \
; RUN:   -reg-exclude=al,bl,cl,dl -reg-reserve \
; RUN:   -allow-externally-defined-symbols | FileCheck %s --check-prefix=REM

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

declare void @useInt(i32 %x)

define internal i32 @add8Bit(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %add = add i8 %b_8, %a_8
  %ret = zext i8 %add to i32
  ret i32 %ret
}
; CHECK-LABEL: add8Bit
; CHECK: add {{[abcd]l}}
; MIPS32-LABEL: add8Bit
; MIPS32: 	addu
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @add8BitConst(i32 %a) {
entry:
  %a_8 = trunc i32 %a to i8
  %add = add i8 %a_8, 123
  %ret = zext i8 %add to i32
  ret i32 %ret
}
; CHECK-LABEL: add8BitConst
; CHECK: add {{[abcd]l}}
; MIPS32-LABEL: add8BitConst
; MIPS32: 	addiu
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @sub8Bit(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %sub = sub i8 %b_8, %a_8
  %ret = zext i8 %sub to i32
  ret i32 %ret
}
; CHECK-LABEL: sub8Bit
; CHECK: sub {{[abcd]l}}
; MIPS32-LABEL: sub8Bit
; MIPS32: 	subu
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @sub8BitConst(i32 %a) {
entry:
  %a_8 = trunc i32 %a to i8
  %sub = sub i8 %a_8, 123
  %ret = zext i8 %sub to i32
  ret i32 %ret
}
; CHECK-LABEL: sub8BitConst
; CHECK: sub {{[abcd]l}}
; MIPS32-LABEL: sub8BitConst
; MIPS32: 	addiu   {{.*}},-123
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @mul8Bit(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %mul = mul i8 %b_8, %a_8
  %ret = zext i8 %mul to i32
  ret i32 %ret
}
; CHECK-LABEL: mul8Bit
; CHECK: mul {{[abcd]l|BYTE PTR}}
; MIPS32-LABEL: mul8Bit
; MIPS32: 	mul
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @mul8BitConst(i32 %a) {
entry:
  %a_8 = trunc i32 %a to i8
  %mul = mul i8 %a_8, 56
  %ret = zext i8 %mul to i32
  ret i32 %ret
}
; CHECK-LABEL: mul8BitConst
; 8-bit imul only accepts r/m, not imm
; CHECK: mov {{.*}},0x38
; CHECK: mul {{[abcd]l|BYTE PTR}}
; MIPS32-LABEL: mul8BitConst
; MIPS32: 	li
; MIPS32: 	mul
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @udiv8Bit(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %udiv = udiv i8 %b_8, %a_8
  %ret = zext i8 %udiv to i32
  ret i32 %ret
}
; CHECK-LABEL: udiv8Bit
; CHECK: div {{[abcd]l|BYTE PTR}}
; MIPS32-LABEL: udiv8Bit
; MIPS32: 	divu
; MIPS32: 	teq
; MIPS32: 	mflo
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @udiv8BitConst(i32 %a) {
entry:
  %a_8 = trunc i32 %a to i8
  %udiv = udiv i8 %a_8, 123
  %ret = zext i8 %udiv to i32
  ret i32 %ret
}
; CHECK-LABEL: udiv8BitConst
; CHECK: div {{[abcd]l|BYTE PTR}}
; MIPS32-LABEL: udiv8BitConst
; MIPS32: 	li
; MIPS32: 	divu
; MIPS32: 	teq
; MIPS32: 	mflo
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @urem8Bit(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %urem = urem i8 %b_8, %a_8
  %ret = zext i8 %urem to i32
  ret i32 %ret
}
; CHECK-LABEL: urem8Bit
; CHECK: div {{[abcd]l|BYTE PTR}}
; REM-LABEL: urem8Bit
; REM: div
; REM-NEXT: mov {{[abcd]}}l,ah
; MIPS32-LABEL: urem8Bit
; MIPS32: 	divu
; MIPS32: 	teq
; MIPS32: 	mfhi
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @urem8BitConst(i32 %a) {
entry:
  %a_8 = trunc i32 %a to i8
  %urem = urem i8 %a_8, 123
  %ret = zext i8 %urem to i32
  ret i32 %ret
}
; CHECK-LABEL: urem8BitConst
; CHECK: div {{[abcd]l|BYTE PTR}}
; REM-LABEL: urem8BitConst
; MIPS32-LABEL: urem8BitConst
; MIPS32: 	li
; MIPS32: 	divu
; MIPS32: 	teq
; MIPS32: 	mfhi
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	jr
; MIPS32: 	nop


define internal i32 @sdiv8Bit(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %sdiv = sdiv i8 %b_8, %a_8
  %ret = zext i8 %sdiv to i32
  ret i32 %ret
}
; CHECK-LABEL: sdiv8Bit
; CHECK: idiv {{[abcd]l|BYTE PTR}}
; MIPS32-LABEL: sdiv8Bit
; MIPS32: 	div
; MIPS32: 	teq
; MIPS32: 	mflo
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @sdiv8BitConst(i32 %a) {
entry:
  %a_8 = trunc i32 %a to i8
  %sdiv = sdiv i8 %a_8, 123
  %ret = zext i8 %sdiv to i32
  ret i32 %ret
}
; CHECK-LABEL: sdiv8BitConst
; CHECK: idiv {{[abcd]l|BYTE PTR}}
; MIPS32-LABEL: sdiv8BitConst
; MIPS32: 	li
; MIPS32: 	div
; MIPS32: 	teq
; MIPS32: 	mflo
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @srem8Bit(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %srem = srem i8 %b_8, %a_8
  %ret = zext i8 %srem to i32
  ret i32 %ret
}
; CHECK-LABEL: srem8Bit
; CHECK: idiv {{[abcd]l|BYTE PTR}}
; REM-LABEL: srem8Bit
; REM: idiv
; REM-NEXT: mov {{[abcd]}}l,ah
; MIPS32-LABEL: srem8Bit
; MIPS32: 	div
; MIPS32: 	teq
; MIPS32: 	mfhi
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @srem8BitConst(i32 %a) {
entry:
  %a_8 = trunc i32 %a to i8
  %srem = srem i8 %a_8, 123
  %ret = zext i8 %srem to i32
  ret i32 %ret
}
; CHECK-LABEL: srem8BitConst
; CHECK: idiv {{[abcd]l|BYTE PTR}}
; REM-LABEL: srem8BitConst
; MIPS32-LABEL: srem8BitConst
; MIPS32: 	li
; MIPS32: 	div
; MIPS32: 	teq
; MIPS32: 	mfhi
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @shl8Bit(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %shl = shl i8 %b_8, %a_8
  %ret = zext i8 %shl to i32
  ret i32 %ret
}
; CHECK-LABEL: shl8Bit
; CHECK: shl {{[abd]l|BYTE PTR}},cl
; MIPS32-LABEL: shl8Bit
; MIPS32: 	sllv
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @shl8BitConst(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %shl = shl i8 %a_8, 6
  %ret = zext i8 %shl to i32
  ret i32 %ret
}
; CHECK-LABEL: shl8BitConst
; CHECK: shl {{[abcd]l|BYTE PTR}},0x6
; MIPS32-LABEL: shl8BitConst
; MIPS32: 	sll
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @lshr8Bit(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %lshr = lshr i8 %b_8, %a_8
  %ret = zext i8 %lshr to i32
  ret i32 %ret
}
; CHECK-LABEL: lshr8Bit
; CHECK: shr {{[abd]l|BYTE PTR}},cl
; MIPS32-LABEL: lshr8Bit
; MIPS32: 	srlv
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @lshr8BitConst(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %lshr = lshr i8 %a_8, 6
  %ret = zext i8 %lshr to i32
  ret i32 %ret
}
; CHECK-LABEL: lshr8BitConst
; CHECK: shr {{[abcd]l|BYTE PTR}},0x6
; MIPS32-LABEL: lshr8BitConst
; MIPS32: 	srl
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @ashr8Bit(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %ashr = ashr i8 %b_8, %a_8
  %ret = zext i8 %ashr to i32
  ret i32 %ret
}
; CHECK-LABEL: ashr8Bit
; CHECK: sar {{[abd]l|BYTE PTR}},cl
; MIPS32-LABEL: ashr8Bit
; MIPS32: 	sra
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @ashr8BitConst(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %ashr = ashr i8 %a_8, 6
  %ret = zext i8 %ashr to i32
  ret i32 %ret
}
; CHECK-LABEL: ashr8BitConst
; CHECK: sar {{[abcd]l|BYTE PTR}},0x6
; MIPS32-LABEL: ashr8BitConst
; MIPS32: 	sra
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @icmp8Bit(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %icmp = icmp ne i8 %b_8, %a_8
  %ret = zext i1 %icmp to i32
  ret i32 %ret
}
; CHECK-LABEL: icmp8Bit
; CHECK: cmp {{[abcd]l|BYTE PTR}}
; MIPS32-LABEL: icmp8Bit
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	xor
; MIPS32: 	sltu
; MIPS32: 	andi	{{.*}},0x1
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @icmp8BitConst(i32 %a) {
entry:
  %a_8 = trunc i32 %a to i8
  %icmp = icmp ne i8 %a_8, 123
  %ret = zext i1 %icmp to i32
  ret i32 %ret
}
; CHECK-LABEL: icmp8BitConst
; CHECK: cmp {{[abcd]l|BYTE PTR}}
; MIPS32-LABEL: icmp8BitConst
; MIPS32: 	li
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	xor
; MIPS32: 	sltu
; MIPS32: 	andi	{{.*}},0x1
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @icmp8BitConstSwapped(i32 %a) {
entry:
  %a_8 = trunc i32 %a to i8
  %icmp = icmp ne i8 123, %a_8
  %ret = zext i1 %icmp to i32
  ret i32 %ret
}
; CHECK-LABEL: icmp8BitConstSwapped
; CHECK: cmp {{[abcd]l|BYTE PTR}}
; MIPS32-LABEL: icmp8BitConstSwapped
; MIPS32: 	li
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	xor	v0,v0,a0
; MIPS32: 	sltu
; MIPS32: 	andi	{{.*}},0x1
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @icmp8BitMem(i32 %a, i32 %b_iptr) {
entry:
  %a_8 = trunc i32 %a to i8
  %bptr = inttoptr i32 %b_iptr to i8*
  %b_8 = load i8, i8* %bptr, align 1
  %icmp = icmp ne i8 %b_8, %a_8
  %ret = zext i1 %icmp to i32
  ret i32 %ret
}
; CHECK-LABEL: icmp8BitMem
; CHECK: cmp {{[abcd]l|BYTE PTR}}
; MIPS32-LABEL: icmp8BitMem
; MIPS32: 	lb
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	xor
; MIPS32: 	sltu
; MIPS32: 	andi	{{.*}},0x1
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @icmp8BitMemSwapped(i32 %a, i32 %b_iptr) {
entry:
  %a_8 = trunc i32 %a to i8
  %bptr = inttoptr i32 %b_iptr to i8*
  %b_8 = load i8, i8* %bptr, align 1
  %icmp = icmp ne i8 %a_8, %b_8
  %ret = zext i1 %icmp to i32
  ret i32 %ret
}
; CHECK-LABEL: icmp8BitMemSwapped
; CHECK: cmp {{[abcd]l|BYTE PTR}}
; MIPS32-LABEL: icmp8BitMemSwapped
; MIPS32: 	lb
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	xor
; MIPS32: 	sltu
; MIPS32: 	andi	{{.*}},0x1
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @selectI8Var(i32 %a, i32 %b) {
entry:
  %a_8 = trunc i32 %a to i8
  %b_8 = trunc i32 %b to i8
  %cmp = icmp slt i8 %a_8, %b_8
  %ret = select i1 %cmp, i8 %a_8, i8 %b_8
  %ret_ext = zext i8 %ret to i32
  ; Create a "fake" use of %cmp to prevent O2 bool folding.
  %d1 = zext i1 %cmp to i32
  call void @useInt(i32 %d1)
  ret i32 %ret_ext
}
; CHECK-LABEL: selectI8Var
; CHECK: cmp
; CHECK: setl
; CHECK: mov {{[a-d]l}}
; MIPS32-LABEL: selectI8Var
; MIPS32: 	addiu
; MIPS32: 	sw
; MIPS32: 	sw
; MIPS32: 	move
; MIPS32: 	move
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	slt
; MIPS32: 	move
; MIPS32: 	movn
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	andi	{{.*}},0x1
; MIPS32: 	move
; MIPS32: 	jal
; MIPS32: 	nop
; MIPS32: 	move
; MIPS32: 	lw
; MIPS32: 	lw
; MIPS32: 	addiu
; MIPS32: 	jr
; MIPS32: 	nop

define internal i32 @testPhi8(i32 %arg, i32 %arg2, i32 %arg3, i32 %arg4, i32 %arg5, i32 %arg6, i32 %arg7, i32 %arg8, i32 %arg9, i32 %arg10) {
entry:
  %trunc = trunc i32 %arg to i8
  %trunc2 = trunc i32 %arg2 to i8
  %trunc3 = trunc i32 %arg3 to i8
  %trunc4 = trunc i32 %arg4 to i8
  %trunc5 = trunc i32 %arg5 to i8
  %cmp1 = icmp sgt i32 %arg, 0
  br i1 %cmp1, label %next, label %target
next:
  %trunc6_16 = trunc i32 %arg6 to i16
  %trunc7_16 = trunc i32 %arg7 to i16
  %trunc8_16 = trunc i32 %arg8 to i16
  %trunc9 = trunc i32 %arg9 to i8
  %trunc10 = trunc i32 %arg10 to i8
  %trunc7_8 = trunc i16 %trunc7_16 to i8
  %trunc6_8 = trunc i16 %trunc6_16 to i8
  %trunc8_8 = trunc i16 %trunc8_16 to i8
  br label %target
target:
  %merge1 = phi i1 [ %cmp1, %entry ], [ false, %next ]
  %merge2 = phi i8 [ %trunc, %entry ], [ %trunc6_8, %next ]
  %merge3 = phi i8 [ %trunc2, %entry ], [ %trunc7_8, %next ]
  %merge5 = phi i8 [ %trunc4, %entry ], [ %trunc9, %next ]
  %merge6 = phi i8 [ %trunc5, %entry ], [ %trunc10, %next ]
  %merge4 = phi i8 [ %trunc3, %entry ], [ %trunc8_8, %next ]
  %res1 = select i1 %merge1, i8 %merge2, i8 %merge3
  %res2 = select i1 %merge1, i8 %merge4, i8 %merge5
  %res1_2 = select i1 %merge1, i8 %res1, i8 %res2
  %res123 = select i1 %merge1, i8 %merge6, i8 %res1_2
  %result = zext i8 %res123 to i32
  ret i32 %result
}
; CHECK-LABEL: testPhi8
; This assumes there will be some copy from an 8-bit register / stack slot.
; CHECK-DAG: mov {{.*}},{{[a-d]}}l
; CHECK-DAG: mov {{.*}},BYTE PTR
; CHECK-DAG: mov BYTE PTR {{.*}}

@global8 = internal global [1 x i8] c"\01", align 4

define i32 @load_i8(i32 %addr_arg) {
entry:
  %addr = inttoptr i32 %addr_arg to i8*
  %ret = load i8, i8* %addr, align 1
  %ret2 = sub i8 %ret, 0
  %ret_ext = zext i8 %ret2 to i32
  ret i32 %ret_ext
}
; CHECK-LABEL: load_i8
; CHECK: mov {{[a-d]l}},BYTE PTR
; MIPS32-LABEL: load_i8
; MIPS32: 	lb
; MIPS32: 	addiu	{{.*}},0
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	move
; MIPS32: 	jr
; MIPS32: 	nop

define i32 @load_i8_global(i32 %addr_arg) {
entry:
  %addr = bitcast [1 x i8]* @global8 to i8*
  %ret = load i8, i8* %addr, align 1
  %ret2 = sub i8 %ret, 0
  %ret_ext = zext i8 %ret2 to i32
  ret i32 %ret_ext
}
; CHECK-LABEL: load_i8_global
; CHECK: mov {{[a-d]l}},{{(BYTE PTR)?}}
; MIPS32-LABEL: load_i8_global
; MIPS32: 	lui
; MIPS32: 	addiu
; MIPS32: 	lb
; MIPS32: 	addiu	{{.*}},0
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	jr
; MIPS32: 	nop

define void @store_i8(i32 %addr_arg, i32 %val) {
entry:
  %val_trunc = trunc i32 %val to i8
  %addr = inttoptr i32 %addr_arg to i8*
  store i8 %val_trunc, i8* %addr, align 1
  ret void
}
; CHECK-LABEL: store_i8
; CHECK: mov BYTE PTR {{.*}},{{[a-d]l}}
; MIPS32-LABEL: store_i8
; MIPS32: 	sb
; MIPS32: 	jr
; MIPS32: 	nop

define void @store_i8_const(i32 %addr_arg) {
entry:
  %addr = inttoptr i32 %addr_arg to i8*
  store i8 123, i8* %addr, align 1
  ret void
}
; CHECK-LABEL: store_i8_const
; CHECK: mov BYTE PTR {{.*}},0x7b
; MIPS32-LABEL: store_i8_const
; MIPS32: 	li
; MIPS32: 	sb
; MIPS32: 	jr
; MIPS32: 	nop
