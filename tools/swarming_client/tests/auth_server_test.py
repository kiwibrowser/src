#!/usr/bin/env python
# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import contextlib
import json
import logging
import os
import socket
import sys
import time
import unittest

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(
    __file__.decode(sys.getfilesystemencoding()))))
sys.path.insert(0, ROOT_DIR)
sys.path.insert(0, os.path.join(ROOT_DIR, 'third_party'))

from depot_tools import auto_stub
from depot_tools import fix_encoding
from third_party import requests

from utils import authenticators
from utils import auth_server
from utils import net
from utils import oauth

from libs import luci_context

import net_utils


def global_test_setup():
  # Terminate HTTP server in tests 50x faster. Impacts performance though so
  # do it only in tests.
  auth_server._HTTPServer.poll_interval = 0.01


def call_rpc(account_id, scopes):
  ctx = luci_context.read('local_auth')
  r = requests.post(
      url='http://127.0.0.1:%d/rpc/LuciLocalAuthService.GetOAuthToken' %
          ctx['rpc_port'],
      data=json.dumps({
        'account_id': account_id,
        'scopes': scopes,
        'secret': ctx['secret'],
      }),
      headers={'Content-Type': 'application/json'})
  return r.json()


@contextlib.contextmanager
def local_auth_server(token_cb, default_account_id, **overrides):
  class MockedProvider(object):
    def generate_token(self, account_id, scopes):
      return token_cb(account_id, scopes)

  acc = lambda aid: auth_server.Account(id=aid, email=aid+'@example.com')

  s = auth_server.LocalAuthServer()
  try:
    local_auth = s.start(
        token_provider=MockedProvider(),
        accounts=(acc('acc_1'), acc('acc_2'), acc('acc_3')),
        default_account_id=default_account_id)
    local_auth.update(overrides)
    with luci_context.write(local_auth=local_auth):
      yield
  finally:
    s.stop()


