# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import sys
import unittest

import mock

from dashboard import find_anomalies
from dashboard import find_change_points
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import graph_data
from dashboard.models import histogram
from dashboard.models import sheriff
from tracing.value.diagnostics import reserved_infos

# Sample time series.
_TEST_ROW_DATA = [
    (241105, 2136.7), (241116, 2140.3), (241151, 2149.1),
    (241154, 2147.2), (241156, 2130.6), (241160, 2136.2),
    (241188, 2146.7), (241201, 2141.8), (241226, 2140.6),
    (241247, 2128.1), (241249, 2134.2), (241254, 2130.0),
    (241262, 2136.0), (241268, 2142.6), (241271, 2149.1),
    (241282, 2156.6), (241294, 2125.3), (241298, 2155.5),
    (241303, 2148.5), (241317, 2146.2), (241323, 2123.3),
    (241330, 2121.5), (241342, 2141.2), (241355, 2145.2),
    (241371, 2136.3), (241386, 2144.0), (241405, 2138.1),
    (241420, 2147.6), (241432, 2140.7), (241441, 2132.2),
    (241452, 2138.2), (241455, 2139.3), (241471, 2134.0),
    (241488, 2137.2), (241503, 2152.5), (241520, 2136.3),
    (241524, 2139.3), (241529, 2143.5), (241532, 2145.5),
    (241535, 2147.0), (241537, 2184.1), (241546, 2180.8),
    (241553, 2181.5), (241559, 2176.8), (241566, 2174.0),
    (241577, 2182.8), (241579, 2184.8), (241582, 2190.5),
    (241584, 2183.1), (241609, 2178.3), (241620, 2178.1),
    (241645, 2190.8), (241653, 2177.7), (241666, 2185.3),
    (241697, 2173.8), (241716, 2172.1), (241735, 2172.5),
    (241757, 2174.7), (241766, 2196.7), (241782, 2184.1),
]


def _MakeSampleChangePoint(x_value, median_before, median_after):
  """Makes a sample find_change_points.ChangePoint for use in these tests."""
  # The only thing that matters in these tests is the revision number
  # and the values before and after.
  return find_change_points.ChangePoint(
      x_value=x_value,
      median_before=median_before,
      median_after=median_after,
      window_start=1,
      window_end=8,
      size_before=None,
      size_after=None,
      relative_change=None,
      std_dev_before=None,
      t_statistic=None,
      degrees_of_freedom=None,
      p_value=None)


class EndRevisionMatcher(object):
  """Custom matcher to test if an anomaly matches a given end rev."""

  def __init__(self, end_revision):
    """Initializes with the end time to check."""
    self._end_revision = end_revision

  def __eq__(self, rhs):
    """Checks to see if RHS has the same end time."""
    return self._end_revision == rhs.end_revision

  def __repr__(self):
    """Shows a readable revision which can be printed when assert fails."""
    return '<IsEndRevision %d>' % self._end_revision


class ModelMatcher(object):
  """Custom matcher to check if two ndb entity names match."""

  def __init__(self, name):
    """Initializes with the name of the entity."""
    self._name = name

  def __eq__(self, rhs):
    """Checks to see if RHS has the same name."""
    return rhs.key.string_id() == self._name

  def __repr__(self):
    """Shows a readable revision which can be printed when assert fails."""
    return '<IsModel %s>' % self._name


