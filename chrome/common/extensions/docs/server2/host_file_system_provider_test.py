#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from copy import deepcopy
import unittest

from extensions_paths import CHROME_API
from file_system import FileNotFoundError
from host_file_system_provider import HostFileSystemProvider
from object_store_creator import ObjectStoreCreator
from test_data.canned_data import CANNED_API_FILE_SYSTEM_DATA
from test_file_system import TestFileSystem

class HostFileSystemProviderTest(unittest.TestCase):
  def setUp(self):
    self._idle_path = CHROME_API + 'idle.json'
    self._canned_data = deepcopy(CANNED_API_FILE_SYSTEM_DATA)

  def _constructor_for_test(self, branch, **optargs):
    return TestFileSystem(self._canned_data[branch])

  def testWithCaching(self):
    creator = HostFileSystemProvider(
        ObjectStoreCreator.ForTest(),
        constructor_for_test=self._constructor_for_test)

    fs = creator.GetBranch('1500')
    first_read = fs.ReadSingle(self._idle_path).Get()
    self._canned_data['1500']['chrome']['common']['extensions'].get('api'
        )['idle.json'] = 'blah blah blah'
    second_read = fs.ReadSingle(self._idle_path).Get()

    self.assertEqual(first_read, second_read)

  def testWithOffline(self):
    creator = HostFileSystemProvider(
        ObjectStoreCreator.ForTest(),
        offline=True,
        constructor_for_test=self._constructor_for_test)

    fs = creator.GetBranch('1500')
    # Offline file system should raise a FileNotFoundError if read is attempted.
    self.assertRaises(FileNotFoundError, fs.ReadSingle(self._idle_path).Get)

if __name__ == '__main__':
  unittest.main()
