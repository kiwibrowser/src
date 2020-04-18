# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for chromite.lib.metrics."""

from __future__ import print_function

import datetime
import mock
import tempfile

from chromite.lib import cros_test_lib
from chromite.lib import metrics
from chromite.lib import parallel
from chromite.lib import ts_mon_config

from infra_libs import ts_mon


class FakeException(Exception):
  """FakeException to raise during tests."""


class TestIndirectMetrics(cros_test_lib.MockTestCase):
  """Tests the behavior of _Indirect metrics."""

  def testEnqueue(self):
    """Test that _Indirect enqueues messages correctly."""
    metric = metrics.Boolean

    with parallel.Manager() as manager:
      q = manager.Queue()
      self.PatchObject(metrics, 'MESSAGE_QUEUE', q)

      proxy_metric = metric('foo')
      proxy_metric.example('arg1', {'field_name': 'value'})

      message = q.get(timeout=10)

    expected_metric_kwargs = {
        'field_spec': [ts_mon.StringField('field_name')],
        'description': 'No description.',
    }
    self.assertEqual(
        message,
        metrics.MetricCall(metric.__name__, ('foo',), expected_metric_kwargs,
                           'example', ('arg1', {'field_name': 'value'}), {},
                           False))

  def patchTime(self):
    """Simulate time passing to force a Flush() every time a metric is sent."""
    def TimeIterator():
      t = 0
      while True:
        t += ts_mon_config.FLUSH_INTERVAL + 1
        yield t

    self.PatchObject(ts_mon_config,
                     'time',
                     mock.Mock(time=mock.Mock(side_effect=TimeIterator())))

  def testShortLived(self):
    """Tests that configuring ts-mon to use short-lived processes works."""
    self.patchTime()
    with tempfile.NamedTemporaryFile(dir='/var/tmp') as out:
      with ts_mon_config.SetupTsMonGlobalState('metrics_unittest',
                                               short_lived=True,
                                               debug_file=out.name):
        # pylint: disable=protected-access
        self.assertTrue(ts_mon_config._WasSetup)


  def testResetAfter(self):
    """Tests that the reset_after flag works to send metrics only once."""
    # By mocking out its "time" module, the forked flushing process will think
    # it should call Flush() whenever we send a metric.
    self.patchTime()

    with tempfile.NamedTemporaryFile(dir='/var/tmp') as out:
      # * The indirect=True flag is required for reset_after to work.
      # * Using debug_file, we send metrics to the temporary file instead of
      # sending metrics to production via PubSub.
      with ts_mon_config.SetupTsMonGlobalState('metrics_unittest',
                                               indirect=True,
                                               debug_file=out.name):
        def MetricName(i, flushed):
          return 'test/metric/name/%d/%s' % (i, flushed)

        # Each of these .set() calls will result in a Flush() call.
        for i in range(7):
          # any extra streams with different fields and reset_after=False
          # will be cleared only if the below metric is cleared.
          metrics.Boolean(
              MetricName(i, True), reset_after=False).set(
                  True, fields={'original': False})

          metrics.Boolean(
              MetricName(i, True), reset_after=True).set(
                  True, fields={'original': True})

        for i in range(7):
          metrics.Boolean(
              MetricName(i, False),
              reset_after=False).set(True)


      # By leaving the context, we .join() the flushing process.
      with open(out.name, 'r') as fh:
        content = fh.read()

      # The reset metrics should be sent only three times, because:
      # * original=False is sent twice
      # * original=True is sent once.
      # The second flush() only results in one occurance of the string
      # MetricName(i, True) because both data points are in a "metrics_data_set"
      # block, like so:
      # metrics_collection {
      #   ... etc ...
      #   metrics_data_set {
      #     metric_name: "/chrome/infra/test/metric/name/0/True"
      #     data {
      #       bool_value: true
      #       field {
      #         name: "original"
      #         bool_value: false
      #       }
      #     }
      #     data {
      #       bool_value: true
      #       ... etc ...
      for i in range(7):
        self.assertEqual(content.count(MetricName(i, True)), 2)

      # The non-reset metrics are sent once-per-flush.
      # There are 7 of these metrics,
      # * The 0th is sent 7 times,
      # * The 1st is sent 6 times,
      # ...
      # * The 6th is sent 1 time.
      # So the "i"th metric is sent (7-i) times.
      for i in range(7):
        self.assertEqual(content.count(MetricName(i, False)), 7 - i)


