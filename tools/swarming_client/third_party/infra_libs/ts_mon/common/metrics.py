# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes representing individual metrics that can be sent."""

import copy
import re

from infra_libs.ts_mon.protos import metrics_pb2

from infra_libs.ts_mon.common import distribution
from infra_libs.ts_mon.common import errors
from infra_libs.ts_mon.common import interface


MICROSECONDS_PER_SECOND = 1000000


class Field(object):
  FIELD_NAME_PATTERN = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')

  allowed_python_types = None
  type_enum = None
  field_name = None

  def __init__(self, name):
    if not self.FIELD_NAME_PATTERN.match(name):
      raise errors.MetricDefinitionError(
          'Invalid metric field name "%s" - must match the regex "%s"' % (
                name, self.FIELD_NAME_PATTERN.pattern))

    self.name = name

  def validate_value(self, metric_name, value):
    if not isinstance(value, self.allowed_python_types):
      raise errors.MonitoringInvalidFieldTypeError(
          metric_name, self.name, value)

  def populate_proto(self, proto, value):
    setattr(proto, self.field_name, value)


class StringField(Field):
  allowed_python_types = basestring
  type_enum = metrics_pb2.MetricsDataSet.MetricFieldDescriptor.STRING
  field_name = 'string_value'


class IntegerField(Field):
  allowed_python_types = (int, long)
  type_enum = metrics_pb2.MetricsDataSet.MetricFieldDescriptor.INT64
  field_name = 'int64_value'


class BooleanField(Field):
  allowed_python_types = bool
  type_enum = metrics_pb2.MetricsDataSet.MetricFieldDescriptor.BOOL
  field_name = 'bool_value'


