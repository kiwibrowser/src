; This file tests support for the select instruction with vector valued inputs.

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

define internal <16 x i8> @test_select_v16i8(<16 x i1> %cond, <16 x i8> %arg1,
                                             <16 x i8> %arg2) {
entry:
  %res = select <16 x i1> %cond, <16 x i8> %arg1, <16 x i8> %arg2
  ret <16 x i8> %res
; CHECK-LABEL: test_select_v16i8
; CHECK: pand
; CHECK: pandn
; CHECK: por

; SSE41-LABEL: test_select_v16i8
; SSE41: pblendvb xmm{{[0-7]}},{{xmm[0-7]|XMMWORD}}

; MIPS32-LABEL: test_select_v16i8
; MIPS32: addiu [[T0:.*]],sp,-32
; MIPS32: sw [[T1:.*]],
; MIPS32: sw [[T2:.*]],
; MIPS32: sw [[T3:.*]],
; MIPS32: sw [[T4:.*]],
; MIPS32: sw [[T5:.*]],
; MIPS32: lw [[T6:.*]],
; MIPS32: lw [[T7:.*]],
; MIPS32: lw [[T8:.*]],
; MIPS32: lw [[T9:.*]],
; MIPS32: lw [[T10:.*]],
; MIPS32: lw [[T11:.*]],
; MIPS32: lw [[T12:.*]],
; MIPS32: lw [[T13:.*]],
; MIPS32: move [[T14:.*]],zero
; MIPS32: move [[T15:.*]],zero
; MIPS32: move [[T5]],zero
; MIPS32: move [[T4]],zero
; MIPS32: move [[T3]],a0
; MIPS32: andi [[T3]],[[T3]],0xff
; MIPS32: andi [[T3]],[[T3]],0x1
; MIPS32: move [[T2]],[[T6]]
; MIPS32: andi [[T2]],[[T2]],0xff
; MIPS32: move [[T1]],[[T10]]
; MIPS32: andi [[T1]],[[T1]],0xff
; MIPS32: movn [[T1]],[[T2]],[[T3]]
; MIPS32: andi [[T1]],[[T1]],0xff
; MIPS32: srl [[T14]],[[T14]],0x8
; MIPS32: sll [[T14]],[[T14]],0x8
; MIPS32: or [[T1]],[[T1]],[[T14]]
; MIPS32: move [[T14]],a0
; MIPS32: srl [[T14]],[[T14]],0x8
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: andi [[T14]],[[T14]],0x1
; MIPS32: move [[T3]],[[T6]]
; MIPS32: srl [[T3]],[[T3]],0x8
; MIPS32: andi [[T3]],[[T3]],0xff
; MIPS32: move [[T2]],[[T10]]
; MIPS32: srl [[T2]],[[T2]],0x8
; MIPS32: andi [[T2]],[[T2]],0xff
; MIPS32: movn [[T2]],[[T3]],[[T14]]
; MIPS32: andi [[T2]],[[T2]],0xff
; MIPS32: sll [[T2]],[[T2]],0x8
; MIPS32: lui [[T14]],0xffff
; MIPS32: ori [[T14]],[[T14]],0xff
; MIPS32: and [[T1]],[[T1]],[[T14]]
; MIPS32: or [[T2]],[[T2]],[[T1]]
; MIPS32: move [[T14]],a0
; MIPS32: srl [[T14]],[[T14]],0x10
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: andi [[T14]],[[T14]],0x1
; MIPS32: move [[T3]],[[T6]]
; MIPS32: srl [[T3]],[[T3]],0x10
; MIPS32: andi [[T3]],[[T3]],0xff
; MIPS32: move [[T1]],[[T10]]
; MIPS32: srl [[T1]],[[T1]],0x10
; MIPS32: andi [[T1]],[[T1]],0xff
; MIPS32: movn [[T1]],[[T3]],[[T14]]
; MIPS32: andi [[T1]],[[T1]],0xff
; MIPS32: sll [[T1]],[[T1]],0x10
; MIPS32: lui [[T14]],0xff00
; MIPS32: ori [[T14]],[[T14]],0xffff
; MIPS32: and [[T2]],[[T2]],[[T14]]
; MIPS32: or [[T1]],[[T1]],[[T2]]
; MIPS32: srl [[T16:.*]],a0,0x18
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: srl [[T6]],[[T6]],0x18
; MIPS32: srl [[T10]],[[T10]],0x18
; MIPS32: movn [[T10]],[[T6]],[[T16]]
; MIPS32: sll [[T10]],[[T10]],0x18
; MIPS32: sll [[T1]],[[T1]],0x8
; MIPS32: srl [[T1]],[[T1]],0x8
; MIPS32: or [[T10]],[[T10]],[[T1]]
; MIPS32: move [[T6]],a1
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T16]],[[T7]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: move [[T14]],[[T11]]
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: movn [[T14]],[[T16]],[[T6]]
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: srl [[T15]],[[T15]],0x8
; MIPS32: sll [[T15]],[[T15]],0x8
; MIPS32: or [[T14]],[[T14]],[[T15]]
; MIPS32: move [[T6]],a1
; MIPS32: srl [[T6]],[[T6]],0x8
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T16]],[[T7]]
; MIPS32: srl [[T16]],[[T16]],0x8
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: move [[T15]],[[T11]]
; MIPS32: srl [[T15]],[[T15]],0x8
; MIPS32: andi [[T15]],[[T15]],0xff
; MIPS32: movn [[T15]],[[T16]],[[T6]]
; MIPS32: andi [[T15]],[[T15]],0xff
; MIPS32: sll [[T15]],[[T15]],0x8
; MIPS32: lui [[T6]],0xffff
; MIPS32: ori [[T6]],[[T6]],0xff
; MIPS32: and [[T14]],[[T14]],[[T6]]
; MIPS32: or [[T15]],[[T15]],[[T14]]
; MIPS32: move [[T6]],a1
; MIPS32: srl [[T6]],[[T6]],0x10
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T16]],[[T7]]
; MIPS32: srl [[T16]],[[T16]],0x10
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: move [[T14]],[[T11]]
; MIPS32: srl [[T14]],[[T14]],0x10
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: movn [[T14]],[[T16]],[[T6]]
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: sll [[T14]],[[T14]],0x10
; MIPS32: lui [[T6]],0xff00
; MIPS32: ori [[T6]],[[T6]],0xffff
; MIPS32: and [[T15]],[[T15]],[[T6]]
; MIPS32: or [[T14]],[[T14]],[[T15]]
; MIPS32: srl [[T17:.*]],a1,0x18
; MIPS32: andi [[T17]],[[T17]],0x1
; MIPS32: srl [[T7]],[[T7]],0x18
; MIPS32: srl [[T11]],[[T11]],0x18
; MIPS32: movn [[T11]],[[T7]],[[T17]]
; MIPS32: sll [[T11]],[[T11]],0x18
; MIPS32: sll [[T14]],[[T14]],0x8
; MIPS32: srl [[T14]],[[T14]],0x8
; MIPS32: or [[T11]],[[T11]],[[T14]]
; MIPS32: move [[T6]],a2
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T8]]
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: move [[T16]],[[T12]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: srl [[T5]],[[T5]],0x8
; MIPS32: sll [[T5]],[[T5]],0x8
; MIPS32: or [[T16]],[[T16]],[[T5]]
; MIPS32: move [[T6]],a2
; MIPS32: srl [[T6]],[[T6]],0x8
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T8]]
; MIPS32: srl [[T7]],[[T7]],0x8
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: move [[T17]],[[T12]]
; MIPS32: srl [[T17]],[[T17]],0x8
; MIPS32: andi [[T17]],[[T17]],0xff
; MIPS32: movn [[T17]],[[T7]],[[T6]]
; MIPS32: andi [[T17]],[[T17]],0xff
; MIPS32: sll [[T17]],[[T17]],0x8
; MIPS32: lui [[T6]],0xffff
; MIPS32: ori [[T6]],[[T6]],0xff
; MIPS32: and [[T16]],[[T16]],[[T6]]
; MIPS32: or [[T17]],[[T17]],[[T16]]
; MIPS32: move [[T6]],a2
; MIPS32: srl [[T6]],[[T6]],0x10
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T8]]
; MIPS32: srl [[T7]],[[T7]],0x10
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: move [[T16]],[[T12]]
; MIPS32: srl [[T16]],[[T16]],0x10
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: sll [[T16]],[[T16]],0x10
; MIPS32: lui [[T6]],0xff00
; MIPS32: ori [[T6]],[[T6]],0xffff
; MIPS32: and [[T17]],[[T17]],[[T6]]
; MIPS32: or [[T16]],[[T16]],[[T17]]
; MIPS32: srl [[T18:.*]],a2,0x18
; MIPS32: andi [[T18]],[[T18]],0x1
; MIPS32: srl [[T8]],[[T8]],0x18
; MIPS32: srl [[T12]],[[T12]],0x18
; MIPS32: movn [[T12]],[[T8]],[[T18]]
; MIPS32: sll [[T12]],[[T12]],0x18
; MIPS32: sll [[T16]],[[T16]],0x8
; MIPS32: srl [[T16]],[[T16]],0x8
; MIPS32: or [[T12]],[[T12]],[[T16]]
; MIPS32: move [[T6]],a3
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T9]]
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: move [[T16]],[[T13]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: srl [[T4]],[[T4]],0x8
; MIPS32: sll [[T4]],[[T4]],0x8
; MIPS32: or [[T16]],[[T16]],[[T4]]
; MIPS32: move [[T6]],a3
; MIPS32: srl [[T6]],[[T6]],0x8
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T9]]
; MIPS32: srl [[T7]],[[T7]],0x8
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: move [[T17]],[[T13]]
; MIPS32: srl [[T17]],[[T17]],0x8
; MIPS32: andi [[T17]],[[T17]],0xff
; MIPS32: movn [[T17]],[[T7]],[[T6]]
; MIPS32: andi [[T17]],[[T17]],0xff
; MIPS32: sll [[T17]],[[T17]],0x8
; MIPS32: lui [[T6]],0xffff
; MIPS32: ori [[T6]],[[T6]],0xff
; MIPS32: and [[T16]],[[T16]],[[T6]]
; MIPS32: or [[T17]],[[T17]],[[T16]]
; MIPS32: move [[T6]],a3
; MIPS32: srl [[T6]],[[T6]],0x10
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T9]]
; MIPS32: srl [[T7]],[[T7]],0x10
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: move [[T16]],[[T13]]
; MIPS32: srl [[T16]],[[T16]],0x10
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: sll [[T16]],[[T16]],0x10
; MIPS32: lui [[T6]],0xff00
; MIPS32: ori [[T6]],[[T6]],0xffff
; MIPS32: and [[T17]],[[T17]],[[T6]]
; MIPS32: or [[T16]],[[T16]],[[T17]]
; MIPS32: srl [[T19:.*]],a3,0x18
; MIPS32: andi [[T19]],[[T19]],0x1
; MIPS32: srl [[T9]],[[T9]],0x18
; MIPS32: srl [[T13]],[[T13]],0x18
; MIPS32: movn [[T13]],[[T9]],[[T19]]
; MIPS32: sll [[T13]],[[T13]],0x18
; MIPS32: sll [[T16]],[[T16]],0x8
; MIPS32: srl [[T16]],[[T16]],0x8
; MIPS32: or [[T13]],[[T13]],[[T16]]
; MIPS32: move v0,[[T10]]
; MIPS32: move v1,[[T11]]
; MIPS32: move a0,[[T12]]
; MIPS32: move a1,[[T13]]
; MIPS32: lw [[T5]],
; MIPS32: lw [[T4]],
; MIPS32: lw [[T3]],
; MIPS32: lw [[T2]],
; MIPS32: lw [[T1]],
; MIPS32: addiu [[T0]],sp,32
}

