# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import mock
import webapp2
import webtest

from dashboard import debug_alert
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import anomaly_config
from dashboard.models import graph_data

_SAMPLE_SERIES = [
    (300, 60.06), (301, 60.36), (302, 61.76), (303, 60.06), (304, 61.24),
    (305, 60.65), (306, 55.61), (307, 61.88), (308, 61.51), (309, 59.58),
    (310, 71.79), (311, 71.97), (312, 71.63), (313, 67.16), (314, 70.91),
    (315, 73.40), (316, 71.00), (317, 69.45), (318, 67.16), (319, 66.05),
]


class DebugAlertTest(testing_common.TestCase):

  def setUp(self):
    super(DebugAlertTest, self).setUp()
    app = webapp2.WSGIApplication(
        [('/debug_alert', debug_alert.DebugAlertHandler)])
    self.testapp = webtest.TestApp(app)
    self.PatchDatastoreHooksRequest()

  def _AddSampleData(self):
    """Adds TestMetadata and Row entities, and returns the TestMetadata key."""
    testing_common.AddTests(['M'], ['b'], {'suite': {'foo': {}}})
    test_path = 'M/b/suite/foo'
    rows_dict = {x: {'value': y} for x, y in _SAMPLE_SERIES}
    testing_common.AddRows(test_path, rows_dict)
    return utils.TestKey(test_path)

  def testGet_WithInvalidTestPath_ShowsFormAndError(self):
    response = self.testapp.get('/debug_alert?test_path=foo')
    self.assertIn('<form', response.body)
    self.assertIn('class="error"', response.body)

  def testGet_WithValidTestPath_ShowsChart(self):
    test_key = self._AddSampleData()
    test_path = utils.TestPath(test_key)
    response = self.testapp.get('/debug_alert?test_path=%s' % test_path)
    self.assertIn('id="plot"', response.body)

  def testPost_SameAsGet(self):
    # Post is the same as get for this endpoint.
    test_key = self._AddSampleData()
    test_path = utils.TestPath(test_key)
    get_response = self.testapp.get('/debug_alert?test_path=%s' % test_path)
    post_response = self.testapp.post('/debug_alert?test_path=%s' % test_path)
    self.assertEqual(get_response.body, post_response.body)

  def testGet_WithNoParameters_ShowsForm(self):
    response = self.testapp.get('/debug_alert')
    self.assertIn('<form', response.body)
    self.assertNotIn('id="plot"', response.body)

  def testGet_WithRevParameter_EmbedsCorrectRevisions(self):
    test_key = self._AddSampleData()
    test_path = utils.TestPath(test_key)
    response = self.testapp.get(
        '/debug_alert?test_path=%s&rev=%s&num_before=%s&num_after=%s' %
        (test_path, 305, 10, 5))
    self.assertEqual(
        [300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310],
        self.GetEmbeddedVariable(response, 'LOOKUP'))

  def testGet_InvalidNumBeforeParameter_ShowsFormAndError(self):
    test_key = self._AddSampleData()
    test_path = utils.TestPath(test_key)
    response = self.testapp.get(
        '/debug_alert?test_path=%s&rev=%s&num_before=%s&num_after=%s' %
        (test_path, 305, 'foo', 5))
    self.assertIn('<form', response.body)
    self.assertIn('class="error"', response.body)
    self.assertNotIn('LOOKUP', response.body)

  def _AddAnomalyConfig(self, config_name, test_key, config_dict):
    """Adds a custom anomaly config which applies to one test."""
    anomaly_config_key = anomaly_config.AnomalyConfig(
        id=config_name,
        config=config_dict,
        patterns=[utils.TestPath(test_key)]).put()
    return anomaly_config_key

  @mock.patch.object(debug_alert, 'SimulateAlertProcessing')
  def testGet_TestHasOverriddenConfig_ConfigUsed(self, simulate_mock):
    test_key = self._AddSampleData()
    # Add a config which applies to the test. The test is updated upon put.
    self._AddAnomalyConfig('X', test_key, {'min_absolute_change': 10})
    test_key.get().put()
    response = self.testapp.get(
        '/debug_alert?test_path=%s' % utils.TestPath(test_key))
    # The custom config should be used when simulating alert processing.
    simulate_mock.assert_called_once_with(mock.ANY, min_absolute_change=10)
    # The config JSON should also be put into the form on the page.
    self.assertIn('"min_absolute_change": 10', response.body)

  @mock.patch.object(debug_alert, 'SimulateAlertProcessing')
  def testGet_WithValidCustomConfig_ConfigUsed(self, simulate_mock):
    test_key = self._AddSampleData()
    response = self.testapp.get(
        '/debug_alert?test_path=%s&config=%s' %
        (utils.TestPath(test_key),
         '{"min_relative_change":0.75}'))
    # The custom config should be used when simulating alert processing.
    simulate_mock.assert_called_once_with(mock.ANY, min_relative_change=0.75)
    # The config JSON should also be put into the form on the page.
    self.assertIn('"min_relative_change": 0.75', response.body)

  @mock.patch.object(debug_alert, 'SimulateAlertProcessing')
  def testGet_WithBogusParameterNames_ParameterIgnored(self, simulate_mock):
    test_key = self._AddSampleData()
    response = self.testapp.get(
        '/debug_alert?test_path=%s&config=%s' %
        (utils.TestPath(test_key), '{"foo":0.75}'))
    simulate_mock.assert_called_once_with(mock.ANY)
    self.assertNotIn('"foo"', response.body)

  def testGet_WithInvalidCustomConfig_ErrorShown(self):
    test_key = self._AddSampleData()
    response = self.testapp.get(
        '/debug_alert?test_path=%s&config=%s' %
        (utils.TestPath(test_key), 'not valid json'))
    # The error message should be on the page; JS constants should not be.
    self.assertIn('Invalid JSON', response.body)
    self.assertNotIn('LOOKUP', response.body)

  def testGet_WithStoredAnomalies_ShowsStoredAnomalies(self):
    test_key = self._AddSampleData()
    anomaly.Anomaly(
        test=test_key, start_revision=309, end_revision=310,
        median_before_anomaly=60, median_after_anomaly=70,
        bug_id=12345).put()
    response = self.testapp.get(
        '/debug_alert?test_path=%s' % utils.TestPath(test_key))
    # Information about the stored anomaly should be somewhere on the page.
    self.assertIn('12345', response.body)

  def testFetchLatestRows(self):
    test_key = self._AddSampleData()
    rows = debug_alert._FetchLatestRows(test_key.get(), 4)
    revisions = [r.revision for r in rows]
    self.assertEqual([316, 317, 318, 319], revisions)

  def testFetchAroundRev(self):
    test_key = self._AddSampleData()
    rows = debug_alert._FetchRowsAroundRev(test_key.get(), 310, 5, 8)
    revisions = [r.revision for r in rows]
    self.assertEqual(
        [306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318],
        revisions)

  def testFetchRowsAroundRev_NotAllRowsAvailable(self):
    test_key = self._AddSampleData()
    rows = debug_alert._FetchRowsAroundRev(test_key.get(), 310, 100, 100)
    # There are only 20 rows in the sample data, so only 20 can be fetched.
    self.assertEqual(20, len(rows))

  def testChartSeries(self):
    test_key = self._AddSampleData()
    rows = debug_alert._FetchRowsAroundRev(test_key.get(), 310, 5, 5)
    # The indexes used in the chart series should match those in the lookup.
    self.assertEqual(
        [(0, 55.61), (1, 61.88), (2, 61.51), (3, 59.58), (4, 71.79),
         (5, 71.97), (6, 71.63), (7, 67.16), (8, 70.91), (9, 73.4)],
        debug_alert._ChartSeries(rows))

  def testRevisionList(self):
    test_key = self._AddSampleData()
    rows = debug_alert._FetchRowsAroundRev(test_key.get(), 310, 5, 5)
    # The lookup dict maps indexes to x-values in the input series.
    self.assertEqual(
        [306, 307, 308, 309, 310, 311, 312, 313, 314, 315],
        debug_alert._RevisionList(rows))

  def testCsvUrl_RowsGiven_AllParamsSpecified(self):
    self._AddSampleData()
    rows = graph_data.Row.query().fetch(limit=20)
    self.assertEqual(
        '/graph_csv?test_path=M%2Fb%2Fsuite%2Ffoo&num_points=20&rev=319',
        debug_alert._CsvUrl('M/b/suite/foo', rows))

  def testCsvUrl_NoRows_OnlyTestPathSpecified(self):
    # If there are no rows available for some reason, a CSV download
    # URL can still be constructed, but without specific revisions.
    self.assertEqual(
        '/graph_csv?test_path=M%2Fb%2Fsuite%2Ffoo',
        debug_alert._CsvUrl('M/b/suite/foo', []))

  def testGraphUrl_RevisionGiven_RevisionParamInUrl(self):
    test_key = self._AddSampleData()
    # Both string and int can be accepted for revision.
    self.assertEqual(
        '/report?masters=M&bots=b&tests=suite%2Ffoo&rev=310',
        debug_alert._GraphUrl(test_key.get(), 310))
    self.assertEqual(
        '/report?masters=M&bots=b&tests=suite%2Ffoo&rev=310',
        debug_alert._GraphUrl(test_key.get(), '310'))

  def testGraphUrl_NoRevisionGiven_NoRevisionParamInUrl(self):
    test_key = self._AddSampleData()
    # Both None and empty string mean "no revision".
    self.assertEqual(
        '/report?masters=M&bots=b&tests=suite%2Ffoo',
        debug_alert._GraphUrl(test_key.get(), ''))
    self.assertEqual(
        '/report?masters=M&bots=b&tests=suite%2Ffoo',
        debug_alert._GraphUrl(test_key.get(), None))


if __name__ == '__main__':
  unittest.main()
