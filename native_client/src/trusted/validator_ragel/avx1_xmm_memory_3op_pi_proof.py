# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for adding 3 operand xmm/memory packed 16 bit (integer/word in x86)."""

import proof_tools
from proof_tools_templates import XmmOrMemory3operand


# Packed integer (16 bit) AVX1 Instructions of the form:
# src1 is an xmm register.
# src2 is an xmm register or memory operand.
# dest is an xmm register.
MNEMONICS = [
    'vpaddsw', 'vpaddusw', 'vpaddw', 'vpavgw', 'vpcmpeqw', 'vpcmpgtw',
    'vphaddsw', 'vphaddw', 'vphsubsw', 'vphsubw', 'vpmaxsw', 'vpmaxuw',
    'vpminsw', 'vpminuw', 'vpmulhrsw', 'vpmulhuw', 'vpmulhw', 'vpmullw',
    'vpsubsw', 'vpsubusw', 'vpsubw', 'vpunpckhwd', 'vpunpcklwd',
]


def Validate(trie_diffs, bitness):
  """Validates that all allowed patterns of MNEMONICS are added."""
  expected_adds = set()
  for mnemonic in MNEMONICS:
    expected_adds.update(XmmOrMemory3operand(mnemonic_name=mnemonic,
                                             bitness=bitness))

  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=expected_adds,
      expected_removes=set())

if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