define internal <16 x i1> @test_select_v16i1(<16 x i1> %cond, <16 x i1> %arg1,
                                             <16 x i1> %arg2) {
entry:
  %res = select <16 x i1> %cond, <16 x i1> %arg1, <16 x i1> %arg2
  ret <16 x i1> %res
; CHECK-LABEL: test_select_v16i1
; CHECK: pand
; CHECK: pandn
; CHECK: por

; SSE41-LABEL: test_select_v16i1
; SSE41: pblendvb xmm{{[0-7]}},{{xmm[0-7]|XMMWORD}}

; MIPS32-LABEL: test_select_v16i1
; MIPS32: addiu [[T0:.*]],sp,-32
; MIPS32: sw [[T1:.*]],
; MIPS32: sw [[T2:.*]],
; MIPS32: sw [[T3:.*]],
; MIPS32: sw [[T4:.*]],
; MIPS32: sw [[T5:.*]],
; MIPS32: lw [[T6:.*]],
; MIPS32: lw [[T7:.*]],
; MIPS32: lw [[T8:.*]],
; MIPS32: lw [[T9:.*]],
; MIPS32: lw [[T10:.*]],
; MIPS32: lw [[T11:.*]],
; MIPS32: lw [[T12:.*]],
; MIPS32: lw [[T13:.*]],
; MIPS32: move [[T14:.*]],zero
; MIPS32: move [[T15:.*]],zero
; MIPS32: move [[T5]],zero
; MIPS32: move [[T4]],zero
; MIPS32: move [[T3]],a0
; MIPS32: andi [[T3]],[[T3]],0xff
; MIPS32: andi [[T3]],[[T3]],0x1
; MIPS32: move [[T2]],[[T6]]
; MIPS32: andi [[T2]],[[T2]],0xff
; MIPS32: andi [[T2]],[[T2]],0x1
; MIPS32: move [[T1]],[[T10]]
; MIPS32: andi [[T1]],[[T1]],0xff
; MIPS32: andi [[T1]],[[T1]],0x1
; MIPS32: movn [[T1]],[[T2]],[[T3]]
; MIPS32: andi [[T1]],[[T1]],0xff
; MIPS32: srl [[T14]],[[T14]],0x8
; MIPS32: sll [[T14]],[[T14]],0x8
; MIPS32: or [[T1]],[[T1]],[[T14]]
; MIPS32: move [[T14]],a0
; MIPS32: srl [[T14]],[[T14]],0x8
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: andi [[T14]],[[T14]],0x1
; MIPS32: move [[T3]],[[T6]]
; MIPS32: srl [[T3]],[[T3]],0x8
; MIPS32: andi [[T3]],[[T3]],0xff
; MIPS32: andi [[T3]],[[T3]],0x1
; MIPS32: move [[T2]],[[T10]]
; MIPS32: srl [[T2]],[[T2]],0x8
; MIPS32: andi [[T2]],[[T2]],0xff
; MIPS32: andi [[T2]],[[T2]],0x1
; MIPS32: movn [[T2]],[[T3]],[[T14]]
; MIPS32: andi [[T2]],[[T2]],0xff
; MIPS32: sll [[T2]],[[T2]],0x8
; MIPS32: lui [[T14]],0xffff
; MIPS32: ori [[T14]],[[T14]],0xff
; MIPS32: and [[T1]],[[T1]],[[T14]]
; MIPS32: or [[T2]],[[T2]],[[T1]]
; MIPS32: move [[T14]],a0
; MIPS32: srl [[T14]],[[T14]],0x10
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: andi [[T14]],[[T14]],0x1
; MIPS32: move [[T3]],[[T6]]
; MIPS32: srl [[T3]],[[T3]],0x10
; MIPS32: andi [[T3]],[[T3]],0xff
; MIPS32: andi [[T3]],[[T3]],0x1
; MIPS32: move [[T1]],[[T10]]
; MIPS32: srl [[T1]],[[T1]],0x10
; MIPS32: andi [[T1]],[[T1]],0xff
; MIPS32: andi [[T1]],[[T1]],0x1
; MIPS32: movn [[T1]],[[T3]],[[T14]]
; MIPS32: andi [[T1]],[[T1]],0xff
; MIPS32: sll [[T1]],[[T1]],0x10
; MIPS32: lui [[T14]],0xff00
; MIPS32: ori [[T14]],[[T14]],0xffff
; MIPS32: and [[T2]],[[T2]],[[T14]]
; MIPS32: or [[T1]],[[T1]],[[T2]]
; MIPS32: srl [[T16:.*]],a0,0x18
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: srl [[T6]],[[T6]],0x18
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: srl [[T10]],[[T10]],0x18
; MIPS32: andi [[T10]],[[T10]],0x1
; MIPS32: movn [[T10]],[[T6]],[[T16]]
; MIPS32: sll [[T10]],[[T10]],0x18
; MIPS32: sll [[T1]],[[T1]],0x8
; MIPS32: srl [[T1]],[[T1]],0x8
; MIPS32: or [[T10]],[[T10]],[[T1]]
; MIPS32: move [[T6]],a1
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T16]],[[T7]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: move [[T14]],[[T11]]
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: andi [[T14]],[[T14]],0x1
; MIPS32: movn [[T14]],[[T16]],[[T6]]
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: srl [[T15]],[[T15]],0x8
; MIPS32: sll [[T15]],[[T15]],0x8
; MIPS32: or [[T14]],[[T14]],[[T15]]
; MIPS32: move [[T6]],a1
; MIPS32: srl [[T6]],[[T6]],0x8
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T16]],[[T7]]
; MIPS32: srl [[T16]],[[T16]],0x8
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: move [[T15]],[[T11]]
; MIPS32: srl [[T15]],[[T15]],0x8
; MIPS32: andi [[T15]],[[T15]],0xff
; MIPS32: andi [[T15]],[[T15]],0x1
; MIPS32: movn [[T15]],[[T16]],[[T6]]
; MIPS32: andi [[T15]],[[T15]],0xff
; MIPS32: sll [[T15]],[[T15]],0x8
; MIPS32: lui [[T6]],0xffff
; MIPS32: ori [[T6]],[[T6]],0xff
; MIPS32: and [[T14]],[[T14]],[[T6]]
; MIPS32: or [[T15]],[[T15]],[[T14]]
; MIPS32: move [[T6]],a1
; MIPS32: srl [[T6]],[[T6]],0x10
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T16]],[[T7]]
; MIPS32: srl [[T16]],[[T16]],0x10
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: move [[T14]],[[T11]]
; MIPS32: srl [[T14]],[[T14]],0x10
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: andi [[T14]],[[T14]],0x1
; MIPS32: movn [[T14]],[[T16]],[[T6]]
; MIPS32: andi [[T14]],[[T14]],0xff
; MIPS32: sll [[T14]],[[T14]],0x10
; MIPS32: lui [[T6]],0xff00
; MIPS32: ori [[T6]],[[T6]],0xffff
; MIPS32: and [[T15]],[[T15]],[[T6]]
; MIPS32: or [[T14]],[[T14]],[[T15]]
; MIPS32: srl [[T17:.*]],a1,0x18
; MIPS32: andi [[T17]],[[T17]],0x1
; MIPS32: srl [[T7]],[[T7]],0x18
; MIPS32: andi [[T7]],[[T7]],0x1
; MIPS32: srl [[T11]],[[T11]],0x18
; MIPS32: andi [[T11]],[[T11]],0x1
; MIPS32: movn [[T11]],[[T7]],[[T17]]
; MIPS32: sll [[T11]],[[T11]],0x18
; MIPS32: sll [[T14]],[[T14]],0x8
; MIPS32: srl [[T14]],[[T14]],0x8
; MIPS32: or [[T11]],[[T11]],[[T14]]
; MIPS32: move [[T6]],a2
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T8]]
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: andi [[T7]],[[T7]],0x1
; MIPS32: move [[T16]],[[T12]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: srl [[T5]],[[T5]],0x8
; MIPS32: sll [[T5]],[[T5]],0x8
; MIPS32: or [[T16]],[[T16]],[[T5]]
; MIPS32: move [[T6]],a2
; MIPS32: srl [[T6]],[[T6]],0x8
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T8]]
; MIPS32: srl [[T7]],[[T7]],0x8
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: andi [[T7]],[[T7]],0x1
; MIPS32: move [[T17]],[[T12]]
; MIPS32: srl [[T17]],[[T17]],0x8
; MIPS32: andi [[T17]],[[T17]],0xff
; MIPS32: andi [[T17]],[[T17]],0x1
; MIPS32: movn [[T17]],[[T7]],[[T6]]
; MIPS32: andi [[T17]],[[T17]],0xff
; MIPS32: sll [[T17]],[[T17]],0x8
; MIPS32: lui [[T6]],0xffff
; MIPS32: ori [[T6]],[[T6]],0xff
; MIPS32: and [[T16]],[[T16]],[[T6]]
; MIPS32: or [[T17]],[[T17]],[[T16]]
; MIPS32: move [[T6]],a2
; MIPS32: srl [[T6]],[[T6]],0x10
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T8]]
; MIPS32: srl [[T7]],[[T7]],0x10
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: andi [[T7]],[[T7]],0x1
; MIPS32: move [[T16]],[[T12]]
; MIPS32: srl [[T16]],[[T16]],0x10
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: sll [[T16]],[[T16]],0x10
; MIPS32: lui [[T6]],0xff00
; MIPS32: ori [[T6]],[[T6]],0xffff
; MIPS32: and [[T17]],[[T17]],[[T6]]
; MIPS32: or [[T16]],[[T16]],[[T17]]
; MIPS32: srl [[T18:.*]],a2,0x18
; MIPS32: andi [[T18]],[[T18]],0x1
; MIPS32: srl [[T8]],[[T8]],0x18
; MIPS32: andi [[T8]],[[T8]],0x1
; MIPS32: srl [[T12]],[[T12]],0x18
; MIPS32: andi [[T12]],[[T12]],0x1
; MIPS32: movn [[T12]],[[T8]],[[T18]]
; MIPS32: sll [[T12]],[[T12]],0x18
; MIPS32: sll [[T16]],[[T16]],0x8
; MIPS32: srl [[T16]],[[T16]],0x8
; MIPS32: or [[T12]],[[T12]],[[T16]]
; MIPS32: move [[T6]],a3
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T9]]
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: andi [[T7]],[[T7]],0x1
; MIPS32: move [[T16]],[[T13]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: srl [[T4]],[[T4]],0x8
; MIPS32: sll [[T4]],[[T4]],0x8
; MIPS32: or [[T16]],[[T16]],[[T4]]
; MIPS32: move [[T6]],a3
; MIPS32: srl [[T6]],[[T6]],0x8
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T9]]
; MIPS32: srl [[T7]],[[T7]],0x8
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: andi [[T7]],[[T7]],0x1
; MIPS32: move [[T17]],[[T13]]
; MIPS32: srl [[T17]],[[T17]],0x8
; MIPS32: andi [[T17]],[[T17]],0xff
; MIPS32: andi [[T17]],[[T17]],0x1
; MIPS32: movn [[T17]],[[T7]],[[T6]]
; MIPS32: andi [[T17]],[[T17]],0xff
; MIPS32: sll [[T17]],[[T17]],0x8
; MIPS32: lui [[T6]],0xffff
; MIPS32: ori [[T6]],[[T6]],0xff
; MIPS32: and [[T16]],[[T16]],[[T6]]
; MIPS32: or [[T17]],[[T17]],[[T16]]
; MIPS32: move [[T6]],a3
; MIPS32: srl [[T6]],[[T6]],0x10
; MIPS32: andi [[T6]],[[T6]],0xff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T9]]
; MIPS32: srl [[T7]],[[T7]],0x10
; MIPS32: andi [[T7]],[[T7]],0xff
; MIPS32: andi [[T7]],[[T7]],0x1
; MIPS32: move [[T16]],[[T13]]
; MIPS32: srl [[T16]],[[T16]],0x10
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xff
; MIPS32: sll [[T16]],[[T16]],0x10
; MIPS32: lui [[T6]],0xff00
; MIPS32: ori [[T6]],[[T6]],0xffff
; MIPS32: and [[T17]],[[T17]],[[T6]]
; MIPS32: or [[T16]],[[T16]],[[T17]]
; MIPS32: srl [[T19:.*]],a3,0x18
; MIPS32: andi [[T19]],[[T19]],0x1
; MIPS32: srl [[T9]],[[T9]],0x18
; MIPS32: andi [[T9]],[[T9]],0x1
; MIPS32: srl [[T13]],[[T13]],0x18
; MIPS32: andi [[T13]],[[T13]],0x1
; MIPS32: movn [[T13]],[[T9]],[[T19]]
; MIPS32: sll [[T13]],[[T13]],0x18
; MIPS32: sll [[T16]],[[T16]],0x8
; MIPS32: srl [[T16]],[[T16]],0x8
; MIPS32: or [[T13]],[[T13]],[[T16]]
; MIPS32: move v0,[[T10]]
; MIPS32: move v1,[[T11]]
; MIPS32: move a0,[[T12]]
; MIPS32: move a1,[[T13]]
; MIPS32: lw [[T5]],
; MIPS32: lw [[T4]],
; MIPS32: lw [[T3]],
; MIPS32: lw [[T2]],
; MIPS32: lw [[T1]],
; MIPS32: addiu [[T0]],sp,32
}