class Metric(object):
  """Abstract base class for a metric.

  A Metric is an attribute that may be monitored across many targets. Examples
  include disk usage or the number of requests a server has received. A single
  process may keep track of many metrics.

  Note that Metric objects may be initialized at any time (for example, at the
  top of a library), but cannot be sent until the underlying Monitor object
  has been set up (usually by the top-level process parsing the command line).

  A Metric can actually store multiple values that are identified by a set of
  fields (which are themselves key-value pairs).  Fields can be passed to the
  set() or increment() methods to modify a particular value, or passed to the
  constructor in which case they will be used as the defaults for this Metric.

  The unit of measurement for Metric data should be specified with
  MetricsDataUnits when a Metric object is created:
  e.g., MetricsDataUnits.SECONDS, MetricsDataUnits.BYTES, and etc..,
  See `MetricsDataUnits` class for a full list of units.

  Do not directly instantiate an object of this class.
  Use the concrete child classes instead:
  * StringMetric for metrics with string value
  * BooleanMetric for metrics with boolean values
  * CounterMetric for metrics with monotonically increasing integer values
  * GaugeMetric for metrics with arbitrarily varying integer values
  * CumulativeMetric for metrics with monotonically increasing float values
  * FloatMetric for metrics with arbitrarily varying float values

  See http://go/inframon-doc for help designing and using your metrics.
  """

  def __init__(self, name, description, field_spec, units=None):
    """Create an instance of a Metric.

    Args:
      name (str): the file-like name of this metric
      description (string): help string for the metric. Should be enough to
                            know what the metric is about.
      field_spec (list): a list of Field subclasses to define the fields that
                         are allowed on this metric.  Pass a list of either
                         StringField, IntegerField or BooleanField here.
      units (string): the unit used to measure data for given metric. Some
                      common units are pre-defined in the MetricsDataUnits
                      class.
    """
    field_spec = field_spec or []

    self._name = name.lstrip('/')

    if not isinstance(description, basestring):
      raise errors.MetricDefinitionError('Metric description must be a string')
    if not description:
      raise errors.MetricDefinitionError('Metric must have a description')
    if (not isinstance(field_spec, (list, tuple)) or
        any(not isinstance(x, Field) for x in field_spec)):
      raise errors.MetricDefinitionError(
          'Metric constructor takes a list of Fields, or None')
    if len(field_spec) > 7:
      raise errors.MonitoringTooManyFieldsError(self._name, field_spec)

    self._start_time = None
    self._field_spec = field_spec
    self._sorted_field_names = sorted(x.name for x in field_spec)
    self._description = description
    self._units = units

    interface.register(self)

  @property
  def field_spec(self):
    return list(self._field_spec)

  @property
  def name(self):
    return self._name

  @property
  def start_time(self):
    return self._start_time

  @property
  def units(self):
    return self._units

  def is_cumulative(self):
    raise NotImplementedError()

  def unregister(self):
    interface.unregister(self)

  def populate_data_set(self, data_set):
    """Populate MetricsDataSet."""
    data_set.metric_name = '%s%s' % (interface.state.metric_name_prefix,
                                     self._name)
    data_set.description = self._description or ''
    if self._units is not None:
      data_set.annotations.unit = self._units

    if self.is_cumulative():
      data_set.stream_kind = metrics_pb2.CUMULATIVE
    else:
      data_set.stream_kind = metrics_pb2.GAUGE

    self._populate_value_type(data_set)
    self._populate_field_descriptors(data_set)

  def populate_data(self, data, start_time, end_time, fields, value):
    """Populate a new metrics_pb2.MetricsData.

    Args:
      data (metrics_pb2.MetricsData): protocol buffer into
        which to populate the current metric values.
      start_time (int): timestamp in microseconds since UNIX epoch.
    """
    data.start_timestamp.seconds = int(start_time)
    data.end_timestamp.seconds = int(end_time)

    self._populate_fields(data, fields)
    self._populate_value(data, value)

  def _populate_field_descriptors(self, data_set):
    """Populate `field_descriptor` in MetricsDataSet.

    Args:
      data_set (metrics_pb2.MetricsDataSet): a data set protobuf to populate
    """
    for spec in self._field_spec:
      descriptor = data_set.field_descriptor.add()
      descriptor.name = spec.name
      descriptor.field_type = spec.type_enum

  def _populate_fields(self, data, field_values):
    """Fill in the fields attribute of a metric protocol buffer.

    Args:
      metric (metrics_pb2.MetricsData): a metrics protobuf to populate
      field_values (tuple): field values
    """
    for spec, value in zip(self._field_spec, field_values):
      field = data.field.add()
      field.name = spec.name
      spec.populate_proto(field, value)

  def _validate_fields(self, fields):
    """Checks the correct number and types of field values were provided.

    Args:
      fields (dict): A dict of field values given by the user, or None.

    Returns:
      fields' values as a tuple, in the same order as the field_spec.

    Raises:
      WrongFieldsError: if you provide a different number of fields to those
        the metric was defined with.
      MonitoringInvalidFieldTypeError: if the field value was the wrong type for
        the field spec.
    """
    fields = fields or {}

    if not isinstance(fields, dict):
      raise ValueError('fields should be a dict, got %r (%s)' % (
          fields, type(fields)))

    if sorted(fields) != self._sorted_field_names:
      raise errors.WrongFieldsError(
          self.name, fields.keys(), self._sorted_field_names)

    for spec in self._field_spec:
      spec.validate_value(self.name, fields[spec.name])

    return tuple(fields[spec.name] for spec in self._field_spec)

  def _populate_value(self, data, value):
    """Fill in the the data values of a metric protocol buffer.

    Args:
      data (metrics_pb2.MetricsData): a metrics protobuf to populate
      value (see concrete class): the value of the metric to be set
    """
    raise NotImplementedError()

  def _populate_value_type(self, data_set):
    """Fill in the the data values of a metric protocol buffer.

    Args:
      data_set (metrics_pb2.MetricsDataSet): a MetricsDataSet protobuf to
          populate
    """
    raise NotImplementedError()

  def set(self, value, fields=None, target_fields=None):
    """Set a new value for this metric. Results in sending a new value.

    The subclass should do appropriate type checking on value and then call
    self._set_and_send_value.

    Args:
      value (see concrete class): the value of the metric to be set
      fields (dict): metric field values
      target_fields (dict): overwrite some of the default target fields
    """
    raise NotImplementedError()

  def get(self, fields=None, target_fields=None):
    """Returns the current value for this metric.

    Subclasses should never use this to get a value, modify it and set it again.
    Instead use _incr with a modify_fn.
    """
    return interface.state.store.get(
        self.name, self._validate_fields(fields), target_fields)

  def get_all(self):
    return interface.state.store.iter_field_values(self.name)

  def reset(self):
    """Clears the values of this metric.  Useful in unit tests.

    It might be easier to call ts_mon.reset_for_unittest() in your setUp()
    method instead of resetting every individual metric.
    """

    interface.state.store.reset_for_unittest(self.name)

  def _set(self, fields, target_fields, value, enforce_ge=False):
    interface.state.store.set(
        self.name, self._validate_fields(fields), target_fields,
        value, enforce_ge=enforce_ge)

  def _incr(self, fields, target_fields, delta, modify_fn=None):
    interface.state.store.incr(
        self.name, self._validate_fields(fields), target_fields,
        delta, modify_fn=modify_fn)


