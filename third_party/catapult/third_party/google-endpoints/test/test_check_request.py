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
import httplib
import unittest2
from expects import be_none, equal, expect, raise_error

from apitools.base.py import encoding

from google.api.control import caches, label_descriptor, timestamp
from google.api.control import check_request, messages, metric_value


class TestSign(unittest2.TestCase):

    def setUp(self):
        op = messages.Operation(
            consumerId=_TEST_CONSUMER_ID,
            operationName=_TEST_OP_NAME
        )
        self.test_check_request = messages.CheckRequest(operation=op)
        self.test_op = op

    def test_should_fail_if_operation_is_not_set(self):
        testf = lambda: check_request.sign(messages.CheckRequest())
        expect(testf).to(raise_error(ValueError))

    def test_should_fail_on_invalid_input(self):
        testf = lambda: check_request.sign(None)
        expect(testf).to(raise_error(ValueError))
        testf = lambda: check_request.sign(object())
        expect(testf).to(raise_error(ValueError))

    def test_should_fail_if_operation_has_no_operation_name(self):
        op = messages.Operation(consumerId=_TEST_CONSUMER_ID)
        testf = lambda: check_request.sign(messages.CheckRequest(operation=op))
        expect(testf).to(raise_error(ValueError))

    def test_should_fail_if_operation_has_no_consumer_id(self):
        op = messages.Operation(operationName=_TEST_OP_NAME)
        testf = lambda: check_request.sign(messages.CheckRequest(operation=op))
        expect(testf).to(raise_error(ValueError))

    def test_should_sign_a_valid_check_request(self):
        check_request.sign(self.test_check_request)

    def test_should_change_signature_when_labels_are_added(self):
        without_labels = check_request.sign(self.test_check_request)
        self.test_op.labels = encoding.PyValueToMessage(
            messages.Operation.LabelsValue, {
                'key1': 'value1',
                'key2': 'value2'
            })
        with_labels = check_request.sign(self.test_check_request)
        expect(with_labels).not_to(equal(without_labels))

    def test_should_change_signature_when_metric_values_are_added(self):
        without_mvs = check_request.sign(self.test_check_request)
        self.test_op.metricValueSets = [
            messages.MetricValueSet(
                metricName='a_float',
                metricValues=[
                    metric_value.create(
                        labels={
                            'key1': 'value1',
                            'key2': 'value2'
                        },
                        doubleValue=1.1,
                    ),
                ]
            )
        ]
        with_mvs = check_request.sign(self.test_check_request)
        expect(with_mvs).not_to(equal(without_mvs))

    def test_should_change_signature_quota_properties_are_specified(self):
        without_qprops = check_request.sign(self.test_check_request)
        self.test_op.quotaProperties = messages.QuotaProperties()
        with_qprops = check_request.sign(self.test_check_request)
        expect(with_qprops).not_to(equal(without_qprops))


class TestAggregatorCheck(unittest2.TestCase):
    SERVICE_NAME = 'service.check'
    FAKE_OPERATION_ID = 'service.general.check'

    def setUp(self):
        self.timer = _DateTimeTimer()
        self.agg = check_request.Aggregator(
            self.SERVICE_NAME, caches.CheckOptions())

    def test_should_fail_if_req_is_bad(self):
        testf = lambda: self.agg.check(object())
        expect(testf).to(raise_error(ValueError))
        testf = lambda: self.agg.check(None)
        expect(testf).to(raise_error(ValueError))

    def test_should_fail_if_service_name_does_not_match(self):
        req = _make_test_request(self.SERVICE_NAME + '-will-not-match')
        testf = lambda: self.agg.check(req)
        expect(testf).to(raise_error(ValueError))

    def test_should_fail_if_check_request_is_missing(self):
        req = messages.ServicecontrolServicesCheckRequest(
            serviceName=self.SERVICE_NAME)
        testf = lambda: self.agg.check(req)
        expect(testf).to(raise_error(ValueError))

    def test_should_fail_if_operation_is_missing(self):
        req = messages.ServicecontrolServicesCheckRequest(
            serviceName=self.SERVICE_NAME,
            checkRequest=messages.CheckRequest())
        testf = lambda: self.agg.check(req)
        expect(testf).to(raise_error(ValueError))

    def test_should_return_none_initially_as_req_is_not_cached(self):
        req = _make_test_request(self.SERVICE_NAME)
        fake_response = messages.CheckResponse(
            operationId=self.FAKE_OPERATION_ID)
        agg = self.agg
        expect(agg.check(req)).to(be_none)


