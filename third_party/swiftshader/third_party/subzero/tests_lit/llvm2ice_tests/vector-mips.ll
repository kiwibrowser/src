; This test checks support for vector type in MIPS.

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target mips32\
; RUN:   -i %s --args -O2 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

define internal i32 @test_0(<4 x i32> %a) #0 {
entry:
  %vecext = extractelement <4 x i32> %a, i32 0
  ret i32 %vecext
}
; MIPS32-LABEL: test_0
; MIPS32: move v0,a0

define internal i32 @test_1(<4 x i32> %a) #0 {
entry:
  %vecext = extractelement <4 x i32> %a, i32 1
  ret i32 %vecext
}
; MIPS32-LABEL: test_1
; MIPS32: move v0,a1

define internal i32 @test_2(<4 x i32> %a) #0 {
entry:
  %vecext = extractelement <4 x i32> %a, i32 2
  ret i32 %vecext
}
; MIPS32-LABEL: test_2
; MIPS32: move v0,a2

define internal i32 @test_3(<4 x i32> %a) #0 {
entry:
  %vecext = extractelement <4 x i32> %a, i32 3
  ret i32 %vecext
}
; MIPS32-LABEL: test_3
; MIPS32: move v0,a3

define internal float @test_4(<4 x float> %a) #0 {
entry:
  %vecext = extractelement <4 x float> %a, i32 1
  ret float %vecext
}
; MIPS32-LABEL: test_4
; MIPS32: mtc1 a1,$f0

define internal float @test_5(<4 x float> %a) #0 {
entry:
  %vecext = extractelement <4 x float> %a, i32 2
  ret float %vecext
}
; MIPS32-LABEL: test_5
; MIPS32: mtc1 a2,$f0

define internal i32 @test_6(<16 x i8> %a) #0 {
entry:
  %vecext = extractelement <16 x i8> %a, i32 0
  %conv = sext i8 %vecext to i32
  ret i32 %conv
}
; MIPS32-LABEL: test_6
; MIPS32: andi a0,a0,0xff
; MIPS32: sll a0,a0,0x18
; MIPS32: sra a0,a0,0x18
; MIPS32: move v0,a0

define internal i32 @test_7(<16 x i8> %a) #0 {
entry:
  %vecext = extractelement <16 x i8> %a, i32 15
  %conv = sext i8 %vecext to i32
  ret i32 %conv
}
; MIPS32-LABEL: test_7
; MIPS32: srl a3,a3,0x18
; MIPS32: sll a3,a3,0x18
; MIPS32: sra a3,a3,0x18
; MIPS32: move v0,a3

define internal i32 @test_8(<8 x i16> %a) #0 {
entry:
  %vecext = extractelement <8 x i16> %a, i32 0
  %conv = sext i16 %vecext to i32
  ret i32 %conv
}
; MIPS32-LABEL: test_8
; MIPS32: andi a0,a0,0xffff
; MIPS32: sll a0,a0,0x10
; MIPS32: sra a0,a0,0x10
; MIPS32: move v0,a0

define internal i32 @test_9(<8 x i16> %a) #0 {
entry:
  %vecext = extractelement <8 x i16> %a, i32 7
  %conv = sext i16 %vecext to i32
  ret i32 %conv
}
; MIPS32-LABEL: test_9
; MIPS32: srl a3,a3,0x10
; MIPS32: sll a3,a3,0x10
; MIPS32: sra a3,a3,0x10
; MIPS32: move v0,a3

define internal i32 @test_10(<4 x i1> %a) #0 {
entry:
  %vecext = extractelement <4 x i1> %a, i32 0
  %conv = sext i1 %vecext to i32
  ret i32 %conv
}
; MIPS32-LABEL: test_10
; MIPS32: andi a0,a0,0x1
; MIPS32: sll a0,a0,0x1f
; MIPS32: sra a0,a0,0x1f
; MIPS32: move v0,a0

define internal i32 @test_11(<4 x i1> %a) #0 {
entry:
  %vecext = extractelement <4 x i1> %a, i32 2
  %conv = sext i1 %vecext to i32
  ret i32 %conv
}
; MIPS32-LABEL: test_11
; MIPS32: andi a2,a2,0x1
; MIPS32: sll a2,a2,0x1f
; MIPS32: sra a2,a2,0x1f
; MIPS32: move v0,a2

