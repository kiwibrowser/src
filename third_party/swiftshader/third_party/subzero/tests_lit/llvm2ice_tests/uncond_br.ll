; TODO: Switch to --filetype=obj when possible.
; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

define internal void @uncond1(i32 %i) {
  %1 = alloca i8, i32 4, align 4
  %.bc = bitcast i8* %1 to i32*
  store i32 %i, i32* %.bc, align 1
  br label %target
target:
  %.bc1 = bitcast i8* %1 to i32*
  %2 = load i32, i32* %.bc1, align 1
  %3 = add i32 %2, 1
  %.bc2 = bitcast i8* %1 to i32*
  store i32 %3, i32* %.bc2, align 1
  br label %target
}

; MIPS32-LABEL: uncond1
; MIPS32: <.Luncond1$target>:
; MIPS32: addiu {{.*}},1
; MIPS32: b {{[0-9a-f]+}} <.Luncond1$target>
