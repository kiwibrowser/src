# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the filelib module."""

from __future__ import print_function

import os
import shutil

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib.paygen import filelib
from chromite.lib.paygen import utils


class TestFileManipulation(cros_test_lib.TestCase):
  """Test cases for filelib."""

  FILE1 = 'file1a'
  FILE2 = 'file2'
  SUBDIR = 'subdir'
  SUBFILE = '%s/file1b' % SUBDIR
  FILE_GLOB = 'file1*'

  FILE1_CONTENTS = 'Howdy doody there dandy'
  FILE2_CONTENTS = 'Once upon a time in a galaxy far far away.'
  SUBFILE_CONTENTS = 'Five little monkeys jumped on the bed.'

  def _SetUpTempdir(self, tempdir):
    with open(os.path.join(tempdir, self.FILE1), 'w') as out1:
      out1.write(self.FILE1_CONTENTS)

    with open(os.path.join(tempdir, self.FILE2), 'w') as out2:
      out2.write(self.FILE2_CONTENTS)

    subdir = os.path.join(tempdir, self.SUBDIR)
    osutils.SafeMakedirs(subdir)

    with open(os.path.join(tempdir, self.SUBFILE), 'w') as out3:
      out3.write(self.SUBFILE_CONTENTS)

  def testIntegrationScript(self):
    dir1 = None
    dir2 = None
    try:
      dir1 = utils.CreateTmpDir('filelib_unittest1-')
      dir2 = utils.CreateTmpDir('filelib_unittest2-')

      self._SetUpTempdir(dir1)

      dir1_file1 = os.path.join(dir1, self.FILE1)
      dir1_file2 = os.path.join(dir1, self.FILE2)
      dir1_subfile = os.path.join(dir1, self.SUBFILE)
      dir1_top_files = [dir1_file1, dir1_file2]
      dir1_deep_files = dir1_top_files + [dir1_subfile]

      dir2_file1 = os.path.join(dir2, self.FILE1)
      dir2_file2 = os.path.join(dir2, self.FILE2)
      dir2_subdir = os.path.join(dir2, self.SUBDIR)
      dir2_subfile = os.path.join(dir2, self.SUBFILE)
      dir2_top_files = [dir2_file1, dir2_file2]
      dir2_deep_files = dir2_top_files + [dir2_subfile]

      # Test Exists.
      for dir1_path in dir1_deep_files:
        self.assertTrue(filelib.Exists(dir1_path))
      for dir2_path in dir2_deep_files:
        self.assertFalse(filelib.Exists(dir2_path))

      # Test ListFiles with various options.
      self.assertEqual(set(dir1_top_files),
                       set(filelib.ListFiles(dir1)))
      self.assertEqual(set(dir1_deep_files),
                       set(filelib.ListFiles(dir1, recurse=True)))
      self.assertEqual(sorted(dir1_deep_files),
                       filelib.ListFiles(dir1, recurse=True, sort=True))
      self.assertEqual(set([dir1_file1, dir1_subfile]),
                       set(filelib.ListFiles(dir1, recurse=True,
                                             filepattern=self.FILE_GLOB)))
      # Test CopyFiles from dir1 to dir2.
      self.assertEqual(set(dir2_deep_files),
                       set(filelib.CopyFiles(dir1, dir2)))
      for dir2_path in dir2_deep_files:
        self.assertTrue(filelib.Exists(dir2_path))

      # Test Cmp.
      self.assertTrue(filelib.Cmp(dir1_file1, dir2_file1))
      self.assertTrue(filelib.Cmp(dir2_file2, dir1_file2))
      self.assertFalse(filelib.Cmp(dir1_file2, dir2_file1))

      # Test RemoveDirContents.
      filelib.RemoveDirContents(dir2_subdir)
      self.assertTrue(filelib.Exists(dir2_subdir, as_dir=True))
      self.assertFalse(filelib.Exists(dir2_subfile))
      filelib.RemoveDirContents(dir2)
      self.assertTrue(filelib.Exists(dir2, as_dir=True))
      for dir2_path in dir2_deep_files:
        self.assertFalse(filelib.Exists(dir2_path))

      filelib.RemoveDirContents(dir1)
      self.assertTrue(filelib.Exists(dir1, as_dir=True))
      for dir1_path in dir1_deep_files:
        self.assertFalse(filelib.Exists(dir1_path))

    finally:
      for d in (dir1, dir2):
        if d and os.path.isdir(d):
          shutil.rmtree(d)