define internal <8 x i16> @test_select_v8i16(<8 x i1> %cond, <8 x i16> %arg1,
                                             <8 x i16> %arg2) {
entry:
  %res = select <8 x i1> %cond, <8 x i16> %arg1, <8 x i16> %arg2
  ret <8 x i16> %res
; CHECK-LABEL: test_select_v8i16
; CHECK: pand
; CHECK: pandn
; CHECK: por

; SSE41-LABEL: test_select_v8i16
; SSE41: pblendvb xmm{{[0-7]}},{{xmm[0-7]|XMMWORD}}

; MIPS32-LABEL: test_select_v8i16
; MIPS32: addiu [[T0:.*]],sp,-32
; MIPS32: sw [[T1:.*]],
; MIPS32: sw [[T2:.*]],
; MIPS32: sw [[T3:.*]],
; MIPS32: sw [[T4:.*]],
; MIPS32: sw [[T5:.*]],
; MIPS32: lw [[T6:.*]],
; MIPS32: lw [[T7:.*]],
; MIPS32: lw [[T8:.*]],
; MIPS32: lw [[T9:.*]],
; MIPS32: lw [[T10:.*]],
; MIPS32: lw [[T11:.*]],
; MIPS32: lw [[T12:.*]],
; MIPS32: lw [[T13:.*]],
; MIPS32: move [[T14:.*]],zero
; MIPS32: move [[T15:.*]],zero
; MIPS32: move [[T5]],zero
; MIPS32: move [[T4]],zero
; MIPS32: move [[T3]],a0
; MIPS32: andi [[T3]],[[T3]],0xffff
; MIPS32: andi [[T3]],[[T3]],0x1
; MIPS32: move [[T2]],[[T6]]
; MIPS32: andi [[T2]],[[T2]],0xffff
; MIPS32: move [[T1]],[[T10]]
; MIPS32: andi [[T1]],[[T1]],0xffff
; MIPS32: movn [[T1]],[[T2]],[[T3]]
; MIPS32: andi [[T1]],[[T1]],0xffff
; MIPS32: srl [[T14]],[[T14]],0x10
; MIPS32: sll [[T14]],[[T14]],0x10
; MIPS32: or [[T1]],[[T1]],[[T14]]
; MIPS32: srl [[T16:.*]],a0,0x10
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: srl [[T6]],[[T6]],0x10
; MIPS32: srl [[T10]],[[T10]],0x10
; MIPS32: movn [[T10]],[[T6]],[[T16]]
; MIPS32: sll [[T10]],[[T10]],0x10
; MIPS32: sll [[T1]],[[T1]],0x10
; MIPS32: srl [[T1]],[[T1]],0x10
; MIPS32: or [[T10]],[[T10]],[[T1]]
; MIPS32: move [[T6]],a1
; MIPS32: andi [[T6]],[[T6]],0xffff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T16]],[[T7]]
; MIPS32: andi [[T16]],[[T16]],0xffff
; MIPS32: move [[T14]],[[T11]]
; MIPS32: andi [[T14]],[[T14]],0xffff
; MIPS32: movn [[T14]],[[T16]],[[T6]]
; MIPS32: andi [[T14]],[[T14]],0xffff
; MIPS32: srl [[T15]],[[T15]],0x10
; MIPS32: sll [[T15]],[[T15]],0x10
; MIPS32: or [[T14]],[[T14]],[[T15]]
; MIPS32: srl [[T17:.*]],a1,0x10
; MIPS32: andi [[T17]],[[T17]],0x1
; MIPS32: srl [[T7]],[[T7]],0x10
; MIPS32: srl [[T11]],[[T11]],0x10
; MIPS32: movn [[T11]],[[T7]],[[T17]]
; MIPS32: sll [[T11]],[[T11]],0x10
; MIPS32: sll [[T14]],[[T14]],0x10
; MIPS32: srl [[T14]],[[T14]],0x10
; MIPS32: or [[T11]],[[T11]],[[T14]]
; MIPS32: move [[T6]],a2
; MIPS32: andi [[T6]],[[T6]],0xffff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T8]]
; MIPS32: andi [[T7]],[[T7]],0xffff
; MIPS32: move [[T16]],[[T12]]
; MIPS32: andi [[T16]],[[T16]],0xffff
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xffff
; MIPS32: srl [[T5]],[[T5]],0x10
; MIPS32: sll [[T5]],[[T5]],0x10
; MIPS32: or [[T16]],[[T16]],[[T5]]
; MIPS32: srl [[T18:.*]],a2,0x10
; MIPS32: andi [[T18]],[[T18]],0x1
; MIPS32: srl [[T8]],[[T8]],0x10
; MIPS32: srl [[T12]],[[T12]],0x10
; MIPS32: movn [[T12]],[[T8]],[[T18]]
; MIPS32: sll [[T12]],[[T12]],0x10
; MIPS32: sll [[T16]],[[T16]],0x10
; MIPS32: srl [[T16]],[[T16]],0x10
; MIPS32: or [[T12]],[[T12]],[[T16]]
; MIPS32: move [[T6]],a3
; MIPS32: andi [[T6]],[[T6]],0xffff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T9]]
; MIPS32: andi [[T7]],[[T7]],0xffff
; MIPS32: move [[T16]],[[T13]]
; MIPS32: andi [[T16]],[[T16]],0xffff
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xffff
; MIPS32: srl [[T4]],[[T4]],0x10
; MIPS32: sll [[T4]],[[T4]],0x10
; MIPS32: or [[T16]],[[T16]],[[T4]]
; MIPS32: srl [[T19:.*]],a3,0x10
; MIPS32: andi [[T19]],[[T19]],0x1
; MIPS32: srl [[T9]],[[T9]],0x10
; MIPS32: srl [[T13]],[[T13]],0x10
; MIPS32: movn [[T13]],[[T9]],[[T19]]
; MIPS32: sll [[T13]],[[T13]],0x10
; MIPS32: sll [[T16]],[[T16]],0x10
; MIPS32: srl [[T16]],[[T16]],0x10
; MIPS32: or [[T13]],[[T13]],[[T16]]
; MIPS32: move v0,[[T10]]
; MIPS32: move v1,[[T11]]
; MIPS32: move a0,[[T12]]
; MIPS32: move a1,[[T13]]
; MIPS32: lw [[T5]],
; MIPS32: lw [[T4]],
; MIPS32: lw [[T3]],
; MIPS32: lw [[T2]],
; MIPS32: lw [[T1]],
; MIPS32: addiu [[T0]],sp,32
}

