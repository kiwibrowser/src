# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for osinfo_metrics."""

# pylint: disable=protected-access

from __future__ import absolute_import
from __future__ import print_function

import mock

from chromite.lib import cros_test_lib
from chromite.scripts.sysmon import osinfo_metrics


class TestOSInfoMetrics(cros_test_lib.TestCase):
  """Tests for osinfo_metrics."""

  def setUp(self):
    patcher = mock.patch('infra_libs.ts_mon.common.interface.state.store',
                         autospec=True)
    self.store = patcher.start()
    self.addCleanup(patcher.stop)

  def test_collect(self):
    with mock.patch('platform.system', autospec=True) as system, \
         mock.patch('platform.dist', autospec=True) as dist, \
         mock.patch('sys.maxsize', 2**64):
      system.return_value = 'Linux'
      dist.return_value = ('Ubuntu', '14.04', 'trusty')
      osinfo_metrics.collect_os_info()

    setter = self.store.set
    calls = [
        mock.call('proc/os/name', (), None, 'ubuntu', enforce_ge=mock.ANY),
        mock.call('proc/os/version', (), None, '14.04', enforce_ge=mock.ANY),
        mock.call('proc/os/arch', (), None, 'x86_64', enforce_ge=mock.ANY),
        mock.call('proc/python/arch', (), None, '64', enforce_ge=mock.ANY),
    ]
    setter.assert_has_calls(calls)
    self.assertEqual(len(setter.mock_calls), len(calls))
