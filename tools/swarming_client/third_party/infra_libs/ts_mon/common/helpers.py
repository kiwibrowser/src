# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper classes that make it easier to instrument code for monitoring."""


from infra_libs.ts_mon.common import metrics

import time


class ScopedIncrementCounter(object):
  """Increment a counter when the wrapped code exits.

  The counter will be given a 'status' = 'success' or 'failure' label whose
  value will be set to depending on whether the wrapped code threw an exception.

  Example:

    mycounter = Counter('foo/stuff_done')
    with ScopedIncrementCounter(mycounter):
      DoStuff()

  To set a custom status label and status value:

    mycounter = Counter('foo/http_requests')
    with ScopedIncrementCounter(mycounter, 'response_code') as sc:
      status = MakeHttpRequest()
      sc.set_status(status)  # This custom status now won't be overwritten if
                             # the code later raises an exception.
  """

  def __init__(self, counter, label='status', success_value='success',
               failure_value='failure'):
    self.counter = counter
    self.label = label
    self.success_value = success_value
    self.failure_value = failure_value
    self.status = None

  def set_failure(self):
    self.set_status(self.failure_value)

  def set_status(self, status):
    self.status = status

  def __enter__(self):
    self.status = None
    return self

  def __exit__(self, exc_type, exc_value, traceback):
    if self.status is None:
      if exc_type is None:
        self.status = self.success_value
      else:
        self.status = self.failure_value
    self.counter.increment({self.label: self.status})


class ScopedMeasureTime(object):
  """Report durations metric with status when the wrapped code exits.

  The metric must be CumulativeDistributionMetric with a field to set status.
  The status field will be set to 'success' or 'failure' depending on whether
  the wrapped code threw an exception. The status field values can be customized
  with constructor kwargs or by calling `set_status`.

  A new instance of this class should be constructed each time it is used.

  Example:

    mymetric = CumulativeDistributionMetric(
      'xxx/durations', 'duration of xxx op'
      [StringField('status')],
      bucketer=ts_mon.GeometricBucketer(10**0.04),
      units=ts_mon.MetricsDataUnits.SECONDS)
    with ScopedMeasureTime(mymetric):
      DoStuff()

  To set a custom label and status value:

    mymetric = CumulativeDistributionMetric(
      'xxx/durations', 'duration of xxx op'
      [IntegerField('response_code')],
      bucketer=ts_mon.GeometricBucketer(10**0.04),
      units=ts_mon.MetricsDataUnits.MILLISECONDS)
    with ScopedMeasureTime(mymetric, field='response_code') as sd:
      sd.set_status(404)  # This custom status now won't be overwritten
                          # even if exception is raised later.
  """

  _UNITS_PER_SECOND = {
      metrics.MetricsDataUnits.SECONDS: 1e0,
      metrics.MetricsDataUnits.MILLISECONDS: 1e3,
      metrics.MetricsDataUnits.MICROSECONDS: 1e6,
      metrics.MetricsDataUnits.NANOSECONDS: 1e9,
  }

  def __init__(self, metric, field='status', success_value='success',
               failure_value='failure', time_fn=time.time):
    assert isinstance(metric, metrics.CumulativeDistributionMetric)
    assert sum(1 for spec in metric.field_spec if spec.name == field) == 1, (
        'typo in field name `%s`?' % field)
    assert metric.units in self._UNITS_PER_SECOND, (
        'metric\'s units (%s) is not one of %s' %
        (metric.units, self._UNITS_PER_SECOND.keys()))

    self._metric = metric
    self._field = field
    self._units_per_second = self._UNITS_PER_SECOND[metric.units]
    self._success_value = success_value
    self._failure_value = failure_value
    self._status = None
    self._start_timestamp = None
    self._time_fn = time_fn

  def set_status(self, status):
    assert self._start_timestamp is not None, (
        'set_status must be called only inside with statement')
    self._status = status

  def set_failure(self):
    return self.set_status(self._failure_value)

  def __enter__(self):
    assert self._start_timestamp is None, ('re-use of ScopedMeasureTime '
                                           'instances detected')
    self._start_timestamp = self._time_fn()
    return self

  def __exit__(self, exc_type, exc_value, traceback):
    elapsed_seconds = self._time_fn() - self._start_timestamp
    if self._status is None:
      if exc_type is None:
        self._status = self._success_value
      else:
        self._status = self._failure_value
    self._metric.add(elapsed_seconds * self._units_per_second,
                     {self._field: self._status})
