# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tests for users_id_token and validate_id_token."""

import base64
import json
import os
import string
import time
import unittest

import endpoints.api_config as api_config

import mox
from protorpc import message_types
from protorpc import messages
from protorpc import remote

import test_util
import endpoints.users_id_token as users_id_token

from google.appengine.api import memcache
from google.appengine.api import oauth
from google.appengine.api import urlfetch
from google.appengine.api import users


# The key response that allows the _SAMPLE_TOKEN to be verified.  This key was
# retrieved from:
# http://www-googleapis-test.sandbox.google.com//oauth2/v1/raw_public_keys
# ...at the same time that _SAMPLE_TOKEN was generated.
# The first cert is too short, which caused an exception 'Plaintext too large'
# from RSA.encrypt (b/19127342); the second cert is the correct cert. Put both
# there to make sure the second cert is tried when the first failed.

_CACHED_CERT = {
    'keyvalues': [
        {
            'algorithm': 'RSA',
            'modulus': ('2bqhkZ+DZSuQvHX3rdoIni39gfl6zny0WZK6dLPP2lRmer1aEAP982'
                        'u2B1siXoXB8HN+pwCZMGV5kbHaG13InopeVNIMFl2IU4aql+hDS0+i'
                        'j+1Rrsa6wHWp4+3eKe9q+VqXMdulclegHjVtxDs76W1lpuP1e6Msc3'
                        'IuSXjR'),
            'exponent': 'AQAB',
            'keyid': '458790a80f9c9957e8df61332b9f06faa6472bad'
        },
        {
            'algorithm': 'RSA',
            'modulus': ('AL18Q+dq5ws4/V7KtgfhC6SwJH20GvUN5z3vf4SUSrpQG2/nySBvUh'
                        'Iv86Hkk4Uy7W+OTq2+csCGhjGnRxBx9BThT85G8F6IGNjcOyNHVtnR'
                        'ifX+T88sUB1l7jAISRMCrgHIRNmwDCmEe1fTqTUOdgDT8nB7pX7SA/'
                        'VH0q+t2xml'),
            'exponent': 'AQAB',
            'keyid': '7411abfccccb4c253cd3e75b4fa5887f49aa83d1'
        },
    ]
}


class ModuleInterfaceTest(test_util.ModuleInterfaceTest,
                          unittest.TestCase):

  MODULE = users_id_token


class TestCache(object):
  """Test stub to replace memcache for id_token verification."""

  def __init__(self):
    self._used_cached_value = False
    self._value_was_set = False

  @property
  def used_cached_value(self):
    return self._used_cached_value

  @property
  def value_was_set(self):
    return self._value_was_set

  # pylint: disable=g-bad-name
  def get(self, key, *unused_args, **kwargs):
    if (key == users_id_token._DEFAULT_CERT_URI and
        kwargs.get('namespace', '') == users_id_token._CERT_NAMESPACE):
      self._used_cached_value = True
      return _CACHED_CERT
    return None

  def set(self, *unused_args, **unused_kwargs):
    self._value_was_set = True


