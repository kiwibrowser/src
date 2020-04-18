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
import mock
import os
import tempfile
import unittest2
from expects import be_false, be_none, be_true, expect, equal, raise_error

from google.api.control import caches, check_request, client, messages, report_request


class TestSimpleLoader(unittest2.TestCase):
    SERVICE_NAME = 'simpler-loader'

    @mock.patch("google.api.control.client.ReportOptions", autospec=True)
    @mock.patch("google.api.control.client.CheckOptions", autospec=True)
    def test_should_create_client_ok(self, check_opts, report_opts):
        # the mocks return fake instances else code using them fails
        check_opts.return_value = caches.CheckOptions()
        report_opts.return_value = caches.ReportOptions()

        # ensure the client is constructed using no args instances of the opts
        expect(client.Loaders.DEFAULT.load(self.SERVICE_NAME)).not_to(be_none)
        check_opts.assert_called_once_with()
        report_opts.assert_called_once_with()

_TEST_CONFIG = """{
    "checkAggregatorConfig": {
       "cacheEntries": 10,
       "responseExpirationMs": 1000,
       "flushIntervalMs": 2000
    },
    "reportAggregatorConfig": {
       "cacheEntries": 10,
       "flushIntervalMs": 1000
    }
}
"""


class TestEnvironmentLoader(unittest2.TestCase):
    SERVICE_NAME = 'environment-loader'

    def setUp(self):
        json_fd = tempfile.NamedTemporaryFile(delete=False)
        with json_fd as f:
            f.write(_TEST_CONFIG)
        self._config_file = json_fd.name
        os.environ[client.CONFIG_VAR] = self._config_file

    def tearDown(self):
        if os.path.exists(self._config_file):
            os.remove(self._config_file)

    @mock.patch("google.api.control.client.ReportOptions", autospec=True)
    @mock.patch("google.api.control.client.CheckOptions", autospec=True)
    def test_should_create_client_from_environment_ok(self, check_opts, report_opts):
        check_opts.return_value = caches.CheckOptions()
        report_opts.return_value = caches.ReportOptions()

        # ensure the client is constructed using options values from the test JSON
        expect(client.Loaders.ENVIRONMENT.load(self.SERVICE_NAME)).not_to(be_none)
        check_opts.assert_called_once_with(expiration=datetime.timedelta(0, 1),
                                           flush_interval=datetime.timedelta(0, 2),
                                           num_entries=10)
        report_opts.assert_called_once_with(flush_interval=datetime.timedelta(0, 1),
                                            num_entries=10)

    @mock.patch("google.api.control.client.ReportOptions", autospec=True)
    @mock.patch("google.api.control.client.CheckOptions", autospec=True)
    def test_should_use_defaults_if_file_is_missing(self, check_opts, report_opts):
        os.remove(self._config_file)
        self._assert_called_with_no_args_options(check_opts, report_opts)

    @mock.patch("google.api.control.client.ReportOptions", autospec=True)
    @mock.patch("google.api.control.client.CheckOptions", autospec=True)
    def test_should_use_defaults_if_file_is_missing(self, check_opts, report_opts):
        del os.environ[client.CONFIG_VAR]
        self._assert_called_with_no_args_options(check_opts, report_opts)

    @mock.patch("google.api.control.client.ReportOptions")
    @mock.patch("google.api.control.client.CheckOptions")
    def test_should_use_defaults_if_json_is_bad(self, check_opts, report_opts):
        with open(self._config_file, 'w') as f:
            f.write(_TEST_CONFIG + '\n{ this will not parse as json}')
        self._assert_called_with_no_args_options(check_opts, report_opts)

    def _assert_called_with_no_args_options(self, check_opts, report_opts):
        # the mocks return fake instances else code using them fails
        check_opts.return_value = caches.CheckOptions()
        report_opts.return_value = caches.ReportOptions()

        # ensure the client is constructed using no args instances of the opts
        expect(client.Loaders.ENVIRONMENT.load(self.SERVICE_NAME)).not_to(be_none)
        check_opts.assert_called_once_with()
        report_opts.assert_called_once_with()


