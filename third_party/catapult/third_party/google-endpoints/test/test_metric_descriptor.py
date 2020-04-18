# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import

import datetime
import unittest2
from expects import be_none, be_true, expect, equal, raise_error

import google.api.gen.servicecontrol_v1_messages as messages
from google.api.control import (distribution, metric_descriptor, timestamp, MetricKind,
                        ValueType)
from google.api.control import metric_value, operation, report_request


_KNOWN = metric_descriptor.KnownMetrics


def _wanted_distribution_with_sample(value, *args):
    d = distribution.create_exponential(*args)
    distribution.add_sample(value, d)
    return d


def _given_info(wanted_size, test_api_key, api_key_valid=True):
    return report_request.Info(
        request_size=wanted_size,
        response_size=wanted_size,
        request_time=datetime.timedelta(seconds=3),
        backend_time=datetime.timedelta(seconds=2),
        overhead_time=datetime.timedelta(seconds=1),
        api_key=test_api_key,
        api_key_valid=api_key_valid,
    )


class KnownMetricsBase(object):
    SUBJECT = None
    WANTED_ADDED_METRICS = []
    WANTED_SIZE = 7426 # arbitrary
    TEST_API_KEY = 'test_key'
    GIVEN_INFO = _given_info(WANTED_SIZE, TEST_API_KEY, api_key_valid=True)

    def _base_operation(self):
        return messages.Operation(
            consumerId='project:project_id',
            operationId='an_op_id',
            operationName='an_op_name')

    def _wanted_operation(self):
        op = self._base_operation()
        if self.WANTED_ADDED_METRICS:
            op.metricValueSets.append(self.WANTED_ADDED_METRICS)
        return op

    def _matching_descriptor(self):
        return messages.MetricDescriptor(
            name=self.SUBJECT.metric_name,
            metricKind=self.SUBJECT.kind,
            valueType=self.SUBJECT.value_type)

    def _not_matched(self):
        d = self._matching_descriptor()
        d.metricKind = MetricKind.METRIC_KIND_UNSPECIFIED
        return d

    def test_should_be_supported(self):
        expect(_KNOWN.is_supported(self._matching_descriptor())).to(be_true)
        expect(_KNOWN.is_supported(self._not_matched())).not_to(be_true)

    def test_should_be_matched_correctly(self):
        expect(self.SUBJECT.matches(self._matching_descriptor())).to(be_true)
        expect(self.SUBJECT.matches(self._not_matched())).not_to(be_true)

    def test_should_update_request_info(self):
        an_op = self._base_operation()
        wanted_op = self._wanted_operation()
        self.SUBJECT.do_operation_update(self.GIVEN_INFO, an_op)
        expect(an_op).to(equal(wanted_op))


class KnownMetricsInvalidApiKey(KnownMetricsBase):
    GIVEN_INFO = _given_info(KnownMetricsBase.WANTED_SIZE,
                             KnownMetricsBase.TEST_API_KEY,
                             api_key_valid=False)


class TestConsumerRequestCount(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_REQUEST_COUNT
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(int64Value=1)])


class TestConsumerRequestCountInvalidApiKey(KnownMetricsInvalidApiKey,
                                            unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_REQUEST_COUNT


class TestProducerRequestCount(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.PRODUCER_REQUEST_COUNT
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(int64Value=1)])


class TestConsumerRequestSizes(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_REQUEST_SIZES
    WANTED_DISTRIBUTION = _wanted_distribution_with_sample(
        KnownMetricsBase.WANTED_SIZE,
        *metric_descriptor._SIZE_DISTRIBUTION_ARGS)
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(distributionValue=WANTED_DISTRIBUTION)])


class TestConsumerRequestSizesInvalidApiKey(KnownMetricsInvalidApiKey,
                                            unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_REQUEST_SIZES


class TestProducerRequestSizes(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.PRODUCER_REQUEST_SIZES
    WANTED_DISTRIBUTION = _wanted_distribution_with_sample(
        KnownMetricsBase.WANTED_SIZE,
        *metric_descriptor._SIZE_DISTRIBUTION_ARGS)
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(distributionValue=WANTED_DISTRIBUTION)])


class TestConsumerResponseSizes(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_RESPONSE_SIZES
    WANTED_DISTRIBUTION = _wanted_distribution_with_sample(
        KnownMetricsBase.WANTED_SIZE,
        *metric_descriptor._SIZE_DISTRIBUTION_ARGS)
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(distributionValue=WANTED_DISTRIBUTION)])


