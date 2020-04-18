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

"""check_request supports aggregation of CheckRequests.

:func:`sign` generated a signature from CheckRequests
:class:`~google.api.gen.servicecontrol_v1_message.Operation` represents
information regarding an operation, and is a key constituent of
:class:`~google.api.gen.servicecontrol_v1_message.CheckRequest` and
:class:`~google.api.gen.servicecontrol_v1_message.ReportRequests.

The :class:`.Aggregator` implements the strategy for aggregating CheckRequests
and caching their responses.

"""

from __future__ import absolute_import

import collections
import hashlib
import httplib
import logging
from datetime import datetime

from apitools.base.py import encoding

from . import caches, label_descriptor, messages
from . import metric_value, operation, signing

logger = logging.getLogger(__name__)

# alias for brevity
_CheckErrors = messages.CheckError.CodeValueValuesEnum
_IS_OK = (httplib.OK, '', True)
_IS_UNKNOWN = (
    httplib.INTERNAL_SERVER_ERROR,
    'Request blocked due to unsupported block reason {detail}',
    False)
_CHECK_ERROR_CONVERSION = {
    _CheckErrors.NOT_FOUND: (
        httplib.BAD_REQUEST,
        'Client project not found. Please pass a valid project',
        False,
    ),
    _CheckErrors.API_KEY_NOT_FOUND: (
        httplib.BAD_REQUEST,
        'API key not found. Please pass a valid API key',
        True,
    ),
    _CheckErrors.API_KEY_EXPIRED: (
        httplib.BAD_REQUEST,
        'API key expired. Please renew the API key',
        True,
    ),
    _CheckErrors.API_KEY_INVALID: (
        httplib.BAD_REQUEST,
        'API not valid. Please pass a valid API key',
        True,
    ),
    _CheckErrors.SERVICE_NOT_ACTIVATED: (
        httplib.FORBIDDEN,
        '{detail} Please enable the project for {project_id}',
        False,
    ),
    _CheckErrors.PERMISSION_DENIED: (
        httplib.FORBIDDEN,
        'Permission denied: {detail}',
        False,
    ),
    _CheckErrors.IP_ADDRESS_BLOCKED: (
        httplib.FORBIDDEN,
        '{detail}',
        False,
    ),
    _CheckErrors.REFERER_BLOCKED: (
        httplib.FORBIDDEN,
        '{detail}',
        False,
    ),
    _CheckErrors.CLIENT_APP_BLOCKED: (
        httplib.FORBIDDEN,
        '{detail}',
        False,
    ),
    _CheckErrors.PROJECT_DELETED: (
        httplib.FORBIDDEN,
        'Project {project_id} has been deleted',
        False,
    ),
    _CheckErrors.PROJECT_INVALID: (
        httplib.BAD_REQUEST,
        'Client Project is not valid.  Please pass a valid project',
        False,
    ),
    _CheckErrors.VISIBILITY_DENIED: (
        httplib.FORBIDDEN,
        'Project {project_id} has no visibility access to the service',
        False,
    ),
    _CheckErrors.BILLING_DISABLED: (
        httplib.FORBIDDEN,
        'Project {project_id} has billing disabled. Please enable it',
        False,
    ),

    # Fail open for internal server errors
    _CheckErrors.NAMESPACE_LOOKUP_UNAVAILABLE: _IS_OK,
    _CheckErrors.SERVICE_STATUS_UNAVAILABLE: _IS_OK,
    _CheckErrors.BILLING_STATUS_UNAVAILABLE: _IS_OK,
    _CheckErrors.QUOTA_CHECK_UNAVAILABLE: _IS_OK,
}


