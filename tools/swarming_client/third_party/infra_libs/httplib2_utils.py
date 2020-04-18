# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import collections
import copy
import httplib
import json
import logging
import os
import re
import socket
import sys
import time

import httplib2
import oauth2client.client

from googleapiclient import errors
from infra_libs.ts_mon.common import http_metrics
from oauth2client import util

DEFAULT_SCOPES = ['email']

# default timeout for http requests, in seconds
DEFAULT_TIMEOUT = 30

# This is part of the API.
if sys.platform.startswith('win'): # pragma: no cover
  SERVICE_ACCOUNTS_CREDS_ROOT = 'C:\\creds\\service_accounts'
else:
  SERVICE_ACCOUNTS_CREDS_ROOT = '/creds/service_accounts'


class AuthError(Exception):
  pass


def load_service_account_credentials(credentials_filename,
                                     service_accounts_creds_root=None):
  """Loads and validate a credential JSON file.

  Example of a well-formatted file:
    {
      "private_key_id": "4168d274cdc7a1eaef1c59f5b34bdf255",
      "private_key": ("-----BEGIN PRIVATE KEY-----\nMIIhkiG9w0BAQEFAASCAmEwsd"
                      "sdfsfFd\ngfxFChctlOdTNm2Wrr919Nx9q+sPV5ibyaQt5Dgn89fKV"
                      "jftrO3AMDS3sMjaE4Ib\nZwJgy90wwBbMT7/YOzCgf5PZfivUe8KkB"
                      -----END PRIVATE KEY-----\n",
      "client_email": "234243-rjstu8hi95iglc8at3@developer.gserviceaccount.com",
      "client_id": "234243-rjstu8hi95iglc8at3.apps.googleusercontent.com",
      "type": "service_account"
    }

  Args:
    credentials_filename (str): path to a .json file containing credentials
      for a Cloud platform service account.

  Keyword Args:
    service_accounts_creds_root (str or None): location where all service
      account credentials are stored. ``credentials_filename`` is relative
      to this path. None means 'use default location'.

  Raises:
    AuthError: if the file content is invalid.
  """
  service_accounts_creds_root = (service_accounts_creds_root
                                 or SERVICE_ACCOUNTS_CREDS_ROOT)

  service_account_file = os.path.join(service_accounts_creds_root,
                                      credentials_filename)
  try:
    with open(service_account_file, 'r') as f:
      key = json.load(f)
  except ValueError as e:
    raise AuthError('Parsing of file as JSON failed (%s): %s',
                    e, service_account_file)

  if key.get('type') != 'service_account':
    msg = ('Credentials type must be for a service_account, got %s.'
           ' Check content of %s' % (key.get('type'), service_account_file))
    logging.error(msg)
    raise AuthError(msg)

  if not key.get('client_email'):
    msg = ('client_email field missing in credentials json file. '
           ' Check content of %s' % service_account_file)
    logging.error(msg)
    raise AuthError(msg)

  if not key.get('private_key'):
    msg = ('private_key field missing in credentials json. '
           ' Check content of %s' % service_account_file)
    logging.error(msg)
    raise AuthError(msg)

  return key


def get_signed_jwt_assertion_credentials(credentials_filename,
                                         scope=None,
                                         service_accounts_creds_root=None):
  """Factory for SignedJwtAssertionCredentials

  Reads and validate the json credential file.

  Args:
    credentials_filename (str): path to the service account key file.
      See load_service_account_credentials() docstring for the file format.

  Keyword Args:
    scope (str|list of str): scope(s) of the credentials being
      requested. Defaults to https://www.googleapis.com/auth/userinfo.email.
    service_accounts_creds_root (str or None): location where all service
      account credentials are stored. ``credentials_filename`` is relative
      to this path. None means 'use default location'.
  """
  scope = scope or DEFAULT_SCOPES
  if isinstance(scope, basestring):
    scope = [scope]
  assert all(isinstance(s, basestring) for s in scope)

  key = load_service_account_credentials(
    credentials_filename,
    service_accounts_creds_root=service_accounts_creds_root)

  return oauth2client.client.SignedJwtAssertionCredentials(
    key['client_email'], key['private_key'], scope)