class TestAggregatorThatCannotCache(unittest2.TestCase):
    SERVICE_NAME = 'service.no_cache'
    FAKE_OPERATION_ID = 'service.no_cache.op_id'

    def setUp(self):
        # -ve num_entries means no cache is present
        self.agg = check_request.Aggregator(
            self.SERVICE_NAME,
            caches.CheckOptions(num_entries=-1))

    def test_should_not_cache_responses(self):
        req = _make_test_request(self.SERVICE_NAME)
        fake_response = messages.CheckResponse(
            operationId=self.FAKE_OPERATION_ID)
        agg = self.agg
        expect(agg.check(req)).to(be_none)
        agg.add_response(req, fake_response)
        expect(agg.check(req)).to(be_none)
        agg.clear()
        expect(agg.check(req)).to(be_none)

    def test_should_have_empty_flush_response(self):
        expect(len(self.agg.flush())).to(equal(0))

    def test_should_have_none_as_flush_interval(self):
        expect(self.agg.flush_interval).to(be_none)



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


class TestCachingAggregator(unittest2.TestCase):
    SERVICE_NAME = 'service.with_cache'
    FAKE_OPERATION_ID = 'service.with_cache.op_id'

    def setUp(self):
        self.timer = _DateTimeTimer()
        self.expiration = datetime.timedelta(seconds=2)
        options = caches.CheckOptions(
            flush_interval=datetime.timedelta(seconds=1),
            expiration=self.expiration)
        self.agg = check_request.Aggregator(
            self.SERVICE_NAME, options, timer=self.timer)

    def test_should_have_expiration_as_flush_interval(self):
        expect(self.agg.flush_interval).to(equal(self.expiration))

    def test_should_cache_responses(self):
        req = _make_test_request(self.SERVICE_NAME)
        fake_response = messages.CheckResponse(
            operationId=self.FAKE_OPERATION_ID)
        agg = self.agg
        expect(agg.check(req)).to(be_none)
        agg.add_response(req, fake_response)
        expect(agg.check(req)).to(equal(fake_response))

    def test_should_not_cache_requests_with_important_operations(self):
        req = _make_test_request(
            self.SERVICE_NAME,
            importance=messages.Operation.ImportanceValueValuesEnum.HIGH)
        fake_response = messages.CheckResponse(
            operationId=self.FAKE_OPERATION_ID)
        agg = self.agg
        expect(agg.check(req)).to(be_none)
        agg.add_response(req, fake_response)
        expect(agg.check(req)).to(be_none)

    def test_signals_a_resend_on_1st_call_after_flush_interval(self):
        req = _make_test_request(self.SERVICE_NAME)
        fake_response = messages.CheckResponse(
            operationId=self.FAKE_OPERATION_ID)
        agg = self.agg
        expect(agg.check(req)).to(be_none)
        agg.add_response(req, fake_response)
        expect(agg.check(req)).to(equal(fake_response))

        # Now flush interval is reached, but not the response expiry
        self.timer.tick() # now past the flush_interval
        expect(agg.check(req)).to(be_none)  # none signals the resend

        # Until expiry, the response will continue to be returned
        expect(agg.check(req)).to(equal(fake_response))
        expect(agg.check(req)).to(equal(fake_response))

        # Once expired the cached response is no longer returned
        # expire
        self.timer.tick()
        self.timer.tick() # now expired
        expect(agg.check(req)).to(be_none)
        expect(agg.check(req)).to(be_none)  # 2nd check is None as well

    def test_signals_resend_on_1st_call_after_flush_interval_with_errors(self):
        req = _make_test_request(self.SERVICE_NAME)
        failure_code = messages.CheckError.CodeValueValuesEnum.NOT_FOUND
        fake_response = messages.CheckResponse(
            operationId=self.FAKE_OPERATION_ID, checkErrors=[
                messages.CheckError(code=failure_code)
            ])
        agg = self.agg
        expect(agg.check(req)).to(be_none)
        agg.add_response(req, fake_response)
        expect(agg.check(req)).to(equal(fake_response))

        # Now flush interval is reached, but not the response expiry
        self.timer.tick() # now past the flush_interval
        expect(agg.check(req)).to(be_none)  # first response is null

        # until expiry, the response will continue to be returned
        expect(agg.check(req)).to(equal(fake_response))
        expect(agg.check(req)).to(equal(fake_response))

        # expire
        self.timer.tick()
        self.timer.tick() # now expired
        expect(agg.check(req)).to(be_none)
        expect(agg.check(req)).to(be_none) # 2nd check is None as well

    def test_should_extend_expiration_on_receipt_of_a_response(self):
        req = _make_test_request(self.SERVICE_NAME)
        fake_response = messages.CheckResponse(
            operationId=self.FAKE_OPERATION_ID
        )
        agg = self.agg
        expect(agg.check(req)).to(be_none)
        agg.add_response(req, fake_response)
        expect(agg.check(req)).to(equal(fake_response))

        # Now flush interval is reached, but not the response expiry
        self.timer.tick() # now past the flush_interval
        expect(agg.check(req)).to(be_none)  # first response is null

        # until expiry, the response will continue to be returned
        expect(agg.check(req)).to(equal(fake_response))
        expect(agg.check(req)).to(equal(fake_response))

        # add a response as the request expires
        self.timer.tick()
        agg.add_response(req, fake_response)
        # it would have expired, but because the response was added it does not
        expect(agg.check(req)).to(equal(fake_response))
        expect(agg.check(req)).to(equal(fake_response))
        self.timer.tick() # now past the flush interval again
        expect(agg.check(req)).to(be_none)
        expect(agg.check(req)).to(equal(fake_response))

    def test_does_not_flush_request_that_has_not_been_updated(self):
        req = _make_test_request(self.SERVICE_NAME)
        fake_response = messages.CheckResponse(
            operationId=self.FAKE_OPERATION_ID
        )
        agg = self.agg
        expect(agg.check(req)).to(be_none)
        agg.add_response(req, fake_response)
        self.timer.tick() # now past the flush_interval
        expect(len(agg.flush())).to(equal(0)) # nothing expired
        self.timer.tick() # now past expiry
        self.timer.tick() # now past expiry
        expect(agg.check(req)).to(be_none)  # confirm nothing in cache
        expect(agg.check(req)).to(be_none)  # confirm nothing in cache
        expect(len(agg.flush())).to(equal(0)) # no cached check request

    def test_does_flush_requests_that_have_been_updated(self):
        req = _make_test_request(self.SERVICE_NAME)
        fake_response = messages.CheckResponse(
            operationId=self.FAKE_OPERATION_ID
        )
        agg = self.agg
        expect(agg.check(req)).to(be_none)
        agg.add_response(req, fake_response)
        expect(agg.check(req)).to(equal(fake_response))
        self.timer.tick() # now past the flush_interval
        expect(len(agg.flush())).to(equal(0)) # nothing expired
        self.timer.tick() # now past expiry
        self.timer.tick() # now past expiry
        expect(len(agg.flush())).to(equal(1)) # got the cached check request

    def test_should_clear_requests(self):
        req = _make_test_request(self.SERVICE_NAME)
        fake_response = messages.CheckResponse(
            operationId=self.FAKE_OPERATION_ID
        )
        agg = self.agg
        expect(agg.check(req)).to(be_none)
        agg.add_response(req, fake_response)
        expect(agg.check(req)).to(equal(fake_response))
        agg.clear()
        expect(agg.check(req)).to(be_none)
        expect(len(agg.flush())).to(equal(0))