class StringMetric(Metric):
  """A metric whose value type is a string."""

  def _populate_value(self, data, value):
    data.string_value = value

  def _populate_value_type(self, data_set):
    data_set.value_type = metrics_pb2.STRING

  def set(self, value, fields=None, target_fields=None):
    if not isinstance(value, basestring):
      raise errors.MonitoringInvalidValueTypeError(self._name, value)
    self._set(fields, target_fields, value)

  def is_cumulative(self):
    return False


class BooleanMetric(Metric):
  """A metric whose value type is a boolean."""

  def _populate_value(self, data, value):
    data.bool_value = value

  def _populate_value_type(self, data_set):
    data_set.value_type = metrics_pb2.BOOL

  def set(self, value, fields=None, target_fields=None):
    if not isinstance(value, bool):
      raise errors.MonitoringInvalidValueTypeError(self._name, value)
    self._set(fields, target_fields, value)

  def is_cumulative(self):
    return False


class NumericMetric(Metric):  # pylint: disable=abstract-method
  """Abstract base class for numeric (int or float) metrics."""

  def increment(self, fields=None, target_fields=None):
    self._incr(fields, target_fields, 1)

  def increment_by(self, step, fields=None, target_fields=None):
    self._incr(fields, target_fields, step)


class CounterMetric(NumericMetric):
  """A metric whose value type is a monotonically increasing integer."""

  def __init__(self, name, description, field_spec, start_time=None,
               units=None):
    super(CounterMetric, self).__init__(
        name, description, field_spec, units=units)
    self._start_time = start_time

  def _populate_value(self, data, value):
    data.int64_value = value

  def _populate_value_type(self, data_set):
    data_set.value_type = metrics_pb2.INT64

  def set(self, value, fields=None, target_fields=None):
    if not isinstance(value, (int, long)):
      raise errors.MonitoringInvalidValueTypeError(self._name, value)
    self._set(fields, target_fields, value, enforce_ge=True)

  def increment_by(self, step, fields=None, target_fields=None):
    if not isinstance(step, (int, long)):
      raise errors.MonitoringInvalidValueTypeError(self._name, step)
    self._incr(fields, target_fields, step)

  def is_cumulative(self):
    return True


class GaugeMetric(NumericMetric):
  """A metric whose value type is an integer."""

  def _populate_value(self, data, value):
    data.int64_value = value

  def _populate_value_type(self, data_set):
    data_set.value_type = metrics_pb2.INT64

  def set(self, value, fields=None, target_fields=None):
    if not isinstance(value, (int, long)):
      raise errors.MonitoringInvalidValueTypeError(self._name, value)
    self._set(fields, target_fields, value)

  def is_cumulative(self):
    return False


class CumulativeMetric(NumericMetric):
  """A metric whose value type is a monotonically increasing float."""

  def __init__(self, name, description, field_spec, start_time=None,
               units=None):
    super(CumulativeMetric, self).__init__(
        name, description, field_spec, units=units)
    self._start_time = start_time

  def _populate_value(self, data, value):
    data.double_value = value

  def _populate_value_type(self, data_set):
    data_set.value_type = metrics_pb2.DOUBLE

  def set(self, value, fields=None, target_fields=None):
    if not isinstance(value, (float, int)):
      raise errors.MonitoringInvalidValueTypeError(self._name, value)
    self._set(fields, target_fields, float(value), enforce_ge=True)

  def is_cumulative(self):
    return True


class FloatMetric(NumericMetric):
  """A metric whose value type is a float."""

  def _populate_value(self, metric, value):
    metric.double_value = value

  def _populate_value_type(self, data_set_pb):
    data_set_pb.value_type = metrics_pb2.DOUBLE

  def set(self, value, fields=None, target_fields=None):
    if not isinstance(value, (float, int)):
      raise errors.MonitoringInvalidValueTypeError(self._name, value)
    self._set(fields, target_fields, float(value))

  def is_cumulative(self):
    return False


