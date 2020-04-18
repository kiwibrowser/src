; REQUIRES: allow_dump

; RUN: %p2i -i %s --filetype=asm --assemble --disassemble --target=mips32 \
; RUN:    -a -sz-seed=1 -nop-insertion \
; RUN:    -nop-insertion-percentage=50 -max-nops-per-instruction=1 \
; RUN:    | FileCheck %s --check-prefix=MIPS32P50N1
; RUN: %p2i -i %s --filetype=asm --assemble --disassemble --target=mips32 \
; RUN:    -a -sz-seed=1 -nop-insertion \
; RUN:    -nop-insertion-percentage=110 -max-nops-per-instruction=2 \
; RUN:    | FileCheck %s --check-prefix=MIPS32P110N2


define internal i32 @nopInsertion(i32 %a, i32 %b, i32 %c) {
entry:
  %a1 = add i32 %a, 1
  %b1 = add i32 %b, 2
  %c1 = add i32 %c, 3
  %a2 = sub i32 %a1, 1
  %b2 = sub i32 %b1, 2
  %c2 = sub i32 %c1, 3
  %a3 = mul i32 %a2, %b2
  %b3 = mul i32 %a3, %c2
  ret i32 %b3
}

; MIPS32P50N1-LABEL: nopInsertion
; MIPS32P50N1: nop
; MIPS32P50N1: addiu {{.*}}
; MIPS32P50N1: sw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: sw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: sw {{.*}}
; MIPS32P50N1: lw {{.*}}
; MIPS32P50N1: addiu {{.*}},1
; MIPS32P50N1: nop
; MIPS32P50N1: sw {{.*}}
; MIPS32P50N1: lw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: addiu {{.*}},2
; MIPS32P50N1: nop
; MIPS32P50N1: sw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: lw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: addiu {{.*}},3
; MIPS32P50N1: sw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: lw {{.*}}
; MIPS32P50N1: addiu {{.*}},-1
; MIPS32P50N1: nop
; MIPS32P50N1: sw {{.*}}
; MIPS32P50N1: lw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: addiu {{.*}},-2
; MIPS32P50N1: nop
; MIPS32P50N1: sw {{.*}}
; MIPS32P50N1: lw {{.*}}
; MIPS32P50N1: addiu {{.*}},-3
; MIPS32P50N1: sw {{.*}}
; MIPS32P50N1: lw {{.*}}
; MIPS32P50N1: lw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: mul {{.*}}
; MIPS32P50N1: sw {{.*}}
; MIPS32P50N1: lw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: lw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: mul {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: sw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: lw {{.*}}
; MIPS32P50N1: nop
; MIPS32P50N1: addiu {{.*}}
; MIPS32P50N1: jr ra
; MIPS32P50N1: nop

; MIPS32P110N2-LABEL: nopInsertion
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: addiu {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: sw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: sw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: sw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: lw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: addiu {{.*}},1
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: sw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: lw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: addiu {{.*}},2
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: sw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: lw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: addiu {{.*}},3
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: sw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: lw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: addiu {{.*}},-1
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: sw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: lw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: addiu {{.*}},-2
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: sw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: lw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: addiu {{.*}},-3
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: sw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: lw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: lw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: mul {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: sw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: lw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: lw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: mul {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: sw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: lw {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: addiu {{.*}}
; MIPS32P110N2: nop
; MIPS32P110N2: nop
; MIPS32P110N2: jr ra
; MIPS32P110N2: nop
; MIPS32P110N2: nop
