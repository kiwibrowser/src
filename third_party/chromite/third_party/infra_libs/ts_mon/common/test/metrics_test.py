# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import sys
import time
import unittest

from infra_libs.ts_mon.common import distribution
from infra_libs.ts_mon.common import errors
from infra_libs.ts_mon.common import interface
from infra_libs.ts_mon.common import metric_store
from infra_libs.ts_mon.common import metrics
from infra_libs.ts_mon.common import targets
from infra_libs.ts_mon.protos import metrics_pb2


class TestBase(unittest.TestCase):
  def setUp(self):
    super(TestBase, self).setUp()
    target = targets.TaskTarget('test_service', 'test_job',
                                'test_region', 'test_host')
    self.mock_state = interface.State(target=target)
    self.state_patcher = mock.patch('infra_libs.ts_mon.common.interface.state',
                                    new=self.mock_state)
    self.state_patcher.start()

    self.mock_state.target = targets.TaskTarget(
        service_name='service', job_name='job', region='region',
        hostname='hostname', task_num=0)

    self.time_fn = mock.create_autospec(time.time, spec_set=True)
    self.mock_state.store = metric_store.InProcessMetricStore(
        self.mock_state, self.time_fn)

  def tearDown(self):
    self.state_patcher.stop()
    super(TestBase, self).tearDown()

  def _test_proto(self, metric, set_fn, value_type, stream_kind):
    self.time_fn.return_value = 100.3
    interface.register(metric)
    set_fn(metric)

    self.time_fn.return_value = 1000.6
    proto = list(interface._generate_proto())[0]
    data_set = proto.metrics_collection[0].metrics_data_set[0]
    data = data_set.data[0]

    self.assertEqual(stream_kind, data_set.stream_kind)
    self.assertEqual(value_type, data_set.value_type)
    self.assertEqual(100, data.start_timestamp.seconds)
    self.assertEqual(1000, data.end_timestamp.seconds)
    self.assertFalse(data_set.annotations.HasField('unit'))

    return data


