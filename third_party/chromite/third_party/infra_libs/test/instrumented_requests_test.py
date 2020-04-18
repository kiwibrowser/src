# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import unittest

from infra_libs.ts_mon.common import http_metrics
from infra_libs import instrumented_requests
from infra_libs import ts_mon

import requests
import mock


class InstrumentedRequestsTest(unittest.TestCase):
  def setUp(self):
    ts_mon.reset_for_unittest()

    self.response = requests.Response()
    self.response.elapsed = datetime.timedelta(seconds=2, milliseconds=500)
    self.response.status_code = 200
    self.response.request = requests.PreparedRequest()
    self.response.request.prepare_headers(None)

    self.hook = instrumented_requests.instrumentation_hook('foo')

  def tearDown(self):
    super(InstrumentedRequestsTest, self).tearDown()
    mock.patch.stopall()

  def test_success_status(self):
    self.hook(self.response)

    self.assertEquals(1, http_metrics.response_status.get(
        {'name': 'foo', 'client': 'requests', 'status': 200}))
    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'foo', 'client': 'requests', 'status': 404}))

  def test_error_status(self):
    self.response.status_code = 404
    self.hook(self.response)

    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'foo', 'client': 'requests', 'status': 200}))
    self.assertEquals(1, http_metrics.response_status.get(
        {'name': 'foo', 'client': 'requests', 'status': 404}))

  def test_response_bytes_none(self):
    self.hook(self.response)

    self.assertEquals(0, http_metrics.response_bytes.get(
        {'name': 'foo', 'client': 'requests'}).sum)

  def test_response_bytes(self):
    self.response.headers['content-length'] = '7'
    self.hook(self.response)

    self.assertEquals(7, http_metrics.response_bytes.get(
        {'name': 'foo', 'client': 'requests'}).sum)

  def test_request_bytes_none(self):
    self.hook(self.response)

    self.assertEquals(0, http_metrics.request_bytes.get(
        {'name': 'foo', 'client': 'requests'}).sum)

  def test_request_bytes(self):
    self.response.request.headers['content-length'] = '5'
    self.hook(self.response)

    self.assertEquals(5, http_metrics.request_bytes.get(
        {'name': 'foo', 'client': 'requests'}).sum)

  def test_durations(self):
    self.hook(self.response)

    self.assertEquals(2500, http_metrics.durations.get(
        {'name': 'foo', 'client': 'requests'}).sum)

  @staticmethod
  def _setup_wrap(hooks=None, side_effect=None):
    requests._instrumented_test = mock.Mock()
    if side_effect is not None:
      requests._instrumented_test.side_effect = side_effect
    kwargs = {}
    if hooks:
      kwargs = {'hooks': hooks}
    instrumented_requests._wrap(
        '_instrumented_test', 'foo', 'http://example.com', **kwargs)
    return requests._instrumented_test

  def test_wrap(self):
    f = self._setup_wrap()

    self.assertTrue(f.called)
    self.assertEquals(('http://example.com',), f.call_args[0])
    self.assertIn('hooks', f.call_args[1])
    self.assertIn('response', f.call_args[1]['hooks'])
    self.assertTrue(hasattr(f.call_args[1]['hooks']['response'], '__call__'))

  def test_wrap_merges_hooks(self):
    f = self._setup_wrap(hooks={'foo': lambda: 42})

    self.assertTrue(f.called)
    self.assertEquals(('http://example.com',), f.call_args[0])
    self.assertIn('hooks', f.call_args[1])
    self.assertIn('response', f.call_args[1]['hooks'])
    self.assertIn('foo', f.call_args[1]['hooks'])
    self.assertTrue(hasattr(f.call_args[1]['hooks']['response'], '__call__'))
    self.assertEquals(42, f.call_args[1]['hooks']['foo']())

  def test_wrap_times_out(self):
    with self.assertRaises(requests.exceptions.ReadTimeout):
      self._setup_wrap(side_effect=requests.exceptions.ReadTimeout)

    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'foo', 'client': 'requests', 'status': 200}))
    self.assertEquals(1, http_metrics.response_status.get(
        {'name': 'foo', 'client': 'requests',
         'status': http_metrics.STATUS_TIMEOUT}))

  def test_wrap_errors_out(self):
    with self.assertRaises(requests.exceptions.ConnectionError):
      self._setup_wrap(side_effect=requests.exceptions.ConnectionError)

    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'foo', 'client': 'requests', 'status': 200}))
    self.assertEquals(1, http_metrics.response_status.get(
        {'name': 'foo', 'client': 'requests',
         'status': http_metrics.STATUS_ERROR}))

  def test_wrap_raises_other_exception(self):
    with self.assertRaises(requests.exceptions.RequestException):
      self._setup_wrap(side_effect=requests.exceptions.RequestException)

    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'foo', 'client': 'requests', 'status': 200}))
    self.assertEquals(1, http_metrics.response_status.get(
        {'name': 'foo', 'client': 'requests',
         'status': http_metrics.STATUS_EXCEPTION}))
