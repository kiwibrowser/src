# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from metrics import cpu

# Testing private method.
# pylint: disable=protected-access


class CpuMetricTest(unittest.TestCase):

  def testSubtractCpuStats(self):
    # The result computed is a ratio of cpu time used to time elapsed.
    start = {'Browser': {'CpuProcessTime': 0, 'TotalTime': 0}}
    end = {'Browser': {'CpuProcessTime': 5, 'TotalTime': 20}}
    self.assertEqual({'Browser': 0.25}, cpu._SubtractCpuStats(end, start))

    # An error is thrown if the args are called in the wrong order.
    self.assertRaises(AssertionError, cpu._SubtractCpuStats, start, end)

    # An error is thrown if there's a process type in end that's not in start.
    end['Renderer'] = {'CpuProcessTime': 2, 'TotalTime': 20}
    self.assertRaises(AssertionError, cpu._SubtractCpuStats, end, start)

    # A process type will be ignored if there's an empty dict for start or end.
    start['Renderer'] = {}
    self.assertEqual({'Browser': 0.25}, cpu._SubtractCpuStats(end, start))

    # Results for multiple process types can be computed.
    start['Renderer'] = {'CpuProcessTime': 0, 'TotalTime': 0}
    self.assertEqual({'Browser': 0.25, 'Renderer': 0.1},
                     cpu._SubtractCpuStats(end, start))

    # Test 32-bit overflow.
    start = {'Browser': {'CpuProcessTime': 0, 'TotalTime': 2 ** 32 / 100. - 20}}
    end = {'Browser': {'CpuProcessTime': 5, 'TotalTime': 20}}
    self.assertEqual({'Browser': 0.125}, cpu._SubtractCpuStats(end, start))
    self.assertRaises(AssertionError, cpu._SubtractCpuStats, start, end)
