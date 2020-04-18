# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

import webapp2
import webtest

from google.appengine.ext import ndb
from google.appengine.ext import testbed

from dashboard.pinpoint.handlers import stats
from dashboard.pinpoint.models import job as job_module


class StatsTest(unittest.TestCase):

  def setUp(self):
    app = webapp2.WSGIApplication([
        webapp2.Route(r'/stats', stats.Stats),
    ])
    self.testapp = webtest.TestApp(app)
    self.testapp.extra_environ.update({'REMOTE_ADDR': 'remote_ip'})

    self.testbed = testbed.Testbed()
    self.testbed.activate()
    self.testbed.init_datastore_v3_stub()
    self.testbed.init_memcache_stub()
    ndb.get_context().clear_cache()

  def tearDown(self):
    self.testbed.deactivate()

  def testPost_ValidRequest(self):
    # Create job.
    job = job_module.Job.New((), ())

    data = json.loads(self.testapp.get('/stats').body)

    expected = [{
        'created': job.created.isoformat(),
        'differences': 0,
        'status': 'Completed',
    }]
    self.assertEqual(data, expected)
