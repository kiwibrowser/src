# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for iter_utils."""

from __future__ import print_function

from chromite.lib import cros_test_lib

from chromite.lib import iter_utils

class IntervalsTest(cros_test_lib.TestCase):
  """Test of interval overlap calculation."""
  # pylint: disable=protected-access

  def testIntervals(self):
    self.assertEqual([], iter_utils.IntersectIntervals([]))
    self.assertEqual([(1, 2)], iter_utils.IntersectIntervals([[(1, 2)]]))

    test_group_0 = [(1, 10)]
    test_group_1 = [(2, 5), (7, 10)]
    test_group_2 = [(2, 8), (9, 12)]
    self.assertEqual(
        [(2, 5), (7, 8), (9, 10)],
        iter_utils.IntersectIntervals([test_group_0, test_group_1,
                                       test_group_2])
    )

    test_group_0 = [(1, 3), (10, 12)]
    test_group_1 = [(2, 5)]
    self.assertEqual(
        [(2, 3)],
        iter_utils.IntersectIntervals([test_group_0, test_group_1]))


class ChunksTest(cros_test_lib.TestCase):
  """Test of chunk-splitting."""

  def testNoInput(self):
    self.assertEqual(list(iter_utils.SplitToChunks([], 1)), [])

  def testExtraFinalChunk(self):
    self.assertEqual(
        list(iter_utils.SplitToChunks([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], 4)),
        [[1, 2, 3, 4], [5, 6, 7, 8], [9, 10]])

  def testEqualChunks(self):
    self.assertEqual(
        list(iter_utils.SplitToChunks([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], 5)),
        [[1, 2, 3, 4, 5], [6, 7, 8, 9, 10]])
