# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Templates for grouping together similar proofs."""

import proof_tools


def LockedRegWithRegOrMem16bit(mnemonic_name, bitness):
  """Computes all 16 bit optionally locked reg, reg/mem ops.

  e.g.

  lock instr reg16 mem
    or
  instr reg16 mem/reg16.

  Note: when locking is issued, the second operand must be memory and not a reg.

  Args:
    mnemonic_name: the mnemonic we are producing the pattern for.
    bitness: the bitness of the architecture (32 or 64)
  Returns:
    All possible disassembly sequences that follow the pattern.
  """
  assert bitness in (32, 64)
  instr = proof_tools.MnemonicOp(mnemonic_name)
  reg16 = proof_tools.GprOperands(bitness=bitness, operand_size=16)
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  lock = proof_tools.LockPrefix()
  return (proof_tools.OpsProd(lock, instr, reg16, mem) |
          proof_tools.OpsProd(instr, reg16, reg16 | mem))


def XmmYmmOrMemory3operand(mnemonic_name, bitness):
  """Set of 3 operand xmm/ymm/memory ops (memory is a possible source operand).

  e.g.

  instr xmm1/mem, xmm2, xmm3
  or
  instr ymm1/mem, ymm2, ymm3

  Args:
    mnemonic_name: the mnemonic we are producing the pattern for.
    bitness: the bitness of the architecture (32 or 64)
  Returns:
    All possible disassembly sequences that follow the pattern.
  """
  xmm = proof_tools.AllXMMOperands(bitness)
  ymm = proof_tools.AllYMMOperands(bitness)
  mem = proof_tools.AllMemoryOperands(bitness)
  mnemonic = proof_tools.MnemonicOp(mnemonic_name)

  return (proof_tools.OpsProd(mnemonic, (xmm | mem), xmm, xmm) |
          proof_tools.OpsProd(mnemonic, (ymm | mem), ymm, ymm))

def XmmOrMemory3operand(mnemonic_name, bitness):
  """Set of 3 operand xmm/memory ops (memory is a possible source operand).

  e.g.  instr xmm1/mem, xmm2, xmm3

  Args:
    mnemonic_name: the mnemonic we are producing the pattern for.
    bitness: the bitness of the architecture (32 or 64)
  Returns:
    All possible disassembly sequences that follow the pattern.
  """
  xmm = proof_tools.AllXMMOperands(bitness)
  mem = proof_tools.AllMemoryOperands(bitness)
  mnemonic = proof_tools.MnemonicOp(mnemonic_name)

  return (proof_tools.OpsProd(mnemonic, (xmm | mem), xmm, xmm))

def AllXmmYmmMemoryMoves(mnemonic_name, bitness):
  """Set of all 2 operand xmm/ymm/memory moves.

  Allows all moves between xmm/ymm registers, and allows moves from xmm/ymm to
  memory or viceversa (x86 only allows encoding one memory operand) .

  Args:
    mnemonic_name: the mnemonic we are producing the pattern for.
    bitness: the bitness of the architecture (32 or 64)
  Returns:
    All possible disassembly sequences that follow the pattern.
  """
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  xmm = proof_tools.AllXMMOperands(bitness)
  ymm = proof_tools.AllYMMOperands(bitness)
  return (proof_tools.OpsProd(
              proof_tools.MnemonicOp(mnemonic_name), mem, xmm | ymm) |
          proof_tools.OpsProd(
              proof_tools.MnemonicOp(mnemonic_name), xmm|ymm, mem)  |
          proof_tools.OpsProd(
              proof_tools.MnemonicOp(mnemonic_name), xmm, xmm) |
          proof_tools.OpsProd(
              proof_tools.MnemonicOp(mnemonic_name), ymm, ymm))

def AllXmmYmmMemorytoXmmYmmMoves(mnemonic_name, bitness):
  """Set of all 2 operand xmm/ymm/memory to xmm/ymm moves.

  Allows all moves between xmm/ymm registers, and allows moves from memory to
  xmm/ymm registers .

  Args:
    mnemonic_name: the mnemonic we are producing the pattern for.
    bitness: the bitness of the architecture (32 or 64)
  Returns:
    All possible disassembly sequences that follow the pattern.
  """
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  xmm = proof_tools.AllXMMOperands(bitness)
  ymm = proof_tools.AllYMMOperands(bitness)
  return (proof_tools.OpsProd(
              proof_tools.MnemonicOp(mnemonic_name), xmm | mem, xmm) |
          proof_tools.OpsProd(
              proof_tools.MnemonicOp(mnemonic_name), ymm | mem, ymm))

def XmmToMemoryOrXmmAndMemoryToXmmMoves(mnemonic_name, bitness):
  """2 operand xmm->memory move or 3 operand xmm,memory->xmm move.

  Args:
    mnemonic_name: the mnemonic we are producing the pattern for.
    bitness: the bitness of the architecture (32 or 64)
  Returns:
    All possible disassembly sequences that follow the pattern.
  """
  mem = proof_tools.AllMemoryOperands(bitness=bitness)
  xmm = proof_tools.AllXMMOperands(bitness)
  return (proof_tools.OpsProd(
              proof_tools.MnemonicOp(mnemonic_name), mem, xmm, xmm) |
          proof_tools.OpsProd(
              proof_tools.MnemonicOp(mnemonic_name), xmm, mem))
