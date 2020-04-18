# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import unittest

from google.appengine.api import oauth
from google.appengine.api import users

from dashboard.api import api_auth
from dashboard.common import datastore_hooks
from dashboard.common import testing_common
from dashboard.common import utils


_SERVICE_ACCOUNT_USER = users.User(
    email='fake@foo.gserviceaccount.com', _auth_domain='google.com')
_AUTHORIZED_USER = users.User(
    email='test@google.com', _auth_domain='google.com')
_UNAUTHORIZED_USER = users.User(
    email='test@chromium.org', _auth_domain='foo.com')


class ApiAuthTest(testing_common.TestCase):

  def setUp(self):
    super(ApiAuthTest, self).setUp()

    patcher = mock.patch.object(api_auth.oauth, 'get_current_user')
    self.addCleanup(patcher.stop)
    self.mock_get_current_user = patcher.start()

    patcher = mock.patch.object(api_auth.oauth, 'get_client_id')
    self.addCleanup(patcher.stop)
    self.mock_get_client_id = patcher.start()

    patcher = mock.patch.object(datastore_hooks, 'SetPrivilegedRequest')
    self.addCleanup(patcher.stop)
    self.mock_set_privileged_request = patcher.start()

  def testPost_NoUser(self):
    self.mock_get_current_user.return_value = None

    with self.assertRaises(api_auth.NotLoggedInError):
      api_auth.Authorize()
    self.assertFalse(self.mock_set_privileged_request.called)

  @mock.patch.object(utils, 'IsGroupMember', mock.MagicMock(return_value=True))
  def testPost_OAuthUser(self):
    self.mock_get_current_user.return_value = _AUTHORIZED_USER
    self.mock_get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])

    api_auth.Authorize()
    self.assertTrue(self.mock_set_privileged_request.called)

  @mock.patch.object(utils, 'IsGroupMember', mock.MagicMock(return_value=True))
  def testPost_OAuthUser_ServiceAccount(self):
    self.mock_get_current_user.return_value = _SERVICE_ACCOUNT_USER

    api_auth.Authorize()
    self.assertTrue(self.mock_set_privileged_request.called)

  @mock.patch.object(utils, 'IsGroupMember', mock.MagicMock(return_value=False))
  def testPost_OAuthUser_ServiceAccount_NotInChromeperfAccess(self):
    self.mock_get_current_user.return_value = _SERVICE_ACCOUNT_USER

    api_auth.Authorize()
    self.assertFalse(self.mock_set_privileged_request.called)

  def testPost_AuthorizedUser_NotInWhitelist(self):
    self.mock_get_current_user.return_value = _AUTHORIZED_USER

    with self.assertRaises(api_auth.OAuthError):
      api_auth.Authorize()
    self.assertFalse(self.mock_set_privileged_request.called)

  @mock.patch.object(utils, 'IsGroupMember', mock.MagicMock(return_value=False))
  def testPost_OAuthUser_User_NotInChromeperfAccess(self):
    self.mock_get_current_user.return_value = _AUTHORIZED_USER
    self.mock_get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])

    api_auth.Authorize()
    self.assertFalse(self.mock_set_privileged_request.called)

  @mock.patch.object(utils, 'IsGroupMember', mock.MagicMock(return_value=True))
  def testPost_OAuthUser_User_InChromeperfAccess(self):
    self.mock_get_current_user.return_value = _AUTHORIZED_USER
    self.mock_get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])

    api_auth.Authorize()
    self.assertTrue(self.mock_set_privileged_request.called)

  def testPost_OauthUser_Unauthorized(self):
    self.mock_get_current_user.return_value = _UNAUTHORIZED_USER

    with self.assertRaises(api_auth.OAuthError):
      api_auth.Authorize()
    self.assertFalse(self.mock_set_privileged_request.called)

  def testEmail(self):
    self.mock_get_current_user.return_value = _UNAUTHORIZED_USER
    self.assertEqual(api_auth.Email(), 'test@chromium.org')

  def testEmail_NoUser(self):
    self.mock_get_current_user.side_effect = oauth.InvalidOAuthParametersError
    self.assertIsNone(api_auth.Email())


if __name__ == '__main__':
  unittest.main()
