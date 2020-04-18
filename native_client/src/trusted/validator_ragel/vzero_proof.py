# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for verifying that vzeroall and vzeroupper were added."""

import proof_tools


def Validate(trie_diffs, _):
  # vzeroall and vzeroupper both disassemble into only the mnemonic with
  # no operands. They zero out all of the YMM registers, or just the top
  # 128 bits of all YMM registers respectively.
  # They should both be added, and no instruction should be removed.
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=(proof_tools.MnemonicOp('vzeroall') |
                     proof_tools.MnemonicOp('vzeroupper')),
      expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
