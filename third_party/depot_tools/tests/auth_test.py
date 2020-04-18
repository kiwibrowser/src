#!/usr/bin/env python
# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit Tests for auth.py"""

import __builtin__
import datetime
import json
import logging
import os
import unittest
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


from testing_support import auto_stub
from third_party import httplib2
from third_party import mock

import auth


class TestLuciContext(auto_stub.TestCase):
  def setUp(self):
    auth._get_luci_context_local_auth_params.clear_cache()

  def _mock_local_auth(self, account_id, secret, rpc_port):
    self.mock(os, 'environ', {'LUCI_CONTEXT': 'default/test/path'})
    self.mock(auth, '_load_luci_context', mock.Mock())
    auth._load_luci_context.return_value = {
      'local_auth': {
        'default_account_id': account_id,
        'secret': secret,
        'rpc_port': rpc_port,
      }
    }

  def _mock_loc_server_resp(self, status, content):
    mock_resp = mock.Mock()
    mock_resp.status = status
    self.mock(httplib2.Http, 'request', mock.Mock())
    httplib2.Http.request.return_value = (mock_resp, content)

  def test_all_good(self):
    self._mock_local_auth('account', 'secret', 8080)
    self.assertTrue(auth.has_luci_context_local_auth())

    expiry_time = datetime.datetime.min + datetime.timedelta(hours=1)
    resp_content = {
      'error_code': None,
      'error_message': None,
      'access_token': 'token',
      'expiry': (expiry_time
                 - datetime.datetime.utcfromtimestamp(0)).total_seconds(),
    }
    self._mock_loc_server_resp(200, json.dumps(resp_content))
    params = auth._get_luci_context_local_auth_params()
    token = auth._get_luci_context_access_token(params, datetime.datetime.min)
    self.assertEquals(token.token, 'token')

  def test_no_account_id(self):
    self._mock_local_auth(None, 'secret', 8080)
    self.assertFalse(auth.has_luci_context_local_auth())
    self.assertIsNone(auth.get_luci_context_access_token())

  def test_incorrect_port_format(self):
    self._mock_local_auth('account', 'secret', 'port')
    self.assertFalse(auth.has_luci_context_local_auth())
    with self.assertRaises(auth.LuciContextAuthError):
      auth.get_luci_context_access_token()

  def test_expired_token(self):
    params = auth._LuciContextLocalAuthParams('account', 'secret', 8080)
    resp_content = {
      'error_code': None,
      'error_message': None,
      'access_token': 'token',
      'expiry': 1,
    }
    self._mock_loc_server_resp(200, json.dumps(resp_content))
    with self.assertRaises(auth.LuciContextAuthError):
      auth._get_luci_context_access_token(
          params, datetime.datetime.utcfromtimestamp(1))

  def test_incorrect_expiry_format(self):
    params = auth._LuciContextLocalAuthParams('account', 'secret', 8080)
    resp_content = {
      'error_code': None,
      'error_message': None,
      'access_token': 'token',
      'expiry': 'dead',
    }
    self._mock_loc_server_resp(200, json.dumps(resp_content))
    with self.assertRaises(auth.LuciContextAuthError):
      auth._get_luci_context_access_token(params, datetime.datetime.min)

  def test_incorrect_response_content_format(self):
    params = auth._LuciContextLocalAuthParams('account', 'secret', 8080)
    self._mock_loc_server_resp(200, '5')
    with self.assertRaises(auth.LuciContextAuthError):
      auth._get_luci_context_access_token(params, datetime.datetime.min)


if __name__ == '__main__':
  if '-v' in sys.argv:
    logging.basicConfig(level=logging.DEBUG)
  unittest.main()