class TestConsumerResponseSizesInvalidApiKey(KnownMetricsInvalidApiKey,
                                             unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_RESPONSE_SIZES


class TestProducerResponseSizes(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.PRODUCER_RESPONSE_SIZES
    WANTED_DISTRIBUTION = _wanted_distribution_with_sample(
        KnownMetricsBase.WANTED_SIZE,
        *metric_descriptor._SIZE_DISTRIBUTION_ARGS)
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(distributionValue=WANTED_DISTRIBUTION)])


class TestConsumerErrorCountNoError(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_ERROR_COUNT


class TestConsumerErrorCountWithErrors(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_ERROR_COUNT
    GIVEN_INFO = report_request.Info(
        response_code=401,
        api_key=KnownMetricsBase.TEST_API_KEY,
        api_key_valid=True,
    )
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(int64Value=1)])


class TestConsumerErrorCountInvalidApiKey(KnownMetricsInvalidApiKey,
                                          unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_ERROR_COUNT


class TestProducerErrorCountNoError(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.PRODUCER_ERROR_COUNT


class TestProducerErrorCountWithErrors(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.PRODUCER_ERROR_COUNT
    GIVEN_INFO = report_request.Info(
        response_code=401,
        api_key=KnownMetricsBase.TEST_API_KEY,
        api_key_valid=True,
    )
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(int64Value=1)])


class TestConsumerTotalLatencies(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_TOTAL_LATENCIES
    WANTED_DISTRIBUTION = _wanted_distribution_with_sample(
        KnownMetricsBase.GIVEN_INFO.request_time.seconds,
        *metric_descriptor._TIME_DISTRIBUTION_ARGS)
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(distributionValue=WANTED_DISTRIBUTION)])


class TestConsumerTotalLatenciesInvalidApiKey(KnownMetricsInvalidApiKey,
                                              unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_TOTAL_LATENCIES


class TestProducerTotalLatencies(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.PRODUCER_TOTAL_LATENCIES
    WANTED_DISTRIBUTION = _wanted_distribution_with_sample(
        KnownMetricsBase.GIVEN_INFO.request_time.seconds,
        *metric_descriptor._TIME_DISTRIBUTION_ARGS)
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(distributionValue=WANTED_DISTRIBUTION)])


class TestConsumerRequestOverheadLatencies(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_REQUEST_OVERHEAD_LATENCIES
    WANTED_DISTRIBUTION = _wanted_distribution_with_sample(
        KnownMetricsBase.GIVEN_INFO.overhead_time.seconds,
        *metric_descriptor._TIME_DISTRIBUTION_ARGS)
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(distributionValue=WANTED_DISTRIBUTION)])


class TestConsumerRequestOverheadLatenciesInvalidApiKey(
    KnownMetricsInvalidApiKey, unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_REQUEST_OVERHEAD_LATENCIES


class TestProducerRequestOverheadLatencies(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.PRODUCER_REQUEST_OVERHEAD_LATENCIES
    WANTED_DISTRIBUTION = _wanted_distribution_with_sample(
        KnownMetricsBase.GIVEN_INFO.overhead_time.seconds,
        *metric_descriptor._TIME_DISTRIBUTION_ARGS)
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(distributionValue=WANTED_DISTRIBUTION)])


class TestConsumerBackendLatencies(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_BACKEND_LATENCIES
    WANTED_DISTRIBUTION = _wanted_distribution_with_sample(
        KnownMetricsBase.GIVEN_INFO.backend_time.seconds,
        *metric_descriptor._TIME_DISTRIBUTION_ARGS)
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(distributionValue=WANTED_DISTRIBUTION)])


class TestConsumerBackendLatenciesInvalidApiKey(KnownMetricsInvalidApiKey,
                                                unittest2.TestCase):
    SUBJECT = _KNOWN.CONSUMER_BACKEND_LATENCIES


class TestProducerBackendLatencies(KnownMetricsBase, unittest2.TestCase):
    SUBJECT = _KNOWN.PRODUCER_BACKEND_LATENCIES
    WANTED_DISTRIBUTION = _wanted_distribution_with_sample(
        KnownMetricsBase.GIVEN_INFO.backend_time.seconds,
        *metric_descriptor._TIME_DISTRIBUTION_ARGS)
    WANTED_ADDED_METRICS = messages.MetricValueSet(
        metricName=SUBJECT.metric_name,
        metricValues=[metric_value.create(distributionValue=WANTED_DISTRIBUTION)])