def convert_response(check_response, project_id):
    """Computes a http status code and message `CheckResponse`

    The return value a tuple (code, message, api_key_is_bad) where

    code: is the http status code
    message: is the message to return
    api_key_is_bad: indicates that a given api_key is bad

    Args:
       check_response (:class:`google.api.gen.servicecontrol_v1_messages.CheckResponse`):
         the response from calling an api

    Returns:
       tuple(code, message, bool)
    """
    if not check_response or not check_response.checkErrors:
        return _IS_OK

    # only check the first error for now, as per ESP
    theError = check_response.checkErrors[0]
    error_tuple = _CHECK_ERROR_CONVERSION.get(theError.code, _IS_UNKNOWN)
    if error_tuple[1].find('{') == -1:  # no replacements needed:
        return error_tuple

    updated_msg = error_tuple[1].replace('{project_id}', project_id)
    updated_msg = updated_msg.replace('{detail}', theError.detail or '')
    error_tuple = (error_tuple[0], updated_msg, error_tuple[2])
    return error_tuple


def sign(check_request):
    """Obtains a signature for an operation in a `CheckRequest`

    Args:
       op (:class:`google.api.gen.servicecontrol_v1_messages.Operation`): an
         operation used in a `CheckRequest`

    Returns:
       string: a secure hash generated from the operation
    """
    if not isinstance(check_request, messages.CheckRequest):
        raise ValueError('Invalid request')
    op = check_request.operation
    if op is None or op.operationName is None or op.consumerId is None:
        logging.error('Bad %s: not initialized => not signed', check_request)
        raise ValueError('check request must be initialized with an operation')
    md5 = hashlib.md5()
    md5.update(op.operationName)
    md5.update('\x00')
    md5.update(op.consumerId)
    if op.labels:
        signing.add_dict_to_hash(md5, encoding.MessageToPyValue(op.labels))
    for value_set in op.metricValueSets:
        md5.update('\x00')
        md5.update(value_set.metricName)
        for mv in value_set.metricValues:
            metric_value.update_hash(md5, mv)

    md5.update('\x00')
    if op.quotaProperties:
        # N.B: this differs form cxx implementation, which serializes the
        # protobuf. This should be OK as the exact hash used does not need to
        # match across implementations.
        md5.update(repr(op.quotaProperties))

    md5.update('\x00')
    return md5.digest()


_KNOWN_LABELS = label_descriptor.KnownLabels


class Info(collections.namedtuple('Info',
                                  ('client_ip',) + operation.Info._fields),
           operation.Info):
    """Holds the information necessary to fill in CheckRequest.

    In addition the attributes in :class:`operation.Info`, this has:

    Attributes:
       client_ip: the client IP address

    """
    def __new__(cls, client_ip='', **kw):
        """Invokes the base constructor with default values."""
        op_info = operation.Info(**kw)
        return super(Info, cls).__new__(cls, client_ip, **op_info._asdict())

    def as_check_request(self, timer=datetime.utcnow):
        """Makes a `ServicecontrolServicesCheckRequest` from this instance

        Returns:
          a ``ServicecontrolServicesCheckRequest``

        Raises:
          ValueError: if the fields in this instance are insufficient to
            to create a valid ``ServicecontrolServicesCheckRequest``

        """
        if not self.service_name:
            raise ValueError('the service name must be set')
        if not self.operation_id:
            raise ValueError('the operation id must be set')
        if not self.operation_name:
            raise ValueError('the operation name must be set')
        op = super(Info, self).as_operation(timer=timer)
        labels = {
            _KNOWN_LABELS.SCC_USER_AGENT.label_name: label_descriptor.USER_AGENT
        }
        if self.client_ip:
            labels[_KNOWN_LABELS.SCC_CALLER_IP.label_name] = self.client_ip

        if self.referer:
            labels[_KNOWN_LABELS.SCC_REFERER.label_name] = self.referer

        op.labels = encoding.PyValueToMessage(
            messages.Operation.LabelsValue, labels)
        check_request = messages.CheckRequest(operation=op)
        return messages.ServicecontrolServicesCheckRequest(
            serviceName=self.service_name,
            checkRequest=check_request)