define internal <8 x i1> @test_select_v8i1(<8 x i1> %cond, <8 x i1> %arg1,
                                           <8 x i1> %arg2) {
entry:
  %res = select <8 x i1> %cond, <8 x i1> %arg1, <8 x i1> %arg2
  ret <8 x i1> %res
; CHECK-LABEL: test_select_v8i1
; CHECK: pand
; CHECK: pandn
; CHECK: por

; SSE41-LABEL: test_select_v8i1
; SSE41: pblendvb xmm{{[0-7]}},{{xmm[0-7]|XMMWORD}}

; MIPS32-LABEL: test_select_v8i1
; MIPS32: addiu [[T0:.*]],sp,-32
; MIPS32: sw [[T1:.*]],
; MIPS32: sw [[T2:.*]],
; MIPS32: sw [[T3:.*]],
; MIPS32: sw [[T4:.*]],
; MIPS32: sw [[T5:.*]],
; MIPS32: lw [[T6:.*]],
; MIPS32: lw [[T7:.*]],
; MIPS32: lw [[T8:.*]],
; MIPS32: lw [[T9:.*]],
; MIPS32: lw [[T10:.*]],
; MIPS32: lw [[T11:.*]],
; MIPS32: lw [[T12:.*]],
; MIPS32: lw [[T13:.*]],
; MIPS32: move [[T14:.*]],zero
; MIPS32: move [[T15:.*]],zero
; MIPS32: move [[T5]],zero
; MIPS32: move [[T4]],zero
; MIPS32: move [[T3]],a0
; MIPS32: andi [[T3]],[[T3]],0xffff
; MIPS32: andi [[T3]],[[T3]],0x1
; MIPS32: move [[T2]],[[T6]]
; MIPS32: andi [[T2]],[[T2]],0xffff
; MIPS32: andi [[T2]],[[T2]],0x1
; MIPS32: move [[T1]],[[T10]]
; MIPS32: andi [[T1]],[[T1]],0xffff
; MIPS32: andi [[T1]],[[T1]],0x1
; MIPS32: movn [[T1]],[[T2]],[[T3]]
; MIPS32: andi [[T1]],[[T1]],0xffff
; MIPS32: srl [[T14]],[[T14]],0x10
; MIPS32: sll [[T14]],[[T14]],0x10
; MIPS32: or [[T1]],[[T1]],[[T14]]
; MIPS32: srl [[T16:.*]],a0,0x10
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: srl [[T6]],[[T6]],0x10
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: srl [[T10]],[[T10]],0x10
; MIPS32: andi [[T10]],[[T10]],0x1
; MIPS32: movn [[T10]],[[T6]],[[T16]]
; MIPS32: sll [[T10]],[[T10]],0x10
; MIPS32: sll [[T1]],[[T1]],0x10
; MIPS32: srl [[T1]],[[T1]],0x10
; MIPS32: or [[T10]],[[T10]],[[T1]]
; MIPS32: move [[T6]],a1
; MIPS32: andi [[T6]],[[T6]],0xffff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T16]],[[T7]]
; MIPS32: andi [[T16]],[[T16]],0xffff
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: move [[T14]],[[T11]]
; MIPS32: andi [[T14]],[[T14]],0xffff
; MIPS32: andi [[T14]],[[T14]],0x1
; MIPS32: movn [[T14]],[[T16]],[[T6]]
; MIPS32: andi [[T14]],[[T14]],0xffff
; MIPS32: srl [[T15]],[[T15]],0x10
; MIPS32: sll [[T15]],[[T15]],0x10
; MIPS32: or [[T14]],[[T14]],[[T15]]
; MIPS32: srl [[T17:.*]],a1,0x10
; MIPS32: andi [[T17]],[[T17]],0x1
; MIPS32: srl [[T7]],[[T7]],0x10
; MIPS32: andi [[T7]],[[T7]],0x1
; MIPS32: srl [[T11]],[[T11]],0x10
; MIPS32: andi [[T11]],[[T11]],0x1
; MIPS32: movn [[T11]],[[T7]],[[T17]]
; MIPS32: sll [[T11]],[[T11]],0x10
; MIPS32: sll [[T14]],[[T14]],0x10
; MIPS32: srl [[T14]],[[T14]],0x10
; MIPS32: or [[T11]],[[T11]],[[T14]]
; MIPS32: move [[T6]],a2
; MIPS32: andi [[T6]],[[T6]],0xffff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T8]]
; MIPS32: andi [[T7]],[[T7]],0xffff
; MIPS32: andi [[T7]],[[T7]],0x1
; MIPS32: move [[T16]],[[T12]]
; MIPS32: andi [[T16]],[[T16]],0xffff
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xffff
; MIPS32: srl [[T5]],[[T5]],0x10
; MIPS32: sll [[T5]],[[T5]],0x10
; MIPS32: or [[T16]],[[T16]],[[T5]]
; MIPS32: srl [[T18:.*]],a2,0x10
; MIPS32: andi [[T18]],[[T18]],0x1
; MIPS32: srl [[T8]],[[T8]],0x10
; MIPS32: andi [[T8]],[[T8]],0x1
; MIPS32: srl [[T12]],[[T12]],0x10
; MIPS32: andi [[T12]],[[T12]],0x1
; MIPS32: movn [[T12]],[[T8]],[[T18]]
; MIPS32: sll [[T12]],[[T12]],0x10
; MIPS32: sll [[T16]],[[T16]],0x10
; MIPS32: srl [[T16]],[[T16]],0x10
; MIPS32: or [[T12]],[[T12]],[[T16]]
; MIPS32: move [[T6]],a3
; MIPS32: andi [[T6]],[[T6]],0xffff
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: move [[T7]],[[T9]]
; MIPS32: andi [[T7]],[[T7]],0xffff
; MIPS32: andi [[T7]],[[T7]],0x1
; MIPS32: move [[T16]],[[T13]]
; MIPS32: andi [[T16]],[[T16]],0xffff
; MIPS32: andi [[T16]],[[T16]],0x1
; MIPS32: movn [[T16]],[[T7]],[[T6]]
; MIPS32: andi [[T16]],[[T16]],0xffff
; MIPS32: srl [[T4]],[[T4]],0x10
; MIPS32: sll [[T4]],[[T4]],0x10
; MIPS32: or [[T16]],[[T16]],[[T4]]
; MIPS32: srl [[T19:.*]],a3,0x10
; MIPS32: andi [[T19]],[[T19]],0x1
; MIPS32: srl [[T9]],[[T9]],0x10
; MIPS32: andi [[T9]],[[T9]],0x1
; MIPS32: srl [[T13]],[[T13]],0x10
; MIPS32: andi [[T13]],[[T13]],0x1
; MIPS32: movn [[T13]],[[T9]],[[T19]]
; MIPS32: sll [[T13]],[[T13]],0x10
; MIPS32: sll [[T16]],[[T16]],0x10
; MIPS32: srl [[T16]],[[T16]],0x10
; MIPS32: or [[T13]],[[T13]],[[T16]]
; MIPS32: move v0,[[T10]]
; MIPS32: move v1,[[T11]]
; MIPS32: move a0,[[T12]]
; MIPS32: move a1,[[T13]]
; MIPS32: lw [[T5]],
; MIPS32: lw [[T4]],
; MIPS32: lw [[T3]],
; MIPS32: lw [[T2]],
; MIPS32: lw [[T1]],
; MIPS32: addiu [[T0]],sp,32
}

