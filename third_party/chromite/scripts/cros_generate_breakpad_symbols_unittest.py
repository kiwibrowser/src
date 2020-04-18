# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test cros_generate_breakpad_symbols."""

from __future__ import print_function

import ctypes
import mock
import os
import StringIO

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import parallel_unittest
from chromite.lib import partial_mock
from chromite.scripts import cros_generate_breakpad_symbols


class FindDebugDirMock(partial_mock.PartialMock):
  """Mock out the DebugDir helper so we can point it to a tempdir."""

  TARGET = 'chromite.scripts.cros_generate_breakpad_symbols'
  ATTRS = ('FindDebugDir',)
  DEFAULT_ATTR = 'FindDebugDir'

  def __init__(self, path, *args, **kwargs):
    self.path = path
    super(FindDebugDirMock, self).__init__(*args, **kwargs)

  def FindDebugDir(self, _board):
    return self.path


@mock.patch('chromite.scripts.cros_generate_breakpad_symbols.'
            'GenerateBreakpadSymbol')
class GenerateSymbolsTest(cros_test_lib.MockTempDirTestCase):
  """Test GenerateBreakpadSymbols."""

  def setUp(self):
    self.board = 'monkey-board'
    self.board_dir = os.path.join(self.tempdir, 'build', self.board)
    self.debug_dir = os.path.join(self.board_dir, 'usr', 'lib', 'debug')
    self.breakpad_dir = os.path.join(self.debug_dir, 'breakpad')

    # Generate a tree of files which we'll scan through.
    elf_files = [
        'bin/elf',
        'iii/large-elf',
        # Need some kernel modules (with & without matching .debug).
        'lib/modules/3.10/module.ko',
        'lib/modules/3.10/module-no-debug.ko',
        # Need a file which has an ELF only, but not a .debug.
        'usr/bin/elf-only',
        'usr/sbin/elf',
    ]
    debug_files = [
        'bin/bad-file',
        'bin/elf.debug',
        'iii/large-elf.debug',
        'lib/modules/3.10/module.ko.debug',
        # Need a file which has a .debug only, but not an ELF.
        'sbin/debug-only.debug',
        'usr/sbin/elf.debug',
    ]
    for f in ([os.path.join(self.board_dir, x) for x in elf_files] +
              [os.path.join(self.debug_dir, x) for x in debug_files]):
      osutils.Touch(f, makedirs=True)

    # Set up random build dirs and symlinks.
    buildid = os.path.join(self.debug_dir, '.build-id', '00')
    osutils.SafeMakedirs(buildid)
    os.symlink('/asdf', os.path.join(buildid, 'foo'))
    os.symlink('/bin/sh', os.path.join(buildid, 'foo.debug'))
    os.symlink('/bin/sh', os.path.join(self.debug_dir, 'file.debug'))
    osutils.WriteFile(os.path.join(self.debug_dir, 'iii', 'large-elf.debug'),
                      'just some content')

    self.StartPatcher(FindDebugDirMock(self.debug_dir))

  def testNormal(self, gen_mock):
    """Verify all the files we expect to get generated do"""
    with parallel_unittest.ParallelMock():
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
          self.board, sysroot=self.board_dir)
      self.assertEquals(ret, 0)
      self.assertEquals(gen_mock.call_count, 3)

      # The largest ELF should be processed first.
      call1 = (os.path.join(self.board_dir, 'iii/large-elf'),
               os.path.join(self.debug_dir, 'iii/large-elf.debug'))
      self.assertEquals(gen_mock.call_args_list[0][0], call1)

      # The other ELFs can be called in any order.
      call2 = (os.path.join(self.board_dir, 'bin/elf'),
               os.path.join(self.debug_dir, 'bin/elf.debug'))
      call3 = (os.path.join(self.board_dir, 'usr/sbin/elf'),
               os.path.join(self.debug_dir, 'usr/sbin/elf.debug'))
      exp_calls = set((call2, call3))
      actual_calls = set((gen_mock.call_args_list[1][0],
                          gen_mock.call_args_list[2][0]))
      self.assertEquals(exp_calls, actual_calls)

  def testFileList(self, gen_mock):
    """Verify that file_list restricts the symbols generated"""
    with parallel_unittest.ParallelMock():
      call1 = (os.path.join(self.board_dir, 'usr/sbin/elf'),
               os.path.join(self.debug_dir, 'usr/sbin/elf.debug'))

      # Filter with elf path.
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
          self.board, sysroot=self.board_dir, breakpad_dir=self.breakpad_dir,
          file_list=[os.path.join(self.board_dir, 'usr', 'sbin', 'elf')])
      self.assertEquals(ret, 0)
      self.assertEquals(gen_mock.call_count, 1)
      self.assertEquals(gen_mock.call_args_list[0][0], call1)

      # Filter with debug symbols file path.
      gen_mock.reset_mock()
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
          self.board, sysroot=self.board_dir, breakpad_dir=self.breakpad_dir,
          file_list=[os.path.join(self.debug_dir, 'usr', 'sbin', 'elf.debug')])
      self.assertEquals(ret, 0)
      self.assertEquals(gen_mock.call_count, 1)
      self.assertEquals(gen_mock.call_args_list[0][0], call1)


  def testGenLimit(self, gen_mock):
    """Verify generate_count arg works"""
    with parallel_unittest.ParallelMock():
      # Generate nothing!
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
          self.board, sysroot=self.board_dir, breakpad_dir=self.breakpad_dir,
          generate_count=0)
      self.assertEquals(ret, 0)
      self.assertEquals(gen_mock.call_count, 0)

      # Generate just one.
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
          self.board, sysroot=self.board_dir, breakpad_dir=self.breakpad_dir,
          generate_count=1)
      self.assertEquals(ret, 0)
      self.assertEquals(gen_mock.call_count, 1)

      # The largest ELF should be processed first.
      call1 = (os.path.join(self.board_dir, 'iii/large-elf'),
               os.path.join(self.debug_dir, 'iii/large-elf.debug'))
      self.assertEquals(gen_mock.call_args_list[0][0], call1)

  def testGenErrors(self, gen_mock):
    """Verify we handle errors from generation correctly"""
    def _SetError(*_args, **kwargs):
      kwargs['num_errors'].value += 1
      return 1
    gen_mock.side_effect = _SetError
    with parallel_unittest.ParallelMock():
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
          self.board, sysroot=self.board_dir)
      self.assertEquals(ret, 3)
      self.assertEquals(gen_mock.call_count, 3)

  def testCleaningTrue(self, gen_mock):
    """Verify behavior of clean_breakpad=True"""
    with parallel_unittest.ParallelMock():
      # Dir does not exist, and then does.
      self.assertNotExists(self.breakpad_dir)
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
          self.board, sysroot=self.board_dir, generate_count=1,
          clean_breakpad=True)
      self.assertEquals(ret, 0)
      self.assertEquals(gen_mock.call_count, 1)
      self.assertExists(self.breakpad_dir)

      # Dir exists before & after.
      # File exists, but then doesn't.
      dummy_file = os.path.join(self.breakpad_dir, 'fooooooooo')
      osutils.Touch(dummy_file)
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
          self.board, sysroot=self.board_dir, generate_count=1,
          clean_breakpad=True)
      self.assertEquals(ret, 0)
      self.assertEquals(gen_mock.call_count, 2)
      self.assertNotExists(dummy_file)

  def testCleaningFalse(self, gen_mock):
    """Verify behavior of clean_breakpad=False"""
    with parallel_unittest.ParallelMock():
      # Dir does not exist, and then does.
      self.assertNotExists(self.breakpad_dir)
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
          self.board, sysroot=self.board_dir, generate_count=1,
          clean_breakpad=False)
      self.assertEquals(ret, 0)
      self.assertEquals(gen_mock.call_count, 1)
      self.assertExists(self.breakpad_dir)

      # Dir exists before & after.
      # File exists before & after.
      dummy_file = os.path.join(self.breakpad_dir, 'fooooooooo')
      osutils.Touch(dummy_file)
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
          self.board, sysroot=self.board_dir, generate_count=1,
          clean_breakpad=False)
      self.assertEquals(ret, 0)
      self.assertEquals(gen_mock.call_count, 2)
      self.assertExists(dummy_file)

  def testExclusionList(self, gen_mock):
    """Verify files in directories of the exclusion list are excluded"""
    exclude_dirs = ['bin', 'usr', 'fake/dir/fake']
    with parallel_unittest.ParallelMock():
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
          self.board, sysroot=self.board_dir, exclude_dirs=exclude_dirs)
      self.assertEquals(ret, 0)
      self.assertEquals(gen_mock.call_count, 1)

