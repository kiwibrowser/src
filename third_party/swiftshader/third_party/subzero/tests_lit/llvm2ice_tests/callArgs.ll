; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s



declare void @voidCall5i32(i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5)
declare void @voidCall5i64(i64 %a1, i64 %a2, i64 %a3, i64 %a4, i64 %a5)
declare void @voidCalli32i64i32(i32 %a1, i64 %a2, i32 %a3)

;TODO(mohit.bhakkad): Add tests for f32/f64 once legalize() or lowerArgument is
;available for f32/f64

define internal void @Call() {
  call void @voidCall5i32(i32 1, i32 2, i32 3, i32 4, i32 5)
  call void @voidCall5i64(i64 1, i64 2, i64 3, i64 4, i64 5)
  call void @voidCalli32i64i32(i32 1, i64 2, i32 3)
  ret void
}
; MIPS32: li    {{.*}},5
; MIPS32: sw	{{.*}},16(sp)
; MIPS32: li	a0,1
; MIPS32: li	a1,2
; MIPS32: li	a2,3
; MIPS32: li	a3,4
; MIPS32: jal

; MIPS32: li	{{.*}},0
; MIPS32: li	{{.*}},3
; MIPS32: sw	{{.*}},20(sp)
; MIPS32: sw	{{.*}},16(sp)
; MIPS32: li	{{.*}},0
; MIPS32: li	{{.*}},4
; MIPS32: sw	{{.*}},28(sp)
; MIPS32: sw	{{.*}},24(sp)
; MIPS32: li	{{.*}},0
; MIPS32: li	{{.*}},5
; MIPS32: sw	{{.*}},36(sp)
; MIPS32: sw	{{.*}},32(sp)
; MIPS32: li	a0,1
; MIPS32: li	a1,0
; MIPS32: li	a2,2
; MIPS32: li	a3,0
; MIPS32: jal

; MIPS32: li	{{.*}},3
; MIPS32: sw	{{.*}},16(sp)
; MIPS32: li	a0,1
; MIPS32: li	a2,2
; MIPS32: li	a3,0
; MIPS32: jal