class TestFileLib(cros_test_lib.MoxTempDirTestCase):
  """Test filelib module.

  Note: We use tools for hashes to avoid relying on hashlib since that's what
  the filelib module uses.  We want to verify things rather than have a single
  hashlib bug break both the code and the tests.
  """

  def _MD5Sum(self, file_path):
    """Use RunCommand to get the md5sum of a file."""
    return cros_build_lib.RunCommand(
        ['md5sum', file_path], redirect_stdout=True).output.split(' ')[0]

  def _SHA1Sum(self, file_path):
    """Use sha1sum utility to get SHA1 of a file."""
    # The sha1sum utility gives SHA1 in base 16 encoding.  We need base 64.
    hash16 = cros_build_lib.RunCommand(
        ['sha1sum', file_path], redirect_stdout=True).output.split(' ')[0]
    return hash16.decode('hex').encode('base64').rstrip()

  def _SHA256Sum(self, file_path):
    """Use sha256 utility to get SHA256 of a file."""
    # The sha256sum utility gives SHA256 in base 16 encoding.  We need base 64.
    hash16 = cros_build_lib.RunCommand(
        ['sha256sum', file_path], redirect_stdout=True).output.split(' ')[0]
    return hash16.decode('hex').encode('base64').rstrip()

  def testMD5Sum(self):
    """Test MD5Sum output with the /usr/bin/md5sum binary."""
    file_path = os.path.abspath(__file__)
    self.assertEqual(self._MD5Sum(file_path), filelib.MD5Sum(file_path))

  def testShaSums(self):
    file_path = os.path.abspath(__file__)
    expected_sha1 = self._SHA1Sum(file_path)
    expected_sha256 = self._SHA256Sum(file_path)
    sha1, sha256 = filelib.ShaSums(file_path)
    self.assertEqual(expected_sha1, sha1)
    self.assertEqual(expected_sha256, sha256)

  def testCmp(self):
    path1 = '/some/local/path'
    path2 = '/other/local/path'

    self.mox.StubOutWithMock(filelib.os.path, 'exists')
    self.mox.StubOutWithMock(filelib.filecmp, 'cmp')

    # Set up the test replay script.
    # Run 1, both exist, are different.
    filelib.os.path.exists(path1).AndReturn(True)
    filelib.os.path.exists(path2).AndReturn(True)
    filelib.filecmp.cmp(path1, path2).AndReturn(True)
    # Run 2, both exist, are different.
    filelib.os.path.exists(path1).AndReturn(True)
    filelib.os.path.exists(path2).AndReturn(True)
    filelib.filecmp.cmp(path1, path2).AndReturn(False)
    # Run 3, second file missing.
    filelib.os.path.exists(path1).AndReturn(True)
    filelib.os.path.exists(path2).AndReturn(False)
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertTrue(filelib.Cmp(path1, path2))
    self.assertFalse(filelib.Cmp(path1, path2))
    self.assertFalse(filelib.Cmp(path1, path2))
    self.mox.VerifyAll()

  def testCopy(self):
    path1 = '/some/local/path'
    path2 = '/other/local/path'
    relative_path = 'relative.bin'

    self.mox.StubOutWithMock(filelib, 'Exists')
    self.mox.StubOutWithMock(osutils, 'SafeMakedirs')
    self.mox.StubOutWithMock(filelib.shutil, 'copy2')

    # Set up the test replay script.
    # Run 1, path2 directory exists.
    filelib.Exists(os.path.dirname(path2), as_dir=True).AndReturn(True)
    filelib.shutil.copy2(path1, path2)
    # Run 2, path2 directory does not exist.
    filelib.Exists(os.path.dirname(path2), as_dir=True).AndReturn(False)
    osutils.SafeMakedirs(os.path.dirname(path2))
    filelib.shutil.copy2(path1, path2)

    # Run 3, there is target directory is '.', don't test existence.
    filelib.shutil.copy2(path1, relative_path)
    self.mox.ReplayAll()

    # Run the test verifications, three times.
    filelib.Copy(path1, path2)
    filelib.Copy(path1, path2)
    filelib.Copy(path1, relative_path)
    self.mox.VerifyAll()

  def testSize(self):
    path = '/some/local/path'
    size = 100

    self.mox.StubOutWithMock(filelib.os.path, 'isfile')
    self.mox.StubOutWithMock(filelib.os, 'stat')

    # Set up the test replay script.
    # Run 1, success.
    filelib.os.path.isfile(path).AndReturn(True)
    filelib.os.stat(path).AndReturn(cros_test_lib.EasyAttr(st_size=size))
    # Run 2, file not found.
    filelib.os.path.isfile(path).AndReturn(False)
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertEqual(size, filelib.Size(path))
    self.assertRaises(filelib.MissingFileError, filelib.Size, path)
    self.mox.VerifyAll()

  def testExists(self):
    path = '/some/local/path'
    result = 'TheResult'

    self.mox.StubOutWithMock(filelib.os.path, 'isdir')
    self.mox.StubOutWithMock(filelib.os.path, 'isfile')

    # Set up the test replay script.
    # Run 1, as file.
    filelib.os.path.isfile(path).AndReturn(result)
    # Run 2, as dir.
    filelib.os.path.isdir(path).AndReturn(result)
    self.mox.ReplayAll()

    # Run the test verification.
    self.assertEqual(result, filelib.Exists(path))
    self.assertEqual(result, filelib.Exists(path, as_dir=True))
    self.mox.VerifyAll()

  def _CreateSimpleFile(self, *args):
    contents = 'Not important, can be anything'
    for path in args:
      with open(path, 'w') as out:
        out.write(contents)

  def testRemove(self):
    # pylint: disable=E1101
    path1 = os.path.join(self.tempdir, 'file1')
    path2 = os.path.join(self.tempdir, 'file2')
    missing_path = os.path.join(self.tempdir, 'missing')
    subdir = os.path.join(self.tempdir, 'subdir')
    subpath1 = os.path.join(subdir, 'file3')
    subpath2 = os.path.join(subdir, 'file4')

    # Test remove on path that does not exist.
    self.assertRaises(filelib.MissingFileError, filelib.Remove, path1)
    self.assertFalse(filelib.Remove(path1, ignore_no_match=True))

    # Test remove on simple file.
    self._CreateSimpleFile(path1)
    self.assertTrue(filelib.Remove(path1))
    self.assertRaises(filelib.MissingFileError, filelib.Remove, path1)
    self.assertFalse(filelib.Remove(path1, ignore_no_match=True))

    # Test remove on more than one file.
    self._CreateSimpleFile(path1, path2)
    self.assertTrue(filelib.Remove(path1, path2))

    # Test remove on multiple files, with one missing.
    self._CreateSimpleFile(path1, path2)
    self.assertRaises(filelib.MissingFileError, filelib.Remove,
                      path1, missing_path, path2)
    # First path1 removed, but path2 not because it was after missing.
    self.assertFalse(filelib.Exists(path1))
    self.assertTrue(filelib.Exists(path2))

    # Test remove multiple files, one missing, with ignore_no_match True.
    self._CreateSimpleFile(path1, path2)
    self.assertFalse(filelib.Remove(path1, missing_path, path2,
                                    ignore_no_match=True))
    self.assertFalse(filelib.Exists(path1))
    self.assertFalse(filelib.Exists(path2))

    # Test recursive Remove.
    os.makedirs(subdir)
    self._CreateSimpleFile(path1, path2, subpath1, subpath2)
    self.assertTrue(filelib.Remove(path1, path2, subdir, recurse=True))
    self.assertFalse(filelib.Exists(path1))
    self.assertFalse(filelib.Exists(subpath1))

    # Test recursive Remove with one missing path.
    os.makedirs(subdir)
    self._CreateSimpleFile(path1, path2, subpath1, subpath2)
    self.assertRaises(filelib.MissingFileError, filelib.Remove,
                      path1, subdir, missing_path, path2, recurse=True)
    self.assertFalse(filelib.Exists(path1))
    self.assertTrue(filelib.Exists(path2))
    self.assertFalse(filelib.Exists(subpath1))

    # Test recursive Remove with one missing path and ignore_no_match True.
    os.makedirs(subdir)
    self._CreateSimpleFile(path1, path2, subpath1, subpath2)
    self.assertFalse(filelib.Remove(path1, subdir, missing_path, path2,
                                    recurse=True, ignore_no_match=True))
    self.assertFalse(filelib.Exists(path1))
    self.assertFalse(filelib.Exists(path2))
    self.assertFalse(filelib.Exists(subpath1))