define internal <4 x i32> @test_select_v4i32(<4 x i1> %cond, <4 x i32> %arg1,
                                             <4 x i32> %arg2) {
entry:
  %res = select <4 x i1> %cond, <4 x i32> %arg1, <4 x i32> %arg2
  ret <4 x i32> %res
; CHECK-LABEL: test_select_v4i32
; CHECK: pand
; CHECK: pandn
; CHECK: por

; SSE41-LABEL: test_select_v4i32
; SSE41: pslld xmm0,0x1f
; SSE41: blendvps xmm{{[0-7]}},{{xmm[0-7]|XMMWORD}}

; MIPS32-LABEL: test_select_v4i32
; MIPS32: lw [[T0:.*]],
; MIPS32: lw [[T1:.*]],
; MIPS32: lw [[T2:.*]],
; MIPS32: lw [[T3:.*]],
; MIPS32: lw [[T4:.*]],
; MIPS32: lw [[T5:.*]],
; MIPS32: lw [[T6:.*]],
; MIPS32: lw [[T7:.*]],
; MIPS32: andi [[T8:.*]],a0,0x1
; MIPS32: movn [[T4]],[[T0]],[[T8]]
; MIPS32: andi [[T9:.*]],a1,0x1
; MIPS32: movn [[T5]],[[T1]],[[T9]]
; MIPS32: andi [[T10:.*]],a2,0x1
; MIPS32: movn [[T6]],[[T2]],[[T10]]
; MIPS32: andi [[T11:.*]],a3,0x1
; MIPS32: movn [[T7]],[[T3]],[[T11]]
; MIPS32: move v0,[[T4]]
; MIPS32: move v1,[[T5]]
; MIPS32: move a0,[[T6]]
; MIPS32: move a1,[[T7]]
}

