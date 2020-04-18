# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from dashboard import can_bisect
from dashboard.common import namespaced_stored_object
from dashboard.common import testing_common


class CanBisectTest(testing_common.TestCase):

  def setUp(self):
    super(CanBisectTest, self).setUp()
    namespaced_stored_object.Set(
        can_bisect.BISECT_BOT_MAP_KEY,
        {'SupportedMaster': ['perf_bot', 'bisect_bot']})

  def testIsValidTestForBisect_BisectableTests_ReturnsTrue(self):
    self.assertEqual(
        can_bisect.IsValidTestForBisect(
            'SupportedMaster/mac/blink_perf.parser/simple-url'),
        True)

  def testIsValidTestForBisect_Supported_ReturnsTrue(self):
    self.assertTrue(
        can_bisect.IsValidTestForBisect('SupportedMaster/b/t/foo'))

  def testIsValidTestForBisect_RefTest_ReturnsFalse(self):
    self.assertFalse(
        can_bisect.IsValidTestForBisect('SupportedMaster/b/t/ref'))

  def testIsValidTestForBisect_UnsupportedMaster_ReturnsFalse(self):
    self.assertFalse(
        can_bisect.IsValidTestForBisect('X/b/t/foo'))


if __name__ == '__main__':
  unittest.main()
