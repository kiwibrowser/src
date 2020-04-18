# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import webapp2
import webtest

from google.appengine.ext import ndb

from dashboard import get_histogram
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import histogram

_TEST_HISTOGRAM_DATA = {
    'binBoundaries': [1, [1, 1000, 20]],
    'diagnostics': {
        'buildbot': 'e9c2891d-2b04-413f-8cf4-099827e67626',
        'revisions': '25f0a111-9bb4-4cea-b0c1-af2609623160',
        'telemetry': '0bc1021b-8107-4db7-bc8c-49d7cf53c5ae'
    },
    'guid': '4989617a-14d6-4f80-8f75-dafda2ff13b0',
    'name': 'foo',
    'unit': 'count'
}


class GetHistogramsTest(testing_common.TestCase):
  def setUp(self):
    super(GetHistogramsTest, self).setUp()
    app = webapp2.WSGIApplication([
        ('/get_histogram', get_histogram.GetHistogramHandler)])
    self.testapp = webtest.TestApp(app)
    testing_common.SetIsInternalUser('foo@bar.com', True)
    self.SetCurrentUser('foo@bar.com', is_admin=True)

  def _AddMockData(self, histogram_data, internal_only=False):
    histogram.Histogram(
        id=histogram_data['guid'], test=ndb.Key('TestMetadata', 'M/B/S'),
        revision=10, data=histogram_data,
        internal_only=internal_only).put()

  def testGetHistogram(self):
    self._AddMockData(_TEST_HISTOGRAM_DATA)

    response = self.testapp.post(
        '/get_histogram', {'guid': _TEST_HISTOGRAM_DATA['guid']})
    data = json.loads(response.body)

    self.assertTrue(type(data) is dict)
    self.assertEqual(_TEST_HISTOGRAM_DATA, data)

  def testGetHistogram_Internal_Succeeds(self):
    self._AddMockData(_TEST_HISTOGRAM_DATA, True)

    response = self.testapp.post(
        '/get_histogram', {'guid': _TEST_HISTOGRAM_DATA['guid']})
    data = json.loads(response.body)

    self.assertTrue(type(data) is dict)
    self.assertEqual(_TEST_HISTOGRAM_DATA, data)

  def testGetHistogram_Internal_Fails(self):
    self.UnsetCurrentUser()
    self._AddMockData(_TEST_HISTOGRAM_DATA, True)

    self.assertFalse(utils.IsInternalUser())
    self.testapp.post(
        '/get_histogram', {'guid': _TEST_HISTOGRAM_DATA['guid']}, status=400)

  def testGetHistogram_WrongGuid(self):
    self.testapp.post('/get_histogram', {'guid': 'foo'}, status=400)

  def testGetHistogram_MissingParam(self):
    self.testapp.post('/get_histogram', status=400)
