#!/usr/bin/env python
# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import collections
import contextlib
import datetime
import json
import logging
import os
import sys
import time
import unittest

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(
    __file__.decode(sys.getfilesystemencoding()))))
sys.path.insert(0, ROOT_DIR)
sys.path.insert(0, os.path.join(ROOT_DIR, 'third_party'))

import httplib2

from depot_tools import auto_stub
from depot_tools import fix_encoding

from utils import oauth


class LuciContextAuthTest(auto_stub.TestCase):
  @contextlib.contextmanager
  def lucicontext(self, token_response=None, response_code=200):
    class MockLuciContextServer(object):
      @staticmethod
      def request(uri=None, method=None, body=None, headers=None):
        self.assertEqual('POST', method)
        self.assertEqual(
            'http://127.0.0.1:0/rpc/LuciLocalAuthService.GetOAuthToken', uri)
        self.assertEqual({'Content-Type': 'application/json'}, headers)
        data = json.loads(body)
        self.assertEqual('secret', data['secret'])
        self.assertEqual(1, len(data['scopes']))
        self.assertEqual('acc_a', data['account_id'])
        self.assertEqual('https://www.googleapis.com/auth/userinfo.email',
                         data['scopes'][0])
        response = collections.namedtuple('HttpResponse', ['status'])
        response.status = response_code
        content = json.dumps(token_response)
        return response, content
    self.mock_local_auth(MockLuciContextServer())
    yield

  def setUp(self):
    super(LuciContextAuthTest, self).setUp()
    self.mock_time(0)

  def mock_local_auth(self, server):
    def load_local_auth():
      return oauth.LocalAuthParameters(
          rpc_port=0,
          secret='secret',
          accounts=[oauth.LocalAuthAccount('acc_a')],
          default_account_id='acc_a')
    self.mock(oauth, '_load_local_auth', load_local_auth)
    def http_server():
      return server
    self.mock(httplib2, 'Http', http_server)

  def mock_time(self, delta):
    self.mock(time, 'time', lambda: delta)

  def _utc_datetime(self, delta):
    return datetime.datetime.utcfromtimestamp(delta)

  def test_get_access_token(self):
    t_expire = 100
    with self.lucicontext({'access_token': 'notasecret', 'expiry': t_expire}):
      token = oauth._get_luci_context_access_token(oauth._load_local_auth())
    self.assertEqual('notasecret', token.token)
    self.assertEqual(self._utc_datetime(t_expire), token.expires_at)

  def test_get_missing_token(self):
    t_expire = 100
    with self.lucicontext({'expiry': t_expire}):
      token = oauth._get_luci_context_access_token(oauth._load_local_auth())
    self.assertIsNone(token)

  def test_get_missing_expiry(self):
    with self.lucicontext({'access_token': 'notasecret'}):
      token = oauth._get_luci_context_access_token(oauth._load_local_auth())
    self.assertIsNone(token)

  def test_get_access_token_with_errors(self):
    with self.lucicontext({'error_code': 5, 'error_msg': 'fail'}):
      token = oauth._get_luci_context_access_token(oauth._load_local_auth())
    self.assertIsNone(token)

  def test_validation(self):
    t_expire = self._utc_datetime(120)
    good_token = oauth.AccessToken("secret", t_expire)
    self.assertEqual(
        True,
        oauth._validate_luci_context_access_token(good_token))
    no_token = oauth.AccessToken("", t_expire)
    self.assertEqual(
        False,
        oauth._validate_luci_context_access_token(no_token))
    not_a_token = {'token': "secret", 'expires_at': t_expire}
    self.assertEqual(
        False,
        oauth._validate_luci_context_access_token(not_a_token))

    self.mock_time(50)

    self.assertEqual(
        True,
        oauth._validate_luci_context_access_token(good_token))

    self.mock_time(100)

    self.assertEqual(
        False,
        oauth._validate_luci_context_access_token(good_token))


if __name__ == '__main__':
  fix_encoding.fix_encoding()
  logging.basicConfig(
      level=logging.DEBUG if '-v' in sys.argv else logging.CRITICAL)
  unittest.main()