class MetricTest(TestBase):

  def test_properties(self):
    field_spec = [metrics.StringField('string')]
    m1 = metrics.Metric('/foo', 'foo', field_spec, 'us')
    self.assertEquals(m1.name, 'foo')
    self.assertEquals(m1.field_spec, field_spec)
    self.assertEquals(m1.units, 'us')

  def test_equality(self):
    field_spec = [metrics.StringField('string')]
    m = metrics.Metric('/foo', 'foo', field_spec, 'us')
    self.assertEquals(m, m)

  def test_init_too_many_fields(self):
    fields = [metrics.StringField('field%d' % i) for i in xrange(8)]
    with self.assertRaises(errors.MonitoringTooManyFieldsError) as e:
      metrics.Metric('test', 'test', fields)
    self.assertEquals(e.exception.metric, 'test')
    self.assertEquals(len(e.exception.fields), 8)

  def test_set_wrong_number_of_fields(self):
    m = metrics.StringMetric('foo', 'foo', [metrics.IntegerField('asdf')])
    with self.assertRaises(errors.WrongFieldsError):
      m.set('bar', {'asdf': 1, 'foo': 2})

  def test_set_list_fields(self):
    m = metrics.StringMetric('foo', 'foo', [metrics.IntegerField('asdf')])
    with self.assertRaises(ValueError):
      m.set('bar', [1])

  def test_set_object_fields(self):
    m = metrics.StringMetric('foo', 'foo', [metrics.IntegerField('asdf')])
    with self.assertRaises(ValueError):
      m.set('bar', object())

  def test_register_unregister(self):
    self.assertEquals(0, len(self.mock_state.metrics))
    m = metrics.Metric('test', 'test', None)
    self.assertEquals(1, len(self.mock_state.metrics))
    m.unregister()
    self.assertEquals(0, len(self.mock_state.metrics))

  def test_reset(self):
    m = metrics.StringMetric('test', 'test', None)
    self.assertIsNone(m.get())
    m.set('foo')
    self.assertEqual('foo', m.get())
    m.reset()
    self.assertIsNone(m.get())

  def test_populate_data_set(self):
    interface.state.metric_name_prefix = '/infra/test/'
    scenarios = [
        (metrics.CounterMetric, 'desc', metrics_pb2.CUMULATIVE),
        (metrics.GaugeMetric, 'desc', metrics_pb2.GAUGE)]
    for m_ctor, desc, stream_kind in scenarios:
      m = m_ctor(m_ctor.__name__, desc, None,
                 units=metrics.MetricsDataUnits.SECONDS)
      data_set = metrics_pb2.MetricsDataSet()
      m.populate_data_set(data_set)

      self.assertEqual(stream_kind, data_set.stream_kind)
      self.assertEqual('/infra/test/%s' % m_ctor.__name__, data_set.metric_name)
      self.assertEqual(desc, data_set.description)
      self.assertEqual('s', data_set.annotations.unit)

  def test_populate_data(self):
    m = metrics.CounterMetric('test', 'test', None)
    data = metrics_pb2.MetricsData()
    m.populate_data(data, 100.4, 1000.6, {}, 5)

    self.assertEqual(100, data.start_timestamp.seconds)
    self.assertEqual(1000, data.end_timestamp.seconds)

  def test_populate_field_descriptor(self):
    data_set_pb = metrics_pb2.MetricsDataSet()
    m = metrics.Metric('test', 'test', [
        metrics.IntegerField('a'),
        metrics.BooleanField('b'),
        metrics.StringField('c'),
    ])
    m._populate_field_descriptors(data_set_pb)

    field_type = metrics_pb2.MetricsDataSet.MetricFieldDescriptor
    self.assertEqual(3, len(data_set_pb.field_descriptor))

    self.assertEqual('a', data_set_pb.field_descriptor[0].name)
    self.assertEqual(field_type.INT64,
                     data_set_pb.field_descriptor[0].field_type)

    self.assertEqual('b', data_set_pb.field_descriptor[1].name)
    self.assertEqual(field_type.BOOL,
                     data_set_pb.field_descriptor[1].field_type)

    self.assertEqual('c', data_set_pb.field_descriptor[2].name)
    self.assertEqual(field_type.STRING,
                     data_set_pb.field_descriptor[2].field_type)

  def test_populate_fields(self):
    data = metrics_pb2.MetricsData()
    m = metrics.Metric('test', 'test', [
        metrics.IntegerField('a'),
        metrics.BooleanField('b'),
        metrics.StringField('c'),
    ])
    m._populate_fields(data, (1, True, 'test'))

    self.assertEqual(3, len(data.field))

    self.assertEqual('a', data.field[0].name)
    self.assertEqual(1, data.field[0].int64_value)

    self.assertEqual('b', data.field[1].name)
    self.assertTrue(data.field[1].bool_value)

    self.assertEqual('c', data.field[2].name)
    self.assertEqual('test', data.field[2].string_value)

  def test_bad_description(self):
    with self.assertRaises(errors.MetricDefinitionError):
      metrics.Metric('test', 123, None)
    with self.assertRaises(errors.MetricDefinitionError):
      metrics.Metric('test', '', None)
    with self.assertRaises(errors.MetricDefinitionError):
      metrics.Metric('test', None, None)

  def test_bad_field_spec(self):
    with self.assertRaises(errors.MetricDefinitionError):
      metrics.Metric('test', 'desc', ['abc'])
    with self.assertRaises(errors.MetricDefinitionError):
      metrics.Metric('test', 'desc', ('abc',))
    with self.assertRaises(errors.MetricDefinitionError):
      metrics.Metric('test', 'desc', [123])
    with self.assertRaises(errors.MetricDefinitionError):
      metrics.Metric('test', 'desc', [None])