class LocalAuthServerTest(auto_stub.TestCase):
  epoch = 12345678

  def setUp(self):
    super(LocalAuthServerTest, self).setUp()
    self.mock_time(0)

  def mock_time(self, delta):
    self.mock(time, 'time', lambda: self.epoch + delta)

  def test_works(self):
    calls = []
    def token_gen(account_id, scopes):
      calls.append((account_id, scopes))
      return auth_server.AccessToken('tok_%s' % account_id, time.time() + 300)

    with local_auth_server(token_gen, 'acc_1'):
      # Accounts are set correctly.
      ctx = luci_context.read('local_auth')
      ctx.pop('rpc_port')
      ctx.pop('secret')
      self.assertEqual({
          'accounts': [
              {'email': 'acc_1@example.com', 'id': 'acc_1'},
              {'email': 'acc_2@example.com', 'id': 'acc_2'},
              {'email': 'acc_3@example.com', 'id': 'acc_3'},
          ],
         'default_account_id': 'acc_1',
      }, ctx)

      # Grab initial token.
      resp = call_rpc('acc_1', ['B', 'B', 'A', 'C'])
      self.assertEqual(
          {u'access_token': u'tok_acc_1', u'expiry': self.epoch + 300}, resp)
      self.assertEqual([('acc_1', ('A', 'B', 'C'))], calls)
      del calls[:]

      # Reuses cached token until it is close to expiration.
      self.mock_time(60)
      resp = call_rpc('acc_1', ['B', 'A', 'C'])
      self.assertEqual(
          {u'access_token': u'tok_acc_1', u'expiry': self.epoch + 300}, resp)
      self.assertFalse(calls)

      # Asking for different account gives another token.
      resp = call_rpc('acc_2', ['B', 'B', 'A', 'C'])
      self.assertEqual(
          {u'access_token': u'tok_acc_2', u'expiry': self.epoch + 360}, resp)
      self.assertEqual([('acc_2', ('A', 'B', 'C'))], calls)
      del calls[:]

      # First token has expired. Generated new one.
      self.mock_time(300)
      resp = call_rpc('acc_1', ['A', 'B', 'C'])
      self.assertEqual(
          {u'access_token': u'tok_acc_1', u'expiry': self.epoch + 600}, resp)
      self.assertEqual([('acc_1', ('A', 'B', 'C'))], calls)

  def test_handles_token_errors(self):
    calls = []
    def token_gen(_account_id, _scopes):
      calls.append(1)
      raise auth_server.TokenError(123, 'error message')

    with local_auth_server(token_gen, 'acc_1'):
      self.assertEqual(
          {u'error_code': 123, u'error_message': u'error message'},
          call_rpc('acc_1', ['B', 'B', 'A', 'C']))
      self.assertEqual(1, len(calls))

      # Errors are cached. Same error is returned.
      self.assertEqual(
          {u'error_code': 123, u'error_message': u'error message'},
          call_rpc('acc_1', ['B', 'B', 'A', 'C']))
      self.assertEqual(1, len(calls))

  def test_http_level_errors(self):
    def token_gen(_account_id, _scopes):
      self.fail('must not be called')

    with local_auth_server(token_gen, 'acc_1'):
      # Wrong URL.
      ctx = luci_context.read('local_auth')
      r = requests.post(
          url='http://127.0.0.1:%d/blah/LuciLocalAuthService.GetOAuthToken' %
              ctx['rpc_port'],
          data=json.dumps({
            'account_id': 'acc_1',
            'scopes': ['A', 'B', 'C'],
            'secret': ctx['secret'],
          }),
          headers={'Content-Type': 'application/json'})
      self.assertEqual(404, r.status_code)

      # Wrong HTTP method.
      r = requests.get(
          url='http://127.0.0.1:%d/rpc/LuciLocalAuthService.GetOAuthToken' %
              ctx['rpc_port'],
          data=json.dumps({
            'account_id': 'acc_1',
            'scopes': ['A', 'B', 'C'],
            'secret': ctx['secret'],
          }),
          headers={'Content-Type': 'application/json'})
      self.assertEqual(501, r.status_code)

      # Wrong content type.
      r = requests.post(
          url='http://127.0.0.1:%d/rpc/LuciLocalAuthService.GetOAuthToken' %
              ctx['rpc_port'],
          data=json.dumps({
            'account_id': 'acc_1',
            'scopes': ['A', 'B', 'C'],
            'secret': ctx['secret'],
          }),
          headers={'Content-Type': 'application/xml'})
      self.assertEqual(400, r.status_code)

      # Bad JSON.
      r = requests.post(
          url='http://127.0.0.1:%d/rpc/LuciLocalAuthService.GetOAuthToken' %
              ctx['rpc_port'],
          data='not a json',
          headers={'Content-Type': 'application/json'})
      self.assertEqual(400, r.status_code)

  def test_validation(self):
    def token_gen(_account_id, _scopes):
      self.fail('must not be called')

    with local_auth_server(token_gen, 'acc_1'):
      ctx = luci_context.read('local_auth')

      def must_fail(body, err, code):
        r = requests.post(
            url='http://127.0.0.1:%d/rpc/LuciLocalAuthService.GetOAuthToken' %
                ctx['rpc_port'],
            data=json.dumps(body),
            headers={'Content-Type': 'application/json'})
        self.assertEqual(code, r.status_code)
        self.assertIn(err, r.text)

      cases = [
        # account_id
        ({}, '"account_id" is required', 400),
        ({'account_id': 123}, '"account_id" must be a string', 400),

        # scopes
        ({'account_id': 'acc_1'}, '"scopes" is required', 400),
        ({'account_id': 'acc_1', 'scopes': []}, '"scopes" is required', 400),
        (
          {'account_id': 'acc_1', 'scopes': 'abc'},
          '"scopes" must be a list of strings',
          400,
        ),
        (
          {'account_id': 'acc_1', 'scopes': [1]},
          '"scopes" must be a list of strings',
          400,
        ),

        # secret
        ({'account_id': 'acc_1', 'scopes': ['a']}, '"secret" is required', 400),
        (
          {'account_id': 'acc_1', 'scopes': ['a'], 'secret': 123},
          '"secret" must be a string',
          400,
        ),
        (
          {'account_id': 'acc_1', 'scopes': ['a'], 'secret': 'abc'},
          'Invalid "secret"',
          403,
        ),

        # The account is known.
        (
          {'account_id': 'zzz', 'scopes': ['a'], 'secret': ctx['secret']},
          'Unrecognized account ID',
          404,
        ),
      ]
      for body, err, code in cases:
        must_fail(body, err, code)