class UsersIdTokenTestBase(unittest.TestCase):
  """A sample token based on JWT.

  Sample token is based on a JWT with this body:
  {
    "iss":"accounts.google.com",
    "email":"kevind@gmail.com",
    "email_verified":"true",
    "aud":"919214422084-c0jrodnkm7ntttjhhttilqjq5d7l7mu5.apps."
          "googleusercontent.com",
    "sub":"104564329451840817415",
    "azp":"919214422084-c0jrodnkm7ntttjhhttilqjq5d7l7mu5.apps."
          "googleusercontent.com",
    "at_hash":"c9aVyHiathUC-pgRFjFWbw",
    "iat":1360964700,
    "exp":1360968600
   }
  """
  _SAMPLE_TOKEN = ('eyJhbGciOiJSUzI1NiIsImtpZCI6Ijc0MTFhYmZjY2NjYjRjMjUzY2QzZTc'
                   '1YjRmYTU4ODdmNDlhYTgzZDEifQ.eyJpc3MiOiJhY2NvdW50cy5nb29nbGU'
                   'uY29tIiwiZW1haWwiOiJrZXZpbmRAZ21haWwuY29tIiwiZW1haWxfdmVyaW'
                   'ZpZWQiOiJ0cnVlIiwiYXVkIjoiOTE5MjE0NDIyMDg0LWMwanJvZG5rbTdud'
                   'HR0amhodHRpbHFqcTVkN2w3bXU1LmFwcHMuZ29vZ2xldXNlcmNvbnRlbnQu'
                   'Y29tIiwic3ViIjoiMTA0NTY0MzI5NDUxODQwODE3NDE1IiwiYXpwIjoiOTE'
                   '5MjE0NDIyMDg0LWMwanJvZG5rbTdudHR0amhodHRpbHFqcTVkN2w3bXU1Lm'
                   'FwcHMuZ29vZ2xldXNlcmNvbnRlbnQuY29tIiwiYXRfaGFzaCI6ImM5YVZ5S'
                   'GlhdGhVQy1wZ1JGakZXYnciLCJpYXQiOjEzNjA5NjQ3MDAsImV4cCI6MTM2'
                   'MDk2ODYwMH0.XwaGmw5n1XHJapwkn6pumK14l9Tiyn1q2C5VeYbvuScNS6Z'
                   '-kdb9mX87Hl2hbdUvHm6TNzabMVTgvHPATjuCAt2lXOpwm8iGnon6vTk5LM'
                   'm0tUAE25IAImvpSc59l0ySd4x2g3BvjauxwaYjkwYJRVczsVlTTB3iKlBhW'
                   'IT01vM')
  _SAMPLE_AUDIENCES = ('919214422084-c0jrodnkm7ntttjhhttilqjq5d7l7mu5.apps.'
                       'googleusercontent.com',)
  _SAMPLE_ALLOWED_CLIENT_IDS = ('919214422084-c0jrodnkm7ntttjhhttilqjq5d7l7mu5.'
                                'apps.googleusercontent.com',
                                '12345.apps.googleusercontent.com')
  _SAMPLE_TIME_NOW = 1360964700
  _SAMPLE_OAUTH_SCOPES = ['https://www.googleapis.com/auth/userinfo.email']
  _SAMPLE_OAUTH_TOKEN_INFO = {
      'issued_to': ('919214422084-c0jrodnkm7ntttjhhttilqjq5d7l7mu5.apps.'
                    'googleusercontent.com'),
      'user_id': '108495933693426793887',
      'expires_in': 3384,
      'access_type': 'online',
      'audience': ('919214422084-c0jrodnkm7ntttjhhttilqjq5d7l7mu5.apps.'
                   'googleusercontent.com'),
      'scope': (
          'https://www.googleapis.com/auth/userinfo.profile '
          'https://www.googleapis.com/auth/userinfo.email'),
      'email': 'kevind@gmail.com',
      'verified_email': True
  }

  def setUp(self):
    self.cache = TestCache()
    self._saved_environ = os.environ.copy()
    if 'AUTH_DOMAIN' not in os.environ:
      os.environ['AUTH_DOMAIN'] = 'gmail.com'
    self.mox = mox.Mox()

  def tearDown(self):
    self.mox.UnsetStubs()
    os.environ = self._saved_environ

  def GetSampleBody(self):
    split_token = self._SAMPLE_TOKEN.split('.')
    body = json.loads(users_id_token._urlsafe_b64decode(split_token[1]))
    return body


