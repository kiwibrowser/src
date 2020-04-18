#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from copy import deepcopy
import unittest

from host_file_system_provider import HostFileSystemProvider
from host_file_system_iterator import HostFileSystemIterator
from object_store_creator import ObjectStoreCreator
from test_branch_utility import TestBranchUtility
from test_data.canned_data import CANNED_API_FILE_SYSTEM_DATA
from test_file_system import TestFileSystem


def _GetIterationTracker(version):
  '''Adds the ChannelInfo object from each iteration to a list, and signals the
  loop to stop when |version| is reached.
  '''
  iterations = []
  def callback(file_system, channel_info):
    if channel_info.version == version:
      return False
    iterations.append(channel_info)
    return True
  return (iterations, callback)


class HostFileSystemIteratorTest(unittest.TestCase):

  def setUp(self):
    def host_file_system_constructor(branch, **optargs):
      return TestFileSystem(deepcopy(CANNED_API_FILE_SYSTEM_DATA[branch]))
    host_file_system_provider = HostFileSystemProvider(
        ObjectStoreCreator.ForTest(),
        constructor_for_test=host_file_system_constructor)
    self._branch_utility = TestBranchUtility.CreateWithCannedData()
    self._iterator = HostFileSystemIterator(
        host_file_system_provider,
        self._branch_utility)

  def _GetStableChannelInfo(self,version):
    return self._branch_utility.GetStableChannelInfo(version)

  def _GetChannelInfo(self, channel_name):
    return self._branch_utility.GetChannelInfo(channel_name)

  def testAscending(self):
    # Start at |stable| version 5, and move up towards |master|.
    # Total: 28 file systems.
    iterations, callback = _GetIterationTracker(0)
    self.assertEqual(
        self._iterator.Ascending(self._GetStableChannelInfo(5), callback),
        self._GetChannelInfo('master'))
    self.assertEqual(len(iterations), 28)

    # Start at |stable| version 5, and move up towards |master|. The callback
    # fails at |beta|, so the last successful callback was the latest version
    # of |stable|. Total: 25 file systems.
    iterations, callback = _GetIterationTracker(
        self._GetChannelInfo('beta').version)
    self.assertEqual(
        self._iterator.Ascending(self._GetStableChannelInfo(5), callback),
        self._GetChannelInfo('stable'))
    self.assertEqual(len(iterations), 25)

    # Start at |stable| version 5, and the callback fails immediately. Since
    # no file systems are successfully processed, expect a return of None.
    iterations, callback = _GetIterationTracker(5)
    self.assertEqual(
        self._iterator.Ascending(self._GetStableChannelInfo(5), callback),
        None)
    self.assertEqual([], iterations)

    # Start at |stable| version 5, and the callback fails at version 6.
    # The return should represent |stable| version 5.
    iterations, callback = _GetIterationTracker(6)
    self.assertEqual(
        self._iterator.Ascending(self._GetStableChannelInfo(5), callback),
        self._GetStableChannelInfo(5))
    self.assertEqual([self._GetStableChannelInfo(5)], iterations)

    # Start at the latest version of |stable|, and the callback fails at
    # |master|. Total: 3 file systems.
    iterations, callback = _GetIterationTracker('master')
    self.assertEqual(
        self._iterator.Ascending(self._GetChannelInfo('stable'), callback),
        self._GetChannelInfo('dev'))
    self.assertEqual([self._GetChannelInfo('stable'),
                      self._GetChannelInfo('beta'),
                      self._GetChannelInfo('dev')], iterations)

    # Start at |stable| version 10, and the callback fails at |master|.
    iterations, callback = _GetIterationTracker('master')
    self.assertEqual(
        self._iterator.Ascending(self._GetStableChannelInfo(10), callback),
        self._GetChannelInfo('dev'))
    self.assertEqual([self._GetStableChannelInfo(10),
                      self._GetStableChannelInfo(11),
                      self._GetStableChannelInfo(12),
                      self._GetStableChannelInfo(13),
                      self._GetStableChannelInfo(14),
                      self._GetStableChannelInfo(15),
                      self._GetStableChannelInfo(16),
                      self._GetStableChannelInfo(17),
                      self._GetStableChannelInfo(18),
                      self._GetStableChannelInfo(19),
                      self._GetStableChannelInfo(20),
                      self._GetStableChannelInfo(21),
                      self._GetStableChannelInfo(22),
                      self._GetStableChannelInfo(23),
                      self._GetStableChannelInfo(24),
                      self._GetStableChannelInfo(25),
                      self._GetStableChannelInfo(26),
                      self._GetStableChannelInfo(27),
                      self._GetStableChannelInfo(28),
                      self._GetChannelInfo('stable'),
                      self._GetChannelInfo('beta'),
                      self._GetChannelInfo('dev')], iterations)

  def testDescending(self):
    # Start at |master|, and the callback fails immediately. No file systems
    # are successfully processed, so Descending() will return None.
    iterations, callback = _GetIterationTracker('master')
    self.assertEqual(
        self._iterator.Descending(self._GetChannelInfo('master'), callback),
        None)
    self.assertEqual([], iterations)

    # Start at |master|, and the callback fails at |dev|. Last good iteration
    # should be |master|.
    iterations, callback = _GetIterationTracker(
        self._GetChannelInfo('dev').version)
    self.assertEqual(
        self._iterator.Descending(self._GetChannelInfo('master'), callback),
        self._GetChannelInfo('master'))
    self.assertEqual([self._GetChannelInfo('master')], iterations)

    # Start at |master|, and then move from |dev| down to |stable| at version 5.
    # Total: 28 file systems.
    iterations, callback = _GetIterationTracker(0)
    self.assertEqual(
        self._iterator.Descending(self._GetChannelInfo('master'), callback),
        self._GetStableChannelInfo(5))
    self.assertEqual(len(iterations), 28)

    # Start at the latest version of |stable|, and move down to |stable| at
    # version 5. Total: 25 file systems.
    iterations, callback = _GetIterationTracker(0)
    self.assertEqual(
        self._iterator.Descending(self._GetChannelInfo('stable'), callback),
        self._GetStableChannelInfo(5))
    self.assertEqual(len(iterations), 25)

    # Start at |dev| and iterate down through |stable| versions. The callback
    # fails at version 10. Total: 18 file systems.
    iterations, callback = _GetIterationTracker(10)
    self.assertEqual(
        self._iterator.Descending(self._GetChannelInfo('dev'), callback),
        self._GetStableChannelInfo(11))
    self.assertEqual([self._GetChannelInfo('dev'),
                      self._GetChannelInfo('beta'),
                      self._GetChannelInfo('stable'),
                      self._GetStableChannelInfo(28),
                      self._GetStableChannelInfo(27),
                      self._GetStableChannelInfo(26),
                      self._GetStableChannelInfo(25),
                      self._GetStableChannelInfo(24),
                      self._GetStableChannelInfo(23),
                      self._GetStableChannelInfo(22),
                      self._GetStableChannelInfo(21),
                      self._GetStableChannelInfo(20),
                      self._GetStableChannelInfo(19),
                      self._GetStableChannelInfo(18),
                      self._GetStableChannelInfo(17),
                      self._GetStableChannelInfo(16),
                      self._GetStableChannelInfo(15),
                      self._GetStableChannelInfo(14),
                      self._GetStableChannelInfo(13),
                      self._GetStableChannelInfo(12),
                      self._GetStableChannelInfo(11)], iterations)


if __name__ == '__main__':
  unittest.main()
