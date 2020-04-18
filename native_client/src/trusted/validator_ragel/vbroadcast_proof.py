# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for adding vbroadcast{f128/sd/ss} instructions"""

import proof_tools


def Validate(trie_diffs, bitness):
  """Validates that all allowed patterns of vbroadcast* are added"""
  xmm = proof_tools.AllXMMOperands(bitness)
  ymm = proof_tools.AllYMMOperands(bitness)
  mem = proof_tools.AllMemoryOperands(bitness)

  # vbroadcastf128 and vbroadcastsd work with ymm registers.
  # vbroadcastsd can work with xmm or ymm registers.
  vbroadcastf128 = proof_tools.OpsProd(
      proof_tools.MnemonicOp('vbroadcastf128'), mem, ymm)
  vbroadcastsd = proof_tools.OpsProd(
      proof_tools.MnemonicOp('vbroadcastsd'), mem, ymm)
  vbroadcastss = proof_tools.OpsProd(
      proof_tools.MnemonicOp('vbroadcastss'), mem, xmm | ymm)

  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=(vbroadcastf128 | vbroadcastsd | vbroadcastss),
      expected_removes=set())

if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
