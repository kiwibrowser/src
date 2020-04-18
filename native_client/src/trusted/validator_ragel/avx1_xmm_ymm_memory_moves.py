# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for adding avx1 move instructions for memory/xmm/ymm registers."""

import proof_tools
from proof_tools import MnemonicOp
from proof_tools import OpsProd
from proof_tools_templates import AllXmmYmmMemoryMoves
from proof_tools_templates import AllXmmYmmMemorytoXmmYmmMoves
from proof_tools_templates import XmmToMemoryOrXmmAndMemoryToXmmMoves


def Avx1XmmYmmMemoryMoves(bitness):
  """Returns avx1 xmm/ymm/memory move instructions (excluding non-temporal)."""
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  xmm = proof_tools.AllXMMOperands(bitness)
  ymm = proof_tools.AllYMMOperands(bitness)

  vmovapd = AllXmmYmmMemoryMoves('vmovapd', bitness)
  vmovaps = AllXmmYmmMemoryMoves('vmovaps', bitness)
  vmovddup = AllXmmYmmMemorytoXmmYmmMoves('vmovddup', bitness)
  vmovdqa = AllXmmYmmMemoryMoves('vmovdqa', bitness)
  vmovdqu = AllXmmYmmMemoryMoves('vmovdqu', bitness)
  vmovhlps = (OpsProd(MnemonicOp('vmovhlps'), xmm, xmm, xmm))
  vmovhpd = XmmToMemoryOrXmmAndMemoryToXmmMoves('vmovhpd', bitness)
  vmovhps = XmmToMemoryOrXmmAndMemoryToXmmMoves('vmovhps', bitness)
  vmovlhps = (OpsProd(MnemonicOp('vmovlhps'), xmm, xmm, xmm))
  vmovlpd = XmmToMemoryOrXmmAndMemoryToXmmMoves('vmovlpd', bitness)
  vmovlps = XmmToMemoryOrXmmAndMemoryToXmmMoves('vmovlps', bitness)
  vmovsd = (OpsProd(MnemonicOp('vmovsd'), xmm, mem) |
            OpsProd(MnemonicOp('vmovsd'), mem, xmm) |
            OpsProd(MnemonicOp('vmovsd'), xmm, xmm, xmm))
  vmovshdup = AllXmmYmmMemorytoXmmYmmMoves('vmovshdup', bitness)
  vmovsldup = AllXmmYmmMemorytoXmmYmmMoves('vmovsldup', bitness)
  vmovss = (OpsProd(MnemonicOp('vmovss'), xmm, xmm, xmm) |
            OpsProd(MnemonicOp('vmovss'), mem, xmm) |
            OpsProd(MnemonicOp('vmovss'), xmm, mem))
  vmovupd = AllXmmYmmMemoryMoves('vmovupd', bitness)
  vmovups = AllXmmYmmMemoryMoves('vmovups', bitness)

  moves = (vmovapd | vmovaps | vmovddup | vmovdqa | vmovdqu | vmovhlps |
           vmovhpd | vmovhps | vmovlhps | vmovlpd | vmovlps | vmovsd |
           vmovshdup | vmovsldup | vmovss | vmovupd | vmovups)
  return moves


def Validate(trie_diffs, bitness):
  """Validates that all allowed patterns of the moves are added."""
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=Avx1XmmYmmMemoryMoves(bitness),
      expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
