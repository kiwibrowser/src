#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import posixpath
import unittest

from extensions_paths import SERVER2
from local_file_system import LocalFileSystem


class LocalFileSystemTest(unittest.TestCase):
  def setUp(self):
    self._file_system = LocalFileSystem.Create(
        SERVER2, 'test_data', 'file_system/')

  def testReadFiles(self):
    expected = {
      'test1.txt': 'test1\n',
      'test2.txt': 'test2\n',
      'test3.txt': 'test3\n',
    }
    self.assertEqual(
        expected,
        self._file_system.Read(['test1.txt', 'test2.txt', 'test3.txt']).Get())

  def testListDir(self):
    expected = ['dir/']
    for i in range(7):
      expected.append('file%d.html' % i)
    self.assertEqual(expected,
                     sorted(self._file_system.ReadSingle('list/').Get()))


if __name__ == '__main__':
  unittest.main()