class UsersIdTokenTest(UsersIdTokenTestBase):

  def testSampleIdToken(self):
    user = users_id_token._get_id_token_user(self._SAMPLE_TOKEN,
                                             self._SAMPLE_AUDIENCES,
                                             self._SAMPLE_ALLOWED_CLIENT_IDS,
                                             self._SAMPLE_TIME_NOW, self.cache)
    self.assertEqual(user.email(), 'kevind@gmail.com')
    # User ID shouldn't be filled in.  See notes in users_id_token.py.
    self.assertIsNone(user.user_id())
    self.assertTrue(self.cache.used_cached_value)

  def testInvalidSignature(self):
    """Verify that a body that doesn't match the signature fails."""
    body = self.GetSampleBody()
    # Modify the issued and expiration times.
    body['iat'] += 60
    body['exp'] += 60
    encoded_body = base64.urlsafe_b64encode(json.dumps(body))

    split_token = self._SAMPLE_TOKEN.split('.')
    token = '.'.join((split_token[0], encoded_body, split_token[2]))

    self.assertRaises(users_id_token._AppIdentityError,
                      users_id_token._verify_signed_jwt_with_certs,
                      token, self._SAMPLE_TIME_NOW, self.cache)

  def testNoCertRaisesException(self):
    """Verify that if we can't get certs, we fail."""
    self.assertRaises(users_id_token._AppIdentityError,
                      users_id_token._verify_signed_jwt_with_certs,
                      self._SAMPLE_TOKEN, self._SAMPLE_TIME_NOW, self.cache,
                      'https://bad.url/not/in/test/cache')

  def testGetCertExpirationTime(self):
    """Test that we can correctly get cert expiration time from headers."""
    tests = [({'Cache-Control': 'max-age=3600'}, 3600),
             ({'Cache-Control': 'max-age=3600', 'Age': '1200'}, 2400),
             ({}, 0),
             ({'Age': '1'}, 0),
             ({'Cache-Control': 'max-age=3600', 'Age': '3700'}, 0),
             ({'Cache-Control': 'max-age=3600', 'Age': 'bad'}, 3600),
             ({'Cache-Control': 'max-age=nomatch,max-age=1200'}, 1200),
             ({'Cache-Control': 'max-age=invalid'}, 0)]
    for headers, expected_result in tests:
      result = users_id_token._get_cert_expiration_time(headers)
      self.assertEqual(expected_result, result)

  def testCertCacheControl(self):
    """Test that cache control headers are respected."""
    self.mox.StubOutWithMock(urlfetch, 'fetch')
    tests = [({'Cache-Control': 'max-age=3600', 'Age': '1200'}, True),
             ({'Cache-Control': 'max-age=100', 'Age': '100'}, False),
             ({}, False)]
    for test_headers, value_set in tests:

      class DummyResponse(object):
        status_code = 200
        content = json.dumps(self._SAMPLE_OAUTH_TOKEN_INFO)
        headers = test_headers

      urlfetch.fetch(mox.IsA(basestring)).AndReturn(DummyResponse())
      cache = TestCache()

      self.mox.ReplayAll()
      users_id_token._get_cached_certs('some_uri', cache)
      self.mox.VerifyAll()
      self.mox.ResetAll()

      self.assertEqual(value_set, cache.value_was_set)

  def testInvalidTokenExtraSections(self):
    """Verify that a token with too many pieces fails."""
    self.assertRaises(users_id_token._AppIdentityError,
                      users_id_token._verify_signed_jwt_with_certs,
                      self._SAMPLE_TOKEN + '.asdf', self._SAMPLE_TIME_NOW,
                      self.cache)

  def testNoCrypto(self):
    """Verify we throw an _AppIdentityError if the Crypto modules don't load."""
    crypto_loaded = users_id_token._CRYPTO_LOADED
    try:
      users_id_token._CRYPTO_LOADED = False
      self.assertRaises(users_id_token._AppIdentityError,
                        users_id_token._verify_signed_jwt_with_certs,
                        self._SAMPLE_TOKEN, self._SAMPLE_TIME_NOW,
                        self.cache)
    finally:
      users_id_token._CRYPTO_LOADED = crypto_loaded

  def testExpiredToken(self):
    """Verify that expired tokens will fail."""
    expired_time_now = (self._SAMPLE_TIME_NOW +
                        users_id_token._MAX_TOKEN_LIFETIME_SECS + 1)
    self.assertRaises(users_id_token._AppIdentityError,
                      users_id_token._verify_signed_jwt_with_certs,
                      self._SAMPLE_TOKEN, expired_time_now,
                      self.cache)
    # Also verify that this doesn't return a user when called from
    # users_id_token.
    user = users_id_token._get_id_token_user(self._SAMPLE_TOKEN,
                                             self._SAMPLE_AUDIENCES,
                                             self._SAMPLE_ALLOWED_CLIENT_IDS,
                                             expired_time_now, self.cache)
    self.assertIsNone(user)

  def testTimeTooEarly(self):
    """Verify that we'll fail if the provided time_now is too early."""
    early_time_now = (self._SAMPLE_TIME_NOW -
                      users_id_token._CLOCK_SKEW_SECS - 1)
    self.assertRaises(users_id_token._AppIdentityError,
                      users_id_token._verify_signed_jwt_with_certs,
                      self._SAMPLE_TOKEN, early_time_now,
                      self.cache)

  def CheckErrorLoggable(self, token):
    """Verify that the error strings we log are valid, loggable strings."""
    try:
      users_id_token._verify_signed_jwt_with_certs(
          token, self._SAMPLE_TIME_NOW, self.cache)
      self.fail('Expected exception.')
    except users_id_token._AppIdentityError, e:
      # Make sure this works without an exception.
      try:
        str(e).decode('utf-8')
      except UnicodeDecodeError:
        printable = ''.join(c if c in string.printable
                            else '\\x%02x' % ord(c)
                            for c in str(e))
        self.fail('Unsafe error sent to log: %s' % printable)

  def testErrorStringLoggableWrongSegments(self):
    """Check that the Wrong Segments error is loggable."""
    self.CheckErrorLoggable('bad utf-8 \xff')

  def testErrorStringLoggableBadHeader(self):
    """Check that the Bad Header error is loggable."""
    token_part = 'bad utf-8 \xff'
    token = '.'.join([base64.urlsafe_b64encode(token_part)] * 3)
    self.CheckErrorLoggable(token)

  def testErrorStringLoggableBadBody(self):
    """Check that the Unparseable Body error is loggable."""
    token_body = 'bad utf-8 \xff'
    token_parts = self._SAMPLE_TOKEN.split('.')
    token = '.'.join([token_parts[0],
                      base64.urlsafe_b64encode(token_body),
                      token_parts[2]])
    self.CheckErrorLoggable(token)

  def CheckToken(self, field_update_dict, valid):
    """Update the sample token and check if it's valid or invalid.

    This updates the body of our sample token with the fields in
    field_update_dict, then passes it to _verify_parsed_token.  The result must
    match the "valid" parameter.

    Args:
      field_update_dict: A dict of fields to update in the sample body.
      valid: A boolean, compared against the result from _verify_parsed_token.
    """
    parsed_token = self.GetSampleBody()
    parsed_token.update(field_update_dict)
    result = users_id_token._verify_parsed_token(
        parsed_token, self._SAMPLE_AUDIENCES, self._SAMPLE_ALLOWED_CLIENT_IDS)
    self.assertEqual(valid, result)

  def testInvalidIssuer(self):
    self.CheckToken({'iss': 'invalid.issuer'}, False)

  def testInvalidAudience(self):
    self.CheckToken({'aud': 'invalid.audience'}, False)

  def testInvalidClientId(self):
    self.CheckToken({'azp': 'invalid.client.id'}, False)

  def testSampleIdTokenWithOldFields(self):
    self.CheckToken({'cid': 'Extra ignored field.'}, True)

  def testSkipClientIdNotAllowedForIdTokens(self):
    """Verify that SKIP_CLIENT_ID_CHECKS does not work for ID tokens."""
    parsed_token = self.GetSampleBody()
    result = users_id_token._verify_parsed_token(
        parsed_token, self._SAMPLE_AUDIENCES,
        users_id_token.SKIP_CLIENT_ID_CHECK)
    self.assertEqual(False, result)

  def testEmptyAudience(self):
    parsed_token = self.GetSampleBody()
    parsed_token.update({'aud': 'invalid.audience'})
    result = users_id_token._verify_parsed_token(
        parsed_token, [], self._SAMPLE_ALLOWED_CLIENT_IDS)
    self.assertEqual(False, result)

  def AttemptOauth(self, client_id, allowed_client_ids=None):
    if allowed_client_ids is None:
      allowed_client_ids = self._SAMPLE_ALLOWED_CLIENT_IDS
    self.mox.StubOutWithMock(oauth, 'get_client_id')
    # We have four cases:
    #  * no client ID is specified, so we raise for every scope.
    #  * the given client ID is in the whitelist or there is no
    #    whitelist, so we'll only be called once.
    #  * we have a client ID not on the whitelist, so we need a
    #    mock call for every scope.
    if client_id is None:
      for scope in self._SAMPLE_OAUTH_SCOPES:
        oauth.get_client_id(scope).AndRaise(oauth.Error)
    elif (list(allowed_client_ids) == users_id_token.SKIP_CLIENT_ID_CHECK or
          client_id in allowed_client_ids):
      scope = self._SAMPLE_OAUTH_SCOPES[0]
      oauth.get_client_id(scope).AndReturn(client_id)
    else:
      for scope in self._SAMPLE_OAUTH_SCOPES:
        oauth.get_client_id(scope).AndReturn(client_id)

    self.mox.ReplayAll()
    users_id_token._set_bearer_user_vars(allowed_client_ids,
                                         self._SAMPLE_OAUTH_SCOPES)
    self.mox.VerifyAll()

  def assertOauthSucceeded(self, client_id):
    self.AttemptOauth(client_id)
    self.assertEqual(os.environ.get('ENDPOINTS_USE_OAUTH_SCOPE'),
                     self._SAMPLE_OAUTH_SCOPES[0])

  def assertOauthFailed(self, client_id):
    self.AttemptOauth(client_id)
    self.assertNotIn('ENDPOINTS_USE_OAUTH_SCOPE', os.environ)

  def testOauthInvalidClientId(self):
    self.assertOauthFailed('abc.appspot.com')

  def testOauthValidClientId(self):
    self.assertOauthSucceeded(self._SAMPLE_ALLOWED_CLIENT_IDS[0])

  def testOauthExplorerClientId(self):
    self.assertOauthFailed(api_config.API_EXPLORER_CLIENT_ID)

  def testOauthInvalidScope(self):
    self.assertOauthFailed(None)

  def testAllowAllClientIds(self):
    client_id = 'clearly_fake_id'
    self.AttemptOauth(client_id,
                      allowed_client_ids=users_id_token.SKIP_CLIENT_ID_CHECK)
    self.assertEqual(os.environ.get('ENDPOINTS_USE_OAUTH_SCOPE'),
                     self._SAMPLE_OAUTH_SCOPES[0])

  def AttemptOauthLocal(self, token_update=None):
    token = self._SAMPLE_OAUTH_TOKEN_INFO.copy()
    token.update(token_update or {})

    class DummyResponse(object):
      status_code = 200
      content = json.dumps(token)

    self.mox.StubOutWithMock(urlfetch, 'fetch')
    urlfetch.fetch(mox.IsA(basestring)).AndReturn(DummyResponse())

    self.mox.ReplayAll()
    users_id_token._set_bearer_user_vars_local('unused_token',
                                               self._SAMPLE_ALLOWED_CLIENT_IDS,
                                               self._SAMPLE_OAUTH_SCOPES)
    self.mox.VerifyAll()

  def testOauthLocal(self):
    self.AttemptOauthLocal()
    self.assertNotIn('ENDPOINTS_USE_OAUTH_SCOPE', os.environ)
    self.assertEqual('kevind@gmail.com',
                     os.environ.get('ENDPOINTS_AUTH_EMAIL'))
    self.assertEqual('', os.environ.get('ENDPOINTS_AUTH_DOMAIN'))

  def assertOauthLocalFailed(self, token_update):
    self.AttemptOauthLocal(token_update)
    self.assertNotIn('ENDPOINTS_USE_OAUTH_SCOPE', os.environ)
    self.assertNotIn('ENDPOINTS_AUTH_EMAIL', os.environ)
    self.assertNotIn('ENDPOINTS_AUTH_DOMAIN', os.environ)

  def testOauthLocalBadEmail(self):
    self.assertOauthLocalFailed({'verified_email': False})

  def testOauthLocalBadClientId(self):
    self.assertOauthLocalFailed({'issued_to': 'abc.appspot.com'})

  def testOauthLocalBadScopes(self):
    self.assertOauthLocalFailed({'scope': 'useless_scope and_another'})

  def testGetCurrentUserNoAuthInfo(self):
    self.assertRaises(users_id_token.InvalidGetUserCall,
                      users_id_token.get_current_user)

  def testGetCurrentUserEmailOnly(self):
    os.environ['ENDPOINTS_AUTH_EMAIL'] = 'test@gmail.com'
    os.environ['ENDPOINTS_AUTH_DOMAIN'] = ''
    user = users_id_token.get_current_user()
    self.assertEqual(user.email(), 'test@gmail.com')
    self.assertIsNone(user.user_id())

  def testGetCurrentUserEmailAndAuth(self):
    os.environ['ENDPOINTS_AUTH_EMAIL'] = 'test@gmail.com'
    os.environ['ENDPOINTS_AUTH_DOMAIN'] = 'gmail.com'
    user = users_id_token.get_current_user()
    self.assertEqual(user.email(), 'test@gmail.com')
    self.assertEqual(user.auth_domain(), 'gmail.com')
    self.assertIsNone(user.user_id())

  def testGetCurrentUserOauth(self):
    self.mox.StubOutWithMock(oauth, 'get_current_user')
    oauth.get_current_user('scope').AndReturn(users.User('test@gmail.com'))
    self.mox.ReplayAll()

    os.environ['ENDPOINTS_USE_OAUTH_SCOPE'] = 'scope'
    user = users_id_token.get_current_user()
    self.assertEqual(user.email(), 'test@gmail.com')
    self.mox.VerifyAll()

  def testGetTokenQueryParamOauthHeader(self):
    os.environ['HTTP_AUTHORIZATION'] = 'OAuth ' + self._SAMPLE_TOKEN
    token = users_id_token._get_token(None)
    self.assertEqual(token, self._SAMPLE_TOKEN)

  def testGetTokenQueryParamBearerHeader(self):
    os.environ['HTTP_AUTHORIZATION'] = 'Bearer ' + self._SAMPLE_TOKEN
    token = users_id_token._get_token(None)
    self.assertEqual(token, self._SAMPLE_TOKEN)

  def testGetTokenQueryParamInvalidBearerHeader(self):
    # Capitalization matters.  This should fail.
    os.environ['HTTP_AUTHORIZATION'] = 'BEARER ' + self._SAMPLE_TOKEN
    token = users_id_token._get_token(None)
    self.assertIsNone(token)

  def testGetTokenQueryParamInvalidHeader(self):
    os.environ['HTTP_AUTHORIZATION'] = 'Invalid ' + self._SAMPLE_TOKEN
    token = users_id_token._get_token(None)
    self.assertIsNone(token)

  def testGetTokenQueryParamBearer(self):
    request = self.mox.CreateMock(messages.Message)
    request.get_unrecognized_field_info('bearer_token').AndReturn(
        (self._SAMPLE_TOKEN, messages.Variant.STRING))

    self.mox.ReplayAll()
    token = users_id_token._get_token(request)
    self.mox.VerifyAll()
    self.assertEqual(token, self._SAMPLE_TOKEN)

  def testGetTokenQueryParamAccess(self):
    request = self.mox.CreateMock(messages.Message)
    request.get_unrecognized_field_info('bearer_token').AndReturn(
        (None, None))
    request.get_unrecognized_field_info('access_token').AndReturn(
        (self._SAMPLE_TOKEN, messages.Variant.STRING))

    self.mox.ReplayAll()
    token = users_id_token._get_token(request)
    self.mox.VerifyAll()
    self.assertEqual(token, self._SAMPLE_TOKEN)

  def testGetTokenNone(self):
    request = self.mox.CreateMock(messages.Message)
    request.get_unrecognized_field_info('bearer_token').AndReturn((None, None))
    request.get_unrecognized_field_info('access_token').AndReturn((None, None))

    self.mox.ReplayAll()
    token = users_id_token._get_token(request)
    self.mox.VerifyAll()
    self.assertIsNone(token)


