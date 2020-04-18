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

"""client provides a complete standalone service control client.

:class:`Client` is a package-level facade that encapsulates all service control
functionality.

The :class:`Loaders` simplify ``Client`` initialization.

``Client`` needs to stop and start a thread to implement its behaviour.  In most
environments, the default thread class is sufficient.  However, on Google App Engine,
it's necessary to use the appengine specific threading class instead.

:func:`use_gae_thread` and `use_default_thread` can be used to change the thread
class used by new instances of `Client`.

Example:

  >>> from google.api.control import client
  >>>
  >>> # use on appengine with package-default settings
  >>> service_name = 'my-appengine-service-name'
  >>> client.use_gae_thread()
  >>> gae_client = client.Loaders.DEFAULT.load(service_name)
  >>> gae_client.start()

"""
from __future__ import absolute_import

from datetime import datetime, timedelta
from enum import Enum
import json
import logging
import os
import threading
import time

from . import api_client, check_request, report_request
from . import USER_AGENT
from google.api.control.caches import CheckOptions, ReportOptions, to_cache_timer
from google.api.control.vendor.py3 import sched


logger = logging.getLogger(__name__)


CONFIG_VAR = 'ENDPOINTS_SERVER_CONFIG_FILE'


def _load_from_well_known_env():
    if CONFIG_VAR not in os.environ:
        logger.info('did not load server config; no environ var %s', CONFIG_VAR)
        return _load_default()
    json_file = os.environ[CONFIG_VAR]
    if not os.path.exists(json_file):
        logger.warn('did not load service; missing config file %s', json_file)
        return _load_default()
    try:
        with open(json_file) as f:
            json_dict = json.load(f)
            check_json = json_dict['checkAggregatorConfig']
            report_json = json_dict['reportAggregatorConfig']
            check_options = CheckOptions(
                num_entries=check_json['cacheEntries'],
                expiration=timedelta(
                    milliseconds=check_json['responseExpirationMs']),
                flush_interval=timedelta(
                    milliseconds=check_json['flushIntervalMs']))
            report_options = ReportOptions(
                num_entries=report_json['cacheEntries'],
                flush_interval=timedelta(
                    milliseconds=report_json['flushIntervalMs']))
            return check_options, report_options
    except (KeyError, ValueError):
        logger.warn('did not load service; bad json config file %s',
                    json_file,
                    exc_info=True)
        return _load_default()


def _load_default():
    return CheckOptions(), ReportOptions()


def _load_no_cache():
    return (CheckOptions(num_entries=-1),
            ReportOptions(num_entries=-1))


class Loaders(Enum):
    """Enumerates the functions used to load clients from server configs."""
    # pylint: disable=too-few-public-methods
    ENVIRONMENT = (_load_from_well_known_env,)
    DEFAULT = (_load_default,)
    NO_CACHE = (_load_no_cache,)

    def __init__(self, load_func):
        """Constructor.

        load_func is used to load a client config
        """
        self._load_func = load_func

    def load(self, service_name, **kw):
        check_opts, report_opts = self._load_func()
        return Client(service_name, check_opts, report_opts, **kw)


_THREAD_CLASS = threading.Thread


def _create_http_transport():
    additional_http_headers = {"user-agent": USER_AGENT}
    do_logging = logger.level <= logging.DEBUG
    return api_client.ServicecontrolV1(
        additional_http_headers=additional_http_headers,
        log_request=do_logging,
        log_response=do_logging)


def _thread_local_http_transport_func():
    local = threading.local()

    def create_transport():
        if not getattr(local, "transport", None):
            local.transport = _create_http_transport()
        return local.transport

    return create_transport


_CREATE_THREAD_LOCAL_TRANSPORT = _thread_local_http_transport_func()


