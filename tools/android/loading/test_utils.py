# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Common utilities used in unit tests, within this directory."""

import clovis_constants
import dependency_graph
import devtools_monitor
import loading_trace
import page_track
import request_track
import tracing_track
import user_satisfied_lens


class FakeRequestTrack(devtools_monitor.Track):
  def __init__(self, events):
    super(FakeRequestTrack, self).__init__(None)
    self._events = events
    for e in self._events:
      e.timing.request_time = e.timestamp

  def Handle(self, _method, _msg):
    assert False  # Should never be called.

  def GetEvents(self):
    return self._events

  def ToJsonDict(self):
    cls = request_track.RequestTrack
    return {cls._EVENTS_KEY: [
        rq.ToJsonDict() for rq in self.GetEvents()],
            cls._METADATA_KEY: {
                cls._DUPLICATES_KEY: 0,
                cls._INCONSISTENT_INITIATORS_KEY: 0}}


class FakePageTrack(devtools_monitor.Track):
  def __init__(self, events):
    super(FakePageTrack, self).__init__(None)
    self._events = events

  def Handle(self, _method, _msg):
    assert False  # Should never be called.

  def GetEvents(self):
    return self._events

  def GetMainFrameId(self):
    event = self._events[0]
    # Make sure our laziness is not an issue here.
    assert event['method'] == page_track.PageTrack.FRAME_STARTED_LOADING
    return event['frame_id']

  def ToJsonDict(self):
    return {'events': [event for event in self._events]}


def MakeRequestWithTiming(
    url, source_url, timing_dict, magic_content_type=False,
    initiator_type='other'):
  """Make a dependent request.

  Args:
    url: a url, or number which will be used as a url.
    source_url: a url or number which will be used as the source (initiating)
      url. If the source url is not present, then url will be a root. The
      convention in tests is to use a source_url of 'null' in this case.
    timing_dict: (dict) Suitable to be passed to request_track.Timing().
    initiator_type: the initiator type to use.

  Returns:
    A request_track.Request.
  """
  assert initiator_type in ('other', 'parser')
  timing = request_track.Timing.FromDevToolsDict(timing_dict)
  rq = request_track.Request.FromJsonDict({
      'timestamp': timing.request_time,
      'request_id': str(MakeRequestWithTiming._next_request_id),
      'url': 'http://' + str(url),
      'initiator': {'type': initiator_type, 'url': 'http://' + str(source_url)},
      'response_headers': {'Content-Type':
                           'null' if not magic_content_type
                           else 'magic-debug-content' },
      'timing': timing.ToJsonDict()
  })
  MakeRequestWithTiming._next_request_id += 1
  return rq


MakeRequestWithTiming._next_request_id = 0


def MakeRequest(
    url, source_url, start_time=None, headers_time=None, end_time=None,
    magic_content_type=False, initiator_type='other'):
  """Make a dependent request.

  Args:
    url: a url, or number which will be used as a url.
    source_url: a url or number which will be used as the source (initiating)
      url. If the source url is not present, then url will be a root. The
      convention in tests is to use a source_url of 'null' in this case.
    start_time: The request start time in milliseconds. If None, this is set to
      the current request id in seconds. If None, the two other time parameters
      below must also be None.
    headers_time: The timestamp when resource headers were received, or None.
    end_time: The timestamp when the resource was finished, or None.
    magic_content_type (bool): if true, set a magic content type that makes url
      always be detected as a valid source and destination request.
    initiator_type: the initiator type to use.

  Returns:
    A request_track.Request.
  """
  assert ((start_time is None and
           headers_time is None and
           end_time is None) or
          (start_time is not None and
           headers_time is not None and
           end_time is not None)), \
      'Need no time specified or all times specified'
  if start_time is None:
    # Use the request id in seconds for timestamps. This guarantees increasing
    # times which makes request dependencies behave as expected.
    start_time = headers_time = end_time = (
        MakeRequestWithTiming._next_request_id * 1000)
  timing_dict = {
      # connectEnd should be ignored.
      'connectEnd': (end_time - start_time) / 2,
      'receiveHeadersEnd': headers_time - start_time,
      'loadingFinished': end_time - start_time,
      'requestTime': start_time / 1000.0}
  return MakeRequestWithTiming(
      url, source_url, timing_dict, magic_content_type, initiator_type)


