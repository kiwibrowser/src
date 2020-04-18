# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The request data track.

When executed, parses a JSON dump of DevTools messages.
"""

import bisect
import collections
import copy
import datetime
import email.utils
import hashlib
import json
import logging
import re
import sys
import urlparse

import devtools_monitor


class Timing(object):
  """Collects the timing data for a request."""
  UNVAILABLE = -1
  _TIMING_NAMES = (
      ('connectEnd', 'connect_end'), ('connectStart', 'connect_start'),
      ('dnsEnd', 'dns_end'), ('dnsStart', 'dns_start'),
      ('proxyEnd', 'proxy_end'), ('proxyStart', 'proxy_start'),
      ('receiveHeadersEnd', 'receive_headers_end'),
      ('requestTime', 'request_time'), ('sendEnd', 'send_end'),
      ('sendStart', 'send_start'), ('sslEnd', 'ssl_end'),
      ('sslStart', 'ssl_start'), ('workerReady', 'worker_ready'),
      ('workerStart', 'worker_start'),
      ('loadingFinished', 'loading_finished'), ('pushStart', 'push_start'),
      ('pushEnd', 'push_end'))
  _TIMING_NAMES_MAPPING = dict(_TIMING_NAMES)
  __slots__ = tuple(x[1] for x in _TIMING_NAMES)

  def __init__(self, **kwargs):
    """Constructor.

    Initialize with keywords arguments from __slots__.
    """
    for slot in self.__slots__:
      setattr(self, slot, self.UNVAILABLE)
    for (attr, value) in kwargs.items():
      setattr(self, attr, value)

  def __eq__(self, o):
    return all(getattr(self, attr) == getattr(o, attr)
               for attr in self.__slots__)

  def __str__(self):
    return str(self.ToJsonDict())

  def LargestOffset(self):
    """Returns the largest offset in the available timings."""
    return max(0, max(
        getattr(self, attr) for attr in self.__slots__
        if attr != 'request_time'))

  def ToJsonDict(self):
    return {attr: getattr(self, attr)
            for attr in self.__slots__ if getattr(self, attr) != -1}

  @classmethod
  def FromJsonDict(cls, json_dict):
    return cls(**json_dict)

  @classmethod
  def FromDevToolsDict(cls, json_dict):
    """Returns an instance of Timing from a dict, as passed by DevTools."""
    timing_dict = {
        cls._TIMING_NAMES_MAPPING[k]: v for (k, v) in json_dict.items()}
    return cls(**timing_dict)


def ShortName(url):
  """Returns a shortened version of a URL."""
  parsed = urlparse.urlparse(url)
  path = parsed.path
  hostname = parsed.hostname if parsed.hostname else '?.?.?'
  if path != '' and path != '/':
    last_path = parsed.path.split('/')[-1]
    if len(last_path) < 10:
      if len(path) < 10:
        return hostname + '/' + path
      else:
        return hostname + '/..' + parsed.path[-10:]
    else:
        return hostname + '/..' + last_path[:5]
  else:
    return hostname


def IntervalBetween(first, second, reason):
  """Returns the start and end of the inteval between two requests, in ms.

  This is defined as:
  - [first.headers, second.start] if reason is 'parser'. This is to account
    for incremental parsing.
  - [first.end, second.start] if reason is 'script', 'redirect' or 'other'.

  Args:
    first: (Request) First request.
    second: (Request) Second request.
    reason: (str) Link between the two requests, in Request.INITIATORS.

  Returns:
    (start_msec (float), end_msec (float)),
  """
  assert reason in Request.INITIATORS
  second_ms = second.timing.request_time * 1000
  if reason == 'parser':
    first_offset_ms = first.timing.receive_headers_end
  else:
    first_offset_ms = first.timing.LargestOffset()
  return (first.timing.request_time * 1000 + first_offset_ms, second_ms)


def TimeBetween(first, second, reason):
  """(end_msec - start_msec), with the values as returned by IntervalBetween().
  """
  (first_ms, second_ms) = IntervalBetween(first, second, reason)
  return second_ms - first_ms


def TimingAsList(timing):
  """Transform Timing to a list, eg as is used in JSON output.

  Args:
    timing: a Timing.

  Returns:
    A list identical to what the eventual JSON output will be (eg,
    Request.ToJsonDict).
  """
  return json.loads(json.dumps(timing))


class Request(object):
  """Represents a single request.

  Generally speaking, fields here closely mirror those documented in
  third_party/blink/renderer/devtools/protocol.json.

  Fields:
    request_id: (str) unique request ID. Postfixed with _REDIRECT_SUFFIX for
                redirects.
    frame_id: (str) unique frame identifier.
    loader_id: (str) unique frame identifier.
    document_url: (str) URL of the document this request is loaded for.
    url: (str) Request URL.
    protocol: (str) protocol used for the request.
    method: (str) HTTP method, such as POST or GET.
    request_headers: (dict) {'header': 'value'} Request headers.
    response_headers: (dict) {'header': 'value'} Response headers.
    initial_priority: (str) Initial request priority, in REQUEST_PRIORITIES.
    timestamp: (float) Request timestamp, in s.
    wall_time: (float) Request timestamp, UTC timestamp in s.
    initiator: (dict) Request initiator, in INITIATORS.
    resource_type: (str) Resource type, in RESOURCE_TYPES
    served_from_cache: (bool) Whether the request was served from cache.
    from_disk_cache: (bool) Whether the request was served from the disk cache.
    from_service_worker: (bool) Whether the request was served by a Service
                         Worker.
    timing: (Timing) Request timing, extended with loading_finished.
    status: (int) Response status code.
    status_text: (str) Response status text received in the status line.
    encoded_data_length: (int) Total encoded data length.
    data_chunks: (list) [(offset, encoded_data_length), ...] List of data
                 chunks received, with their offset in ms relative to
                 Timing.requestTime.
    failed: (bool) Whether the request failed.
    error_text: (str) User friendly error message when request failed.
    start_msec: (float) Request start time, in milliseconds from chrome start.
    end_msec: (float) Request end time, in milliseconds from chrome start.
      start_msec.
  """
  REQUEST_PRIORITIES = ('VeryLow', 'Low', 'Medium', 'High', 'VeryHigh')
  RESOURCE_TYPES = ('Document', 'Stylesheet', 'Image', 'Media', 'Font',
                    'Script', 'TextTrack', 'XHR', 'Fetch', 'EventSource',
                    'WebSocket', 'Manifest', 'Other')
  INITIATORS = ('parser', 'script', 'other', 'redirect')
  INITIATING_REQUEST = 'initiating_request'
  ORIGINAL_INITIATOR = 'original_initiator'
  def __init__(self):
    self.request_id = None
    self.frame_id = None
    self.loader_id = None
    self.document_url = None
    self.url = None
    self.protocol = None
    self.method = None
    self.mime_type = None
    self.request_headers = None
    self.response_headers = None
    self.initial_priority = None
    self.timestamp = -1
    self.wall_time = -1
    self.initiator = None
    self.resource_type = None
    self.served_from_cache = False
    self.from_disk_cache = False
    self.from_service_worker = False
    self.timing = None
    self.status = None
    self.status_text = None
    self.response_headers_length = 0
    self.encoded_data_length = 0
    self.data_chunks = []
    self.failed = False
    self.error_text = None

  @property
  def start_msec(self):
    return self.timing.request_time * 1000

  @property
  def end_msec(self):
    if self.start_msec is None:
      return None
    return self.start_msec + self.timing.LargestOffset()

  @property
  def fingerprint(self):
    h = hashlib.sha256()
    h.update(self.url)
    return h.hexdigest()[:10]

  def _TimestampOffsetFromStartMs(self, timestamp):
    assert self.timing.request_time != -1
    request_time = self.timing.request_time
    return (timestamp - request_time) * 1000

  def ToJsonDict(self):
    result = copy.deepcopy(self.__dict__)
    result['timing'] = self.timing.ToJsonDict() if self.timing else {}
    return result

  @classmethod
  def FromJsonDict(cls, data_dict):
    result = Request()
    for (k, v) in data_dict.items():
      setattr(result, k, v)
    if not result.response_headers:
      result.response_headers = {}
    if result.timing:
      result.timing = Timing.FromJsonDict(result.timing)
    else:
      result.timing = Timing(request_time=result.timestamp)
    return result

  def GetResponseTransportLength(self):
    """Get the total amount of encoded data no matter whether load has finished
    or not.
    """
    assert self.HasReceivedResponse()
    assert not self.from_disk_cache and not self.served_from_cache
    assert self.protocol not in {'about', 'blob', 'data'}
    if self.timing.loading_finished != Timing.UNVAILABLE:
      encoded_data_length = self.encoded_data_length
    else:
      encoded_data_length = sum(
          [chunk_size for _, chunk_size in self.data_chunks])
      assert encoded_data_length > 0 or len(self.data_chunks) == 0
    return encoded_data_length + self.response_headers_length

  def GetHTTPResponseHeader(self, header_name):
    """Gets the value of a HTTP response header.

    Does a case-insensitive search for the header name in the HTTP response
    headers, in order to support servers that use a wrong capitalization.
    """
    lower_case_name = header_name.lower()
    result = None
    for name, value in self.response_headers.iteritems():
      if name.lower() == lower_case_name:
        result = value
        break
    return result

  def SetHTTPResponseHeader(self, header, header_value):
    """Sets the value of a HTTP response header."""
    assert header.islower()
    for name in self.response_headers.keys():
      if name.lower() == header:
        del self.response_headers[name]
    self.response_headers[header] = header_value

  def GetResponseHeaderValue(self, header, value):
    """Returns a copy of |value| iff response |header| contains it."""
    header_values = self.GetHTTPResponseHeader(header)
    if not header_values:
      return None
    values = header_values.split(',')
    for header_value in values:
      if header_value.lower() == value.lower():
        return header_value
    return None

  def HasResponseHeaderValue(self, header, value):
    """Returns True iff the response headers |header| contains |value|."""
    return self.GetResponseHeaderValue(header, value) is not None

  def GetContentType(self):
    """Returns the content type, or None."""
    # Check for redirects. Use the "Location" header, because the HTTP status is
    # not reliable.
    if self.GetHTTPResponseHeader('Location') is not None:
      return 'redirect'

    # Check if the response is empty.
    if (self.GetHTTPResponseHeader('Content-Length') == '0' or
        self.status == 204):
      return 'ping'

    if self.mime_type:
      return self.mime_type

    content_type = self.GetHTTPResponseHeader('Content-Type')
    if not content_type or ';' not in content_type:
      return content_type
    else:
      return content_type[:content_type.index(';')]

  def IsDataRequest(self):
    return self.protocol == 'data'

  def HasReceivedResponse(self):
    return self.status is not None

  def GetCacheControlDirective(self, directive_name):
    """Returns the value of a Cache-Control directive, or None."""
    cache_control_str = self.GetHTTPResponseHeader('Cache-Control')
    if cache_control_str is None:
      return None
    directives = [s.strip() for s in cache_control_str.split(',')]
    for directive in directives:
      parts = directive.split('=')
      if len(parts) != 2:
        continue
      (name, value) = parts
      if name == directive_name:
        return value
    return None

  def MaxAge(self):
    """Returns the max-age of a resource, or -1."""
    # TODO(lizeb): Handle the "Expires" header as well.
    cache_control = {}
    if not self.response_headers:
      return -1

    cache_control_str = self.GetHTTPResponseHeader('Cache-Control')
    if cache_control_str is not None:
      directives = [s.strip() for s in cache_control_str.split(',')]
      for directive in directives:
        parts = [s.strip() for s in directive.split('=')]
        if len(parts) == 1:
          cache_control[parts[0]] = True
        else:
          cache_control[parts[0]] = parts[1]
    if (u'no-store' in cache_control
        or u'no-cache' in cache_control
        or len(cache_control) == 0):
      return -1
    max_age = self.GetCacheControlDirective('max-age')
    if max_age:
      return int(max_age)
    return -1

  def Cost(self):
    """Returns the cost of this request in ms, defined as time between
    request_time and the latest timing event.
    """
    # All fields in timing are millis relative to request_time.
    return self.timing.LargestOffset()

  def GetRawResponseHeaders(self):
    """Gets the request's raw response headers compatible with
    net::HttpResponseHeaders's constructor.
    """
    assert not self.IsDataRequest()
    assert self.HasReceivedResponse()
    headers = bytes('{} {} {}\x00'.format(
        self.protocol.upper(), self.status, self.status_text))
    for key in sorted(self.response_headers.keys()):
      headers += (bytes(key.encode('latin-1')) + b': ' +
          bytes(self.response_headers[key].encode('latin-1')) + b'\x00')
    return headers

  def __eq__(self, o):
    return self.__dict__ == o.__dict__

  def __hash__(self):
    return hash(self.request_id)

  def __str__(self):
    return json.dumps(self.ToJsonDict(), sort_keys=True, indent=2)


def _ParseStringToInt(string):
  """Parses a string to an integer like base::StringToInt64().

  Returns:
    Parsed integer.
  """
  string = string.strip()
  while string:
    try:
      parsed_integer = int(string)
      if parsed_integer > sys.maxint:
        return sys.maxint
      if parsed_integer < -sys.maxint - 1:
        return -sys.maxint - 1
      return parsed_integer
    except ValueError:
      string = string[:-1]
  return 0


class CachingPolicy(object):
  """Represents the caching policy at an arbitrary time for a cached response.
  """
  FETCH = 'FETCH'
  VALIDATION_NONE = 'VALIDATION_NONE'
  VALIDATION_SYNC = 'VALIDATION_SYNC'
  VALIDATION_ASYNC = 'VALIDATION_ASYNC'
  POLICIES = (FETCH, VALIDATION_NONE, VALIDATION_SYNC, VALIDATION_ASYNC)
  def __init__(self, request):
    """Constructor.

    Args:
      request: (Request)
    """
    assert request.response_headers is not None
    self.request = request
    # This is incorrect, as the timestamp corresponds to when devtools is made
    # aware of the request, not when it was sent. However, this is good enough
    # for computing cache expiration, which doesn't need sub-second precision.
    self._request_time = self.request.wall_time
    # Used when the date is not available.
    self._response_time = (
        self._request_time + self.request.timing.receive_headers_end)

  def HasValidators(self):
    """Returns wether the request has a validator."""
    # Assuming HTTP 1.1+.
    return (self.request.GetHTTPResponseHeader('Last-Modified')
            or self.request.GetHTTPResponseHeader('Etag'))

  def IsCacheable(self):
    """Returns whether the request could be stored in the cache."""
    return not self.request.HasResponseHeaderValue('Cache-Control', 'no-store')

  def PolicyAtDate(self, timestamp):
    """Returns the caching policy at an aribitrary timestamp.

    Args:
      timestamp: (float) Seconds since Epoch.

    Returns:
      A policy in POLICIES.
    """
    # Note: the implementation is largely transcribed from
    # net/http/http_response_headers.cc, itself following RFC 2616.
    if not self.IsCacheable():
      return self.FETCH
    freshness = self.GetFreshnessLifetimes()
    if freshness[0] == 0 and freshness[1] == 0:
      return self.VALIDATION_SYNC
    age = self._GetCurrentAge(timestamp)
    if freshness[0] > age:
      return self.VALIDATION_NONE
    if (freshness[0] + freshness[1]) > age:
      return self.VALIDATION_ASYNC
    return self.VALIDATION_SYNC

  def GetFreshnessLifetimes(self):
    """Returns [freshness, stale-while-revalidate freshness] in seconds."""
    # This is adapted from GetFreshnessLifetimes() in
    # //net/http/http_response_headers.cc (which follows the RFC).
    r = self.request
    result = [0, 0]
    if (r.HasResponseHeaderValue('Cache-Control', 'no-cache')
        or r.HasResponseHeaderValue('Cache-Control', 'no-store')
        or r.HasResponseHeaderValue('Vary', '*')):  # RFC 2616, 13.6.
      return result
    must_revalidate = r.HasResponseHeaderValue(
        'Cache-Control', 'must-revalidate')
    swr_header = r.GetCacheControlDirective('stale-while-revalidate')
    if not must_revalidate and swr_header:
      result[1] = _ParseStringToInt(swr_header)

    max_age_header = r.GetCacheControlDirective('max-age')
    if max_age_header:
      result[0] = _ParseStringToInt(max_age_header)
      return result

    date = self._GetDateValue('Date') or self._response_time
    expires = self._GetDateValue('Expires')
    if expires:
      result[0] = expires - date
      return result

    if self.request.status in (200, 203, 206) and not must_revalidate:
      last_modified = self._GetDateValue('Last-Modified')
      if last_modified and last_modified < date:
        result[0] = (date - last_modified) / 10
        return result

    if self.request.status in (300, 301, 308, 410):
      return [2**48, 0] # ~forever.
    # No header -> not fresh.
    return result

  def _GetDateValue(self, name):
    date_str = self.request.GetHTTPResponseHeader(name)
    if not date_str:
      return None
    parsed_date = email.utils.parsedate_tz(date_str)
    if parsed_date is None:
      return None
    return email.utils.mktime_tz(parsed_date)

  def _GetCurrentAge(self, current_time):
    # See GetCurrentAge() in //net/http/http_response_headers.cc.
    r = self.request
    date_value = self._GetDateValue('Date') or self._response_time
    age_value = int(r.GetHTTPResponseHeader('Age') or '0')

    apparent_age = max(0, self._response_time - date_value)
    corrected_received_age = max(apparent_age, age_value)
    response_delay = self._response_time - self._request_time
    corrected_initial_age = corrected_received_age + response_delay
    resident_time = current_time - self._response_time
    current_age = corrected_initial_age + resident_time

    return current_age


class RequestTrack(devtools_monitor.Track):
  """Aggregates request data."""
  _REDIRECT_SUFFIX = '.redirect'
  # Request status
  _STATUS_SENT = 0
  _STATUS_RESPONSE = 1
  _STATUS_DATA = 2
  _STATUS_FINISHED = 3
  _STATUS_FAILED = 4
  # Serialization KEYS
  _EVENTS_KEY = 'events'
  _METADATA_KEY = 'metadata'
  _DUPLICATES_KEY = 'duplicates_count'
  _INCONSISTENT_INITIATORS_KEY = 'inconsistent_initiators'
  def __init__(self, connection):
    super(RequestTrack, self).__init__(connection)
    self._connection = connection
    self._requests = []
    self._requests_in_flight = {}  # requestId -> (request, status)
    self._completed_requests_by_id = {}
    self._redirects_count_by_id = collections.defaultdict(int)
    self._indexed = False
    self._request_start_timestamps = None
    self._request_end_timestamps = None
    self._requests_by_start = None
    self._requests_by_end = None
    if connection:  # Optional for testing.
      for method in RequestTrack._METHOD_TO_HANDLER:
        self._connection.RegisterListener(method, self)
      # Enable asynchronous callstacks to get full javascript callstacks in
      # initiators
      self._connection.SetScopedState('Debugger.setAsyncCallStackDepth',
                                      {'maxDepth': 4}, {'maxDepth': 0}, True)
    # responseReceived message are sometimes duplicated. Records the message to
    # detect this.
    self._request_id_to_response_received = {}
    self.duplicates_count = 0
    self.inconsistent_initiators_count = 0

  def Handle(self, method, msg):
    assert method in RequestTrack._METHOD_TO_HANDLER
    self._indexed = False
    params = msg['params']
    request_id = params['requestId']
    RequestTrack._METHOD_TO_HANDLER[method](self, request_id, params)

  def GetEvents(self):
    if self._requests_in_flight:
      logging.warning('Number of requests still in flight: %d.'
                      % len(self._requests_in_flight))
    return self._requests

  def GetFirstResourceRequest(self):
    return self.GetEvents()[0]

  def GetFirstRequestMillis(self):
    """Find the canonical start time for this track.

    Returns:
      The millisecond timestamp of the first request.
    """
    assert self._requests, "No requests to analyze."
    self._IndexRequests()
    return self._request_start_timestamps[0]

  def GetLastRequestMillis(self):
    """Find the canonical start time for this track.

    Returns:
      The millisecond timestamp of the first request.
    """
    assert self._requests, "No requests to analyze."
    self._IndexRequests()
    return self._request_end_timestamps[-1]

  def GetEventsStartingBetween(self, start_ms, end_ms):
    """Return events that started in a range.

    Args:
      start_ms: the start time to query, in milliseconds from the first request.
      end_ms: the end time to query, in milliseconds from the first request.

    Returns:
      A list of requests whose start time is in [start_ms, end_ms].
    """
    self._IndexRequests()
    low = bisect.bisect_left(self._request_start_timestamps, start_ms)
    high = bisect.bisect_right(self._request_start_timestamps, end_ms)
    return self._requests_by_start[low:high]

  def GetEventsEndingBetween(self, start_ms, end_ms):
    """Return events that ended in a range.

    Args:
      start_ms: the start time to query, in milliseconds from the first request.
      end_ms: the end time to query, in milliseconds from the first request.

    Returns:
      A list of requests whose end time is in [start_ms, end_ms].
    """
    self._IndexRequests()
    low = bisect.bisect_left(self._request_end_timestamps, start_ms)
    high = bisect.bisect_right(self._request_end_timestamps, end_ms)
    return self._requests_by_end[low:high]

  def ToJsonDict(self):
    if self._requests_in_flight:
      logging.warning('Requests in flight, will be ignored in the dump')
    return {self._EVENTS_KEY: [
        request.ToJsonDict() for request in self._requests],
            self._METADATA_KEY: {
                self._DUPLICATES_KEY: self.duplicates_count,
                self._INCONSISTENT_INITIATORS_KEY:
                self.inconsistent_initiators_count}}

  @classmethod
  def FromJsonDict(cls, json_dict):
    assert cls._EVENTS_KEY in json_dict
    assert cls._METADATA_KEY in json_dict
    result = RequestTrack(None)
    requests = [Request.FromJsonDict(request)
                for request in json_dict[cls._EVENTS_KEY]]
    result._requests = requests
    metadata = json_dict[cls._METADATA_KEY]
    result.duplicates_count = metadata.get(cls._DUPLICATES_KEY, 0)
    result.inconsistent_initiators_count = metadata.get(
        cls._INCONSISTENT_INITIATORS_KEY, 0)
    return result

  def _IndexRequests(self):
    # TODO(mattcary): if we ever have requests without timing then we either
    # need a default, or to make an index that only includes requests with
    # timings.
    if self._indexed:
      return
    valid_requests = [r for r in self._requests
                      if r.start_msec is not None]
    self._requests_by_start = sorted(valid_requests,
                                     key=lambda r: r.start_msec)
    self._request_start_timestamps = [r.start_msec
                                      for r in self._requests_by_start]
    self._requests_by_end = sorted(valid_requests,
                                     key=lambda r: r.end_msec)
    self._request_end_timestamps = [r.end_msec
                                    for r in self._requests_by_end]
    self._indexed = True

  def _RequestWillBeSent(self, request_id, params):
    # Several "requestWillBeSent" events can be dispatched in a row in the case
    # of redirects.
    redirect_initiator = None
    if request_id in self._completed_requests_by_id:
      assert request_id not in self._requests_in_flight
      return
    if request_id in self._requests_in_flight:
      redirect_initiator = self._HandleRedirect(request_id, params)
    assert (request_id not in self._requests_in_flight)
    r = Request()
    r.request_id = request_id
    _CopyFromDictToObject(
        params, r, (('frameId', 'frame_id'), ('loaderId', 'loader_id'),
                    ('documentURL', 'document_url'),
                    ('timestamp', 'timestamp'), ('wallTime', 'wall_time'),
                    ('initiator', 'initiator')))
    request = params['request']
    _CopyFromDictToObject(
        request, r, (('url', 'url'), ('method', 'method'),
                     ('headers', 'headers'),
                     ('initialPriority', 'initial_priority')))
    r.resource_type = params.get('type', 'Other')
    if redirect_initiator:
      original_initiator = r.initiator
      r.initiator = redirect_initiator
      r.initiator[Request.ORIGINAL_INITIATOR] = original_initiator
      initiating_request = self._completed_requests_by_id[
          redirect_initiator[Request.INITIATING_REQUEST]]
      initiating_initiator = initiating_request.initiator.get(
          Request.ORIGINAL_INITIATOR, initiating_request.initiator)
      if initiating_initiator != original_initiator:
        self.inconsistent_initiators_count += 1
    self._requests_in_flight[request_id] = (r, RequestTrack._STATUS_SENT)

  def _HandleRedirect(self, request_id, params):
    (r, status) = self._requests_in_flight[request_id]
    assert status == RequestTrack._STATUS_SENT
    # The second request contains timing information pertaining to the first
    # one. Finalize the first request.
    assert 'redirectResponse' in params
    redirect_response = params['redirectResponse']

    _CopyFromDictToObject(redirect_response, r,
                          (('headers', 'response_headers'),
                           ('encodedDataLength', 'response_headers_length'),
                           ('fromDiskCache', 'from_disk_cache'),
                           ('protocol', 'protocol'), ('status', 'status'),
                           ('statusText', 'status_text')))
    r.timing = Timing.FromDevToolsDict(redirect_response['timing'])

    redirect_index = self._redirects_count_by_id[request_id]
    self._redirects_count_by_id[request_id] += 1
    r.request_id = '%s%s.%d' % (request_id, self._REDIRECT_SUFFIX,
                                 redirect_index + 1)
    initiator = {
        'type': 'redirect', Request.INITIATING_REQUEST: r.request_id}
    self._requests_in_flight[r.request_id] = (r, RequestTrack._STATUS_FINISHED)
    del self._requests_in_flight[request_id]
    self._FinalizeRequest(r.request_id)
    return initiator

  def _RequestServedFromCache(self, request_id, _):
    if request_id not in self._requests_in_flight:
      return
    (request, status) = self._requests_in_flight[request_id]
    assert status == RequestTrack._STATUS_SENT
    request.served_from_cache = True

  def _ResponseReceived(self, request_id, params):
    if request_id in self._completed_requests_by_id:
      assert request_id not in self._requests_in_flight
      return
    assert request_id in self._requests_in_flight
    (r, status) = self._requests_in_flight[request_id]
    if status == RequestTrack._STATUS_RESPONSE:
      # Duplicated messages (apart from the timestamp) are OK.
      old_params = self._request_id_to_response_received[request_id]
      params_copy = copy.deepcopy(params)
      params_copy['timestamp'] = None
      old_params['timestamp'] = None
      assert params_copy == old_params
      self.duplicates_count += 1
      return
    assert status == RequestTrack._STATUS_SENT
    assert (r.frame_id == params['frameId'] or
            params['response']['protocol'] == 'data')
    assert r.timestamp <= params['timestamp']
    if r.resource_type == 'Other':
      r.resource_type = params.get('type', 'Other')
    else:
      assert r.resource_type == params.get('type', 'Other')
    response = params['response']
    _CopyFromDictToObject(
        response, r, (('status', 'status'), ('mimeType', 'mime_type'),
                      ('fromDiskCache', 'from_disk_cache'),
                      ('fromServiceWorker', 'from_service_worker'),
                      ('protocol', 'protocol'), ('statusText', 'status_text'),
                      # Actual request headers are not known before reaching the
                      # network stack.
                      ('requestHeaders', 'request_headers'),
                      ('encodedDataLength', 'response_headers_length'),
                      ('headers', 'response_headers')))
    timing_dict = {}
    # Some URLs don't have a timing dict (e.g. data URLs), and timings for
    # cached requests are stale.
    # TODO(droger): the timestamp is inacurate, get the real timings instead.
    if not response.get('timing') or r.served_from_cache:
      timing_dict = {'requestTime': r.timestamp}
    else:
      timing_dict = response['timing']
    r.timing = Timing.FromDevToolsDict(timing_dict)
    self._requests_in_flight[request_id] = (r, RequestTrack._STATUS_RESPONSE)
    self._request_id_to_response_received[request_id] = params

  def _DataReceived(self, request_id, params):
    if request_id not in self._requests_in_flight:
      return
    (r, status) = self._requests_in_flight[request_id]
    assert (status == RequestTrack._STATUS_RESPONSE
            or status == RequestTrack._STATUS_DATA)
    offset = r._TimestampOffsetFromStartMs(params['timestamp'])
    r.data_chunks.append((offset, params['encodedDataLength']))
    self._requests_in_flight[request_id] = (r, RequestTrack._STATUS_DATA)

  def _LoadingFinished(self, request_id, params):
    if request_id not in self._requests_in_flight:
      return
    (r, status) = self._requests_in_flight[request_id]
    assert (status == RequestTrack._STATUS_RESPONSE
            or status == RequestTrack._STATUS_DATA)
    r.encoded_data_length = params['encodedDataLength']
    r.timing.loading_finished = r._TimestampOffsetFromStartMs(
        params['timestamp'])
    self._requests_in_flight[request_id] = (r, RequestTrack._STATUS_FINISHED)
    self._FinalizeRequest(request_id)

  def _LoadingFailed(self, request_id, params):
    if request_id not in self._requests_in_flight:
      logging.warning('An unknown request failed: %s' % request_id)
      return
    (r, _) = self._requests_in_flight[request_id]
    r.failed = True
    r.error_text = params['errorText']
    self._requests_in_flight[request_id] = (r, RequestTrack._STATUS_FINISHED)
    self._FinalizeRequest(request_id)

  def _FinalizeRequest(self, request_id):
    (request, status) = self._requests_in_flight[request_id]
    assert status == RequestTrack._STATUS_FINISHED
    del self._requests_in_flight[request_id]
    self._completed_requests_by_id[request_id] = request
    self._requests.append(request)

  def __eq__(self, o):
    return self._requests == o._requests


RequestTrack._METHOD_TO_HANDLER = {
    'Network.requestWillBeSent': RequestTrack._RequestWillBeSent,
    'Network.requestServedFromCache': RequestTrack._RequestServedFromCache,
    'Network.responseReceived': RequestTrack._ResponseReceived,
    'Network.dataReceived': RequestTrack._DataReceived,
    'Network.loadingFinished': RequestTrack._LoadingFinished,
    'Network.loadingFailed': RequestTrack._LoadingFailed}


def _CopyFromDictToObject(d, o, key_attrs):
  for (key, attr) in key_attrs:
    if key in d:
      setattr(o, attr, d[key])


if __name__ == '__main__':
  import json
  import sys
  events = json.load(open(sys.argv[1], 'r'))
  request_track = RequestTrack(None)
  for event in events:
    event_method = event['method']
    request_track.Handle(event_method, event)
