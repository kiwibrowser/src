#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Generate all acceptable regular instructions by traversing validator DFA
and run check them against RDFA decoder and text-based specification.
"""

import itertools
import multiprocessing
import optparse
import os
import re
import subprocess
import sys
import tempfile
import traceback

import dfa_parser
import dfa_traversal
import objdump_parser
import validator
import spec


FWAIT = 0x9b
NOP = 0x90


def IsRexPrefix(byte):
  return 0x40 <= byte < 0x50


def PadToBundleSize(bytes):
  assert len(bytes) <= validator.BUNDLE_SIZE
  return bytes + [NOP] * (validator.BUNDLE_SIZE - len(bytes))


def ConditionToRestrictedRegisterNumber(condition):
  restricted, restricted_instead_of_sandboxed = condition.GetAlteredRegisters()
  if restricted is not None:
    assert restricted_instead_of_sandboxed is None
    return validator.REGISTER_BY_NAME[restricted]
  elif restricted_instead_of_sandboxed is not None:
    return validator.REGISTER_BY_NAME[restricted_instead_of_sandboxed]
  else:
    return None


def RestrictedRegisterNumberToCondition(rr):
  assert rr is None or rr in validator.ALL_REGISTERS, rr
  if rr is None:
    return spec.Condition()
  elif rr == validator.NC_REG_RBP:
    return spec.Condition(restricted_instead_of_sandboxed='%rbp')
  elif rr == validator.NC_REG_RSP:
    return spec.Condition(restricted_instead_of_sandboxed='%rsp')
  else:
    return spec.Condition(validator.REGISTER_NAMES[rr])


def ValidateAndGetPostcondition(
    bundle, actual_size, precondition, validator_inst):
  """Validate single x86-64 instruction and get postcondition for it.

  Args:
    bundle: sequence of bytes (as python string)
    actual_size: size of the instruction in the beginning of the bundle
                 (remaining bytes are expected to be NOPS)
    precondition: instance of spec.Condition representing precondition validator
    is allowed to assume.
    validator_inst: implementation of validator.
  Returns:
    Pair (validation_result, postcondition).
    Validation_result is True or False. When validation_result is True,
    postcondition is spec.Condition instance representing postcondition
    for this instruction established by validator.
  """

  valid, final_rr = validator_inst.ValidateAndGetFinalRestrictedRegister(
      bundle,
      actual_size,
      initial_rr=ConditionToRestrictedRegisterNumber(precondition))

  if valid:
    return True, RestrictedRegisterNumberToCondition(final_rr)
  else:
    return False, None


def CheckValid64bitInstruction(
    instruction,
    dis,
    precondition,
    postcondition,
    validator_inst):
  bundle = ''.join(map(chr, PadToBundleSize(instruction)))
  for conflicting_precondition in spec.Condition.All():
    if conflicting_precondition.Implies(precondition):
      continue  # not conflicting
    result, _ = ValidateAndGetPostcondition(
        bundle, len(instruction), conflicting_precondition, validator_inst)
    assert not result, (
        'validator incorrectly accepted %s with precondition %s, '
        'while specification requires precondition %s'
        % (dis, conflicting_precondition, precondition))

  result, actual_postcondition = ValidateAndGetPostcondition(
      bundle, len(instruction), precondition, validator_inst)
  if not result:
    print 'warning: validator rejected %s with precondition %s' % (
        dis, precondition)
  else:
    # We are checking for implication, not for equality, because in some cases
    # specification computes postcondition with better precision.
    # For example, xchg with memory is not treated as zero-extending
    # instruction, so validator thinks that postcondition of
    #   xchg %eax, (%rip)
    # is Condition(default), while in fact it's Condition(%rax is restricted).
    # (https://code.google.com/p/nativeclient/issues/detail?id=3071)
    # TODO(shcherbina): change it to equality test when such cases
    # are eliminated.
    assert postcondition.Implies(actual_postcondition), (
        'validator incorrectly determined postcondition %s '
        'for %s where specification predicted %s'
        % (actual_postcondition, dis, postcondition))
    if postcondition != actual_postcondition:
      print (
          'warning: validator reported too broad postcondition %s for %s, '
          'where specification predicted %s'
          % (actual_postcondition, dis, postcondition))


def CheckInvalid64bitInstruction(
    instruction,
    dis,
    validator_inst,
    sandboxing_error):
  bundle = ''.join(map(chr, PadToBundleSize(instruction)))
  for precondition in spec.Condition.All():
    result, _ = ValidateAndGetPostcondition(
        bundle, len(instruction), precondition, validator_inst)
    assert not result, (
        'validator incorrectly accepted %s with precondition %s, '
        'while specification rejected because %s'
        % (dis, precondition, sandboxing_error))


def ValidateInstruction(instruction, validator_inst):
  dis = validator_inst.DisassembleChunk(
      ''.join(map(chr, instruction)),
      bitness=options.bitness)

  # Objdump 2.24 (and consequently our decoder) displays fwait with rex prefix
  # in  the following way (note the rex byte is extraneous here):
  #   0: 41      rex.B
  #   1: 9b      fwait
  # We manually convert it to
  #   0: 41 9b   fwait
  # for the purpose of validation.
  # TODO(shyamsundarr): investigate whether we can get rid of this special
  # handling. Also https://code.google.com/p/nativeclient/issues/detail?id=3496.
  if (len(instruction) == 2 and
      IsRexPrefix(instruction[0]) and
      instruction[1] == FWAIT):
    assert len(dis) == 2, (instruction, dis)
    assert dis[1].disasm == 'fwait', (instruction, dis)
    dis[0] = objdump_parser.Instruction(
        address=dis[0].address, bytes=(dis[0].bytes + dis[1].bytes),
        disasm=dis[1].disasm)
    del dis[1]

  assert len(dis) == 1, (instruction, dis)
  (dis,) = dis
  assert dis.bytes == instruction, (instruction, dis)

  if options.bitness == 32:
    result = validator_inst.ValidateChunk(
        ''.join(map(chr, PadToBundleSize(instruction))),
        bitness=32)

    try:
      spec.ValidateDirectJumpOrRegularInstruction(dis, bitness=32)
      if not result:
        print 'warning: validator rejected', dis
      return True
    except spec.SandboxingError as e:
      if result:
        print 'validator accepted instruction disallowed by specification'
        raise
    except spec.DoNotMatchError:
      if result:
        print 'validator accepted instruction not recognized by specification'
        raise

    return False

  else:
    try:
      _, precondition, postcondition = (
          spec.ValidateDirectJumpOrRegularInstruction(dis, bitness=64))

      CheckValid64bitInstruction(instruction,
                                 dis,
                                 precondition,
                                 postcondition,
                                 validator_inst)
      return True
    except spec.SandboxingError as e:
      CheckInvalid64bitInstruction(instruction,
                                   dis,
                                   validator_inst,
                                   sandboxing_error=e)
    except spec.DoNotMatchError as e:
      CheckInvalid64bitInstruction(
          instruction,
          dis,
          validator_inst,
          sandboxing_error=spec.SandboxingError(
              'unrecognized instruction %s' % e))

    return False


class WorkerState(object):
  def __init__(self, prefix, validator_inst):
    self.total_instructions = 0
    self.num_valid = 0
    self.validator_inst = validator_inst

  def ReceiveInstruction(self, bytes):
    self.total_instructions += 1
    self.num_valid += ValidateInstruction(bytes, self.validator_inst)


def Worker((prefix, state_index)):
  worker_state = WorkerState(prefix, worker_validator)

  try:
    dfa_traversal.TraverseTree(
        dfa.states[state_index],
        final_callback=worker_state.ReceiveInstruction,
        prefix=prefix,
        anyfield=0)
  except Exception as e:
    traceback.print_exc() # because multiprocessing imap swallows traceback
    raise

  return (
      prefix,
      worker_state.total_instructions,
      worker_state.num_valid)


def ParseOptions():
  parser = optparse.OptionParser(usage='%prog [options] xmlfile')

  parser.add_option('--bitness',
                    type=int,
                    help='The subarchitecture: 32 or 64')
  parser.add_option('--validator_dll',
                    help='Path to librdfa_validator_dll')
  parser.add_option('--decoder_dll',
                    help='Path to librdfa_decoder_dll')

  options, args = parser.parse_args()

  if options.bitness not in [32, 64]:
    parser.error('specify -b 32 or -b 64')

  if len(args) != 1:
    parser.error('specify one xml file')

  (xml_file,) = args

  return options, xml_file


options, xml_file = ParseOptions()
# We are doing it here to share state graph between workers spawned by
# multiprocess. Passing it every time is slow.
dfa = dfa_parser.ParseXml(xml_file)
worker_validator = validator.Validator(
    validator_dll=options.validator_dll,
    decoder_dll=options.decoder_dll)


def main():
  assert dfa.initial_state.is_accepting
  assert not dfa.initial_state.any_byte

  print len(dfa.states), 'states'

  num_suffixes = dfa_traversal.GetNumSuffixes(dfa.initial_state)

  # We can't just write 'num_suffixes[dfa.initial_state]' because
  # initial state is accepting.
  total_instructions = sum(
      num_suffixes[t.to_state]
      for t in dfa.initial_state.forward_transitions.values())
  print total_instructions, 'regular instructions total'

  tasks = dfa_traversal.CreateTraversalTasks(dfa.states, dfa.initial_state)
  print len(tasks), 'tasks'

  pool = multiprocessing.Pool()

  results = pool.imap(Worker, tasks)

  total = 0
  num_valid = 0
  for prefix, count, valid_count in results:
    print ', '.join(map(hex, prefix))
    total += count
    num_valid += valid_count

  print total, 'instructions were processed'
  print num_valid, 'valid instructions'


if __name__ == '__main__':
  main()
