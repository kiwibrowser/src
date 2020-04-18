# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import mock
import unittest

import webapp2
import webtest

from google.appengine.ext import ndb
from google.appengine.ext import testbed

from dashboard.pinpoint.handlers import jobs
from dashboard.pinpoint.models import job as job_module


class JobsTest(unittest.TestCase):

  def setUp(self):
    app = webapp2.WSGIApplication([
        webapp2.Route(r'/jobs', jobs.Jobs),
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

  @mock.patch.object(jobs.api_auth, 'Email', mock.MagicMock(return_value=None))
  def testGet_NoUser(self):
    job = job_module.Job.New((), ())

    data = json.loads(self.testapp.get('/jobs?o=STATE').body)

    self.assertEqual(1, data['count'])
    self.assertEqual(1, len(data['jobs']))
    self.assertEqual(job.AsDict([job_module.OPTION_STATE]), data['jobs'][0])

  @mock.patch.object(jobs.api_auth, 'Email',
                     mock.MagicMock(return_value='lovely.user@example.com'))
  def testGet_WithUser(self):
    job_module.Job.New((), ())
    job_module.Job.New((), (), user='lovely.user@example.com')
    job = job_module.Job.New((), (), user='lovely.user@example.com')

    data = json.loads(self.testapp.get('/jobs?o=STATE').body)

    self.assertEqual(2, data['count'])
    self.assertEqual(2, len(data['jobs']))

    sorted_data = sorted(data['jobs'], key=lambda d: d['job_id'])
    self.assertEqual(job.AsDict([job_module.OPTION_STATE]), sorted_data[-1])
