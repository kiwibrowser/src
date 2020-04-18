# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for som."""

from __future__ import print_function

import json
import mock

from chromite.lib import auth
from chromite.lib import cros_test_lib
from chromite.lib import som


class SheriffOMaticClientTest(cros_test_lib.MockTestCase):
  """Tests for SheriffOMaticClient."""
  def setUp(self):
    self.mock_http = mock.MagicMock()
    self.PatchObject(auth, 'AuthorizedHttp', return_value=self.mock_http)
    self.client = som.SheriffOMaticClient(host='foo.com')

  def testSendRequest(self):
    """Test SendRequest."""
    self.mock_http.request.return_value = ({'status': 200}, 'test')
    self.assertEqual(self.client.SendRequest('http://foo.com/abc',
                                             som.POST_METHOD, 'in', False),
                     'test')
    self.assertEqual(self.mock_http.request.call_count, 1)

  def testSendRequestFailure(self):
    """Test SendRequest with bad HTTP status code."""
    self.mock_http.request.return_value = ({'status': 400}, 'test')
    self.assertRaises(som.SheriffOMaticResponseException,
                      self.client.SendRequest,
                      'http://foo.com/abc', som.POST_METHOD, 'in', False)
    self.assertEqual(self.mock_http.request.call_count, 4)

  def testSendAlerts(self):
    """Test SendAlerts."""
    self.mock_http.request.return_value = ({'status': 200}, '')
    body = json.dumps({'test': 'stuff'})
    self.assertEqual(self.client.SendAlerts(body), '')
    self.mock_http.request.assert_called_with(
        'https://foo.com/api/v1/alerts/chromeos', som.POST_METHOD,
        body=body, headers={'Content-Type': 'application/json'})
    self.assertEqual(self.mock_http.request.call_count, 1)

  def testSendAlertsSpaces(self):
    """Test SendAlerts with space in hostname."""
    self.client = som.SheriffOMaticClient(host='bar.com ')
    self.mock_http.request.return_value = ({'status': 200}, '')
    body = json.dumps({'test': 'stuff'})
    self.assertEqual(self.client.SendAlerts(body), '')
    self.mock_http.request.assert_called_with(
        'https://bar.com/api/v1/alerts/chromeos', som.POST_METHOD,
        body=body, headers={'Content-Type': 'application/json'})
    self.assertEqual(self.mock_http.request.call_count, 1)
