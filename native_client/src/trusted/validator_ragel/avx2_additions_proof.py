# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for avx2 instructions that are newly added (except vgather)"""

import proof_tools
from proof_tools import MnemonicOp
from proof_tools import OpsProd

def Instructions(bitness):
  """Returns new avx2 instructions."""
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  ymm = proof_tools.AllYMMOperands(bitness)
  xmm = proof_tools.AllXMMOperands(bitness)
  imm = proof_tools.ImmOp()

  vbroadcasti128 = OpsProd(MnemonicOp('vbroadcasti128'), mem, ymm)
  vbroadcastsd = OpsProd(MnemonicOp('vbroadcastsd'), xmm, ymm)
  vbroadcastss = OpsProd(MnemonicOp('vbroadcastss'), xmm, xmm|ymm)
  vextracti128 = OpsProd(MnemonicOp('vextracti128'), imm, ymm, mem|xmm)
  vinserti128 = OpsProd(MnemonicOp('vinserti128'), imm, mem|xmm, ymm, ymm)
  vpblendd = (OpsProd(MnemonicOp('vpblendd'), imm, mem|xmm, xmm, xmm) |
              OpsProd(MnemonicOp('vpblendd'), imm, mem|ymm, ymm, ymm))
  vpbroadcastb = OpsProd(MnemonicOp('vpbroadcastb'), mem|xmm, xmm|ymm)
  vpbroadcastd = OpsProd(MnemonicOp('vpbroadcastd'), mem|xmm, xmm|ymm)
  vpbroadcastw = OpsProd(MnemonicOp('vpbroadcastw'), mem|xmm, xmm|ymm)
  vpbroadcastq = OpsProd(MnemonicOp('vpbroadcastq'), mem|xmm, xmm|ymm)
  vperm2i128 = OpsProd(MnemonicOp('vperm2i128'), imm, mem|ymm, ymm, ymm)
  vpermd = OpsProd(MnemonicOp('vpermd'), mem|ymm, ymm, ymm)
  vpermpd = OpsProd(MnemonicOp('vpermpd'), imm, mem|ymm, ymm)
  vpermps = OpsProd(MnemonicOp('vpermps'), mem|ymm, ymm, ymm)
  vpermq = OpsProd(MnemonicOp('vpermq'), imm, mem|ymm, ymm)
  vpmaskmovd = (OpsProd(MnemonicOp('vpmaskmovd'), mem, xmm, xmm) |
                OpsProd(MnemonicOp('vpmaskmovd'), mem, ymm, ymm) |
                OpsProd(MnemonicOp('vpmaskmovd'), xmm, xmm, mem) |
                OpsProd(MnemonicOp('vpmaskmovd'), ymm, ymm, mem))
  vpmaskmovq = (OpsProd(MnemonicOp('vpmaskmovq'), mem, xmm, xmm) |
                OpsProd(MnemonicOp('vpmaskmovq'), mem, ymm, ymm) |
                OpsProd(MnemonicOp('vpmaskmovq'), xmm, xmm, mem) |
                OpsProd(MnemonicOp('vpmaskmovq'), ymm, ymm, mem))
  vpsllvd = (OpsProd(MnemonicOp('vpsllvd'), mem|xmm, xmm, xmm) |
             OpsProd(MnemonicOp('vpsllvd'), mem|ymm, ymm, ymm))
  vpsllvq = (OpsProd(MnemonicOp('vpsllvq'), mem|xmm, xmm, xmm) |
             OpsProd(MnemonicOp('vpsllvq'), mem|ymm, ymm, ymm))
  vpsravd = (OpsProd(MnemonicOp('vpsravd'), mem|xmm, xmm, xmm) |
             OpsProd(MnemonicOp('vpsravd'), mem|ymm, ymm, ymm))
  vpsrlvd = (OpsProd(MnemonicOp('vpsrlvd'), mem|xmm, xmm, xmm) |
             OpsProd(MnemonicOp('vpsrlvd'), mem|ymm, ymm, ymm))
  vpsrlvq = (OpsProd(MnemonicOp('vpsrlvq'), mem|xmm, xmm, xmm) |
             OpsProd(MnemonicOp('vpsrlvq'), mem|ymm, ymm, ymm))

  return (
    vbroadcasti128 | vbroadcastsd | vbroadcastss |
    vextracti128 | vinserti128 |
    vpblendd |
    vpbroadcastb | vpbroadcastd | vpbroadcastq | vpbroadcastw |
    vperm2i128 | vpermd | vpermpd | vpermps | vpermq |
    vpmaskmovd | vpmaskmovq |
    vpsllvd | vpsllvq |
    vpsravd |
    vpsrlvd | vpsrlvq
  )


def Validate(trie_diffs, bitness):
  """Validates that all new avx2 instructions (except vgather) are added."""
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=Instructions(bitness),
      expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