def LoadingTraceFromEvents(requests, page_events=None, trace_events=None):
  """Returns a LoadingTrace instance from various events."""
  request = FakeRequestTrack(requests)
  page_event_track = FakePageTrack(page_events if page_events else [])
  if trace_events is not None:
    track = tracing_track.TracingTrack(None,
        clovis_constants.DEFAULT_CATEGORIES)
    track.Handle('Tracing.dataCollected',
                 {'params': {'value': [e for e in trace_events]}})
  else:
    track = None
  return loading_trace.LoadingTrace(
      None, None, page_event_track, request, track)


class SimpleLens(object):
  """A simple replacement for RequestDependencyLens.

  Uses only the initiator url of a request for determining a dependency.
  """
  def __init__(self, trace):
    self._trace = trace

  def GetRequestDependencies(self):
    url_to_rq = {}
    deps = []
    for rq in self._trace.request_track.GetEvents():
      assert rq.url not in url_to_rq
      url_to_rq[rq.url] = rq
    for rq in self._trace.request_track.GetEvents():
      initiating_url = rq.initiator['url']
      if initiating_url in url_to_rq:
        deps.append((url_to_rq[initiating_url], rq, rq.initiator['type']))
    return deps


class TestDependencyGraph(dependency_graph.RequestDependencyGraph):
  """A dependency graph created from requests using a simple lens."""
  def __init__(self, requests):
    lens = SimpleLens(LoadingTraceFromEvents(requests))
    super(TestDependencyGraph, self).__init__(requests, lens)


class MockConnection(object):
  """Mock out connection for testing.

  Use Expect* for requests expecting a repsonse. SyncRequestNoResponse puts
  requests into no_response_requests_seen.

  TODO(mattcary): use a standard mock system (the back-ported python3
  unittest.mock? devil.utils.mock_calls?)

  """
  def __init__(self, test_case):
    # List of (method, params) tuples.
    self.no_response_requests_seen = []

    self._test_case = test_case
    self._expected_responses = {}

  def ExpectSyncRequest(self, response, method, params=None):
    """Test method when the connection is expected to make a SyncRequest.

    Args:
      response: (dict) the response to generate.
      method: (str) the expected method in the call.
      params: (dict) the expected params in the call.
    """
    self._expected_responses.setdefault(method, []).append((params, response))

  def AllExpectationsUsed(self):
    """Returns true when all expectations where used."""
    return not self._expected_responses

  def SyncRequestNoResponse(self, method, params):
    """Mocked method."""
    self.no_response_requests_seen.append((method, params))

  def SyncRequest(self, method, params=None):
    """Mocked method."""
    expected_params, response = self._expected_responses[method].pop(0)
    if not self._expected_responses[method]:
      del self._expected_responses[method]
    self._test_case.assertEqual(expected_params, params)
    return response


class MockUserSatisfiedLens(user_satisfied_lens._FirstEventLens):
  def _CalculateTimes(self, _):
    self._satisfied_msec = float('inf')
    self._event_msec = float('inf')


class TraceCreator(object):
  def __init__(self):
    self._request_index = 1

  def RequestAt(self, timestamp_msec, duration=1, frame_id=None):
    timestamp_sec = float(timestamp_msec) / 1000
    rq = request_track.Request.FromJsonDict({
        'url': 'http://bla-%s-.com' % timestamp_msec,
        'document_url': 'http://bla.com',
        'request_id': '0.%s' % self._request_index,
        'frame_id': frame_id or '123.%s' % timestamp_msec,
        'initiator': {'type': 'other'},
        'timestamp': timestamp_sec,
        'timing': {'request_time': timestamp_sec,
                   'loading_finished': duration},
        'status': 200})
    self._request_index += 1
    return rq

  def CreateTrace(self, requests, events, main_frame_id):
    page_event = {'method': 'Page.frameStartedLoading',
                  'frame_id': main_frame_id}
    trace = LoadingTraceFromEvents(
        requests, trace_events=events, page_events=[page_event])
    trace.tracing_track.SetMainFrameID(main_frame_id)
    return trace
