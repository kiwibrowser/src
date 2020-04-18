# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import random
import StringIO
import time
import unittest

import httplib2
import mock

import infra_libs
from infra_libs.event_mon import config
from infra_libs.event_mon import router
from infra_libs.event_mon.protos.chrome_infra_log_pb2 import ChromeInfraEvent
from infra_libs.event_mon.protos.log_request_lite_pb2 import LogRequestLite
import infra_libs.event_mon.monitoring


DATA_DIR = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data')


class HttpRouterTests(unittest.TestCase):
  def test_without_credentials_smoke(self):
    # Use dry_run to avoid code that deals with http.
    r = router._HttpRouter({}, 'https://any.where', dry_run=True)
    self.assertIsInstance(r._http, infra_libs.InstrumentedHttp)

  def test_push_smoke(self):
    r = router._HttpRouter({}, 'https://any.where', dry_run=True)

    req = LogRequestLite.LogEventLite()
    req.event_time_ms = router.time_ms()
    req.event_code = 1
    req.event_flow_id = 2
    self.assertTrue(r.push_event(req))

  def test_push_invalid_event(self):
    r = router._HttpRouter({}, 'https://any.where', dry_run=True)
    self.assertFalse(r.push_event(None))

  def test_invalid_url(self):
    event = LogRequestLite.LogEventLite()
    event.event_time_ms = router.time_ms()
    event.event_code = 1
    event.event_flow_id = 2

    r = router._HttpRouter({}, 'ftp://any.where', dry_run=True)
    self.assertFalse(r.push_event(event))

  # Below this line, test the send/retry loop.
  def test_push_ok(self):
    # Successfully push event the first time.
    sleep = mock.create_autospec(time.sleep, auto_set=True)
    r = router._HttpRouter({}, 'https://bla.bla', _sleep_fn=sleep)
    r._http = infra_libs.HttpMock([('https://bla.bla', {'status': 200}, '')])

    event = LogRequestLite.LogEventLite()
    event.event_time_ms = router.time_ms()
    event.event_code = 1
    event.event_flow_id = 2
    self.assertTrue(r.push_event(event))
    self.assertEquals(len(sleep.call_args_list), 0)

  def test_push_fail(self):
    # Fail to push events even after all retries
    sleep = mock.create_autospec(time.sleep, auto_set=True)
    r = router._HttpRouter({}, 'https://bla.bla', _sleep_fn=sleep)
    r._http = infra_libs.HttpMock([('https://bla.bla', {'status': 403}, '')])

    event = LogRequestLite.LogEventLite()
    event.event_time_ms = router.time_ms()
    event.event_code = 1
    event.event_flow_id = 2
    self.assertFalse(r.push_event(event))
    self.assertEquals(len(sleep.call_args_list), r.try_num)

  def test_push_exception(self):
    # Fail to push events even after all retries
    sleep = mock.create_autospec(time.sleep, auto_set=True)
    r = router._HttpRouter({}, 'https://bla.bla', _sleep_fn=sleep)

    class FakeHttp(object):
      # pylint: disable=unused-argument
      def request(self, *args, **kwargs):
        raise ValueError()

    r._http = FakeHttp()

    event = LogRequestLite.LogEventLite()
    event.event_time_ms = router.time_ms()
    event.event_code = 1
    event.event_flow_id = 2
    self.assertFalse(r.push_event(event))
    self.assertEquals(len(sleep.call_args_list), 3)

  @mock.patch('logging.debug', autospec=True)
  def test_logs_success(self, logdebug):
    r = router._HttpRouter({}, 'https://bla.bla')
    r._http = infra_libs.HttpMock([('https://bla.bla', {'status': 200}, '')])

    event = LogRequestLite.LogEventLite()
    event.event_time_ms = router.time_ms()
    event.event_code = 1
    event.event_flow_id = 2
    self.assertTrue(r.push_event(event))
    self.assertIn(mock.call('Succeeded POSTing data after %d attempts', 1),
                  logdebug.call_args_list)


class TextStreamRouterTests(unittest.TestCase):
  def test_stdout_smoke(self):
    event = LogRequestLite.LogEventLite()
    event.event_time_ms = router.time_ms()
    event.event_code = 1
    event.event_flow_id = 2
    r = router._TextStreamRouter()
    self.assertTrue(r.push_event(event))

  def test_stringio_stream(self):
    config._cache['default_event'] = ChromeInfraEvent()
    log_event = infra_libs.event_mon.monitoring._get_service_event(
      'START', timestamp_kind='POINT', event_timestamp=1234).log_event()

    stream = StringIO.StringIO()
    r = router._TextStreamRouter(stream=stream)
    self.assertTrue(r.push_event(log_event))
    stream.seek(0)
    output = stream.read()
    self.assertGreater(stream.len, 10)
    # Minimal checking because if this is printed the rest is printed as well
    # unless google.protobuf is completely broken.
    self.assertIn('timestamp_kind: POINT', output)

  def test_closed_stream(self):
    event = LogRequestLite.LogEventLite()
    event.event_time_ms = router.time_ms()
    event.event_code = 1
    event.event_flow_id = 2
    stream = StringIO.StringIO()
    stream.close()
    r = router._TextStreamRouter(stream=stream)
    self.assertFalse(r.push_event(event))


