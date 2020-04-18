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
import mock
import json
import time
import unittest

from Crypto import PublicKey
from jwkest import ecc
from jwkest import jwk
from jwkest import jws
from test import token_utils

from google.api.auth import suppliers
from google.api.auth import tokens


class AuthenticatorTest(unittest.TestCase):
  _ec_kid = "ec-key-id"
  _rsa_kid = "rsa-key-id"

  _mock_timer = mock.MagicMock()

  def setUp(self):
    ec_jwk = jwk.ECKey(use="sig").load_key(ecc.P256)
    ec_jwk.kid = self._ec_kid

    rsa_key = jwk.RSAKey(use="sig").load_key(PublicKey.RSA.generate(1024))
    rsa_key.kid = self._rsa_kid

    jwks = jwk.KEYS()
    jwks._keys.append(ec_jwk)
    jwks._keys.append(rsa_key)

    self._issuers_to_provider_ids = {}
    self._jwks_supplier = mock.MagicMock()
    self._authenticator = tokens.Authenticator(self._issuers_to_provider_ids,
                                               self._jwks_supplier)
    self._jwks = jwks
    self._jwks_supplier.supply.return_value = self._jwks

    self._method_info = mock.MagicMock()
    self._service_name = "service.name.com"

    self._jwt_claims = {
        "aud": ["first.com", "second.com"],
        "email": "someone@email.com",
        "exp": int(time.time()) + 10,
        "iss": "https://issuer.com",
        "sub": "subject-id"
    }

  def test_get_jwt_claims(self):
    auth_token = token_utils.generate_auth_token(self._jwt_claims,
                                                 self._jwks._keys,
                                                 kid=self._ec_kid)
    actual_jwt_claims = self._authenticator.get_jwt_claims(auth_token)
    self.assertEqual(self._jwt_claims, actual_jwt_claims)

  def test_get_jwt_claims_without_kid(self):
    auth_token = token_utils.generate_auth_token(self._jwt_claims,
                                                 self._jwks._keys)
    actual_jwt_claims = self._authenticator.get_jwt_claims(auth_token)
    self.assertEqual(self._jwt_claims, actual_jwt_claims)

  def test_required_claims(self):
    def assert_missing_claim_raise_exception(claim_name):
      jwt_claims = copy.deepcopy(self._jwt_claims)
      del jwt_claims[claim_name]
      auth_token = token_utils.generate_auth_token(jwt_claims,
                                                   self._jwks._keys,
                                       kid=self._ec_kid)
      with self.assertRaisesRegexp(suppliers.UnauthenticatedException,
                                   'Missing "%s" claim' % claim_name):
        self._authenticator.get_jwt_claims(auth_token)

    assert_missing_claim_raise_exception("aud")
    assert_missing_claim_raise_exception("exp")
    assert_missing_claim_raise_exception("sub")
    assert_missing_claim_raise_exception("iss")

  @mock.patch("time.time", _mock_timer)
  def test_get_jwt_claims_via_caching(self):
    AuthenticatorTest._mock_timer.return_value = 10

    auth_token = token_utils.generate_auth_token(self._jwt_claims,
                                                 self._jwks._keys)
    # Populate the decoded result into cache.
    self._authenticator.get_jwt_claims(auth_token)

    # Reset the returned JWKS so the signature verification will fail next
    # time.
    self._jwks_supplier.supply.return_value = jwk.KEYS()

    # Forword time by 10 seconds.
    AuthenticatorTest._mock_timer.return_value += 10
    # This call should succeed since the auth_token is cached.
    self._authenticator.get_jwt_claims(auth_token)

    # Forword time by 5 minutes.
    AuthenticatorTest._mock_timer.return_value += 5 * 60
    # This call should fail since the cache expires and it needs to re-decode
    # the auth token with a different key set.
    with self.assertRaises(suppliers.UnauthenticatedException):
      self._authenticator.get_jwt_claims(auth_token)

  def test_auth_token_cache_capacity(self):
    authenticator = tokens.Authenticator({}, self._jwks_supplier, cache_capacity=2)

    self._jwt_claims["email"] = "1@email.com"
    auth_token1 = token_utils.generate_auth_token(self._jwt_claims,
                                                  self._jwks._keys)
    self._jwt_claims["email"] = "2@email.com"
    auth_token2 = token_utils.generate_auth_token(self._jwt_claims,
                                                  self._jwks._keys)

    # Populate the decoded result into cache.
    authenticator.get_jwt_claims(auth_token1)
    authenticator.get_jwt_claims(auth_token2)

    # Reset the returned JWKS so the signature verification will fail next
    # time.
    new_ec_jwk = jwk.ECKey(use="sig").load_key(ecc.P256)
    new_ec_jwk.kid = self._ec_kid
    new_jwks = jwk.KEYS()
    new_jwks._keys.append(new_ec_jwk)
    self._jwks_supplier.supply.return_value = new_jwks

    # Verify the following calls still succeed since the auth tokens are
    # cached.
    authenticator.get_jwt_claims(auth_token1)
    authenticator.get_jwt_claims(auth_token2)

    # Populate a third auth token into the cache.
    self._jwt_claims["email"] = "3@email.com"
    auth_token3 = token_utils.generate_auth_token(self._jwt_claims,
                                                  new_jwks._keys)
    authenticator.get_jwt_claims(auth_token3)

    # Make sure the first auth token is evicted from the cache since the cache
    # is full.
    with self.assertRaises(suppliers.UnauthenticatedException):
      authenticator.get_jwt_claims(auth_token1)

  def test_verify_fails(self):
    auth_token = token_utils.generate_auth_token(self._jwt_claims,
                                                 self._jwks._keys,
                                     kid=self._ec_kid)

    # Let the _jwks_supplier return a different key than the one we use to sign
    # the JWT.
    new_jwk = jwk.ECKey(use="sig").load_key(ecc.P256)
    new_jwks = jwk.KEYS()
    new_jwks._keys.append(new_jwk)
    self._jwks_supplier.supply.return_value = new_jwks

    with self.assertRaises(suppliers.UnauthenticatedException):
      self._authenticator.get_jwt_claims(auth_token)

  def test_authenticate_successfully(self):
    auth_token = token_utils.generate_auth_token(self._jwt_claims,
                                                 self._jwks._keys,
                                                 kid=self._ec_kid)
    self._method_info.get_allowed_audiences.return_value = ["first.com"]
    self._issuers_to_provider_ids[self._jwt_claims["iss"]] = "provider-id"
    actual_user_info = self._authenticator.authenticate(auth_token,
                                                        self._method_info,
                                                        "service.name.com")
    self.assert_user_info(actual_user_info, self._jwt_claims["aud"],
                          self._jwt_claims["email"], self._jwt_claims["sub"],
                          self._jwt_claims["iss"])

  def test_authenticate_with_single_audience(self):
    aud = "first.aud.com"
    self._jwt_claims["aud"] = aud
    auth_token = token_utils.generate_auth_token(self._jwt_claims,
                                                 self._jwks._keys,
                                                 kid=self._ec_kid)
    self._issuers_to_provider_ids[self._jwt_claims["iss"]] = "provider-id"
    actual_user_info = self._authenticator.authenticate(auth_token,
                                                        self._method_info, aud)
    self.assertEqual([aud], actual_user_info.audiences)

  def test_authenticate_with_malformed_claims(self):
    def assert_malformed_time_claim_raises_exception(claim_name, expiration):
      jwt_claims = copy.deepcopy(self._jwt_claims)
      jwt_claims[claim_name] = expiration
      auth_token = token_utils.generate_auth_token(jwt_claims,
                                                   self._jwks._keys)
      message = 'Malformed claim: "%s" must be an integer' % claim_name
      with self.assertRaisesRegexp(suppliers.UnauthenticatedException, message):
        self._authenticator.authenticate(auth_token, self._method_info,
                                         "service.name")

    assert_malformed_time_claim_raises_exception("exp", "1")
    assert_malformed_time_claim_raises_exception("exp", 1.1)
    assert_malformed_time_claim_raises_exception("exp", [1])
    assert_malformed_time_claim_raises_exception("nbf", "1")
    assert_malformed_time_claim_raises_exception("nbf", 1.1)
    assert_malformed_time_claim_raises_exception("nbf", [1])

  def test_authenticate_with_expired_auth_token(self):
    self._jwt_claims["exp"] = long(time.time() - 10)
    auth_token = token_utils.generate_auth_token(self._jwt_claims,
                                                 self._jwks._keys)
    message = "The auth token has already expired"
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException, message):
      self._authenticator.authenticate(auth_token,
                                       self._method_info,
                                       "service.name")

  def test_authenticate_with_nbf_claim(self):
    # Set the "nbf" claim to some time in the future.
    self._jwt_claims["nbf"] = long(time.time() + 5)
    auth_token = token_utils.generate_auth_token(self._jwt_claims,
                                                 self._jwks._keys)
    message = 'Current time is less than the "nbf" time'
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException, message):
      self._authenticator.authenticate(auth_token, self._method_info,
                                       "service.name")

  def test_authenticate_with_service_name_as_audience(self):
    self._jwt_claims["aud"].append(self._service_name)
    self._issuers_to_provider_ids[self._jwt_claims["iss"]] = "provider-id"
    self._method_info.get_allowed_audiences.return_value = []
    auth_token = token_utils.generate_auth_token(self._jwt_claims,
                                                 self._jwks._keys,
                                     kid=self._ec_kid)
    actual_user_info = self._authenticator.authenticate(auth_token,
                                                        self._method_info,
                                                        self._service_name)
    self.assert_user_info(actual_user_info, self._jwt_claims["aud"],
                          self._jwt_claims["email"], self._jwt_claims["sub"],
                          self._jwt_claims["iss"])

  def test_authenticate_with_disallowed_provider_id(self):
    auth_token = token_utils.generate_auth_token(self._jwt_claims,
                                                 self._jwks._keys,
                                     kid=self._ec_kid)
    self._method_info.is_provider_allowed.return_value = False
    self._issuers_to_provider_ids[self._jwt_claims["iss"]] = "id"
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException,
                                 "The requested method does not allow provider "
                                 "id: id"):
      self._authenticator.authenticate(auth_token, self._method_info,
                                       self._service_name)

  def test_authenticate_with_disallowed_audiences(self):
    auth_token = token_utils.generate_auth_token(self._jwt_claims,
                                                 self._jwks._keys,
                                                 kid=self._ec_kid)
    self._method_info.get_allowed_audiences.return_value = []
    self._issuers_to_provider_ids[self._jwt_claims["iss"]] = "project-id"
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException,
                                 "Audiences not allowed"):
      self._authenticator.authenticate(auth_token, self._method_info,
                                       self._service_name)

  def test_unicode_decode_error(self):
    auth_token = "ya29.CjA8A3Hrca1hCCvRg69U3Tg85CG5pRqZj7gOJUsicpRafWAW63zvg6a0ZM6wZ5mJwM0"
    with self.assertRaisesRegexp(suppliers.UnauthenticatedException,
                                 "Cannot decode the auth token"):
      self._authenticator.authenticate(auth_token, None, None)

  def assert_user_info(self, actual_user_info, audiences, email, subject_id,
                       issuer):
    self.assertEqual(audiences, actual_user_info.audiences)
    self.assertEqual(email, actual_user_info.email)
    self.assertEqual(subject_id, actual_user_info.subject_id)
    self.assertEqual(issuer, actual_user_info.issuer)
