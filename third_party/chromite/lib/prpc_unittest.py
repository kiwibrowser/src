# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for prpc."""

from __future__ import print_function

import json
import mock

from chromite.lib import auth
from chromite.lib import cros_test_lib
from chromite.lib import prpc


class TestClient(prpc.PRPCClient):
  """Test PRPCClient object."""
  def _GetHost(self):
    return 'foo.com'


class PRPCCodeTest(cros_test_lib.MockTestCase):
  """Tests for PRPCCode."""
  def testGetCodeString(self):
    """Test GetCodeString."""
    self.assertEqual(prpc.PRPCCode.OK, 0)
    self.assertEqual(prpc.PRPCCode.Unauthenticated, 16)
    self.assertEqual(prpc.GetCodeString(prpc.PRPCCode.OK), 'OK')
    self.assertEqual(prpc.GetCodeString(prpc.PRPCCode.Unauthenticated),
                     'Unauthenticated')
    self.assertEqual(prpc.GetCodeString(-1), 'pRPC code (-1) out of range')
    self.assertEqual(prpc.GetCodeString(prpc.PRPCCode.Unauthenticated + 1),
                     'pRPC code (17) out of range')


class PRPCClientTest(cros_test_lib.MockTestCase):
  """Tests for PRPCClient."""
  def setUp(self):
    self.mock_http = mock.MagicMock()
    self.PatchObject(auth, 'AuthorizedHttp', return_value=self.mock_http)
    self.xssi_prefix = ')]}\'\n'
    self.success_response = {
        'status': 200,
        'x-prpc-grpc-code': 0,
    }
    self.success_content = self.xssi_prefix + json.dumps({
        'key': 'value',
    })
    self.client = TestClient()

  def testConstructURL(self):
    """Test ConstructURL."""
    client = prpc.PRPCClient(insecure=True, host='bar.com')
    self.assertEqual(client.ConstructURL('svc', 'm'), 'http://bar.com/svc/m')
    self.assertEqual(self.client.ConstructURL('svc2', 'm2'),
                     'https://foo.com/svc2/m2')

  def testConstructURLSpaces(self):
    """Test ConstructURL when host has spaces."""
    client = prpc.PRPCClient(insecure=True, host=' baz.com ')
    self.assertEqual(client.ConstructURL('svc', 'm'), 'http://baz.com/svc/m')

  def testSendRequest(self):
    """Test SendRequest."""
    self.mock_http.request.return_value = (self.success_response,
                                           self.success_content)
    resp = self.client.SendRequest('svc', 'm')
    self.assertEqual(resp['key'], 'value')
    self.assertEqual(self.mock_http.request.call_count, 1)

  def testMissingHTTPStatus(self):
    """Test SendRequest with missing HTTP status code."""
    response = {
        'x-prpc-grpc-code': 0,
    }
    self.mock_http.request.return_value = (response, self.success_content)
    self.assertRaises(prpc.PRPCResponseException,
                      self.client.SendRequest, 'svc', 'm')
    self.assertEqual(self.mock_http.request.call_count, 1)

  def testBadHTTPStatus(self):
    """Test SendRequest with bad HTTP status code."""
    response = {
        'status': 203,
        'x-prpc-grpc-code': 0,
    }
    self.mock_http.request.return_value = (response, self.success_content)
    self.assertRaises(prpc.PRPCResponseException,
                      self.client.SendRequest, 'svc', 'm')
    self.assertEqual(self.mock_http.request.call_count, 1)

  def testBadHTTPRetry(self):
    """Test SendRequest with bad HTTP status code causing retry."""
    response = {
        'status': 503,
        'x-prpc-grpc-code': 0,
    }
    self.mock_http.request.return_value = (response, self.success_content)
    self.assertRaises(prpc.PRPCResponseException,
                      self.client.SendRequest, 'svc', 'm')
    self.assertEqual(self.mock_http.request.call_count, 4)

  def testMissingPRPCCode(self):
    """Test SendRequest with missing pRPC status code."""
    response = {
        'status': 200,
    }
    self.mock_http.request.return_value = (response, self.success_content)
    self.assertRaises(prpc.PRPCResponseException,
                      self.client.SendRequest, 'svc', 'm')
    self.assertEqual(self.mock_http.request.call_count, 1)

  def testBadPRPCCode(self):
    """Test SendRequest with bad pRPC status code."""
    response = {
        'status': 200,
        'x-prpc-grpc-code': 1,
    }
    response['x-prpc-grpc-code'] = 1
    self.mock_http.request.return_value = (response, self.success_content)
    self.assertRaises(prpc.PRPCResponseException,
                      self.client.SendRequest, 'svc', 'm')
    self.assertEqual(self.mock_http.request.call_count, 1)

  def testBadPRPCRetry(self):
    """Test SendRequest with bad pRPC status code causing retry."""
    response = {
        'status': 200,
        'x-prpc-grpc-code': prpc.PRPCCode.Unavailable,
    }
    self.mock_http.request.return_value = (response, self.success_content)
    self.assertRaises(prpc.PRPCResponseException,
                      self.client.SendRequest, 'svc', 'm',
                      retry_count=1)
    self.assertEqual(self.mock_http.request.call_count, 2)

  def testInvalidXSSI(self):
    """Test SendRequest with invalid XSSI prefix."""
    content = 'x' + self.success_content
    self.mock_http.request.return_value = (self.success_response, content)
    self.assertRaises(prpc.PRPCResponseException,
                      self.client.SendRequest, 'svc', 'm')
    self.assertEqual(self.mock_http.request.call_count, 1)
