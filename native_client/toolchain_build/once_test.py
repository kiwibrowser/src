#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of a network memoizer."""

import os
import subprocess
import shutil
import sys
import unittest

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.directory_storage
import pynacl.fake_storage
import pynacl.file_tools
import pynacl.gsd_storage
import pynacl.working_directory

import command
import once


class TestOnce(unittest.TestCase):

  def GenerateTestData(self, noise, work_dir):
    self._input_dirs = {}
    self._input_files = []
    for i in range(2):
      dir_name = os.path.join(work_dir, noise + 'input%d_dir' % i)
      os.mkdir(dir_name)
      filename = os.path.join(dir_name, 'in%d' % i)
      pynacl.file_tools.WriteFile(noise + 'data%d' % i, filename)
      self._input_dirs['input%d' % i] = dir_name
      self._input_files.append(filename)
    self._output_dirs = []
    self._output_files = []
    for i in range(2):
      dir_name = os.path.join(work_dir, noise + 'output%d_dir' % i)
      os.mkdir(dir_name)
      filename = os.path.join(dir_name, 'out')
      self._output_dirs.append(dir_name)
      self._output_files.append(filename)

  def test_FirstTime(self):
    # Test that the computation is always performed if the cache is empty.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('FirstTime', work_dir)
      o = once.Once(storage=pynacl.fake_storage.FakeStorage(),
                    system_summary='test')
      o.Run('test', self._input_dirs, self._output_dirs[0],
            [command.Copy('%(input0)s/in0', '%(output)s/out')])
      self.assertEquals('FirstTimedata0',
                        pynacl.file_tools.ReadFile(self._output_files[0]))

  def test_HitsCacheSecondTime(self):
    # Test that the computation is not performed on a second instance.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('HitsCacheSecondTime', work_dir)
      self._tally = 0
      def Copy(logger, subst, src, dst):
        self._tally += 1
        shutil.copyfile(subst.SubstituteAbsPaths(src),
                        subst.SubstituteAbsPaths(dst))
      self._url = None
      def stash_url(urls):
        self._url = urls
      o = once.Once(storage=pynacl.fake_storage.FakeStorage(),
                    print_url=stash_url, system_summary='test')
      o.Run('test', self._input_dirs, self._output_dirs[0],
            [command.Runnable(None, Copy,'%(input0)s/in0', '%(output)s/out')])
      initial_url = self._url
      self._url = None
      o.Run('test', self._input_dirs, self._output_dirs[1],
            [command.Runnable(None, Copy,'%(input0)s/in0', '%(output)s/out')])
      self.assertEquals(pynacl.file_tools.ReadFile(self._input_files[0]),
                        pynacl.file_tools.ReadFile(self._output_files[0]))
      self.assertEquals(pynacl.file_tools.ReadFile(self._input_files[0]),
                        pynacl.file_tools.ReadFile(self._output_files[1]))
      self.assertEquals(1, self._tally)
      self.assertEquals(initial_url, self._url)

  def test_CachedCommandRecorded(self):
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('CachedCommand', work_dir)
      o = once.Once(storage=pynacl.fake_storage.FakeStorage(),
                    system_summary='test')
      o.Run('test', self._input_dirs, self._output_dirs[0],
            [command.Copy('%(input0)s/in0', '%(output)s/out')])
      self.assertEquals(len(o.GetCachedCloudItems()), 1)

  def test_UncachedCommandsNotRecorded(self):
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('CachedCommand', work_dir)
      o = once.Once(storage=pynacl.fake_storage.FakeStorage(),
                    system_summary='test', cache_results=False)
      o.Run('test', self._input_dirs, self._output_dirs[0],
            [command.Copy('%(input0)s/in0', '%(output)s/out')])
      self.assertEquals(len(o.GetCachedCloudItems()), 0)

  def FileLength(self, src, dst, **kwargs):
    """Command object to write the length of one file into another."""
    return command.Command([
        sys.executable, '-c',
          'import sys; open(sys.argv[2], "wb").write('
          'str(len(open(sys.argv[1], "rb").read())))', src, dst
        ], **kwargs)

  def test_RecomputeHashMatches(self):
    # Test that things don't get stored to the output cache if they exist
    # already.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      # Setup test data in input0, input1 using memory storage.
      self.GenerateTestData('RecomputeHashMatches', work_dir)
      fs = pynacl.fake_storage.FakeStorage()
      o = once.Once(storage=fs, system_summary='test')

      # Run the computation (compute the length of a file) from input0 to
      # output0.
      o.Run('test', self._input_dirs, self._output_dirs[0],
            [self.FileLength(
                '%(input0)s/in0', '%(output)s/out')])

      # Check that 3 writes have occurred. One to write a mapping from in->out,
      # one for the output data, and one for the log file.
      self.assertEquals(3, fs.WriteCount())

      # Run the computation again from input1 to output1.
      # (These should have the same length.)
      o.Run('test', self._input_dirs, self._output_dirs[1],
            [self.FileLength(
                '%(input1)s/in1', '%(output)s/out')])

      # Write count goes up by one as an in->out hash is added,
      # but no new output is stored (as it is the same).
      self.assertEquals(4, fs.WriteCount())

      # Check that the test is still valid:
      #   - in0 and in1 have equal length.
      #   - out0 and out1 have that length in them.
      #   - out0 and out1 agree.
      self.assertEquals(
          str(len(pynacl.file_tools.ReadFile(self._input_files[0]))),
          pynacl.file_tools.ReadFile(self._output_files[0])
      )
      self.assertEquals(
          str(len(pynacl.file_tools.ReadFile(self._input_files[1]))),
          pynacl.file_tools.ReadFile(self._output_files[1])
      )
      self.assertEquals(
          pynacl.file_tools.ReadFile(self._output_files[0]),
          pynacl.file_tools.ReadFile(self._output_files[1])
      )

  def test_FailsWhenWritingFails(self):
    # Check that once doesn't eat the storage layer failures for writes.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('FailsWhenWritingFails', work_dir)
      def call(cmd, **kwargs):
        # Cause gsutil commands to fail.
        return 1
      bad_storage = pynacl.gsd_storage.GSDStorage(
          gsutil=['mygsutil'],
          write_bucket='mybucket',
          read_buckets=[],
          call=call)
      o = once.Once(storage=bad_storage, system_summary='test')
      self.assertRaises(pynacl.gsd_storage.GSDStorageError, o.Run, 'test',
          self._input_dirs, self._output_dirs[0],
          [command.Copy('%(input0)s/in0', '%(output)s/out')])

  def test_UseCachedResultsFalse(self):
    # Check that the use_cached_results=False does indeed cause computations
    # to be redone, even when present in the cache.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('UseCachedResultsFalse', work_dir)
      self._tally = 0
      def Copy(logger, subst, src, dst):
        self._tally += 1
        shutil.copyfile(subst.SubstituteAbsPaths(src),
                        subst.SubstituteAbsPaths(dst))
      o = once.Once(storage=pynacl.fake_storage.FakeStorage(),
                    use_cached_results=False,
                    system_summary='test')
      o.Run('test', self._input_dirs, self._output_dirs[0],
            [command.Runnable(None, Copy,'%(input0)s/in0', '%(output)s/out')])
      o.Run('test', self._input_dirs, self._output_dirs[1],
            [command.Runnable(None, Copy,'%(input0)s/in0', '%(output)s/out')])
      self.assertEquals(2, self._tally)
      self.assertEquals(pynacl.file_tools.ReadFile(self._input_files[0]),
                        pynacl.file_tools.ReadFile(self._output_files[0]))
      self.assertEquals(pynacl.file_tools.ReadFile(self._input_files[0]),
                        pynacl.file_tools.ReadFile(self._output_files[1]))

  def test_CacheResultsFalse(self):
    # Check that setting cache_results=False prevents results from being written
    # to the cache.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('CacheResultsFalse', work_dir)
      storage = pynacl.fake_storage.FakeStorage()
      o = once.Once(storage=storage, cache_results=False, system_summary='test')
      o.Run('test', self._input_dirs, self._output_dirs[0],
            [command.Copy('%(input0)s/in0', '%(output)s/out')])
      self.assertEquals(0, storage.ItemCount())
      self.assertEquals(pynacl.file_tools.ReadFile(self._input_files[0]),
                        pynacl.file_tools.ReadFile(self._output_files[0]))

  def test_Mkdir(self):
    # Test the Mkdir convenience wrapper works.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('Mkdir', work_dir)
      foo = os.path.join(work_dir, 'foo')
      o = once.Once(storage=pynacl.fake_storage.FakeStorage(),
                    cache_results=False, system_summary='test')
      o.Run('test', self._input_dirs, foo,
            [command.Mkdir('%(output)s/hi')])
      self.assertTrue(os.path.isdir(os.path.join(foo, 'hi')))

  def test_Command(self):
    # Test a plain command.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('Command', work_dir)
      o = once.Once(storage=pynacl.fake_storage.FakeStorage(),
                    system_summary='test')
      o.Run('test', self._input_dirs, self._output_dirs[0],
            [command.Command([
                sys.executable, '-c',
                'import sys; open(sys.argv[1], "wb").write("hello")',
                '%(output)s/out'])])
      self.assertEquals(
          'hello',
          pynacl.file_tools.ReadFile(self._output_files[0])
      )

  def test_NumCores(self):
    # Test that the core count is substituted. Since we don't know how many
    # cores the test machine will have, just check that it's an integer.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('NumCores', work_dir)
      o = once.Once(storage=pynacl.fake_storage.FakeStorage(),
                    system_summary='test')
      def CheckCores(logger, subst):
        self.assertNotEquals(0, int(subst.Substitute('%(cores)s')))
      o.Run('test', {}, self._output_dirs[0], [command.Runnable(None,
                                                                CheckCores)])

  def test_RunConditionsFalse(self):
    # Test that a command uses run conditions to decide whether or not to run.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('Command', work_dir)
      o = once.Once(storage=pynacl.fake_storage.FakeStorage(),
                    system_summary='test')
      o.Run('test', self._input_dirs, self._output_dirs[0],
            [command.Command([
                sys.executable, '-c',
                'import sys; open(sys.argv[1], "wb").write("hello")',
                '%(output)s/out'],
                run_cond=lambda cmd_opts: True),
             command.Command([
                 sys.executable, '-c',
                 'import sys; open(sys.argv[1], "wb").write("not hello")',
                 '%(output)s/out'],
                 run_cond=lambda cmd_opts: False)])
      self.assertEquals(
          'hello',
          pynacl.file_tools.ReadFile(self._output_files[0])
      )

  def test_OutputsFlushPathHashCache(self):
    # Test that commands that output to a directory that has an input hash
    # value cached raise an error indicating an input output cycle.
    with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
      self.GenerateTestData('CacheFlush', work_dir)
      o = once.Once(storage=pynacl.fake_storage.FakeStorage(),
                    system_summary='test')
      self.assertRaises(
          once.UserError, o.Run,
          'test', self._input_dirs, self._input_dirs['input0'],
          [command.Copy('%(input0)s/in0', '%(output)s/out')])


if __name__ == '__main__':
  unittest.main()