def _make_dummy_report_request(project_id, service_name):
    rules = report_request.ReportingRules()
    info = report_request.Info(
        consumer_project_id=project_id,
        operation_id='an_op_id',
        operation_name='an_op_name',
        method='GET',
        referer='a_referer',
        service_name=service_name)
    return info.as_report_request(rules)


def _make_dummy_check_request(project_id, service_name):
    info = check_request.Info(
        consumer_project_id=project_id,
        operation_id='an_op_id',
        operation_name='an_op_name',
        referer='a_referer',
        service_name=service_name)
    return info.as_check_request()


class TestClientStartAndStop(unittest2.TestCase):
    SERVICE_NAME = 'start-and-stop'
    PROJECT_ID = SERVICE_NAME + '.project'

    def setUp(self):
        self._mock_transport = mock.MagicMock()
        self._subject = client.Loaders.DEFAULT.load(
            self.SERVICE_NAME,
            create_transport=lambda: self._mock_transport)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_create_a_thread_when_started(self, thread_class):
        self._subject.start()
        expect(thread_class.called).to(be_true)
        expect(len(thread_class.call_args_list)).to(equal(1))

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_only_create_thread_on_first_start(self, thread_class):
        self._subject.start()
        self._subject.start()
        expect(len(thread_class.call_args_list)).to(equal(1))

    def test_should_noop_stop_if_not_started(self):
        # stop the subject, the transport should not see a request
        self._subject.stop()
        expect(self._mock_transport.called).to(be_false)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_clear_requests_on_stop(self, dummy_thread_class):
        # stop the subject, the transport did not see a request
        self._subject.start()
        self._subject.report(
            _make_dummy_report_request(self.PROJECT_ID, self.SERVICE_NAME))
        self._subject.stop()
        expect(self._mock_transport.services.report.called).to(be_true)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_ignore_stop_if_already_stopped(self, dummy_thread_class):
        # stop the subject, the transport did not see a request
        self._subject.start()
        self._subject.report(
            _make_dummy_report_request(self.PROJECT_ID, self.SERVICE_NAME))
        self._subject.stop()
        self._mock_transport.reset_mock()
        self._subject.stop()
        expect(self._mock_transport.services.report.called).to(be_false)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_ignore_bad_transport_when_not_cached(self, dummy_thread_class):
        self._subject.start()
        self._subject.report(
            _make_dummy_report_request(self.PROJECT_ID, self.SERVICE_NAME))
        self._mock_transport.services.report.side_effect = lambda: 1/0
        self._subject.stop()
        expect(self._mock_transport.services.report.called).to(be_true)


class TestClientCheck(unittest2.TestCase):
    SERVICE_NAME = 'check'
    PROJECT_ID = SERVICE_NAME + '.project'

    def setUp(self):
        self._mock_transport = mock.MagicMock()
        self._subject = client.Loaders.DEFAULT.load(
            self.SERVICE_NAME,
            create_transport=lambda: self._mock_transport)

    def test_should_raise_on_check_without_start(self):
        dummy_request = _make_dummy_check_request(self.PROJECT_ID,
                                                  self.SERVICE_NAME)
        expect(lambda: self._subject.check(dummy_request)).to(
            raise_error(AssertionError))

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_send_the_request_if_not_cached(self, dummy_thread_class):
        self._subject.start()
        dummy_request = _make_dummy_check_request(self.PROJECT_ID,
                                                  self.SERVICE_NAME)
        self._subject.check(dummy_request)
        expect(self._mock_transport.services.check.called).to(be_true)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_not_send_the_request_if_cached(self, dummy_thread_class):
        t = self._mock_transport
        self._subject.start()
        dummy_request = _make_dummy_check_request(self.PROJECT_ID,
                                                  self.SERVICE_NAME)
        dummy_response = messages.CheckResponse(
            operationId=dummy_request.checkRequest.operation.operationId)
        t.services.check.return_value = dummy_response
        expect(self._subject.check(dummy_request)).to(equal(dummy_response))
        t.reset_mock()
        expect(self._subject.check(dummy_request)).to(equal(dummy_response))
        expect(t.services.check.called).to(be_false)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_return_null_if_transport_fails(self, dummy_thread_class):
        self._subject.start()
        dummy_request = _make_dummy_check_request(self.PROJECT_ID,
                                                  self.SERVICE_NAME)
        self._mock_transport.services.check.side_effect = lambda: 1/0
        expect(self._subject.check(dummy_request)).to(be_none)