class UsersIdTokenTestWithSimpleApi(UsersIdTokenTestBase):

  # pylint: disable=g-bad-name

  @api_config.api('TestApi', 'v1')
  class TestApiAnnotatedAtMethod(remote.Service):
    """Describes TestApi."""

    @api_config.method(
        message_types.VoidMessage, message_types.VoidMessage,
        audiences=UsersIdTokenTestBase._SAMPLE_AUDIENCES,
        allowed_client_ids=UsersIdTokenTestBase._SAMPLE_ALLOWED_CLIENT_IDS,
        scopes=UsersIdTokenTestBase._SAMPLE_OAUTH_SCOPES)
    def method(self):
      pass

  @api_config.api(
      'TestApi', 'v1', audiences=UsersIdTokenTestBase._SAMPLE_AUDIENCES,
      allowed_client_ids=UsersIdTokenTestBase._SAMPLE_ALLOWED_CLIENT_IDS)
  class TestApiAnnotatedAtApi(remote.Service):
    """Describes TestApi."""

    @api_config.method(message_types.VoidMessage, message_types.VoidMessage)
    def method(self, request):
      return request
  # pylint: enable=g-bad-name

  def testMaybeSetVarsAlreadySetOauth(self):
    os.environ['ENDPOINTS_USE_OAUTH_SCOPE'] = (
        'https://www.googleapis.com/auth/userinfo.email')
    users_id_token._maybe_set_current_user_vars(
        self.TestApiAnnotatedAtApi().method)
    self.assertEqual('https://www.googleapis.com/auth/userinfo.email',
                     os.environ.get('ENDPOINTS_USE_OAUTH_SCOPE'))
    self.assertNotIn('ENDPOINTS_AUTH_EMAIL', os.environ)
    self.assertNotIn('ENDPOINTS_AUTH_DOMAIN', os.environ)

  def testMaybeSetVarsAlreadySetIdToken(self):
    os.environ['ENDPOINTS_AUTH_EMAIL'] = 'test@gmail.com'
    os.environ['ENDPOINTS_AUTH_DOMAIN'] = 'gmail.com'
    users_id_token._maybe_set_current_user_vars(
        self.TestApiAnnotatedAtApi().method)
    self.assertNotIn('ENDPOINTS_USE_OAUTH_SCOPE', os.environ)
    self.assertEqual('test@gmail.com', os.environ.get('ENDPOINTS_AUTH_EMAIL'))
    self.assertEqual('gmail.com', os.environ.get('ENDPOINTS_AUTH_DOMAIN'))

  def testMaybeSetVarsAlreadySetIdTokenNoDomain(self):
    os.environ['ENDPOINTS_AUTH_EMAIL'] = 'test@gmail.com'
    os.environ['ENDPOINTS_AUTH_DOMAIN'] = ''
    users_id_token._maybe_set_current_user_vars(
        self.TestApiAnnotatedAtApi().method)
    self.assertNotIn('ENDPOINTS_USE_OAUTH_SCOPE', os.environ)
    self.assertEqual('test@gmail.com', os.environ.get('ENDPOINTS_AUTH_EMAIL'))
    self.assertEqual('', os.environ.get('ENDPOINTS_AUTH_DOMAIN'))

  def VerifyIdToken(self, cls, *args):
    self.mox.StubOutWithMock(time, 'time')
    self.mox.StubOutWithMock(users_id_token, '_get_id_token_user')
    time.time().AndReturn(1001)
    users_id_token._get_id_token_user(
        self._SAMPLE_TOKEN,
        self._SAMPLE_AUDIENCES,
        self._SAMPLE_ALLOWED_CLIENT_IDS,
        1001, memcache).AndReturn(users.User('test@gmail.com'))
    self.mox.ReplayAll()

    os.environ['HTTP_AUTHORIZATION'] = ('Bearer ' + self._SAMPLE_TOKEN)
    if args:
      cls.method(*args)
    else:
      users_id_token._maybe_set_current_user_vars(cls.method)
    self.assertEqual(os.environ.get('ENDPOINTS_AUTH_EMAIL'), 'test@gmail.com')
    self.mox.VerifyAll()

  def testMaybeSetVarsIdTokenApiAnnotation(self):
    self.VerifyIdToken(self.TestApiAnnotatedAtApi())

  def testMaybeSetVarsIdTokenMethodAnnotation(self):
    self.VerifyIdToken(self.TestApiAnnotatedAtMethod())

  def testMethodCallParsesIdToken(self):
    self.VerifyIdToken(self.TestApiAnnotatedAtApi(),
                       message_types.VoidMessage())

  def testMaybeSetVarsWithActualRequestAccessToken(self):
    dummy_scope = 'scope'
    dummy_token = 'dummy_token'
    dummy_email = 'test@gmail.com'
    dummy_client_id = self._SAMPLE_ALLOWED_CLIENT_IDS[0]

    @api_config.api('TestApi', 'v1',
                    allowed_client_ids=self._SAMPLE_ALLOWED_CLIENT_IDS,
                    scopes=[dummy_scope])
    class TestApiScopes(remote.Service):
      """Describes TestApiScopes."""

      # pylint: disable=g-bad-name
      @api_config.method(message_types.VoidMessage, message_types.VoidMessage)
      def method(self, request):
        return request

    # users_id_token._get_id_token_user and time.time don't need to be stubbed
    # because the scopes used will not be [EMAIL_SCOPE] hence _get_id_token_user
    # will never be attempted

    self.mox.StubOutWithMock(users_id_token, '_is_local_dev')
    users_id_token._is_local_dev().AndReturn(False)

    self.mox.StubOutWithMock(oauth, 'get_client_id')
    oauth.get_client_id(dummy_scope).AndReturn(dummy_client_id)

    self.mox.ReplayAll()

    api_instance = TestApiScopes()
    os.environ['HTTP_AUTHORIZATION'] = 'Bearer ' + dummy_token
    api_instance.method(message_types.VoidMessage())
    self.assertEqual(os.getenv('ENDPOINTS_USE_OAUTH_SCOPE'), dummy_scope)
    self.mox.VerifyAll()

  def testMaybeSetVarsFail(self):
    self.mox.StubOutWithMock(time, 'time')
    time.time().MultipleTimes().AndReturn(1001)
    self.mox.StubOutWithMock(users_id_token, '_get_id_token_user')
    users_id_token._get_id_token_user(
        self._SAMPLE_TOKEN,
        self._SAMPLE_AUDIENCES,
        self._SAMPLE_ALLOWED_CLIENT_IDS,
        1001, memcache).MultipleTimes().AndReturn(users.User('test@gmail.com'))
    self.mox.ReplayAll()
    # This token should correctly result in _get_id_token_user being called
    os.environ['HTTP_AUTHORIZATION'] = ('Bearer ' + self._SAMPLE_TOKEN)
    api_instance = self.TestApiAnnotatedAtApi()

    # No im_self is present and no api_info can be used, so the method itself
    # has no access to scopes, hence scopes will be null and neither of the
    # token checks will occur
    users_id_token._maybe_set_current_user_vars(api_instance.method.im_func)
    self.assertNotIn('ENDPOINTS_USE_OAUTH_SCOPE', os.environ)
    self.assertEqual(os.getenv('ENDPOINTS_AUTH_EMAIL'), '')
    self.assertEqual(os.getenv('ENDPOINTS_AUTH_DOMAIN'), '')

    # Test the same works when using the method and not im_func
    os.environ.pop('ENDPOINTS_AUTH_EMAIL')
    os.environ.pop('ENDPOINTS_AUTH_DOMAIN')
    users_id_token._maybe_set_current_user_vars(api_instance.method)
    self.assertEqual(os.getenv('ENDPOINTS_AUTH_EMAIL'), 'test@gmail.com')

    # Test that it works using the api info from the API
    os.environ.pop('ENDPOINTS_AUTH_EMAIL')
    os.environ.pop('ENDPOINTS_AUTH_DOMAIN')
    users_id_token._maybe_set_current_user_vars(api_instance.method.im_func,
                                                api_info=api_instance.api_info)
    self.assertEqual(os.getenv('ENDPOINTS_AUTH_EMAIL'), 'test@gmail.com')
    self.mox.VerifyAll()


if __name__ == '__main__':
  unittest.main()
