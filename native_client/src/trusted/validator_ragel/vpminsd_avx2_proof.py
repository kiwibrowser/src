# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for avx2 upgrade (allow ymm) of vpminsd."""

import proof_tools
from proof_tools import MnemonicOp
from proof_tools import OpsProd


def Validate(trie_diffs, bitness):
  """Validates that all ymm vpminsd variants are added."""
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  ymm = proof_tools.AllYMMOperands(bitness)
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=OpsProd(MnemonicOp('vpminsd'), mem|ymm, ymm, ymm),
      expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
