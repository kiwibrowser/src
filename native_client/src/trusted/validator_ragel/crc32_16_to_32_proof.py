# Copyright (c) 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for enabling crc32w"""

import proof_tools
from proof_tools import MnemonicOp
from proof_tools import OpsProd


def Instructions(bitness):
  """Returns crc32w variants."""
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  reg16_read = proof_tools.GprOperands(bitness, operand_size=16,
                                       is_write_for_64_bit=False)
  reg32_write = proof_tools.GprOperands(bitness, operand_size=32,
                                        is_write_for_64_bit=True)
  crc32w = OpsProd(MnemonicOp('crc32w'), mem|reg16_read, reg32_write)
  return crc32w


def Validate(trie_diffs, bitness):
  """Validates that all crc32w variants are added."""
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=Instructions(bitness),
      expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
