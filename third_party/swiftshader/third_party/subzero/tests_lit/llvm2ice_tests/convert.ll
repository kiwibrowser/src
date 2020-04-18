; Simple test of signed and unsigned integer conversions.

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
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

@i8v = internal global [1 x i8] zeroinitializer, align 1
@i16v = internal global [2 x i8] zeroinitializer, align 2
@i32v = internal global [4 x i8] zeroinitializer, align 4
@i64v = internal global [8 x i8] zeroinitializer, align 8
@u8v = internal global [1 x i8] zeroinitializer, align 1
@u16v = internal global [2 x i8] zeroinitializer, align 2
@u32v = internal global [4 x i8] zeroinitializer, align 4
@u64v = internal global [8 x i8] zeroinitializer, align 8

define internal void @from_int8() {
entry:
  %__0 = bitcast [1 x i8]* @i8v to i8*
  %v0 = load i8, i8* %__0, align 1
  %v1 = sext i8 %v0 to i16
  %__3 = bitcast [2 x i8]* @i16v to i16*
  store i16 %v1, i16* %__3, align 1
  %v2 = sext i8 %v0 to i32
  %__5 = bitcast [4 x i8]* @i32v to i32*
  store i32 %v2, i32* %__5, align 1
  %v3 = sext i8 %v0 to i64
  %__7 = bitcast [8 x i8]* @i64v to i64*
  store i64 %v3, i64* %__7, align 1
  ret void
}
; CHECK-LABEL: from_int8
; CHECK: mov {{.*}},{{(BYTE PTR)?}}
; CHECK: movsx {{.*}},{{[a-d]l|BYTE PTR}}
; CHECK: mov {{(WORD PTR)?}}
; CHECK: movsx
; CHECK: mov {{(DWORD PTR)?}}
; CHECK: movsx
; CHECK: sar {{.*}},0x1f
; CHECK-DAG: ds:0x{{.}},{{.*}}{{i64v|.bss}}
; CHECK-DAG: ds:0x{{.}},{{.*}}{{i64v|.bss}}