class GenerateSymbolTest(cros_test_lib.RunCommandTempDirTestCase):
  """Test GenerateBreakpadSymbol."""

  def setUp(self):
    self.elf_file = os.path.join(self.tempdir, 'elf')
    osutils.Touch(self.elf_file)
    self.debug_dir = os.path.join(self.tempdir, 'debug')
    self.debug_file = os.path.join(self.debug_dir, 'elf.debug')
    osutils.Touch(self.debug_file, makedirs=True)
    # Not needed as the code itself should create it as needed.
    self.breakpad_dir = os.path.join(self.debug_dir, 'breakpad')

    self.rc.SetDefaultCmdResult(output='MODULE OS CPU ID NAME')
    self.assertCommandContains = self.rc.assertCommandContains
    self.sym_file = os.path.join(self.breakpad_dir, 'NAME/ID/NAME.sym')

    self.StartPatcher(FindDebugDirMock(self.debug_dir))

  def assertCommandArgs(self, i, args):
    """Helper for looking at the args of the |i|th call"""
    self.assertEqual(self.rc.call_args_list[i][0][0], args)

  def testNormal(self):
    """Normal run -- given an ELF and a debug file"""
    ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbol(
        self.elf_file, self.debug_file, self.breakpad_dir)
    self.assertEqual(ret, self.sym_file)
    self.assertEqual(self.rc.call_count, 1)
    self.assertCommandArgs(0, ['dump_syms', '-v', self.elf_file,
                               self.debug_dir])
    self.assertExists(self.sym_file)

  def testNormalNoCfi(self):
    """Normal run w/out CFI"""
    # Make sure the num_errors flag works too.
    num_errors = ctypes.c_int()
    ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbol(
        self.elf_file, breakpad_dir=self.breakpad_dir,
        strip_cfi=True, num_errors=num_errors)
    self.assertEqual(ret, self.sym_file)
    self.assertEqual(num_errors.value, 0)
    self.assertCommandArgs(0, ['dump_syms', '-v', '-c', self.elf_file])
    self.assertEqual(self.rc.call_count, 1)
    self.assertExists(self.sym_file)

  def testNormalElfOnly(self):
    """Normal run -- given just an ELF"""
    ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbol(
        self.elf_file, breakpad_dir=self.breakpad_dir)
    self.assertEqual(ret, self.sym_file)
    self.assertCommandArgs(0, ['dump_syms', '-v', self.elf_file])
    self.assertEqual(self.rc.call_count, 1)
    self.assertExists(self.sym_file)

  def testNormalSudo(self):
    """Normal run where ELF is readable only by root"""
    with mock.patch.object(os, 'access') as mock_access:
      mock_access.return_value = False
      ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbol(
          self.elf_file, breakpad_dir=self.breakpad_dir)
    self.assertEqual(ret, self.sym_file)
    self.assertCommandArgs(0, ['sudo', '--', 'dump_syms', '-v', self.elf_file])

  def testLargeDebugFail(self):
    """Running w/large .debug failed, but retry worked"""
    self.rc.AddCmdResult(['dump_syms', '-v', self.elf_file, self.debug_dir],
                         returncode=1)
    ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbol(
        self.elf_file, self.debug_file, self.breakpad_dir)
    self.assertEqual(ret, self.sym_file)
    self.assertEqual(self.rc.call_count, 2)
    self.assertCommandArgs(0, ['dump_syms', '-v', self.elf_file,
                               self.debug_dir])
    self.assertCommandArgs(
        1, ['dump_syms', '-v', '-c', '-r', self.elf_file, self.debug_dir])
    self.assertExists(self.sym_file)

  def testDebugFail(self):
    """Running w/.debug always failed, but works w/out"""
    self.rc.AddCmdResult(['dump_syms', '-v', self.elf_file, self.debug_dir],
                         returncode=1)
    self.rc.AddCmdResult(['dump_syms', '-v', '-c', '-r', self.elf_file,
                          self.debug_dir],
                         returncode=1)
    ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbol(
        self.elf_file, self.debug_file, self.breakpad_dir)
    self.assertEqual(ret, self.sym_file)
    self.assertEqual(self.rc.call_count, 3)
    self.assertCommandArgs(0, ['dump_syms', '-v', self.elf_file,
                               self.debug_dir])
    self.assertCommandArgs(
        1, ['dump_syms', '-v', '-c', '-r', self.elf_file, self.debug_dir])
    self.assertCommandArgs(2, ['dump_syms', '-v', self.elf_file])
    self.assertExists(self.sym_file)

  def testCompleteFail(self):
    """Running dump_syms always fails"""
    self.rc.SetDefaultCmdResult(returncode=1)
    ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbol(
        self.elf_file, breakpad_dir=self.breakpad_dir)
    self.assertEqual(ret, 1)
    # Make sure the num_errors flag works too.
    num_errors = ctypes.c_int()
    ret = cros_generate_breakpad_symbols.GenerateBreakpadSymbol(
        self.elf_file, breakpad_dir=self.breakpad_dir, num_errors=num_errors)
    self.assertEqual(ret, 1)
    self.assertEqual(num_errors.value, 1)


