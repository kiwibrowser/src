#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import unittest

import proof_tools
import spec


class ProofToolsTest(unittest.TestCase):

  def testOpsMerge(self):
    op1 = proof_tools.Operands(disasms=('0x0(%r13)',), input_rr='%r13',
                               output_rr=None)
    op2 = proof_tools.Operands(disasms=('0x0(%r14)',), input_rr='%r14',
                               output_rr=None)
    op3 = proof_tools.Operands(disasms=('%r12',),
                               input_rr=None, output_rr='%r12')
    self.assertEquals(
        proof_tools.Operands(disasms=('0x0(%r13)', '%r12'),
                             input_rr='%r13', output_rr='%r12'),
        proof_tools.MergeOperands(op1, op3))

    self.assertEquals(
        proof_tools.Operands(disasms=('0x0(%r14)', '%r12'),
                             input_rr='%r14', output_rr='%r12'),
        proof_tools.MergeOperands(op2, op3))

    try:
      proof_tools.MergeOperands(op1, op2)
    except AssertionError:
      pass
    else:
      self.fail('Should have thrown exception as restrictions conflict')

  def testOpsProd(self):
    ops1 = set(
        [proof_tools.Operands(disasms=('a',)),
         proof_tools.Operands(disasms=('b',))])

    ops2 = set(
        [proof_tools.Operands(disasms=('1',)),
         proof_tools.Operands(disasms=('2',))])

    ops3 = set(
        [proof_tools.Operands(disasms=('i',)),
         proof_tools.Operands(disasms=('ii',))])

    self.assertEquals(
        set([proof_tools.Operands(disasms=('a', '1', 'i')),
             proof_tools.Operands(disasms=('a', '1', 'ii')),
             proof_tools.Operands(disasms=('a', '2', 'i')),
             proof_tools.Operands(disasms=('a', '2', 'ii')),
             proof_tools.Operands(disasms=('b', '1', 'i')),
             proof_tools.Operands(disasms=('b', '1', 'ii')),
             proof_tools.Operands(disasms=('b', '2', 'i')),
             proof_tools.Operands(disasms=('b', '2', 'ii'))]),
        proof_tools.OpsProd(ops1, ops2, ops3))

  def testMemoryOperandsTemplate32(self):
    mem = proof_tools.MemoryOperandsTemplate(
        disp='0x0', base='%ebx', index='%eax', scale=2, bitness=32)
    self.assertEquals(
        [proof_tools.Operands(disasms=('(%ebx)',)),
         proof_tools.Operands(disasms=('0x0',)),
         proof_tools.Operands(disasms=('(%ebx,%eax,2)',)),
         proof_tools.Operands(disasms=('0x0(,%eax,2)',)),
         proof_tools.Operands(disasms=('0x0(%ebx)',)),
         proof_tools.Operands(disasms=('0x0(%ebx,%eax,2)',))],
        mem)

  def testMemoryOperandsTemplate64(self):
    mem = proof_tools.MemoryOperandsTemplate(
        disp='0x0', base='%rsp', index='%r8', scale=2, bitness=64)
    self.assertEquals(
        [proof_tools.Operands(disasms=('(%rsp)',)),
         proof_tools.Operands(disasms=('0x0(%rsp)',)),
         proof_tools.Operands(disasms=('(%rsp,%r8,2)',), input_rr='%r8'),
         proof_tools.Operands(disasms=('0x0(%rsp,%r8,2)',), input_rr='%r8')],
        mem)

  def testAllMemoryOps32(self):
    mems = proof_tools.AllMemoryOperands(bitness=32)
    indexes = set()
    bases = set()
    for mem in mems:
      self.assertTrue(mem.input_rr is None, mem)
      self.assertTrue(mem.output_rr is None, mem)
      self.assertEquals(len(mem.disasms), 1)
      m = re.match(spec.MemoryRE() + r'$', mem.disasms[0])
      self.assertTrue(m is not None, msg=mem.disasms[0])
      self.assertTrue(m.group('memory_segment') is None, msg=mem.disasms[0])
      base = m.group('memory_base')
      index = m.group('memory_index')
      if base is not None:
        bases.add(base)
      if index is not None:
        indexes.add(index)

    self.assertEquals(
        set(['%ebp', '%eax', '%edi', '%ebx', '%esi', '%ecx', '%edx', '%eiz']),
        indexes)
    self.assertEquals(
        set(['%ebp', '%eax', '%edi', '%ebx', '%esi', '%ecx', '%edx', '%esp']),
        bases)

  def testAllMemoryOps64(self):
    mems = proof_tools.AllMemoryOperands(bitness=64)
    indexes = set()
    bases = set()
    for mem in mems:
      self.assertTrue(mem.output_rr is None, mem)
      self.assertEquals(len(mem.disasms), 1)
      m = re.match(spec.MemoryRE() + r'$', mem.disasms[0])
      self.assertTrue(m is not None, msg=mem.disasms[0])
      self.assertTrue(m.group('memory_segment') is None, msg=mem.disasms[0])
      base = m.group('memory_base')
      index = m.group('memory_index')
      if base is not None:
        bases.add(base)
      if index is not None and index != '%riz':
        indexes.add(index)
        self.assertEquals(mem.input_rr, index)

    self.assertEquals(
        set(['%rax', '%rbx', '%rcx', '%rdx', '%rsi', '%rdi',
             '%r8', '%r9', '%r10', '%r11', '%r12', '%r13', '%r14', '%r15']),
        indexes)

    self.assertEquals(
        set(['%rsp', '%r15', '%rbp', '%rip']),
        bases)

if __name__ == '__main__':
  unittest.main()