class TestSecondsTimer(cros_test_lib.MockTestCase):
  """Tests the behavior of SecondsTimer and SecondsTimerDecorator."""

  def setUp(self):
    self._mockMetric = mock.MagicMock()
    self.PatchObject(metrics, 'CumulativeSecondsDistribution',
                     return_value=self._mockMetric)

  @metrics.SecondsTimerDecorator('fooname', fields={'foo': 'bar'})
  def _DecoratedFunction(self, *args, **kwargs):
    pass

  def testDecorator(self):
    """Test that calling a decorated function ends up emitting metric."""
    self._DecoratedFunction(1, 2, 3, foo='bar')
    self.assertEqual(metrics.CumulativeSecondsDistribution.call_count, 1)
    self.assertEqual(self._mockMetric.add.call_count, 1)

  def testContextManager(self):
    """Test that timing context manager emits a metric."""
    with metrics.SecondsTimer('fooname'):
      pass
    self.assertEqual(metrics.CumulativeSecondsDistribution.call_count, 1)
    self.assertEqual(self._mockMetric.add.call_count, 1)

  def testContextManagerWithUpdate(self):
    """Tests that timing context manager with a field update emits metric."""
    with metrics.SecondsTimer('fooname', fields={'foo': 'bar'}) as c:
      c['foo'] = 'qux'
    self._mockMetric.add.assert_called_with(mock.ANY, fields={'foo': 'qux'})

  def testContextManagerWithoutUpdate(self):
    """Tests that the default value for fields is used when not updated."""
    # pylint: disable=unused-variable
    with metrics.SecondsTimer('fooname', fields={'foo': 'bar'}) as c:
      pass
    self._mockMetric.add.assert_called_with(mock.ANY, fields={'foo': 'bar'})

  def testContextManagerIgnoresInvalidField(self):
    """Test that we ignore fields that are set with no default."""
    with metrics.SecondsTimer('fooname', fields={'foo': 'bar'}) as c:
      c['qux'] = 'qwert'
    self._mockMetric.add.assert_called_with(mock.ANY, fields={'foo': 'bar'})

  def testContextManagerWithException(self):
    """Tests that we emit metrics if the timed method raised something."""
    with self.assertRaises(AssertionError):
      with metrics.SecondsTimer('fooname', fields={'foo': 'bar'}):
        assert False

    self._mockMetric.add.assert_called_with(mock.ANY, fields={'foo': 'bar'})

class TestSuccessCounter(cros_test_lib.MockTestCase):
  """Tests the behavior of SecondsTimer."""

  def setUp(self):
    self._mockMetric = mock.MagicMock()
    self.PatchObject(metrics, 'Counter',
                     return_value=self._mockMetric)

  def testContextManager(self):
    """Test that timing context manager emits a metric."""
    with metrics.SuccessCounter('fooname'):
      pass
    self._mockMetric.increment.assert_called_with(
        fields={'success': True})
    self.assertEqual(self._mockMetric.increment.call_count, 1)

  def testContextManagerFailedException(self):
    """Test that we fail when an exception is raised."""
    with self.assertRaises(FakeException):
      with metrics.SuccessCounter('fooname'):
        raise FakeException

    self._mockMetric.increment.assert_called_with(
        fields={'success': False})

  def testContextManagerFailedExplicit(self):
    """Test that we fail when an exception is raised."""
    with metrics.SuccessCounter('fooname') as s:
      s['success'] = False

    self._mockMetric.increment.assert_called_with(
        fields={'success': False})

  def testContextManagerWithUpdate(self):
    """Tests that context manager with a field update emits metric."""
    with metrics.SuccessCounter('fooname', fields={'foo': 'bar'}) as c:
      c['foo'] = 'qux'
    self._mockMetric.increment.assert_called_with(
        fields={'foo': 'qux', 'success': True})

  def testContextManagerWithoutUpdate(self):
    """Tests that the default value for fields is used when not updated."""
    # pylint: disable=unused-variable
    with metrics.SuccessCounter('fooname', fields={'foo': 'bar'}) as c:
      pass
    self._mockMetric.increment.assert_called_with(
        fields={'foo': 'bar', 'success': True})

  def testContextManagerIgnoresInvalidField(self):
    """Test that we ignore fields that are set with no default."""
    with metrics.SuccessCounter('fooname', fields={'foo': 'bar'}) as c:
      c['qux'] = 'qwert'

    self._mockMetric.increment.assert_called_with(
        fields={'foo': 'bar', 'success': True})


