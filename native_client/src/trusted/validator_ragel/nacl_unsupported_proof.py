# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof that tries do not change when introducing a new attribute
nacl-unsupported in def files."""

import proof_tools


def Validate(trie_diffs, bitness):
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=set(),
      expected_removes=set())

if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
