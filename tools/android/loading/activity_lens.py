# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Gives a picture of the CPU activity between timestamps.

When executed as a script, takes a loading trace, and prints the activity
breakdown for the request dependencies.
"""

import collections
import logging
import operator

import request_track


class ActivityLens(object):
  """Reconstructs the activity of the main renderer thread between requests."""
  _SCRIPT_EVENT_NAMES = ('EvaluateScript', 'FunctionCall')
  _PARSING_EVENT_NAMES = ('ParseHTML', 'ParseAuthorStyleSheet')

  def __init__(self, trace):
    """Initializes an instance of ActivityLens.

    Args:
      trace: (LoadingTrace) loading trace.
    """
    self._trace = trace
    events = trace.tracing_track.GetEvents()
    self._renderer_main_pid_tid = self._GetRendererMainThreadId(events)
    self._tracing = self._trace.tracing_track.Filter(
        *self._renderer_main_pid_tid)

  @classmethod
  def _GetRendererMainThreadId(cls, events):
    """Returns the most active main renderer thread.

    Several renderers may be running concurrently, but we assume that only one
    of them is busy during the time covered by the loading trace.. It can be
    selected by looking at the number of trace events generated.

    Args:
      events: [tracing.Event] List of trace events.

    Returns:
      (PID (int), TID (int)) of the busiest renderer main thread.
    """
    events_count_per_pid_tid = collections.defaultdict(int)
    main_renderer_thread_ids = set()
    for event in events:
      tracing_event = event.tracing_event
      pid = event.tracing_event['pid']
      tid = event.tracing_event['tid']
      events_count_per_pid_tid[(pid, tid)] += 1
      if (tracing_event['cat'] == '__metadata'
          and tracing_event['name'] == 'thread_name'
          and event.args['name'] == 'CrRendererMain'):
        main_renderer_thread_ids.add((pid, tid))
    events_count_per_pid_tid = {
        pid_tid: count for (pid_tid, count) in events_count_per_pid_tid.items()
        if pid_tid in main_renderer_thread_ids}
    pid_tid_events_counts = sorted(events_count_per_pid_tid.items(),
                                   key=operator.itemgetter(1), reverse=True)
    if (len(pid_tid_events_counts) > 1
        and pid_tid_events_counts[0][1] < 2 * pid_tid_events_counts[1][1]):
      logging.warning(
          'Several active renderers (%d and %d with %d and %d events).'
          % (pid_tid_events_counts[0][0][0], pid_tid_events_counts[1][0][0],
             pid_tid_events_counts[0][1], pid_tid_events_counts[1][1]))
    return pid_tid_events_counts[0][0]

  def _OverlappingMainRendererThreadEvents(self, start_msec, end_msec):
    return self._tracing.OverlappingEvents(start_msec, end_msec)

  @classmethod
  def _ClampedDuration(cls, event, start_msec, end_msec):
      return max(0, (min(end_msec, event.end_msec)
                     - max(start_msec, event.start_msec)))

  @classmethod
  def _ThreadBusyness(cls, events, start_msec, end_msec):
    """Amount of time a thread spent executing from the message loop."""
    busy_duration = 0
    message_loop_events = [
        e for e in events
        if (e.tracing_event['cat'] == 'toplevel'
            and e.tracing_event['name'] == 'MessageLoop::RunTask')]
    for event in message_loop_events:
      clamped_duration = cls._ClampedDuration(event, start_msec, end_msec)
      busy_duration += clamped_duration
    interval_msec = end_msec - start_msec
    assert busy_duration <= interval_msec
    return busy_duration

  @classmethod
  def _ScriptsExecuting(cls, events, start_msec, end_msec):
    """Returns the time during which scripts executed within an interval.

    Args:
      events: ([tracing.Event]) list of tracing events.
      start_msec: (float) start time in ms, inclusive.
      end_msec: (float) end time in ms, inclusive.

    Returns:
      A dict {URL (str) -> duration_msec (float)}. The dict may have a None key
      for scripts that aren't associated with a URL.
    """
    script_to_duration = collections.defaultdict(float)
    script_events = [e for e in events
                     if ('devtools.timeline' in e.tracing_event['cat']
                         and (e.tracing_event['name']
                              in cls._SCRIPT_EVENT_NAMES))]
    for event in script_events:
      clamped_duration = cls._ClampedDuration(event, start_msec, end_msec)
      script_url = event.args['data'].get('scriptName', None)
      script_to_duration[script_url] += clamped_duration
    return dict(script_to_duration)

  @classmethod
  def _FullyIncludedEvents(cls, events, event):
    """Return a list of events wholly included in the |event| span."""
    (start, end) = (event.start_msec, event.end_msec)
    result = []
    for event in events:
      if start <= event.start_msec < end and start <= event.end_msec < end:
        result.append(event)
    return result

  @classmethod
  def _Parsing(cls, events, start_msec, end_msec):
    """Returns the HTML/CSS parsing time within an interval.

    Args:
      events: ([tracing.Event]) list of events.
      start_msec: (float) start time in ms, inclusive.
      end_msec: (float) end time in ms, inclusive.

    Returns:
      A dict {URL (str) -> duration_msec (float)}. The dict may have a None key
      for tasks that aren't associated with a URL.
    """
    url_to_duration = collections.defaultdict(float)
    parsing_events = [e for e in events
                      if ('devtools.timeline' in e.tracing_event['cat']
                          and (e.tracing_event['name']
                               in cls._PARSING_EVENT_NAMES))]
    for event in parsing_events:
      # Parsing events can contain nested script execution events, avoid
      # double-counting by discounting these.
      nested_events = cls._FullyIncludedEvents(events, event)
      events_tree = _EventsTree(event, nested_events)
      js_events = events_tree.DominatingEventsWithNames(cls._SCRIPT_EVENT_NAMES)
      duration_to_subtract = sum(
          cls._ClampedDuration(e, start_msec, end_msec) for e in js_events)
      tracing_event = event.tracing_event
      clamped_duration = cls._ClampedDuration(event, start_msec, end_msec)
      if tracing_event['name'] == 'ParseAuthorStyleSheet':
        url = tracing_event['args']['data']['styleSheetUrl']
      else:
        url = tracing_event['args']['beginData']['url']
      parsing_duration = clamped_duration - duration_to_subtract
      assert parsing_duration >= 0
      url_to_duration[url] += parsing_duration
    return dict(url_to_duration)

  def GenerateEdgeActivity(self, dep):
    """For a dependency between two requests, returns the renderer activity
    breakdown.

    Args:
      dep: (Request, Request, str) As returned from
           RequestDependencyLens.GetRequestDependencies().

    Returns:
      {'edge_cost': (float) ms, 'busy': (float) ms,
       'parsing': {'url' -> time_ms}, 'script' -> {'url' -> time_ms}}
    """
    (first, second, reason) = dep
    (start_msec, end_msec) = request_track.IntervalBetween(
        first, second, reason)
    assert end_msec - start_msec >= 0.
    events = self._OverlappingMainRendererThreadEvents(start_msec, end_msec)
    result = {'edge_cost': end_msec - start_msec,
              'busy': self._ThreadBusyness(events, start_msec, end_msec)}
    result.update(self.ComputeActivity(start_msec, end_msec))
    return result

  def ComputeActivity(self, start_msec, end_msec):
    """Returns a breakdown of the main renderer thread activity between two
    timestamps.

    Args:
      start_msec: (float)
      end_msec: (float)

    Returns:
       {'parsing': {'url' -> time_ms}, 'script': {'url' -> time_ms}}.
    """
    assert end_msec - start_msec >= 0.
    events = self._OverlappingMainRendererThreadEvents(start_msec, end_msec)
    result = {'parsing': self._Parsing(events, start_msec, end_msec),
              'script': self._ScriptsExecuting(events, start_msec, end_msec)}
    return result

  def BreakdownEdgeActivityByInitiator(self, dep):
    """For a dependency between two requests, categorizes the renderer activity.

    Args:
      dep: (Request, Request, str) As returned from
           RequestDependencyLens.GetRequestDependencies().

    Returns:
      {'script': float, 'parsing': float, 'other_url': float,
       'unknown_url': float, 'unrelated_work': float}
      where the values are durations in ms:
      - idle: The renderer main thread was idle.
      - script: The initiating file was executing.
      - parsing: The initiating file was being parsed.
      - other_url: Other scripts and/or parsing activities.
      - unknown_url: Activity which is not associated with a URL.
      - unrelated_work: Activity unrelated to scripts or parsing.
    """
    activity = self.GenerateEdgeActivity(dep)
    breakdown = {'unrelated_work': activity['busy'],
                 'idle': activity['edge_cost'] - activity['busy'],
                 'script': 0, 'parsing': 0,
                 'other_url': 0, 'unknown_url': 0}
    for kind in ('script', 'parsing'):
      for (script_name, duration_ms) in activity[kind].items():
        if not script_name:
          breakdown['unknown_url'] += duration_ms
        elif script_name == dep[0].url:
          breakdown[kind] += duration_ms
        else:
          breakdown['other_url'] += duration_ms
    breakdown['unrelated_work'] -= sum(
        breakdown[x] for x in ('script', 'parsing', 'other_url', 'unknown_url'))
    return breakdown

  def MainRendererThreadBusyness(self, start_msec, end_msec):
    """Returns the amount of time the main renderer thread was busy.

    Args:
      start_msec: (float) Start of the interval.
      end_msec: (float) End of the interval.
    """
    events = self._OverlappingMainRendererThreadEvents(start_msec, end_msec)
    return self._ThreadBusyness(events, start_msec, end_msec)


class _EventsTree(object):
  """Builds the hierarchy of events from a list of fully nested events."""
  def __init__(self, root_event, events):
    """Creates the tree.

    Args:
      root_event: (Event) Event held by the tree root.
      events: ([Event]) List of events that are fully included in |root_event|.
    """
    self.event = root_event
    self.start_msec = root_event.start_msec
    self.end_msec = root_event.end_msec
    self.children = []
    events.sort(key=operator.attrgetter('start_msec'))
    if not events:
      return
    current_child = (events[0], [])
    for event in events[1:]:
      if event.end_msec < current_child[0].end_msec:
        current_child[1].append(event)
      else:
        self.children.append(_EventsTree(current_child[0], current_child[1]))
        current_child = (event, [])
    self.children.append(_EventsTree(current_child[0], current_child[1]))

  def DominatingEventsWithNames(self, names):
    """Returns a list of the top-most events in the tree with a matching name.
    """
    if self.event.name in names:
      return [self.event]
    else:
      result = []
      for child in self.children:
        result += child.DominatingEventsWithNames(names)
      return result


if __name__ == '__main__':
  import sys
  import json
  import loading_trace
  import request_dependencies_lens

  filename = sys.argv[1]
  json_dict = json.load(open(filename))
  loading_trace = loading_trace.LoadingTrace.FromJsonDict(json_dict)
  activity_lens = ActivityLens(loading_trace)
  dependencies_lens = request_dependencies_lens.RequestDependencyLens(
      loading_trace)
  deps = dependencies_lens.GetRequestDependencies()
  for requests_dep in deps:
    print activity_lens.GenerateEdgeActivity(requests_dep)
