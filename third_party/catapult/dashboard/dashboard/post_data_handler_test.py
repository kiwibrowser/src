# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

import webapp2
import webtest

from dashboard import post_data_handler
from dashboard.common import testing_common

_SAMPLE_POINT = {
    'master': 'ChromiumPerf',
    'bot': 'win7',
    'test': 'foo/bar/baz',
    'revision': '12345',
    'value': '10',
}


class PostDataHandlerTest(testing_common.TestCase):

  def setUp(self):
    super(PostDataHandlerTest, self).setUp()
    app = webapp2.WSGIApplication(
        [('/whitelist_test', post_data_handler.PostDataHandler)])
    self.testapp = webtest.TestApp(app)

  def testPost_NoIPWhitelist_Authorized(self):
    self.testapp.post('/whitelist_test', {'data': json.dumps([_SAMPLE_POINT])})

  def testPost_IPNotInWhitelist_NotAuthorized(self):
    testing_common.SetIpWhitelist(['123.45.67.89', '98.76.54.32'])
    self.testapp.post(
        '/whitelist_test', {'data': json.dumps([_SAMPLE_POINT])}, status=403,
        extra_environ={'REMOTE_ADDR': '22.45.67.89'})

  def testPost_IPInWhiteList_Authorized(self):
    testing_common.SetIpWhitelist(['123.45.67.89', '98.76.54.32'])
    self.testapp.post(
        '/whitelist_test', {'data': json.dumps([_SAMPLE_POINT])},
        extra_environ={'REMOTE_ADDR': '123.45.67.89'})


if __name__ == '__main__':
  unittest.main()