class UtilsTestDir(cros_test_lib.TempDirTestCase):
  """Tests ReadSymsHeader."""

  def testReadSymsHeaderGoodFile(self):
    """Make sure ReadSymsHeader can parse sym files"""
    sym_file = os.path.join(self.tempdir, 'sym')
    osutils.WriteFile(sym_file, 'MODULE Linux x86 s0m31D chrooome')
    result = cros_generate_breakpad_symbols.ReadSymsHeader(sym_file)
    self.assertEquals(result.cpu, 'x86')
    self.assertEquals(result.id, 's0m31D')
    self.assertEquals(result.name, 'chrooome')
    self.assertEquals(result.os, 'Linux')


class UtilsTest(cros_test_lib.TestCase):
  """Tests ReadSymsHeader."""

  def testReadSymsHeaderGoodBuffer(self):
    """Make sure ReadSymsHeader can parse sym file handles"""
    result = cros_generate_breakpad_symbols.ReadSymsHeader(
        StringIO.StringIO('MODULE Linux arm MY-ID-HERE blkid'))
    self.assertEquals(result.cpu, 'arm')
    self.assertEquals(result.id, 'MY-ID-HERE')
    self.assertEquals(result.name, 'blkid')
    self.assertEquals(result.os, 'Linux')

  def testReadSymsHeaderBadd(self):
    """Make sure ReadSymsHeader throws on bad sym files"""
    self.assertRaises(ValueError, cros_generate_breakpad_symbols.ReadSymsHeader,
                      StringIO.StringIO('asdf'))

  def testBreakpadDir(self):
    """Make sure board->breakpad path expansion works"""
    expected = '/build/blah/usr/lib/debug/breakpad'
    result = cros_generate_breakpad_symbols.FindBreakpadDir('blah')
    self.assertEquals(expected, result)

  def testDebugDir(self):
    """Make sure board->debug path expansion works"""
    expected = '/build/blah/usr/lib/debug'
    result = cros_generate_breakpad_symbols.FindDebugDir('blah')
    self.assertEquals(expected, result)


def main(_argv):
  # pylint: disable=W0212
  # Set timeouts small so that if the unit test hangs, it won't hang for long.
  parallel._BackgroundTask.STARTUP_TIMEOUT = 5
  parallel._BackgroundTask.EXIT_TIMEOUT = 5

  # Run the tests.
  cros_test_lib.main(level='info', module=__name__)
