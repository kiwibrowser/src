# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json

import webapp2
import webtest

from dashboard.common import namespaced_stored_object
from dashboard.common import testing_common
from dashboard.pinpoint.handlers import config


class ConfigTest(testing_common.TestCase):

  def setUp(self):
    super(ConfigTest, self).setUp()
    app = webapp2.WSGIApplication([
        webapp2.Route(r'/api/config', config.Config),
    ])
    self.testapp = webtest.TestApp(app)

    self.SetCurrentUser('external@chromium.org')

    namespaced_stored_object.Set('bot_configurations', {
        'chromium-rel-mac11-pro': {},
    })

    self.SetCurrentUser('internal@chromium.org', is_admin=True)
    testing_common.SetIsInternalUser('internal@chromium.org', True)

    namespaced_stored_object.Set('bot_configurations', {
        'internal-only-bot': {},
    })

  def testGet_External(self):
    self.SetCurrentUser('external@chromium.org')

    actual = json.loads(self.testapp.get('/api/config').body)
    expected = {
        'configurations': ['chromium-rel-mac11-pro'],
    }
    self.assertEqual(actual, expected)

  def testGet_Internal(self):
    self.SetCurrentUser('internal@chromium.org')

    actual = json.loads(self.testapp.get('/api/config').body)
    expected = {
        'configurations': ['internal-only-bot'],
    }
    self.assertEqual(actual, expected)