define internal i32 @test_12(<8 x i1> %a) #0 {
entry:
  %vecext = extractelement <8 x i1> %a, i32 0
  %conv = sext i1 %vecext to i32
  ret i32 %conv
}
; MIPS32-LABEL: test_12
; MIPS32: andi a0,a0,0xffff
; MIPS32: andi a0,a0,0x1
; MIPS32: sll a0,a0,0x1f
; MIPS32: sra a0,a0,0x1f
; MIPS32: move v0,a0

define internal i32 @test_13(<8 x i1> %a) #0 {
entry:
  %vecext = extractelement <8 x i1> %a, i32 7
  %conv = sext i1 %vecext to i32
  ret i32 %conv
}
; MIPS32-LABEL: test_13
; MIPS32: srl a3,a3,0x10
; MIPS32: andi a3,a3,0x1
; MIPS32: sll a3,a3,0x1f
; MIPS32: sra a3,a3,0x1f
; MIPS32: move v0,a3

define internal i32 @test_14(<16 x i1> %a) #0 {
entry:
  %vecext = extractelement <16 x i1> %a, i32 0
  %conv = sext i1 %vecext to i32
  ret i32 %conv
}
; MIPS32-LABEL: test_14
; MIPS32: andi a0,a0,0xff
; MIPS32: andi a0,a0,0x1
; MIPS32: sll a0,a0,0x1f
; MIPS32: sra a0,a0,0x1f
; MIPS32: move v0,a0

define internal i32 @test_15(<16 x i1> %a) #0 {
entry:
  %vecext = extractelement <16 x i1> %a, i32 15
  %conv = sext i1 %vecext to i32
  ret i32 %conv
}
; MIPS32-LABEL: test_15
; MIPS32: srl a3,a3,0x18
; MIPS32: andi a3,a3,0x1
; MIPS32: sll a3,a3,0x1f
; MIPS32: sra a3,a3,0x1f
; MIPS32: move v0,a3

define internal i32 @test_16(i32 %i, <4 x i32> %a) #0 {
entry:
  %vecext = extractelement <4 x i32> %a, i32 0
  %add = add nsw i32 %vecext, %i
  ret i32 %add
}
; MIPS32-LABEL: test_16
; MIPS32: addu a2,a2,a0
; MIPS32: move v0,a2

define internal i32 @test_17(i32 %i, <4 x i32> %a) #0 {
entry:
  %vecext = extractelement <4 x i32> %a, i32 3
  %add = add nsw i32 %vecext, %i
  ret i32 %add
}
; MIPS32-LABEL: test_17
; MIPS32: lw v0,{{.*}}(sp)
; MIPS32: addu v0,v0,a0

define internal float @test_18(float %f, <4 x float> %a) #0 {
entry:
  %vecext = extractelement <4 x float> %a, i32 0
  %add = fadd float %vecext, %f
  ret float %add
}
; MIPS32-LABEL: test_18
; MIPS32: mtc1 a2,$f0
; MIPS32: add.s $f0,$f0,$f12

define internal float @test_19(float %f, <4 x float> %a) #0 {
entry:
  %vecext = extractelement <4 x float> %a, i32 3
  %add = fadd float %vecext, %f
  ret float %add
}
; MIPS32-LABEL: test_19
; MIPS32: lw v0,{{.*}}(sp)
; MIPS32: mtc1 v0,$f0
; MIPS32: add.s $f0,$f0,$f12

define internal <4 x float> @test_20(i32 %addr_i, <4 x float> %addend) {
entry:
  %addr = inttoptr i32 %addr_i to <4 x float>*
  %loaded = load <4 x float>, <4 x float>* %addr, align 4
  %result = fadd <4 x float> %addend, %loaded
  ret <4 x float> %result
}
; MIPS32-LABEL: test_20
; MIPS32: add.s
; MIPS32: add.s
; MIPS32: add.s
; MIPS32: add.s

define internal <4 x i32> @test_21(i32 %addr_i, <4 x i32> %addend) {
entry:
  %addr = inttoptr i32 %addr_i to <4 x i32>*
  %loaded = load <4 x i32>, <4 x i32>* %addr, align 4
  %result = add <4 x i32> %addend, %loaded
  ret <4 x i32> %result
}
; MIPS32-LABEL: test_21
; MIPS32: add
; MIPS32: add
; MIPS32: add
; MIPS32: add