class Client(object):
    """Client is a package-level facade that encapsulates all service control
    functionality.

    Using one of the :class:`Loaders` makes it easy to initialize ``Client``
    instances.

    Example:

      >>> from google.api.control import client
      >>> service_name = 'my-service-name'
      >>>
      >>> # create an scc client using the package default values
      >>> default_client = client.Loaders.DEFAULT.load(service_name)

      >>> # create an scc client by loading configuration from the
      >>> # a JSON file configured by an environment variable
      >>> json_conf_client = client.Loaders.ENVIRONMENT.load(service_name)

    Client is thread-compatible

    """
    # pylint: disable=too-many-instance-attributes, too-many-arguments

    def __init__(self,
                 service_name,
                 check_options,
                 report_options,
                 timer=datetime.utcnow,
                 create_transport=_CREATE_THREAD_LOCAL_TRANSPORT):
        """

        Args:
            service_name (str): the name of the service to be controlled
            check_options (:class:`google.api.control.caches.CheckOptions`):
              configures checking
            report_options (:class:`google.api.control.caches.ReportOptions`):
              configures reporting
            timer (:func[[datetime.datetime]]: used to obtain the current time.
        """
        self._check_aggregator = check_request.Aggregator(service_name,
                                                          check_options,
                                                          timer=timer)
        self._report_aggregator = report_request.Aggregator(service_name,
                                                            report_options,
                                                            timer=timer)
        self._running = False
        self._scheduler = None
        self._stopped = False
        self._timer = timer
        self._thread = None
        self._create_transport = create_transport
        self._lock = threading.RLock()

    def start(self):
        """Starts processing.

        Calling this method

        - starts the thread that regularly flushes all enabled caches.
        - enables the other methods on the instance to be called successfully

        I.e, even when the configuration disables aggregation, it is invalid to
        access the other methods of an instance until ``start`` is called -
        Calls to other public methods will fail with an AssertionError.

        """
        with self._lock:
            if self._running:
                logger.info('%s is already started', self)
                return

            self._stopped = False
            self._running = True
            logger.info('starting thread of type %s to run the scheduler',
                        _THREAD_CLASS)
            self._thread = _THREAD_CLASS(target=self._schedule_flushes)
            try:
                self._thread.start()
            except Exception:  # pylint: disable=broad-except
                logger.warn(
                    'no scheduler thread, scheduler.run() will be invoked by report(...)',
                    exc_info=True)
                self._thread = None
                self._initialize_flushing()

    def stop(self):
        """Halts processing

        This will lead to the reports being flushed, the caches being cleared
        and a stop to the current processing thread.

        """
        with self._lock:
            if self._stopped:
                logger.info('%s is already stopped', self)
                return

            self._flush_all_reports()
            self._stopped = True
            if self._run_scheduler_directly:
                self._cleanup_if_stopped()

            if self._scheduler and self._scheduler.empty():
                # if there are events scheduled, then _running will subsequently
                # be set False by the scheduler thread.  This handles the
                # case where there are no events, e.g because all aggreagation
                # was disabled
                self._running = False
            self._scheduler = None

    def check(self, check_req):
        """Process a check_request.

        The req is first passed to the check_aggregator.  If there is a valid
        cached response, that is returned, otherwise a response is obtained from
        the transport.

        Args:
          check_req (``ServicecontrolServicesCheckRequest``): to be sent to
            the service control service

        Returns:
           ``CheckResponse``: either the cached response if one is applicable
            or a response from making a transport request, or None if
            if the request to the transport fails

        """

        self._assert_is_running()
        res = self._check_aggregator.check(check_req)
        if res:
            logger.debug('using cached check response for %s: %s',
                         check_request, res)
            return res

        # Application code should not fail because check request's don't
        # complete, They should fail open, so here simply log the error and
        # return None to indicate that no response was obtained
        try:
            transport = self._create_transport()
            resp = transport.services.check(check_req)
            self._check_aggregator.add_response(check_req, resp)
            return resp
        except Exception:  # pylint: disable=broad-except
            logger.error('direct send of check request failed %s',
                         check_request, exc_info=True)
            return None

    def report(self, report_req):
        """Processes a report request.

        It will aggregate it with prior report_requests to be send later
        or it will send it immediately if that's appropriate.
        """
        self._assert_is_running()

        # no thread running, run the scheduler to ensure any pending
        # flush tasks are executed.
        if self._run_scheduler_directly:
            self._scheduler.run(blocking=False)

        if not self._report_aggregator.report(report_req):
            logger.info('need to send a report request directly')
            try:
                transport = self._create_transport()
                transport.services.report(report_req)
            except Exception:  # pylint: disable=broad-except
                logger.error('direct send for report request failed',
                             exc_info=True)

    @property
    def _run_scheduler_directly(self):
        return self._running and self._thread is None

    def _assert_is_running(self):
        assert self._running, '%s needs to be running' % (self,)

    def _initialize_flushing(self):
        with self._lock:
            logger.info('created a scheduler to control flushing')
            self._scheduler = sched.scheduler(to_cache_timer(self._timer),
                                              time.sleep)
            logger.info('scheduling initial check and flush')
            self._flush_schedule_check_aggregator()
            self._flush_schedule_report_aggregator()

    def _schedule_flushes(self):
        # the method expects to be run in the thread created in start()
        self._initialize_flushing()
        self._scheduler.run() # should block until self._stopped is set
        logger.info('scheduler.run completed, %s will exit', threading.current_thread())

    def _cleanup_if_stopped(self):
        if not self._stopped:
            return False

        self._check_aggregator.clear()
        self._report_aggregator.clear()
        self._running = False
        return True

    def _flush_schedule_check_aggregator(self):
        if self._cleanup_if_stopped():
            logger.info('did not schedule check flush: client is stopped')
            return

        flush_interval = self._check_aggregator.flush_interval
        if not flush_interval or flush_interval.total_seconds() < 0:
            logger.debug('did not schedule check flush: caching is disabled')
            return

        if self._run_scheduler_directly:
            logger.debug('did not schedule check flush: no scheduler thread')
            return

        logger.debug('flushing the check aggregator')
        transport = self._create_transport()
        for req in self._check_aggregator.flush():
            try:
                resp = transport.services.check(req)
                self._check_aggregator.add_response(req, resp)
            except Exception:  # pylint: disable=broad-except
                logger.error('failed to flush check_req %s', req, exc_info=True)

        # schedule a repeat of this method
        self._scheduler.enter(
            flush_interval.total_seconds(),
            2,  # a higher priority than report flushes
            self._flush_schedule_check_aggregator,
            ()
        )

    def _flush_schedule_report_aggregator(self):
        if self._cleanup_if_stopped():
            logger.info('did not schedule report flush: client is stopped')
            return

        flush_interval = self._report_aggregator.flush_interval
        if not flush_interval or flush_interval.total_seconds() < 0:
            logger.debug('did not schedule report flush: caching is disabled')
            return

        # flush reports and schedule a repeat of this method
        transport = self._create_transport()
        reqs = self._report_aggregator.flush()
        logger.debug("will flush %d report requests", len(reqs))
        for req in reqs:
            try:
                transport.services.report(req)
            except Exception:  # pylint: disable=broad-except
                logger.error('failed to flush report_req %s', req, exc_info=True)

        self._scheduler.enter(
            flush_interval.total_seconds(),
            1,  # a lower priority than check flushes
            self._flush_schedule_report_aggregator,
            ()
        )

    def _flush_all_reports(self):
        all_requests = self._report_aggregator.clear()
        logger.info('flushing all reports (count=%d)', len(all_requests))
        transport = self._create_transport()
        for req in all_requests:
            try:
                transport.services.report(req)
            except Exception:  # pylint: disable=broad-except
                logger.error('failed to flush report_req %s', req, exc_info=True)


def use_default_thread():
    """Makes ``Client``s started after this use the standard Thread class."""
    global _THREAD_CLASS  # pylint: disable=global-statement
    _THREAD_CLASS = threading.Thread


def use_gae_thread():
    """Makes ``Client``s started after this use the appengine thread class."""
    global _THREAD_CLASS  # pylint: disable=global-statement
    try:
        from google.appengine.api.background_thread import background_thread
        _THREAD_CLASS = background_thread.BackgroundThread
    except ImportError:
        logger.error(
            'Could not install appengine background threads!'
            ' Please install the python AppEngine SDK and use this from there'
        )