class ProcessAlertsTest(testing_common.TestCase):

  def setUp(self):
    super(ProcessAlertsTest, self).setUp()
    self.SetCurrentUser('foo@bar.com', is_admin=True)

  def _AddDataForTests(self):
    testing_common.AddTests(
        ['ChromiumGPU'],
        ['linux-release'], {
            'scrolling_benchmark': {
                'ref': {},
            },
        })
    ref = utils.TestKey(
        'ChromiumGPU/linux-release/scrolling_benchmark/ref').get()
    ref.units = 'ms'
    for i in range(9000, 10070, 5):
      # Internal-only data should be found.
      test_container_key = utils.GetTestContainerKey(ref.key)
      graph_data.Row(
          id=i + 1, value=float(i * 3),
          parent=test_container_key, internal_only=True).put()

  @mock.patch.object(
      find_anomalies.find_change_points, 'FindChangePoints',
      mock.MagicMock(return_value=[
          _MakeSampleChangePoint(10011, 50, 100),
          _MakeSampleChangePoint(10041, 200, 100),
          _MakeSampleChangePoint(10061, 0, 100),
      ]))
  @mock.patch.object(find_anomalies.email_sheriff, 'EmailSheriff')
  def testProcessTest(self, mock_email_sheriff):
    self._AddDataForTests()
    test_path = 'ChromiumGPU/linux-release/scrolling_benchmark/ref'
    test = utils.TestKey(test_path).get()
    sheriff.Sheriff(
        email='a@google.com', id='sheriff', patterns=[test_path]).put()
    test.put()

    find_anomalies.ProcessTests([test.key])

    expected_calls = [
        mock.call(ModelMatcher('sheriff'),
                  ModelMatcher(
                      'ChromiumGPU/linux-release/scrolling_benchmark/ref'),
                  EndRevisionMatcher(10011)),
        mock.call(ModelMatcher('sheriff'),
                  ModelMatcher(
                      'ChromiumGPU/linux-release/scrolling_benchmark/ref'),
                  EndRevisionMatcher(10041)),
        mock.call(ModelMatcher('sheriff'),
                  ModelMatcher(
                      'ChromiumGPU/linux-release/scrolling_benchmark/ref'),
                  EndRevisionMatcher(10061))]
    self.assertEqual(expected_calls, mock_email_sheriff.call_args_list)

    anomalies = anomaly.Anomaly.query().fetch()
    self.assertEqual(len(anomalies), 3)

    def AnomalyExists(
        anomalies, test, percent_changed, direction,
        start_revision, end_revision, sheriff_name, internal_only, units,
        absolute_delta):
      for a in anomalies:
        if (a.test == test and
            a.percent_changed == percent_changed and
            a.direction == direction and
            a.start_revision == start_revision and
            a.end_revision == end_revision and
            a.sheriff.string_id() == sheriff_name and
            a.internal_only == internal_only and
            a.units == units and
            a.absolute_delta == absolute_delta):
          return True
      return False

    self.assertTrue(
        AnomalyExists(
            anomalies, test.key, percent_changed=100, direction=anomaly.UP,
            start_revision=10007, end_revision=10011, sheriff_name='sheriff',
            internal_only=False, units='ms', absolute_delta=50))

    self.assertTrue(
        AnomalyExists(
            anomalies, test.key, percent_changed=-50, direction=anomaly.DOWN,
            start_revision=10037, end_revision=10041, sheriff_name='sheriff',
            internal_only=False, units='ms', absolute_delta=-100))

    self.assertTrue(
        AnomalyExists(
            anomalies, test.key, percent_changed=sys.float_info.max,
            direction=anomaly.UP, start_revision=10057, end_revision=10061,
            sheriff_name='sheriff', internal_only=False, units='ms',
            absolute_delta=100))

    # This is here just to verify that AnomalyExists returns False sometimes.
    self.assertFalse(
        AnomalyExists(
            anomalies, test.key, percent_changed=100, direction=anomaly.DOWN,
            start_revision=10037, end_revision=10041, sheriff_name='sheriff',
            internal_only=False, units='ms', absolute_delta=500))

  @mock.patch.object(
      find_anomalies.find_change_points, 'FindChangePoints',
      mock.MagicMock(return_value=[
          _MakeSampleChangePoint(10011, 100, 50)
      ]))
  def testProcessTest_ImprovementMarkedAsImprovement(self):
    self._AddDataForTests()
    test = utils.TestKey(
        'ChromiumGPU/linux-release/scrolling_benchmark/ref').get()
    sheriff.Sheriff(
        email='a@google.com', id='sheriff', patterns=[test.test_path]).put()
    test.improvement_direction = anomaly.DOWN
    test.put()
    find_anomalies.ProcessTests([test.key])
    anomalies = anomaly.Anomaly.query().fetch()
    self.assertEqual(len(anomalies), 1)
    self.assertTrue(anomalies[0].is_improvement)

  @mock.patch('logging.error')
  def testProcessTest_NoSheriff_ErrorLogged(self, mock_logging_error):
    self._AddDataForTests()
    ref = utils.TestKey(
        'ChromiumGPU/linux-release/scrolling_benchmark/ref').get()
    find_anomalies.ProcessTests([ref.key])
    mock_logging_error.assert_called_with('No sheriff for %s', ref.key)

  @mock.patch.object(
      find_anomalies.find_change_points, 'FindChangePoints',
      mock.MagicMock(return_value=[
          _MakeSampleChangePoint(10026, 55.2, 57.8),
          _MakeSampleChangePoint(10041, 45.2, 37.8),
      ]))
  @mock.patch.object(find_anomalies.email_sheriff, 'EmailSheriff')
  def testProcessTest_FiltersOutImprovements(self, mock_email_sheriff):
    self._AddDataForTests()
    test = utils.TestKey(
        'ChromiumGPU/linux-release/scrolling_benchmark/ref').get()
    sheriff.Sheriff(
        email='a@google.com', id='sheriff', patterns=[test.test_path]).put()
    test.improvement_direction = anomaly.UP
    test.put()
    find_anomalies.ProcessTests([test.key])
    mock_email_sheriff.assert_called_once_with(
        ModelMatcher('sheriff'),
        ModelMatcher('ChromiumGPU/linux-release/scrolling_benchmark/ref'),
        EndRevisionMatcher(10041))

  @mock.patch.object(
      find_anomalies.find_change_points, 'FindChangePoints',
      mock.MagicMock(return_value=[
          _MakeSampleChangePoint(10011, 50, 100),
      ]))
  @mock.patch.object(find_anomalies.email_sheriff, 'EmailSheriff')
  def testProcessTest_InternalOnlyTest(self, mock_email_sheriff):
    self._AddDataForTests()
    test = utils.TestKey(
        'ChromiumGPU/linux-release/scrolling_benchmark/ref').get()
    test.internal_only = True
    sheriff.Sheriff(
        email='a@google.com', id='sheriff', patterns=[test.test_path]).put()
    test.put()

    find_anomalies.ProcessTests([test.key])
    expected_calls = [
        mock.call(ModelMatcher('sheriff'),
                  ModelMatcher(
                      'ChromiumGPU/linux-release/scrolling_benchmark/ref'),
                  EndRevisionMatcher(10011))]
    self.assertEqual(expected_calls, mock_email_sheriff.call_args_list)

    anomalies = anomaly.Anomaly.query().fetch()
    self.assertEqual(len(anomalies), 1)
    self.assertEqual(test.key, anomalies[0].test)
    self.assertEqual(100, anomalies[0].percent_changed)
    self.assertEqual(anomaly.UP, anomalies[0].direction)
    self.assertEqual(10007, anomalies[0].start_revision)
    self.assertEqual(10011, anomalies[0].end_revision)
    self.assertTrue(anomalies[0].internal_only)

  def testProcessTest_CreatesAnAnomaly_RefMovesToo_BenchmarkDuration(self):
    testing_common.AddTests(
        ['ChromiumGPU'], ['linux-release'], {
            'foo': {'benchmark_duration': {'ref': {}}},
        })
    ref = utils.TestKey(
        'ChromiumGPU/linux-release/foo/benchmark_duration/ref').get()
    non_ref = utils.TestKey(
        'ChromiumGPU/linux-release/foo/benchmark_duration').get()
    test_container_key = utils.GetTestContainerKey(ref.key)
    test_container_key_non_ref = utils.GetTestContainerKey(non_ref.key)
    for row in _TEST_ROW_DATA:
      graph_data.Row(id=row[0], value=row[1], parent=test_container_key).put()
      graph_data.Row(id=row[0], value=row[1],
                     parent=test_container_key_non_ref).put()
    sheriff.Sheriff(
        email='a@google.com', id='sheriff', patterns=[ref.test_path]).put()
    ref.put()
    find_anomalies.ProcessTests([ref.key])
    new_anomalies = anomaly.Anomaly.query().fetch()
    self.assertEqual(1, len(new_anomalies))

  def testProcessTest_AnomaliesMatchRefSeries_NoAlertCreated(self):
    # Tests that a Anomaly entity is not created if both the test and its
    # corresponding ref build series have the same data.
    testing_common.AddTests(
        ['ChromiumGPU'], ['linux-release'], {
            'scrolling_benchmark': {'ref': {}},
        })
    ref = utils.TestKey(
        'ChromiumGPU/linux-release/scrolling_benchmark/ref').get()
    non_ref = utils.TestKey(
        'ChromiumGPU/linux-release/scrolling_benchmark').get()
    test_container_key = utils.GetTestContainerKey(ref.key)
    test_container_key_non_ref = utils.GetTestContainerKey(non_ref.key)
    for row in _TEST_ROW_DATA:
      graph_data.Row(id=row[0], value=row[1], parent=test_container_key).put()
      graph_data.Row(id=row[0], value=row[1],
                     parent=test_container_key_non_ref).put()
    sheriff.Sheriff(
        email='a@google.com', id='sheriff', patterns=[non_ref.test_path]).put()
    ref.put()
    non_ref.put()
    find_anomalies.ProcessTests([non_ref.key])
    new_anomalies = anomaly.Anomaly.query().fetch()
    self.assertEqual(0, len(new_anomalies))

  def testProcessTest_AnomalyDoesNotMatchRefSeries_AlertCreated(self):
    # Tests that an Anomaly entity is created when non-ref series goes up, but
    # the ref series stays flat.
    testing_common.AddTests(
        ['ChromiumGPU'], ['linux-release'], {
            'scrolling_benchmark': {'ref': {}},
        })
    ref = utils.TestKey(
        'ChromiumGPU/linux-release/scrolling_benchmark/ref').get()
    non_ref = utils.TestKey(
        'ChromiumGPU/linux-release/scrolling_benchmark').get()
    test_container_key = utils.GetTestContainerKey(ref.key)
    test_container_key_non_ref = utils.GetTestContainerKey(non_ref.key)
    for row in _TEST_ROW_DATA:
      graph_data.Row(id=row[0], value=2125.375, parent=test_container_key).put()
      graph_data.Row(id=row[0], value=row[1],
                     parent=test_container_key_non_ref).put()
    sheriff.Sheriff(
        email='a@google.com', id='sheriff', patterns=[ref.test_path]).put()
    sheriff.Sheriff(
        email='a@google.com', id='sheriff', patterns=[non_ref.test_path]).put()
    ref.put()
    non_ref.put()
    find_anomalies.ProcessTests([non_ref.key])
    new_anomalies = anomaly.Anomaly.query().fetch()
    self.assertEqual(len(new_anomalies), 1)

  def testProcessTest_CreatesAnAnomaly(self):
    testing_common.AddTests(
        ['ChromiumGPU'], ['linux-release'], {
            'scrolling_benchmark': {'ref': {}},
        })
    ref = utils.TestKey(
        'ChromiumGPU/linux-release/scrolling_benchmark/ref').get()
    test_container_key = utils.GetTestContainerKey(ref.key)
    for row in _TEST_ROW_DATA:
      graph_data.Row(id=row[0], value=row[1], parent=test_container_key).put()
    sheriff.Sheriff(
        email='a@google.com', id='sheriff', patterns=[ref.test_path]).put()
    ref.put()
    find_anomalies.ProcessTests([ref.key])
    new_anomalies = anomaly.Anomaly.query().fetch()
    self.assertEqual(1, len(new_anomalies))
    self.assertEqual(anomaly.UP, new_anomalies[0].direction)
    self.assertEqual(241536, new_anomalies[0].start_revision)
    self.assertEqual(241537, new_anomalies[0].end_revision)

  @mock.patch('logging.error')
  def testProcessTest_LastAlertedRevisionTooHigh_PropertyReset(
      self, mock_logging_error):
    # If the last_alerted_revision property of the TestMetadata is too high,
    # then the property should be reset and an error should be logged.
    self._AddDataForTests()
    test = utils.TestKey(
        'ChromiumGPU/linux-release/scrolling_benchmark/ref').get()

    sheriff.Sheriff(
        email='a@google.com', id='sheriff', patterns=[test.test_path]).put()

    test.last_alerted_revision = 1234567890
    test.put()
    find_anomalies.ProcessTests([test.key])
    self.assertIsNone(test.key.get().last_alerted_revision)
    calls = [
        mock.call(
            'last_alerted_revision %d is higher than highest rev %d for test '
            '%s; setting last_alerted_revision to None.',
            1234567890,
            10066,
            'ChromiumGPU/linux-release/scrolling_benchmark/ref'),
        mock.call(
            'No rows fetched for %s',
            'ChromiumGPU/linux-release/scrolling_benchmark/ref')
    ]
    mock_logging_error.assert_has_calls(calls, any_order=True)

  def testMakeAnomalyEntity_NoRefBuild(self):
    testing_common.AddTests(
        ['ChromiumPerf'],
        ['linux'], {
            'page_cycler_v2': {
                'cnn': {},
                'yahoo': {},
                'nytimes': {},
            },
        })
    test = utils.TestKey('ChromiumPerf/linux/page_cycler_v2').get()
    testing_common.AddRows(test.test_path, [100, 200, 300, 400])

    alert = find_anomalies._MakeAnomalyEntity(
        _MakeSampleChangePoint(10011, 50, 100),
        test,
        list(graph_data.Row.query())).get_result()
    self.assertIsNone(alert.ref_test)

  def testMakeAnomalyEntity_RefBuildSlash(self):
    testing_common.AddTests(
        ['ChromiumPerf'],
        ['linux'], {
            'page_cycler_v2': {
                'ref': {},
                'cnn': {},
                'yahoo': {},
                'nytimes': {},
            },
        })
    test = utils.TestKey('ChromiumPerf/linux/page_cycler_v2').get()
    testing_common.AddRows(test.test_path, [100, 200, 300, 400])

    alert = find_anomalies._MakeAnomalyEntity(
        _MakeSampleChangePoint(10011, 50, 100),
        test,
        list(graph_data.Row.query())).get_result()
    self.assertEqual(alert.ref_test.string_id(),
                     'ChromiumPerf/linux/page_cycler_v2/ref')

  def testMakeAnomalyEntity_RefBuildUnderscore(self):
    testing_common.AddTests(
        ['ChromiumPerf'],
        ['linux'], {
            'page_cycler_v2': {
                'cnn': {},
                'cnn_ref': {},
                'yahoo': {},
                'nytimes': {},
            },
        })
    test = utils.TestKey('ChromiumPerf/linux/page_cycler_v2/cnn').get()
    testing_common.AddRows(test.test_path, [100, 200, 300, 400])

    alert = find_anomalies._MakeAnomalyEntity(
        _MakeSampleChangePoint(10011, 50, 100),
        test,
        list(graph_data.Row.query())).get_result()
    self.assertEqual(alert.ref_test.string_id(),
                     'ChromiumPerf/linux/page_cycler_v2/cnn_ref')
    self.assertIsNone(alert.display_start)
    self.assertIsNone(alert.display_end)

  def testMakeAnomalyEntity_RevisionRanges(self):
    testing_common.AddTests(
        ['ClankInternal'],
        ['linux'], {
            'page_cycler_v2': {
                'cnn': {},
                'cnn_ref': {},
                'yahoo': {},
                'nytimes': {},
            },
        })
    test = utils.TestKey('ClankInternal/linux/page_cycler_v2/cnn').get()
    testing_common.AddRows(test.test_path, [100, 200, 300, 400])
    for row in graph_data.Row.query():
      row.r_commit_pos = int(row.value) + 2 # Different enough to ensure it is
                                            # picked up properly.
      row.put()

    alert = find_anomalies._MakeAnomalyEntity(
        _MakeSampleChangePoint(300, 50, 100),
        test,
        list(graph_data.Row.query())).get_result()
    self.assertEqual(alert.display_start, 203)
    self.assertEqual(alert.display_end, 302)

  def testMakeAnomalyEntity_AddsOwnership(self):
    data_samples = [
        {
            'type': 'GenericSet',
            'guid': 'eb212e80-db58-4cbd-b331-c2245ecbb826',
            'values': ['alice@chromium.org', 'bob@chromium.org']
        },
        {
            'type': 'GenericSet',
            'guid': 'eb212e80-db58-4cbd-b331-c2245ecbb827',
            'values': ['abc']
        }]

    test_key = utils.TestKey('ChromiumPerf/linux/page_cycler_v2/cnn')
    testing_common.AddTests(
        ['ChromiumPerf'],
        ['linux'], {
            'page_cycler_v2': {
                'cnn': {},
                'cnn_ref': {},
                'yahoo': {},
                'nytimes': {},
            },
        })
    test = test_key.get()
    testing_common.AddRows(test.test_path, [100, 200, 300, 400])

    entity = histogram.SparseDiagnostic(
        data=json.dumps(data_samples[0]), test=test_key, start_revision=1,
        end_revision=sys.maxint, id=data_samples[0]['guid'],
        name=reserved_infos.OWNERS.name)
    entity.put()

    entity = histogram.SparseDiagnostic(
        data=json.dumps(data_samples[1]), test=test_key, start_revision=1,
        end_revision=sys.maxint, id=data_samples[1]['guid'],
        name=reserved_infos.BUG_COMPONENTS.name)
    entity.put()

    alert = find_anomalies._MakeAnomalyEntity(
        _MakeSampleChangePoint(10011, 50, 100),
        test,
        list(graph_data.Row.query())).get_result()

    self.assertEqual(alert.ownership['component'], 'abc')
    self.assertListEqual(alert.ownership['emails'],
                         ['alice@chromium.org', 'bob@chromium.org'])

if __name__ == '__main__':
  unittest.main()
