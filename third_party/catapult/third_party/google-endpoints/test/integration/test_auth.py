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

import copy
import flask
import mock
import os
import ssl
import threading
import time
import unittest

from Crypto import PublicKey
from jwkest import ecc
from jwkest import jwk
from test import token_utils

from google.api import auth
from google.api.auth import suppliers
from google.api.auth import tokens


class IntegrationTest(unittest.TestCase):

  _CURRENT_TIME = int(time.time())
  _PORT = 8080
  _ISSUER = "https://localhost:%d" % _PORT
  _PROVIDER_ID = "localhost"
  _INVALID_X509_PATH = "invalid-x509"
  _JWKS_PATH = "jwks"
  _SERVICE_NAME = "service@name.com"
  _X509_PATH = "x509"

  _JWT_CLAIMS = {
      "aud": ["https://aud1.local.host", "https://aud2.local.host"],
      "exp": _CURRENT_TIME + 60,
      "email": "user@local.host",
      "iss": _ISSUER,
      "sub": "subject-id"
  }

  _ec_jwk = jwk.ECKey(use="sig").load_key(ecc.P256)
  _rsa_key = jwk.RSAKey(use="sig").load_key(PublicKey.RSA.generate(1024))

  _ec_jwk.kid = "ec-key-id"
  _rsa_key.kid = "rsa-key-id"

  _mock_timer = mock.MagicMock()

  _jwks = jwk.KEYS()
  _jwks._keys.append(_ec_jwk)
  _jwks._keys.append(_rsa_key)

  _AUTH_TOKEN = token_utils.generate_auth_token(_JWT_CLAIMS, _jwks._keys,
                                                alg="RS256", kid=_rsa_key.kid)


  @classmethod
  def setUpClass(cls):
    dirname = os.path.dirname(os.path.realpath(__file__))
    cls._cert_file = os.path.join(dirname, "ssl.cert")
    cls._key_file = os.path.join(dirname, "ssl.key")
    os.environ["REQUESTS_CA_BUNDLE"] = cls._cert_file

    rest_server = cls._RestServer()
    rest_server.start()

  def setUp(self):
    self._provider_ids = {}
    self._configs = {}
    self._authenticator = auth.create_authenticator(self._provider_ids,
                                                    self._configs)

    self._auth_info = mock.MagicMock()
    self._auth_info.is_provider_allowed.return_value = True
    self._auth_info.get_allowed_audiences.return_value = [
        "https://aud1.local.host"
    ]

  def test_verify_auth_token_with_jwks(self):
    url = get_url(IntegrationTest._JWKS_PATH)
    self._provider_ids[self._ISSUER] = self._PROVIDER_ID
    self._configs[IntegrationTest._ISSUER] = suppliers.IssuerUriConfig(False,
                                                                       url)
    user_info = self._authenticator.authenticate(IntegrationTest._AUTH_TOKEN,
                                                 self._auth_info,
                                                 IntegrationTest._SERVICE_NAME)
    self._assert_user_info_equals(tokens.UserInfo(IntegrationTest._JWT_CLAIMS),
                                  user_info)

  def test_authenticate_auth_token_with_bad_signature(self):
    new_rsa_key = jwk.RSAKey(use="sig").load_key(PublicKey.RSA.generate(2048))
    kid = IntegrationTest._rsa_key.kid
    new_rsa_key.kid = kid
    new_jwks = jwk.KEYS()
    new_jwks._keys.append(new_rsa_key)
    auth_token = token_utils.generate_auth_token(IntegrationTest._JWT_CLAIMS,
                                                 new_jwks._keys, alg="RS256",
                                                 kid=kid)
    url = get_url(IntegrationTest._JWKS_PATH)
    self._provider_ids[self._ISSUER] = self._PROVIDER_ID
    self._configs[IntegrationTest._ISSUER] = suppliers.IssuerUriConfig(False,
                                                                       url)
    message = "Signature verification failed"
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException, message):
      self._authenticator.authenticate(auth_token, self._auth_info,
                                       IntegrationTest._SERVICE_NAME)

  def test_verify_auth_token_with_x509(self):
    url = get_url(IntegrationTest._X509_PATH)
    self._provider_ids[self._ISSUER] = self._PROVIDER_ID
    self._configs[IntegrationTest._ISSUER] = suppliers.IssuerUriConfig(False,
                                                                       url)
    user_info = self._authenticator.authenticate(IntegrationTest._AUTH_TOKEN,
                                                 self._auth_info,
                                                 IntegrationTest._SERVICE_NAME)
    self._assert_user_info_equals(tokens.UserInfo(IntegrationTest._JWT_CLAIMS),
                                  user_info)

  def test_verify_auth_token_with_invalid_x509(self):
    url = get_url(IntegrationTest._INVALID_X509_PATH)
    self._provider_ids[self._ISSUER] = self._PROVIDER_ID
    self._configs[IntegrationTest._ISSUER] = suppliers.IssuerUriConfig(False,
                                                                       url)
    message = "Cannot load X.509 certificate"
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException, message):
      self._authenticator.authenticate(IntegrationTest._AUTH_TOKEN,
                                       self._auth_info,
                                       IntegrationTest._SERVICE_NAME)

  def test_openid_discovery(self):
    self._provider_ids[self._ISSUER] = self._PROVIDER_ID
    self._configs[IntegrationTest._ISSUER] = suppliers.IssuerUriConfig(True,
                                                                       None)
    user_info = self._authenticator.authenticate(IntegrationTest._AUTH_TOKEN,
                                                 self._auth_info,
                                                 IntegrationTest._SERVICE_NAME)
    self._assert_user_info_equals(tokens.UserInfo(IntegrationTest._JWT_CLAIMS),
                                  user_info)

  def test_openid_discovery_failed(self):
    self._provider_ids[self._ISSUER] = self._PROVIDER_ID
    self._configs[IntegrationTest._ISSUER] = suppliers.IssuerUriConfig(False,
                                                                       None)
    message = ("Cannot find the `jwks_uri` for issuer %s" %
               IntegrationTest._ISSUER)
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException, message):
      self._authenticator.authenticate(IntegrationTest._AUTH_TOKEN,
                                       self._auth_info,
                                       IntegrationTest._SERVICE_NAME)

  def test_authenticate_with_malformed_auth_code(self):
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException,
                                 "Cannot decode the auth token"):
      self._authenticator.authenticate("invalid-auth-code", self._auth_info,
                                       IntegrationTest._SERVICE_NAME)

  def test_authenticate_with_disallowed_issuer(self):
    url = get_url(IntegrationTest._JWKS_PATH)
    self._configs[IntegrationTest._ISSUER] = suppliers.IssuerUriConfig(False,
                                                                       url)
    message = "Unknown issuer: " + self._ISSUER
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException, message):
      self._authenticator.authenticate(IntegrationTest._AUTH_TOKEN,
                                       self._auth_info,
                                       IntegrationTest._SERVICE_NAME)

  def test_authenticate_with_unknown_issuer(self):
    message = ("Cannot find the `jwks_uri` for issuer %s: "
               "either the issuer is unknown") % IntegrationTest._ISSUER
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException, message):
      self._authenticator.authenticate(IntegrationTest._AUTH_TOKEN,
                                       self._auth_info,
                                       IntegrationTest._SERVICE_NAME)

  def test_authenticate_with_invalid_audience(self):
    url = get_url(IntegrationTest._JWKS_PATH)
    self._provider_ids[self._ISSUER] = self._PROVIDER_ID
    self._configs[IntegrationTest._ISSUER] = suppliers.IssuerUriConfig(False,
                                                                       url)
    self._auth_info.get_allowed_audiences.return_value = []
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException,
                                 "Audiences not allowed"):
      self._authenticator.authenticate(IntegrationTest._AUTH_TOKEN,
                                       self._auth_info,
                                       IntegrationTest._SERVICE_NAME)

  @mock.patch("time.time", _mock_timer)
  def test_authenticate_with_expired_auth_token(self):
    url = get_url(IntegrationTest._JWKS_PATH)
    self._provider_ids[self._ISSUER] = self._PROVIDER_ID
    self._configs[IntegrationTest._ISSUER] = suppliers.IssuerUriConfig(False,
                                                                       url)
    IntegrationTest._mock_timer.return_value = 0

    # Create an auth token that expires in 10 seconds.
    jwt_claims = copy.deepcopy(IntegrationTest._JWT_CLAIMS)
    jwt_claims["exp"] = time.time() + 10
    auth_token = token_utils.generate_auth_token(jwt_claims,
                                                 IntegrationTest._jwks._keys,
                                                 alg="RS256",
                                                 kid=IntegrationTest._rsa_key.kid)

    # Verify that the auth token can be authenticated successfully.
    self._authenticator.authenticate(IntegrationTest._AUTH_TOKEN,
                                     self._auth_info,
                                     IntegrationTest._SERVICE_NAME)

    # Advance the timer by 20 seconds and make sure the token is expired.
    IntegrationTest._mock_timer.return_value += 20
    message = "The auth token has already expired"
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException, message):
      self._authenticator.authenticate(auth_token, self._auth_info,
                                       IntegrationTest._SERVICE_NAME)

  def test_invalid_openid_discovery_url(self):
    issuer = "https://invalid.issuer"
    self._provider_ids[self._ISSUER] = self._PROVIDER_ID
    self._configs[issuer] = suppliers.IssuerUriConfig(True, None)

    jwt_claims = copy.deepcopy(IntegrationTest._JWT_CLAIMS)
    jwt_claims["iss"] = issuer
    auth_token = token_utils.generate_auth_token(jwt_claims,
                                                 IntegrationTest._jwks._keys,
                                                 alg="RS256",
                                                 kid=IntegrationTest._rsa_key.kid)
    message = "Cannot discover the jwks uri"
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException, message):
      self._authenticator.authenticate(auth_token, self._auth_info,
                                       IntegrationTest._SERVICE_NAME)

  def test_invalid_jwks_uri(self):
    url = "https://invalid.jwks.uri"
    self._provider_ids[self._ISSUER] = self._PROVIDER_ID
    self._configs[IntegrationTest._ISSUER] = suppliers.IssuerUriConfig(False,
                                                                       url)
    message = "Cannot retrieve valid verification keys from the `jwks_uri`"
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException, message):
      self._authenticator.authenticate(IntegrationTest._AUTH_TOKEN,
                                       self._auth_info,
                                       IntegrationTest._SERVICE_NAME)

  def _assert_user_info_equals(self, expected, actual):
    self.assertEqual(expected.audiences, actual.audiences)
    self.assertEqual(expected.email, actual.email)
    self.assertEqual(expected.subject_id, actual.subject_id)
    self.assertEqual(expected.issuer, actual.issuer)


  class _RestServer(object):

    def __init__(self):
      app = flask.Flask("integration-test-server")

      @app.route("/" + IntegrationTest._JWKS_PATH)
      def get_json_web_key_set():  # pylint: disable=unused-variable
        return IntegrationTest._jwks.dump_jwks()

      @app.route("/" + IntegrationTest._X509_PATH)
      def get_x509_certificates():  # pylint: disable=unused-variable
        cert = IntegrationTest._rsa_key.key.publickey().exportKey("PEM")
        return flask.jsonify({IntegrationTest._rsa_key.kid: cert})

      @app.route("/" + IntegrationTest._INVALID_X509_PATH)
      def get_invalid_x509_certificates():  # pylint: disable=unused-variable
        return flask.jsonify({IntegrationTest._rsa_key.kid: "invalid cert"})

      @app.route("/.well-known/openid-configuration")
      def get_openid_configuration():  # pylint: disable=unused-variable
        return flask.jsonify({"jwks_uri": get_url(IntegrationTest._JWKS_PATH)})

      self._application = app

    def start(self):
      def run_app():
        ssl_context = (IntegrationTest._cert_file, IntegrationTest._key_file)
        self._application.run(port=IntegrationTest._PORT,
                              ssl_context=ssl_context)

      thread = threading.Thread(target=run_app, args=())
      thread.daemon = True
      thread.start()


def get_url(path):
  return "https://localhost:%d/%s" % (IntegrationTest._PORT, path)
