#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
from cStringIO import StringIO
from zipfile import ZipFile

from compiled_file_system import CompiledFileSystem
from directory_zipper import DirectoryZipper
from file_system import FileNotFoundError
from test_file_system import TestFileSystem
from object_store_creator import ObjectStoreCreator


_TEST_DATA = {
  'top': {
    'one.txt': 'one.txt contents',
    'two': {
      'three.txt': 'three.txt contents',
      'four.txt': 'four.txt contents',
    },
  }
}


class DirectoryZipperTest(unittest.TestCase):
  def setUp(self):
    self._directory_zipper = DirectoryZipper(
        CompiledFileSystem.Factory(ObjectStoreCreator.ForTest()),
        TestFileSystem(_TEST_DATA))

  def testTopZip(self):
    top_zip = ZipFile(StringIO(self._directory_zipper.Zip('top/').Get()))
    self.assertEqual(['top/one.txt', 'top/two/four.txt', 'top/two/three.txt'],
                     sorted(top_zip.namelist()))
    self.assertEqual('one.txt contents', top_zip.read('top/one.txt'))
    self.assertEqual('three.txt contents', top_zip.read('top/two/three.txt'))
    self.assertEqual('four.txt contents', top_zip.read('top/two/four.txt'))

  def testTwoZip(self):
    two_zip = ZipFile(StringIO(self._directory_zipper.Zip('top/two/').Get()))
    self.assertEqual(['two/four.txt', 'two/three.txt'],
                     sorted(two_zip.namelist()))
    self.assertEqual('three.txt contents', two_zip.read('two/three.txt'))
    self.assertEqual('four.txt contents', two_zip.read('two/four.txt'))

  def testNotFound(self):
    self.assertRaises(FileNotFoundError,
                      self._directory_zipper.Zip('notfound/').Get)


if __name__ == '__main__':
  unittest.main()
