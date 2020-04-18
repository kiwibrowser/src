; Tests the integrated assembler for instructions with a reloc + offset.

; RUN: %if --need=target_X8632 --need=allow_dump \
; RUN:   --command %p2i --target x8632 -i %s --args -O2 \
; RUN:   | %if --need=target_X8632 --need=allow_dump --command FileCheck %s

@p_global_char = internal global [8 x i8] zeroinitializer, align 8

define internal void @reloc_in_global(i64 %x) {
entry:
  %p_global_char.bc = bitcast [8 x i8]* @p_global_char to i64*
  ; This 64-bit load is split into an i32 store to [p_global_char]
  ; and an i32 store to [p_global_char + 4] on 32-bit architectures.
  store i64 %x, i64* %p_global_char.bc, align 1
  ret void
}
; CHECK-LABEL: reloc_in_global
; CHECK: .long p_global_char + 4
