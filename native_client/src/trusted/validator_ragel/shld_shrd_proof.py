# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for verifying that 16bit shld/shrd is added to the 32 bit DFA."""

import proof_tools


def Validate(trie_diffs, bitness):
  """Validate that trie_diffs adds 16 bit shld/shrd in 32bit mode."""

  # No instructions should be removed for 32/64 bit DFAs.
  # No instructions should be added for 64 bit DFA because it already
  # contains 16 bit shld/shrd instructions.
  # in 32-bit mode, 16 bit shld/shrd should be added.
  # This is of the forms:
  # shld|shrd, imm|%cl, reg16, reg16|mem
  if bitness == 32:
    mnemonics = proof_tools.MnemonicOp('shld') | proof_tools.MnemonicOp('shrd')
    reg16 = proof_tools.GprOperands(bitness=bitness, operand_size=16)
    mem = proof_tools.AllMemoryOperands(bitness=bitness)
    imm = proof_tools.ImmOp()
    cl = set([proof_tools.Operands(disasms=('%cl',))])

    expected_adds = proof_tools.OpsProd(
        mnemonics, imm | cl, reg16, reg16 | mem)

  else:
    expected_adds = set()

  proof_tools.AssertDiffSetEquals(trie_diffs,
                                  expected_adds=expected_adds,
                                  expected_removes=set())

if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