class TestClientReport(unittest2.TestCase):
    SERVICE_NAME = 'report'
    PROJECT_ID = SERVICE_NAME + '.project'

    def setUp(self):
        self._mock_transport = mock.MagicMock()
        self._subject = client.Loaders.DEFAULT.load(
            self.SERVICE_NAME,
            create_transport=lambda: self._mock_transport)

    def test_should_raise_on_report_without_start(self):
        dummy_request = _make_dummy_report_request(self.PROJECT_ID,
                                                   self.SERVICE_NAME)
        expect(lambda: self._subject.report(dummy_request)).to(
            raise_error(AssertionError))

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_not_send_the_request_if_cached(self, dummy_thread_class):
        t = self._mock_transport
        self._subject.start()
        dummy_request = _make_dummy_report_request(self.PROJECT_ID,
                                                   self.SERVICE_NAME)
        self._subject.report(dummy_request)
        expect(t.services.report.called).to(be_false)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_send_a_request_if_not_cached(self, dummy_thread_class):
        self._subject = client.Loaders.NO_CACHE.load(
            self.SERVICE_NAME,
            create_transport=lambda: self._mock_transport)

        t = self._mock_transport
        self._subject.start()
        dummy_request = _make_dummy_report_request(self.PROJECT_ID,
                                                   self.SERVICE_NAME)
        self._subject.report(dummy_request)
        expect(t.services.report.called).to(be_true)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_ignore_bad_transport_when_not_cached(self, dummy_thread_class):
        self._subject = client.Loaders.NO_CACHE.load(
            self.SERVICE_NAME,
            create_transport=lambda: self._mock_transport)

        self._mock_transport.services.report.side_effect = lambda: 1/0
        self._subject.start()
        dummy_request = _make_dummy_report_request(self.PROJECT_ID,
                                                   self.SERVICE_NAME)
        self._subject.report(dummy_request)
        expect(self._mock_transport.services.report.called).to(be_true)


