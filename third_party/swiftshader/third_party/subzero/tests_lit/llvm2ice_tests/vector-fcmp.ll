; This file checks support for comparing vector values with the fcmp
; instruction.

; RUN: %p2i -i %s --filetype=obj --disassemble -a -O2 | FileCheck %s
; RUN: %p2i -i %s --filetype=obj --disassemble -a -Om1 | FileCheck %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target mips32\
; RUN:   -i %s --args -O2 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; Check that sext elimination occurs when the result of the comparison
; instruction is alrady sign extended.  Sign extension to 4 x i32 uses
; the pslld instruction.
define internal <4 x i32> @sextElimination(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp oeq <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: sextElimination
; CHECK: cmpeqps
; CHECK-NOT: pslld
}
; MIPS32-LABEL: sextElimination
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0

define internal <4 x i32> @fcmpFalseVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp false <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpFalseVector
; CHECK: pxor
}
; MIPS32-LABEL: fcmpFalseVector
; MIPS32: li v0,0
; MIPS32: li v1,0
; MIPS32: li a0,0
; MIPS32: li a1,0

define internal <4 x i32> @fcmpOeqVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp oeq <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpOeqVector
; CHECK: cmpeqps
}
; MIPS32-LABEL: fcmpOeqVector
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0

define internal <4 x i32> @fcmpOgeVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp oge <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpOgeVector
; CHECK: cmpleps
}
; MIPS32-LABEL: fcmpOgeVector
; MIPS32: c.ult.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ult.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ult.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ult.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0

define internal <4 x i32> @fcmpOgtVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp ogt <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpOgtVector
; CHECK: cmpltps
}
; MIPS32-LABEL: fcmpOgtVector
; MIPS32: c.ule.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ule.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ule.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ule.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0

define internal <4 x i32> @fcmpOleVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp ole <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpOleVector
; CHECK: cmpleps
}
; MIPS32-LABEL: fcmpOleVector
; MIPS32: c.ole.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ole.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ole.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ole.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0

define internal <4 x i32> @fcmpOltVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp olt <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpOltVector
; CHECK: cmpltps
}
; MIPS32-LABEL: fcmpOltVector
; MIPS32: c.olt.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.olt.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.olt.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.olt.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0

define internal <4 x i32> @fcmpOneVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp one <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpOneVector
; CHECK: cmpneqps
; CHECK: cmpordps
; CHECK: pand
}
; MIPS32-LABEL: fcmpOneVector
; MIPS32: c.ueq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ueq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ueq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ueq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0

define internal <4 x i32> @fcmpOrdVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp ord <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpOrdVector
; CHECK: cmpordps
}
; MIPS32-LABEL: fcmpOrdVector
; MIPS32: c.un.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.un.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.un.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.un.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0

define internal <4 x i32> @fcmpTrueVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp true <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpTrueVector
; CHECK: pcmpeqd
}
; MIPS32-LABEL: fcmpTrueVector
; MIPS32: li v0,1
; MIPS32: li v1,1
; MIPS32: li a0,1
; MIPS32: li a1,1

define internal <4 x i32> @fcmpUeqVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp ueq <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpUeqVector
; CHECK: cmpeqps
; CHECK: cmpunordps
; CHECK: por
}
; MIPS32-LABEL: fcmpUeqVector
; MIPS32: c.ueq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ueq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ueq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ueq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0

define internal <4 x i32> @fcmpUgeVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp uge <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpUgeVector
; CHECK: cmpnltps
}
; MIPS32-LABEL: fcmpUgeVector
; MIPS32: c.olt.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.olt.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.olt.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.olt.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0

define internal <4 x i32> @fcmpUgtVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp ugt <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpUgtVector
; CHECK: cmpnleps
}
; MIPS32-LABEL: fcmpUgtVector
; MIPS32: c.ole.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ole.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ole.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.ole.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0

define internal <4 x i32> @fcmpUleVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp ule <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpUleVector
; CHECK: cmpnltps
}
; MIPS32-LABEL: fcmpUleVector
; MIPS32: c.ule.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ule.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ule.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ule.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0

define internal <4 x i32> @fcmpUltVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp ult <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpUltVector
; CHECK: cmpnleps
}
; MIPS32-LABEL: fcmpUltVector
; MIPS32: c.ult.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ult.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ult.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.ult.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0

define internal <4 x i32> @fcmpUneVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp une <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpUneVector
; CHECK: cmpneqps
}
; MIPS32-LABEL: fcmpUneVector
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0
; MIPS32: c.eq.s
; MIPS32: li [[R:.*]],1
; MIPS32: movt [[R]],zero,$fcc0

define internal <4 x i32> @fcmpUnoVector(<4 x float> %a, <4 x float> %b) {
entry:
  %res.trunc = fcmp uno <4 x float> %a, %b
  %res = sext <4 x i1> %res.trunc to <4 x i32>
  ret <4 x i32> %res
; CHECK-LABEL: fcmpUnoVector
; CHECK: cmpunordps
}
; MIPS32-LABEL: fcmpUnoVector
; MIPS32: c.un.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.un.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.un.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
; MIPS32: c.un.s
; MIPS32: li [[R:.*]],1
; MIPS32: movf [[R]],zero,$fcc0
