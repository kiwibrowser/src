# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import itertools
import re
import spec


def SkipHeader(objdump_output_lines):
  # Objdump prints few lines about file and section before disassembly
  # listing starts, so we have to skip that.
  objdump_header_size = 7
  return itertools.islice(objdump_output_lines, objdump_header_size, None)


Instruction = collections.namedtuple(
    'Instruction',
    ['address', 'bytes', 'disasm'])

class Instruction(Instruction):
  def __repr__(self):
    hex_bytes = ' '.join('%02x' % b for b in self.bytes)
    return 'Instruction(0x%x: %s  %s)' % (self.address, hex_bytes, self.disasm)


def ParseLine(line):
  """Split objdump line.

  Typical line from objdump output looks like this:

  "   0:  90                    nop"

  There are three columns (separated with tabs):
    address: offset for a particular line
    bytes: few bytes which encode a given instruction
    disasm: textual representation of the instruction

  Args:
      line: objdump line (a single one).
  Returns
      Instruction tuple.
  """

  address, bytes, disasm = line.strip().split('\t')
  assert disasm.strip() != ''

  assert address.endswith(':')
  address = int(address[:-1], 16)

  return Instruction(address, [int(byte, 16) for byte in bytes.split()], disasm)


def CanonicalizeInstruction(insn, simplify_condition_jumps=False):
  # Canonicalize whitespaces.
  disasm = ' '.join(insn.disasm.split())

  # Find and remove "rex" or "rex.WRXB" prefix from line.
  # Note that when objdump line consists entirely of rex prefix,
  # it won't be removed (because there would be no space at the end)
  rex_prefix = re.compile(r'(rex([.]W?R?X?B?)? )')
  disasm = re.sub(rex_prefix, '', disasm, count=1)

  # Drop branch prediction and address from jumps.
  if simplify_condition_jumps:
    m = spec.CONDITION_JUMPS_RE.match(disasm)
    if m:
      disasm = m.group('name')

  return Instruction(insn.address, insn.bytes, disasm)
