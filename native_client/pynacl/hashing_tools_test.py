#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of hashing tools."""

import os
import unittest

import file_tools
import hashing_tools
import working_directory


def GenerateTestTree(noise, path):
  """Generate an interesting tree for testing.

  Args:
    noise: A string to inject to vary tree content.
    path: Location to emit tree (should not exist).
  """
  os.mkdir(path)
  b = os.path.join(path, 'b' + noise)
  os.mkdir(b)
  b1 = os.path.join(path, 'b1' + noise)
  os.mkdir(b1)
  b2 = os.path.join(path, 'b2' + noise)
  file_tools.WriteFile('inside b2' + noise, b2)
  c = os.path.join(b, 'c' + noise)
  file_tools.WriteFile('inside c' + noise, c)


class TestHashingTools(unittest.TestCase):

  def test_File(self):
    # Check that one file works.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      filename1 = os.path.join(work_dir, 'myfile1')
      filename2 = os.path.join(work_dir, 'myfile2')
      file_tools.WriteFile('booga', filename1)
      file_tools.WriteFile('booga', filename2)
      h1 = hashing_tools.StableHashPath(filename1)
      h2 = hashing_tools.StableHashPath(filename2)
      self.assertEqual(h1, h2)
      self.assertEqual(40, len(h1))

  def test_NonmatchingFile(self):
    # Check that one file works.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      filename1 = os.path.join(work_dir, 'myfile1')
      filename2 = os.path.join(work_dir, 'myfile2')
      file_tools.WriteFile('booga', filename1)
      file_tools.WriteFile('boo', filename2)
      h1 = hashing_tools.StableHashPath(filename1)
      h2 = hashing_tools.StableHashPath(filename2)
      self.assertNotEqual(h1, h2)
      self.assertEqual(40, len(h1))
      self.assertEqual(40, len(h2))

  def test_Directory(self):
    # Check that a directory tree works.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      a1 = os.path.join(work_dir, 'a1')
      a2 = os.path.join(work_dir, 'a2')
      for path in [a1, a2]:
        GenerateTestTree('gorp', path)
      h1 = hashing_tools.StableHashPath(a1)
      h2 = hashing_tools.StableHashPath(a2)
      self.assertEqual(h1, h2)
      self.assertEqual(40, len(h1))

  def test_NonmatchingDirectory(self):
    # Check that a directory tree works.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      a1 = os.path.join(work_dir, 'a1')
      a2 = os.path.join(work_dir, 'a2')
      GenerateTestTree('gorp', a1)
      GenerateTestTree('gorpy', a2)
      h1 = hashing_tools.StableHashPath(a1)
      h2 = hashing_tools.StableHashPath(a2)
      self.assertNotEqual(h1, h2)
      self.assertEqual(40, len(h1))
      self.assertEqual(40, len(h2))


if __name__ == '__main__':
  unittest.main()
