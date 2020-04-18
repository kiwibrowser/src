# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import webapp2
import webtest

from google.appengine.api import users

from dashboard.common import request_handler
from dashboard.common import testing_common
from dashboard.common import xsrf


class ExampleHandler(request_handler.RequestHandler):
  """Example request handler that uses a XSRF token."""

  @xsrf.TokenRequired
  def post(self):
    pass


class XsrfTest(testing_common.TestCase):

  def setUp(self):
    super(XsrfTest, self).setUp()
    app = webapp2.WSGIApplication([('/example', ExampleHandler)])
    self.testapp = webtest.TestApp(app)

  def testGenerateToken_CanBeValidatedWithSameUser(self):
    self.SetCurrentUser('foo@bar.com')
    token = xsrf.GenerateToken(users.get_current_user())
    self.assertTrue(xsrf._ValidateToken(token, users.get_current_user()))

  def testGenerateToken_CanNotBeValidatedWithDifferentUser(self):
    self.SetCurrentUser('foo@bar.com', user_id='x')
    token = xsrf.GenerateToken(users.get_current_user())
    self.SetCurrentUser('foo@other.com', user_id='y')
    self.assertFalse(xsrf._ValidateToken(token, users.get_current_user()))

  def testTokenRequired_NoToken_Returns403(self):
    self.testapp.post('/example', {}, status=403)

  def testTokenRequired_BogusToken_Returns403(self):
    self.testapp.post(
        '/example',
        {'xsrf_token': 'abcdefghijklmnopqrstuvwxyz0123456789'},
        status=403)

  def testTokenRequired_CorrectToken_Success(self):
    self.SetCurrentUser('foo@bar.com')
    token = xsrf.GenerateToken(users.get_current_user())
    self.testapp.post('/example', {'xsrf_token': token})


if __name__ == '__main__':
  unittest.main()
