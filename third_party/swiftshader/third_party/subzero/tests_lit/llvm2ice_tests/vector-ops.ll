; This checks support for insertelement and extractelement.

; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:   | FileCheck %s
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:   | FileCheck %s
; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 -mattr=sse4.1 \
; RUN:   | FileCheck --check-prefix=SSE41 %s
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 -mattr=sse4.1 \
; RUN:   | FileCheck --check-prefix=SSE41 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target mips32\
; RUN:   -i %s --args -O2 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; insertelement operations

define internal <4 x float> @insertelement_v4f32_0(<4 x float> %vec,
                                                   float %elt) {
entry:
  %res = insertelement <4 x float> %vec, float %elt, i32 0
  ret <4 x float> %res
; CHECK-LABEL: insertelement_v4f32_0
; CHECK: movss

; SSE41-LABEL: insertelement_v4f32_0
; SSE41: insertps {{.*}},{{.*}},0x0

; *** a0 - implicit return <4 x float>
; *** a1 - unused due to alignment of %vec
; *** a2:a3:sp[16]:s[20] - %vec
; *** sp[24] - %elt
; MIPS32-LABEL: insertelement_v4f32_0
; *** Load element 2 and 3 of %vec
; MIPS32: lw [[BV_E2:.*]],
; MIPS32: lw [[BV_E3:.*]],
; *** Load %elt
; MIPS32: lwc1 [[ELT:.*]],
; *** Insert %elt at %vec[0]
; MIPS32: mfc1 [[RV_E0:.*]],[[ELT]]
; MIPS32: move [[RET_PTR:.*]],a0
; MIPS32: sw [[RV_E0]],0([[RET_PTR]])
; MIPS32: sw a3,4([[RET_PTR]])
; MIPS32: sw [[BV_E2]],8([[RET_PTR]])
; MIPS32: sw [[BV_E3]],12([[RET_PTR]])
}

define internal <4 x i32> @insertelement_v4i32_0(<4 x i32> %vec, i32 %elt) {
entry:
  %res = insertelement <4 x i32> %vec, i32 %elt, i32 0
  ret <4 x i32> %res
; CHECK-LABEL: insertelement_v4i32_0
; CHECK: movd xmm{{.*}},
; CHECK: movss

; SSE41-LABEL: insertelement_v4i32_0
; SSE41: pinsrd {{.*}},{{.*}},0x0

; *** a0:a1:a2:a3 - %vec
; *** sp[16] - %elt
; MIPS32-LABEL: insertelement_v4i32_0
; *** Load %elt
; MIPS32: lw v0,16(sp)
; MIPS32: move v1,a1
; MIPS32: move a0,a2
; MIPS32: move a1,a3
}


define internal <4 x float> @insertelement_v4f32_1(<4 x float> %vec,
                                                   float %elt) {
entry:
  %res = insertelement <4 x float> %vec, float %elt, i32 1
  ret <4 x float> %res
; CHECK-LABEL: insertelement_v4f32_1
; CHECK: shufps
; CHECK: shufps

; SSE41-LABEL: insertelement_v4f32_1
; SSE41: insertps {{.*}},{{.*}},0x10

; MIPS32-LABEL: insertelement_v4f32_1
; MIPS32: lw [[VEC_E2:.*]],16(sp)
; MIPS32: lw [[VEC_E3:.*]],20(sp)
; MIPS32: lwc1 [[ELT:.*]],24(sp)
; MIPS32: mfc1 [[R_E1:.*]],[[ELT]]
; MIPS32: move [[PTR:.*]],a0
; MIPS32: sw a2,0([[PTR]])
; MIPS32: sw [[R_E1]],4([[PTR]])
; MIPS32: sw [[VEC_E2]],8([[PTR]])
; MIPS32: sw [[VEC_E3]],12([[PTR]])
}

define internal <4 x i32> @insertelement_v4i32_1(<4 x i32> %vec, i32 %elt) {
entry:
  %res = insertelement <4 x i32> %vec, i32 %elt, i32 1
  ret <4 x i32> %res
; CHECK-LABEL: insertelement_v4i32_1
; CHECK: shufps
; CHECK: shufps

; SSE41-LABEL: insertelement_v4i32_1
; SSE41: pinsrd {{.*}},{{.*}},0x1

; MIPS32-LABEL: insertelement_v4i32_1
; MIPS32: lw [[ELT:.*]],16(sp)
; MIPS32: move v1,[[ELT]]
; MIPS32: move v0,a0
; MIPS32: move a0,a2
; MIPS32: move a1,a3
}