_TEST_CONSUMER_ID = 'testConsumerID'
_TEST_OP_NAME = 'testOperationName'


def _make_test_request(service_name, importance=None):
    if importance is None:
        importance = messages.Operation.ImportanceValueValuesEnum.LOW
    op = messages.Operation(
        consumerId=_TEST_CONSUMER_ID,
        operationName=_TEST_OP_NAME,
        importance=importance
    )
    check_request = messages.CheckRequest(operation=op)
    return messages.ServicecontrolServicesCheckRequest(
        serviceName=service_name,
        checkRequest=check_request)


_WANTED_USER_AGENT = label_descriptor.USER_AGENT
_START_OF_EPOCH = timestamp.to_rfc3339(datetime.datetime(1970, 1, 1, 0, 0, 0))
_TEST_SERVICE_NAME = 'a_service_name'
_INFO_TESTS = [
    (check_request.Info(
        operation_id='an_op_id',
        operation_name='an_op_name',
        referer='a_referer',
        service_name=_TEST_SERVICE_NAME),
     messages.Operation(
         importance=messages.Operation.ImportanceValueValuesEnum.LOW,
         labels = encoding.PyValueToMessage(
             messages.Operation.LabelsValue, {
                 'servicecontrol.googleapis.com/user_agent': _WANTED_USER_AGENT,
                 'servicecontrol.googleapis.com/referer': 'a_referer'
             }),
         operationId='an_op_id',
         operationName='an_op_name',
         startTime=_START_OF_EPOCH,
         endTime=_START_OF_EPOCH)),
    (check_request.Info(
        api_key='an_api_key',
        api_key_valid=True,
        operation_id='an_op_id',
        operation_name='an_op_name',
        referer='a_referer',
        service_name=_TEST_SERVICE_NAME),
     messages.Operation(
         importance=messages.Operation.ImportanceValueValuesEnum.LOW,
         consumerId='api_key:an_api_key',
         labels = encoding.PyValueToMessage(
             messages.Operation.LabelsValue, {
                 'servicecontrol.googleapis.com/user_agent': _WANTED_USER_AGENT,
                 'servicecontrol.googleapis.com/referer': 'a_referer'
             }),
         operationId='an_op_id',
         operationName='an_op_name',
         startTime=_START_OF_EPOCH,
         endTime=_START_OF_EPOCH)),
    (check_request.Info(
        api_key='an_api_key',
        api_key_valid=False,
        client_ip='127.0.0.1',
        consumer_project_id='project_id',
        operation_id='an_op_id',
        operation_name='an_op_name',
        referer='a_referer',
        service_name=_TEST_SERVICE_NAME),
     messages.Operation(
         importance=messages.Operation.ImportanceValueValuesEnum.LOW,
         consumerId='project:project_id',
         labels = encoding.PyValueToMessage(
             messages.Operation.LabelsValue, {
                 'servicecontrol.googleapis.com/caller_ip': '127.0.0.1',
                 'servicecontrol.googleapis.com/user_agent': _WANTED_USER_AGENT,
                 'servicecontrol.googleapis.com/referer': 'a_referer'
             }),
         operationId='an_op_id',
         operationName='an_op_name',
         startTime=_START_OF_EPOCH,
         endTime=_START_OF_EPOCH)),
]
_INCOMPLETE_INFO_TESTS = [
    check_request.Info(
        operation_name='an_op_name',
        service_name=_TEST_SERVICE_NAME),
    check_request.Info(
        operation_id='an_op_id',
        service_name=_TEST_SERVICE_NAME),
    check_request.Info(
        operation_id='an_op_id',
        operation_name='an_op_name')
]


