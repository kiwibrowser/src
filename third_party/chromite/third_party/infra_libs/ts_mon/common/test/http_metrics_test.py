# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import mock

from infra_libs.ts_mon.common import http_metrics
from infra_libs.ts_mon.common import interface
from infra_libs.ts_mon.common import targets


class TestHttpMetrics(unittest.TestCase):
  def setUp(self):
    super(TestHttpMetrics, self).setUp()
    target = targets.TaskTarget('test_service', 'test_job',
                                'test_region', 'test_host')
    self.mock_state = interface.State(target=target)
    self.state_patcher = mock.patch('infra_libs.ts_mon.common.interface.state',
                                    new=self.mock_state)
    self.state_patcher.start()

  def tearDown(self):
    self.state_patcher.stop()
    super(TestHttpMetrics, self).tearDown()

  def test_update_http_server_metrics(self):
    http_metrics.update_http_server_metrics(
        '/', 200, 125.4,
        request_size=100, response_size=200, user_agent='Chrome')
    fields = {'status': 200, 'name': '/', 'is_robot': False}
    self.assertEqual(1, http_metrics.server_response_status.get(fields))
    self.assertEqual(125.4, http_metrics.server_durations.get(fields).sum)
    self.assertEqual(100, http_metrics.server_request_bytes.get(fields).sum)
    self.assertEqual(200, http_metrics.server_response_bytes.get(fields).sum)

  def test_update_http_server_metrics_no_sizes(self):
    http_metrics.update_http_server_metrics('/', 200, 125.4)
    fields = {'status': 200, 'name': '/', 'is_robot': False}
    self.assertEqual(1, http_metrics.server_response_status.get(fields))
    self.assertEqual(125.4, http_metrics.server_durations.get(fields).sum)
    self.assertIsNone(http_metrics.server_request_bytes.get(fields))
    self.assertIsNone(http_metrics.server_response_bytes.get(fields))
