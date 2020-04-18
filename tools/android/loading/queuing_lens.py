# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Lens for resource load queuing.

When executed as a script, takes a loading trace and prints queuing information
for each request.
"""

import collections
import itertools
import logging

import clovis_constants


class QueuingLens(object):
  """Attaches queuing related trace events to request objects."""
  QUEUING_CATEGORY = clovis_constants.QUEUING_CATEGORY
  ASYNC_NAME = 'ScheduledResourceRequest'
  READY_NAME = 'ScheduledResourceRequest.Ready'
  SET_PRIORITY_NAME = 'ScheduledResourceRequest.SetPriority'
  QUEUING_NAMES = set([ASYNC_NAME,
                       READY_NAME,
                       SET_PRIORITY_NAME])

  IN_FLIGHT_NAME = 'ResourceScheduler::Client.InFlightRequests'
  SHOULD_START_NAME = 'ResourceScheduler::Client::ShouldStartRequestInfo'

  def __init__(self, trace):
    self._request_track = trace.request_track
    self._queuing_events_by_id = self._GetQueuingEvents(trace.tracing_track)
    self._source_id_to_url = {}
    for source_id, events in self._queuing_events_by_id.iteritems():
      self._source_id_to_url[source_id] = self._GetQueuingEventUrl(events)

  def GenerateRequestQueuing(self):
    """Computes queuing information for each request.

    We determine blocking requests by looking at which urls are in-flight
    (created but not yet destroyed) at the time of the creation of each
    request. This means that a request that we list as blocking may just be
    queued (throttled) at the same time as our request, and not actually
    blocking.

    The lifetime of the queuing events extends from when a resource is first
    slotted into the sytem until the request is complete. The main interesting
    queuing events are begin, end (which define the lifetime) and ready, an
    instant event that is usually within a millisecond after the request_time of
    the Request.

    Returns:
      {request_track.Request:
         (start_msec: throttle start, end_msec: throttle end,
          ready_msec: ready,
          blocking: [blocking requests],
          source_ids: [source ids of the request])}, where the map values are
      a named tuple with the specified fields.
    """
    url_to_requests = collections.defaultdict(list)
    for rq in self._request_track.GetEvents():
      url_to_requests[rq.url].append(rq)
    # Queuing events are organized by source id, which corresponds to a load of
    # a url. First collect timing information for each source id, then associate
    # with each request.
    timing_by_source_id = {}
    for source_id, events in self._queuing_events_by_id.iteritems():
      assert all(e.end_msec is None for e in events), \
          'Unexpected end_msec for nested async queuing events'
      ready_times = [e.start_msec for e in events if e.name == self.READY_NAME]
      if not ready_times:
        ready_msec = None
      else:
        assert len(ready_times) == 1, events
        ready_msec = ready_times[0]
      timing_by_source_id[source_id] = (
          min(e.start_msec for e in events),
          max(e.start_msec for e in events),
          ready_msec)
    queue_info = {}
    for request_url, requests in url_to_requests.iteritems():
      matching_source_ids = set(
          source_id for source_id, url in self._source_id_to_url.iteritems()
          if url == request_url)
      if len(matching_source_ids) > 1:
        logging.warning('Multiple matching source ids, probably duplicated'
                        'urls: %s', [rq.url for rq in requests])
      # Get first source id.
      sid = next(s for s in matching_source_ids) \
          if matching_source_ids else None
      (throttle_start_msec, throttle_end_msec, ready_msec) = \
         timing_by_source_id[sid] if matching_source_ids else (-1, -1, -1)

      blocking_requests = []
      for sid, (flight_start_msec,
                flight_end_msec, _)  in timing_by_source_id.iteritems():
        if (flight_start_msec < throttle_start_msec and
            flight_end_msec > throttle_start_msec and
            flight_end_msec < throttle_end_msec):
          blocking_requests.extend(
              url_to_requests.get(self._source_id_to_url[sid], []))

      info = collections.namedtuple(
          'QueueInfo', ['start_msec', 'end_msec', 'ready_msec', 'blocking'
                        'source_ids'])
      info.start_msec = throttle_start_msec
      info.end_msec = throttle_end_msec
      info.ready_msec = ready_msec
      current_request_ids = set(rq.request_id for rq in requests)
      info.blocking = [b for b in blocking_requests
                       if b is not None and
                       b.request_id not in current_request_ids]
      info.source_ids = matching_source_ids
      for rq in requests:
        queue_info[rq] = info
    return queue_info

  def _GetQueuingEvents(self, tracing_track):
    events = collections.defaultdict(list)
    for e in tracing_track.GetEvents():
      if (e.category == self.QUEUING_CATEGORY and
          e.name in self.QUEUING_NAMES):
        events[e.args['data']['source_id']].append(e)
    return events

  def _GetQueuingEventUrl(self, events):
    urls = set()
    for e in events:
      if 'request_url' in e.args['data']:
        urls.add(e.args['data']['request_url'])
    assert len(urls) == 1, urls
    return urls.pop()

  def _GetEventsForRequest(self, request):
    request_events = []
    for source_id, url in self._source_id_to_url:
      if url == request.url:
        request_events.extend(self._queuing_events_by_id[source_id])
    return request_events


def _Main(trace_file):
  import loading_trace
  trace = loading_trace.LoadingTrace.FromJsonFile(trace_file)
  lens = QueuingLens(trace)
  queue_info = lens.GenerateRequestQueuing()
  base_msec = trace.request_track.GetFirstRequestMillis()
  mkmsec = lambda ms: ms - base_msec if ms > 0 else -1
  for rq, info in queue_info.iteritems():
    print '{fp} ({ts}->{te})[{rs}->{re}] {ids} {url}'.format(
        fp=rq.fingerprint,
        ts=mkmsec(info.start_msec), te=mkmsec(info.end_msec),
        rs=mkmsec(rq.start_msec), re=mkmsec(rq.end_msec),
        ids=info.source_ids, url=rq.url)
    for blocking_request in info.blocking:
      print '  {} {}'.format(blocking_request.fingerprint, blocking_request.url)

if __name__ == '__main__':
  import sys
  _Main(sys.argv[1])