class _DistributionMetricBase(Metric):
  """A metric that holds a distribution of values.

  By default buckets are chosen from a geometric progression, each bucket being
  approximately 1.59 times bigger than the last.  In practice this is suitable
  for many kinds of data, but you may want to provide a FixedWidthBucketer or
  GeometricBucketer with different parameters."""

  def __init__(self, name, description, field_spec, is_cumulative=True,
               bucketer=None, start_time=None, units=None):
    super(_DistributionMetricBase, self).__init__(
        name, description, field_spec, units=units)
    self._start_time = start_time

    if bucketer is None:
      bucketer = distribution.GeometricBucketer()

    self._is_cumulative = is_cumulative
    self.bucketer = bucketer

  def _populate_value(self, metric, value):
    pb = metric.distribution_value

    # Copy the bucketer params.
    if value.bucketer.width == 0:
      pb.exponential_buckets.growth_factor = value.bucketer.growth_factor
      pb.exponential_buckets.scale = value.bucketer.scale
      pb.exponential_buckets.num_finite_buckets = (
          value.bucketer.num_finite_buckets)
    else:
      pb.linear_buckets.width = value.bucketer.width
      pb.linear_buckets.offset = 0.0
      pb.linear_buckets.num_finite_buckets = value.bucketer.num_finite_buckets

    # Copy the distribution bucket values.  Include the overflow buckets on
    # either end.
    pb.bucket_count.extend(
        value.buckets.get(i, 0) for i in
        xrange(0, value.bucketer.total_buckets))

    pb.count = value.count
    pb.mean = float(value.sum) / max(value.count, 1)

  def _populate_value_type(self, data_set_pb):
    data_set_pb.value_type = metrics_pb2.DISTRIBUTION

  def add(self, value, fields=None, target_fields=None):
    def modify_fn(dist, value):
      if dist == 0:
        dist = distribution.Distribution(self.bucketer)
      dist.add(value)
      return dist

    self._incr(fields, target_fields, value, modify_fn=modify_fn)

  def set(self, value, fields=None, target_fields=None):
    """Replaces the distribution with the given fields with another one.

    This only makes sense on non-cumulative DistributionMetrics.

    Args:
      value: A infra_libs.ts_mon.Distribution.
    """

    if self._is_cumulative:
      raise TypeError(
          'Cannot set() a cumulative DistributionMetric (use add() instead)')

    if not isinstance(value, distribution.Distribution):
      raise errors.MonitoringInvalidValueTypeError(self._name, value)

    self._set(fields, target_fields, value)

  def is_cumulative(self):
    return self._is_cumulative


class CumulativeDistributionMetric(_DistributionMetricBase):
  """A DistributionMetric with is_cumulative set to True."""

  def __init__(self, name, description, field_spec, bucketer=None, units=None):
    super(CumulativeDistributionMetric, self).__init__(
        name, description, field_spec,
        is_cumulative=True,
        bucketer=bucketer,
        units=units)


class NonCumulativeDistributionMetric(_DistributionMetricBase):
  """A DistributionMetric with is_cumulative set to False."""

  def __init__(self, name, description, field_spec, bucketer=None, units=None):
    super(NonCumulativeDistributionMetric, self).__init__(
        name, description, field_spec,
        is_cumulative=False,
        bucketer=bucketer,
        units=units)


class MetricsDataUnits(object):
  """An container for units of measurement for Metrics data."""

  UNKNOWN_UNITS = '{unknown}'
  SECONDS = 's'
  MILLISECONDS = 'ms'
  MICROSECONDS = 'us'
  NANOSECONDS = 'ns'
  BITS = 'B'
  BYTES = 'By'
  KILOBYTES = 'kBy'
  MEGABYTES = 'MBy'
  GIGABYTES = 'GBy'
  KIBIBYTES = 'kiBy'
  MEBIBYTES = 'MiBy'
  GIBIBYTES = 'GiBy'
  AMPS = 'A'
  MILLIAMPS = 'mA'
  DEGREES_CELSIUS = 'Cel'
