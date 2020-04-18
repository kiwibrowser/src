# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for remaining avx2 upgrade of avx1 instructions."""

import proof_tools
from proof_tools import MnemonicOp
from proof_tools import OpsProd

def Instructions(bitness):
  """Returns all remaining AVX2 promotions of avx1 instructions."""
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  ymm = proof_tools.AllYMMOperands(bitness)
  xmm = proof_tools.AllXMMOperands(bitness)
  imm = proof_tools.ImmOp()
  reg32_write = proof_tools.GprOperands(bitness, operand_size=32,
                                        is_write_for_64_bit=True)

  # 0F EX Group
  vpavgb = OpsProd(MnemonicOp('vpavgb'), mem|ymm, ymm, ymm)
  vpsraw = OpsProd(MnemonicOp('vpsraw'), mem|xmm|imm, ymm, ymm)
  vpsrad = OpsProd(MnemonicOp('vpsrad'), mem|xmm|imm, ymm, ymm)
  vpavgw = OpsProd(MnemonicOp('vpavgw'), mem|ymm, ymm, ymm)
  vpmulhuw = OpsProd(MnemonicOp('vpmulhuw'), mem|ymm, ymm, ymm)
  vpmulhw = OpsProd(MnemonicOp('vpmulhw'), mem|ymm, ymm, ymm)
  vmovntdq = OpsProd(MnemonicOp('vmovntdq'), ymm, mem)
  vpsubsb = OpsProd(MnemonicOp('vpsubsb'), mem|ymm, ymm, ymm)
  vpsubsw = OpsProd(MnemonicOp('vpsubsw'), mem|ymm, ymm, ymm)
  vpminsw = OpsProd(MnemonicOp('vpminsw'), mem|ymm, ymm, ymm)
  vpor = OpsProd(MnemonicOp('vpor'), mem|ymm, ymm, ymm)
  vpaddsb = OpsProd(MnemonicOp('vpaddsb'), mem|ymm, ymm, ymm)
  vpaddsw = OpsProd(MnemonicOp('vpaddsw'), mem|ymm, ymm, ymm)
  vpmaxsw = OpsProd(MnemonicOp('vpmaxsw'), mem|ymm, ymm, ymm)
  vpxor = OpsProd(MnemonicOp('vpxor'), mem|ymm, ymm, ymm)

  # 0F FX Group
  vpsllw = OpsProd(MnemonicOp('vpsllw'), mem|xmm|imm, ymm, ymm)
  vpslld = OpsProd(MnemonicOp('vpslld'), mem|xmm|imm, ymm, ymm)
  vpsllq = OpsProd(MnemonicOp('vpsllq'), mem|xmm|imm, ymm, ymm)
  vpmuludq = OpsProd(MnemonicOp('vpmuludq'), mem|ymm, ymm, ymm)
  vpmaddwd = OpsProd(MnemonicOp('vpmaddwd'), mem|ymm, ymm, ymm)
  vpsadbw = OpsProd(MnemonicOp('vpsadbw'), mem|ymm, ymm, ymm)
  vpsubb = OpsProd(MnemonicOp('vpsubb'), mem|ymm, ymm, ymm)
  vpsubw = OpsProd(MnemonicOp('vpsubw'), mem|ymm, ymm, ymm)
  vpsubd = OpsProd(MnemonicOp('vpsubd'), mem|ymm, ymm, ymm)
  vpsubq = OpsProd(MnemonicOp('vpsubq'), mem|ymm, ymm, ymm)
  vpaddb = OpsProd(MnemonicOp('vpaddb'), mem|ymm, ymm, ymm)
  vpaddw = OpsProd(MnemonicOp('vpaddw'), mem|ymm, ymm, ymm)
  vpaddd = OpsProd(MnemonicOp('vpaddd'), mem|ymm, ymm, ymm)

  # SSE3 Group
  vphaddw = OpsProd(MnemonicOp('vphaddw'), mem|ymm, ymm, ymm)
  vphaddsw = OpsProd(MnemonicOp('vphaddsw'), mem|ymm, ymm, ymm)
  vphaddd = OpsProd(MnemonicOp('vphaddd'), mem|ymm, ymm, ymm)
  vphsubw = OpsProd(MnemonicOp('vphsubw'), mem|ymm, ymm, ymm)
  vphsubsw = OpsProd(MnemonicOp('vphsubsw'), mem|ymm, ymm, ymm)
  vphsubd = OpsProd(MnemonicOp('vphsubd'), mem|ymm, ymm, ymm)
  vpmaddubsw = OpsProd(MnemonicOp('vpmaddubsw'), mem|ymm, ymm, ymm)
  vpalignr = OpsProd(MnemonicOp('vpalignr'), imm, mem|ymm, ymm, ymm)
  vpshufb = OpsProd(MnemonicOp('vpshufb'), mem|ymm, ymm, ymm)
  vpmulhrsw = OpsProd(MnemonicOp('vpmulhrsw'), mem|ymm, ymm, ymm)
  vpsignb = OpsProd(MnemonicOp('vpsignb'), mem|ymm, ymm, ymm)
  vpsignw = OpsProd(MnemonicOp('vpsignw'), mem|ymm, ymm, ymm)
  vpsignd = OpsProd(MnemonicOp('vpsignd'), mem|ymm, ymm, ymm)
  vpabsb = OpsProd(MnemonicOp('vpabsb'), mem|ymm, ymm)
  vpabsw = OpsProd(MnemonicOp('vpabsw'), mem|ymm, ymm)
  vpabsd = OpsProd(MnemonicOp('vpabsd'), mem|ymm, ymm)
  vmovntdqa = OpsProd(MnemonicOp('vmovntdqa'), mem, ymm)
  vmpsadbw = OpsProd(MnemonicOp('vmpsadbw'), imm, mem|ymm, ymm, ymm)
  vpackusdw = OpsProd(MnemonicOp('vpackusdw'), mem|ymm, ymm, ymm)
  vpblendvb = OpsProd(MnemonicOp('vpblendvb'), ymm, ymm|mem, ymm, ymm)
  vpblendw = OpsProd(MnemonicOp('vpblendw'), imm, ymm|mem, ymm, ymm)
  vpcmpeqq = OpsProd(MnemonicOp('vpcmpeqq'), ymm|mem, ymm, ymm)
  vpmaxsb = OpsProd(MnemonicOp('vpmaxsb'), ymm|mem, ymm, ymm)
  vpmaxsd = OpsProd(MnemonicOp('vpmaxsd'), ymm|mem, ymm, ymm)
  vpmaxud = OpsProd(MnemonicOp('vpmaxud'), ymm|mem, ymm, ymm)
  vpmaxuw = OpsProd(MnemonicOp('vpmaxuw'), ymm|mem, ymm, ymm)
  vpminsb = OpsProd(MnemonicOp('vpminsb'), ymm|mem, ymm, ymm)
  vpminsd = OpsProd(MnemonicOp('vpminsd'), ymm|mem, ymm, ymm)
  vpminud = OpsProd(MnemonicOp('vpminud'), ymm|mem, ymm, ymm)
  vpminuw = OpsProd(MnemonicOp('vpminuw'), ymm|mem, ymm, ymm)
  vpmovsxbw = OpsProd(MnemonicOp('vpmovsxbw'), mem|xmm, ymm)
  vpmovsxbd = OpsProd(MnemonicOp('vpmovsxbd'), mem|xmm, ymm)
  vpmovsxbq = OpsProd(MnemonicOp('vpmovsxbq'), mem|xmm, ymm)
  vpmovsxwd = OpsProd(MnemonicOp('vpmovsxwd'), mem|xmm, ymm)
  vpmovsxwq = OpsProd(MnemonicOp('vpmovsxwq'), mem|xmm, ymm)
  vpmovsxdq = OpsProd(MnemonicOp('vpmovsxdq'), mem|xmm, ymm)
  vpmovzxbw = OpsProd(MnemonicOp('vpmovzxbw'), mem|xmm, ymm)
  vpmovzxbd = OpsProd(MnemonicOp('vpmovzxbd'), mem|xmm, ymm)
  vpmovzxbq = OpsProd(MnemonicOp('vpmovzxbq'), mem|xmm, ymm)
  vpmovzxwd = OpsProd(MnemonicOp('vpmovzxwd'), mem|xmm, ymm)
  vpmovzxwq = OpsProd(MnemonicOp('vpmovzxwq'), mem|xmm, ymm)
  vpmovzxdq = OpsProd(MnemonicOp('vpmovzxdq'), mem|xmm, ymm)
  vpmuldq = OpsProd(MnemonicOp('vpmuldq'), mem|ymm, ymm, ymm)
  vpmulld = OpsProd(MnemonicOp('vpmulld'), mem|ymm, ymm, ymm)
  # SSE 4.2 group.
  vpcmpgtq = OpsProd(MnemonicOp('vpcmpgtq'), mem|ymm, ymm, ymm)

  return (
    # 0F EX Group
    vpavgb | vpsraw | vpsrad | vpavgw | vpmulhuw | vpmulhw | vmovntdqa |
    vpsubsb | vpsubsw | vpminsw | vpor | vpaddsb | vpaddsw | vpmaxsw | vpxor |
    # 0F FX Group
    vpsllw | vpslld | vpsllq | vpmuludq | vpmaddwd | vpsadbw | vpsubb |
    vpsubw | vpsubd | vpsubq | vpaddb | vpaddw | vpaddd |
    # SSE3 Group
    vphaddw | vphaddsw | vphaddd | vphsubw | vphsubsw | vphsubd | vpmaddubsw |
    vpalignr | vpshufb | vpmulhrsw | vpsignb | vpsignw | vpsignd |
    vpabsb | vpabsw  | vpabsd | vmovntdqa | vmpsadbw | vpackusdw |
    vpblendvb | vpblendw | vpcmpeqq |
    vpmaxsb | vpmaxsd | vpmaxud | vpmaxuw | vpminsb | vpminud | vpminuw |
    vpmovsxbw | vpmovsxbd | vpmovsxbq | vpmovsxwd | vpmovsxwq | vpmovsxdq |
    vpmovzxbw | vpmovzxbd | vpmovzxbq | vpmovzxwd | vpmovzxwq | vpmovzxdq |
    vpmuldq | vpmulld |
    # SSE4.2 Group
    vpcmpgtq
  )


def Validate(trie_diffs, bitness):
  """Validates that all ymm variants of remaining avx2 promotions are added."""
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=Instructions(bitness),
      expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
