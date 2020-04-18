# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import httplib
import json
import os
import socket
import time
import unittest

import infra_libs
from infra_libs.ts_mon.common import http_metrics
from infra_libs import httplib2_utils
from infra_libs import ts_mon

import httplib2
import mock
import oauth2client.client


DATA_DIR = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data')


class LoadJsonCredentialsTest(unittest.TestCase):
  # Everything's good, should not raise any exceptions.
  def test_valid_credentials(self):
    creds = httplib2_utils.load_service_account_credentials(
      'valid_creds.json',
      service_accounts_creds_root=DATA_DIR)
    self.assertIsInstance(creds, dict)
    self.assertIn('type', creds)
    self.assertIn('client_email', creds)
    self.assertIn('private_key', creds)

  # File exists but issue with the content: raises AuthError.
  def test_missing_type(self):
    with self.assertRaises(infra_libs.AuthError):
      httplib2_utils.load_service_account_credentials(
        'creds_missing_type.json',
        service_accounts_creds_root=DATA_DIR)

  def test_wrong_type(self):
    with self.assertRaises(infra_libs.AuthError):
      httplib2_utils.load_service_account_credentials(
        'creds_wrong_type.json',
        service_accounts_creds_root=DATA_DIR)

  def test_missing_client_email(self):
    with self.assertRaises(infra_libs.AuthError):
      httplib2_utils.load_service_account_credentials(
        'creds_missing_client_email.json',
        service_accounts_creds_root=DATA_DIR)

  def test_missing_private_key(self):
    with self.assertRaises(infra_libs.AuthError):
      httplib2_utils.load_service_account_credentials(
        'creds_missing_private_key.json',
        service_accounts_creds_root=DATA_DIR)

  def test_malformed(self):
    with self.assertRaises(infra_libs.AuthError):
      httplib2_utils.load_service_account_credentials(
        'creds_malformed.json',
        service_accounts_creds_root=DATA_DIR)

  # Problem with the file itself
  def test_file_not_found(self):
    with self.assertRaises(IOError):
      httplib2_utils.load_service_account_credentials(
        'this_file_should_not_exist.json',
        service_accounts_creds_root=DATA_DIR)


class GetSignedJwtAssertionCredentialsTest(unittest.TestCase):
  def test_valid_credentials(self):
    creds = infra_libs.get_signed_jwt_assertion_credentials(
      'valid_creds.json',
      service_accounts_creds_root=DATA_DIR)
    self.assertIsInstance(creds,
                          oauth2client.client.SignedJwtAssertionCredentials)
    # A default scope must be provided, we don't care which one
    self.assertTrue(creds.scope)

  def test_valid_credentials_with_scope_as_string(self):
    creds = infra_libs.get_signed_jwt_assertion_credentials(
      'valid_creds.json',
      scope='repo',
      service_accounts_creds_root=DATA_DIR)
    self.assertIsInstance(creds,
                          oauth2client.client.SignedJwtAssertionCredentials)
    self.assertIn('repo', creds.scope)

  def test_valid_credentials_with_scope_as_list(self):
    creds = infra_libs.get_signed_jwt_assertion_credentials(
      'valid_creds.json',
      scope=['gist'],
      service_accounts_creds_root=DATA_DIR)
    self.assertIsInstance(creds,
                          oauth2client.client.SignedJwtAssertionCredentials)
    self.assertIn('gist', creds.scope)

  # Only test one malformed case and rely on LoadJsonCredentialsTest
  # for the other cases.
  def test_malformed_credentials(self):
    with self.assertRaises(infra_libs.AuthError):
      infra_libs.get_signed_jwt_assertion_credentials(
        'creds_malformed.json',
        service_accounts_creds_root=DATA_DIR)


