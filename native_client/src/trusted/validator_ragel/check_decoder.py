#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Generate all acceptable sequences from acyclic DFA, run objdump and
decoder on them and compare results.
"""

import itertools
import multiprocessing
import optparse
import os
import re
import subprocess
import tempfile

import dfa_parser
import dfa_traversal
import objdump_parser


FWAIT = 0x9b
NOP = 0x90


def IsRexPrefix(byte):
  return 0x40 <= byte < 0x50


def LineRoughlyEqual(s1, s2):
  """Check whether disasms lines are equal ignoring whitespaces and comments."""
  # strip comments.
  # These are mainly used by objdump to print out the absolute address
  # for rip relative jumps.
  # e.g.
  # 0x400408: ff 35 e2 0b 20 00     pushq  0x200be2(%rip)        # 0x600ff0
  s1, _, _ = s1.partition('#')
  s2, _, _ = s2.partition('#')
  # compare ignoring whitespace.
  result = s1.split() == s2.split()
  return result


class WorkerState(object):
  __slots__ = [
      'total_instructions',
      '_num_instructions',
      '_asm_file',
      '_file_prefix',
  ]

  def __init__(self, prefix):
    self._file_prefix = 'check_decoder_%s_' % '_'.join(map(hex, prefix))
    self.total_instructions = 0

    self._StartNewFile()

  def _StartNewFile(self):
    self._asm_file = tempfile.NamedTemporaryFile(
        mode='wt',
        prefix=self._file_prefix,
        suffix='.s',
        delete=False)
    self._asm_file.write('.text\n')
    self._num_instructions = 0

  def ReceiveInstruction(self, bytes):
    # We do not disassemble x87 instructions prefixed with fwait as objdump
    # does. To avoid such situations in enumeration test, we insert nop after
    # fwait.
    if (bytes == [FWAIT] or
        len(bytes) == 2 and IsRexPrefix(bytes[0]) and bytes[1] == FWAIT):
      bytes = bytes + [NOP]
    self._asm_file.write('  .byte %s\n' % ','.join(map(hex, bytes)))
    self.total_instructions += 1
    self._num_instructions += 1
    if self._num_instructions == 2000000:
      self.Finish()
      self._StartNewFile()

  def _CheckFile(self):
    try:
      object_file = tempfile.NamedTemporaryFile(
          prefix=self._file_prefix,
          suffix='.o',
          delete=False)
      object_file.close()
      subprocess.check_call([
          options.gas,
          '--%s' % options.bits,
          '--strip-local-absolute',
          self._asm_file.name,
          '-o', object_file.name])

      objdump_proc = subprocess.Popen(
          [options.objdump, '-d', '--insn-width=15', object_file.name],
          stdout=subprocess.PIPE)

      decoder_proc = subprocess.Popen(
          [options.decoder, object_file.name],
          stdout=subprocess.PIPE)

      for line1, line2 in itertools.izip_longest(
          objdump_parser.SkipHeader(objdump_proc.stdout),
          decoder_proc.stdout,
          fillvalue=None):

        if not LineRoughlyEqual(line1, line2):
          print 'objdump: %r' % line1
          print 'decoder: %r' % line2
          raise AssertionError('%r != %r' % (line1, line2))

      return_code = objdump_proc.wait()
      assert return_code == 0

      return_code = decoder_proc.wait()
      assert return_code == 0

    finally:
      os.remove(self._asm_file.name)
      os.remove(object_file.name)

  def Finish(self):
    self._asm_file.close()
    self._CheckFile()


def Worker((prefix, state_index)):
  worker_state = WorkerState(prefix)

  try:
    dfa_traversal.TraverseTree(
        dfa.states[state_index],
        final_callback=worker_state.ReceiveInstruction,
        prefix=prefix)
  finally:
    worker_state.Finish()

  return prefix, worker_state.total_instructions


def ParseOptions():
  parser = optparse.OptionParser(
      usage='%prog [options] xmlfile',
      description=__doc__)
  parser.add_option('--bits',
                    type=int,
                    help='The subarchitecture: 32 or 64')
  parser.add_option('--gas',
                    help='Path to GNU AS executable')
  parser.add_option('--objdump',
                    help='Path to objdump executable')
  parser.add_option('--decoder',
                    help='Path to decoder executable')
  options, args = parser.parse_args()
  if options.bits not in [32, 64]:
    parser.error('specify --bits 32 or --bits 64')
  if not (options.gas and options.objdump and options.decoder):
    parser.error('specify path to gas, objdump and decoder')

  if len(args) != 1:
    parser.error('specify one xml file')

  (xml_file,) = args

  return options, xml_file


options, xml_file = ParseOptions()
# We are doing it here to share state graph between workers spawned by
# multiprocess. Passing it every time is slow.
dfa = dfa_parser.ParseXml(xml_file)


def main():
  # Decoder is supposed to have one accepting state.
  accepting_states = [state for state in dfa.states if state.is_accepting]
  assert accepting_states == [dfa.initial_state]

  assert not dfa.initial_state.any_byte

  num_suffixes = dfa_traversal.GetNumSuffixes(dfa.initial_state)
  # We can't just write 'num_suffixes[dfa.initial_state]' because
  # initial state is accepting.
  total_instructions = sum(
      num_suffixes[t.to_state]
      for t in dfa.initial_state.forward_transitions.values())
  print total_instructions, 'instructions total'

  tasks = dfa_traversal.CreateTraversalTasks(dfa.states, dfa.initial_state)
  print len(tasks), 'tasks'

  pool = multiprocessing.Pool()

  results = pool.imap(Worker, tasks)

  total = 0
  for prefix, count in results:
    print ', '.join(map(hex, prefix))
    total += count

  print total, 'instructions were processed'


if __name__ == '__main__':
  main()
