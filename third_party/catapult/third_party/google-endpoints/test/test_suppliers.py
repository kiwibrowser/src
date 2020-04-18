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

import json
import unittest
import httmock
import mock

from Crypto import PublicKey
from jwkest import jwk

from google.api.auth import suppliers


class KeyUriSupplierTest(unittest.TestCase):

  def test_supply_issuer(self):
    issuer = "https://issuer.com"
    jwks_uri = "https://issuer.com/jwks/uri"
    configs = {issuer: suppliers.IssuerUriConfig(False, jwks_uri)}
    supplier = suppliers.KeyUriSupplier(configs)
    self.assertEquals(jwks_uri, supplier.supply(issuer))
    self.assertIsNone(supplier.supply("random-issuer"))

  def test_openid_discovery(self):
    jwks_uri = "https://issuer.com/jwks/uri"
    @httmock.urlmatch(scheme="https", netloc="issuer.com",
                      path="/" + suppliers._OPEN_ID_CONFIG_PATH)
    def _mock_response(url, request):  # pylint: disable=unused-argument
      response = {"jwks_uri": jwks_uri}
      return json.dumps(response)

    issuer = "https://issuer.com"
    configs = {issuer: suppliers.IssuerUriConfig(True, None)}
    supplier = suppliers.KeyUriSupplier(configs)
    with httmock.HTTMock(_mock_response):
      self.assertEquals(jwks_uri, supplier.supply(issuer))

  def test_issuer_without_protocol(self):
    jwks_uri = "https://issuer.com/jwks/uri"
    @httmock.urlmatch(scheme="https", netloc="issuer.com",
                      path="/" + suppliers._OPEN_ID_CONFIG_PATH)
    def _mock_response(url, request):  # pylint: disable=unused-argument
      response = {"jwks_uri": jwks_uri}
      return json.dumps(response)

    # Specify an issuer without protocol to make sure the "https://" prefix is
    # added automatically.
    issuer = "issuer.com"
    configs = {issuer: suppliers.IssuerUriConfig(True, None)}
    supplier = suppliers.KeyUriSupplier(configs)
    with httmock.HTTMock(_mock_response):
      self.assertEquals(jwks_uri, supplier.supply(issuer))

  def test_openid_discovery_with_bad_json(self):
    @httmock.urlmatch(scheme="https", netloc="issuer.com")
    def _mock_response_with_bad_json(url, request):  # pylint: disable=unused-argument
      return "bad-json"

    issuer = "https://issuer.com"
    configs = {issuer: suppliers.IssuerUriConfig(True, None)}
    supplier = suppliers.KeyUriSupplier(configs)
    with httmock.HTTMock(_mock_response_with_bad_json):
      with self.assertRaises(suppliers.UnauthenticatedException):
        supplier.supply(issuer)