class TestPresence(cros_test_lib.MockTestCase):
  """Tests the behavior of SecondsTimer."""

  def setUp(self):
    self._mockMetric = mock.MagicMock()
    self.PatchObject(metrics, 'Boolean',
                     return_value=self._mockMetric)

  def testContextManager(self):
    """Test that timing context manager emits a metric."""
    with metrics.Presence('fooname'):
      self.assertEquals(self._mockMetric.mock_calls, [
          mock.call.set(True, fields=None),
      ])

    self.assertEquals(self._mockMetric.mock_calls, [
        mock.call.set(True, fields=None),
        mock.call.set(False, fields=None),
    ])

  def testContextManagerException(self):
    """Test that we fail when an exception is raised."""
    with self.assertRaises(FakeException):
      with metrics.Presence('fooname'):
        raise FakeException

    self.assertEquals(self._mockMetric.mock_calls, [
        mock.call.set(True, fields=None),
        mock.call.set(False, fields=None),
    ])

  def testContextManagerFields(self):
    """Test that we fail when an exception is raised."""
    with metrics.Presence('fooname', {'foo': 'bar', 'c': 3}):
      pass

    self.assertEquals(self._mockMetric.mock_calls, [
        mock.call.set(True, fields={'c': 3, 'foo': 'bar'}),
        mock.call.set(False, fields={'c': 3, 'foo': 'bar'}),
    ])


class ClientException(Exception):
  """An exception that client of the metrics module raises."""


