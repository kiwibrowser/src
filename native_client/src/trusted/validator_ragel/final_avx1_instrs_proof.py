# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Proof for enabling remaining disabled avx1 instrs."""

import proof_tools
from proof_tools import MnemonicOp
from proof_tools import OpsProd


def Instructions(bitness):
  """Returns all new instructions to be added with this change."""
  imm = proof_tools.ImmOp()
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  xmm = proof_tools.AllXMMOperands(bitness)
  ymm = proof_tools.AllYMMOperands(bitness)
  reg32_write = proof_tools.GprOperands(bitness, operand_size=32,
                                        is_write_for_64_bit=True)
  reg32_restricting_write = proof_tools.GprOperands(bitness, operand_size=32,
                                                    is_write_for_64_bit=True,
                                                    can_restrict=True)

  reg32 = proof_tools.GprOperands(bitness, operand_size=32,
                                  is_write_for_64_bit=False)
  reg64 = None
  reg64_write = None
  if bitness == 64:
    reg64 = proof_tools.GprOperands(bitness=bitness, operand_size=64,
                                    is_write_for_64_bit=False)
    reg64_write = proof_tools.GprOperands(bitness=bitness, operand_size=64,
                                          is_write_for_64_bit=True)

  vcmpeqpd = (OpsProd(MnemonicOp('vcmpeqpd'), mem|xmm, xmm, xmm) |
              OpsProd(MnemonicOp('vcmpeqpd'), mem|ymm, ymm, ymm))
  vcmpeqps = (OpsProd(MnemonicOp('vcmpeqps'), mem|xmm, xmm, xmm) |
              OpsProd(MnemonicOp('vcmpeqps'), mem|ymm, ymm, ymm))
  vcmpeqss = OpsProd(MnemonicOp('vcmpeqss'), mem|xmm, xmm, xmm)
  vcvtpd2dq = OpsProd(MnemonicOp('vcvtpd2dq'), xmm|ymm, xmm)
  vcvtpd2dqx = OpsProd(MnemonicOp('vcvtpd2dqx'), mem, xmm)
  vcvtpd2dqy = OpsProd(MnemonicOp('vcvtpd2dqy'), mem, xmm)
  vcvtpd2ps = OpsProd(MnemonicOp('vcvtpd2ps'), xmm|ymm, xmm)
  vcvtpd2psx = OpsProd(MnemonicOp('vcvtpd2psx'), mem, xmm)
  vcvtpd2psy = OpsProd(MnemonicOp('vcvtpd2psy'), mem, xmm)

  vcvtsi2sd = (OpsProd(MnemonicOp('vcvtsi2sd'), reg32, xmm, xmm) |
               OpsProd(MnemonicOp('vcvtsi2sdl'), mem, xmm, xmm))
  if bitness == 64:
    vcvtsi2sd |= (OpsProd(MnemonicOp('vcvtsi2sd'), reg64, xmm, xmm) |
                  OpsProd(MnemonicOp('vcvtsi2sdq'), mem, xmm, xmm))

  vcvtsi2ss = (OpsProd(MnemonicOp('vcvtsi2ss'), reg32, xmm, xmm) |
               OpsProd(MnemonicOp('vcvtsi2ssl'), mem, xmm, xmm))
  if bitness == 64:
    vcvtsi2ss |= (OpsProd(MnemonicOp('vcvtsi2ss'), reg64, xmm, xmm) |
                  OpsProd(MnemonicOp('vcvtsi2ssq'), mem, xmm, xmm))

  vcvtsd2si = OpsProd(MnemonicOp('vcvtsd2si'), xmm|mem, reg32_write)
  if bitness == 64:
    vcvtsd2si |= OpsProd(MnemonicOp('vcvtsd2si'), xmm|mem, reg64_write)

  vcvtss2si = OpsProd(MnemonicOp('vcvtss2si'), xmm|mem, reg32_write)
  if bitness == 64:
    vcvtss2si |= OpsProd(MnemonicOp('vcvtss2si'), xmm|mem, reg64_write)

  vcvttpd2dq = OpsProd(MnemonicOp('vcvttpd2dq'), xmm|ymm, xmm)
  vcvttpd2dqx = OpsProd(MnemonicOp('vcvttpd2dqx'), mem, xmm)
  vcvttpd2dqy = OpsProd(MnemonicOp('vcvttpd2dqy'), mem, xmm)

  vcvttsd2si = OpsProd(MnemonicOp('vcvttsd2si'), mem|xmm, reg32_write)
  if bitness == 64:
    vcvttsd2si |= OpsProd(MnemonicOp('vcvttsd2si'), mem|xmm, reg64_write)

  vcvttss2si = OpsProd(MnemonicOp('vcvttss2si'), mem|xmm, reg32_write)
  if bitness == 64:
    vcvttss2si |= OpsProd(MnemonicOp('vcvttss2si'), mem|xmm, reg64_write)

  vcvttps2dq = (OpsProd(MnemonicOp('vcvttps2dq'), mem|xmm, xmm) |
                OpsProd(MnemonicOp('vcvttps2dq'), mem|ymm, ymm))

  vextractps = OpsProd(MnemonicOp('vextractps'), imm, xmm, mem)

  vinsertf128 = OpsProd(MnemonicOp('vinsertf128'), imm, mem|xmm, ymm, ymm)

  vmovd = (OpsProd(MnemonicOp('vmovd'), mem|reg32, xmm) |
           OpsProd(MnemonicOp('vmovd'), xmm, mem|reg32_restricting_write))

  vmovq = (OpsProd(MnemonicOp('vmovq'), xmm, xmm|mem) |
           OpsProd(MnemonicOp('vmovq'), xmm|mem, xmm))
  if bitness == 64:
    vmovq |= (OpsProd(MnemonicOp('vmovq'), reg64|mem, xmm) |
              OpsProd(MnemonicOp('vmovq'), xmm, reg64_write|mem))

  vpinsrb = OpsProd(MnemonicOp('vpinsrb'), imm, mem|reg32, xmm, xmm)
  vpinsrd = OpsProd(MnemonicOp('vpinsrd'), imm, mem|reg32, xmm, xmm)
  vpinsrw = OpsProd(MnemonicOp('vpinsrw'), imm, mem|reg32, xmm, xmm)
  vrsqrtss = OpsProd(MnemonicOp('vrsqrtss'), mem|xmm, xmm, xmm)

  instructions = (
      vcmpeqpd | vcmpeqps | vcmpeqss |
      vcvtpd2dq | vcvtpd2dqx | vcvtpd2dqy |
      vcvtpd2ps | vcvtpd2psx | vcvtpd2psy |
      vcvtsi2sd | vcvtsi2ss | vcvtsd2si | vcvtss2si |
      vcvttpd2dq | vcvttpd2dqx | vcvttpd2dqy |
      vcvttps2dq |
      vcvttsd2si | vcvttss2si |
      vextractps |
      vinsertf128 |
      vmovd | vmovq |
      vpinsrb | vpinsrd | vpinsrw |
      vrsqrtss
  )

  if bitness == 32:
    vmaskmovdqu = OpsProd(MnemonicOp('vmaskmovdqu'), xmm, xmm)
    # While this instruction is allowed in 64 bit mode, it has an implicit
    # masked memory operand, and is hence a superinstruction covered by
    # super instruction verification.
    instructions |= vmaskmovdqu
  elif bitness == 64:
    vpinsrq = OpsProd(MnemonicOp('vpinsrq'), imm, mem|reg64, xmm, xmm)
    vpextrq = OpsProd(MnemonicOp('vpextrq'), imm, xmm, mem|reg64_write)
    instructions |= (vpinsrq | vpextrq)

  return instructions


def Validate(trie_diffs, bitness):
  proof_tools.AssertDiffSetEquals(
      trie_diffs,
      expected_adds=Instructions(bitness),
      expected_removes=set())

if __name__ == '__main__':
  proof_tools.RunProof(proof_tools.ParseStandardOpts(), Validate)
