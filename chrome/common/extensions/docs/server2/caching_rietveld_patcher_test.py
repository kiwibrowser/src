#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
from caching_rietveld_patcher import (CachingRietveldPatcher,
                                      _VERSION_CACHE_MAXAGE)
from datetime import datetime
from object_store_creator import ObjectStoreCreator
from test_patcher import TestPatcher

_TEST_PATCH_VERSION = '1'
_TEST_PATCH_FILES = (['add.txt'], ['del.txt'], ['modify.txt'])
_TEST_PATCH_DATA = {
  'add.txt': 'add',
  'modify.txt': 'modify',
}

class FakeDateTime(object):
  def __init__(self, time=datetime.now()):
    self.time = time

  def now(self):
    return self.time

class CachingRietveldPatcherTest(unittest.TestCase):
  def setUp(self):
    self._datetime = FakeDateTime()
    self._test_patcher = TestPatcher(_TEST_PATCH_VERSION,
                                     _TEST_PATCH_FILES,
                                     _TEST_PATCH_DATA)
    self._patcher = CachingRietveldPatcher(
        self._test_patcher,
        ObjectStoreCreator(start_empty=False),
        self._datetime)

  def testGetVersion(self):
    # Invalidate cache.
    self._datetime.time += _VERSION_CACHE_MAXAGE
    # Fill cache.
    self._patcher.GetVersion()
    count = self._test_patcher.get_version_count
    # Should read from cache.
    self._patcher.GetVersion()
    self.assertEqual(count, self._test_patcher.get_version_count)
    # Invalidate cache.
    self._datetime.time += _VERSION_CACHE_MAXAGE
    # Should fetch version.
    self._patcher.GetVersion()
    self.assertEqual(count + 1, self._test_patcher.get_version_count)

  def testGetPatchedFiles(self):
    # Fill cache.
    self._patcher.GetPatchedFiles()
    count = self._test_patcher.get_patched_files_count
    # Should read from cache.
    self._patcher.GetPatchedFiles()
    self.assertEqual(count, self._test_patcher.get_patched_files_count)

  def testApply(self):
    # Fill cache.
    self._patcher.Apply(['add.txt'], None).Get()
    count = self._test_patcher.apply_count
    # Should read from cache even though it's reading another file.
    self._patcher.Apply(['modify.txt'], None).Get()
    self.assertEqual(count, self._test_patcher.apply_count)

if __name__ == '__main__':
  unittest.main()
