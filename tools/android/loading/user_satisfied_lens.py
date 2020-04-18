# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Identifies key events related to user satisfaction.

Several lenses are defined, for example FirstTextPaintLens and
FirstSignificantPaintLens.

When run from the command line, takes a lens name and a trace, and prints the
fingerprints of the critical resources to stdout.
"""
import logging
import operator

import common_util


class _UserSatisfiedLens(object):
  """A base class for all user satisfaction metrics.

  All of these work by identifying a user satisfaction event from the trace, and
  then building a set of request ids whose loading is needed to achieve that
  event. Subclasses need only provide the time computation. The base class will
  use that to construct the request ids.
  """
  _ATTRS = ['_satisfied_msec', '_event_msec', '_postload_msec',
            '_critical_request_ids']

  def CriticalRequests(self):
    """Critical requests.

    Returns:
      A sequence of request_track.Request objects representing an estimate of
      all requests that are necessary for the user satisfaction defined by this
      class.
    """
    raise NotImplementedError

  def CriticalRequestIds(self):
    """Ids of critical requests."""
    return set(rq.request_id for rq in self.CriticalRequests())

  def CriticalFingerprints(self):
    """Fingerprints of critical requests."""
    return set(rq.fingerprint for rq in self.CriticalRequests())

  def PostloadTimeMsec(self):
    """Return postload time.

    The postload time is an estimate of the amount of time needed by chrome to
    transform the critical results into the satisfying event.

    Returns:
      Postload time in milliseconds.
    """
    return 0

  def SatisfiedMs(self):
    """Returns user satisfied timestamp, in ms.

    This is *not* a unix timestamp. It is relative to the same point in time
    as the request_time field in request_track.Timing.
    """
    return self._satisfied_msec

  @classmethod
  def RequestsBefore(cls, request_track, time_ms):
    return [rq for rq in request_track.GetEvents()
            if rq.end_msec <= time_ms]


class PLTLens(_UserSatisfiedLens):
  """A lens built using page load time (PLT) as the metric of user satisfaction.
  """
  def __init__(self, trace):
    self._satisfied_msec = PLTLens._ComputePlt(trace)
    self._critical_requests = _UserSatisfiedLens.RequestsBefore(
        trace.request_track, self._satisfied_msec)

  def CriticalRequests(self):
    return self._critical_requests

  @classmethod
  def _ComputePlt(cls, trace):
    mark_load_events = trace.tracing_track.GetMatchingEvents(
        'devtools.timeline', 'MarkLoad')
    # Some traces contain several load events for the main frame.
    main_frame_load_events = filter(
        lambda e: e.args['data']['isMainFrame'], mark_load_events)
    if main_frame_load_events:
      return max(e.start_msec for e in main_frame_load_events)
    # Main frame onLoad() didn't finish. Take the end of the last completed
    # request.
    return max(r.end_msec or -1 for r in trace.request_track.GetEvents())


class RequestFingerprintLens(_UserSatisfiedLens):
  """A lens built using requests in a trace that match a set of fingerprints."""
  def __init__(self, trace, fingerprints):
    fingerprints = set(fingerprints)
    self._critical_requests = [rq for rq in trace.request_track.GetEvents()
                               if rq.fingerprint in fingerprints]

  def CriticalRequests(self):
    """Ids of critical requests."""
    return set(self._critical_requests)


class _FirstEventLens(_UserSatisfiedLens):
  """Helper abstract subclass that defines users first event manipulations."""
  # pylint can't handle abstract subclasses.
  # pylint: disable=abstract-method

  def __init__(self, trace):
    """Initialize the lens.

    Args:
      trace: (LoadingTrace) the trace to use in the analysis.
    """
    self._satisfied_msec = None
    self._event_msec = None
    self._postload_msec = None
    self._critical_request_ids = None
    if trace is None:
      return
    self._CalculateTimes(trace)
    self._critical_requests = _UserSatisfiedLens.RequestsBefore(
        trace.request_track, self._satisfied_msec)
    self._critical_request_ids = set(rq.request_id
                                     for rq in self._critical_requests)
    if self._critical_requests:
      last_load = max(rq.end_msec for rq in self._critical_requests)
    else:
      last_load = float('inf')
    self._postload_msec = self._event_msec - last_load

  def CriticalRequests(self):
    """Override."""
    return self._critical_requests

  def PostloadTimeMsec(self):
    """Override."""
    return self._postload_msec

  def ToJsonDict(self):
    return common_util.SerializeAttributesToJsonDict({}, self, self._ATTRS)

  @classmethod
  def FromJsonDict(cls, json_dict):
    result = cls(None)
    return common_util.DeserializeAttributesFromJsonDict(
        json_dict, result, cls._ATTRS)

  def _CalculateTimes(self, trace):
    """Subclasses should implement to set _satisfied_msec and _event_msec."""
    raise NotImplementedError

  @classmethod
  def _CheckCategory(cls, tracing_track, category):
    assert category in tracing_track.Categories(), (
        'The "%s" category must be enabled.' % category)

  @classmethod
  def _ExtractBestTiming(cls, times):
    if not times:
      return float('inf')
    assert len(times) == 1, \
        'Unexpected duplicate {}: {} with spread of {}'.format(
            str(cls), len(times), max(times) - min(times))
    return float(max(times))


class FirstTextPaintLens(_FirstEventLens):
  """Define satisfaction by the first text paint.

  This event is taken directly from a trace.
  """
  _EVENT_CATEGORY = 'blink.user_timing'
  def _CalculateTimes(self, trace):
    self._CheckCategory(trace.tracing_track, self._EVENT_CATEGORY)
    first_paints = [
        e.start_msec for e in trace.tracing_track.GetMatchingMainFrameEvents(
            'blink.user_timing', 'firstPaint')]
    self._satisfied_msec = self._event_msec = \
        self._ExtractBestTiming(first_paints)


class FirstContentfulPaintLens(_FirstEventLens):
  """Define satisfaction by the first contentful paint.

  This event is taken directly from a trace. Internally to chrome it's computed
  by filtering out things like background paint from firstPaint.
  """
  _EVENT_CATEGORY = 'blink.user_timing'
  def _CalculateTimes(self, trace):
    self._CheckCategory(trace.tracing_track, self._EVENT_CATEGORY)
    first_paints = [
        e.start_msec for e in trace.tracing_track.GetMatchingMainFrameEvents(
            'blink.user_timing', 'firstContentfulPaint')]
    self._satisfied_msec = self._event_msec = \
       self._ExtractBestTiming(first_paints)


class FirstSignificantPaintLens(_FirstEventLens):
  """Define satisfaction by the first paint after a big layout change.

  Our satisfaction time is that of the layout change, as all resources must have
  been loaded to compute the layout. Our event time is that of the next paint as
  that is the observable event.
  """
  _FIRST_LAYOUT_COUNTER = 'LayoutObjectsThatHadNeverHadLayout'
  _EVENT_CATEGORIES = ['blink', 'disabled-by-default-blink.debug.layout']
  def _CalculateTimes(self, trace):
    for cat in self._EVENT_CATEGORIES:
      self._CheckCategory(trace.tracing_track, cat)
    paint_tree_times = []
    layouts = []  # (layout item count, msec).
    for e in trace.tracing_track.GetEvents():
      if ('frame' in e.args and
          e.args['frame'] != trace.tracing_track.GetMainFrameID()):
        continue
      # If we don't know have a frame id, we assume it applies to all events.

      if e.Matches('blink', 'FrameView::paintTree'):
        paint_tree_times.append(e.start_msec)
      if ('counters' in e.args and
          self._FIRST_LAYOUT_COUNTER in e.args['counters']):
        layouts.append((e.args['counters'][self._FIRST_LAYOUT_COUNTER],
                        e.start_msec))
    assert layouts, 'No layout events'
    assert paint_tree_times,'No paintTree times'
    layouts.sort(key=operator.itemgetter(0), reverse=True)
    self._satisfied_msec = layouts[0][1]
    self._event_msec = min(t for t in paint_tree_times
                           if t > self._satisfied_msec)


def main(lens_name, trace_file):
  assert (lens_name in globals() and
          not lens_name.startswith('_') and
          lens_name.endswith('Lens')), 'Bad lens %s' % lens_name
  lens_cls = globals()[lens_name]
  trace = loading_trace.LoadingTrace.FromJsonFile(trace_file)
  lens = lens_cls(trace)
  for fp in sorted(lens.CriticalFingerprints()):
    print fp


if __name__ == '__main__':
  import sys
  import loading_trace
  main(sys.argv[1], sys.argv[2])