class LocalAuthHttpServiceTest(auto_stub.TestCase):
  """Tests for LocalAuthServer and LuciContextAuthenticator."""
  epoch = 12345678

  def setUp(self):
    super(LocalAuthHttpServiceTest, self).setUp()
    self.mock_time(0)

  def mock_time(self, delta):
    self.mock(time, 'time', lambda: self.epoch + delta)

  @staticmethod
  def mocked_http_service(
      url='http://example.com',
      perform_request=None):

    class MockedRequestEngine(object):
      def perform_request(self, request):
        return perform_request(request) if perform_request else None

    return net.HttpService(
        url,
        authenticator=authenticators.LuciContextAuthenticator(),
        engine=MockedRequestEngine())

  def test_works(self):
    service_url = 'http://example.com'
    request_url = '/some_request'
    response = 'True'
    token = 'notasecret'

    def token_gen(account_id, scopes):
      self.assertEqual('acc_1', account_id)
      self.assertEqual(1, len(scopes))
      self.assertEqual(oauth.OAUTH_SCOPES, scopes[0])
      return auth_server.AccessToken(token, time.time() + 300)

    def handle_request(request):
      self.assertTrue(
          request.get_full_url().startswith(service_url + request_url))
      self.assertEqual('', request.body)
      self.assertEqual(u'Bearer %s' % token,
                       request.headers['Authorization'])
      return net_utils.make_fake_response(response, request.get_full_url())

    with local_auth_server(token_gen, 'acc_1'):
      service = self.mocked_http_service(perform_request=handle_request)
      self.assertEqual(service.request(request_url, data={}).read(), response)

  def test_bad_secret(self):
    service_url = 'http://example.com'
    request_url = '/some_request'
    response = 'False'

    def token_gen(_account_id, _scopes):
      self.fail('must not be called')

    def handle_request(request):
      self.assertTrue(
          request.get_full_url().startswith(service_url + request_url))
      self.assertEqual('', request.body)
      self.assertIsNone(request.headers.get('Authorization'))
      return net_utils.make_fake_response(response, request.get_full_url())

    with local_auth_server(token_gen, 'acc_1', secret='invalid'):
      service = self.mocked_http_service(perform_request=handle_request)
      self.assertEqual(service.request(request_url, data={}).read(), response)

  def test_bad_port(self):
    request_url = '/some_request'

    def token_gen(_account_id, _scopes):
      self.fail('must not be called')

    def handle_request(_request):
      self.fail('must not be called')

    # This little dance should pick an unused port, bind it and then close it,
    # trusting that the OS will not reallocate it between now and when the http
    # client attempts to use it as a local_auth service. This is better than
    # picking a static port number, as there's at least some guarantee that the
    # port WASN'T in use before this test ran.
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(('localhost', 0))
    port = sock.getsockname()[1]
    sock.close()
    with local_auth_server(token_gen, 'acc_1', rpc_port=port):
      service = self.mocked_http_service(perform_request=handle_request)
      with self.assertRaises(socket.error):
        self.assertRaises(service.request(request_url, data={}).read())

  def test_expired_token(self):
    service_url = 'http://example.com'
    request_url = '/some_request'
    response = 'False'
    token = 'notasecret'

    def token_gen(account_id, scopes):
      self.assertEqual('acc_1', account_id)
      self.assertEqual(1, len(scopes))
      self.assertEqual(oauth.OAUTH_SCOPES, scopes[0])
      return auth_server.AccessToken(token, time.time())

    def handle_request(request):
      self.assertTrue(
          request.get_full_url().startswith(service_url + request_url))
      self.assertEqual('', request.body)
      self.assertIsNone(request.headers.get('Authorization'))
      return net_utils.make_fake_response(response, request.get_full_url())

    with local_auth_server(token_gen, 'acc_1'):
      service = self.mocked_http_service(perform_request=handle_request)
      self.assertEqual(service.request(request_url, data={}).read(), response)


if __name__ == '__main__':
  fix_encoding.fix_encoding()
  logging.basicConfig(
      level=logging.DEBUG if '-v' in sys.argv else logging.CRITICAL)
  global_test_setup()
  unittest.main()
