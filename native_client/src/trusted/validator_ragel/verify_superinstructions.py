#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""%prog [option...] superinsructions1.txt superinsructions2.txt...

Regex-based verifier for superinstructions list."""

import optparse
import os
import re
import subprocess
import sys
import tempfile

import objdump_parser
import spec
import validator


def ValidateSuperinstruction(superinstruction, validator_inst, bitness):
  """Validate superinstruction using the actual validator.

  Args:
    superinstruction: superinstruction byte sequence to validate
    validator_inst: instance of validator
    bitness: 32 or 64
  Returns:
    True if superinstruction is valid, otherwise False
  """

  bundle = bytes(superinstruction +
                 ((-len(superinstruction)) % validator.BUNDLE_SIZE) *  b'\x90')
  assert len(bundle) % validator.BUNDLE_SIZE == 0

  result = validator_inst.ValidateChunk(bundle, bitness=bitness)

  # Superinstructions are accepted if restricted_register != NC_REG_RSP or
  # NC_REG_RBP
  if bitness == 64:
    for register in validator.ALL_REGISTERS:
      # restricted_register can not be ever equal to %r15
      if register == validator.NC_REG_R15:
        continue
      expected = result and register not in (validator.NC_REG_RBP,
                                             validator.NC_REG_RSP)
      assert validator_inst.ValidateChunk(bundle,
                                          restricted_register=register,
                                          bitness=bitness) == expected

  # All bytes in the superinstruction are invalid jump targets
  #
  # Note: valid boundaries are determined in a C code without looking on
  # restricted_register variable.
  # TODO(khim): collect all the assumptions about C code fragments in one
  # document and move this note there.
  for offset in range(0, len(superinstruction) + 1):
    jmp_command = b'\xeb' + bytearray([0xfe - len(bundle) + offset])
    jmp_check = bundle + bytes(jmp_command) + (validator.BUNDLE_SIZE -
                                               len(jmp_command)) * b'\x90'
    expected = (offset == 0 or offset == len(superinstruction)) and result
    assert validator_inst.ValidateChunk(jmp_check, bitness=bitness) == expected

  return result


def ProcessSuperinstructionsFile(
    filename,
    validator_inst,
    bitness,
    gas,
    objdump,
    out_file):
  """Process superinstructions file.

  Each line produces either "True" or "False" plus text of original command
  (for the documentation purposes).  "True" means instruction is safe, "False"
  means instruction is unsafe.  This is needed since some instructions are
  incorrect but accepted by DFA (these should be rejected by actions embedded
  in DFA).

  If line contains something except valid set of x86 instruction assert
  error is triggered.

  Args:
      filename: name of file to process
      validator_inst: instance of validator
      bitness: 32 or 64
      gas: path to the GAS executable
      objdump: path to the OBJDUMP executable
      out_file: file object where log must be dumped
  Returns:
      None
  """

  try:
    object_file = tempfile.NamedTemporaryFile(
        prefix='verify_superinstructions_',
        suffix='.o',
        delete=False)
    object_file.close()

    subprocess.check_call([gas,
                           '--{0}'.format(bitness),
                           filename,
                           '-o{0}'.format(object_file.name)])

    objdump_proc = subprocess.Popen(
        [objdump, '-d', object_file.name, '--insn-width=15'],
        stdout=subprocess.PIPE)

    objdump_iter = iter(objdump_parser.SkipHeader(objdump_proc.stdout))

    line_prefix = '.byte '

    with open(filename) as superinstructions:
      for superinstruction_line in superinstructions:
        # Split the source line to find bytes
        assert superinstruction_line.startswith(line_prefix)
        # Leave only bytes here
        bytes_only = superinstruction_line[len(line_prefix):]
        superinstruction_bytes = [byte.strip(' \n')
                                  for byte in bytes_only.split(',')]
        superinstruction_validated = ValidateSuperinstruction(
            bytearray([int(byte, 16) for byte in superinstruction_bytes]),
            validator_inst,
            bitness)
        # Pick disassembled form of the superinstruction from objdump output
        superinstruction = []
        objdump_bytes = []
        while len(objdump_bytes) < len(superinstruction_bytes):
          nextline = next(objdump_iter).decode()
          instruction = objdump_parser.ParseLine(nextline)
          instruction = objdump_parser.CanonicalizeInstruction(instruction)
          superinstruction.append(instruction)
          objdump_bytes += instruction.bytes
        # Bytes in objdump output in and source file should match
        assert ['0x%02x' % b for b in objdump_bytes] == superinstruction_bytes
        if bitness == 32:
          validate_superinstruction = spec.ValidateSuperinstruction32
        else:
          validate_superinstruction = spec.ValidateSuperinstruction64

        try:
          validate_superinstruction(superinstruction)
          assert superinstruction_validated, (
              'validator rejected superinstruction allowed by spec',
              superinstruction)
        except spec.SandboxingError as e:
          assert not superinstruction_validated, (
              'validator allowed superinstruction rejected by spec',
              superinstruction,
              e)
        except spec.DoNotMatchError:
          raise
  finally:
    os.remove(object_file.name)


def main():
  parser = optparse.OptionParser(__doc__)

  parser.add_option('-b', '--bitness',
                    type=int,
                    help='The subarchitecture: 32 or 64')
  parser.add_option('-a', '--gas',
                    help='Path to GNU AS executable')
  parser.add_option('-d', '--objdump',
                    help='Path to objdump executable')
  parser.add_option('-v', '--validator_dll',
                    help='Path to librdfa_validator_dll')
  parser.add_option('-o', '--out',
                    help='Output file name (instead of sys.stdout')

  (options, args) = parser.parse_args()

  if options.bitness not in [32, 64]:
    parser.error('specify -b 32 or -b 64')

  if not (options.gas and options.objdump and options.validator_dll):
    parser.error('specify path to gas, objdump, and validator_dll')

  if options.out is not None:
    out_file = open(options.out, "w")
  else:
    out_file = sys.stdout

  validator_inst = validator.Validator(validator_dll=options.validator_dll)

  try:
    for file in args:
      ProcessSuperinstructionsFile(file,
                                   validator_inst,
                                   options.bitness,
                                   options.gas,
                                   options.objdump,
                                   out_file)
  finally:
    if out_file is not sys.stdout:
      out_file.close()


if __name__ == '__main__':
  main()
