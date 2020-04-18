# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for logdog."""

from __future__ import print_function

import mock
import os
import time

from chromite.lib import auth
from chromite.lib import cros_test_lib
from chromite.lib import logdog


TESTDATA_PATH = os.path.join(os.path.dirname(__file__), 'testdata', 'logdog')


class LogdogClientTest(cros_test_lib.MockTestCase):
  """Tests for LogdogClient."""
  def setUp(self):
    self.mock_request = self.PatchObject(logdog.LogdogClient, 'SendRequest')
    self.PatchObject(logdog.LogdogClient, '_GetHost', return_value='foo.com')
    self.PatchObject(auth, 'GetAccessToken', return_value='token')
    self.PatchObject(time, 'sleep')
    self.client = logdog.LogdogClient()

    text_dict = {
        'lines': [
            {'value': 'hello', 'delimiter': '\n'},
            {'value': 'foo', 'delimiter': '\n'},
            {'delimiter': '\n'},
        ]
    }

    self.get_response = {
        'logs': [
            {'text': text_dict,},
            {'streamIndex': 1, 'text': text_dict,},
            {'streamIndex': 2,},
        ],
        'state': {'terminalIndex': 3,}
    }

    self.get_response2 = {
        'logs': [
            {'streamIndex': 3,},
        ],
        'state': {'terminalIndex': 3,}
    }

    self.logs_response = {
        'logs': [{'text': text_dict}],
    }

  def testLogsQueryAnnotations(self):
    """Test LogsQueryAnnotations."""
    self.mock_request.return_value = {
        'streams': [{
            'path': 'prefix/+/name',
        }]
    }
    resp = self.client.LogsQueryAnnotations('chromeos', 'elm-paladin', 1535)
    self.assertEqual(resp, 'prefix/+/name')
    self.mock_request.assert_called_with('prpc/logdog.Logs', 'Query', mock.ANY,
                                         dryrun=False)
    body_json = self.mock_request.call_args[0][2]
    self.assertIn('chromeos', body_json)
    self.assertIn('elm-paladin', body_json)

  def testLogsQueryAnnotationsEmpty(self):
    """Test LogsQueryAnnotations with empty response."""
    self.mock_request.return_value = {
        'streams': [],
    }
    resp = self.client.LogsQueryAnnotations('chromeos', 'elm-paladin', 1535)
    self.assertEqual(resp, None)

  def testLogsQueryAnnotationsInvalid(self):
    """Test LogsQueryAnnotations with invalid stream."""
    self.mock_request.return_value = {
        'streams': [{}]
    }
    self.assertRaises(logdog.LogdogResponseException,
                      self.client.LogsQueryAnnotations,
                      'chromeos', 'elm-paladin', 1535)

  def testLogsGet(self):
    """Test LogsGet."""
    self.mock_request.return_value = 'foo'
    resp = self.client.LogsGet('proj', 'path', index=237, byte_count=1982,
                               log_count=88821)
    self.assertEqual(resp, 'foo')
    self.mock_request.assert_called_with('prpc/logdog.Logs', 'Get', mock.ANY,
                                         dryrun=False)
    body_json = self.mock_request.call_args[0][2]
    self.assertIn('proj', body_json)
    self.assertIn('path', body_json)
    self.assertIn('237', body_json)
    self.assertIn('88821', body_json)

  def testLogsGetChunked(self):
    """Test LogsGetChunked."""
    self.mock_request.side_effect = [self.get_response, self.get_response2]
    resp = self.client.LogsGetChunked('proj', 'path', index=0, log_count=3)
    self.assertEqual(list(resp), [self.get_response])
    self.mock_request.assert_called_with('prpc/logdog.Logs', 'Get', mock.ANY,
                                         dryrun=False)
    body_json = self.mock_request.call_args[0][2]
    self.assertIn('proj', body_json)
    self.assertIn('path', body_json)
    self.assertEqual(1, self.mock_request.call_count)

  def testLogsGetChunkedShort(self):
    """Test LogsGetChunked with log_count being short of initial response."""
    self.mock_request.side_effect = [self.get_response, self.get_response2]
    resp = self.client.LogsGetChunked('proj', 'path', index=0, log_count=2)
    self.assertEqual(list(resp), [self.get_response])
    self.mock_request.assert_called_with('prpc/logdog.Logs', 'Get', mock.ANY,
                                         dryrun=False)
    body_json = self.mock_request.call_args[0][2]
    self.assertIn('proj', body_json)
    self.assertIn('path', body_json)
    self.assertEqual(1, self.mock_request.call_count)

  def testLogsGetChunkedMultiple(self):
    """Test LogsGetChunked with multiple chunks."""
    self.mock_request.side_effect = [self.get_response, self.get_response2]
    resp = self.client.LogsGetChunked('proj', 'path', index=0, log_count=4)
    self.assertEqual(list(resp), [self.get_response, self.get_response2])
    self.mock_request.assert_called_with('prpc/logdog.Logs', 'Get', mock.ANY,
                                         dryrun=False)
    body_json = self.mock_request.call_args[0][2]
    self.assertIn('proj', body_json)
    self.assertIn('path', body_json)
    self.assertEqual(2, self.mock_request.call_count)

  def testLogsGetChunkedTerminal(self):
    """Test LogsGetChunked stopping with terminal index."""
    self.mock_request.side_effect = [self.get_response, self.get_response2]
    resp = self.client.LogsGetChunked('proj', 'path', index=0, log_count=0)
    self.assertEqual(list(resp), [self.get_response, self.get_response2])
    self.mock_request.assert_called_with('prpc/logdog.Logs', 'Get', mock.ANY,
                                         dryrun=False)
    body_json = self.mock_request.call_args[0][2]
    self.assertIn('proj', body_json)
    self.assertIn('path', body_json)
    self.assertEqual(2, self.mock_request.call_count)

  def testLogsGetChunkedRetry(self):
    """Test LogsGetChunked retry and timeout."""
    empty_response = {
        'logs': [
        ],
        'state': {'terminalIndex': 3,}
    }
    self.mock_request.return_value = empty_response
    resp = self.client.LogsGetChunked('proj', 'path', index=0, log_count=0)
    self.assertRaises(logdog.LogdogTimeoutException, list, resp)
    self.mock_request.assert_called_with('prpc/logdog.Logs', 'Get', mock.ANY,
                                         dryrun=False)
    body_json = self.mock_request.call_args[0][2]
    self.assertIn('proj', body_json)
    self.assertIn('path', body_json)
    self.assertEqual(4, self.mock_request.call_count)

  def testExtractLines(self):
    """Test ExtractLines."""
    self.assertEquals(['hello\n', 'foo\n', '\n'],
                      list(self.client.ExtractLines(self.logs_response)))

  def testGetLines(self):
    """Test GetLines."""
    self.mock_request.side_effect = [self.get_response, self.get_response2]
    resp = self.client.GetLines('proj', 'path', index=0, log_count=4)
    self.assertEquals(['hello\n', 'foo\n', '\n', 'hello\n', 'foo\n', '\n'],
                      list(resp))
    body_json = self.mock_request.call_args[0][2]
    self.assertIn('proj', body_json)
    self.assertIn('path', body_json)
    self.assertEqual(2, self.mock_request.call_count)

  def testLogsTail(self):
    """Test LogsTail."""
    self.mock_request.return_value = 'foo'
    resp = self.client.LogsTail('hello', 'proj')
    self.assertEqual(resp, 'foo')
    self.mock_request.assert_called_with('prpc/logdog.Logs', 'Tail', mock.ANY,
                                         dryrun=False)
    body_json = self.mock_request.call_args[0][2]
    self.assertIn('hello', body_json)
    self.assertIn('proj', body_json)

  def testGetAnnotations(self):
    """Test GetAnnotations."""
    # Protobuf corresponding to: chromeos elm-paladin 1535
    with open(os.path.join(TESTDATA_PATH,
                           'LogdogClientTest.testGetAnnotations.data')) as f:
      proto = f.read()
    mock_query = self.PatchObject(logdog.LogdogClient, 'LogsQueryAnnotations')
    mock_query.return_value = 'prefx/+/stream'
    self.mock_request.return_value = {'logs': [{'datagram': {'data': proto}}]}
    annotations, prefix = self.client.GetAnnotations('chromeos', 'elm-paladin',
                                                     1535)
    self.assertEqual(prefix, 'prefx')
    self.assertEqual(annotations.substep[5].step.name, 'ManifestVersionedSync')

  def testGetAnnotationsInvalidResponse(self):
    """Test GetAnnotations with an invalid response."""
    mock_query = self.PatchObject(logdog.LogdogClient, 'LogsQueryAnnotations')
    mock_query.return_value = 'prefx/+/stream'
    self.mock_request.return_value = {'logs': [{'datagram': {}}]}
    self.assertRaises(logdog.LogdogResponseException,
                      self.client.GetAnnotations,
                      'chromeos', 'elm-paladin', 1535)

  def testConstructViewerURL(self):
    """Test ConstructViewerURL."""
    url = self.client.ConstructViewerURL('proj', 'prefium', 'foobarbaz')
    self.assertIn('foo.com', url)
    self.assertIn('proj', url)
    self.assertIn('prefium', url)
