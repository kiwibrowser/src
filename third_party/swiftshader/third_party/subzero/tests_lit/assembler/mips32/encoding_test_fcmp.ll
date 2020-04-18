; Test encoding of MIPS32 floating point comparison

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=mips32 --args -O2 \
; RUN:   --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=mips32 --assemble --disassemble \
; RUN:   --args -O2 --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=mips32 --args -O2 \
; RUN:   --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=mips32 --assemble --disassemble \
; RUN:   --args -O2 --allow-externally-defined-symbols \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal i32 @fcmpFalseFloat(float %a, float %b) {
entry:
  %cmp = fcmp false float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpFalseFloat:
; ASM-NEXT: .LfcmpFalseFloat$entry:
; ASM: 	addiu	$v0, $zero, 0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000000 <fcmpFalseFloat>:
; DIS-NEXT:    0:	24020000 	li	v0,0
; DIS-NEXT:    4:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:    8:	03e00008 	jr	ra

; IASM-LABEL: fcmpFalseFloat:
; IASM-NEXT: .LfcmpFalseFloat$entry:
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpFalseDouble(double %a, double %b) {
entry:
  %cmp = fcmp false double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpFalseDouble:
; ASM-NEXT: .LfcmpFalseDouble$entry:
; ASM: 	addiu	$v0, $zero, 0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000010 <fcmpFalseDouble>:
; DIS-NEXT:   10:	24020000 	li	v0,0
; DIS-NEXT:   14:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:   18:	03e00008 	jr	ra

; IASM-LABEL: fcmpFalseDouble:
; IASM-NEXT: .LfcmpFalseDouble$entry:
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOeqFloat(float %a, float %b) {
entry:
  %cmp = fcmp oeq float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOeqFloat
; ASM-NEXT: .LfcmpOeqFloat$entry:
; ASM: 	c.eq.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000020 <fcmpOeqFloat>:
; DIS-NEXT:   20:	460e6032 	c.eq.s	$f12,$f14
; DIS-NEXT:   24:	24020001 	li	v0,1
; DIS-NEXT:   28:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:   2c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:   30:	03e00008 	jr	ra

; IASM-LABEL: fcmpOeqFloat:
; IASM-NEXT: .LfcmpOeqFloat$entry:
; IASM-NEXT: 	.byte 0x32
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOeqDouble(double %a, double %b) {
entry:
  %cmp = fcmp oeq double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOeqDouble
; ASM-NEXT: .LfcmpOeqDouble$entry:
; ASM: 	c.eq.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000040 <fcmpOeqDouble>:
; DIS-NEXT:   40:	462e6032 	c.eq.d	$f12,$f14
; DIS-NEXT:   44:	24020001 	li	v0,1
; DIS-NEXT:   48:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:   4c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:   50:	03e00008 	jr	ra

; IASM-LABEL: fcmpOeqDouble:
; IASM-NEXT: .LfcmpOeqDouble$entry:
; IASM-NEXT: 	.byte 0x32
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOgtFloat(float %a, float %b) {
entry:
  %cmp = fcmp ogt float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOgtFloat
; ASM-NEXT: .LfcmpOgtFloat$entry:
; ASM: 	c.ule.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000060 <fcmpOgtFloat>:
; DIS-NEXT:   60:	460e6037 	c.ule.s	$f12,$f14
; DIS-NEXT:   64:	24020001 	li	v0,1
; DIS-NEXT:   68:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:   6c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:   70:	03e00008 	jr	ra

; IASM-LABEL: fcmpOgtFloat:
; IASM-NEXT: .LfcmpOgtFloat$entry:
; IASM-NEXT: 	.byte 0x37
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOgtDouble(double %a, double %b) {
entry:
  %cmp = fcmp ogt double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOgtDouble
; ASM-NEXT: .LfcmpOgtDouble$entry:
; ASM: 	c.ule.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000080 <fcmpOgtDouble>:
; DIS-NEXT:   80:	462e6037 	c.ule.d	$f12,$f14
; DIS-NEXT:   84:	24020001 	li	v0,1
; DIS-NEXT:   88:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:   8c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:   90:	03e00008 	jr	ra

; IASM-LABEL: fcmpOgtDouble:
; IASM-NEXT: .LfcmpOgtDouble$entry:
; IASM-NEXT: 	.byte 0x37
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOgeFloat(float %a, float %b) {
entry:
  %cmp = fcmp oge float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOgeFloat
; ASM-NEXT: .LfcmpOgeFloat$entry:
; ASM: 	c.ult.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 000000a0 <fcmpOgeFloat>:
; DIS-NEXT:   a0:	460e6035 	c.ult.s	$f12,$f14
; DIS-NEXT:   a4:	24020001 	li	v0,1
; DIS-NEXT:   a8:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:   ac:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:   b0:	03e00008 	jr	ra

; IASM-LABEL: fcmpOgeFloat:
; IASM-NEXT: .LfcmpOgeFloat$entry:
; IASM-NEXT: 	.byte 0x35
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOgeDouble(double %a, double %b) {
entry:
  %cmp = fcmp oge double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOgeDouble
; ASM-NEXT: .LfcmpOgeDouble$entry:
; ASM: 	c.ult.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 000000c0 <fcmpOgeDouble>:
; DIS-NEXT:   c0:	462e6035 	c.ult.d	$f12,$f14
; DIS-NEXT:   c4:	24020001 	li	v0,1
; DIS-NEXT:   c8:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:   cc:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:   d0:	03e00008 	jr	ra

; IASM-LABEL: fcmpOgeDouble:
; IASM-NEXT: .LfcmpOgeDouble$entry:
; IASM-NEXT: 	.byte 0x35
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOltFloat(float %a, float %b) {
entry:
  %cmp = fcmp olt float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOltFloat
; ASM-NEXT: .LfcmpOltFloat$entry:
; ASM: 	c.olt.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 000000e0 <fcmpOltFloat>:
; DIS-NEXT:   e0:	460e6034 	c.olt.s	$f12,$f14
; DIS-NEXT:   e4:	24020001 	li	v0,1
; DIS-NEXT:   e8:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:   ec:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:   f0:	03e00008 	jr	ra

; IASM-LABEL: fcmpOltFloat:
; IASM-NEXT: .LfcmpOltFloat$entry:
; IASM-NEXT: 	.byte 0x34
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOltDouble(double %a, double %b) {
entry:
  %cmp = fcmp olt double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOltDouble
; ASM-NEXT: .LfcmpOltDouble$entry:
; ASM: 	c.olt.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000100 <fcmpOltDouble>:
; DIS-NEXT:  100:	462e6034 	c.olt.d	$f12,$f14
; DIS-NEXT:  104:	24020001 	li	v0,1
; DIS-NEXT:  108:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:  10c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  110:	03e00008 	jr	ra

; IASM-LABEL: fcmpOltDouble:
; IASM-NEXT: .LfcmpOltDouble$entry:
; IASM-NEXT: 	.byte 0x34
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOleFloat(float %a, float %b) {
entry:
  %cmp = fcmp ole float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOleFloat
; ASM-NEXT: .LfcmpOleFloat$entry:
; ASM: 	c.ole.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000120 <fcmpOleFloat>:
; DIS-NEXT:  120:	460e6036 	c.ole.s	$f12,$f14
; DIS-NEXT:  124:	24020001 	li	v0,1
; DIS-NEXT:  128:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:  12c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  130:	03e00008 	jr	ra

; IASM-LABEL: fcmpOleFloat:
; IASM-NEXT: .LfcmpOleFloat$entry:
; IASM-NEXT: 	.byte 0x36
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOleDouble(double %a, double %b) {
entry:
  %cmp = fcmp ole double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOleDouble
; ASM-NEXT: .LfcmpOleDouble$entry:
; ASM: 	c.ole.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000140 <fcmpOleDouble>:
; DIS-NEXT:  140:	462e6036 	c.ole.d	$f12,$f14
; DIS-NEXT:  144:	24020001 	li	v0,1
; DIS-NEXT:  148:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:  14c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  150:	03e00008 	jr	ra

; IASM-LABEL: fcmpOleDouble:
; IASM-NEXT: .LfcmpOleDouble$entry:
; IASM-NEXT: 	.byte 0x36
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOneFloat(float %a, float %b) {
entry:
  %cmp = fcmp one float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOneFloat
; ASM-NEXT: .LfcmpOneFloat$entry:
; ASM: 	c.ueq.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000160 <fcmpOneFloat>:
; DIS-NEXT:  160:	460e6033 	c.ueq.s	$f12,$f14
; DIS-NEXT:  164:	24020001 	li	v0,1
; DIS-NEXT:  168:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:  16c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  170:	03e00008 	jr	ra

; IASM-LABEL: fcmpOneFloat:
; IASM-NEXT: .LfcmpOneFloat$entry:
; IASM-NEXT: 	.byte 0x33
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOneDouble(double %a, double %b) {
entry:
  %cmp = fcmp one double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOneDouble
; ASM-NEXT: .LfcmpOneDouble$entry:
; ASM: 	c.ueq.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000180 <fcmpOneDouble>:
; DIS-NEXT:  180:	462e6033 	c.ueq.d	$f12,$f14
; DIS-NEXT:  184:	24020001 	li	v0,1
; DIS-NEXT:  188:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:  18c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  190:	03e00008 	jr	ra

; IASM-LABEL: fcmpOneDouble:
; IASM-NEXT: .LfcmpOneDouble$entry:
; IASM-NEXT: 	.byte 0x33
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOrdFloat(float %a, float %b) {
entry:
  %cmp = fcmp ord float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOrdFloat:
; ASM-NEXT: .LfcmpOrdFloat$entry:
; ASM: 	c.un.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 000001a0 <fcmpOrdFloat>:
; DIS-NEXT:  1a0:	460e6031 	c.un.s	$f12,$f14
; DIS-NEXT:  1a4:	24020001 	li	v0,1
; DIS-NEXT:  1a8:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:  1ac:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  1b0:	03e00008 	jr	ra

; IASM-LABEL: fcmpOrdFloat:
; IASM-NEXT: .LfcmpOrdFloat$entry:
; IASM-NEXT: 	.byte 0x31
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpOrdDouble(double %a, double %b) {
entry:
  %cmp = fcmp ord double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpOrdDouble:
; ASM-NEXT: .LfcmpOrdDouble$entry:
; ASM: 	c.un.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 000001c0 <fcmpOrdDouble>:
; DIS-NEXT:  1c0:	462e6031 	c.un.d	$f12,$f14
; DIS-NEXT:  1c4:	24020001 	li	v0,1
; DIS-NEXT:  1c8:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:  1cc:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  1d0:	03e00008 	jr	ra

; IASM-LABEL: fcmpOrdDouble:
; IASM-NEXT: .LfcmpOrdDouble$entry:
; IASM-NEXT: 	.byte 0x31
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUeqFloat(float %a, float %b) {
entry:
  %cmp = fcmp ueq float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUeqFloat
; ASM-NEXT: .LfcmpUeqFloat$entry:
; ASM: 	c.ueq.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 000001e0 <fcmpUeqFloat>:
; DIS-NEXT:  1e0:	460e6033 	c.ueq.s	$f12,$f14
; DIS-NEXT:  1e4:	24020001 	li	v0,1
; DIS-NEXT:  1e8:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:  1ec:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  1f0:	03e00008 	jr	ra

; IASM-LABEL: fcmpUeqFloat:
; IASM-NEXT: .LfcmpUeqFloat$entry:
; IASM-NEXT: 	.byte 0x33
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUeqDouble(double %a, double %b) {
entry:
  %cmp = fcmp ueq double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUeqDouble
; ASM-NEXT: .LfcmpUeqDouble$entry:
; ASM: 	c.ueq.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000200 <fcmpUeqDouble>:
; DIS-NEXT:  200:	462e6033 	c.ueq.d	$f12,$f14
; DIS-NEXT:  204:	24020001 	li	v0,1
; DIS-NEXT:  208:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:  20c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  210:	03e00008 	jr	ra

; IASM-LABEL: fcmpUeqDouble:
; IASM-NEXT: .LfcmpUeqDouble$entry:
; IASM-NEXT: 	.byte 0x33
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUgtFloat(float %a, float %b) {
entry:
  %cmp = fcmp ugt float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUgtFloat
; ASM-NEXT: .LfcmpUgtFloat$entry:
; ASM: 	c.ole.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000220 <fcmpUgtFloat>:
; DIS-NEXT:  220:	460e6036 	c.ole.s	$f12,$f14
; DIS-NEXT:  224:	24020001 	li	v0,1
; DIS-NEXT:  228:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:  22c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  230:	03e00008 	jr	ra

; IASM-LABEL: fcmpUgtFloat:
; IASM-NEXT: .LfcmpUgtFloat$entry:
; IASM-NEXT: 	.byte 0x36
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUgtDouble(double %a, double %b) {
entry:
  %cmp = fcmp ugt double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUgtDouble
; ASM-NEXT: .LfcmpUgtDouble$entry:
; ASM: 	c.ole.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000240 <fcmpUgtDouble>:
; DIS-NEXT:  240:	462e6036 	c.ole.d	$f12,$f14
; DIS-NEXT:  244:	24020001 	li	v0,1
; DIS-NEXT:  248:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:  24c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  250:	03e00008 	jr	ra

; IASM-LABEL: fcmpUgtDouble:
; IASM-NEXT: .LfcmpUgtDouble$entry:
; IASM-NEXT: 	.byte 0x36
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUgeFloat(float %a, float %b) {
entry:
  %cmp = fcmp uge float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUgeFloat
; ASM-NEXT: .LfcmpUgeFloat$entry:
; ASM: 	c.olt.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000260 <fcmpUgeFloat>:
; DIS-NEXT:  260:	460e6034 	c.olt.s	$f12,$f14
; DIS-NEXT:  264:	24020001 	li	v0,1
; DIS-NEXT:  268:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:  26c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  270:	03e00008 	jr	ra

; IASM-LABEL: fcmpUgeFloat:
; IASM-NEXT: .LfcmpUgeFloat$entry:
; IASM-NEXT: 	.byte 0x34
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUgeDouble(double %a, double %b) {
entry:
  %cmp = fcmp uge double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUgeDouble
; ASM-NEXT: .LfcmpUgeDouble$entry:
; ASM: 	c.olt.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000280 <fcmpUgeDouble>:
; DIS-NEXT:  280:	462e6034 	c.olt.d	$f12,$f14
; DIS-NEXT:  284:	24020001 	li	v0,1
; DIS-NEXT:  288:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:  28c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  290:	03e00008 	jr	ra

; IASM-LABEL: fcmpUgeDouble:
; IASM-NEXT: .LfcmpUgeDouble$entry:
; IASM-NEXT: 	.byte 0x34
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUltFloat(float %a, float %b) {
entry:
  %cmp = fcmp ult float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUltFloat
; ASM-NEXT: .LfcmpUltFloat$entry:
; ASM: 	c.ult.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 000002a0 <fcmpUltFloat>:
; DIS-NEXT:  2a0:	460e6035 	c.ult.s	$f12,$f14
; DIS-NEXT:  2a4:	24020001 	li	v0,1
; DIS-NEXT:  2a8:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:  2ac:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  2b0:	03e00008 	jr	ra

; IASM-LABEL: fcmpUltFloat:
; IASM-NEXT: .LfcmpUltFloat$entry:
; IASM-NEXT: 	.byte 0x35
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUltDouble(double %a, double %b) {
entry:
  %cmp = fcmp ult double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUltDouble
; ASM-NEXT: .LfcmpUltDouble$entry:
; ASM: 	c.ult.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 000002c0 <fcmpUltDouble>:
; DIS-NEXT:  2c0:	462e6035 	c.ult.d	$f12,$f14
; DIS-NEXT:  2c4:	24020001 	li	v0,1
; DIS-NEXT:  2c8:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:  2cc:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  2d0:	03e00008 	jr	ra

; IASM-LABEL: fcmpUltDouble:
; IASM-NEXT: .LfcmpUltDouble$entry:
; IASM-NEXT: 	.byte 0x35
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUleFloat(float %a, float %b) {
entry:
  %cmp = fcmp ule float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUleFloat
; ASM-NEXT: .LfcmpUleFloat$entry:
; ASM: 	c.ule.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 000002e0 <fcmpUleFloat>:
; DIS-NEXT:  2e0:	460e6037 	c.ule.s	$f12,$f14
; DIS-NEXT:  2e4:	24020001 	li	v0,1
; DIS-NEXT:  2e8:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:  2ec:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  2f0:	03e00008 	jr	ra

; IASM-LABEL: fcmpUleFloat:
; IASM-NEXT: .LfcmpUleFloat$entry:
; IASM-NEXT: 	.byte 0x37
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUleDouble(double %a, double %b) {
entry:
  %cmp = fcmp ule double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUleDouble
; ASM-NEXT: .LfcmpUleDouble$entry:
; ASM: 	c.ule.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000300 <fcmpUleDouble>:
; DIS-NEXT:  300:	462e6037 	c.ule.d	$f12,$f14
; DIS-NEXT:  304:	24020001 	li	v0,1
; DIS-NEXT:  308:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:  30c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  310:	03e00008 	jr	ra

; IASM-LABEL: fcmpUleDouble:
; IASM-NEXT: .LfcmpUleDouble$entry:
; IASM-NEXT: 	.byte 0x37
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUneFloat(float %a, float %b) {
entry:
  %cmp = fcmp une float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUneFloat
; ASM-NEXT: .LfcmpUneFloat$entry:
; ASM: 	c.eq.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000320 <fcmpUneFloat>:
; DIS-NEXT:  320:	460e6032 	c.eq.s	$f12,$f14
; DIS-NEXT:  324:	24020001 	li	v0,1
; DIS-NEXT:  328:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:  32c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  330:	03e00008 	jr	ra

; IASM-LABEL: fcmpUneFloat:
; IASM-NEXT: .LfcmpUneFloat$entry:
; IASM-NEXT: 	.byte 0x32
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUneDouble(double %a, double %b) {
entry:
  %cmp = fcmp une double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUneDouble
; ASM-NEXT: .LfcmpUneDouble$entry:
; ASM: 	c.eq.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movt	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000340 <fcmpUneDouble>:
; DIS-NEXT:  340:	462e6032 	c.eq.d	$f12,$f14
; DIS-NEXT:  344:	24020001 	li	v0,1
; DIS-NEXT:  348:	00011001 	movt	v0,zero,$fcc0
; DIS-NEXT:  34c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  350:	03e00008 	jr	ra

; IASM-LABEL: fcmpUneDouble:
; IASM-NEXT: .LfcmpUneDouble$entry:
; IASM-NEXT: 	.byte 0x32
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUnoFloat(float %a, float %b) {
entry:
  %cmp = fcmp uno float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUnoFloat
; ASM-NEXT: .LfcmpUnoFloat$entry:
; ASM: 	c.un.s	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000360 <fcmpUnoFloat>:
; DIS-NEXT:  360:	460e6031 	c.un.s	$f12,$f14
; DIS-NEXT:  364:	24020001 	li	v0,1
; DIS-NEXT:  368:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:  36c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  370:	03e00008 	jr	ra

; IASM-LABEL: fcmpUnoFloat:
; IASM-NEXT: .LfcmpUnoFloat$entry:
; IASM-NEXT: 	.byte 0x31
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpUnoDouble(double %a, double %b) {
entry:
  %cmp = fcmp uno double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpUnoDouble
; ASM-NEXT: .LfcmpUnoDouble$entry:
; ASM: 	c.un.d	$f12, $f14
; ASM: 	addiu	$v0, $zero, 1
; ASM: 	movf	$v0, $zero, $fcc0
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000380 <fcmpUnoDouble>:
; DIS-NEXT:  380:	462e6031 	c.un.d	$f12,$f14
; DIS-NEXT:  384:	24020001 	li	v0,1
; DIS-NEXT:  388:	00001001 	movf	v0,zero,$fcc0
; DIS-NEXT:  38c:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  390:	03e00008 	jr	ra

; IASM-LABEL: fcmpUnoDouble:
; IASM-NEXT: .LfcmpUnoDouble$entry:
; IASM-NEXT: 	.byte 0x31
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x10
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpTrueFloat(float %a, float %b) {
entry:
  %cmp = fcmp true float %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpTrueFloat
; ASM-NEXT: .LfcmpTrueFloat$entry:
; ASM: 	addiu	$v0, $zero, 1
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 000003a0 <fcmpTrueFloat>:
; DIS-NEXT:  3a0:	24020001 	li	v0,1
; DIS-NEXT:  3a4:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  3a8:	03e00008 	jr	ra

; IASM-LABEL: fcmpTrueFloat:
; IASM-NEXT: .LfcmpTrueFloat$entry:
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3

define internal i32 @fcmpTrueDouble(double %a, double %b) {
entry:
  %cmp = fcmp true double %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

; ASM-LABEL: fcmpTrueDouble
; ASM-NEXT: .LfcmpTrueDouble$entry:
; ASM: 	addiu	$v0, $zero, 1
; ASM-NEXT: 	andi	$v0, $v0, 1
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 000003b0 <fcmpTrueDouble>:
; DIS-NEXT:  3b0:	24020001 	li	v0,1
; DIS-NEXT:  3b4:	30420001 	andi	v0,v0,0x1
; DIS-NEXT:  3b8:	03e00008 	jr	ra

; IASM-LABEL: fcmpTrueDouble:
; IASM-NEXT: .LfcmpTrueDouble$entry:
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x24
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x42
; IASM-NEXT: 	.byte 0x30
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