class TestNoSchedulerThread(unittest2.TestCase):
    SERVICE_NAME = 'no-scheduler-thread'
    PROJECT_ID = SERVICE_NAME + '.project'

    def setUp(self):
        self._timer = _DateTimeTimer()
        self._mock_transport = mock.MagicMock()
        self._subject = client.Loaders.DEFAULT.load(
            self.SERVICE_NAME,
            create_transport=lambda: self._mock_transport,
            timer=self._timer)
        self._no_cache_subject = client.Loaders.NO_CACHE.load(
            self.SERVICE_NAME,
            create_transport=lambda: self._mock_transport,
            timer=self._timer)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    @mock.patch("google.api.control.client.sched", spec=True)
    def test_should_initialize_scheduler(self, sched, thread_class):
        thread_class.return_value.start.side_effect = lambda: 1/0
        for s in (self._subject, self._no_cache_subject):
            s.start()
            expect(sched.scheduler.called).to(be_true)
            sched.reset_mock()

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    @mock.patch("google.api.control.client.sched", spec=True)
    def test_should_not_enter_scheduler_when_there_is_no_cache(self, sched, thread_class):
        thread_class.return_value.start.side_effect = lambda: 1/0
        self._no_cache_subject.start()
        expect(sched.scheduler.called).to(be_true)
        scheduler = sched.scheduler.return_value
        expect(scheduler.enter.called).to(be_false)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    @mock.patch("google.api.control.client.sched", spec=True)
    def test_should_enter_scheduler_when_there_is_a_cache(self, sched, thread_class):
        thread_class.return_value.start.side_effect = lambda: 1/0
        self._subject.start()
        expect(sched.scheduler.called).to(be_true)
        scheduler = sched.scheduler.return_value
        expect(scheduler.enter.called).to(be_true)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    @mock.patch("google.api.control.client.sched", spec=True)
    def test_should_not_enter_scheduler_for_cached_checks(self, sched, thread_class):
        thread_class.return_value.start.side_effect = lambda: 1/0
        self._subject.start()

        # confirm scheduler is created and initialized
        expect(sched.scheduler.called).to(be_true)
        scheduler = sched.scheduler.return_value
        expect(scheduler.enter.called).to(be_true)
        scheduler.reset_mock()

        # call check once, to a cache response
        dummy_request = _make_dummy_check_request(self.PROJECT_ID,
                                                  self.SERVICE_NAME)
        dummy_response = messages.CheckResponse(
            operationId=dummy_request.checkRequest.operation.operationId)
        t = self._mock_transport
        t.services.check.return_value = dummy_response
        expect(self._subject.check(dummy_request)).to(equal(dummy_response))
        t.reset_mock()

        # call check again - response is cached...
        expect(self._subject.check(dummy_request)).to(equal(dummy_response))
        expect(self._mock_transport.services.check.called).to(be_false)

        # ... the scheduler is not run
        expect(scheduler.run.called).to(be_false)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    @mock.patch("google.api.control.client.sched", spec=True)
    def test_should_enter_scheduler_for_aggregated_reports(self, sched, thread_class):
        thread_class.return_value.start.side_effect = lambda: 1/0
        self._subject.start()

        # confirm scheduler is created and initialized
        expect(sched.scheduler.called).to(be_true)
        scheduler = sched.scheduler.return_value
        expect(scheduler.enter.called).to(be_true)
        scheduler.reset_mock()

        # call report once; transport is not called, but the scheduler is run
        dummy_request = _make_dummy_report_request(self.PROJECT_ID,
                                                   self.SERVICE_NAME)
        self._subject.report(dummy_request)
        expect(self._mock_transport.services.report.called).to(be_false)
        expect(scheduler.run.called).to(be_true)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    def test_should_flush_report_cache_in_scheduler(self, thread_class):
        thread_class.return_value.start.side_effect = lambda: 1/0
        self._subject.start()

        # call report once; transport is not called
        dummy_request = _make_dummy_report_request(self.PROJECT_ID,
                                                   self.SERVICE_NAME)
        self._subject.report(dummy_request)  # cached a report
        expect(self._mock_transport.services.report.called).to(be_false)
        # pass time, at least the flush interval, after which the report
        # cache to flush
        self._timer.tick()
        self._timer.tick()
        self._subject.report(dummy_request)
        expect(self._mock_transport.services.report.called).to(be_true)

    @mock.patch("google.api.control.client._THREAD_CLASS", spec=True)
    @mock.patch("google.api.control.client.sched", spec=True)
    def test_should_not_run_scheduler_when_stopping(self, sched, thread_class):
        thread_class.return_value.start.side_effect = lambda: 1/0
        self._subject.start()

        # confirm scheduler is created and initialized
        expect(sched.scheduler.called).to(be_true)
        scheduler = sched.scheduler.return_value
        expect(scheduler.enter.called).to(be_true)

        # stop the subject. transport is called, but the scheduler is not run
        self._subject.report(
            _make_dummy_report_request(self.PROJECT_ID, self.SERVICE_NAME))
        scheduler.reset_mock()
        self._subject.stop()
        expect(self._mock_transport.services.report.called).to(be_true)
        expect(scheduler.run.called).to(be_false)


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
