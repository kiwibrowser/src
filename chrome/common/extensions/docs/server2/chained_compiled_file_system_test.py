#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from chained_compiled_file_system import ChainedCompiledFileSystem
from compiled_file_system import CompiledFileSystem
from object_store_creator import ObjectStoreCreator
from test_file_system import TestFileSystem

_TEST_DATA_BASE = {
  'a.txt': 'base a.txt',
  'dir': {
    'b.txt': 'base b.txt'
  },
}

_TEST_DATA_NEW = {
  'a.txt': 'new a.txt',
  'new.txt': 'a new file',
  'dir': {
    'b.txt': 'new b.txt',
    'new.txt': 'new file in dir',
  },
}

identity = lambda _, x: x

class ChainedCompiledFileSystemTest(unittest.TestCase):
  def setUp(self):
    object_store_creator = ObjectStoreCreator(start_empty=False)
    base_file_system = TestFileSystem(_TEST_DATA_BASE, identity='base')
    self._base_compiled_fs = CompiledFileSystem.Factory(
        object_store_creator).Create(base_file_system,
                                     identity,
                                     ChainedCompiledFileSystemTest)
    chained_factory = ChainedCompiledFileSystem.Factory([base_file_system],
                                                        object_store_creator)
    self._new_file_system = TestFileSystem(_TEST_DATA_NEW, identity='new')
    self._chained_compiled_fs = chained_factory.Create(
        self._new_file_system, identity, ChainedCompiledFileSystemTest)

  def testGetFromFile(self):
    self.assertEqual(self._chained_compiled_fs.GetFromFile('a.txt').Get(),
                     self._base_compiled_fs.GetFromFile('a.txt').Get())
    self.assertEqual(self._chained_compiled_fs.GetFromFile('new.txt').Get(),
                     'a new file')
    self.assertEqual(self._chained_compiled_fs.GetFromFile('dir/new.txt').Get(),
                     'new file in dir')
    self._new_file_system.IncrementStat('a.txt')
    self.assertNotEqual(self._chained_compiled_fs.GetFromFile('a.txt').Get(),
                        self._base_compiled_fs.GetFromFile('a.txt').Get())
    self.assertEqual(self._chained_compiled_fs.GetFromFile('a.txt').Get(),
                     self._new_file_system.ReadSingle('a.txt').Get())

  def testGetFromFileListing(self):
    self.assertEqual(self._chained_compiled_fs.GetFromFileListing('dir/').Get(),
                     self._base_compiled_fs.GetFromFileListing('dir/').Get())
    self._new_file_system.IncrementStat('dir/new.txt')
    self.assertNotEqual(
        self._chained_compiled_fs.GetFromFileListing('dir/').Get(),
        self._base_compiled_fs.GetFromFileListing('dir/').Get())
    self.assertEqual(
        self._chained_compiled_fs.GetFromFileListing('dir/').Get(),
        self._new_file_system.ReadSingle('dir/').Get())

if __name__ == '__main__':
  unittest.main()
