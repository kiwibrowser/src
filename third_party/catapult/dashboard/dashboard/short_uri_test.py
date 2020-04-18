# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

import webapp2
import webtest

from dashboard import short_uri
from dashboard.common import testing_common


class ShortUriTest(testing_common.TestCase):

  def setUp(self):
    super(ShortUriTest, self).setUp()
    app = webapp2.WSGIApplication(
        [('/short_uri',
          short_uri.ShortUriHandler)])
    self.testapp = webtest.TestApp(app)

  def testPostAndGet(self):
    sample_page_state = {
        'charts': [['Chromium/win/sunspider/total', 'important']]
    }

    response = self.testapp.post(
        '/short_uri', {'page_state': json.dumps(sample_page_state)})
    page_state_id = json.loads(response.body)['sid']
    self.assertIsNotNone(page_state_id)

    response = self.testapp.get('/short_uri', {'sid': page_state_id})
    page_state = json.loads(response.body)
    self.assertEqual(sample_page_state, page_state)

  def testGet_InvalidSID(self):
    self.testapp.get('/short_uri', {'sid': '123xyz'}, status=400)

  def testGet_NoSID(self):
    self.testapp.get('/short_uri', status=400)

  def testPost_NoPageState(self):
    self.testapp.post('/short_uri', status=400)


if __name__ == '__main__':
  unittest.main()