class FieldValidationTest(TestBase):
  def test_string_field(self):
    f = metrics.StringField('name')
    f.validate_value('', 'string')
    f.validate_value('', u'string')
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', 123)
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', long(123))
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', True)
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', None)
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', 12.34)

  def test_integer_field(self):
    f = metrics.IntegerField('name')
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', 'string')
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', u'string')
    f.validate_value('', 123)
    f.validate_value('', long(123))
    f.validate_value('', True)  # Python allows this *shrug*
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', None)
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', 12.34)

  def test_boolean_field(self):
    f = metrics.BooleanField('name')
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', 'string')
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', u'string')
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', 123)
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', long(123))
    f.validate_value('', True)
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', None)
    with self.assertRaises(errors.MonitoringInvalidFieldTypeError):
      f.validate_value('', 12.34)

  def test_invalid_field_name(self):
    with self.assertRaises(errors.MetricDefinitionError):
      metrics.StringField('foo', 'desc', [metrics.StringField(' ')])
    with self.assertRaises(errors.MetricDefinitionError):
      metrics.StringField('foo', 'desc', [metrics.StringField('123')])
    with self.assertRaises(errors.MetricDefinitionError):
      metrics.StringField('foo', 'desc', [metrics.StringField('')])
    with self.assertRaises(errors.MetricDefinitionError):
      metrics.StringField('foo', 'desc', [metrics.StringField(u'\U0001F4A9')])

  def test_equality(self):
    f = metrics.IntegerField('name')
    self.assertEquals(f, f)


class StringMetricTest(TestBase):

  def test_generate_proto(self):
    proto = self._test_proto(
        metrics.StringMetric('t', 't', None), lambda m: m.set('aaa'),
        metrics_pb2.STRING, metrics_pb2.GAUGE)
    self.assertEqual('aaa', proto.string_value)

  def test_set(self):
    m = metrics.StringMetric('test', 'test', None)
    m.set('hello world')
    self.assertEquals(m.get(), 'hello world')

  def test_non_string_raises(self):
    m = metrics.StringMetric('test', 'test', None)
    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.set(object())

  def test_is_cumulative(self):
    m = metrics.StringMetric('test', 'test', None)
    self.assertFalse(m.is_cumulative())


class BooleanMetricTest(TestBase):

  def test_generate_proto(self):
    proto = self._test_proto(
        metrics.BooleanMetric('test', 'test', None),
        lambda m: m.set(True),
        metrics_pb2.BOOL, metrics_pb2.GAUGE)
    self.assertTrue(proto.bool_value)

  def test_set(self):
    m = metrics.BooleanMetric('test', 'test', None)
    m.set(False)
    self.assertEquals(m.get(), False)

  def test_non_bool_raises(self):
    m = metrics.BooleanMetric('test', 'test', None)
    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.set(object())
    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.set('True')
    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.set(123)

  def test_is_cumulative(self):
    m = metrics.BooleanMetric('test', 'test', None)
    self.assertFalse(m.is_cumulative())


class CounterMetricTest(TestBase):

  def test_generate_proto(self):
    proto = self._test_proto(
        metrics.CounterMetric('c', 'test', None),
        lambda m: m.increment_by(5),
        metrics_pb2.INT64, metrics_pb2.CUMULATIVE)
    self.assertEqual(5, proto.int64_value)

  def test_set(self):
    m = metrics.CounterMetric('test', 'test', None)
    m.set(10)
    self.assertEquals(m.get(), 10)

  def test_increment(self):
    m = metrics.CounterMetric('test', 'test', None)
    m.set(1)
    self.assertEquals(m.get(), 1)
    m.increment()
    self.assertEquals(m.get(), 2)
    m.increment_by(3)
    self.assertAlmostEquals(m.get(), 5)

  def test_decrement_raises(self):
    m = metrics.CounterMetric('test', 'test', None)
    m.set(1)
    with self.assertRaises(errors.MonitoringDecreasingValueError):
      m.set(0)
    with self.assertRaises(errors.MonitoringDecreasingValueError):
      m.increment_by(-1)

  def test_non_int_raises(self):
    m = metrics.CounterMetric('test', 'test', None)
    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.set(object())
    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.set(1.5)
    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.increment_by(1.5)

  def test_multiple_field_values(self):
    m = metrics.CounterMetric('test', 'test', [metrics.StringField('foo')])
    m.increment({'foo': 'bar'})
    m.increment({'foo': 'baz'})
    m.increment({'foo': 'bar'})
    with self.assertRaises(errors.WrongFieldsError):
      m.get()
    self.assertIsNone(m.get({'foo': ''}))
    self.assertEquals(2, m.get({'foo': 'bar'}))
    self.assertEquals(1, m.get({'foo': 'baz'}))

  def test_is_cumulative(self):
    m = metrics.CounterMetric('test', 'test', None)
    self.assertTrue(m.is_cumulative())

  def test_get_all(self):
    m = metrics.CounterMetric('test', 'test', [metrics.StringField('foo')])
    m.increment({'foo': ''})
    m.increment({'foo': 'bar'})
    self.assertEqual([
        (('',), 1),
        (('bar',), 1),
    ], sorted(m.get_all()))


