# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for adding fma3 ops."""

import proof_tools
from proof_tools_templates import XmmOrMemory3operand
from proof_tools_templates import XmmYmmOrMemory3operand


def Instructions(bitness):
  """Returns all new instructions to be added with this change."""
  return (XmmYmmOrMemory3operand('vfmadd132pd', bitness) |
          XmmYmmOrMemory3operand('vfmadd213pd', bitness) |
          XmmYmmOrMemory3operand('vfmadd231pd', bitness) |
          XmmYmmOrMemory3operand('vfmadd132ps', bitness) |
          XmmYmmOrMemory3operand('vfmadd213ps', bitness) |
          XmmYmmOrMemory3operand('vfmadd231ps', bitness) |
          XmmOrMemory3operand('vfmadd132sd', bitness) |
          XmmOrMemory3operand('vfmadd213sd', bitness) |
          XmmOrMemory3operand('vfmadd231sd', bitness) |
          XmmOrMemory3operand('vfmadd132ss', bitness) |
          XmmOrMemory3operand('vfmadd213ss', bitness) |
          XmmOrMemory3operand('vfmadd231ss', bitness) |
          XmmYmmOrMemory3operand('vfmaddsub132pd', bitness) |
          XmmYmmOrMemory3operand('vfmaddsub213pd', bitness) |
          XmmYmmOrMemory3operand('vfmaddsub231pd', bitness) |
          XmmYmmOrMemory3operand('vfmaddsub132ps', bitness) |
          XmmYmmOrMemory3operand('vfmaddsub213ps', bitness) |
          XmmYmmOrMemory3operand('vfmaddsub231ps', bitness) |
          XmmYmmOrMemory3operand('vfmsubadd132pd', bitness) |
          XmmYmmOrMemory3operand('vfmsubadd213pd', bitness) |
          XmmYmmOrMemory3operand('vfmsubadd231pd', bitness) |
          XmmYmmOrMemory3operand('vfmsubadd132ps', bitness) |
          XmmYmmOrMemory3operand('vfmsubadd213ps', bitness) |
          XmmYmmOrMemory3operand('vfmsubadd231ps', bitness) |
          XmmYmmOrMemory3operand('vfmsub132pd', bitness) |
          XmmYmmOrMemory3operand('vfmsub213pd', bitness) |
          XmmYmmOrMemory3operand('vfmsub231pd', bitness) |
          XmmYmmOrMemory3operand('vfmsub132ps', bitness) |
          XmmYmmOrMemory3operand('vfmsub213ps', bitness) |
          XmmYmmOrMemory3operand('vfmsub231ps', bitness) |
          XmmOrMemory3operand('vfmsub132sd', bitness) |
          XmmOrMemory3operand('vfmsub213sd', bitness) |
          XmmOrMemory3operand('vfmsub231sd', bitness) |
          XmmOrMemory3operand('vfmsub132ss', bitness) |
          XmmOrMemory3operand('vfmsub213ss', bitness) |
          XmmOrMemory3operand('vfmsub231ss', bitness) |
          XmmYmmOrMemory3operand('vfnmadd132pd', bitness) |
          XmmYmmOrMemory3operand('vfnmadd213pd', bitness) |
          XmmYmmOrMemory3operand('vfnmadd231pd', bitness) |
          XmmYmmOrMemory3operand('vfnmadd132ps', bitness) |
          XmmYmmOrMemory3operand('vfnmadd213ps', bitness) |
          XmmYmmOrMemory3operand('vfnmadd231ps', bitness) |
          XmmOrMemory3operand('vfnmadd132sd', bitness) |
          XmmOrMemory3operand('vfnmadd213sd', bitness) |
          XmmOrMemory3operand('vfnmadd231sd', bitness) |
          XmmOrMemory3operand('vfnmadd132ss', bitness) |
          XmmOrMemory3operand('vfnmadd213ss', bitness) |
          XmmOrMemory3operand('vfnmadd231ss', bitness) |
          XmmYmmOrMemory3operand('vfnmsub132pd', bitness) |
          XmmYmmOrMemory3operand('vfnmsub213pd', bitness) |
          XmmYmmOrMemory3operand('vfnmsub231pd', bitness) |
          XmmYmmOrMemory3operand('vfnmsub132ps', bitness) |
          XmmYmmOrMemory3operand('vfnmsub213ps', bitness) |
          XmmYmmOrMemory3operand('vfnmsub231ps', bitness) |
          XmmOrMemory3operand('vfnmsub132sd', bitness) |
          XmmOrMemory3operand('vfnmsub213sd', bitness) |
          XmmOrMemory3operand('vfnmsub231sd', bitness) |
          XmmOrMemory3operand('vfnmsub132ss', bitness) |
          XmmOrMemory3operand('vfnmsub213ss', bitness) |
          XmmOrMemory3operand('vfnmsub231ss', bitness))


def Validate(trie_diffs, bitness):
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=Instructions(bitness),
      expected_removes=set())


if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
