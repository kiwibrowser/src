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
from expects import be_none, expect, equal, raise_error

from google.api.control import messages, metric_value, operation, timestamp
from google.api.control import MetricKind

_A_FLOAT_VALUE = 1.1
_REALLY_EARLY = timestamp.to_rfc3339(datetime.datetime(1970, 1, 1, 0, 0, 0))
_EARLY = timestamp.to_rfc3339(datetime.datetime(1980, 1, 1, 10, 0, 0))
_LATER = timestamp.to_rfc3339(datetime.datetime(1980, 2, 2, 10, 0, 0))
_LATER_STILL = timestamp.to_rfc3339(datetime.datetime(1981, 2, 2, 10, 0, 0))

_TEST_LABELS = {
    'key1': 'value1',
    'key2': 'value2',
}

# in tests, the description field is not currently used, but should be filled
_TESTS = [
    {
        'description': 'update the start time to that of the earliest',
        'kinds': None,
        'initial': messages.Operation(
            startTime=_EARLY,
            endTime=_LATER
        ),
        'ops': [
            messages.Operation(
                startTime=_REALLY_EARLY,
                endTime=_LATER
            ),
            messages.Operation(
                startTime=_LATER,
                endTime=_LATER
            ),
        ],
        'want': messages.Operation(startTime=_REALLY_EARLY, endTime=_LATER)
    },
    {
        'description': 'update the end time to that of the latest',
        'kinds': None,
        'initial': messages.Operation(
            startTime=_EARLY,
            endTime=_LATER
        ),
        'ops': [
            messages.Operation(
                startTime=_EARLY,
                endTime=_LATER
            ),
            messages.Operation(
                startTime=_EARLY,
                endTime=_LATER_STILL
            ),
        ],
        'want': messages.Operation(startTime=_EARLY, endTime=_LATER_STILL)
    },
    {
        'description': 'combine the log entries',
        'kinds': None,
        'initial': messages.Operation(
            startTime=_EARLY,
            endTime=_LATER,
            logEntries=[messages.LogEntry(textPayload='initial')]
        ),
        'ops': [
            messages.Operation(
                startTime=_EARLY,
                endTime=_LATER,
                logEntries=[messages.LogEntry(textPayload='agg1')]
            ),
            messages.Operation(
                startTime=_EARLY,
                endTime=_LATER,
                logEntries=[messages.LogEntry(textPayload='agg2')]
            ),
        ],
        'want': messages.Operation(
            startTime=_EARLY,
            endTime=_LATER,
            logEntries=[
                messages.LogEntry(textPayload='initial'),
                messages.LogEntry(textPayload='agg1'),
                messages.LogEntry(textPayload='agg2')
            ]
        )
    },
    {
        'description': 'combines the metric value using the default kind',
        'kinds': None,
        'initial': messages.Operation(
            startTime=_EARLY,
            endTime=_LATER,
            metricValueSets = [
                messages.MetricValueSet(
                    metricName='some_floats',
                    metricValues=[
                        metric_value.create(
                            labels=_TEST_LABELS,
                            doubleValue=_A_FLOAT_VALUE,
                            endTime=_EARLY
                        ),
                    ]
                ),
                messages.MetricValueSet(
                    metricName='other_floats',
                    metricValues=[
                        metric_value.create(
                            labels=_TEST_LABELS,
                            doubleValue=_A_FLOAT_VALUE,
                            endTime=_EARLY
                        ),
                    ]
                )
            ]
        ),
        'ops': [
            messages.Operation(
                startTime=_EARLY,
                endTime=_LATER,
                metricValueSets = [
                    messages.MetricValueSet(
                        metricName='some_floats',
                        metricValues=[
                            metric_value.create(
                                labels=_TEST_LABELS,
                                doubleValue=_A_FLOAT_VALUE,
                                endTime=_LATER
                            ),
                        ]
                    ),
                ]
            ),
            messages.Operation(
                startTime=_EARLY,
                endTime=_LATER,
                metricValueSets = [
                    messages.MetricValueSet(
                        metricName='other_floats',
                        metricValues=[
                            metric_value.create(
                                labels=_TEST_LABELS,
                                doubleValue=_A_FLOAT_VALUE,
                                endTime=_LATER_STILL
                            ),
                        ]
                    )
                ]

            ),
        ],
        'want': messages.Operation(
            startTime=_EARLY,
            endTime=_LATER,
            metricValueSets = [
                messages.MetricValueSet(
                    metricName='other_floats',
                    metricValues=[
                        metric_value.create(
                            labels=_TEST_LABELS,
                            doubleValue=_A_FLOAT_VALUE * 2,
                            endTime=_LATER_STILL
                        ),
                    ]
                ),
                messages.MetricValueSet(
                    metricName='some_floats',
                    metricValues=[
                        metric_value.create(
                            labels=_TEST_LABELS,
                            doubleValue=_A_FLOAT_VALUE * 2,
                            endTime=_LATER
                        ),
                    ]
                )
            ]
        )
    },
    {
        'description': 'combines a metric value using a kind that is not DELTA',
        'kinds': { 'some_floats': MetricKind.GAUGE },
        'initial': messages.Operation(
            startTime=_EARLY,
            endTime=_LATER,
            metricValueSets = [
                messages.MetricValueSet(
                    metricName='some_floats',
                    metricValues=[
                        metric_value.create(
                            labels=_TEST_LABELS,
                            doubleValue=_A_FLOAT_VALUE,
                            endTime=_EARLY
                        ),
                    ]
                ),
                messages.MetricValueSet(
                    metricName='other_floats',
                    metricValues=[
                        metric_value.create(
                            labels=_TEST_LABELS,
                            doubleValue=_A_FLOAT_VALUE,
                            endTime=_EARLY
                        ),
                    ]
                )
            ]
        ),
        'ops': [
            messages.Operation(
                startTime=_EARLY,
                endTime=_LATER,
                metricValueSets = [
                    messages.MetricValueSet(
                        metricName='some_floats',
                        metricValues=[
                            metric_value.create(
                                labels=_TEST_LABELS,
                                doubleValue=_A_FLOAT_VALUE,
                                endTime=_LATER
                            ),
                        ]
                    ),
                ]
            ),
            messages.Operation(
                startTime=_EARLY,
                endTime=_LATER,
                metricValueSets = [
                    messages.MetricValueSet(
                        metricName='other_floats',
                        metricValues=[
                            metric_value.create(
                                labels=_TEST_LABELS,
                                doubleValue=_A_FLOAT_VALUE,
                                endTime=_LATER_STILL
                            ),
                        ]
                    )
                ]

            ),
        ],
        'want': messages.Operation(
            startTime=_EARLY,
            endTime=_LATER,
            metricValueSets = [
                messages.MetricValueSet(
                    metricName='other_floats',
                    metricValues=[
                        metric_value.create(
                            labels=_TEST_LABELS,
                            doubleValue=_A_FLOAT_VALUE * 2,
                            endTime=_LATER_STILL
                        ),
                    ]
                ),
                messages.MetricValueSet(
                    metricName='some_floats',
                    metricValues=[
                        metric_value.create(
                            labels=_TEST_LABELS,
                            doubleValue=_A_FLOAT_VALUE,
                            endTime=_LATER
                        ),
                    ]
                )
            ]
        )
    }
]

