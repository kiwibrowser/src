# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import mock
import webapp2
import webtest

# pylint: disable=unused-import
from dashboard import mock_oauth2_decorator
# pylint: enable=unused-import

from dashboard import associate_alerts
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import sheriff
from dashboard.services import issue_tracker_service


class AssociateAlertsTest(testing_common.TestCase):

  def setUp(self):
    super(AssociateAlertsTest, self).setUp()
    app = webapp2.WSGIApplication([(
        '/associate_alerts', associate_alerts.AssociateAlertsHandler)])
    self.testapp = webtest.TestApp(app)
    testing_common.SetSheriffDomains(['chromium.org'])
    self.SetCurrentUser('foo@chromium.org', is_admin=True)

  def _AddSheriff(self):
    """Adds a Sheriff and returns its key."""
    return sheriff.Sheriff(
        id='Chromium Perf Sheriff', email='sullivan@google.com').put()

  def _AddTests(self):
    """Adds sample Tests and returns a list of their keys."""
    testing_common.AddTests(['ChromiumGPU'], ['linux-release'], {
        'scrolling-benchmark': {
            'first_paint': {},
            'mean_frame_time': {},
        }
    })
    return map(utils.TestKey, [
        'ChromiumGPU/linux-release/scrolling-benchmark/first_paint',
        'ChromiumGPU/linux-release/scrolling-benchmark/mean_frame_time',
    ])

  def _AddAnomalies(self):
    """Adds sample Anomaly data and returns a dict of revision to key."""
    sheriff_key = self._AddSheriff()
    test_keys = self._AddTests()
    key_map = {}

    # Add anomalies to the two tests alternately.
    for end_rev in range(10000, 10120, 10):
      test_key = test_keys[0] if end_rev % 20 == 0 else test_keys[1]
      anomaly_key = anomaly.Anomaly(
          start_revision=(end_rev - 5), end_revision=end_rev, test=test_key,
          median_before_anomaly=100, median_after_anomaly=200,
          sheriff=sheriff_key).put()
      key_map[end_rev] = anomaly_key.urlsafe()

    # Add an anomaly that overlaps.
    anomaly_key = anomaly.Anomaly(
        start_revision=9990, end_revision=9996, test=test_keys[0],
        median_before_anomaly=100, median_after_anomaly=200,
        sheriff=sheriff_key).put()
    key_map[9996] = anomaly_key.urlsafe()

    # Add an anomaly that overlaps and has bug ID.
    anomaly_key = anomaly.Anomaly(
        start_revision=9990, end_revision=9997, test=test_keys[0],
        median_before_anomaly=100, median_after_anomaly=200,
        sheriff=sheriff_key, bug_id=12345).put()
    key_map[9997] = anomaly_key.urlsafe()
    return key_map

  def testGet_NoKeys_ShowsError(self):
    response = self.testapp.get('/associate_alerts')
    self.assertIn('<div class="error">', response.body)

  def testGet_SameAsPost(self):
    get_response = self.testapp.get('/associate_alerts')
    post_response = self.testapp.post('/associate_alerts')
    self.assertEqual(get_response.body, post_response.body)

  def testGet_InvalidBugId_ShowsError(self):
    key_map = self._AddAnomalies()
    response = self.testapp.get(
        '/associate_alerts?keys=%s&bug_id=foo' % key_map[9996])
    self.assertIn('<div class="error">', response.body)
    self.assertIn('Invalid bug ID', response.body)

  # Mocks fetching bugs from issue tracker.
  @mock.patch('services.issue_tracker_service.discovery.build',
              mock.MagicMock())
  @mock.patch.object(
      issue_tracker_service.IssueTrackerService, 'List',
      mock.MagicMock(return_value={
          'items': [
              {
                  'id': 12345,
                  'summary': '5% regression in bot/suite/x at 10000:20000',
                  'state': 'open',
                  'status': 'New',
                  'author': {'name': 'exam...@google.com'},
              },
              {
                  'id': 13579,
                  'summary': '1% regression in bot/suite/y at 10000:20000',
                  'state': 'closed',
                  'status': 'WontFix',
                  'author': {'name': 'exam...@google.com'},
              },
          ]}))
  def testGet_NoBugId_ShowsDialog(self):
    # When a GET request is made with some anomaly keys but no bug ID,
    # A HTML form is shown for the user to input a bug number.
    key_map = self._AddAnomalies()
    response = self.testapp.get('/associate_alerts?keys=%s' % key_map[10000])
    # The response contains a table of recent bugs and a form.
    self.assertIn('12345', response.body)
    self.assertIn('13579', response.body)
    self.assertIn('<form', response.body)

  def testGet_WithBugId_AlertIsAssociatedWithBugId(self):
    # When the bug ID is given and the alerts overlap, then the Anomaly
    # entities are updated and there is a response indicating success.
    key_map = self._AddAnomalies()
    response = self.testapp.get(
        '/associate_alerts?keys=%s,%s&bug_id=12345' % (
            key_map[9996], key_map[10000]))
    # The response page should have a bug number.
    self.assertIn('12345', response.body)
    # The Anomaly entities should be updated.
    for anomaly_entity in anomaly.Anomaly.query().fetch():
      if anomaly_entity.end_revision in (10000, 9996):
        self.assertEqual(12345, anomaly_entity.bug_id)
      elif anomaly_entity.end_revision != 9997:
        self.assertIsNone(anomaly_entity.bug_id)

  def testGet_TargetBugHasNoAlerts_DoesNotAskForConfirmation(self):
    # Associating alert with bug ID that has no alerts is always OK.
    key_map = self._AddAnomalies()
    response = self.testapp.get(
        '/associate_alerts?keys=%s,%s&bug_id=578' % (
            key_map[9996], key_map[10000]))
    # The response page should have a bug number.
    self.assertIn('578', response.body)
    # The Anomaly entities should be updated.
    self.assertEqual(
        578, anomaly.Anomaly.query(
            anomaly.Anomaly.end_revision == 9996).get().bug_id)
    self.assertEqual(
        578, anomaly.Anomaly.query(
            anomaly.Anomaly.end_revision == 10000).get().bug_id)

  def testGet_NonOverlappingAlerts_AsksForConfirmation(self):
    # Associating alert with bug ID that contains non-overlapping revision
    # ranges should show a confirmation page.
    key_map = self._AddAnomalies()
    response = self.testapp.get(
        '/associate_alerts?keys=%s,%s&bug_id=12345' % (
            key_map[10000], key_map[10010]))
    # The response page should show confirmation page.
    self.assertIn('Do you want to continue?', response.body)
    # The Anomaly entities should not be updated.
    for anomaly_entity in anomaly.Anomaly.query().fetch():
      if anomaly_entity.end_revision != 9997:
        self.assertIsNone(anomaly_entity.bug_id)

  def testGet_WithConfirm_AssociatesWithNewBugId(self):
    # Associating alert with bug ID and with confirmed non-overlapping revision
    # range should update alert with bug ID.
    key_map = self._AddAnomalies()
    response = self.testapp.get(
        '/associate_alerts?confirm=true&keys=%s,%s&bug_id=12345' % (
            key_map[10000], key_map[10010]))
    # The response page should have the bug number.
    self.assertIn('12345', response.body)
    # The Anomaly entities should be updated.
    for anomaly_entity in anomaly.Anomaly.query().fetch():
      if anomaly_entity.end_revision in (10000, 10010):
        self.assertEqual(12345, anomaly_entity.bug_id)
      elif anomaly_entity.end_revision != 9997:
        self.assertIsNone(anomaly_entity.bug_id)

  def testRevisionRangeFromSummary(self):
    # If the summary is in the expected format, a pair is returned.
    self.assertEqual(
        (10000, 10500),
        associate_alerts._RevisionRangeFromSummary(
            '1% regression in bot/my_suite/test at 10000:10500'))
    # Otherwise None is returned.
    self.assertIsNone(
        associate_alerts._RevisionRangeFromSummary(
            'Regression in rev ranges 12345 to 20000'))

  def testRangesOverlap_NonOverlapping_ReturnsFalse(self):
    self.assertFalse(associate_alerts._RangesOverlap((1, 5), (6, 9)))
    self.assertFalse(associate_alerts._RangesOverlap((6, 9), (1, 5)))

  def testRangesOverlap_NoneGiven_ReturnsFalse(self):
    self.assertFalse(associate_alerts._RangesOverlap((1, 5), None))
    self.assertFalse(associate_alerts._RangesOverlap(None, (1, 5)))
    self.assertFalse(associate_alerts._RangesOverlap(None, None))

  def testRangesOverlap_OneIncludesOther_ReturnsTrue(self):
    # True if one range envelopes the other.
    self.assertTrue(associate_alerts._RangesOverlap((1, 9), (2, 5)))
    self.assertTrue(associate_alerts._RangesOverlap((2, 5), (1, 9)))

  def testRangesOverlap_PartlyOverlap_ReturnsTrue(self):
    self.assertTrue(associate_alerts._RangesOverlap((1, 6), (5, 9)))
    self.assertTrue(associate_alerts._RangesOverlap((5, 9), (1, 6)))

  def testRangesOverlap_CommonBoundary_ReturnsTrue(self):
    self.assertTrue(associate_alerts._RangesOverlap((1, 6), (6, 9)))
    self.assertTrue(associate_alerts._RangesOverlap((6, 9), (1, 6)))


if __name__ == '__main__':
  unittest.main()
