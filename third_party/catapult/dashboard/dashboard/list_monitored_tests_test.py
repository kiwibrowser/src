# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

import webapp2
import webtest

from dashboard import list_monitored_tests
from dashboard.common import testing_common
from dashboard.models import graph_data
from dashboard.models import sheriff


class ListMonitoredTestsTest(testing_common.TestCase):

  def setUp(self):
    super(ListMonitoredTestsTest, self).setUp()
    app = webapp2.WSGIApplication(
        [('/list_monitored_tests',
          list_monitored_tests.ListMonitoredTestsHandler)])
    self.testapp = webtest.TestApp(app)

  def _AddSampleTestData(self):
    """Adds some sample data used in the tests below."""
    master = graph_data.Master(id='TheMaster').put()
    graph_data.Bot(id='TheBot', parent=master).put()
    graph_data.TestMetadata(id='TheMaster/TheBot/Suite1').put()
    graph_data.TestMetadata(id='TheMaster/TheBot/Suite2').put()
    graph_data.TestMetadata(
        id='TheMaster/TheBot/Suite1/aaa', has_rows=True).put()
    graph_data.TestMetadata(
        id='TheMaster/TheBot/Suite1/bbb', has_rows=True).put()
    graph_data.TestMetadata(
        id='TheMaster/TheBot/Suite2/ccc', has_rows=True).put()
    graph_data.TestMetadata(
        id='TheMaster/TheBot/Suite2/ddd', has_rows=True).put()

  def _AddSheriff(self, name, email=None, url=None,
                  internal_only=False, summarize=False, patterns=None):
    """Adds a Sheriff entity to the datastore."""
    sheriff.Sheriff(
        id=name, email=email, url=url, internal_only=internal_only,
        summarize=summarize, patterns=patterns or []).put()

  def testGet_ValidSheriff_ReturnsJSONListOfTests(self):
    self._AddSheriff('X', patterns=['*/*/Suite1/*'])
    self._AddSampleTestData()
    response = self.testapp.get(
        '/list_monitored_tests', {'get-sheriffed-by': 'X'})
    self.assertEqual(
        ['TheMaster/TheBot/Suite1/aaa', 'TheMaster/TheBot/Suite1/bbb'],
        json.loads(response.body))

  def testGet_NoParameterGiven_ReturnsError(self):
    # This would raise an exception (and fail the test) if the status
    # doesn't match the given status.
    self.testapp.get('/list_monitored_tests', status=400)

  def testGet_NonExistentSheriff_ReturnsJSONEmptyList(self):
    response = self.testapp.get(
        '/list_monitored_tests', {'get-sheriffed-by': 'Bogus Sheriff'})
    self.assertEqual([], json.loads(response.body))


if __name__ == '__main__':
  unittest.main()
