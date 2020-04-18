# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Gathers and infers dependencies between requests.

When executed as a script, loads a trace and outputs synthetic frame load nodes
and the new introduced dependencies.
"""

import bisect
import collections
import logging
import operator

import loading_trace


class FrameLoadLens(object):
  """Analyses and creates request dependencies for inferred frame events."""
  _FRAME_EVENT = 'RenderFrameImpl::didFinishLoad'
  _REQUEST_TO_LOAD_GAP_MSEC = 100
  _LOAD_TO_REQUEST_GAP_MSEC = 100
  def __init__(self, trace):
    """Instance initialization.

    Args:
      trace: (LoadingTrace) Loading trace.
    """
    self._frame_load_events = self._GetFrameLoadEvents(trace.tracing_track)
    self._request_track = trace.request_track
    self._tracing_track = trace.tracing_track
    self._load_dependencies = []
    self._request_dependencies = []
    for i, load in enumerate(self._frame_load_events):
      self._load_dependencies.extend(
          [(i, r) for r in self._GetLoadDependencies(load)])
      self._request_dependencies.extend(
          [(r, i) for r in self._GetRequestDependencies(load)])

  def GetFrameLoadInfo(self):
    """Returns [(index, msec)]."""
    return [collections.namedtuple('LoadInfo', ['index', 'msec'])._make(
        (i, self._frame_load_events[i].start_msec))
            for i in xrange(len(self._frame_load_events))]

  def GetFrameResourceComplete(self, request_track):
    """Returns [(frame id, msec)]."""
    frame_to_end_msec = collections.defaultdict(int)
    for r in request_track.GetEvents():
      if r.end_msec > frame_to_end_msec[r.frame_id]:
        frame_to_end_msec[r.frame_id] = r.end_msec
    loads = []
    for f in sorted(frame_to_end_msec.keys()):
      loads.append((f, frame_to_end_msec[f]))
    return loads

  def GetFrameLoadDependencies(self):
    """Returns a list of frame load dependencies.

    Returns:
      ([(frame load index, request), ...],
       [(request, frame load index), ...]), where request are instances of
      request_trace.Request, and frame load index is an integer. The first list
      in the tuple gives the requests that are dependent on the given frame
      load, and the second list gives the frame loads that are dependent on the
      given request.
    """
    return (self._load_dependencies, self._request_dependencies)

  def _GetFrameLoadEvents(self, tracing_track):
    events = []
    for e in tracing_track.GetEvents():
      if e.tracing_event['name'] == self._FRAME_EVENT:
        events.append(e)
    return events

  def _GetLoadDependencies(self, load):
    for r in self._request_track.GetEventsStartingBetween(
        load.start_msec, load.start_msec + self._LOAD_TO_REQUEST_GAP_MSEC):
      yield r

  def _GetRequestDependencies(self, load):
    for r in self._request_track.GetEventsEndingBetween(
        load.start_msec - self._REQUEST_TO_LOAD_GAP_MSEC, load.start_msec):
      yield r


if __name__ == '__main__':
  import loading_trace
  import json
  import sys
  lens = FrameLoadLens(loading_trace.LoadingTrace.FromJsonDict(
      json.load(open(sys.argv[1]))))
  load_times = lens.GetFrameLoadInfo()
  for t in load_times:
    print t
  print (lens._request_track.GetFirstRequestMillis(),
         lens._request_track.GetLastRequestMillis())
  load_dep, request_dep = lens.GetFrameLoadDependencies()
  rq_str = lambda r: '%s (%d-%d)' % (
      r.request_id,
      r.start_msec - lens._request_track.GetFirstRequestMillis(),
      r.end_msec - lens._request_track.GetFirstRequestMillis())
  load_str = lambda i: '%s (%d)' % (i, load_times[i][1])
  for load_idx, request in load_dep:
    print '%s -> %s' % (load_str(load_idx), rq_str(request))
  for request, load_idx in request_dep:
    print '%s -> %s' % (rq_str(request), load_str(load_idx))