class JwksSupplierTest(unittest.TestCase):
  _mock_timer = mock.MagicMock()

  def setUp(self):
    self._key_uri_supplier = mock.MagicMock()
    self._jwks_uri_supplier = suppliers.JwksSupplier(self._key_uri_supplier)

  def test_supply_with_unknown_issuer(self):
    self._key_uri_supplier.supply.return_value = None
    issuer = "unknown-issuer"
    expected_message = "Cannot find the `jwks_uri` for issuer " + issuer
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException,
                                 expected_message):
      self._jwks_uri_supplier.supply(issuer)

  def test_supply_with_invalid_json_response(self):
    scheme = "https"
    issuer = "issuer.com"
    self._key_uri_supplier.supply.return_value = scheme + "://" + issuer

    @httmock.urlmatch(scheme=scheme, netloc=issuer)
    def _mock_response_with_invalid_json(url, response):  # pylint: disable=unused-argument
      return "invalid-json"

    with httmock.HTTMock(_mock_response_with_invalid_json):
      with self.assertRaises(suppliers.UnauthenticatedException):
        self._jwks_uri_supplier.supply(issuer)

  def test_supply_jwks(self):
    rsa_key = PublicKey.RSA.generate(2048)
    jwks = jwk.KEYS()
    jwks.wrap_add(rsa_key)

    scheme = "https"
    issuer = "issuer.com"
    self._key_uri_supplier.supply.return_value = scheme + "://" + issuer

    @httmock.urlmatch(scheme=scheme, netloc=issuer)
    def _mock_response_with_jwks(url, response):  # pylint: disable=unused-argument
      return jwks.dump_jwks()

    with httmock.HTTMock(_mock_response_with_jwks):
      actual_jwks = self._jwks_uri_supplier.supply(issuer)
      self.assertEquals(1, len(actual_jwks))
      actual_key = actual_jwks[0].key
      self.assertEquals(rsa_key.n, actual_key.n)
      self.assertEquals(rsa_key.e, actual_key.e)

  def test_supply_jwks_with_x509_certificate(self):
    rsa_key = PublicKey.RSA.generate(2048)
    cert = rsa_key.publickey().exportKey("PEM")
    kid = "rsa-cert"

    scheme = "https"
    issuer = "issuer.com"
    self._key_uri_supplier.supply.return_value = scheme + "://" + issuer

    @httmock.urlmatch(scheme=scheme, netloc=issuer)
    def _mock_response_with_x509_certificates(url, response):  # pylint: disable=unused-argument
      return json.dumps({kid: cert})

    with httmock.HTTMock(_mock_response_with_x509_certificates):
      actual_jwks = self._jwks_uri_supplier.supply(issuer)
      self.assertEquals(1, len(actual_jwks))
      actual_key = actual_jwks[0].key

      self.assertEquals(kid, actual_jwks[0].kid)
      self.assertEquals(rsa_key.n, actual_key.n)
      self.assertEquals(rsa_key.e, actual_key.e)

  def test_supply_empty_x509_certificate(self):
    scheme = "https"
    issuer = "issuer.com"
    self._key_uri_supplier.supply.return_value = scheme + "://" + issuer

    @httmock.urlmatch(scheme=scheme, netloc=issuer)
    def _mock_invalid_response(url, response):  # pylint: disable=unused-argument
      return json.dumps({"kid": "invlid-certificate"})

    with httmock.HTTMock(_mock_invalid_response):
      with self.assertRaises(suppliers.UnauthenticatedException):
        self._jwks_uri_supplier.supply(issuer)

  @mock.patch("time.time", _mock_timer)
  def test_supply_cached_jwks(self):
    rsa_key = PublicKey.RSA.generate(2048)
    jwks = jwk.KEYS()
    jwks.wrap_add(rsa_key)

    scheme = "https"
    issuer = "issuer.com"
    self._key_uri_supplier.supply.return_value = scheme + "://" + issuer

    @httmock.urlmatch(scheme=scheme, netloc=issuer)
    def _mock_response_with_jwks(url, response):  # pylint: disable=unused-argument
      return jwks.dump_jwks()

    with httmock.HTTMock(_mock_response_with_jwks):
      JwksSupplierTest._mock_timer.return_value = 10
      self.assertEqual(1, len(self._jwks_uri_supplier.supply(issuer)))

      # Add an additional key to the JWKS to be returned by the HTTP request.
      jwks.wrap_add(PublicKey.RSA.generate(2048))

      # Forward the clock by 1 second. The JWKS should remain cached.
      JwksSupplierTest._mock_timer.return_value += 1
      self._jwks_uri_supplier.supply(issuer)
      self.assertEqual(1, len(self._jwks_uri_supplier.supply(issuer)))

      # Forward the clock by 5 minutes. The cache entry should have expired so
      # the returned JWKS should be the updated one with two keys.
      JwksSupplierTest._mock_timer.return_value += 5 * 60
      self._jwks_uri_supplier.supply(issuer)
      self.assertEqual(2, len(self._jwks_uri_supplier.supply(issuer)))