class TestInfo(unittest2.TestCase):

    def test_should_construct_with_no_args(self):
        expect(check_request.Info()).not_to(be_none)

    def test_should_convert_using_as_check_request(self):
        timer = _DateTimeTimer()
        for info, want in _INFO_TESTS:
            got = info.as_check_request(timer=timer)
            expect(got.checkRequest.operation).to(equal(want))
            expect(got.serviceName).to(equal(_TEST_SERVICE_NAME))

    def test_should_fail_as_check_request_on_incomplete_info(self):
        timer = _DateTimeTimer()
        for info in _INCOMPLETE_INFO_TESTS:
            testf = lambda: info.as_check_request(timer=timer)
            expect(testf).to(raise_error(ValueError))


class TestConvertResponse(unittest2.TestCase):
    PROJECT_ID = 'test_convert_response'

    def test_should_be_ok_with_no_errors(self):
        code, message, _ = check_request.convert_response(
            messages.CheckResponse(), self.PROJECT_ID)
        expect(code).to(equal(httplib.OK))
        expect(message).to(equal(''))

    def test_should_include_project_id_in_error_text_when_needed(self):
        resp = messages.CheckResponse(
            checkErrors = [
                messages.CheckError(
                    code=messages.CheckError.CodeValueValuesEnum.PROJECT_DELETED)
            ]
        )
        code, got, _ = check_request.convert_response(resp, self.PROJECT_ID)
        want = 'Project %s has been deleted' % (self.PROJECT_ID,)
        expect(code).to(equal(httplib.FORBIDDEN))
        expect(got).to(equal(want))

    def test_should_include_detail_in_error_text_when_needed(self):
        detail = 'details, details, details'
        resp = messages.CheckResponse(
            checkErrors = [
                messages.CheckError(
                    code=messages.CheckError.CodeValueValuesEnum.IP_ADDRESS_BLOCKED,
                    detail=detail)
            ]
        )
        code, got, _ = check_request.convert_response(resp, self.PROJECT_ID)
        expect(code).to(equal(httplib.FORBIDDEN))
        expect(got).to(equal(detail))


class _DateTimeTimer(object):
    def __init__(self, auto=False):
        self.auto = auto
        self.time = datetime.datetime(1970, 1, 1)

    def __call__(self):
        if self.auto:
            self.tick()
        return self.time

    def tick(self):
        self.time += datetime.timedelta(seconds=1)
