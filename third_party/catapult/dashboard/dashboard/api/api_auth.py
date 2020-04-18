# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from google.appengine.api import oauth

from dashboard.common import datastore_hooks
from dashboard.common import utils


OAUTH_CLIENT_ID_WHITELIST = [
    # This oauth client id is from Pinpoint.
    '62121018386-aqdfougp0ddn93knqj6g79vvn42ajmrg.apps.googleusercontent.com',
    # This oauth client id is from the 'chromeperf' API console.
    '62121018386-h08uiaftreu4dr3c4alh3l7mogskvb7i.apps.googleusercontent.com',
    # This oauth client id is from chromiumdash-staging.
    '377415874083-slpqb5ur4h9sdfk8anlq4qct9imivnmt.apps.googleusercontent.com',
    # This oauth client id is from chromiumdash.
    '975044924533-p122oecs8h6eibv5j5a8fmj82b0ct0nk.apps.googleusercontent.com',
    # This oauth client id is used to upload histograms from the perf waterfall.
    '113172445342431053212',
    'chromeperf@webrtc-perf-test.google.com.iam.gserviceaccount.com',
    # This oauth client id is used to upload histograms when debugging Fuchsia
    # locally (e.g. in a cron-job).
    'catapult-uploader@fuchsia-infra.iam.gserviceaccount.com',
    # This oauth client id is used to upload histograms from Fuchsia dev
    # builders.
    'garnet-ci-builder-dev@fuchsia-infra.iam.gserviceaccount.com',
    # This oauth client id is used from Fuchsia Garnet builders.
    'garnet-ci-builder@fuchsia-infra.iam.gserviceaccount.com',
    # This oauth client id used to upload histograms from cronet bots.
    '113172445342431053212'
]
OAUTH_SCOPES = (
    'https://www.googleapis.com/auth/userinfo.email',
)


class ApiAuthException(Exception):
  pass


class OAuthError(ApiAuthException):
  def __init__(self):
    super(OAuthError, self).__init__('User authentication error')


class NotLoggedInError(ApiAuthException):
  def __init__(self):
    super(NotLoggedInError, self).__init__('User not authenticated')


class InternalOnlyError(ApiAuthException):
  def __init__(self):
    super(InternalOnlyError, self).__init__('User does not have access')


def Authorize():
  try:
    user = oauth.get_current_user(OAUTH_SCOPES)
  except oauth.Error:
    raise NotLoggedInError

  if not user:
    raise NotLoggedInError

  try:
    if not user.email().endswith('.gserviceaccount.com'):
      # For non-service account, need to verify that the OAuth client ID
      # is in our whitelist.
      client_id = oauth.get_client_id(OAUTH_SCOPES)
      if client_id not in OAUTH_CLIENT_ID_WHITELIST:
        logging.info('OAuth client id %s for user %s not in whitelist',
                     client_id, user.email())
        user = None
        raise OAuthError
  except oauth.Error:
    raise OAuthError

  logging.info('OAuth user logged in as: %s', user.email())
  if utils.IsGroupMember(user.email(), 'chromeperf-access'):
    datastore_hooks.SetPrivilegedRequest()


def Email():
  """Retrieves the email address of the logged-in user.

  Returns:
    The email address, as a string or None if there is no user logged in.
  """
  try:
    return oauth.get_current_user(OAUTH_SCOPES).email()
  except oauth.InvalidOAuthParametersError:
    return None