class TestOperationAggregation(unittest2.TestCase):

    def test_should_aggregate_as_expected(self):
        for t in _TESTS:
            desc = t['description']
            initial = t['initial']
            want = t['want']
            agg = operation.Aggregator(initial, kinds=t['kinds'])
            for o in t['ops']:
                agg.add(o)
                got = agg.as_operation()
            try:
                expect(got).to(equal(want))
            except AssertionError as e:
                raise AssertionError('Failed to {0}\n{1}'.format(desc, e))


_INFO_TESTS = [
    (operation.Info(
        referer='a_referer',
        service_name='a_service_name'),
     messages.Operation(
         importance=messages.Operation.ImportanceValueValuesEnum.LOW,
         startTime=_REALLY_EARLY,
         endTime=_REALLY_EARLY)),
    (operation.Info(
        operation_id='an_op_id',
        referer='a_referer',
        service_name='a_service_name'),
     messages.Operation(
         importance=messages.Operation.ImportanceValueValuesEnum.LOW,
         operationId='an_op_id',
         startTime=_REALLY_EARLY,
         endTime=_REALLY_EARLY)),
    (operation.Info(
        operation_id='an_op_id',
        operation_name='an_op_name',
        referer='a_referer',
        service_name='a_service_name'),
     messages.Operation(
         importance=messages.Operation.ImportanceValueValuesEnum.LOW,
         operationId='an_op_id',
         operationName='an_op_name',
         startTime=_REALLY_EARLY,
         endTime=_REALLY_EARLY)),
    (operation.Info(
        api_key='an_api_key',
        api_key_valid=True,
        operation_id='an_op_id',
        operation_name='an_op_name',
        referer='a_referer',
        service_name='a_service_name'),
     messages.Operation(
         importance=messages.Operation.ImportanceValueValuesEnum.LOW,
         consumerId='api_key:an_api_key',
         operationId='an_op_id',
         operationName='an_op_name',
         startTime=_REALLY_EARLY,
         endTime=_REALLY_EARLY)),
    (operation.Info(
        api_key='an_api_key',
        api_key_valid=False,
        consumer_project_id='project_id',
        operation_id='an_op_id',
        operation_name='an_op_name',
        referer='a_referer',
        service_name='a_service_name'),
     messages.Operation(
         importance=messages.Operation.ImportanceValueValuesEnum.LOW,
         consumerId='project:project_id',
         operationId='an_op_id',
         operationName='an_op_name',
         startTime=_REALLY_EARLY,
         endTime=_REALLY_EARLY)),
]

class TestInfo(unittest2.TestCase):

    def test_should_construct_with_no_args(self):
        expect(operation.Info()).not_to(be_none)

    def test_should_convert_to_operation_ok(self):
        timer = _DateTimeTimer()
        for info, want in _INFO_TESTS:
            expect(info.as_operation(timer=timer)).to(equal(want))


class _DateTimeTimer(object):
    def __init__(self, auto=False):
        self.auto = auto
        self.time = datetime.datetime.utcfromtimestamp(0)

    def __call__(self):
        if self.auto:
            self.tick()
        return self.time

    def tick(self):
        self.time += datetime.timedelta(seconds=1)
