# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import mock
import webapp2
import webtest

from google.appengine.ext import ndb

from dashboard import mark_recovered_alerts
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import bug_data
from dashboard.models import sheriff
from dashboard.services import issue_tracker_service


@mock.patch('apiclient.discovery.build', mock.MagicMock())
@mock.patch.object(utils, 'ServiceAccountHttp', mock.MagicMock())
@mock.patch.object(utils, 'TickMonitoringCustomMetric', mock.MagicMock())
class MarkRecoveredAlertsTest(testing_common.TestCase):

  def setUp(self):
    super(MarkRecoveredAlertsTest, self).setUp()
    app = webapp2.WSGIApplication(
        [('/mark_recovered_alerts',
          mark_recovered_alerts.MarkRecoveredAlertsHandler)])
    self.testapp = webtest.TestApp(app)

  def _AddTestData(self, series, sheriff_key,
                   improvement_direction=anomaly.UP):
    """Adds one sample TestMetadata and associated data.

    Args:
      series: Either a list of values, or a list of (x, y) pairs.
      sheriff_key: A Sheriff entity key.
      improvement_direction: One of {anomaly.UP, anomaly.DOWN, anomaly.UNKNOWN}.

    Returns:
      The key of the TestMetadata entity that was added.
    """
    testing_common.AddTests(['M'], ['b'], {'benchmark': {'t': {}}})
    test_path = 'M/b/benchmark/t'
    test = utils.TestKey(test_path).get()
    test.improvement_direction = improvement_direction
    test.sheriff = sheriff_key
    sheriff_entity = sheriff_key.get()
    sheriff_entity.patterns.append(test.test_path)
    sheriff_entity.put()
    if series and isinstance(series[0], (int, float)):
      series = enumerate(series, start=1)
    testing_common.AddRows(test_path, {x: {'value': y} for x, y in series})
    return test.put()

  def _AddAnomalyForTest(self, sheriff_key, test_key, revision,
                         median_before, median_after, bug_id=None):
    """Adds a sample Anomaly and returns the key."""
    if bug_id > 0:
      bug = ndb.Key('Bug', bug_id).get()
      if not bug:
        bug_data.Bug(id=bug_id).put()
    return anomaly.Anomaly(
        start_revision=revision,
        end_revision=revision,
        test=test_key,
        median_before_anomaly=median_before,
        median_after_anomaly=median_after,
        bug_id=bug_id,
        sheriff=sheriff_key).put()

  def testPost_Recovered_MarkedAsRecovered(self):
    sheriff_key = sheriff.Sheriff(email='a@google.com', id='sheriff_key').put()
    values = [
        49, 50, 51, 50, 51, 49, 51, 50, 50, 49,
        55, 54, 55, 56, 54, 56, 57, 56, 55, 56,
        49, 50, 51, 50, 51, 49, 51, 50, 50, 49,
    ]
    test_key = self._AddTestData(values, sheriff_key)
    anomaly_key = self._AddAnomalyForTest(
        sheriff_key, test_key, revision=11, median_before=50, median_after=55)
    self.testapp.post('/mark_recovered_alerts')
    self.ExecuteTaskQueueTasks(
        '/mark_recovered_alerts', mark_recovered_alerts._TASK_QUEUE_NAME)
    self.assertTrue(anomaly_key.get().recovered)

  def testPost_NotRecovered_NotMarkedAsRecovered(self):
    sheriff_key = sheriff.Sheriff(email='a@google.com', id='sheriff_key').put()
    values = [
        49, 50, 51, 50, 51, 49, 51, 50, 50, 49,
        55, 54, 55, 56, 54, 56, 57, 56, 55, 56,
        55, 54, 55, 56, 54, 56, 57, 56, 55, 56,
    ]
    test_key = self._AddTestData(values, sheriff_key)
    anomaly_key = self._AddAnomalyForTest(
        sheriff_key, test_key, revision=11, median_before=50, median_after=55)
    self.testapp.post('/mark_recovered_alerts')
    self.ExecuteTaskQueueTasks(
        '/mark_recovered_alerts', mark_recovered_alerts._TASK_QUEUE_NAME)
    self.assertFalse(anomaly_key.get().recovered)

  def testPost_ChangeTooLarge_NotMarkedAsRecovered(self):
    sheriff_key = sheriff.Sheriff(email='a@google.com', id='sheriff_key').put()
    values = [
        49, 50, 51, 50, 51, 49, 51, 50, 50, 49,
        55, 54, 55, 56, 54, 56, 57, 56, 55, 56,
        30, 29, 32, 34, 30, 31, 31, 32, 33, 30,
    ]
    test_key = self._AddTestData(values, sheriff_key)
    anomaly_key = self._AddAnomalyForTest(
        sheriff_key, test_key, revision=11, median_before=50, median_after=55)
    self.testapp.post('/mark_recovered_alerts')
    self.ExecuteTaskQueueTasks(
        '/mark_recovered_alerts', mark_recovered_alerts._TASK_QUEUE_NAME)
    self.assertFalse(anomaly_key.get().recovered)

  def testPost_ChangeWrongDirection_NotMarkedAsRecovered(self):
    sheriff_key = sheriff.Sheriff(email='a@google.com', id='sheriff_key').put()
    values = [
        49, 50, 51, 50, 51, 49, 51, 50, 50, 49,
        55, 54, 55, 56, 54, 56, 57, 56, 55, 56,
        59, 60, 61, 60, 61, 59, 61, 60, 60, 59,
    ]
    test_key = self._AddTestData(values, sheriff_key)
    anomaly_key = self._AddAnomalyForTest(
        sheriff_key, test_key, revision=11, median_before=50, median_after=55)
    self.testapp.post('/mark_recovered_alerts')
    self.ExecuteTaskQueueTasks(
        '/mark_recovered_alerts', mark_recovered_alerts._TASK_QUEUE_NAME)
    self.assertFalse(anomaly_key.get().recovered)

  def testPost_AlertInvalid_NotMarkedAsRecovered(self):
    sheriff_key = sheriff.Sheriff(email='a@google.com', id='sheriff_key').put()
    values = [
        49, 50, 51, 50, 51, 49, 51, 50, 50, 49,
        55, 54, 55, 56, 54, 56, 57, 56, 55, 56,
        49, 50, 51, 50, 51, 49, 51, 50, 50, 49,
    ]
    test_key = self._AddTestData(values, sheriff_key)
    anomaly_key = self._AddAnomalyForTest(
        sheriff_key, test_key, revision=11, median_before=50, median_after=55,
        bug_id=-1)
    self.testapp.post('/mark_recovered_alerts')
    self.ExecuteTaskQueueTasks(
        '/mark_recovered_alerts', mark_recovered_alerts._TASK_QUEUE_NAME)
    self.assertFalse(anomaly_key.get().recovered)

  @mock.patch.object(
      issue_tracker_service.IssueTrackerService, 'AddBugComment')
  @mock.patch.object(
      issue_tracker_service.IssueTrackerService,
      'List',
      return_value={'items': [{'id': 1234}]})
  def testPost_AllAnomaliesRecovered_AddsComment(
      self, _, add_bug_comment_mock):
    sheriff_key = sheriff.Sheriff(email='a@google.com', id='sheriff_key').put()
    values = [
        49, 50, 51, 50, 51, 49, 51, 50, 50, 49,
        55, 54, 55, 56, 54, 56, 57, 56, 55, 56,
        49, 50, 51, 50, 51, 49, 51, 50, 50, 49,
    ]
    test_key = self._AddTestData(values, sheriff_key)
    anomaly_key = self._AddAnomalyForTest(
        sheriff_key, test_key, revision=11, median_before=50, median_after=55,
        bug_id=1234)
    self.testapp.post('/mark_recovered_alerts')
    self.ExecuteTaskQueueTasks(
        '/mark_recovered_alerts', mark_recovered_alerts._TASK_QUEUE_NAME)
    self.assertTrue(anomaly_key.get().recovered)
    add_bug_comment_mock.assert_called_once_with(
        mock.ANY, mock.ANY, labels='Performance-Regression-Recovered')

  @mock.patch.object(
      issue_tracker_service.IssueTrackerService, 'AddBugComment')
  @mock.patch.object(
      issue_tracker_service.IssueTrackerService,
      'List',
      return_value={'items': [{'id': 1234}]})
  def testPost_BugHasNoAlerts_NoCommentPosted(
      self, _, add_bug_comment_mock):
    self.testapp.post('/mark_recovered_alerts')
    self.ExecuteTaskQueueTasks(
        '/mark_recovered_alerts', mark_recovered_alerts._TASK_QUEUE_NAME)
    self.assertFalse(add_bug_comment_mock.called)

  def testPost_RealWorldExample_NoClearRecovery(self):
    # This test is based on a real-world case on a relatively noisy graph where
    # after the step up at r362262 the results meandered down again with no
    # clear step. Alert key agxzfmNocm9tZXBlcmZyFAsSB0Fub21hbHkYgIDAnYnIqAoM.
    sheriff_key = sheriff.Sheriff(email='a@google.com', id='sheriff_key').put()
    series = [
        (362080, 1562.6), (362086, 1641.4), (362095, 1572.4), (362102, 1552.9),
        (362104, 1579.9), (362114, 1564.6), (362118, 1570.5), (362122, 1555.7),
        (362129, 1550.1), (362134, 1547.5), (362149, 1536.2), (362186, 1533.1),
        (362224, 1542.0), (362262, 1658.9), (362276, 1675.6), (362305, 1630.8),
        (362321, 1664.7), (362345, 1659.6), (362361, 1669.4), (362366, 1681.6),
        (362367, 1601.3), (362369, 1664.7), (362401, 1648.4), (362402, 1595.2),
        (362417, 1676.9), (362445, 1532.0), (362470, 1631.9), (362490, 1585.1),
        (362500, 1674.3), (362543, 1639.7), (362565, 1670.7), (362611, 1594.7),
        (362635, 1677.5), (362638, 1687.5), (362650, 1702.2), (362663, 1614.9),
        (362676, 1650.1), (362686, 1724.0), (362687, 1594.8), (362700, 1633.7),
        (362721, 1684.1), (362744, 1678.7), (362776, 1642.4), (362899, 1591.1),
        (362915, 1639.1), (362925, 1633.7), (362935, 1539.6), (362937, 1572.0),
        (362950, 1567.4), (362963, 1608.3)
    ]
    test_key = self._AddTestData(
        series, sheriff_key, improvement_direction=anomaly.DOWN)
    anomaly_key = self._AddAnomalyForTest(
        sheriff_key, test_key, revision=362262,
        median_before=1579.2, median_after=1680.7)
    self.testapp.post('/mark_recovered_alerts')
    self.ExecuteTaskQueueTasks(
        '/mark_recovered_alerts', mark_recovered_alerts._TASK_QUEUE_NAME)
    self.assertFalse(anomaly_key.get().recovered)

  def testPost_RealisticExample_Recovered(self):
    # This test is based on a real-world case where there was a step up at
    # r362399, and shortly thereafter a step down at r362680 of roughly similar
    # magnitude. Alert key agxzfmNocm9tZXBlcmZyFAsSB0Fub21hbHkYgIDAnbimogoM
    sheriff_key = sheriff.Sheriff(email='a@google.com', id='sheriff_key').put()
    series = [
        (361776, 78260720), (361807, 78760907), (361837, 77723737),
        (361864, 77984606), (361869, 78660955), (361879, 78276998),
        (361903, 77420262), (362399, 79629598), (362416, 79631028),
        (362428, 79074016), (362445, 79348860), (362483, 79724728),
        (362532, 79673772), (362623, 79120915), (362641, 79384809),
        (362666, 79885480), (362680, 78308585), (362701, 78063846),
        (362730, 78244836), (362759, 77375408), (362799, 77836310),
        (362908, 78069878), (362936, 77191699), (362958, 77951200),
        (362975, 77906097)
    ]
    test_key = self._AddTestData(
        series, sheriff_key, improvement_direction=anomaly.DOWN)
    anomaly_key = self._AddAnomalyForTest(
        sheriff_key, test_key, revision=362399,
        median_before=78275468.8, median_after=79630313.6)
    self.testapp.post('/mark_recovered_alerts')
    self.ExecuteTaskQueueTasks(
        '/mark_recovered_alerts', mark_recovered_alerts._TASK_QUEUE_NAME)
    self.assertTrue(anomaly_key.get().recovered)


if __name__ == '__main__':
  unittest.main()
