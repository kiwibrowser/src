# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import httplib2
import json
import os
import tempfile
import unittest

from googleapiclient import errors
import mock

from infra_libs import httplib2_utils
from infra_libs.ts_mon.common import interface
from infra_libs.ts_mon.common import monitors
from infra_libs.ts_mon.common import pb_to_popo
from infra_libs.ts_mon.common import targets
from infra_libs.ts_mon.protos import metrics_pb2
import infra_libs


class MonitorTest(unittest.TestCase):

  def test_send(self):
    m = monitors.Monitor()
    metric1 = metrics_pb2.MetricsPayload()
    with self.assertRaises(NotImplementedError):
      m.send(metric1)


class HttpsMonitorTest(unittest.TestCase):

  def setUp(self):
    interface.state.reset_for_unittest()

  def message(self, pb):
    return json.dumps({'payload': pb_to_popo.convert(pb)})

  def _test_send(self, http):
    mon = monitors.HttpsMonitor('endpoint',
        monitors.CredentialFactory.from_string(':gce'), http=http)
    resp = mock.MagicMock(spec=httplib2.Response, status=200)
    mon._http.request = mock.MagicMock(return_value=[resp, ""])

    payload = metrics_pb2.MetricsPayload()
    payload.metrics_collection.add().metrics_data_set.add().metric_name = 'a'
    mon.send(payload)

    mon._http.request.assert_has_calls([
      mock.call('endpoint', method='POST', body=self.message(payload),
                headers={'Content-Type': 'application/json'}),
    ])

  def test_default_send(self):
    self._test_send(None)

  def test_http_send(self):
    self._test_send(httplib2.Http())

  def test_instrumented_http_send(self):
    self._test_send(httplib2_utils.InstrumentedHttp('test'))

  @mock.patch('infra_libs.ts_mon.common.monitors.CredentialFactory.'
              'from_string')
  def test_send_resp_failure(self, _load_creds):
    mon = monitors.HttpsMonitor('endpoint',
        monitors.CredentialFactory.from_string('/path/to/creds.p8.json'))
    resp = mock.MagicMock(spec=httplib2.Response, status=400)
    mon._http.request = mock.MagicMock(return_value=[resp, ""])

    metric1 = metrics_pb2.MetricsPayload()
    metric1.metrics_collection.add().metrics_data_set.add().metric_name = 'a'
    mon.send(metric1)

    mon._http.request.assert_called_once_with(
        'endpoint',
        method='POST',
        body=self.message(metric1),
        headers={'Content-Type': 'application/json'})

  @mock.patch('infra_libs.ts_mon.common.monitors.CredentialFactory.'
              'from_string')
  def test_send_http_failure(self, _load_creds):
    mon = monitors.HttpsMonitor('endpoint',
        monitors.CredentialFactory.from_string('/path/to/creds.p8.json'))
    mon._http.request = mock.MagicMock(side_effect=ValueError())

    metric1 = metrics_pb2.MetricsPayload()
    metric1.metrics_collection.add().metrics_data_set.add().metric_name = 'a'
    mon.send(metric1)

    mon._http.request.assert_called_once_with(
        'endpoint',
        method='POST',
        body=self.message(metric1),
        headers={'Content-Type': 'application/json'})


class DebugMonitorTest(unittest.TestCase):

  def test_send_file(self):
    with infra_libs.temporary_directory() as temp_dir:
      filename = os.path.join(temp_dir, 'out')
      m = monitors.DebugMonitor(filename)
      payload = metrics_pb2.MetricsPayload()
      payload.metrics_collection.add().metrics_data_set.add().metric_name = 'm1'
      m.send(payload)
      with open(filename) as fh:
        output = fh.read()
    self.assertIn('metrics_data_set {\n    metric_name: "m1"\n  }', output)

  @mock.patch('logging.info')
  def test_send_log(self, mock_logging_info):
    m = monitors.DebugMonitor()
    payload = metrics_pb2.MetricsPayload()
    payload.metrics_collection.add().metrics_data_set.add().metric_name = 'm1'
    m.send(payload)
    self.assertEqual(1, mock_logging_info.call_count)
    output = mock_logging_info.call_args[0][1]
    self.assertIn('metrics_data_set {\n    metric_name: "m1"\n  }', output)


class NullMonitorTest(unittest.TestCase):

  def test_send(self):
    m = monitors.NullMonitor()
    payload = metrics_pb2.MetricsPayload()
    payload.metrics_collection.add().metrics_data_set.add().metric_name = 'm1'
    m.send(payload)


class CredentialFactoryTest(unittest.TestCase):

  def test_from_string(self):
    self.assertIsInstance(monitors.CredentialFactory.from_string(':gce'),
        monitors.GCECredentials)
    self.assertIsInstance(monitors.CredentialFactory.from_string(':appengine'),
        monitors.AppengineCredentials)
    self.assertIsInstance(monitors.CredentialFactory.from_string('/foo'),
        monitors.FileCredentials)

  @mock.patch('infra_libs.httplib2_utils.DelegateServiceAccountCredentials',
              autospec=True)
  def test_actor_credentials(self, mock_creds):
    base = mock.Mock(monitors.CredentialFactory)
    c = monitors.DelegateServiceAccountCredentials('test@example.com', base)

    creds = c.create(['foo'])
    base.create.assert_called_once_with(['https://www.googleapis.com/auth/iam'])
    base.create.return_value.authorize.assert_called_once_with(mock.ANY)
    mock_creds.assert_called_once_with(
        base.create.return_value.authorize.return_value,
        'test@example.com', ['foo'])
    self.assertEqual(mock_creds.return_value, creds)

  @mock.patch('oauth2client.client.GoogleCredentials.from_stream')
  def test_file_credentials_google(self, mock_from_stream):
    with infra_libs.temporary_directory() as temp_dir:
      path = os.path.join(temp_dir, 'foo')
      with open(path, 'w') as fh:
        json.dump({'type': 'blah'}, fh)

      ret = monitors.FileCredentials(path).create(['bar'])

      mock_from_stream.assert_called_once_with(path)
      creds = mock_from_stream.return_value
      creds.create_scoped.assert_called_once_with(['bar'])
      self.assertEqual(ret, creds.create_scoped.return_value)

  @mock.patch('infra_libs.ts_mon.common.monitors.Storage')
  def test_file_credentials_non_google(self, mock_storage):
    with infra_libs.temporary_directory() as temp_dir:
      path = os.path.join(temp_dir, 'foo')
      with open(path, 'w') as fh:
        json.dump({}, fh)

      ret = monitors.FileCredentials(path).create(['bar'])

      mock_storage.assert_called_once_with(path)
      mock_storage.return_value.get.assert_called_once_with()
      self.assertEqual(ret, mock_storage.return_value.get.return_value)