class TestRuntimeBreakdownTimer(cros_test_lib.MockTestCase):
  """Tests the behaviour of RuntimeBreakdownTimer."""

  def setUp(self):
    # Only patch metrics.datetime because we don't want to affect calls to
    # functions from the unittest running framework itself.
    self.datetime_mock = self.PatchObject(metrics, 'datetime')
    self.datetime_mock.datetime.now.side_effect = self._GetFakeTime
    # An arbitrary, but fixed, seed time.
    self._fake_time = datetime.datetime(1, 2, 3)

    metric_mock = self.PatchObject(metrics, 'CumulativeSecondsDistribution',
                                   autospec=True)
    self._mockCumulativeSecondsDistribution = metric_mock.return_value
    metric_mock = self.PatchObject(metrics, 'PercentageDistribution',
                                   autospec=True)
    self._mockPercentageDistribution = metric_mock.return_value
    metric_mock = self.PatchObject(metrics, 'Float', autospec=True)
    self._mockFloat = metric_mock.return_value

    metric_mock = self.PatchObject(metrics, 'CumulativeMetric', autospec=True)
    self._mockCumulativeMetric = metric_mock.return_value

  def testSucessfulBreakdown(self):
    """Tests that the context manager emits expected breakdowns."""
    with metrics.RuntimeBreakdownTimer('fubar') as runtime:
      with runtime.Step('step1'):
        self._IncrementFakeTime(4)
      with runtime.Step('step2'):
        self._IncrementFakeTime(1)
      self._IncrementFakeTime(5)

    self.assertEqual(metrics.CumulativeSecondsDistribution.call_count, 1)
    self.assertEqual(metrics.CumulativeSecondsDistribution.call_args[0][0],
                     'fubar/total_duration')
    self.assertEqual(self._mockCumulativeSecondsDistribution.add.call_count, 1)
    self.assertEqual(
        self._mockCumulativeSecondsDistribution.add.call_args[0][0], 10.0)

    self.assertEqual(metrics.PercentageDistribution.call_count, 3)
    breakdown_names = [x[0][0] for x in
                       metrics.PercentageDistribution.call_args_list]
    self.assertEqual(set(breakdown_names),
                     {'fubar/breakdown/step1', 'fubar/breakdown/step2',
                      'fubar/breakdown_unaccounted'})
    breakdown_values = [x[0][0] for x in
                        self._mockPercentageDistribution.add.call_args_list]
    self.assertEqual(set(breakdown_values), {40.0, 10.0, 50.0})

    self.assertEqual(metrics.CumulativeMetric.call_count, 1)
    self.assertEqual(metrics.CumulativeMetric.call_args[0][0],
                     'fubar/bucketing_loss')

    self.assertEqual(metrics.Float.call_count, 2)
    for call_args in metrics.Float.call_args_list:
      self.assertEqual(call_args[0][0], 'fubar/duration_breakdown')
    self.assertEqual(self._mockFloat.set.call_count, 2)
    step_names = [x[1]['fields']['step_name']
                  for x  in self._mockFloat.set.call_args_list]
    step_ratios = [x[0][0] for x  in self._mockFloat.set.call_args_list]
    self.assertEqual(set(step_names), {'step1', 'step2'})
    self.assertEqual(set(step_ratios), {0.4, 0.1})

  def testBucketingLossApproximately(self):
    """Tests that we report the bucketing loss correctly."""
    with metrics.RuntimeBreakdownTimer('fubar') as runtime:
      for i in range(300):
        with runtime.Step('step%d' % i):
          self._IncrementFakeTime(1)

    self.assertEqual(metrics.CumulativeSecondsDistribution.call_count, 1)
    self.assertEqual(metrics.CumulativeSecondsDistribution.call_args[0][0],
                     'fubar/total_duration')
    self.assertEqual(self._mockCumulativeSecondsDistribution.add.call_count, 1)
    self.assertEqual(
        self._mockCumulativeSecondsDistribution.add.call_args[0][0], 300.0)

    self.assertEqual(metrics.CumulativeMetric.call_count, 1)
    self.assertEqual(metrics.CumulativeMetric.call_args[0][0],
                     'fubar/bucketing_loss')
    self.assertEqual(self._mockCumulativeMetric.increment_by.call_count, 1)
    # Each steps is roughly 1/300 ~ .33%.
    # Our bucket resolution is 0.1 % so we expect to lose ~ 0.033% each report.
    # Total # of reports = 300. So we'll lose ~9.99%
    # Let's loosely bound that number to allow for floating point computation
    # errors.
    error = self._mockCumulativeMetric.increment_by.call_args[0][0]
    self.assertGreater(error, 9.6)
    self.assertLess(error, 10.2)

  def testStepsWithClientCodeException(self):
    """Test that breakdown is reported correctly when client code raises."""
    with self.assertRaises(ClientException):
      with metrics.RuntimeBreakdownTimer('fubar') as runtime:
        with runtime.Step('step1'):
          self._IncrementFakeTime(1)
          raise ClientException()

    self.assertEqual(metrics.PercentageDistribution.call_count, 2)
    breakdown_names = [x[0][0] for x in
                       metrics.PercentageDistribution.call_args_list]
    self.assertEqual(set(breakdown_names),
                     {'fubar/breakdown/step1', 'fubar/breakdown_unaccounted'})

    self.assertEqual(metrics.Float.call_count, 1)
    self.assertEqual(metrics.Float.call_args[0][0], 'fubar/duration_breakdown')
    self.assertEqual(self._mockFloat.set.call_count, 1)
    self.assertEqual(self._mockFloat.set.call_args[1]['fields']['step_name'],
                     'step1')

  def testNestedStepIgnored(self):
    """Tests that trying to enter nested .Step contexts raises."""
    with metrics.RuntimeBreakdownTimer('fubar') as runtime:
      with runtime.Step('step1'):
        with runtime.Step('step2'):
          self._IncrementFakeTime(1)

    self.assertEqual(metrics.PercentageDistribution.call_count, 2)
    breakdown_names = [x[0][0] for x in
                       metrics.PercentageDistribution.call_args_list]
    self.assertEqual(set(breakdown_names),
                     {'fubar/breakdown/step1', 'fubar/breakdown_unaccounted'})

    self.assertEqual(metrics.Float.call_count, 1)
    self.assertEqual(metrics.Float.call_args[0][0], 'fubar/duration_breakdown')
    self.assertEqual(self._mockFloat.set.call_count, 1)
    self.assertEqual(self._mockFloat.set.call_args[1]['fields']['step_name'],
                     'step1')

  def testNestedStepsWithClientCodeException(self):
    """Test that breakdown is reported correctly when client code raises."""
    with self.assertRaises(ClientException):
      with metrics.RuntimeBreakdownTimer('fubar') as runtime:
        with runtime.Step('step1'):
          with runtime.Step('step2'):
            self._IncrementFakeTime(1)
            raise ClientException()

    self.assertEqual(metrics.PercentageDistribution.call_count, 2)
    breakdown_names = [x[0][0] for x in
                       metrics.PercentageDistribution.call_args_list]
    self.assertEqual(set(breakdown_names),
                     {'fubar/breakdown/step1', 'fubar/breakdown_unaccounted'})

    self.assertEqual(metrics.Float.call_count, 1)
    self.assertEqual(metrics.Float.call_args[0][0], 'fubar/duration_breakdown')
    self.assertEqual(self._mockFloat.set.call_count, 1)
    self.assertEqual(self._mockFloat.set.call_args[1]['fields']['step_name'],
                     'step1')

  def _GetFakeTime(self):
    return self._fake_time

  def _IncrementFakeTime(self, seconds):
    self._fake_time = self._fake_time + datetime.timedelta(seconds=seconds)