def get_authenticated_http(credentials_filename,
                           scope=None,
                           service_accounts_creds_root=None,
                           http_identifier=None,
                           timeout=DEFAULT_TIMEOUT):
  """Creates an httplib2.Http wrapped with a service account authenticator.

  Args:
    credentials_filename (str): relative path to the file containing
      credentials in json format. Path is relative to the default
      location where credentials are stored (platform-dependent).

  Keyword Args:
    scope (str|list of str): scope(s) of the credentials being
      requested. Defaults to https://www.googleapis.com/auth/userinfo.email.
    service_accounts_creds_root (str or None): location where all service
      account credentials are stored. ``credentials_filename`` is relative
      to this path. None means 'use default location'.
    http_identifier (str): if provided, returns an instrumented http request
      and use this string to identify it to ts_mon.
    timeout (int): timeout passed to httplib2.Http, in seconds.

  Returns:
    httplib2.Http authenticated with master's service account.
  """
  creds = get_signed_jwt_assertion_credentials(
    credentials_filename,
    scope=scope,
    service_accounts_creds_root=service_accounts_creds_root)

  if http_identifier:
    http = InstrumentedHttp(http_identifier, timeout=timeout)
  else:
    http = httplib2.Http(timeout=timeout)
  return creds.authorize(http)


class DelegateServiceAccountCredentials(
    oauth2client.client.AssertionCredentials):
  """Authorizes an HTTP client with a service account for which we are an actor.

  This class uses the IAM API to sign a JWT with the private key of another
  service account for which we have the "Service Account Actor" role.
  """

  MAX_TOKEN_LIFETIME_SECS = 3600 # 1 hour in seconds
  _SIGN_BLOB_URL = 'https://iam.googleapis.com/v1/%s:signBlob'

  def __init__(self, http, service_account_email, scopes, project='-'):
    """
    Args:
      http: An httplib2.Http object that is authorized by another
        oauth2client.client.OAuth2Credentials with credentials that have the
        service account actor role on the service_account_email.
      service_account_email: The email address of the service account for which
        to obtain an access token.
      scopes: The desired scopes for the token.
      project: The cloud project to which service_account_email belongs.  The
        default of '-' makes the IAM API figure it out for us.
    """

    super(DelegateServiceAccountCredentials, self).__init__(None)
    self._service_account_email = service_account_email
    self._scopes = util.scopes_to_string(scopes)
    self._http = http
    self._name = 'projects/%s/serviceAccounts/%s' % (
        project, service_account_email)

  def sign_blob(self, blob):
    response, content = self._http.request(
        self._SIGN_BLOB_URL % self._name,
        method='POST',
        body=json.dumps({'bytesToSign': base64.b64encode(blob)}),
        headers={'Content-Type': 'application/json'})
    if response.status != 200:
      raise AuthError('Failed to sign blob as %s: %d %s' % (
          self._service_account_email, response.status, response.reason))

    data = json.loads(content)
    return data['keyId'], data['signature']

  def _generate_assertion(self):
    # This is copied with small modifications from
    # oauth2client.service_account._ServiceAccountCredentials.

    header = {
        'alg': 'RS256',
        'typ': 'JWT',
    }

    now = int(time.time())
    payload = {
        'aud': self.token_uri,
        'scope': self._scopes,
        'iat': now,
        'exp': now + self.MAX_TOKEN_LIFETIME_SECS,
        'iss': self._service_account_email,
    }

    assertion_input = (
        self._urlsafe_b64encode(header) + b'.' +
        self._urlsafe_b64encode(payload))

    # Sign the assertion.
    _, rsa_bytes = self.sign_blob(assertion_input)
    signature = rsa_bytes.rstrip(b'=')

    return assertion_input + b'.' + signature

  def _urlsafe_b64encode(self, data):
    # Copied verbatim from oauth2client.service_account.
    return base64.urlsafe_b64encode(
        json.dumps(data, separators=(',', ':')).encode('UTF-8')).rstrip(b'=')


class RetriableHttp(object):
  """A httplib2.Http object that retries on failure."""

  def __init__(self, http, max_tries=5, backoff_time=1,
               retrying_statuses_fn=None):
    """
    Args:
      http: an httplib2.Http instance
      max_tries: a number of maximum tries
      backoff_time: a number of seconds to sleep between retries
      retrying_statuses_fn: a function that returns True if a given status
                            should be retried
    """
    self._http = http
    self._max_tries = max_tries
    self._backoff_time = backoff_time
    self._retrying_statuses_fn = retrying_statuses_fn or \
                                 set(range(500,599)).__contains__

  def request(self, uri, method='GET', body=None, *args, **kwargs):
    for i in range(1, self._max_tries + 1):
      try:
        response, content = self._http.request(uri, method, body, *args,
                                               **kwargs)

        if self._retrying_statuses_fn(response.status):
          logging.info('RetriableHttp: attempt %d receiving status %d, %s',
                       i, response.status,
                       'final attempt' if i == self._max_tries else \
                       'will retry')
        else:
          break
      except (ValueError, errors.Error,
              socket.timeout, socket.error, socket.herror, socket.gaierror,
              httplib2.HttpLib2Error) as error:
        logging.info('RetriableHttp: attempt %d received exception: %s, %s',
                     i, error, 'final attempt' if i == self._max_tries else \
                     'will retry')
        if i == self._max_tries:
          raise
      time.sleep(self._backoff_time)

    return response, content

  def __getattr__(self, name):
    return getattr(self._http, name)

  def __setattr__(self, name, value):
    if name in ('request', '_http', '_max_tries', '_backoff_time',
                '_retrying_statuses_fn'):
      self.__dict__[name] = value
    else:
      setattr(self._http, name, value)