class GetAuthenticatedHttp(unittest.TestCase):
  def test_valid_credentials(self):
    http = infra_libs.get_authenticated_http(
      'valid_creds.json',
      service_accounts_creds_root=DATA_DIR)
    self.assertIsInstance(http, httplib2.Http)

  def test_valid_credentials_authenticated(self):
    http = infra_libs.get_authenticated_http(
      'valid_creds.json',
      service_accounts_creds_root=DATA_DIR,
      http_identifier='test_case')
    self.assertIsInstance(http, infra_libs.InstrumentedHttp)

  # Only test one malformed case and rely on LoadJsonCredentialsTest
  # for the other cases.
  def test_malformed_credentials(self):
    with self.assertRaises(infra_libs.AuthError):
      infra_libs.get_authenticated_http(
        'creds_malformed.json',
        service_accounts_creds_root=DATA_DIR)

class RetriableHttplib2Test(unittest.TestCase):
  def setUp(self):
    super(RetriableHttplib2Test, self).setUp()
    self.http = infra_libs.RetriableHttp(httplib2.Http())
    self.http._http.request = mock.create_autospec(self.http._http.request,
                                                   spec_set=True)

  _MOCK_REQUEST = mock.call('http://foo/', 'GET', None)

  def test_authorize(self):
    http = infra_libs.RetriableHttp(httplib2.Http())
    creds = infra_libs.get_signed_jwt_assertion_credentials(
      'valid_creds.json',
      service_accounts_creds_root=DATA_DIR)
    creds.authorize(http)

  def test_delegate_get_attr(self):
    """RetriableHttp should delegate getting attribute except request() to
       Http"""
    self.http._http.clear_credentials = mock.create_autospec(
        self.http._http.clear_credentials, spec_set=True)
    self.http.clear_credentials()
    self.http._http.clear_credentials.assert_called_once_with()

  def test_delegate_set_attr(self):
    """RetriableHttp should delegate setting attributes to Http"""
    self.http.ignore_etag = False
    self.assertFalse(self.http.ignore_etag)
    self.assertFalse(self.http._http.ignore_etag)
    self.http.ignore_etag = True
    self.assertTrue(self.http.ignore_etag)
    self.assertTrue(self.http._http.ignore_etag)

  @mock.patch('time.sleep', autospec=True)
  def test_succeed(self, _sleep):
    self.http._http.request.return_value = (
        httplib2.Response({'status': 400}), 'content')
    response, _ = self.http.request('http://foo/')
    self.assertEqual(400, response.status)
    self.http._http.request.assert_has_calls([ self._MOCK_REQUEST ])

  @mock.patch('time.sleep', autospec=True)
  def test_retry_succeed(self, _sleep):
    self.http._http.request.side_effect = iter([
      (httplib2.Response({'status': 500}), 'content'),
      httplib2.HttpLib2Error,
      (httplib2.Response({'status': 200}), 'content')
    ])
    response, _ = self.http.request('http://foo/')
    self.assertEqual(200, response.status)
    self.http._http.request.assert_has_calls([ self._MOCK_REQUEST ] * 3)

  @mock.patch('time.sleep', autospec=True)
  def test_fail_exception(self, _sleep):
    self.http._http.request.side_effect = httplib2.HttpLib2Error()
    self.assertRaises(httplib2.HttpLib2Error, self.http.request, 'http://foo/')
    self.http._http.request.assert_has_calls([ self._MOCK_REQUEST ] * 5)

  @mock.patch('time.sleep', autospec=True)
  def test_fail_status_code(self, _sleep):
    self.http._http.request.return_value = (
        httplib2.Response({'status': 500}), 'content')
    response, _ = self.http.request('http://foo/')
    self.assertEqual(500, response.status)
    self.http._http.request.assert_has_calls([ self._MOCK_REQUEST ] * 5)