define internal <4 x float> @test_select_v4f32(
    <4 x i1> %cond, <4 x float> %arg1, <4 x float> %arg2) {
entry:
  %res = select <4 x i1> %cond, <4 x float> %arg1, <4 x float> %arg2
  ret <4 x float> %res
; CHECK-LABEL: test_select_v4f32
; CHECK: pand
; CHECK: pandn
; CHECK: por

; SSE41-LABEL: test_select_v4f32
; SSE41: pslld xmm0,0x1f
; SSE41: blendvps xmm{{[0-7]}},{{xmm[0-7]|XMMWORD}}

; MIPS32-LABEL: test_select_v4f32
; MIPS32: lw [[T0:.*]],
; MIPS32: lw [[T1:.*]],
; MIPS32: lw [[T2:.*]],
; MIPS32: lw [[T3:.*]],
; MIPS32: lw [[T4:.*]],
; MIPS32: lw [[T5:.*]],
; MIPS32: lw [[T6:.*]],
; MIPS32: lw [[T7:.*]],
; MIPS32: lw [[T8:.*]],
; MIPS32: lw [[T9:.*]],
; MIPS32: andi [[T10:.*]],a2,0x1
; MIPS32: mtc1 [[T2]],$f0
; MIPS32: mtc1 [[T6]],$f1
; MIPS32: movn.s [[T11:.*]],$f0,[[T10]]
; MIPS32: mfc1 [[T2]],[[T11]]
; MIPS32: andi [[T12:.*]],a3,0x1
; MIPS32: mtc1 [[T3]],$f0
; MIPS32: mtc1 [[T7]],[[T11]]
; MIPS32: movn.s [[T11]],$f0,[[T12]]
; MIPS32: mfc1 [[T3]],[[T11]]
; MIPS32: andi [[T0]],[[T0]],0x1
; MIPS32: mtc1 [[T4]],$f0
; MIPS32: mtc1 [[T8]],[[T11]]
; MIPS32: movn.s [[T11]],$f0,[[T0]]
; MIPS32: mfc1 [[T4]],[[T11]]
; MIPS32: andi [[T1]],[[T1]],0x1
; MIPS32: mtc1 [[T5]],$f0
; MIPS32: mtc1 [[T9]],[[T11]]
; MIPS32: movn.s [[T11]],$f0,[[T1]]
; MIPS32: mfc1 [[T10]],[[T11]]
; MIPS32: move [[T12]],a0
; MIPS32: sw [[T2]],0(a3)
; MIPS32: sw v1,4(a3)
; MIPS32: sw a1,8(a3)
; MIPS32: sw [[T10]],12(a3)
; MIPS32: move v0,a0
}