class GaugeMetricTest(TestBase):

  def test_generate_proto(self):
    proto = self._test_proto(
        metrics.GaugeMetric('test', 'test', None), lambda m: m.set(5),
        metrics_pb2.INT64, metrics_pb2.GAUGE)
    self.assertEqual(5, proto.int64_value)

  def test_set(self):
    m = metrics.GaugeMetric('test', 'test', None)
    m.set(10)
    self.assertEquals(m.get(), 10)
    m.set(sys.maxint + 1)
    self.assertEquals(m.get(), sys.maxint + 1)

  def test_non_int_raises(self):
    m = metrics.GaugeMetric('test', 'test', None)
    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.set(object())

  def test_is_cumulative(self):
    m = metrics.GaugeMetric('test', 'test', None)
    self.assertFalse(m.is_cumulative())


class CumulativeMetricTest(TestBase):

  def test_generate_proto(self):
    proto = self._test_proto(
        metrics.CumulativeMetric('c', 'test', None),
        lambda m: m.increment_by(5.2),
        metrics_pb2.DOUBLE, metrics_pb2.CUMULATIVE)
    self.assertAlmostEqual(5.2, proto.double_value)

  def test_set(self):
    m = metrics.CumulativeMetric('test', 'test', None)
    m.set(3.14)
    self.assertAlmostEquals(m.get(), 3.14)

  def test_decrement_raises(self):
    m = metrics.CumulativeMetric('test', 'test', None)
    m.set(3.14)
    with self.assertRaises(errors.MonitoringDecreasingValueError):
      m.set(0)
    with self.assertRaises(errors.MonitoringDecreasingValueError):
      m.increment_by(-1)

  def test_non_number_raises(self):
    m = metrics.CumulativeMetric('test', 'test', None)
    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.set(object())

  def test_is_cumulative(self):
    m = metrics.CumulativeMetric('test', 'test', None)
    self.assertTrue(m.is_cumulative())


class FloatMetricTest(TestBase):

  def test_generate_proto(self):
    proto = self._test_proto(
        metrics.FloatMetric('test', 'test', None), lambda m: m.set(1.23),
        metrics_pb2.DOUBLE, metrics_pb2.GAUGE)
    self.assertAlmostEqual(1.23, proto.double_value)

  def test_set(self):
    m = metrics.FloatMetric('test', 'test', None)
    m.set(3.14)
    self.assertEquals(m.get(), 3.14)

  def test_non_number_raises(self):
    m = metrics.FloatMetric('test', 'test', None)
    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.set(object())

  def test_is_cumulative(self):
    m = metrics.FloatMetric('test', 'test', None)
    self.assertFalse(m.is_cumulative())


