# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for verifying that 16bit bsr/bsf is added to the 32 bit DFA."""

import proof_tools


def Validate(trie_diffs, bitness):
  """Validate the trie_diffs adds 16 bit bsr/bsf is added in 32bit mode."""

  # No instructions should be removed for 32/64 bit DFAs.
  # No instructions should be added for 64 bit DFA because it already
  # contains 16 bit bsr/bsf instructions.
  # in 32-bit mode, 16 bit bsr/bsf should be added.
  # This is of the forms:
  #   {bsr,bsf} mem16/reg16, reg16
  if bitness == 32:
    mnemonics = proof_tools.MnemonicOp('bsr') | proof_tools.MnemonicOp('bsf')
    reg16 = proof_tools.GprOperands(bitness=bitness, operand_size=16)
    mem = proof_tools.AllMemoryOperands(bitness=bitness)
    expected_adds = proof_tools.OpsProd(mnemonics, reg16 | mem, reg16)
  else:
    expected_adds = set()

  proof_tools.AssertDiffSetEquals(trie_diffs,
                                  expected_adds=expected_adds,
                                  expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
