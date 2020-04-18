# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from google.appengine.ext import ndb
from google.appengine.ext import testbed

from dashboard.pinpoint.models import change
from dashboard.pinpoint.models import isolate


_CHANGE_1 = change.Change((change.Commit('chromium', 'f9f2b720'),))
_CHANGE_2 = change.Change((change.Commit('chromium', 'f35be4f1'),))


class IsolateTest(unittest.TestCase):

  def setUp(self):
    self.testbed = testbed.Testbed()
    self.testbed.activate()
    self.testbed.init_datastore_v3_stub()
    self.testbed.init_memcache_stub()
    ndb.get_context().clear_cache()

  def tearDown(self):
    self.testbed.deactivate()

  def testPutAndGet(self):
    isolate_infos = (
        ('Mac Builder Perf', _CHANGE_1, 'telemetry_perf', '7c7e90be'),
        ('Mac Builder Perf', _CHANGE_2, 'telemetry_perf', '38e2f262'),
    )
    isolate.Put('https://isolate.server', isolate_infos)

    isolate_server, isolate_hash = isolate.Get(
        'Mac Builder Perf', _CHANGE_1, 'telemetry_perf')
    self.assertEqual(isolate_server, 'https://isolate.server')
    self.assertEqual(isolate_hash, '7c7e90be')

  def testUnknownIsolate(self):
    with self.assertRaises(KeyError):
      isolate.Get('Wrong Builder', _CHANGE_1, 'telemetry_perf')

  # TODO: In October 2018, remove this and delete all Isolates without
  # a creation date. Isolates expire after about 6 months. crbug.com/828778
  def testOldKey(self):
    i = isolate.Isolate(
        isolate_server='https://isolate.server',
        isolate_hash='7c7e90be',
        id='3476dfea796ba01b966d08b090fff6b2ad18c8c0f1e7c91bd1f1be0f8c299050')
    i.put()

    isolate_server, isolate_hash = isolate.Get(
        'Mac Builder Perf', _CHANGE_1, 'telemetry_perf')
    self.assertEqual(isolate_server, 'https://isolate.server')
    self.assertEqual(isolate_hash, '7c7e90be')