class Aggregator(object):
    """Caches and aggregates ``CheckRequests``.

    Concurrency: Thread safe.

    Usage:

    Creating a new cache entry and use cached response

    Example:
      >>> options = caches.CheckOptions()
      >>> agg = Aggregator('my_service', options)
      >>> req = ServicecontrolServicesCheckRequest(...)
      >>> # check returns None as the request is not cached
      >>> if agg.check(req) is not None:
      ...    resp = service.check(req)
      ...    agg = service.add_response(req, resp)
      >>> agg.check(req)  # response now cached according as-per options
      <CheckResponse ....>

    Refreshing a cached entry after a flush interval

    The flush interval is constrained to be shorter than the actual cache
    expiration.  This allows the response to potentially remain cached and be
    aggregated with subsequent check requests for the same operation.

    Example:
      >>> # continuing from the previous example,
      >>> # ... after the flush interval
      >>> # - the response is still in the cache, i.e, not expired
      >>> # - the first call after the flush interval returns None, subsequent
      >>> #  calls continue to return the cached response
      >>> agg.check(req)  # signals the caller to call service.check(req)
      None
      >>> agg.check(req)  # next call returns the cached response
      <CheckResponse ....>

    Flushing the cache

    Once a response is expired, if there is an outstanding, cached CheckRequest
    for it, this should be sent and their responses added back to the
    aggregator instance, as they will contain quota updates that have not been
    sent.

    Example:

      >>> # continuing the previous example
      >>> for req in agg.flush():  # an iterable of cached CheckRequests
      ...     resp = caller.send_req(req)  # caller sends them
      >>>     agg.add_response(req, resp)  # and caches their responses

    """

    def __init__(self, service_name, options, kinds=None,
                 timer=datetime.utcnow):
        """Constructor.

        Args:
          service_name (string): names the service that all requests aggregated
            by this instance will be sent
          options (:class:`~google.api.caches.CheckOptions`): configures the
            caching and flushing behavior of this instance
          kinds (dict[string,[google.api.control.MetricKind]]): specifies the
            kind of metric for each each metric name.
          timer (function([[datetime]]): a function that returns the current
            as a time as a datetime instance
        """
        self._service_name = service_name
        self._options = options
        self._cache = caches.create(options, timer=timer)
        self._kinds = {} if kinds is None else dict(kinds)
        self._timer = timer

    @property
    def service_name(self):
        """The service to which all aggregated requests should belong."""
        return self._service_name

    @property
    def flush_interval(self):
        """The interval between calls to flush.

        Returns:
           timedelta: the period between calls to flush if, or ``None`` if no
           cache is set

        """
        return None if self._cache is None else self._options.expiration

    def flush(self):
        """Flushes this instance's cache.

        The driver of this instance should call this method every
        `flush_interval`.

        Returns:
          list['CheckRequest']: corresponding to CheckRequests that were
          pending

        """
        if self._cache is None:
            return []
        with self._cache as c:
            flushed_items = list(c.out_deque)
            c.out_deque.clear()
            cached_reqs = [item.extract_request() for item in flushed_items]
            cached_reqs = [req for req in cached_reqs if req is not None]
            return cached_reqs

    def clear(self):
        """Clears this instance's cache."""
        if self._cache is not None:
            with self._cache as c:
                c.clear()
                c.out_deque.clear()

    def add_response(self, req, resp):
        """Adds the response from sending to `req` to this instance's cache.

        Args:
          req (`ServicecontrolServicesCheckRequest`): the request
          resp (CheckResponse): the response from sending the request
        """
        if self._cache is None:
            return
        signature = sign(req.checkRequest)
        with self._cache as c:
            now = self._timer()
            quota_scale = 0  # WIP
            item = c.get(signature)
            if item is None:
                c[signature] = CachedItem(
                    resp, self.service_name, now, quota_scale)
            else:
                # Update the cached item to reflect that it is updated
                item.last_check_time = now
                item.response = resp
                item.quota_scale = quota_scale
                item.is_flushing = False
                c[signature] = item

    def check(self, req):
        """Determine if ``req`` is in this instances cache.

        Determine if there are cache hits for the request in this aggregator
        instance.

        Not in the cache

        If req is not in the cache, it returns ``None`` to indicate that the
        caller should send the request.

        Cache Hit; response has errors

        When a cached CheckResponse has errors, it's assumed that ``req`` would
        fail as well, so the cached CheckResponse is returned.  However, the
        first CheckRequest after the flush interval has elapsed should be sent
        to the server to refresh the CheckResponse, though until it's received,
        subsequent CheckRequests should fail with the cached CheckResponse.

        Cache behaviour - response passed

        If the cached CheckResponse has no errors, it's assumed that ``req``
        will succeed as well, so the CheckResponse is returned, with the quota
        info updated to the same as requested.  The requested tokens are
        aggregated until flushed.

        Args:
          req (``ServicecontrolServicesCheckRequest``): to be sent to
            the service control service

        Raises:
           ValueError: if the ``req`` service_name is not the same as
             this instances

        Returns:
           ``CheckResponse``: if an applicable response is cached by this
             instance is available for use or None, if there is no applicable
             response

        """
        if self._cache is None:
            return None  # no cache, send request now
        if not isinstance(req, messages.ServicecontrolServicesCheckRequest):
            raise ValueError('Invalid request')
        if req.serviceName != self.service_name:
            logger.error('bad check(): service_name %s does not match ours %s',
                         req.serviceName, self.service_name)
            raise ValueError('Service name mismatch')
        check_request = req.checkRequest
        if check_request is None:
            logger.error('bad check(): no check_request in %s', req)
            raise ValueError('Expected operation not set')
        op = check_request.operation
        if op is None:
            logger.error('bad check(): no operation in %s', req)
            raise ValueError('Expected operation not set')
        if op.importance != messages.Operation.ImportanceValueValuesEnum.LOW:
            return None  # op is important, send request now

        signature = sign(check_request)
        with self._cache as cache:
            logger.debug('checking the cache for %s\n%s', signature, cache)
            item = cache.get(signature)
            if item is None:
                return None  # signal to caller to send req
            else:
                return self._handle_cached_response(req, item)

    def _handle_cached_response(self, req, item):
        with self._cache:  # defensive, this re-entrant lock should be held
            if len(item.response.checkErrors) > 0:
                if self._is_current(item):
                    return item.response

                # There are errors, but now it's ok to send a new request
                item.last_check_time = self._timer()
                return None  # signal caller to send req
            else:
                item.update_request(req, self._kinds)
                if self._is_current(item):
                    return item.response

                if (item.is_flushing):
                    logger.warn('last refresh request did not complete')

                item.is_flushing = True
                item.last_check_time = self._timer()
                return None  # signal caller to send req

    def _is_current(self, item):
        age = self._timer() - item.last_check_time
        return age < self._options.flush_interval


class CachedItem(object):
    """CachedItem holds items cached along with a ``CheckRequest``.

    Thread compatible.

    Attributes:
       response (:class:`messages.CachedResponse`): the cached response
       is_flushing (bool): indicates if it's been detected that item
         is stale, and needs to be flushed
       quota_scale (int): WIP, used to determine quota
       last_check_time (datetime.datetime): the last time this instance
         was checked

    """

    def __init__(self, resp, service_name, last_check_time, quota_scale):
        self.last_check_time = last_check_time
        self.quota_scale = quota_scale
        self.is_flushing = False
        self.response = resp
        self._service_name = service_name
        self._op_aggregator = None

    def update_request(self, req, kinds):
        agg = self._op_aggregator
        if agg is None:
            self._op_aggregator = operation.Aggregator(
                req.checkRequest.operation, kinds)
        else:
            agg.add(req.checkRequest.operation)

    def extract_request(self):
        if self._op_aggregator is None:
            return None

        op = self._op_aggregator.as_operation()
        self._op_aggregator = None
        check_request = messages.CheckRequest(operation=op)
        return messages.ServicecontrolServicesCheckRequest(
            serviceName=self._service_name,
            checkRequest=check_request)
