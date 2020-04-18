# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from infra_libs.ts_mon.common import interface
from infra_libs.ts_mon.common import standard_metrics
from infra_libs.ts_mon.common import targets
from infra_libs import ts_mon

class StandardMetricsTest(unittest.TestCase):

  def setUp(self):
    interface.state = interface.State()
    interface.state.reset_for_unittest()
    interface.state.target = targets.TaskTarget(
        'test_service', 'test_job', 'test_region', 'test_host')
    ts_mon.reset_for_unittest()

  def test_up(self):
    standard_metrics.init()
    self.assertTrue(standard_metrics.up.get())