define internal <4 x i1> @test_select_v4i1(<4 x i1> %cond, <4 x i1> %arg1,
                                           <4 x i1> %arg2) {
entry:
  %res = select <4 x i1> %cond, <4 x i1> %arg1, <4 x i1> %arg2
  ret <4 x i1> %res
; CHECK-LABEL: test_select_v4i1
; CHECK: pand
; CHECK: pandn
; CHECK: por

; SSE41-LABEL: test_select_v4i1
; SSE41: pslld xmm0,0x1f
; SSE41: blendvps xmm{{[0-7]}},{{xmm[0-7]|XMMWORD}}

; MIPS32-LABEL: test_select_v4i1
; MIPS32: lw [[T0:.*]],
; MIPS32: lw [[T1:.*]],
; MIPS32: lw [[T2:.*]],
; MIPS32: lw [[T3:.*]],
; MIPS32: lw [[T4:.*]],
; MIPS32: lw [[T5:.*]],
; MIPS32: lw [[T6:.*]],
; MIPS32: lw [[T7:.*]],
; MIPS32: andi [[T8:.*]],a0,0x1
; MIPS32: andi [[T0]],[[T0]],0x1
; MIPS32: andi [[T4]],[[T4]],0x1
; MIPS32: movn [[T4]],[[T0]],[[T8]]
; MIPS32: andi [[T9:.*]],a1,0x1
; MIPS32: andi [[T1]],[[T1]],0x1
; MIPS32: andi [[T5]],[[T5]],0x1
; MIPS32: movn [[T5]],[[T1]],[[T9]]
; MIPS32: andi [[T10:.*]],a2,0x1
; MIPS32: andi [[T2]],[[T2]],0x1
; MIPS32: andi [[T6]],[[T6]],0x1
; MIPS32: movn [[T6]],[[T2]],[[T10]]
; MIPS32: andi [[T11:.*]],a3,0x1
; MIPS32: andi [[T3]],[[T3]],0x1
; MIPS32: andi [[T7]],[[T7]],0x1
; MIPS32: movn [[T7]],[[T3]],[[T11]]
; MIPS32: move v0,[[T4]]
; MIPS32: move v1,[[T5]]
; MIPS32: move a0,[[T6]]
; MIPS32: move a1,[[T7]]
}
