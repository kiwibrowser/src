; Show that pops are generated in reverse order of pushes.

; NOTE: Restricts to nonconsecutive S registers to force the generation of
; multiple vpush/vpop instructions. Also tests that we generate them in the
; right order (the reverse of the corresponding push). Uses -O2 to keep all
; results in S registers.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 -allow-extern \
; RUN:   -reg-use s20,s22,s23 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 -allow-extern \
; RUN:   -reg-use s20,s22,s23 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 -allow-extern \
; RUN:   -reg-use s20,s22,s23 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 -allow-extern \
; RUN:   -reg-use s20,s22,s23 \
; RUN:   | FileCheck %s --check-prefix=DIS

declare external void @f()

define internal float @test2SPops(float %p1, float %p2) {
; ASM-LABEL: test2SPops:
; DIS-LABEL: 00000000 <test2SPops>:
; IASM-LABEL: test2SPops:

; ASM:          vpush   {s20}
; ASM-NEXT:     vpush   {s22, s23}
; ASM-NEXT:     push    {lr}

; DIS:          0:      ed2daa01
; DIS-NEXT:     4:      ed2dba02
; DIS-NEXT:     8:      e52de004

; IASM-NOT:     vpush
; IASM-NOT:     push

  %v1 = fadd float %p1, %p2
  %v2 = fsub float %p1, %p2
  %v3 = fsub float %p2, %p1
  call void @f()
  %v4 = fadd float %v1, %v2
  %res = fadd float %v3, %v4

; ASM:          pop     {lr}
; ASM-NEXT:     # lr = def.pseudo
; ASM-NEXT:     vpop    {s22, s23}
; ASM-NEXT:     vpop    {s20}

; DIS:         40:      e49de004
; DIS-NEXT:    44:      ecbdba02
; DIS-NEXT:    48:      ecbdaa01

; IASM-NOT: pop
; IASM-NOT: vpop

  ret float %res
}
