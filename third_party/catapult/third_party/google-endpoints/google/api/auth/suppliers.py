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

"""Defines several suppliers that are used by the authenticator."""

import datetime
from dogpile import cache
from jwkest import jwk
import requests
import ssl


_HTTP_PROTOCOL_PREFIX = "http://"
_HTTPS_PROTOCOL_PREFIX = "https://"

_OPEN_ID_CONFIG_PATH = ".well-known/openid-configuration"


class KeyUriSupplier(object):  # pylint: disable=too-few-public-methods
  """A supplier that provides the `jwks_uri` for an issuer."""

  def __init__(self, issuer_uri_configs):
    """Construct an instance of KeyUriSupplier.

    Args:
      issuer_uri_configs: a dictionary mapping from an issuer to its jwks_uri
        configuration.
    """
    self._issuer_uri_configs = issuer_uri_configs

  def supply(self, issuer):
    """Supplies the `jwks_uri` for the given issuer.

    Args:
      issuer: the issuer.

    Returns:
      The `jwks_uri` that is either statically configured or retrieved via
      OpenId discovery. None is returned when the issuer is unknown or the
      OpenId discovery fails.
    """
    issuer_uri_config = self._issuer_uri_configs.get(issuer)

    if not issuer_uri_config:
      # The issuer is unknown.
      return

    jwks_uri = issuer_uri_config.jwks_uri
    if jwks_uri:
      # When jwks_uri is set, return it directly.
      return jwks_uri

    # When jwksUri is empty, we try to retrieve it through the OpenID
    # discovery.
    open_id_valid = issuer_uri_config.open_id_valid
    if open_id_valid:
      discovered_jwks_uri = _discover_jwks_uri(issuer)
      self._issuer_uri_configs[issuer] = IssuerUriConfig(False,
                                                         discovered_jwks_uri)
      return discovered_jwks_uri


class JwksSupplier(object):  # pylint: disable=too-few-public-methods
  """A supplier that returns the Json Web Token Set of an issuer."""

  def __init__(self, key_uri_supplier):
    """Constructs an instance of JwksSupplier.

    Args:
      key_uri_supplier: a KeyUriSupplier instance that returns the `jwks_uri`
        based on the given issuer.
    """
    self._key_uri_supplier = key_uri_supplier
    self._jwks_cache = cache.make_region().configure(
        "dogpile.cache.memory", expiration_time=datetime.timedelta(minutes=5))

  def supply(self, issuer):
    """Supplies the `Json Web Key Set` for the given issuer.

    Args:
      issuer: the issuer.

    Returns:
      The successfully retrieved Json Web Key Set. None is returned if the
        issuer is unknown or the retrieval process fails.

    Raises:
      UnauthenticatedException: When this method cannot supply JWKS for the
        given issuer (e.g. unknown issuer, HTTP request error).
    """
    def _retrieve_jwks():
      """Retrieve the JWKS from the given jwks_uri when cache misses."""
      jwks_uri = self._key_uri_supplier.supply(issuer)

      if not jwks_uri:
        raise UnauthenticatedException("Cannot find the `jwks_uri` for issuer "
                                       "%s: either the issuer is unknown or "
                                       "the OpenID discovery failed" % issuer)

      try:
        response = requests.get(jwks_uri)
        json_response = response.json()
      except Exception as exception:
        message = "Cannot retrieve valid verification keys from the `jwks_uri`"
        raise UnauthenticatedException(message, exception)

      if "keys" in json_response:
        # De-serialize the JSON as a JWKS object.
        jwks_keys = jwk.KEYS()
        jwks_keys.load_jwks(response.text)
        return jwks_keys._keys
      else:
        # The JSON is a dictionary mapping from key id to X.509 certificates.
        # Thus we extract the public key from the X.509 certificates and
        # construct a JWKS object.
        return _extract_x509_certificates(json_response)

    return self._jwks_cache.get_or_create(issuer, _retrieve_jwks)


def _extract_x509_certificates(x509_certificates):
  keys = []
  for kid, certificate in x509_certificates.iteritems():
    try:
      if certificate.startswith(jwk.PREFIX):
        # The certificate is PEM-encoded
        der = ssl.PEM_cert_to_DER_cert(certificate)
        key = jwk.der2rsa(der)
      else:
        key = jwk.import_rsa_key(certificate)
    except Exception as exception:
      raise UnauthenticatedException("Cannot load X.509 certificate",
                                     exception)
    rsa_key = jwk.RSAKey().load_key(key)
    rsa_key.kid = kid
    keys.append(rsa_key)
  return keys


def _discover_jwks_uri(issuer):
  open_id_url = _construct_open_id_url(issuer)
  try:
    response = requests.get(open_id_url)
    return response.json().get("jwks_uri")
  except Exception as error:
    raise UnauthenticatedException("Cannot discover the jwks uri", error)


def _construct_open_id_url(issuer):
  url = issuer
  if (not url.startswith(_HTTP_PROTOCOL_PREFIX) and
      not url.startswith(_HTTPS_PROTOCOL_PREFIX)):
    url = _HTTPS_PROTOCOL_PREFIX + url
  if not url.endswith("/"):
    url += "/"
  url += _OPEN_ID_CONFIG_PATH
  return url


class IssuerUriConfig(object):
  """The jwks_uri configuration for an issuer.

  TODO (yangguan): this class should be removed after we figure out how to
  fetch the external configs.
  """

  def __init__(self, open_id_valid, jwks_uri):
    """Create an instance of IsserUriConfig.

    Args:
      open_id_valid: indicates whether the corresponding issuer is valid for
        OpenId discovery.
      jwks_uri: is the saved jwks_uri. Its value can be None if the OpenId
        discovery process has not begun or has already failed.
    """
    self._open_id_valid = open_id_valid
    self._jwks_uri = jwks_uri

  @property
  def open_id_valid(self):
    return self._open_id_valid

  @property
  def jwks_uri(self):
    return self._jwks_uri


class UnauthenticatedException(Exception):
  pass
