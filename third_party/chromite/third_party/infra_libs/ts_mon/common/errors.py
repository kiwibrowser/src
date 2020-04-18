# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes representing errors that can be raised by the monitoring library."""


class MonitoringError(Exception):
  """Base class for exceptions raised by this module."""


class MonitoringDecreasingValueError(MonitoringError):
  """Raised when setting a metric value that should increase but doesn't."""

  def __init__(self, metric, old_value, new_value):
    self.metric = metric
    self.old_value = old_value
    self.new_value = new_value

  def __str__(self):
    return ('Monotonically increasing metric "%s" was given value "%s", which '
            'is not greater than or equal to "%s".' % (
                self.metric, self.new_value, self.old_value))


class MonitoringDuplicateRegistrationError(MonitoringError):
  """Raised when trying to register a metric with the same name as another."""

  def __init__(self, metric):
    self.metric = metric

  def __str__(self):
    return 'Different metrics with the same name "%s" were both registered.' % (
        self.metric)


class MonitoringIncrementUnsetValueError(MonitoringError):
  """Raised when trying to increment a metric which hasn't been set."""

  def __init__(self, metric):
    self.metric = metric

  def __str__(self):
    return 'Metric "%s" was incremented without first setting a value.' % (
        self.metric)


class MonitoringInvalidValueTypeError(MonitoringError):
  """Raised when sending a metric value is not a valid type."""

  def __init__(self, metric, value):
    self.metric = metric
    self.value = value

  def __str__(self):
    return 'Metric "%s" was given invalid value "%s" (%s).' % (
        self.metric, self.value, type(self.value))


class MonitoringInvalidFieldTypeError(MonitoringError):
  """Raised when sending a metric with a field value of an invalid type."""

  def __init__(self, metric, field, value):
    self.metric = metric
    self.field = field
    self.value = value

  def __str__(self):
    return 'Metric "%s" was given field "%s" with invalid value "%s" (%s).' % (
        self.metric, self.field, self.value, type(self.value))


class MonitoringTooManyFieldsError(MonitoringError):
  """Raised when sending a metric with more than 7 fields."""

  def __init__(self, metric, fields):
    self.metric = metric
    self.fields = fields

  def __str__(self):
    return 'Metric "%s" was given too many (%d > 7) fields: %s.' % (
        self.metric, len(self.fields), self.fields)


class MonitoringNoConfiguredMonitorError(MonitoringError):
  """Raised when sending a metric without configuring the global Monitor."""

  def __init__(self, metric):
    self.metric = metric

  def __str__(self):
    if self.metric is not None:
      return 'Metric "%s" was sent before initializing the global Monitor.' % (
          self.metric)
    else:
      return 'Metrics were sent before initializing the global Monitor.'


class MonitoringNoConfiguredTargetError(MonitoringError):
  """Raised when sending a metric with no global nor local Target."""

  def __init__(self, metric):
    self.metric = metric

  def __str__(self):
    return 'Metric "%s" was sent with no Target configured.' % (self.metric)


class MonitoringFailedToFlushAllMetricsError(MonitoringError):
  """Raised when some error is encountered in flushing specific metrics."""

  def __init__(self, error_count):
    self.error_count = error_count

  def __str__(self):
    return ('Failed to flush %d metrics. See tracebacks above' %
            (self.error_count))


class MetricDefinitionError(MonitoringError):
  """Raised when a metric was defined incorrectly."""


class WrongFieldsError(MonitoringError):
  """Raised when a metric is given different fields to its definition."""

  def __init__(self, metric_name, got, expected):
    self.metric_name = metric_name
    self.got = got
    self.expected = expected

  def __str__(self):
    return 'Metric "%s" is defined with %s fields but was given %s' % (
        self.metric_name, self.expected, self.got)