class InstrumentedHttp(httplib2.Http):
  """A httplib2.Http object that reports ts_mon metrics about its requests."""

  def __init__(self, name, time_fn=time.time, timeout=DEFAULT_TIMEOUT,
               **kwargs):
    """
    Args:
      name: An identifier for the HTTP requests made by this object.
      time_fn: Function returning the current time in seconds. Use for testing
        purposes only.
    """

    super(InstrumentedHttp, self).__init__(timeout=timeout, **kwargs)
    self.fields = {'name': name, 'client': 'httplib2'}
    self.time_fn = time_fn

  def _update_metrics(self, status, start_time):
    status_fields = {'status': status}
    status_fields.update(self.fields)
    http_metrics.response_status.increment(fields=status_fields)

    duration_msec = (self.time_fn() - start_time) * 1000
    http_metrics.durations.add(duration_msec, fields=self.fields)

  def request(self, uri, method="GET", body=None, *args, **kwargs):
    request_bytes = 0
    if body is not None:
      request_bytes = len(body)
    http_metrics.request_bytes.add(request_bytes, fields=self.fields)

    start_time = self.time_fn()
    try:
      response, content = super(InstrumentedHttp, self).request(
          uri, method, body, *args, **kwargs)
    except socket.timeout:
      self._update_metrics(http_metrics.STATUS_TIMEOUT, start_time)
      raise
    except (socket.error, socket.herror, socket.gaierror):
      self._update_metrics(http_metrics.STATUS_ERROR, start_time)
      raise
    except (httplib.HTTPException, httplib2.HttpLib2Error) as ex:
      status = http_metrics.STATUS_EXCEPTION
      if 'Deadline exceeded while waiting for HTTP response' in str(ex):
        # Raised on Appengine (gae_override/httplib.py).
        status = http_metrics.STATUS_TIMEOUT
      self._update_metrics(status, start_time)
      raise
    http_metrics.response_bytes.add(len(content), fields=self.fields)

    self._update_metrics(response.status, start_time)

    return response, content


class HttpMock(object):
  """Mock of httplib2.Http"""
  HttpCall = collections.namedtuple('HttpCall', ('uri', 'method', 'body',
                                                 'headers'))

  def __init__(self, uris):
    """
    Args:
      uris(dict): list of  (uri, headers, body). `uri` is a regexp for
        matching the requested uri, (headers, body) gives the values returned
        by the mock. Uris are tested in the order from `uris`.
        `headers` is a dict mapping headers to value. The 'status' key is
        mandatory. `body` is a string.
        Ex: [('.*', {'status': 200}, 'nicely done.')]
    """
    self._uris = []
    self.requests_made = []

    for value in uris:
      if not isinstance(value, (list, tuple)) or len(value) != 3:
        raise ValueError("'uris' must be a sequence of (uri, headers, body)")
      uri, headers, body = value
      compiled_uri = re.compile(uri)
      if not isinstance(headers, dict):
        raise TypeError("'headers' must be a dict")
      if not 'status' in headers:
        raise ValueError("'headers' must have 'status' as a key")

      new_headers = copy.copy(headers)
      new_headers['status'] = int(new_headers['status'])

      if not isinstance(body, basestring):
        raise TypeError("'body' must be a string, got %s" % type(body))
      self._uris.append((compiled_uri, new_headers, body))

  # pylint: disable=unused-argument
  def request(self, uri,
              method='GET',
              body=None,
              headers=None,
              redirections=1,
              connection_type=None):
    self.requests_made.append(self.HttpCall(uri, method, body, headers))
    headers = None
    body = None
    for candidate in self._uris:
      if candidate[0].match(uri):
        _, headers, body = candidate
        break
    if not headers:
      raise AssertionError("Unexpected request to %s" % uri)
    return httplib2.Response(headers), body
