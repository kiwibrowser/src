# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for avx2 upgrade (allow ymm) for 0f 6x and 0f 7x groups"""

import proof_tools
from proof_tools import MnemonicOp
from proof_tools import OpsProd

def Instructions(bitness):
  """Returns avx2 promotions of 0f 6x and 0f 7x groups."""
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  ymm = proof_tools.AllYMMOperands(bitness)
  xmm = proof_tools.AllXMMOperands(bitness)
  imm = proof_tools.ImmOp()
  reg32_write = proof_tools.GprOperands(bitness, operand_size=32,
                                        is_write_for_64_bit=True)

  vpunpcklbw = OpsProd(MnemonicOp('vpunpcklbw'), mem|ymm, ymm, ymm)
  vpunpcklwd = OpsProd(MnemonicOp('vpunpcklwd'), mem|ymm, ymm, ymm)
  vpunpckldq = OpsProd(MnemonicOp('vpunpckldq'), mem|ymm, ymm, ymm)
  vpacksswb = OpsProd(MnemonicOp('vpacksswb'), mem|ymm, ymm, ymm)
  vpunpcklqdq = OpsProd(MnemonicOp('vpunpcklqdq'), mem|ymm, ymm, ymm)
  vpunpckhqdq = OpsProd(MnemonicOp('vpunpckhqdq'), mem|ymm, ymm, ymm)
  vpcmpgtb = OpsProd(MnemonicOp('vpcmpgtb'), mem|ymm, ymm, ymm)
  vpcmpgtw = OpsProd(MnemonicOp('vpcmpgtw'), mem|ymm, ymm, ymm)
  vpcmpgtd = OpsProd(MnemonicOp('vpcmpgtd'), mem|ymm, ymm, ymm)
  vpackuswb = OpsProd(MnemonicOp('vpackuswb'), mem|ymm, ymm, ymm)
  vpunpckhbw = OpsProd(MnemonicOp('vpunpckhbw'), mem|ymm, ymm, ymm)
  vpunpckhwd = OpsProd(MnemonicOp('vpunpckhwd'), mem|ymm, ymm, ymm)
  vpunpckhdq = OpsProd(MnemonicOp('vpunpckhdq'), mem|ymm, ymm, ymm)
  vpackssdw = OpsProd(MnemonicOp('vpackssdw'), mem|ymm, ymm, ymm)
  vpshufd = OpsProd(MnemonicOp('vpshufd'), imm, mem|ymm, ymm)
  vpshufhw = OpsProd(MnemonicOp('vpshufhw'), imm, mem|ymm, ymm)
  vpshuflw = OpsProd(MnemonicOp('vpshuflw'), imm, mem|ymm, ymm)
  vpcmpeqb = OpsProd(MnemonicOp('vpcmpeqb'), mem|ymm, ymm, ymm)
  vpcmpeqw = OpsProd(MnemonicOp('vpcmpeqw'), mem|ymm, ymm, ymm)
  vpcmpeqd = OpsProd(MnemonicOp('vpcmpeqd'), mem|ymm, ymm, ymm)
  vpsrlw = OpsProd(MnemonicOp('vpsrlw'), imm|mem|xmm, ymm, ymm)
  vpsrld = OpsProd(MnemonicOp('vpsrld'), imm|mem|xmm, ymm, ymm)
  vpsrlq = OpsProd(MnemonicOp('vpsrlq'), imm|mem|xmm, ymm, ymm)
  vpaddq = OpsProd(MnemonicOp('vpaddq'), mem|ymm, ymm, ymm)
  vpmullw = OpsProd(MnemonicOp('vpmullw'), mem|ymm, ymm, ymm)
  vpmovmskb = OpsProd(MnemonicOp('vpmovmskb'), ymm, reg32_write)
  vpsubusb = OpsProd(MnemonicOp('vpsubusb'), mem|ymm, ymm, ymm)
  vpsubusw = OpsProd(MnemonicOp('vpsubusw'), mem|ymm, ymm, ymm)
  vpminub = OpsProd(MnemonicOp('vpminub'), mem|ymm, ymm, ymm)
  vpand = OpsProd(MnemonicOp('vpand'), mem|ymm, ymm, ymm)
  vpaddusb = OpsProd(MnemonicOp('vpaddusb'), mem|ymm, ymm, ymm)
  vpaddusw = OpsProd(MnemonicOp('vpaddusw'), mem|ymm, ymm, ymm)
  vpmaxub = OpsProd(MnemonicOp('vpmaxub'), mem|ymm, ymm, ymm)
  vpandn = OpsProd(MnemonicOp('vpandn'), mem|ymm, ymm, ymm)

  return (
      vpunpcklbw | vpunpcklwd | vpunpckldq |
      vpacksswb |
      vpcmpgtb | vpcmpgtw | vpcmpgtd |
      vpackuswb |
      vpunpckhbw | vpunpckhwd | vpunpckhdq |
      vpackssdw |
      vpunpcklqdq | vpunpckhqdq |
      vpshufd | vpshufhw | vpshuflw |
      vpcmpeqb | vpcmpeqw | vpcmpeqd |
      vpsrlw | vpsrld | vpsrlq |
      vpaddq |
      vpmullw |
      vpmovmskb |
      vpsubusb | vpsubusw |
      vpminub |
      vpand |
      vpaddusb | vpaddusw |
      vpmaxub |
      vpandn
  )


def Validate(trie_diffs, bitness):
  """Validates that all ymm variants 0f 6x and 0f 7x groups are added."""
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=Instructions(bitness),
      expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