define internal <8 x i16> @insertelement_v8i16(<8 x i16> %vec, i32 %elt.arg) {
entry:
  %elt = trunc i32 %elt.arg to i16
  %res = insertelement <8 x i16> %vec, i16 %elt, i32 1
  ret <8 x i16> %res
; CHECK-LABEL: insertelement_v8i16
; CHECK: pinsrw

; SSE41-LABEL: insertelement_v8i16
; SSE41: pinsrw

; MIPS32-LABEL: insertelement_v8i16
; MIPS32: lw [[ELT:.*]],16(sp)
; MIPS32: sll [[ELT]],[[ELT]],0x10
; MIPS32: sll a0,a0,0x10
; MIPS32: srl a0,a0,0x10
; MIPS32: or v0,[[ELT]],a0
; MIPS32: move v1,a1
; MIPS32: move a0,a2
; MIPS32: move a1,a3
}

define internal <16 x i8> @insertelement_v16i8(<16 x i8> %vec, i32 %elt.arg) {
entry:
  %elt = trunc i32 %elt.arg to i8
  %res = insertelement <16 x i8> %vec, i8 %elt, i32 1
  ret <16 x i8> %res
; CHECK-LABEL: insertelement_v16i8
; CHECK: movups
; CHECK: lea
; CHECK: mov

; SSE41-LABEL: insertelement_v16i8
; SSE41: pinsrb

; MIPS32-LABEL: insertelement_v16i8
; MIPS32: lw [[ELT:.*]],16(sp)
; MIPS32: andi [[ELT]],[[ELT]],0xff
; MIPS32: sll [[ELT]],[[ELT]],0x8
; MIPS32: lui [[T:.*]],0xffff
; MIPS32: ori [[T]],[[T]],0xff
; MIPS32: and a0,a0,[[T]]
; MIPS32: or v0,v0,a0
; MIPS32: move v1,a1
; MIPS32: move a0,a2
; MIPS32: move a1,a3
}

define internal <4 x i1> @insertelement_v4i1_0(<4 x i1> %vec, i32 %elt.arg) {
entry:
  %elt = trunc i32 %elt.arg to i1
  %res = insertelement <4 x i1> %vec, i1 %elt, i32 0
  ret <4 x i1> %res
; CHECK-LABEL: insertelement_v4i1_0
; CHECK: movss

; SSE41-LABEL: insertelement_v4i1_0
; SSE41: pinsrd {{.*}},{{.*}},0x0

; MIPS32-LABEL: insertelement_v4i1_0
; MIPS32: lw v0,16(sp)
; MIPS32: move v1,a1
; MIPS32: move a0,a2
; MIPS32: move a1,a3
}

define internal <4 x i1> @insertelement_v4i1_1(<4 x i1> %vec, i32 %elt.arg) {
entry:
  %elt = trunc i32 %elt.arg to i1
  %res = insertelement <4 x i1> %vec, i1 %elt, i32 1
  ret <4 x i1> %res
; CHECK-LABEL: insertelement_v4i1_1
; CHECK: shufps
; CHECK: shufps

; SSE41-LABEL: insertelement_v4i1_1
; SSE41: pinsrd {{.*}},{{.*}},0x1

; MIPS32-LABEL: insertelement_v4i1_1
; MIPS32: lw [[ELT:.*]],16(sp)
; MIPS32: move v1,[[ELT]]
; MIPS32: move v0,a0
; MIPS32: move a0,a2
; MIPS32: move a1,a3
}

define internal <8 x i1> @insertelement_v8i1(<8 x i1> %vec, i32 %elt.arg) {
entry:
  %elt = trunc i32 %elt.arg to i1
  %res = insertelement <8 x i1> %vec, i1 %elt, i32 1
  ret <8 x i1> %res
; CHECK-LABEL: insertelement_v8i1
; CHECK: pinsrw

; SSE41-LABEL: insertelement_v8i1
; SSE41: pinsrw

; MIPS32-LABEL: insertelement_v8i1
; MIPS32: lw [[ELT:.*]],16(sp)
; MIPS32: sll [[ELT]],[[ELT]],0x10
; MIPS32: sll a0,a0,0x10
; MIPS32: srl a0,a0,0x10
; MIPS32: or v0,[[ELT]],a0
; MIPS32: move v1,a1
; MIPS32: move a0,a2
; MIPS32: move a1,a3
}

