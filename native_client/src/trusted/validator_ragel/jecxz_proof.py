# Copyright (c) 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for enabling jecxz"""

import proof_tools
from proof_tools import MnemonicOp
from proof_tools import OpsProd


def Instructions(bitness):
  """Returns 32-bit jecxz variants."""
  if bitness == 32:
    jecxz = OpsProd(MnemonicOp('jecxz'))
    return jecxz
  return set()


def Validate(trie_diffs, bitness):
  """Validates that 32-bit jecxz variants are added."""
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=Instructions(bitness),
      expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