class DistributionMetricTest(TestBase):

  def _test_distribution_proto(self, dist):
    interface.register(dist)

    self.time_fn.return_value = 100.3
    for num in [0, 1, 5, 5.5, 9, 10, 10000]:
      dist.add(num)

    self.time_fn.return_value = 1000.6
    proto = list(interface._generate_proto())[0]
    data_set = proto.metrics_collection[0].metrics_data_set[0]
    data = data_set.data[0]

    self.assertAlmostEqual(1432.928571428, data.distribution_value.mean)
    self.assertEqual(metrics_pb2.DISTRIBUTION, data_set.value_type)
    self.assertEqual(100, data.start_timestamp.seconds)
    self.assertEqual(1000, data.end_timestamp.seconds)
    self.assertFalse(data_set.annotations.HasField('unit'))

    return data_set, data

  def test_generate_fixed_width_distribution(self):
    bucketer = distribution.FixedWidthBucketer(width=1, num_finite_buckets=10)
    dists = [
      (metrics.NonCumulativeDistributionMetric(
           'test0', 'test', None, bucketer=bucketer),
       metrics_pb2.GAUGE),
      (metrics.CumulativeDistributionMetric(
           'test1', 'test', None, bucketer=bucketer),
       metrics_pb2.CUMULATIVE)
    ]

    for dist, stream_kind in dists:
      data_set, data = self._test_distribution_proto(dist)

      self.assertListEqual([0, 1, 1, 0, 0, 0, 2, 0, 0, 0, 1, 2],
                           list(data.distribution_value.bucket_count))
      self.assertEqual(
          10, data.distribution_value.linear_buckets.num_finite_buckets)
      self.assertEqual(1, data.distribution_value.linear_buckets.width)
      self.assertEqual(stream_kind, data_set.stream_kind)
      self.assertEqual(7, data.distribution_value.count)

  def test_generate_geometric_distribution(self):
    bucketer = distribution.GeometricBucketer(growth_factor=10**2,
                                              num_finite_buckets=10)
    dists = [
      (metrics.NonCumulativeDistributionMetric(
           'test0', 'test', None, bucketer=bucketer),
       metrics_pb2.GAUGE),
      (metrics.CumulativeDistributionMetric(
           'test1', 'test', None, bucketer=bucketer),
       metrics_pb2.CUMULATIVE)
    ]

    for dist, stream_kind in dists:
      data_set, data = self._test_distribution_proto(dist)

      self.assertListEqual([1, 5, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0],
                           list(data.distribution_value.bucket_count))
      self.assertEqual(
          10, data.distribution_value.exponential_buckets.num_finite_buckets)
      self.assertEqual(
          10**2, data.distribution_value.exponential_buckets.growth_factor)
      self.assertEqual(stream_kind, data_set.stream_kind)
      self.assertEqual(7, data.distribution_value.count)

  def test_generate_geometric_distribution_with_scale(self):
    bucketer = distribution.GeometricBucketer(growth_factor=10.0,
                                              num_finite_buckets=10,
                                              scale=0.1)
    dists = [
      (metrics.NonCumulativeDistributionMetric(
           'test0', 'test', None, bucketer=bucketer),
       metrics_pb2.GAUGE),
      (metrics.CumulativeDistributionMetric(
           'test1', 'test', None, bucketer=bucketer),
       metrics_pb2.CUMULATIVE)
    ]

    for dist, stream_kind in dists:
      data_set, data = self._test_distribution_proto(dist)

      self.assertListEqual([1, 0, 4, 1, 0, 0, 1, 0, 0, 0, 0, 0],
                           list(data.distribution_value.bucket_count))
      self.assertEqual(
          10, data.distribution_value.exponential_buckets.num_finite_buckets)
      self.assertEqual(
          10.0, data.distribution_value.exponential_buckets.growth_factor)
      self.assertEqual(
          0.1, data.distribution_value.exponential_buckets.scale)
      self.assertEqual(stream_kind, data_set.stream_kind)
      self.assertEqual(7, data.distribution_value.count)

  def test_add(self):
    m = metrics.CumulativeDistributionMetric('test', 'test', None)
    m.add(1)
    m.add(10)
    m.add(100)
    self.assertEquals({1: 1, 5: 1, 10: 1}, m.get().buckets)
    self.assertEquals(111, m.get().sum)
    self.assertEquals(3, m.get().count)

  def test_add_custom_bucketer(self):
    m = metrics.CumulativeDistributionMetric('test', 'test', None,
        bucketer=distribution.FixedWidthBucketer(10))
    m.add(1)
    m.add(10)
    m.add(100)
    self.assertEquals({1: 1, 2: 1, 11: 1}, m.get().buckets)
    self.assertEquals(111, m.get().sum)
    self.assertEquals(3, m.get().count)

  def test_set(self):
    d = distribution.Distribution(
        distribution.FixedWidthBucketer(10, num_finite_buckets=10))
    d.add(1)
    d.add(10)
    d.add(100)

    m = metrics.CumulativeDistributionMetric('test', 'test', None)
    with self.assertRaises(TypeError):
      m.set(d)

    m = metrics.NonCumulativeDistributionMetric('test2', 'test', None)
    m.set(d)
    self.assertEquals(d, m.get())

    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.set(1)
    with self.assertRaises(errors.MonitoringInvalidValueTypeError):
      m.set('foo')

  def test_is_cumulative(self):
    cd = metrics.CumulativeDistributionMetric('test', 'test', None)
    ncd = metrics.NonCumulativeDistributionMetric('test2', 'test', None)
    self.assertTrue(cd.is_cumulative())
    self.assertFalse(ncd.is_cumulative())
