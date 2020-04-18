#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from mock_function import MockFunction


class MockFunctionUnittest(unittest.TestCase):
  def testMockFunction(self):
    @MockFunction
    def calc(a, b, mult=1):
      return (a + b) * mult

    self.assertTrue(*calc.CheckAndReset(0))
    self.assertEqual(
        (False, 'calc: expected 1 call(s), got 0'), calc.CheckAndReset(1))

    self.assertEqual(20, calc(2, 3, mult=4))
    self.assertTrue(*calc.CheckAndReset(1))
    self.assertTrue(*calc.CheckAndReset(0))

    self.assertEqual(20, calc(2, 3, mult=4))
    self.assertEqual(
        (False, 'calc: expected 0 call(s), got 1'), calc.CheckAndReset(0))

    self.assertEqual(3, calc(1, 2))
    self.assertEqual(0, calc(3, 4, mult=0))
    self.assertTrue(*calc.CheckAndReset(2))
    self.assertTrue(*calc.CheckAndReset(0))

    self.assertEqual(3, calc(1, 2))
    self.assertEqual(0, calc(3, 4, mult=0))
    self.assertEqual(
        (False, 'calc: expected 3 call(s), got 2'), calc.CheckAndReset(3))


if __name__ == '__main__':
  unittest.main()
