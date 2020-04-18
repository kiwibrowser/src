# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Mocks out oauth2_decorator for unit testing."""

from apiclient import http
from dashboard import oauth2_decorator


class MockOAuth2Decorator(object):
  """Mocks OAuth2Decorator for testing."""

  def __init__(self, client_id, client_secret, scope, message, callback_path):
    self.client_id = client_id
    self.client_secret = client_secret
    self.scope = scope
    self.message = message
    self.callback_path = callback_path

  # Lowercase method names are used in this class to match those
  # in oauth2client.appengine.Oauth2Decorator.
  # pylint: disable=invalid-name

  def http(self):
    return http.HttpMock(headers={'status': '200'})

  def oauth_required(self, method):
    def check_oauth(request_handler, *args, **kwargs):
      resp = method(request_handler, *args, **kwargs)
      return resp
    return check_oauth


oauth2_decorator.DECORATOR = MockOAuth2Decorator(
    client_id='client_id',
    client_secret='client_secret',
    scope='scope',
    message='message',
    callback_path='callback_path')
