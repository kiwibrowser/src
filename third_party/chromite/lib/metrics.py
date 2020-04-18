# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wrapper library around ts_mon.

This library provides some wrapper functionality around ts_mon, to make it more
friendly to developers. It also provides import safety, in case ts_mon is not
deployed with your code.
"""

from __future__ import print_function

import collections
import contextlib
import datetime
import Queue
import ssl
from functools import wraps

from chromite.lib import cros_logging as logging

try:
  from infra_libs import ts_mon
except (ImportError, RuntimeError):
  ts_mon = None


# This number is chosen because 1.16^100 seconds is about
# 32 days. This is a good compromise between bucket size
# and dynamic range.
_SECONDS_BUCKET_FACTOR = 1.16

# If none, we create metrics in this process. Otherwise, we send metrics via
# this Queue to a dedicated flushing processes.
# These attributes are set by chromite.lib.ts_mon_config.SetupTsMonGlobalState.
FLUSHING_PROCESS = None
MESSAGE_QUEUE = None

_MISSING = object()

MetricCall = collections.namedtuple('MetricCall', [
    'metric_name', 'metric_args', 'metric_kwargs',
    'method', 'method_args', 'method_kwargs',
    'reset_after'
])


def _FlushingProcessClosed():
  """Returns whether the metrics flushing process has been closed."""
  return (FLUSHING_PROCESS is not None and
          FLUSHING_PROCESS.exitcode is not None)


class ProxyMetric(object):
  """Redirects any method calls to the message queue."""
  def __init__(self, metric, metric_args, metric_kwargs):
    self.metric = metric
    self.metric_args = metric_args
    self.reset_after = metric_kwargs.pop('reset_after', False)
    self.metric_kwargs = metric_kwargs

  def __getattr__(self, method_name):
    """Redirects all method calls to the MESSAGE_QUEUE."""
    def enqueue(*args, **kwargs):
      if not _FlushingProcessClosed():
        try:
          MESSAGE_QUEUE.put_nowait(
              MetricCall(
                  metric_name=self.metric,
                  metric_args=self.metric_args,
                  metric_kwargs=self.metric_kwargs,
                  method=method_name,
                  method_args=args,
                  method_kwargs=kwargs,
                  reset_after=self.reset_after))
        except Queue.Full:
          logging.warning(
              "Metrics queue is full; skipped sending metric '%s'",
              self.metric)
      else:
        try:
          exit_code = FLUSHING_PROCESS.exitcode
        except AttributeError:
          exit_code = None
        logging.warning(
            "Flushing process has been closed (exit code %s),"
            " skipped sending metric '%s'",
            exit_code,
            self.metric)

    return enqueue


def _Indirect(fn):
  """Decorates a function to be indirect If MESSAGE_QUEUE is set.

  If MESSAGE_QUEUE is set, the indirect function will return a proxy metrics
  object; otherwise, it behaves normally.
  """
  @wraps(fn)
  def AddToQueueIfPresent(*args, **kwargs):
    if MESSAGE_QUEUE:
      return ProxyMetric(fn.__name__, args, kwargs)
    else:
      # Whether to reset the metric after the flush; this is only used by
      # |ProxyMetric|, so remove this from the kwargs.
      kwargs.pop('reset_after', None)
      return fn(*args, **kwargs)
  return AddToQueueIfPresent


class MockMetric(object):
  """Mock metric object, to be returned if ts_mon is not set up."""

  def _mock_method(self, *args, **kwargs):
    pass

  def __getattr__(self, _):
    return self._mock_method


def _ImportSafe(fn):
  """Decorator which causes |fn| to return MockMetric if ts_mon not imported."""
  @wraps(fn)
  def wrapper(*args, **kwargs):
    if ts_mon:
      return fn(*args, **kwargs)
    else:
      return MockMetric()

  return wrapper


class FieldSpecAdapter(object):
  """Infers the types of fields values to work around field_spec requirement.

  See: https://chromium-review.googlesource.com/c/432120/ for the change
  which added a required field_spec argument. This class is a temporary
  workaround to allow inferring the field_spec if is not provided.
  """
  FIELD_CLASSES = {} if ts_mon is None else {
      bool: ts_mon.BooleanField,
      int: ts_mon.IntegerField,
      str: ts_mon.StringField,
      unicode: ts_mon.StringField,
  }

  def __init__(self, metric_cls, *args, **kwargs):
    self._metric_cls = metric_cls
    self._args = args
    self._kwargs = kwargs
    self._instance = _MISSING

  def __getattr__(self, prop):
    """Return a wrapper which constructs the metric object on demand.

    Args:
      prop: The property name

    Returns:
      If self._instance has been created, the instance's .|prop| property,
      otherwise, a wrapper function which creates the ._instance and then
      calls the |prop| method on the instance.
    """
    if self._instance is not _MISSING:
      return getattr(self._instance, prop)

    def func(*args, **kwargs):
      if self._instance is not _MISSING:
        return getattr(self._instance, prop)(*args, **kwargs)
      fields = FieldSpecAdapter._InferFields(prop, args, kwargs)
      self._kwargs['field_spec'] = FieldSpecAdapter._InferFieldSpec(fields)
      self._instance = self._metric_cls(*self._args, **self._kwargs)
      return getattr(self._instance, prop)(*args, **kwargs)

    func.__name__ = prop
    return func

  @staticmethod
  def _InferFields(method_name, args, kwargs):
    """Infers the fields argument.

    Args:
      method_name: The method called.
      args: The args list
      kwargs: The keyword args
    """
    if 'fields' in kwargs:
      return kwargs['fields']

    if method_name == 'increment' and args:
      return args[0]

    if len(args) >= 2:
      return args[1]

  @staticmethod
  def _InferFieldSpec(fields):
    """Infers the fields types from the given fields.

    Args:
      fields: A dictionary with metric fields.
    """
    if not fields or not ts_mon:
      return None

    return [FieldSpecAdapter.FIELD_CLASSES[type(v)](field)
            for (field, v) in sorted(fields.iteritems())]


def _OptionalFieldSpec(fn):
  """Decorates a function to allow an optional description and field_spec."""
  @wraps(fn)
  def wrapper(*args, **kwargs):
    kwargs = dict(**kwargs)  # It's bad practice to mutate **kwargs
    # Slightly different than .setdefault, this line sets a default even when
    # the key is present (as long as the value is not truthy). Empty or None is
    # not allowed for descriptions.
    kwargs['description'] = kwargs.get('description') or 'No description.'
    if 'field_spec' in kwargs and kwargs['field_spec'] is not _MISSING:
      return fn(*args, **kwargs)
    else:
      return FieldSpecAdapter(fn, *args, **kwargs)
  return wrapper


def _Metric(fn):
  """A pipeline of decorators to apply to our metric constructors."""
  return _OptionalFieldSpec(_ImportSafe(_Indirect(fn)))


# This is needed for the reset_after flag used by @Indirect.
# pylint: disable=unused-argument

@_Metric
def CounterMetric(name, reset_after=False, description=None,
                  field_spec=_MISSING, start_time=None):
  """Returns a metric handle for a counter named |name|."""
  return ts_mon.CounterMetric(name,
                              description=description, field_spec=field_spec,
                              start_time=start_time)
Counter = CounterMetric


@_Metric
def GaugeMetric(name, reset_after=False, description=None, field_spec=_MISSING):
  """Returns a metric handle for a gauge named |name|."""
  return ts_mon.GaugeMetric(name, description=description,
                            field_spec=field_spec)
Gauge = GaugeMetric


@_Metric
def CumulativeMetric(name, reset_after=False, description=None,
                     field_spec=_MISSING):
  """Returns a metric handle for a cumulative float named |name|."""
  return ts_mon.CumulativeMetric(name, description=description,
                                 field_spec=field_spec)


@_Metric
def StringMetric(name, reset_after=False, description=None,
                 field_spec=_MISSING):
  """Returns a metric handle for a string named |name|."""
  return ts_mon.StringMetric(name, description=description,
                             field_spec=field_spec)
String = StringMetric


@_Metric
def BooleanMetric(name, reset_after=False, description=None,
                  field_spec=_MISSING):
  """Returns a metric handle for a boolean named |name|."""
  return ts_mon.BooleanMetric(name, description=description,
                              field_spec=field_spec)
Boolean = BooleanMetric


@_Metric
def FloatMetric(name, reset_after=False, description=None, field_spec=_MISSING):
  """Returns a metric handle for a float named |name|."""
  return ts_mon.FloatMetric(name, description=description,
                            field_spec=field_spec)
Float = FloatMetric


@_Metric
def CumulativeDistributionMetric(name, reset_after=False, description=None,
                                 bucketer=None, field_spec=_MISSING):
  """Returns a metric handle for a cumulative distribution named |name|."""
  return ts_mon.CumulativeDistributionMetric(
      name, description=description, bucketer=bucketer, field_spec=field_spec)
CumulativeDistribution = CumulativeDistributionMetric


@_Metric
def DistributionMetric(name, reset_after=False, description=None,
                       bucketer=None, field_spec=_MISSING):
  """Returns a metric handle for a distribution named |name|."""
  return ts_mon.NonCumulativeDistributionMetric(
      name, description=description, bucketer=bucketer, field_spec=field_spec)
Distribution = DistributionMetric


@_Metric
def CumulativeSmallIntegerDistribution(name, reset_after=False,
                                       description=None, field_spec=_MISSING):
  """Returns a metric handle for a cumulative distribution named |name|.

  This differs slightly from CumulativeDistribution, in that the underlying
  metric uses a uniform bucketer rather than a geometric one.

  This metric type is suitable for holding a distribution of numbers that are
  nonnegative integers in the range of 0 to 100.
  """
  return ts_mon.CumulativeDistributionMetric(
      name,
      bucketer=ts_mon.FixedWidthBucketer(1),
      description=description,
      field_spec=field_spec)


@_Metric
def CumulativeSecondsDistribution(name, scale=1, reset_after=False,
                                  description=None, field_spec=_MISSING):
  """Returns a metric handle for a cumulative distribution named |name|.

  The distribution handle returned by this method is better suited than the
  default one for recording handling times, in seconds.

  This metric handle has bucketing that is optimized for time intervals
  (in seconds) in the range of 1 second to 32 days. Use |scale| to adjust this
  (e.g. scale=0.1 covers a range from .1 seconds to 3.2 days).

  Args:
    name: string name of metric
    scale: scaling factor of buckets, and size of the first bucket. default: 1
    reset_after: Should the metric be reset after reporting.
    description: A string description of the metric.
    field_spec: A sequence of ts_mon.Field objects to specify the field schema.
  """
  b = ts_mon.GeometricBucketer(growth_factor=_SECONDS_BUCKET_FACTOR,
                               scale=scale)
  return ts_mon.CumulativeDistributionMetric(
      name, bucketer=b, units=ts_mon.MetricsDataUnits.SECONDS,
      description=description, field_spec=field_spec)

SecondsDistribution = CumulativeSecondsDistribution


@_Metric
def PercentageDistribution(
    name, num_buckets=1000, reset_after=False,
    description=None, field_spec=_MISSING):
  """Returns a metric handle for a cumulative distribution for percentage.

  The distribution handle returned by this method is better suited for reporting
  percentage values than the default one. The bucketing is optimized for values
  in [0,100].

  Args:
    name: The name of this metric.
    num_buckets: This metric buckets the percentage values before
        reporting. This argument controls the number of the bucket the range
        [0,100] is divided in. The default gives you 0.1% resolution.
    reset_after: Should the metric be reset after reporting.
    description: A string description of the metric.
    field_spec: A sequence of ts_mon.Field objects to specify the field schema.
  """
  # The last bucket actually covers [100, 100 + 1.0/num_buckets), so it
  # corresponds to values that exactly match 100%.
  bucket_width = 100.0 / num_buckets
  b = ts_mon.FixedWidthBucketer(bucket_width, num_buckets)
  return ts_mon.CumulativeDistributionMetric(
      name, bucketer=b,
      description=description, field_spec=field_spec)


@contextlib.contextmanager
def SecondsTimer(name, fields=None, description=None, field_spec=_MISSING,
                 scale=1, record_on_exception=True, add_exception_field=False):
  """Record the time of an operation to a CumulativeSecondsDistributionMetric.

  Records the time taken inside of the context block, to the
  CumulativeSecondsDistribution named |name|, with the given fields.

  Usage:

  # Time the doSomething() call, with field values that are independent of the
  # results of the operation.
  with SecondsTimer('timer/name', fields={'foo': 'bar'},
                    description="My timer",
                    field_spec=[ts_mon.StringField('foo'),
                                ts_mon.BooleanField('success')]):
    doSomething()

  # Time the doSomethingElse call, with field values that depend on the results
  # of that operation. Note that it is important that a default value is
  # specified for these fields, in case an exception is thrown by
  # doSomethingElse()
  f = {'success': False, 'foo': 'bar'}
  with SecondsTimer('timer/name', fields=f, description="My timer",
                    field_spec=[ts_mon.StringField('foo')]) as c:
    doSomethingElse()
    c['success'] = True

  # Incorrect Usage!
  with SecondsTimer('timer/name', description="My timer") as c:
    doSomething()
    c['foo'] = bar # 'foo' is not a valid field, because no default
                   # value for it was specified in the context constructor.
                   # It will be silently ignored.

  Args:
    name: The name of the metric to create
    fields: The fields of the metric to create.
    description: A string description of the metric.
    field_spec: A sequence of ts_mon.Field objects to specify the field schema.
    scale: A float to scale the CumulativeSecondsDistribution buckets by.
    record_on_exception: Whether to record metrics if an exception is raised.
    add_exception_field: Whether to add a BooleanField("encountered_exception")
        to the FieldSpec provided, and set its value to True iff an exception
        was raised in the context.
  """
  if field_spec is not None and field_spec is not _MISSING:
    field_spec.append(ts_mon.BooleanField('encountered_exception'))

  m = CumulativeSecondsDistribution(
      name, scale=scale, description=description, field_spec=field_spec)
  f = fields or {}
  f = dict(f)
  keys = f.keys()
  t0 = datetime.datetime.now()

  error = True
  try:
    yield f
    error = False
  finally:
    if record_on_exception and add_exception_field:
      keys.append('encountered_exception')
      f.setdefault('encountered_exception', error)
    # Filter out keys that were not part of the initial key set. This is to
    # avoid inconsistent fields.
    # TODO(akeshet): Doing this filtering isn't super efficient. Would be better
    # to implement some key-restricted subclass or wrapper around dict, and just
    # yield that above rather than yielding a regular dict.
    if record_on_exception or not error:
      dt = (datetime.datetime.now() - t0).total_seconds()
      m.add(dt, fields={k: f[k] for k in keys})


def SecondsTimerDecorator(name, fields=None, description=None,
                          field_spec=_MISSING, scale=1,
                          record_on_exception=True, add_exception_field=False):
  """Decorator to time the duration of function calls.

  Usage:
    @SecondsTimerDecorator('timer/name', fields={'foo': 'bar'},
                           description="My timer",
                           field_spec=[ts_mon.StringField('foo')])
    def Foo(bar):
      return doStuff()

    is equivalent to

    def Foo(bar):
      with SecondsTimer('timer/name', fields={'foo': 'bar'},
                        description="My timer",
                        field_spec=[ts_mon.StringField('foo')])
        return doStuff()

  Args:
    name: The name of the metric to create
    fields: The fields of the metric to create
    description: A string description of the metric.
    field_spec: A sequence of ts_mon.Field objects to specify the field schema.
    scale: A float to scale the distrubtion by
    record_on_exception: Whether to record metrics if an exception is raised.
    add_exception_field: Whether to add a BooleanField("encountered_exception")
        to the FieldSpec provided, and set its value to True iff an exception
        was raised in the context.
  """
  def decorator(fn):
    @wraps(fn)
    def wrapper(*args, **kwargs):
      with SecondsTimer(name, fields=fields, description=description,
                        field_spec=field_spec, scale=scale,
                        record_on_exception=record_on_exception,
                        add_exception_field=add_exception_field):
        return fn(*args, **kwargs)

    return wrapper

  return decorator


@contextlib.contextmanager
def SuccessCounter(name, fields=None, description=None, field_spec=_MISSING):
  """Create a counter that tracks if something succeeds.

  Args:
    name: The name of the metric to create
    fields: The fields of the metric
    description: A string description of the metric.
    field_spec: A sequence of ts_mon.Field objects to specify the field schema.
  """
  c = Counter(name)
  f = fields or {}
  f = f.copy()
  keys = f.keys() + ['success']  # We add in the additional field success.
  success = False
  try:
    yield f
    success = True
  finally:
    f.setdefault('success', success)
    f = {k: f[k] for k in keys}
    c.increment(fields=f)


@contextlib.contextmanager
def Presence(name, fields=None, description=None, field_spec=_MISSING):
  """A counter of 'active' things.

  This keeps track of how many name's are active at any given time. However,
  it's only suitable for long running tasks, since the initial true value may
  never be written out if the task doesn't run for at least a minute.
  """
  b = Boolean(name, description=None, field_spec=field_spec)
  b.set(True, fields=fields)
  try:
    yield
  finally:
    b.set(False, fields=fields)


class RuntimeBreakdownTimer(object):
  """Record the time of an operation and the breakdown into sub-steps.

  Usage:
    with RuntimeBreakdownTimer('timer/name', fields={'foo':'bar'},
                               description="My timer",
                               field_spec=[ts_mon.StringField('foo')]) as timer:
      with timer.Step('first_step'):
        doFirstStep()
      with timer.Step('second_step'):
        doSecondStep()
      # The time spent next will show up under .../timer/name/breakdown_no_step
      doSomeNonStepWork()

  This will emit the following metrics:
  - .../timer/name/total_duration - A CumulativeSecondsDistribution metric for
        the time spent inside the outer with block.
  - .../timer/name/breakdown/first_step and
    .../timer/name/breakdown/second_step - PercentageDistribution metrics for
        the fraction of time devoted to each substep.
  - .../timer/name/breakdown_unaccounted - PercentageDistribution metric for the
        fraction of time that is not accounted for in any of the substeps.
  - .../timer/name/bucketing_loss - PercentageDistribution metric buckets values
        before reporting them as distributions. This causes small errors in the
        reported values because they are rounded to the reported buckets lower
        bound. This is a CumulativeMetric measuring the total rounding error
        accrued in reporting all the percentages. The worst case bucketing loss
        for x steps is (x+1)/10. So, if you time across 9 steps, you should
        expect no more than 1% rounding error.
  [experimental]
  - .../timer/name/duration_breakdown - A Float metric, with one stream per Step
        indicating the ratio of time spent in that step. The different steps are
        differentiated via a field with key 'step_name'. Since some of the time
        can be spent outside any steps, these ratios will sum to <= 1.

  NB: This helper can only be used if the field values are known at the
  beginning of the outer context and do not change as a result of any of the
  operations timed.
  """

  PERCENT_BUCKET_COUNT = 1000

  _StepMetrics = collections.namedtuple('_StepMetrics', ['name', 'time_s'])

  def __init__(self, name, fields=None, description=None, field_spec=_MISSING):
    self._name = name
    self._fields = fields
    self._field_spec = field_spec
    self._description = description
    self._outer_t0 = None
    self._total_time_s = 0
    self._inside_step = False
    self._step_metrics = []

  def __enter__(self):
    self._outer_t0 = datetime.datetime.now()
    return self

  def __exit__(self, _type, _value, _traceback):
    self._RecordTotalTime()

    outer_timer = CumulativeSecondsDistribution(
        '%s/total_duration' % (self._name,),
        field_spec=self._field_spec,
        description=self._description)
    outer_timer.add(self._total_time_s, fields=self._fields)

    for name, percent in self._GetStepBreakdowns().iteritems():
      step_metric = PercentageDistribution(
          '%s/breakdown/%s' % (self._name, name),
          num_buckets=self.PERCENT_BUCKET_COUNT,
          field_spec=self._field_spec,
          description=self._description)
      step_metric.add(percent, fields=self._fields)

      fields = dict(self._fields) if self._fields is not None else dict()
      fields['step_name'] = name
      # TODO(pprabhu): Convert _GetStepBreakdowns() to return ratios instead of
      # percentage when the old PercentageDistribution reporting is deleted.
      Float('%s/duration_breakdown' % self._name).set(percent / 100.0,
                                                      fields=fields)

    unaccounted_metric = PercentageDistribution(
        '%s/breakdown_unaccounted' % self._name,
        num_buckets=self.PERCENT_BUCKET_COUNT,
        field_spec=self._field_spec,
        description=self._description)
    unaccounted_metric.add(self._GetUnaccountedBreakdown(), fields=self._fields)

    bucketing_loss_metric = CumulativeMetric(
        '%s/bucketing_loss' % self._name,
        field_spec=self._field_spec,
        description=self._description)
    bucketing_loss_metric.increment_by(self._GetBucketingLoss(),
                                       fields=self._fields)

  @contextlib.contextmanager
  def Step(self, step_name):
    """Start a new step named step_name in the timed operation.

    Note that it is not possible to start a step inside a step. i.e.,

    with RuntimeBreakdownTimer('timer') as timer:
      with timer.Step('outer_step'):
        with timer.Step('inner_step'):
          # will by design raise an exception.

    Args:
      step_name: The name of the step being timed.
    """
    if self._inside_step:
      logging.error('RuntimeBreakdownTimer.Step is not reentrant. '
                    'Dropping step: %s', step_name)
      yield
      return

    self._inside_step = True
    t0 = datetime.datetime.now()
    try:
      yield
    finally:
      self._inside_step = False
      step_time_s = (datetime.datetime.now() - t0).total_seconds()
      self._step_metrics.append(self._StepMetrics(step_name, step_time_s))

  def _GetStepBreakdowns(self):
    """Returns percentage of time spent in each step.

    Must be called after |_RecordTotalTime|.
    """
    if not self._total_time_s:
      return {}
    return {x.name: (x.time_s * 100.0) / self._total_time_s
            for x in self._step_metrics}

  def _GetUnaccountedBreakdown(self):
    """Returns the percentage time spent outside of all steps.

    Must be called after |_RecordTotalTime|.
    """
    breakdown_percentages = sum(self._GetStepBreakdowns().itervalues())
    return max(0, 100 - breakdown_percentages)

  def _GetBucketingLoss(self):
    """Compute the actual loss in reported percentages due to bucketing.

    Must be called after |_RecordTotalTime|.
    """
    reported = self._GetStepBreakdowns().values()
    reported.append(self._GetUnaccountedBreakdown())
    bucket_width = 100.0 / self.PERCENT_BUCKET_COUNT
    return sum(x % bucket_width for x in reported)

  def _RecordTotalTime(self):
    self._total_time_s = (
        datetime.datetime.now() - self._outer_t0).total_seconds()


def Flush(reset_after=()):
  """Flushes metrics, but warns on transient errors.

  Args:
    reset_after: A list of metrics to reset after flushing.
  """
  if not ts_mon:
    return

  try:
    ts_mon.flush()
    while reset_after:
      reset_after.pop().reset()
  except ssl.SSLError as e:
    logging.warning('Caught transient network error while flushing: %s', e)
  except Exception as e:
    logging.error('Caught exception while flushing: %s', e)
