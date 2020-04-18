; Simple test of the load instruction.

; REQUIRES: allow_dump

; RUN: %p2i -i %s --args --verbose inst -threads=0 | FileCheck %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

define internal void @load_i64(i32 %addr_arg) {
entry:
  %__1 = inttoptr i32 %addr_arg to i64*
  %iv = load i64, i64* %__1, align 1
  ret void

; CHECK:       Initial CFG
; CHECK:     entry:
; CHECK-NEXT:  %iv = load i64, i64* %addr_arg, align 1
; CHECK-NEXT:  ret void
}

; MIPS32-LABEL: load_i64
; MIPS32: lw [[BASE:.*]],
; MIPS32-NEXT: lw {{.*}},0([[BASE]])
; MIPS32-NEXT: lw {{.*}},4([[BASE]])

define internal void @load_i32(i32 %addr_arg) {
entry:
  %__1 = inttoptr i32 %addr_arg to i32*
  %iv = load i32, i32* %__1, align 1
  ret void

; CHECK:       Initial CFG
; CHECK:     entry:
; CHECK-NEXT:  %iv = load i32, i32* %addr_arg, align 1
; CHECK-NEXT:  ret void
}

; MIPS32-LABEL: load_i32
; MIPS32: lw {{.*}},0({{.*}})

define internal void @load_i16(i32 %addr_arg) {
entry:
  %__1 = inttoptr i32 %addr_arg to i16*
  %iv = load i16, i16* %__1, align 1
  ret void

; CHECK:       Initial CFG
; CHECK:     entry:
; CHECK-NEXT:  %iv = load i16, i16* %addr_arg, align 1
; CHECK-NEXT:  ret void
}

; MIPS32-LABEL: load_i16
; MIPS32: lh {{.*}},0({{.*}})

define internal void @load_i8(i32 %addr_arg) {
entry:
  %__1 = inttoptr i32 %addr_arg to i8*
  %iv = load i8, i8* %__1, align 1
  ret void

; CHECK:       Initial CFG
; CHECK:     entry:
; CHECK-NEXT:  %iv = load i8, i8* %addr_arg, align 1
; CHECK-NEXT:  ret void
}

; MIPS32-LABEL: load_i8
; MIPS32: lb {{.*}},0({{.*}})
