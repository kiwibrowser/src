# Copyright (c) 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for enabling vmovd Vo Ed, vmovq Vo Eq instrs."""

import proof_tools
from proof_tools import MnemonicOp
from proof_tools import OpsProd


def Instructions(bitness):
  """Returns all new instructions to be added with this change."""
  xmm = proof_tools.AllXMMOperands(bitness)
  reg32_restricting_write = proof_tools.GprOperands(bitness, operand_size=32,
                                                    is_write_for_64_bit=True,
                                                    can_restrict=True)
  mem = proof_tools.AllMemoryOperands(bitness=bitness)

  vmovd = OpsProd(MnemonicOp('vmovd'), xmm, mem|reg32_restricting_write)
  instructions = (
      vmovd
  )

  if bitness == 64:
    reg64_write = proof_tools.GprOperands(bitness=bitness, operand_size=64,
                                          is_write_for_64_bit=True)
    vmovq = OpsProd(MnemonicOp('vmovq'), xmm, mem|reg64_write)
    instructions = (
        instructions | vmovq
    )

  return instructions


def Validate(trie_diffs, bitness):
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=Instructions(bitness),
      expected_removes=Instructions(bitness))

if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
