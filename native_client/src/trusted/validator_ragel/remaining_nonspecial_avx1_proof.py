# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for adding all remaining non-special avx1 ops."""

import proof_tools
from proof_tools import MnemonicOp
from proof_tools import OpsProd


def Instructions(bitness):
  """Returns all new instructions to be added with this change."""
  imm = proof_tools.ImmOp()
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  xmm = proof_tools.AllXMMOperands(bitness)
  ymm = proof_tools.AllYMMOperands(bitness)
  reg32_write = proof_tools.GprOperands(bitness, operand_size=32,
                                        is_write_for_64_bit=True)
  reg32 = proof_tools.GprOperands(bitness, operand_size=32,
                                  is_write_for_64_bit=False)

  vblendpd = (OpsProd(MnemonicOp('vblendpd'), imm, mem|xmm, xmm, xmm) |
              OpsProd(MnemonicOp('vblendpd'), imm, mem|ymm, ymm, ymm))
  vblendps = (OpsProd(MnemonicOp('vblendps'), imm, mem|xmm, xmm, xmm) |
              OpsProd(MnemonicOp('vblendps'), imm, mem|ymm, ymm, ymm))
  vblendvpd = (OpsProd(MnemonicOp('vblendvpd'), xmm, mem|xmm, xmm, xmm) |
               OpsProd(MnemonicOp('vblendvpd'), ymm, mem|ymm, ymm, ymm))
  vblendvps = (OpsProd(MnemonicOp('vblendvps'), xmm, mem|xmm, xmm, xmm) |
               OpsProd(MnemonicOp('vblendvps'), ymm, mem|ymm, ymm, ymm))

  vcomisd = OpsProd(MnemonicOp('vcomisd'), mem|xmm, xmm)
  vcomiss = OpsProd(MnemonicOp('vcomiss'), mem|xmm, xmm)

  vcvtdq2pd = (OpsProd(MnemonicOp('vcvtdq2pd'), mem|xmm, xmm) |
               OpsProd(MnemonicOp('vcvtdq2pd'), mem|xmm, ymm))
  vcvtdq2ps = (OpsProd(MnemonicOp('vcvtdq2ps'), mem|xmm, xmm) |
               OpsProd(MnemonicOp('vcvtdq2ps'), mem|ymm, ymm))

  vcvtps2dq = (OpsProd(MnemonicOp('vcvtps2dq'), mem|xmm, xmm) |
               OpsProd(MnemonicOp('vcvtps2dq'), mem|ymm, ymm))
  vcvtps2pd = (OpsProd(MnemonicOp('vcvtps2pd'), mem|xmm, xmm) |
               OpsProd(MnemonicOp('vcvtps2pd'), mem|xmm, ymm))

  vdppd = OpsProd(MnemonicOp('vdppd'), imm, xmm|mem, xmm, xmm)
  vdpps = (OpsProd(MnemonicOp('vdpps'), imm, xmm|mem, xmm, xmm) |
           OpsProd(MnemonicOp('vdpps'), imm, ymm|mem, ymm, ymm))

  vextractf128 = OpsProd(MnemonicOp('vextractf128'), imm, ymm, xmm|mem)
  vinsertps = OpsProd(MnemonicOp('vinsertps'), imm, xmm|mem, xmm, xmm)

  vldmxcsr = OpsProd(MnemonicOp('vldmxcsr'), mem)
  vstmxcsr = OpsProd(MnemonicOp('vstmxcsr'), mem)

  vldqqu = OpsProd(MnemonicOp('vlddqu'), mem, xmm|ymm)

  vmaskmovpd = (OpsProd(MnemonicOp('vmaskmovpd'), mem, xmm, xmm) |
                OpsProd(MnemonicOp('vmaskmovpd'), xmm, xmm, mem) |
                OpsProd(MnemonicOp('vmaskmovpd'), ymm, ymm, mem) |
                OpsProd(MnemonicOp('vmaskmovpd'), mem, ymm, ymm))
  vmaskmovps = (OpsProd(MnemonicOp('vmaskmovps'), mem, xmm, xmm) |
                OpsProd(MnemonicOp('vmaskmovps'), xmm, xmm, mem) |
                OpsProd(MnemonicOp('vmaskmovps'), ymm, ymm, mem) |
                OpsProd(MnemonicOp('vmaskmovps'), mem, ymm, ymm))

  vmovmskps = OpsProd(MnemonicOp('vmovmskps'), xmm|ymm, reg32_write)
  vmovmskpd = OpsProd(MnemonicOp('vmovmskpd'), xmm|ymm, reg32_write)

  vmovntdqa = OpsProd(MnemonicOp('vmovntdqa'), mem, xmm)
  vmovntdq = OpsProd(MnemonicOp('vmovntdq'), xmm|ymm, mem)
  vmovntpd = OpsProd(MnemonicOp('vmovntpd'), xmm|ymm, mem)
  vmovntps = OpsProd(MnemonicOp('vmovntps'), xmm|ymm, mem)

  vmpsadbw = OpsProd(MnemonicOp('vmpsadbw'), imm, mem|xmm, xmm, xmm)

  vpabsb = OpsProd(MnemonicOp('vpabsb'), mem|xmm, xmm)
  vpabsw = OpsProd(MnemonicOp('vpabsw'), mem|xmm, xmm)
  vpabsd = OpsProd(MnemonicOp('vpabsd'), mem|xmm, xmm)

  vpalignr = OpsProd(MnemonicOp('vpalignr'), imm, xmm|mem, xmm, xmm)

  vpblendvb = OpsProd(MnemonicOp('vpblendvb'), xmm, xmm|mem, xmm, xmm)
  vpblendw = OpsProd(MnemonicOp('vpblendw'), imm, xmm|mem, xmm, xmm)

  vpcmpestri = OpsProd(MnemonicOp('vpcmpestri'), imm, mem|xmm, xmm)
  vpcmpestrm = OpsProd(MnemonicOp('vpcmpestrm'), imm, mem|xmm, xmm)
  vpcmpistri = OpsProd(MnemonicOp('vpcmpistri'), imm, mem|xmm, xmm)
  vpcmpistrm = OpsProd(MnemonicOp('vpcmpistrm'), imm, mem|xmm, xmm)

  vperm2f128 = OpsProd(MnemonicOp('vperm2f128'), imm, ymm|mem, ymm, ymm)
  vpermilpd = (OpsProd(MnemonicOp('vpermilpd'), imm, xmm|mem, xmm) |
               OpsProd(MnemonicOp('vpermilpd'), imm, ymm|mem, ymm))
  vpermilps = (OpsProd(MnemonicOp('vpermilps'), imm, xmm|mem, xmm) |
               OpsProd(MnemonicOp('vpermilps'), imm, ymm|mem, ymm))

  vpextrb = OpsProd(MnemonicOp('vpextrb'), imm, xmm, reg32_write|mem)
  vpextrd = OpsProd(MnemonicOp('vpextrd'), imm, xmm, reg32_write|mem)
  vpextrw = OpsProd(MnemonicOp('vpextrw'), imm, xmm, reg32_write|mem)

  vphminposuw = OpsProd(MnemonicOp('vphminposuw'), mem|xmm, xmm)

  vpmovmskb = OpsProd(MnemonicOp('vpmovmskb'), xmm, reg32_write)

  vpmovsxbw = OpsProd(MnemonicOp('vpmovsxbw'), xmm|mem, xmm)
  vpmovsxbd = OpsProd(MnemonicOp('vpmovsxbd'), xmm|mem, xmm)
  vpmovsxbq = OpsProd(MnemonicOp('vpmovsxbq'), xmm|mem, xmm)
  vpmovsxdq = OpsProd(MnemonicOp('vpmovsxdq'), xmm|mem, xmm)
  vpmovsxwd = OpsProd(MnemonicOp('vpmovsxwd'), xmm|mem, xmm)
  vpmovsxwq = OpsProd(MnemonicOp('vpmovsxwq'), xmm|mem, xmm)

  vpmovzxbw = OpsProd(MnemonicOp('vpmovzxbw'), xmm|mem, xmm)
  vpmovzxbd = OpsProd(MnemonicOp('vpmovzxbd'), xmm|mem, xmm)
  vpmovzxbq = OpsProd(MnemonicOp('vpmovzxbq'), xmm|mem, xmm)
  vpmovzxdq = OpsProd(MnemonicOp('vpmovzxdq'), xmm|mem, xmm)
  vpmovzxwd = OpsProd(MnemonicOp('vpmovzxwd'), xmm|mem, xmm)
  vpmovzxwq = OpsProd(MnemonicOp('vpmovzxwq'), xmm|mem, xmm)

  vpshufd = OpsProd(MnemonicOp('vpshufd'), imm, mem|xmm, xmm)
  vpshufhw = OpsProd(MnemonicOp('vpshufhw'), imm, mem|xmm, xmm)
  vpshuflw = OpsProd(MnemonicOp('vpshuflw'), imm, mem|xmm, xmm)

  # These have non immediate versions where the source operand is xmm/mem
  # instead of the immediate. But they have already been enabled.
  vpslld = OpsProd(MnemonicOp('vpslld'), imm, xmm, xmm)
  vpslldq = OpsProd(MnemonicOp('vpslldq'), imm, xmm, xmm)
  vpsllq = OpsProd(MnemonicOp('vpsllq'), imm, xmm, xmm)
  vpsllw = OpsProd(MnemonicOp('vpsllw'), imm, xmm, xmm)
  vpsrad = OpsProd(MnemonicOp('vpsrad'), imm, xmm, xmm)
  vpsraw = OpsProd(MnemonicOp('vpsraw'), imm, xmm, xmm)
  vpsrld = OpsProd(MnemonicOp('vpsrld'), imm, xmm, xmm)
  vpsrldq = OpsProd(MnemonicOp('vpsrldq'), imm, xmm, xmm)
  vpsrlq = OpsProd(MnemonicOp('vpsrlq'), imm, xmm, xmm)
  vpsrlw = OpsProd(MnemonicOp('vpsrlw'), imm, xmm, xmm)

  vptest = (OpsProd(MnemonicOp('vptest'), mem|xmm, xmm) |
            OpsProd(MnemonicOp('vptest'), mem|ymm, ymm))

  vrcpps = (OpsProd(MnemonicOp('vrcpps'), mem|xmm, xmm) |
            OpsProd(MnemonicOp('vrcpps'), mem|ymm, ymm))

  vroundpd = (OpsProd(MnemonicOp('vroundpd'), imm, mem|xmm, xmm) |
              OpsProd(MnemonicOp('vroundpd'), imm, mem|ymm, ymm))
  vroundps = (OpsProd(MnemonicOp('vroundps'), imm, mem|xmm, xmm) |
              OpsProd(MnemonicOp('vroundps'), imm, mem|ymm, ymm))
  vroundsd = OpsProd(MnemonicOp('vroundsd'), imm, mem|xmm, xmm, xmm)
  vroundss = OpsProd(MnemonicOp('vroundss'), imm, mem|xmm, xmm, xmm)

  vrsqrtps = (OpsProd(MnemonicOp('vrsqrtps'), xmm|mem, xmm) |
              OpsProd(MnemonicOp('vrsqrtps'), ymm|mem, ymm))

  vshufpd = (OpsProd(MnemonicOp('vshufpd'), imm, mem|xmm, xmm, xmm) |
             OpsProd(MnemonicOp('vshufpd'), imm, mem|ymm, ymm, ymm))
  vshufps = (OpsProd(MnemonicOp('vshufps'), imm, mem|xmm, xmm, xmm) |
             OpsProd(MnemonicOp('vshufps'), imm, mem|ymm, ymm, ymm))

  vsqrtpd = (OpsProd(MnemonicOp('vsqrtpd'), mem|xmm, xmm) |
             OpsProd(MnemonicOp('vsqrtpd'), mem|ymm, ymm))
  vsqrtps = (OpsProd(MnemonicOp('vsqrtps'), mem|xmm, xmm) |
             OpsProd(MnemonicOp('vsqrtps'), mem|ymm, ymm))

  vtestpd = (OpsProd(MnemonicOp('vtestpd'), mem|xmm, xmm) |
             OpsProd(MnemonicOp('vtestpd'), mem|ymm, ymm))
  vtestps = (OpsProd(MnemonicOp('vtestps'), mem|xmm, xmm) |
             OpsProd(MnemonicOp('vtestps'), mem|ymm, ymm))

  vucomisd = OpsProd(MnemonicOp('vucomisd'), mem|xmm, xmm)
  vucomiss = OpsProd(MnemonicOp('vucomiss'), mem|xmm, xmm)

  return (
      vblendpd | vblendps | vblendvpd | vblendvps |
      vcomisd | vcomiss |
      vcvtdq2pd | vcvtdq2ps | vcvtps2dq | vcvtps2pd |
      vdppd | vdpps |
      vextractf128 | vinsertps |
      vldmxcsr | vstmxcsr |
      vldqqu |
      vmaskmovpd | vmaskmovps |
      vmovmskpd | vmovmskps |
      vmovntdqa | vmovntdq | vmovntpd | vmovntps |
      vmpsadbw |
      vpabsb | vpabsd | vpabsw |
      vpalignr |
      vpblendvb | vpblendw |
      vpcmpestri | vpcmpestrm | vpcmpistri | vpcmpistrm |
      vperm2f128 | vpermilpd | vpermilps |
      vpextrb | vpextrd | vpextrw |
      vphminposuw |
      vpmovmskb |
      vpmovsxbw | vpmovsxbd | vpmovsxbq | vpmovsxdq | vpmovsxwd | vpmovsxwq |
      vpmovzxbw | vpmovzxbd | vpmovzxbq | vpmovzxdq | vpmovzxwd | vpmovzxwq |
      vpshufd | vpshufhw | vpshuflw |
      vpslld | vpslldq | vpsllq | vpsllw |
      vpsrad | vpsraw | vpsrld | vpsrldq | vpsrlq  | vpsrlw |
      vptest |
      vrcpps |
      vroundpd | vroundps |
      vroundsd | vroundss |
      vrsqrtps |
      vshufpd | vshufps |
      vsqrtpd | vsqrtps |
      vtestpd | vtestps |
      vucomisd | vucomiss)


def Validate(trie_diffs, bitness):
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=Instructions(bitness),
      expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
