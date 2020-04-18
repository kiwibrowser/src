# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import mock
import datetime
import unittest

import webapp2
import webtest

from google.appengine.api import users
from google.appengine.ext import ndb

from dashboard.api import alerts
from dashboard.api import api_auth
from dashboard.common import datastore_hooks
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import bug_data
from dashboard.models import sheriff


GOOGLER_USER = users.User(email='sullivan@chromium.org',
                          _auth_domain='google.com')
NON_GOOGLE_USER = users.User(email='foo@bar.com', _auth_domain='bar.com')


class AlertsTest(testing_common.TestCase):

  def setUp(self):
    super(AlertsTest, self).setUp()
    app = webapp2.WSGIApplication(
        [(r'/api/alerts/(.*)', alerts.AlertsHandler)])
    self.testapp = webtest.TestApp(app)

  def _AddAnomalyEntities(
      self, revision_ranges, test_key, sheriff_key, bug_id=None,
      internal_only=False, timestamp=None):
    """Adds a group of Anomaly entities to the datastore."""
    urlsafe_keys = []
    for start_rev, end_rev in revision_ranges:
      anomaly_entity = anomaly.Anomaly(
          start_revision=start_rev, end_revision=end_rev,
          test=test_key, bug_id=bug_id, sheriff=sheriff_key,
          median_before_anomaly=100, median_after_anomaly=200,
          internal_only=internal_only)
      if timestamp:
        anomaly_entity.timestamp = timestamp
      anomaly_key = anomaly_entity.put()
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

  def _SetGooglerOAuth(self, mock_oauth):
    mock_oauth.get_current_user.return_value = GOOGLER_USER
    mock_oauth.get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])

  @mock.patch.object(api_auth, 'oauth')
  def testPost_WithAnomalyKeys_ShowsSelectedAndOverlapping(self, mock_oauth):
    mock_oauth.get_current_user.return_value = GOOGLER_USER
    mock_oauth.get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])
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
        '/api/alerts/keys/%s' % ','.join(selected_keys))
    anomalies = self.GetJsonValue(response, 'anomalies')

    # Expect selected alerts + overlapping alerts,
    # but not the non-overlapping alert.
    self.assertEqual(5, len(anomalies))

  @mock.patch.object(api_auth, 'oauth')
  def testPost_WithKeyOfNonExistentAlert_ShowsError(self, mock_oauth):
    self._SetGooglerOAuth(mock_oauth)
    key = ndb.Key('Anomaly', 123)
    response = self.testapp.post('/api/alerts/keys/%s' % key.urlsafe(),
                                 status=400)
    self.assertIn('No Anomaly found for key %s.' % key.urlsafe(), response.body)

  @mock.patch.object(api_auth, 'oauth')
  def testPost_WithRevParameter(self, mock_oauth):
    self._SetGooglerOAuth(mock_oauth)
    # If the rev parameter is given, then all alerts whose revision range
    # includes the given revision should be included.
    sheriff_key = self._AddSheriff()
    test_keys = self._AddTests()
    self._AddAnomalyEntities(
        [(190, 210), (200, 300), (100, 200), (400, 500)],
        test_keys[0], sheriff_key)
    response = self.testapp.post('/api/alerts/rev/200')
    anomalies = self.GetJsonValue(response, 'anomalies')
    self.assertEqual(3, len(anomalies))

  @mock.patch.object(api_auth, 'oauth')
  def testPost_WithInvalidRevParameter_ShowsError(self, mock_oauth):
    self._SetGooglerOAuth(mock_oauth)
    response = self.testapp.post('/api/alerts/rev/foo', status=400)
    self.assertEqual(
        {'error': 'Invalid rev "foo".'}, json.loads(response.body))

  @mock.patch.object(api_auth, 'oauth')
  def testPost_WithBugIdParameter(self, mock_oauth):
    self._SetGooglerOAuth(mock_oauth)
    sheriff_key = self._AddSheriff()
    test_keys = self._AddTests()
    bug_data.Bug(id=123).put()
    self._AddAnomalyEntities(
        [(200, 300), (100, 200), (400, 500)],
        test_keys[0], sheriff_key, bug_id=123)
    self._AddAnomalyEntities(
        [(150, 250)], test_keys[0], sheriff_key)
    response = self.testapp.post('/api/alerts/bug_id/123')
    anomalies = self.GetJsonValue(response, 'anomalies')
    self.assertEqual(3, len(anomalies))

  @mock.patch.object(utils, 'IsGroupMember')
  @mock.patch.object(api_auth, 'oauth')
  def testPost_WithBugIdParameterExternalUser_ExternaData(
      self, mock_oauth, mock_utils):
    mock_oauth.get_current_user.return_value = NON_GOOGLE_USER
    mock_oauth.get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])
    mock_utils.return_value = False
    datastore_hooks.InstallHooks()
    sheriff_key = self._AddSheriff()
    test_keys = self._AddTests()
    bug_data.Bug(id=123).put()
    self._AddAnomalyEntities(
        [(200, 300), (100, 200), (400, 500)],
        test_keys[0], sheriff_key, bug_id=123, internal_only=True)
    self._AddAnomalyEntities(
        [(150, 250)], test_keys[0], sheriff_key, bug_id=123)
    response = self.testapp.post('/api/alerts/bug_id/123')
    anomalies = self.GetJsonValue(response, 'anomalies')
    self.assertEqual(1, len(anomalies))

  @mock.patch.object(api_auth, 'oauth')
  def testPost_WithInvalidBugIdParameter_ShowsError(self, mock_oauth):
    self._SetGooglerOAuth(mock_oauth)
    response = self.testapp.post('/api/alerts/bug_id/foo', status=400)
    self.assertEqual(
        {'error': 'Invalid bug ID "foo".'}, json.loads(response.body))

  @mock.patch.object(api_auth, 'oauth')
  def testPost_WithHistoryParameter_ListsAlerts(self, mock_oauth):
    self._SetGooglerOAuth(mock_oauth)
    sheriff_key = self._AddSheriff()
    test_keys = self._AddTests()
    recent_ranges = [(300, 500), (500, 600), (600, 800)]
    old_ranges = [(100, 200)]
    old_time = datetime.datetime.now() - datetime.timedelta(days=6)
    self._AddAnomalyEntities(recent_ranges, test_keys[0], sheriff_key)
    self._AddAnomalyEntities(
        old_ranges, test_keys[0], sheriff_key, timestamp=old_time)

    response = self.testapp.post(
        '/api/alerts/history/5')
    anomalies = self.GetJsonValue(response, 'anomalies')
    self.assertEqual(3, len(anomalies))

  @mock.patch.object(api_auth, 'oauth')
  def testPost_WithBenchmarkParameter_ListsOnlyMatching(self, mock_oauth):
    self._SetGooglerOAuth(mock_oauth)
    sheriff_key = self._AddSheriff()
    test_keys = [
        utils.TestKey('ChromiumPerf/bot/sunspider/Total'),
        utils.TestKey('ChromiumPerf/bot/octane/Total')]
    ranges = [(300, 500), (500, 600), (600, 800)]
    for key in test_keys:
      self._AddAnomalyEntities(ranges, key, sheriff_key)

    response = self.testapp.post(
        '/api/alerts/history/5', {'benchmark': 'octane'})
    anomalies = self.GetJsonValue(response, 'anomalies')
    self.assertEqual(3, len(anomalies))



if __name__ == '__main__':
  unittest.main()
