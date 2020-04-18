#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from driver_env import env
import driver_test_utils
import driver_tools
import elftools
import filetype
import re

class TestNativeDriverOptions(driver_test_utils.DriverTesterCommon):
  def setUp(self):
    super(TestNativeDriverOptions, self).setUp()
    driver_test_utils.ApplyTestEnvOverrides(env)
    if not driver_test_utils.CanRunHost():
      self.skipTest("Cannot run host binaries")

  def getFakeSourceFile(self):
    with self.getTemp(suffix='.c', close=False) as s:
      s.write('void _start() {}')
      s.close()
      return s

  def getBitcodeArch(self, filename):
    with self.getTemp(suffix='.ll') as ll:
      driver_tools.RunDriver('pnacl-dis', [filename, '-o', ll.name])
      with open(ll.name) as f:
        disassembly = f.read()
        match = re.search(r'target triple = "(.+)"', disassembly)
        if not match:
          return None
        triple = match.group(1)
        return driver_tools.ParseTriple(triple)

  def test_bc_objects(self):
    s = self.getFakeSourceFile()
    with self.getTemp(suffix='.o') as obj:
      # Test that clang with "normal" args results in a portable bitcode object
      driver_tools.RunDriver('pnacl-clang', [s.name, '-c', '-o', obj.name])
      self.assertTrue(filetype.IsLLVMBitcode(obj.name))
      self.assertEqual(self.getBitcodeArch(obj.name), 'le32')

      # Test that the --target flag produces biased bitcode objects
      test_args = [
          ('armv7',  'ARM',    '-gnueabihf', ['-mfloat-abi=hard']),
          ('i686',   'X8632',  '',           []),
          ('x86_64', 'X8664',  '',           []),
          ('mips',   'MIPS32', '',           []),
      ]
      for (target, arch, target_extra, cmd_extra) in test_args:
        target_arg = '--target=%s-unknown-nacl%s' % (target, target_extra)
        driver_tools.RunDriver('pnacl-clang',
          [s.name, target_arg, '-c', '-o', obj.name] + cmd_extra)
        self.assertTrue(filetype.IsLLVMBitcode(obj.name))
        self.assertEqual(self.getBitcodeArch(obj.name), arch)

  def test_compile_native_objects(self):
    s = self.getFakeSourceFile()
    with self.getTemp(suffix='.o') as obj:
      # TODO(dschuff): Use something more descriptive instead of -arch
      # (i.e. something that indicates that a translation is requested)
      # and remove pnacl-allow-translate
      driver_tools.RunDriver('pnacl-clang',
          [s.name, '-c', '-o', obj.name, '--target=x86_64-unknown-nacl',
           '-arch', 'x86-64', '--pnacl-allow-translate'])
      self.assertTrue(filetype.IsNativeObject(obj.name))
      ehdr = elftools.GetELFHeader(obj.name)
      self.assertEqual(ehdr.arch, 'X8664')
      # ET_REL does not have program headers.
      self.assertEqual(ehdr.phnum, 0)

      driver_tools.RunDriver('pnacl-clang',
          [s.name, '-c', '-o', obj.name, '--target=i686-unknown-nacl',
           '-arch', 'x86-32', '--pnacl-allow-translate'])
      self.assertTrue(filetype.IsNativeObject(obj.name))
      self.assertEqual(elftools.GetELFHeader(obj.name).arch, 'X8632')

      driver_tools.RunDriver('pnacl-clang',
          [s.name, '-c', '-o', obj.name,
           '--target=armv7-unknown-nacl-gnueabihf', '-mfloat-abi=hard',
           '-arch', 'arm', '--pnacl-allow-translate'])
      self.assertTrue(filetype.IsNativeObject(obj.name))
      self.assertEqual(elftools.GetELFHeader(obj.name).arch, 'ARM')

      # TODO(dschuff): This should be an error.
      driver_tools.RunDriver('pnacl-clang',
          [s.name, '-c', '-o', obj.name, '--target=x86_64-unknown-nacl',
           '-arch', 'x86-32', '--pnacl-allow-translate'])
      self.assertTrue(filetype.IsNativeObject(obj.name))
      self.assertEqual(elftools.GetELFHeader(obj.name).arch, 'X8632')

  def test_compile_native_executables(self):
    s = self.getFakeSourceFile()
    with self.getTemp(suffix='.nexe') as obj:
      driver_tools.RunDriver('pnacl-clang',
          [s.name, '-nostdlib', '-o', obj.name, '-arch', 'x86-64'])
      # This test is a sanity check that the ELF header parsing code
      # works. If the ELF header parsing code were no longer needed
      # for other purposes, this test could be removed.
      ehdr, phdrs = elftools.GetELFAndProgramHeaders(obj.name)
      self.assertEqual(ehdr.arch, 'X8664')
      self.assertNotEqual(ehdr.phnum, 0)
      self.assertEqual(ehdr.phentsize, 56)
      self.assertNotEqual(len(phdrs), 0)

      phdr_type_nums = {}
      for phdr in phdrs:
        phdr_type_nums[phdr.type] = phdr_type_nums.get(phdr.type, 0) + 1
        if phdr.type == elftools.ProgramHeader.PT_LOAD:
          self.assertEqual(phdr.align, 0x10000)
          self.assertIn(phdr.flags, xrange(8))
      self.assertTrue(phdr_type_nums[elftools.ProgramHeader.PT_LOAD] >= 2)

  def test_compile_nonsfi_executables(self):
    s = self.getFakeSourceFile()
    with self.getTemp(suffix='.nexe') as obj:
      driver_tools.RunDriver('pnacl-clang',
          [s.name, '-nostdlib', '-o', obj.name, '-arch', 'x86-32-nonsfi'])
      ehdr, phdrs = elftools.GetELFAndProgramHeaders(obj.name)
      self.assertEqual(ehdr.arch, 'X8632')
      self.assertNotEqual(ehdr.phnum, 0)
      self.assertEqual(ehdr.phentsize, 32)
      self.assertNotEqual(len(phdrs), 0)

      phdr_type_nums = {}
      for phdr in phdrs:
        phdr_type_nums[phdr.type] = phdr_type_nums.get(phdr.type, 0) + 1
        if phdr.type == elftools.ProgramHeader.PT_LOAD:
          self.assertIn(phdr.flags, xrange(8))
      self.assertTrue(phdr_type_nums[elftools.ProgramHeader.PT_LOAD] >= 2)
      # Check that PT_INTERP was replaced by PT_NULL.
      self.assertNotIn(elftools.ProgramHeader.PT_INTERP, phdr_type_nums)
      self.assertEqual(phdr_type_nums[elftools.ProgramHeader.PT_NULL], 1)


if __name__ == '__main__':
  unittest.main()
