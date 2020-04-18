; This file checks support for address mode optimization.
; This test file is same as address-mode-opt.ll however the functions in this
; file are relevant to MIPS only.

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble \
; RUN:   --target mips32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:     --command FileCheck --check-prefix MIPS32 %s

define internal float @load_arg_plus_offset(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr.int = add i32 %arg.int, 16
  %addr.ptr = inttoptr i32 %addr.int to float*
  %addr.load = load float, float* %addr.ptr, align 4
  ret float %addr.load
}
; MIPS32-LABEL: load_arg_plus_offset
; MIPS32: lwc1 $f0,16(a0)

define internal float @load_arg_minus_offset(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr.int = sub i32 %arg.int, 16
  %addr.ptr = inttoptr i32 %addr.int to float*
  %addr.load = load float, float* %addr.ptr, align 4
  ret float %addr.load
}
; MIPS32-LABEL: load_arg_minus_offset
; MIPS32 lwc1 $f0,-16(a0)

define internal float @address_mode_opt_chaining(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr1.int = add i32 12, %arg.int
  %addr2.int = sub i32 %addr1.int, 4
  %addr2.ptr = inttoptr i32 %addr2.int to float*
  %addr2.load = load float, float* %addr2.ptr, align 4
  ret float %addr2.load
}
; MIPS32-LABEL: address_mode_opt_chaining
; MIPS32 lwc1 $f0,8(a0)