define internal <16 x i1> @insertelement_v16i1(<16 x i1> %vec, i32 %elt.arg) {
entry:
  %elt = trunc i32 %elt.arg to i1
  %res = insertelement <16 x i1> %vec, i1 %elt, i32 1
  ret <16 x i1> %res
; CHECK-LABEL: insertelement_v16i1
; CHECK: movups
; CHECK: lea
; CHECK: mov

; SSE41-LABEL: insertelement_v16i1
; SSE41: pinsrb

; MIPS32-LABEL: insertelement_v16i1
; MIPS32: lw [[ELT:.*]],16(sp)
; MIPS32: andi [[ELT]],[[ELT]],0xff
; MIPS32: sll [[ELT]],[[ELT]],0x8
; MIPS32: lui [[T:.*]],0xffff
; MIPS32: ori [[T]],[[T]],0xff
; MIPS32: and a0,a0,[[T]]
; MIPS32: or v0,[[ELT]],a0
; MIPS32: move v1,a1
; MIPS32: move a0,a2
; MIPS32: move a1,a3
}

; extractelement operations

define internal float @extractelement_v4f32(<4 x float> %vec) {
entry:
  %res = extractelement <4 x float> %vec, i32 1
  ret float %res
; CHECK-LABEL: extractelement_v4f32
; CHECK: pshufd

; SSE41-LABEL: extractelement_v4f32
; SSE41: pshufd

; MIPS32-LABEL: extractelement_v4f32
; MIPS32: mtc1 a1,$f0
}

define internal i32 @extractelement_v4i32(<4 x i32> %vec) {
entry:
  %res = extractelement <4 x i32> %vec, i32 1
  ret i32 %res
; CHECK-LABEL: extractelement_v4i32
; CHECK: pshufd
; CHECK: movd {{.*}},xmm

; SSE41-LABEL: extractelement_v4i32
; SSE41: pextrd

; MIPS32-LABEL: extractelement_v4i32
; MIPS32L move v0,a1
}

define internal i32 @extractelement_v8i16(<8 x i16> %vec) {
entry:
  %res = extractelement <8 x i16> %vec, i32 1
  %res.ext = zext i16 %res to i32
  ret i32 %res.ext
; CHECK-LABEL: extractelement_v8i16
; CHECK: pextrw

; SSE41-LABEL: extractelement_v8i16
; SSE41: pextrw

; MIPS32-LABEL: extractelement_v8i16
; MIPS32: srl a0,a0,0x10
; MIPS32: andi a0,a0,0xffff
; MIPS32: move v0,a0
}

define internal i32 @extractelement_v16i8(<16 x i8> %vec) {
entry:
  %res = extractelement <16 x i8> %vec, i32 1
  %res.ext = zext i8 %res to i32
  ret i32 %res.ext
; CHECK-LABEL: extractelement_v16i8
; CHECK: movups
; CHECK: lea
; CHECK: mov

; SSE41-LABEL: extractelement_v16i8
; SSE41: pextrb

; MIPS32-LABEL: extractelement_v16i8
; MIPS32: srl a0,a0,0x8
; MIPS32: andi a0,a0,0xff
; MIPS32: andi a0,a0,0xff
; MIPS32: move v0,a0
}

define internal i32 @extractelement_v4i1(<4 x i1> %vec) {
entry:
  %res = extractelement <4 x i1> %vec, i32 1
  %res.ext = zext i1 %res to i32
  ret i32 %res.ext
; CHECK-LABEL: extractelement_v4i1
; CHECK: pshufd

; SSE41-LABEL: extractelement_v4i1
; SSE41: pextrd

; MIPS32-LABEL: extractelement_v4i1
; MIPS32: andi a1,a1,0x1
; MIPS32: andi a1,a1,0x1
; MIPS32: move v0,a1
}

define internal i32 @extractelement_v8i1(<8 x i1> %vec) {
entry:
  %res = extractelement <8 x i1> %vec, i32 1
  %res.ext = zext i1 %res to i32
  ret i32 %res.ext
; CHECK-LABEL: extractelement_v8i1
; CHECK: pextrw

; SSE41-LABEL: extractelement_v8i1
; SSE41: pextrw

; MIPS32-LABEL: extractelement_v8i1
; MIPS32: srl a0,a0,0x10
; MIPS32: andi a0,a0,0x1
; MIPS32: andi a0,a0,0x1
; MIPS32: move v0,a0
}

define internal i32 @extractelement_v16i1(<16 x i1> %vec) {
entry:
  %res = extractelement <16 x i1> %vec, i32 1
  %res.ext = zext i1 %res to i32
  ret i32 %res.ext
; CHECK-LABEL: extractelement_v16i1
; CHECK: movups
; CHECK: lea
; CHECK: mov

; SSE41-LABEL: extractelement_v16i1
; SSE41: pextrb

; MIPS32-LABEL: extractelement_v16i1
; MIPS32: srl a0,a0,0x8
; MIPS32: andi a0,a0,0xff
; MIPS32: andi a0,a0,0x1
; MIPS32: andi a0,a0,0x1
; MIPS32: move v0,a0
}
