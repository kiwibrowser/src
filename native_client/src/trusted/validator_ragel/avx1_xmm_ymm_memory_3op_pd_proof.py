# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for adding 3 operand xmm/ymm/memory packed double precision ops."""

import proof_tools
from proof_tools_templates import XmmYmmOrMemory3operand


# Packed FP-double-precision AVX1 Instructions of the form:
# src1 is an xmm or ymm register.
# src2 is an xmm or ymm register or memory operand.
# dest is an xmm or ymm register.
MNEMONICS = [
    'vaddpd', 'vaddsubpd', 'vandnpd', 'vandpd', 'vdivpd', 'vhaddpd',
    'vhsubpd', 'vmaxpd', 'vminpd', 'vmulpd', 'vorpd', 'vsubpd', 'vunpckhpd',
    'vunpcklpd', 'vxorpd'
]


def Validate(trie_diffs, bitness):
  """Validates that all allowed patterns of MNEMONICS are added."""
  expected_adds = set()
  for mnemonic in MNEMONICS:
    expected_adds.update(XmmYmmOrMemory3operand(mnemonic_name=mnemonic,
                                                bitness=bitness))

  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=expected_adds,
      expected_removes=set())

if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