class InstrumentedHttplib2Test(unittest.TestCase):
  def setUp(self):
    super(InstrumentedHttplib2Test, self).setUp()
    self.mock_time = mock.create_autospec(time.time, spec_set=True)
    self.mock_time.return_value = 42
    self.http = infra_libs.InstrumentedHttp('test', time_fn=self.mock_time)
    self.http._request = mock.Mock()
    ts_mon.reset_for_unittest()

  def test_success_status(self):
    self.http._request.return_value = (
        httplib2.Response({'status': 200}),
        'content')

    response, _ = self.http.request('http://foo/')
    self.assertEqual(200, response.status)
    self.assertEqual(1, http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2', 'status': 200}))
    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2', 'status': 404}))

  def test_error_status(self):
    self.http._request.return_value = (
        httplib2.Response({'status': 404}),
        'content')

    response, _ = self.http.request('http://foo/')
    self.assertEqual(404, response.status)
    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2', 'status': 200}))
    self.assertEqual(1, http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2', 'status': 404}))

  def test_timeout(self):
    self.http._request.side_effect = socket.timeout

    with self.assertRaises(socket.timeout):
      self.http.request('http://foo/')
    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2', 'status': 200}))
    self.assertEqual(1, http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2',
         'status': http_metrics.STATUS_TIMEOUT}))

  def test_connection_error(self):
    self.http._request.side_effect = socket.error

    with self.assertRaises(socket.error):
      self.http.request('http://foo/')
    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2', 'status': 200}))
    self.assertEqual(1, http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2',
         'status': http_metrics.STATUS_ERROR}))

  def test_exception(self):
    self.http._request.side_effect = httplib2.HttpLib2Error

    with self.assertRaises(httplib2.HttpLib2Error):
      self.http.request('http://foo/')
    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2', 'status': 200}))
    self.assertEqual(1, http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2',
         'status': http_metrics.STATUS_EXCEPTION}))

  def test_httplib_exception(self):
    self.http._request.side_effect = httplib.HTTPException

    with self.assertRaises(httplib.HTTPException):
      self.http.request('http://foo/')
    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2', 'status': 200}))
    self.assertEqual(1, http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2',
         'status': http_metrics.STATUS_EXCEPTION}))

  def test_gae_httplib_timeout_exception(self):
    self.http._request.side_effect = httplib.HTTPException(
        'Deadline exceeded while waiting for HTTP response from URL: '
        'http://foo/')

    with self.assertRaises(httplib.HTTPException):
      self.http.request('http://foo/')
    self.assertIsNone(http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2', 'status': 200}))
    self.assertEqual(1, http_metrics.response_status.get(
        {'name': 'test', 'client': 'httplib2',
         'status': http_metrics.STATUS_TIMEOUT}))

  def test_response_bytes(self):
    self.http._request.return_value = (
        httplib2.Response({'status': 200}),
        'content')

    _, content = self.http.request('http://foo/')
    self.assertEqual('content', content)
    self.assertEqual(1, http_metrics.response_bytes.get(
        {'name': 'test', 'client': 'httplib2'}).count)
    self.assertEqual(7, http_metrics.response_bytes.get(
        {'name': 'test', 'client': 'httplib2'}).sum)

  def test_request_bytes(self):
    self.http._request.return_value = (
        httplib2.Response({'status': 200}),
        'content')

    _, content = self.http.request('http://foo/', body='wibblewibble')
    self.assertEqual('content', content)
    self.assertEqual(1, http_metrics.request_bytes.get(
        {'name': 'test', 'client': 'httplib2'}).count)
    self.assertEqual(12, http_metrics.request_bytes.get(
        {'name': 'test', 'client': 'httplib2'}).sum)

  def test_duration(self):
    current_time = [4.2]

    def time_side_effect():
      ret = current_time[0]
      current_time[0] += 0.3
      return ret
    self.mock_time.side_effect = time_side_effect

    self.http._request.return_value = (
        httplib2.Response({'status': 200}),
        'content')

    _, _ = self.http.request('http://foo/')
    self.assertEqual(1, http_metrics.durations.get(
        {'name': 'test', 'client': 'httplib2'}).count)
    self.assertAlmostEqual(300, http_metrics.durations.get(
        {'name': 'test', 'client': 'httplib2'}).sum)


class HttpMockTest(unittest.TestCase):
  def test_empty(self):
    http = infra_libs.HttpMock([])
    with self.assertRaises(AssertionError):
      http.request('https://www.google.com', 'GET')

  def test_invalid_parameter(self):
    with self.assertRaises(TypeError):
      infra_libs.HttpMock(None)

  def test_uris_wrong_length(self):
    with self.assertRaises(ValueError):
      infra_libs.HttpMock([(1, 2)])

  def test_uris_wrong_type(self):
    with self.assertRaises(ValueError):
      infra_libs.HttpMock([(None,)])

  def test_invalid_uri(self):
    with self.assertRaises(TypeError):
      infra_libs.HttpMock([(1, {'status': '100'}, None)])

  def test_invalid_headers(self):
    with self.assertRaises(TypeError):
      infra_libs.HttpMock([('https://www.google.com', None, None)])

  def test_headers_without_status(self):
    with self.assertRaises(ValueError):
      infra_libs.HttpMock([('https://www.google.com', {'foo': 'bar'}, None)])

  def test_invalid_body(self):
    with self.assertRaises(TypeError):
      infra_libs.HttpMock([('https://www.google.com', {'status': '200'}, 42)])

  def test_one_uri(self):
    http = infra_libs.HttpMock([('https://www.google.com',
                                 {'status': '403'},
                                 'bar')])
    response, body = http.request('https://www.google.com', 'GET')
    self.assertIsInstance(response, httplib2.Response)
    self.assertEqual(response.status, 403)
    self.assertEqual(body, 'bar')

  def test_two_uris(self):
    http = infra_libs.HttpMock([('https://www.google.com',
                                 {'status': 200}, 'foo'),
                                ('.*', {'status': 404}, '')])
    response, body = http.request('https://mywebserver.woo.hoo', 'GET')
    self.assertIsInstance(response, httplib2.Response)
    self.assertEqual(response.status, 404)
    self.assertEqual(body, '')

    self.assertEqual(http.requests_made[0].uri, 'https://mywebserver.woo.hoo')
    self.assertEqual(http.requests_made[0].method, 'GET')
    self.assertEqual(http.requests_made[0].body, None)
    self.assertEqual(http.requests_made[0].headers, None)


class DelegateServiceAccountCredentialsTest(unittest.TestCase):

  def setUp(self):
    self.mock_http = mock.Mock()
    self.c = httplib2_utils.DelegateServiceAccountCredentials(
        self.mock_http, 'test@example.com', ['scope'])

  def tearDown(self):
    mock.patch.stopall()

  def test_sign_blob(self):
    self.mock_http.request.return_value = (
        httplib2.Response({'status': 200}),
        '{"keyId": "foo", "signature": "bar"}')
    key_id, signature = self.c.sign_blob('foo')

    self.mock_http.request.assert_called_once_with(
        'https://iam.googleapis.com/v1/projects/-/serviceAccounts/'
        'test@example.com:signBlob',
        method='POST',
        body='{"bytesToSign": "Zm9v"}',
        headers={'Content-Type': 'application/json'})
    self.assertEqual('foo', key_id)
    self.assertEqual('bar', signature)

  def test_sign_blob_http_error(self):
    self.mock_http.request.return_value = (
        httplib2.Response({'status': 404}), 'Oh dear')
    with self.assertRaises(httplib2_utils.AuthError):
      self.c.sign_blob('foo')

  @mock.patch('infra_libs.httplib2_utils.DelegateServiceAccountCredentials.'
              'sign_blob', autospec=True)
  def test_generate_assertion(self, mock_sign_blob):
    mock_sign_blob.return_value = ('key_id', 'aGVsbG8=')
    ret = self.c._generate_assertion()

    header, payload, signature = (
        self._unpadded_b64decode(x) for x in ret.split('.'))
    payload = json.loads(payload)

    self.assertEqual('{"alg":"RS256","typ":"JWT"}', header)
    self.assertEqual('test@example.com', payload['iss'])
    self.assertEqual('scope', payload['scope'])
    self.assertEqual('https://accounts.google.com/o/oauth2/token',
                     payload['aud'])
    self.assertIn('exp', payload)
    self.assertIn('iat', payload)
    self.assertEqual('hello', signature)

  def _unpadded_b64decode(self, encoded):
    # Pad to a multiple of 4 bytes.
    return base64.b64decode(encoded + ('=' * (len(encoded) % 4)))

