# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

import webapp2
import webtest

from google.appengine.ext import ndb

from dashboard import group_report
from dashboard import short_uri
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import bug_data
from dashboard.models import page_state
from dashboard.models import sheriff


class GroupReportTest(testing_common.TestCase):

  def setUp(self):
    super(GroupReportTest, self).setUp()
    app = webapp2.WSGIApplication(
        [('/group_report', group_report.GroupReportHandler)])
    self.testapp = webtest.TestApp(app)

  def _AddAnomalyEntities(
      self, revision_ranges, test_key, sheriff_key, bug_id=None):
    """Adds a group of Anomaly entities to the datastore."""
    urlsafe_keys = []
    for start_rev, end_rev in revision_ranges:
      anomaly_key = anomaly.Anomaly(
          start_revision=start_rev, end_revision=end_rev,
          test=test_key, bug_id=bug_id, sheriff=sheriff_key,
          median_before_anomaly=100, median_after_anomaly=200).put()
      urlsafe_keys.append(anomaly_key.urlsafe())
    return urlsafe_keys

  def _AddTests(self):
    """Adds sample TestMetadata entities and returns their keys."""
    testing_common.AddTests(['ChromiumGPU'], ['linux-release'], {
        'scrolling-benchmark': {
            'first_paint': {},
            'mean_frame_time': {},
        }
    })
    keys = [
        utils.TestKey(
            'ChromiumGPU/linux-release/scrolling-benchmark/first_paint'),
        utils.TestKey(
            'ChromiumGPU/linux-release/scrolling-benchmark/mean_frame_time'),
    ]
    # By default, all TestMetadata entities have an improvement_direction of
    # UNKNOWN, meaning that neither direction is considered an improvement.
    # Here we set the improvement direction so that some anomalies are
    # considered improvements.
    for test_key in keys:
      test = test_key.get()
      test.improvement_direction = anomaly.DOWN
      test.put()
    return keys

  def _AddSheriff(self):
    """Adds a Sheriff entity and returns the key."""
    return sheriff.Sheriff(
        id='Chromium Perf Sheriff', email='sullivan@google.com').put()

  def testGet(self):
    response = self.testapp.get('/group_report')
    self.assertEqual('text/html', response.content_type)
    self.assertIn('Chrome Performance Dashboard', response.body)

  def testPost_WithAnomalyKeys_ShowsSelectedAndOverlapping(self):
    sheriff_key = self._AddSheriff()
    test_keys = self._AddTests()
    selected_ranges = [(400, 900), (200, 700)]
    overlapping_ranges = [(300, 500), (500, 600), (600, 800)]
    non_overlapping_ranges = [(100, 200)]
    selected_keys = self._AddAnomalyEntities(
        selected_ranges, test_keys[0], sheriff_key)
    self._AddAnomalyEntities(
        overlapping_ranges, test_keys[0], sheriff_key)
    self._AddAnomalyEntities(
        non_overlapping_ranges, test_keys[0], sheriff_key)

    response = self.testapp.post(
        '/group_report?keys=%s' % ','.join(selected_keys))
    alert_list = self.GetJsonValue(response, 'alert_list')

    # Confirm the first N keys are the selected keys.
    first_keys = [alert_list[i]['key'] for i in xrange(len(selected_keys))]
    self.assertSetEqual(set(selected_keys), set(first_keys))

    # Expect selected alerts + overlapping alerts,
    # but not the non-overlapping alert.
    self.assertEqual(5, len(alert_list))

  def testPost_WithInvalidSidParameter_ShowsError(self):
    response = self.testapp.post('/group_report?sid=foobar')
    error = self.GetJsonValue(response, 'error')
    self.assertIn('No anomalies specified', error)

  def testPost_WithValidSidParameter(self):
    sheriff_key = self._AddSheriff()
    test_keys = self._AddTests()
    selected_ranges = [(400, 900), (200, 700)]
    selected_keys = self._AddAnomalyEntities(
        selected_ranges, test_keys[0], sheriff_key)

    json_keys = json.dumps(selected_keys)
    state_id = short_uri.GenerateHash(','.join(selected_keys))
    page_state.PageState(id=state_id, value=json_keys).put()

    response = self.testapp.post('/group_report?sid=%s' % state_id)
    alert_list = self.GetJsonValue(response, 'alert_list')

    # Confirm the first N keys are the selected keys.
    first_keys = [alert_list[i]['key'] for i in xrange(len(selected_keys))]
    self.assertSetEqual(set(selected_keys), set(first_keys))
    self.assertEqual(2, len(alert_list))

  def testPost_WithKeyOfNonExistentAlert_ShowsError(self):
    key = ndb.Key('Anomaly', 123)
    response = self.testapp.post('/group_report?keys=%s' % key.urlsafe())
    error = self.GetJsonValue(response, 'error')
    self.assertEqual('No Anomaly found for key %s.' % key.urlsafe(), error)

  def testPost_WithInvalidKeyParameter_ShowsError(self):
    response = self.testapp.post('/group_report?keys=foobar')
    error = self.GetJsonValue(response, 'error')
    self.assertIn('Invalid Anomaly key', error)

  def testPost_WithRevParameter(self):
    # If the rev parameter is given, then all alerts whose revision range
    # includes the given revision should be included.
    sheriff_key = self._AddSheriff()
    test_keys = self._AddTests()
    self._AddAnomalyEntities(
        [(190, 210), (200, 300), (100, 200), (400, 500)],
        test_keys[0], sheriff_key)
    response = self.testapp.post('/group_report?rev=200')
    alert_list = self.GetJsonValue(response, 'alert_list')
    self.assertEqual(3, len(alert_list))

  def testPost_WithInvalidRevParameter_ShowsError(self):
    response = self.testapp.post('/group_report?rev=foo')
    error = self.GetJsonValue(response, 'error')
    self.assertEqual('Invalid rev "foo".', error)

  def testPost_WithBugIdParameter(self):
    sheriff_key = self._AddSheriff()
    test_keys = self._AddTests()
    bug_data.Bug(id=123).put()
    self._AddAnomalyEntities(
        [(200, 300), (100, 200), (400, 500)],
        test_keys[0], sheriff_key, bug_id=123)
    self._AddAnomalyEntities(
        [(150, 250)], test_keys[0], sheriff_key)
    response = self.testapp.post('/group_report?bug_id=123')
    alert_list = self.GetJsonValue(response, 'alert_list')
    self.assertEqual(3, len(alert_list))

  def testPost_WithInvalidBugIdParameter_ShowsError(self):
    response = self.testapp.post('/group_report?bug_id=foo')
    alert_list = self.GetJsonValue(response, 'alert_list')
    self.assertIsNone(alert_list)
    error = self.GetJsonValue(response, 'error')
    self.assertEqual('Invalid bug ID "foo".', error)


if __name__ == '__main__':
  unittest.main()