; ARM32-LABEL: from_int8
; ARM32: movw {{.*}}i8v
; ARM32: ldrb
; ARM32: sxtb
; ARM32: movw {{.*}}i16v
; ARM32: strh
; ARM32: sxtb
; ARM32: movw {{.*}}i32v
; ARM32: str r
; ARM32: sxtb
; ARM32: asr
; ARM32: movw {{.*}}i64v
; ARM32-DAG: str r{{.*}}, [r{{[0-9]+}}]
; ARM32-DAG: str r{{.*}}, [{{.*}}, #4]

; MIPS32-LABEL: from_int8
; MIPS32: 	lui	{{.*}}	i8v
; MIPS32: 	addiu	{{.*}}	i8v
; MIPS32: 	lb
; MIPS32: 	move
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	sra	{{.*}},0x18
; MIPS32: 	lui	{{.*}}	i16v
; MIPS32: 	addiu	{{.*}}	i16v
; MIPS32: 	sh
; MIPS32: 	move
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	sra	{{.*}},0x18
; MIPS32: 	lui	{{.*}}	i32v
; MIPS32: 	addiu	{{.*}}	i32v
; MIPS32: 	sw
; MIPS32: 	sll	{{.*}},0x18
; MIPS32: 	sra	{{.*}},0x18
; MIPS32: 	sra	{{.*}},0x1f
; MIPS32: 	lui	{{.*}}	i64v
; MIPS32: 	addiu	{{.*}}	i64v

define internal void @from_int16() {
entry:
  %__0 = bitcast [2 x i8]* @i16v to i16*
  %v0 = load i16, i16* %__0, align 1
  %v1 = trunc i16 %v0 to i8
  %__3 = bitcast [1 x i8]* @i8v to i8*
  store i8 %v1, i8* %__3, align 1
  %v2 = sext i16 %v0 to i32
  %__5 = bitcast [4 x i8]* @i32v to i32*
  store i32 %v2, i32* %__5, align 1
  %v3 = sext i16 %v0 to i64
  %__7 = bitcast [8 x i8]* @i64v to i64*
  store i64 %v3, i64* %__7, align 1
  ret void
}
; CHECK-LABEL: from_int16
; CHECK: mov {{.*}},{{(WORD PTR)?}}
; CHECK: 0x{{.}} {{.*}}{{i16v|.bss}}
; CHECK: movsx e{{.*}},{{.*x|[ds]i|bp|WORD PTR}}
; CHECK: 0x{{.}},{{.*}}{{i32v|.bss}}
; CHECK: movsx e{{.*}},{{.*x|[ds]i|bp|WORD PTR}}
; CHECK: sar {{.*}},0x1f
; CHECK: 0x{{.}},{{.*}}{{i64v|.bss}}

; ARM32-LABEL: from_int16
; ARM32: movw {{.*}}i16v
; ARM32: ldrh
; ARM32: movw {{.*}}i8v
; ARM32: strb
; ARM32: sxth
; ARM32: movw {{.*}}i32v
; ARM32: str r
; ARM32: sxth
; ARM32: asr
; ARM32: movw {{.*}}i64v
; ARM32: str r

; MIPS32-LABEL: from_int16
; MIPS32: 	lui	{{.*}}	i16v
; MIPS32: 	addiu	{{.*}}	i16v
; MIPS32: 	lh
; MIPS32: 	move
; MIPS32: 	lui	{{.*}}	i8v
; MIPS32: 	addiu	{{.*}}	i8v
; MIPS32: 	sb
; MIPS32: 	move
; MIPS32: 	sll	{{.*}},0x10
; MIPS32: 	sra	{{.*}},0x10
; MIPS32: 	lui	{{.*}}	i32v
; MIPS32: 	addiu	{{.*}}	i32v
; MIPS32: 	sw
; MIPS32: 	sll	{{.*}},0x10
; MIPS32: 	sra	{{.*}},0x10
; MIPS32: 	sra	{{.*}},0x1f
; MIPS32: 	lui	{{.*}}	i64v
; MIPS32: 	addiu	{{.*}}	i64v

define internal void @from_int32() {
entry:
  %__0 = bitcast [4 x i8]* @i32v to i32*
  %v0 = load i32, i32* %__0, align 1
  %v1 = trunc i32 %v0 to i8
  %__3 = bitcast [1 x i8]* @i8v to i8*
  store i8 %v1, i8* %__3, align 1
  %v2 = trunc i32 %v0 to i16
  %__5 = bitcast [2 x i8]* @i16v to i16*
  store i16 %v2, i16* %__5, align 1
  %v3 = sext i32 %v0 to i64
  %__7 = bitcast [8 x i8]* @i64v to i64*
  store i64 %v3, i64* %__7, align 1
  ret void
}
; CHECK-LABEL: from_int32
; CHECK: 0x{{.}} {{.*}} {{i32v|.bss}}
; CHECK: 0x{{.}},{{.*}} {{i8v|.bss}}
; CHECK: 0x{{.}},{{.*}} {{i16v|.bss}}
; CHECK: sar {{.*}},0x1f
; CHECK: 0x{{.}},{{.*}} {{i64v|.bss}}

; ARM32-LABEL: from_int32
; ARM32: movw {{.*}}i32v
; ARM32: ldr r
; ARM32: movw {{.*}}i8v
; ARM32: strb
; ARM32: movw {{.*}}i16v
; ARM32: strh
; ARM32: asr
; ARM32: movw {{.*}}i64v
; ARM32: str r

; MIPS32-LABEL: from_int32
; MIPS32: 	lui	{{.*}}	i32v
; MIPS32: 	addiu	{{.*}}	i32v
; MIPS32: 	lw
; MIPS32: 	move
; MIPS32: 	lui	{{.*}}	i8v
; MIPS32: 	addiu	{{.*}}	i8v
; MIPS32: 	sb
; MIPS32: 	move
; MIPS32: 	lui	{{.*}}	i16v
; MIPS32: 	addiu	{{.*}}	i16v
; MIPS32: 	sh
; MIPS32: 	sra	{{.*}},0x1f
; MIPS32: 	lui	{{.*}}	i64v
; MIPS32: 	addiu	{{.*}}	i64v

define internal void @from_int64() {
entry:
  %__0 = bitcast [8 x i8]* @i64v to i64*
  %v0 = load i64, i64* %__0, align 1
  %v1 = trunc i64 %v0 to i8
  %__3 = bitcast [1 x i8]* @i8v to i8*
  store i8 %v1, i8* %__3, align 1
  %v2 = trunc i64 %v0 to i16
  %__5 = bitcast [2 x i8]* @i16v to i16*
  store i16 %v2, i16* %__5, align 1
  %v3 = trunc i64 %v0 to i32
  %__7 = bitcast [4 x i8]* @i32v to i32*
  store i32 %v3, i32* %__7, align 1
  ret void
}
; CHECK-LABEL: from_int64
; CHECK: 0x{{.}} {{.*}} {{i64v|.bss}}
; CHECK: 0x{{.}},{{.*}} {{i8v|.bss}}
; CHECK: 0x{{.}},{{.*}} {{i16v|.bss}}
; CHECK: 0x{{.}},{{.*}} {{i32v|.bss}}

; ARM32-LABEL: from_int64
; ARM32: movw {{.*}}i64v
; ARM32: ldr r
; ARM32: movw {{.*}}i8v
; ARM32: strb
; ARM32: movw {{.*}}i16v
; ARM32: strh
; ARM32: movw {{.*}}i32v
; ARM32: str r

; MIPS32-LABEL: from_int64
; MIPS32: 	lui	{{.*}}	i64v
; MIPS32: 	addiu	{{.*}}	i64v
; MIPS32: 	lw
; MIPS32: 	move
; MIPS32: 	lui	{{.*}}	i8v
; MIPS32: 	addiu	{{.*}}	i8v
; MIPS32: 	sb
; MIPS32: 	move
; MIPS32: 	lui	{{.*}}	i16v
; MIPS32: 	addiu	{{.*}}	i16v
; MIPS32: 	sh
; MIPS32: 	lui	{{.*}}	i32v
; MIPS32: 	addiu	{{.*}}	i32v

define internal void @from_uint8() {
entry:
  %__0 = bitcast [1 x i8]* @u8v to i8*
  %v0 = load i8, i8* %__0, align 1
  %v1 = zext i8 %v0 to i16
  %__3 = bitcast [2 x i8]* @i16v to i16*
  store i16 %v1, i16* %__3, align 1
  %v2 = zext i8 %v0 to i32
  %__5 = bitcast [4 x i8]* @i32v to i32*
  store i32 %v2, i32* %__5, align 1
  %v3 = zext i8 %v0 to i64
  %__7 = bitcast [8 x i8]* @i64v to i64*
  store i64 %v3, i64* %__7, align 1
  ret void
}
; CHECK-LABEL: from_uint8
; CHECK: 0x{{.*}} {{.*}} {{u8v|.bss}}
; CHECK: movzx {{.*}},{{[a-d]l|BYTE PTR}}
; CHECK: 0x{{.}},{{.*}} {{i16v|.bss}}
; CHECK: movzx
; CHECK: 0x{{.}},{{.*}} {{i32v|.bss}}
; CHECK: movzx
; CHECK: mov {{.*}},0x0
; CHECK: 0x{{.}},{{.*}} {{i64v|.bss}}

; ARM32-LABEL: from_uint8
; ARM32: movw {{.*}}u8v
; ARM32: ldrb
; ARM32: uxtb
; ARM32: movw {{.*}}i16v
; ARM32: strh
; ARM32: uxtb
; ARM32: movw {{.*}}i32v
; ARM32: str r
; ARM32: uxtb
; ARM32: mov {{.*}}, #0
; ARM32: movw {{.*}}i64v
; ARM32: str r

; MIPS32-LABEL: from_uint8
; MIPS32: 	lui	{{.*}}	u8v
; MIPS32: 	addiu	{{.*}}	u8v
; MIPS32: 	lb
; MIPS32: 	move
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	lui	{{.*}}	i16v
; MIPS32: 	addiu	{{.*}}	i16v
; MIPS32: 	sh
; MIPS32: 	move
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	lui	{{.*}}	i32v
; MIPS32: 	addiu	{{.*}}	i32v
; MIPS32: 	sw
; MIPS32: 	andi	{{.*}},0xff
; MIPS32: 	li	{{.*}},0
; MIPS32: 	lui	{{.*}}	i64v
; MIPS32: 	addiu	{{.*}}	i64v

define internal void @from_uint16() {
entry:
  %__0 = bitcast [2 x i8]* @u16v to i16*
  %v0 = load i16, i16* %__0, align 1
  %v1 = trunc i16 %v0 to i8
  %__3 = bitcast [1 x i8]* @i8v to i8*
  store i8 %v1, i8* %__3, align 1
  %v2 = zext i16 %v0 to i32
  %__5 = bitcast [4 x i8]* @i32v to i32*
  store i32 %v2, i32* %__5, align 1
  %v3 = zext i16 %v0 to i64
  %__7 = bitcast [8 x i8]* @i64v to i64*
  store i64 %v3, i64* %__7, align 1
  ret void
}
; CHECK-LABEL: from_uint16
; CHECK: 0x{{.*}} {{.*}} {{u16v|.bss}}
; CHECK: 0x{{.}},{{.*}} {{i8v|.bss}}
; CHECK: movzx e{{.*}},{{.*x|[ds]i|bp|WORD PTR}}
; CHECK: 0x{{.}},{{.*}} {{i32v|.bss}}
; CHECK: movzx e{{.*}},{{.*x|[ds]i|bp|WORD PTR}}
; CHECK: mov {{.*}},0x0
; CHECK: 0x{{.}},{{.*}} {{i64v|.bss}}

; ARM32-LABEL: from_uint16
; ARM32: movw {{.*}}u16v
; ARM32: ldrh
; ARM32: movw {{.*}}i8v
; ARM32: strb
; ARM32: uxth
; ARM32: movw {{.*}}i32v
; ARM32: str r
; ARM32: uxth
; ARM32: mov {{.*}}, #0
; ARM32: movw {{.*}}i64v
; ARM32: str r

; MIPS32-LABEL: from_uint16
; MIPS32: 	lui	{{.*}}	u16v
; MIPS32: 	addiu	{{.*}}	u16v
; MIPS32: 	lh
; MIPS32: 	move
; MIPS32: 	lui	{{.*}}	i8v
; MIPS32: 	addiu	{{.*}}	i8v
; MIPS32: 	sb
; MIPS32: 	move
; MIPS32: 	andi	{{.*}},0xffff
; MIPS32: 	lui	{{.*}}	i32v
; MIPS32: 	addiu	{{.*}}	i32v
; MIPS32: 	sw
; MIPS32: 	andi	{{.*}},0xffff
; MIPS32: 	li	{{.*}},0
; MIPS32: 	lui	{{.*}}	i64v
; MIPS32: 	addiu	{{.*}}	i64v

define internal void @from_uint32() {
entry:
  %__0 = bitcast [4 x i8]* @u32v to i32*
  %v0 = load i32, i32* %__0, align 1
  %v1 = trunc i32 %v0 to i8
  %__3 = bitcast [1 x i8]* @i8v to i8*
  store i8 %v1, i8* %__3, align 1
  %v2 = trunc i32 %v0 to i16
  %__5 = bitcast [2 x i8]* @i16v to i16*
  store i16 %v2, i16* %__5, align 1
  %v3 = zext i32 %v0 to i64
  %__7 = bitcast [8 x i8]* @i64v to i64*
  store i64 %v3, i64* %__7, align 1
  ret void
}
; CHECK-LABEL: from_uint32
; CHECK: 0x{{.*}} {{.*}} {{u32v|.bss}}
; CHECK: 0x{{.}},{{.*}} {{i8v|.bss}}
; CHECK: 0x{{.}},{{.*}} {{i16v|.bss}}
; CHECK: mov {{.*}},0x0
; CHECK: 0x{{.}},{{.*}} {{i64v|.bss}}

; ARM32-LABEL: from_uint32
; ARM32: movw {{.*}}u32v
; ARM32: ldr r
; ARM32: movw {{.*}}i8v
; ARM32: strb
; ARM32: movw {{.*}}i16v
; ARM32: strh
; ARM32: mov {{.*}}, #0
; ARM32: movw {{.*}}i64v
; ARM32: str r

; MIPS32-LABEL: from_uint32
; MIPS32: 	lui	{{.*}}	u32v
; MIPS32: 	addiu	{{.*}}	u32v
; MIPS32: 	lw
; MIPS32: 	move
; MIPS32: 	lui	{{.*}}	i8v
; MIPS32: 	addiu	{{.*}}	i8v
; MIPS32: 	sb
; MIPS32: 	move
; MIPS32: 	lui	{{.*}}	i16v
; MIPS32: 	addiu	{{.*}}	i16v
; MIPS32: 	sh
; MIPS32: 	li	{{.*}},0
; MIPS32: 	lui	{{.*}}	i64v
; MIPS32: 	addiu	{{.*}}	i64v

define internal void @from_uint64() {
entry:
  %__0 = bitcast [8 x i8]* @u64v to i64*
  %v0 = load i64, i64* %__0, align 1
  %v1 = trunc i64 %v0 to i8
  %__3 = bitcast [1 x i8]* @i8v to i8*
  store i8 %v1, i8* %__3, align 1
  %v2 = trunc i64 %v0 to i16
  %__5 = bitcast [2 x i8]* @i16v to i16*
  store i16 %v2, i16* %__5, align 1
  %v3 = trunc i64 %v0 to i32
  %__7 = bitcast [4 x i8]* @i32v to i32*
  store i32 %v3, i32* %__7, align 1
  ret void
}
; CHECK-LABEL: from_uint64
; CHECK: 0x{{.*}} {{.*}} {{u64v|.bss}}
; CHECK: 0x{{.}},{{.*}} {{i8v|.bss}}
; CHECK: 0x{{.}},{{.*}} {{i16v|.bss}}
; CHECK: 0x{{.}},{{.*}} {{i32v|.bss}}

; ARM32-LABEL: from_uint64
; ARM32: movw {{.*}}u64v
; ARM32: ldr r
; ARM32: movw {{.*}}i8v
; ARM32: strb
; ARM32: movw {{.*}}i16v
; ARM32: strh
; ARM32: movw {{.*}}i32v
; ARM32: str r

; MIPS32-LABEL: from_uint64
; MIPS32: 	lui	{{.*}}	u64v
; MIPS32: 	addiu	{{.*}}	u64v
; MIPS32: 	lw
; MIPS32: 	move
; MIPS32: 	lui	{{.*}}	i8v
; MIPS32: 	addiu	{{.*}}	i8v
; MIPS32: 	sb
; MIPS32: 	move
; MIPS32: 	lui	{{.*}}	i16v
; MIPS32: 	addiu	{{.*}}	i16v
; MIPS32: 	sh
; MIPS32: 	lui	{{.*}}	i32v
; MIPS32: 	addiu	{{.*}}	i32v
