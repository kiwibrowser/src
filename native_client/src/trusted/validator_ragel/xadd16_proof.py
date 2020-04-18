# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for verifying that 16bit xadd is added to the 32 bit DFA."""

import proof_tools
from proof_tools_templates import LockedRegWithRegOrMem16bit


def Validate(trie_diffs, bitness):
  """Validate the trie_diffs adds 16 bit xadd to 32bit mode."""
  # No instructions should be removed for 32/64 bit DFAs.
  # No instructions should be added for 64 bit DFA because it already
  # contains 16 bit cmpxchg instruction.
  if bitness == 32:
    expected_adds = LockedRegWithRegOrMem16bit(mnemonic_name='xadd',
                                               bitness=bitness)
  else:
    expected_adds = set()

  proof_tools.AssertDiffSetEquals(trie_diffs,
                                  expected_adds=expected_adds,
                                  expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
