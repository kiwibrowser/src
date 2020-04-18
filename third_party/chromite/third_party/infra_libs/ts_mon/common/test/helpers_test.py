# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time
import unittest

import mock

from infra_libs.ts_mon.common import metrics
from infra_libs.ts_mon.common import helpers


class _CustomException(Exception):
  pass


class ScopedIncrementCounterTest(unittest.TestCase):
  def setUp(self):
    self.counter = mock.create_autospec(metrics.CounterMetric, spec_set=True)

  def test_success(self):
    with helpers.ScopedIncrementCounter(self.counter):
      pass
    self.counter.increment.assert_called_once_with({'status': 'success'})

  def test_exception(self):
    with self.assertRaises(_CustomException):
      with helpers.ScopedIncrementCounter(self.counter):
        raise _CustomException()
    self.counter.increment.assert_called_once_with({'status': 'failure'})

  def test_custom_status(self):
    with helpers.ScopedIncrementCounter(self.counter) as sc:
      sc.set_status('foo')
    self.counter.increment.assert_called_once_with({'status': 'foo'})

  def test_set_failure(self):
    with helpers.ScopedIncrementCounter(self.counter) as sc:
      sc.set_failure()
    self.counter.increment.assert_called_once_with({'status': 'failure'})

  def test_custom_status_and_exception(self):
    with self.assertRaises(_CustomException):
      with helpers.ScopedIncrementCounter(self.counter) as sc:
        sc.set_status('foo')
        raise _CustomException()
    self.counter.increment.assert_called_once_with({'status': 'foo'})

  def test_multiple_custom_status_calls(self):
    with helpers.ScopedIncrementCounter(self.counter) as sc:
      sc.set_status('foo')
      sc.set_status('bar')
    self.counter.increment.assert_called_once_with({'status': 'bar'})

  def test_custom_label_success(self):
    with helpers.ScopedIncrementCounter(self.counter, 'a', 'b', 'c'):
      pass
    self.counter.increment.assert_called_once_with({'a': 'b'})

  def test_custom_label_exception(self):
    with self.assertRaises(_CustomException):
      with helpers.ScopedIncrementCounter(self.counter, 'a', 'b', 'c'):
        raise _CustomException()
    self.counter.increment.assert_called_once_with({'a': 'c'})


class ScopedMeasureTimeTest(unittest.TestCase):
  def setUp(self):
    # To avoid floating point nightmare, use values which are exact in IEEE754.
    self.time_fn = mock.Mock(time.time, autospec=True, side_effect=[0.25, 0.50])
    self.metric = mock.create_autospec(metrics.CumulativeDistributionMetric,
                                       spec_set=True)
    self.metric.field_spec = [metrics.StringField('status')]
    self.metric.units = metrics.MetricsDataUnits.SECONDS

  def test_wrong_field(self):
    self.metric.field_spec = [metrics.StringField('wrong')]
    with self.assertRaises(AssertionError):
      helpers.ScopedMeasureTime(self.metric, 'status')

  def test_bad_units(self):
    self.metric.units = metrics.MetricsDataUnits.GIBIBYTES
    with self.assertRaises(AssertionError):
      helpers.ScopedMeasureTime(self.metric)

    self.metric.units = ''
    with self.assertRaises(AssertionError):
      helpers.ScopedMeasureTime(self.metric)

  def test_success(self):
    with helpers.ScopedMeasureTime(self.metric, time_fn=self.time_fn):
      pass
    self.metric.add.assert_called_once_with(0.25, {'status': 'success'})

  def test_exception(self):
    with self.assertRaises(_CustomException):
      with helpers.ScopedMeasureTime(self.metric, time_fn=self.time_fn):
        raise _CustomException()
    self.metric.add.assert_called_once_with(0.25, {'status': 'failure'})

  def test_custom_status(self):
    with helpers.ScopedMeasureTime(self.metric, time_fn=self.time_fn) as sd:
      sd.set_status('foo')
    self.metric.add.assert_called_once_with(0.25, {'status': 'foo'})

  def test_set_failure(self):
    with helpers.ScopedMeasureTime(self.metric, time_fn=self.time_fn) as sd:
      sd.set_failure()
    self.metric.add.assert_called_once_with(0.25, {'status': 'failure'})

  def test_custom_status_and_exception(self):
    with self.assertRaises(_CustomException):
      with helpers.ScopedMeasureTime(self.metric, time_fn=self.time_fn) as sd:
        sd.set_status('foo')
        raise _CustomException()
    self.metric.add.assert_called_once_with(0.25, {'status': 'foo'})

  def test_multiple_custom_status_calls(self):
    with helpers.ScopedMeasureTime(self.metric, time_fn=self.time_fn) as sd:
      sd.set_status('foo')
      sd.set_status('bar')
    self.metric.add.assert_called_once_with(0.25, {'status': 'bar'})

  def test_custom_success(self):
    self.metric.field_spec = [metrics.StringField('label')]
    self.metric.units = metrics.MetricsDataUnits.MILLISECONDS
    with helpers.ScopedMeasureTime(self.metric, 'label', 'ok', 'fail',
                                   time_fn=self.time_fn):
      pass
    self.metric.add.assert_called_once_with(250.0, {'label': 'ok'})

  def test_custom_exception(self):
    self.metric.field_spec = [metrics.StringField('label')]
    self.metric.units = metrics.MetricsDataUnits.MICROSECONDS
    with self.assertRaises(_CustomException):
      with helpers.ScopedMeasureTime(self.metric, 'label', 'ok', 'fail',
                                     time_fn=self.time_fn):
        raise _CustomException()
    self.metric.add.assert_called_once_with(250000.0, {'label': 'fail'})

  def test_extra_fields_should_exclude_status(self):
    with self.assertRaises(AssertionError):
      helpers.ScopedMeasureTime(self.metric,
                                extra_fields_values={'status': 'x'})

  def test_extra_fields(self):
    self.metric.field_spec = [metrics.StringField('custom'),
                              metrics.StringField('type')]
    with helpers.ScopedMeasureTime(self.metric, 'custom', 'ok', 'fail',
                                   extra_fields_values={'type': 'normal'},
                                   time_fn=self.time_fn):
      pass
    self.metric.add.assert_called_once_with(0.25, {'custom': 'ok',
                                                   'type': 'normal'})
