# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for adding all remaining disabled 3 operand xmm/ymm/memory avx1 ops."""

import proof_tools
from proof_tools_templates import XmmOrMemory3operand
from proof_tools_templates import XmmYmmOrMemory3operand

# AVX1 Instructions of the form:
# src1 is an xmm register.
# src2 is an xmm register or memory operand.
# dest is an xmm register.
XMM_MNEMONICS = [
    'vcvtsd2ss', 'vcvtss2sd', 'vpackssdw', 'vpacksswb', 'vpackusdw',
    'vpackuswb', 'vpaddd', 'vpaddq', 'vpand', 'vpandn', 'vpcmpeqd',
    'vpcmpeqq', 'vpcmpgtd', 'vpcmpgtq', 'vphaddd', 'vphsubd', 'vpmaddubsw',
    'vpmaddwd', 'vpmaxsd', 'vpmaxud', 'vpminsd', 'vpminud', 'vpmuldq',
    'vpmulld', 'vpmuludq', 'vpor', 'vpsadbw', 'vpsignd', 'vpsignw', 'vpslld',
    'vpsllq', 'vpsllw', 'vpsrad', 'vpsraw', 'vpsrld', 'vpsrlq', 'vpsrlw',
    'vpsubd', 'vpsubq', 'vpunpckhdq', 'vpunpckhqdq', 'vpunpckldq',
    'vpunpcklqdq', 'vpxor', 'vrcpss', 'vsqrtsd', 'vsqrtss',
]

# AVX1 Instructions of the form:
# src1 is an xmm or ymm register.
# src2 is an xmm or ymm register or memory operand.
# dest is an xmm or ymm register.
XMM_YMM_MNEMONICS = [
    'vpermilpd', 'vpermilps',
]

def Validate(trie_diffs, bitness):
  """Validates that all allowed patterns of MNEMONICS are added."""
  expected_adds = set()
  for mnemonic in XMM_MNEMONICS:
    expected_adds.update(XmmOrMemory3operand(mnemonic_name=mnemonic,
                                             bitness=bitness))
  for mnemonic in XMM_YMM_MNEMONICS:
    expected_adds.update(XmmYmmOrMemory3operand(mnemonic_name=mnemonic,
                                                bitness=bitness))

  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=expected_adds,
      expected_removes=set())

if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