class LocalFileRouterTests(unittest.TestCase):
  def test_existing_directory(self):
    event = LogRequestLite.LogEventLite()
    event.event_time_ms = router.time_ms()
    event.event_code = 1
    event.event_flow_id = 2

    with infra_libs.temporary_directory(prefix='event-mon-') as tempdir:
      filename = os.path.join(tempdir, 'output.db')
      r = router._LocalFileRouter(filename, dry_run=False)
      self.assertTrue(r.push_event(event))

      # Check that the file is readable and contains the right data.
      with open(filename, 'rb') as f:
        req_read = LogRequestLite.FromString(f.read())
        self.assertEqual(len(req_read.log_event), 1)
        event_read = req_read.log_event[0]
        self.assertEqual(event_read.event_time_ms, event.event_time_ms)
        self.assertEqual(event_read.event_code, event.event_code)
        self.assertEqual(event_read.event_flow_id, event.event_flow_id)

  def test_existing_directory_dry_run(self):
    event = LogRequestLite.LogEventLite()
    event.event_time_ms = router.time_ms()
    event.event_code = 1
    event.event_flow_id = 2

    with infra_libs.temporary_directory(prefix='event-mon-') as tempdir:
      filename = os.path.join(tempdir, 'output.db')
      r = router._LocalFileRouter(filename, dry_run=True)
      self.assertTrue(r.push_event(event))

      # Check that the file has not been written
      self.assertFalse(os.path.exists(filename))

  def test_non_existing_directory(self):
    req = LogRequestLite.LogEventLite()
    req.event_time_ms = router.time_ms()
    req.event_code = 1
    req.event_flow_id = 2
    r = router._LocalFileRouter(
      os.path.join('non_existing_d1r_31401789', 'output.db'), dry_run=False)
    self.assertFalse(r.push_event(req))


class BackoffTest(unittest.TestCase):
  def test_backoff_time_first_value(self):
    t = router.backoff_time(attempt=0, retry_backoff=2.)
    random.seed(0)
    self.assertTrue(1.5 <= t <= 2.5)

  def test_backoff_time_max_value(self):
    t = router.backoff_time(attempt=10, retry_backoff=2., max_delay=5)
    self.assertTrue(abs(t - 5.) < 0.0001)


class LoggingStreamRouterTests(unittest.TestCase):
  @mock.patch('logging.log')
  def test_events_are_logged_correctly(self, log_mock):
    logger = router._LoggingStreamRouter()
    events = []
    for i in range(3):
      event = LogRequestLite.LogEventLite()
      event.event_time_ms = router.time_ms()
      event.event_code = 1
      event.event_flow_id = 2
      infra_event = ChromeInfraEvent()
      infra_event.cq_event.issue = str(i + 1)
      event.source_extension = infra_event.SerializeToString()
      events.append(event)

    self.assertTrue(logger.push_event(events))

    expected_calls = [
        ((logging.INFO,
          'Sending event_mon event:\ncq_event {\n  issue: "1"\n}\n'),),
        ((logging.INFO,
          'Sending event_mon event:\ncq_event {\n  issue: "2"\n}\n'),),
        ((logging.INFO,
          'Sending event_mon event:\ncq_event {\n  issue: "3"\n}\n'),)]
    self.assertEqual(log_mock.call_args_list, expected_calls)

  @mock.patch('logging.log')
  def test_events_are_logged_with_specified_severity(self, log_mock):
    logger = router._LoggingStreamRouter(logging.WARN)
    event = LogRequestLite.LogEventLite()
    event.event_time_ms = router.time_ms()
    event.event_code = 1
    event.event_flow_id = 2
    self.assertTrue(logger.push_event(event))
    self.assertEqual(log_mock.call_args[0], (logging.WARN, mock.ANY))

  @mock.patch('logging.exception')
  def test_fails_to_log(self, exception_mock):
    logger = router._LoggingStreamRouter(logging.WARN)
    event = LogRequestLite.LogEventLite()
    event.event_time_ms = router.time_ms()
    event.event_code = 1
    event.event_flow_id = 2
    event.source_extension = 'not-a-message'
    self.assertFalse(logger.push_event(event))
    self.assertEqual(exception_mock.call_args[0], ('Unable to log the events',))
