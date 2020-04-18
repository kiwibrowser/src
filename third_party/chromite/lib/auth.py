# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Functions for authenticating httplib2 requests with OAuth2 tokens."""

from __future__ import print_function

import os

from chromite.lib import cipd
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import retry_util
from chromite.lib import path_util

# from third_party
import httplib2

REFRESH_STATUS_CODES = [401]

# Retry times on get_access_token
RETRY_GET_ACCESS_TOKEN = 3


class AccessTokenError(Exception):
  """Error accessing the token."""


def GetLuciAuth(instance_id='latest'):
  """Returns a path to the luci-auth binary.

  This will download and install the luci-auth package if it is not already
  deployed.

  Args:
    instance_id: The instance-id of the package to install. Defaults to 'latest'

  Returns:
    the path to the luci-auth binary.
  """
  cache_dir = os.path.join(path_util.GetCacheDir(), 'cipd/packages')
  path = cipd.InstallPackage(
      cipd.GetCIPDFromCache(),
      'infra/tools/luci-auth/linux-amd64',
      instance_id,
      destination=cache_dir)

  return os.path.join(path, 'luci-auth')


def Login(service_account_json=None):
  """Logs a user into chrome-infra-auth using luci-auth.

  Runs 'luci-auth login' to get a OAuth2 refresh token.

  Args:
    service_account_json: A optional path to a service account.

  Raises:
    AccessTokenError if login command failed.
  """
  logging.info('Logging into chrome-infra-auth with service_account %s',
               service_account_json)

  cmd = [GetLuciAuth(), 'login']
  if service_account_json:
    cmd += ['-service-account-json=%s' % service_account_json]

  result = cros_build_lib.RunCommand(
      cmd,
      mute_output=False,
      error_code_ok=True)

  if result.returncode:
    raise AccessTokenError('Failed at  logging in to chrome-infra-auth: %s,'
                           ' may retry.')


def Token(service_account_json=None):
  """Get the token using luci-auth.

  Runs 'luci-auth token' to get the OAuth2 token.

  Args:
    service_account_json: A optional path to a service account.

  Returns:
    The token string if the command succeeded;

  Raises:
    AccessTokenError if token command failed.
  """
  cmd = [GetLuciAuth(), 'token']
  if service_account_json:
    cmd += ['-service-account-json=%s' % service_account_json]

  result = cros_build_lib.RunCommand(
      cmd,
      capture_output=True,
      error_code_ok=True)

  if result.returncode:
    raise AccessTokenError('Failed at getting the access token, may retry.')

  return result.output.strip()


def _TokenAndLoginIfNeed(service_account_json=None, force_token_renew=False):
  """Run Token and Login opertions.

  If force_token_renew is on, run Login operation first to force token renew,
  then run Token operation to return token string.
  If force_token_renew is off, run Token operation first. If no token found,
  run Login operation to refresh the token. Throw an AccessTokenError after
  running the Login operation, so that GetAccessToken can retry on
  _TokenAndLoginIfNeed.

  Args:
    service_account_json: A optional path to a service account.
    force_token_renew: Boolean indicating whether to force login to renew token
      before returning a token. Default to False.

  Returns:
    The token string if the command succeeded; else, None.

  Raises:
    AccessTokenError if the Token operation failed.
  """
  if force_token_renew:
    Login(service_account_json=service_account_json)
    return Token(service_account_json=service_account_json)
  else:
    try:
      return Token(service_account_json=service_account_json)
    except AccessTokenError as e:
      Login(service_account_json=service_account_json)
      # Raise the error and let the caller decide wether to retry
      raise e


def GetAccessToken(**kwargs):
  """Returns an OAuth2 access token using luci-auth.

  Retry the _TokenAndLoginIfNeed function when the error threw is an
  AccessTokenError.

  Args:
    kwargs: A list of keyword arguments to pass to _TokenAndLoginIfNeed.

  Returns:
    The access token string or None if failed to get access token.
  """
  service_account_json = kwargs.get('service_account_json')
  force_token_renew = kwargs.get('force_token_renew', False)
  retry = lambda e: isinstance(e, AccessTokenError)
  try:
    result = retry_util.GenericRetry(
        retry, RETRY_GET_ACCESS_TOKEN,
        _TokenAndLoginIfNeed,
        service_account_json=service_account_json,
        force_token_renew=force_token_renew,
        sleep=3)
    return result
  except AccessTokenError as e:
    logging.error('Failed at getting the access token: %s ', e)
    # Do not raise the AccessTokenError here.
    # Let the response returned by the request handler
    # tell the status and errors.
    return


class AuthorizedHttp(object):
  """Authorized http instance"""

  def __init__(self, get_access_token, http, **kwargs):
    self.get_access_token = get_access_token
    self.http = http if http is not None else httplib2.Http()
    self.token = self.get_access_token(**kwargs)
    self.kwargs = kwargs

  # Adapted from oauth2client.OAuth2Credentials.authorize.
  # We can't use oauthclient2 because the import will fail on slaves due to
  # missing PyOpenSSL (crbug.com/498467).
  def request(self, *args, **kwargs):
    headers = kwargs.get('headers', {}).copy()
    headers['Authorization'] = 'Bearer %s' % self.token
    kwargs['headers'] = headers

    resp, content = self.http.request(*args, **kwargs)
    if resp.status in REFRESH_STATUS_CODES:
      logging.info('Refreshing due to a %s', resp.status)

      # Token expired, force token renew
      kwargs_copy = dict(self.kwargs, force_token_renew=True)
      self.token = self.get_access_token(**kwargs_copy)

      # TODO(phobbs): delete the "access_token" key from the token file used.
      headers['Authorization'] = 'Bearer %s' % self.token
      resp, content = self.http.request(*args, **kwargs)

    return resp, content
