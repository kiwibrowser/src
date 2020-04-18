; This file checks support for legalization in MIPS.

; REQUIRES: allow_dump

; RUN: %if --need=target_MIPS32 \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble \
; RUN:   --target mips32 -i %s --args -O2 \
; RUN:   | %if --need=target_MIPS32 \
; RUN:     --command FileCheck --check-prefix MIPS32 %s

define internal i32 @legalization(i32 %a, i32 %b, i32 %c, i32 %d,
                                  i32 %e, i32 %f) {
entry:
  %a.addr = alloca i8, i32 4, align 4
  %b.addr = alloca i8, i32 4, align 4
  %c.addr = alloca i8, i32 4, align 4
  %d.addr = alloca i8, i32 4, align 4
  %e.addr = alloca i8, i32 4, align 4
  %f.addr = alloca i8, i32 4, align 4
  %r1 = alloca i8, i32 4, align 4
  %r2 = alloca i8, i32 4, align 4
  %r3 = alloca i8, i32 4, align 4
  %a.addr.bc = bitcast i8* %a.addr to i32*
  store i32 %a, i32* %a.addr.bc, align 1
  %b.addr.bc = bitcast i8* %b.addr to i32*
  store i32 %b, i32* %b.addr.bc, align 1
  %c.addr.bc = bitcast i8* %c.addr to i32*
  store i32 %c, i32* %c.addr.bc, align 1
  %d.addr.bc = bitcast i8* %d.addr to i32*
  store i32 %d, i32* %d.addr.bc, align 1
  %e.addr.bc = bitcast i8* %e.addr to i32*
  store i32 %e, i32* %e.addr.bc, align 1
  %f.addr.bc = bitcast i8* %f.addr to i32*
  store i32 %f, i32* %f.addr.bc, align 1
  %a.addr.bc1 = bitcast i8* %a.addr to i32*
  %0 = load i32, i32* %a.addr.bc1, align 1
  %f.addr.bc2 = bitcast i8* %f.addr to i32*
  %1 = load i32, i32* %f.addr.bc2, align 1
  %add = add i32 %0, %1
  %r1.bc = bitcast i8* %r1 to i32*
  store i32 %add, i32* %r1.bc, align 1
  %b.addr.bc3 = bitcast i8* %b.addr to i32*
  %2 = load i32, i32* %b.addr.bc3, align 1
  %e.addr.bc4 = bitcast i8* %e.addr to i32*
  %3 = load i32, i32* %e.addr.bc4, align 1
  %add1 = add i32 %2, %3
  %r2.bc = bitcast i8* %r2 to i32*
  store i32 %add1, i32* %r2.bc, align 1
  %r1.bc5 = bitcast i8* %r1 to i32*
  %4 = load i32, i32* %r1.bc5, align 1
  %r2.bc6 = bitcast i8* %r2 to i32*
  %5 = load i32, i32* %r2.bc6, align 1
  %add2 = add i32 %4, %5
  %r3.bc = bitcast i8* %r3 to i32*
  store i32 %add2, i32* %r3.bc, align 1
  %r3.bc7 = bitcast i8* %r3 to i32*
  %6 = load i32, i32* %r3.bc7, align 1
  ret i32 %6
}
; MIPS32-LABEL: legalization
; MIPS32: addiu sp,sp,-48
; MIPS32: lw [[ARG_E:.*]],64(sp)
; MIPS32: lw [[ARG_F:.*]],68(sp)
; MIPS32: sw a0,0(sp)
; MIPS32: sw a1,4(sp)
; MIPS32: sw a2,8(sp)
; MIPS32: sw a3,12(sp)
; MIPS32: sw [[ARG_E]],16(sp)
; MIPS32: sw [[ARG_F]],20(sp)
; MIPS32: lw [[TMP_A:.*]],0(sp)
; MIPS32: lw [[TMP_F:.*]],20(sp)
; MIPS32: addu [[ADD1:.*]],[[TMP_A]],[[TMP_F]]
; MIPS32: sw [[ADD1]],24(sp)
; MIPS32: lw [[TMP_B:.*]],4(sp)
; MIPS32: lw [[TMP_E:.*]],16(sp)
; MIPS32: addu [[ADD2:.*]],[[TMP_B]],[[TMP_E]]
; MIPS32: sw [[ADD2]],28(sp)
; MIPS32: lw [[TMP_ADD1:.*]],24(sp)
; MIPS32: lw [[TMP_ADD2:.*]],28(sp)
; MIPS32: addu [[ADD3:.*]],[[TMP_ADD1]],[[TMP_ADD2]]
; MIPS32: sw [[ADD3]],32(sp)
; MIPS32: lw v0,32(sp)
; MIPS32: addiu sp,sp,48
; MIPS32: jr ra
