# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Monitor tracing events on chrome via chrome remote debugging."""

import itertools
import logging
import operator

import clovis_constants
import devtools_monitor


class TracingTrack(devtools_monitor.Track):
  """Grabs and processes trace event messages.

  See https://goo.gl/Qabkqk for details on the protocol.
  """
  def __init__(self, connection, categories, fetch_stream=False):
    """Initialize this TracingTrack.

    Args:
      connection: a DevToolsConnection.
      categories: ([str] or None) If set, a list of categories to enable or
                  disable in Chrome tracing. Categories prefixed with '-' are
                  disabled.
      fetch_stream: if true, use a websocket stream to fetch tracing data rather
        than dataCollected events. It appears based on very limited testing that
        a stream is slower than the default reporting as dataCollected events.
    """
    super(TracingTrack, self).__init__(connection)
    if connection:
      connection.RegisterListener('Tracing.dataCollected', self)

    self._categories = set(categories)
    params = {}
    params['categories'] = ','.join(self._categories)
    if fetch_stream:
      params['transferMode'] = 'ReturnAsStream'

    if connection:
      connection.SyncRequestNoResponse('Tracing.start', params)

    self._events = []
    self._base_msec = None
    self._interval_tree = None
    self._main_frame_id = None

  def Handle(self, method, event):
    for e in event['params']['value']:
      event = Event(e)
      self._events.append(event)
      if self._base_msec is None or event.start_msec < self._base_msec:
        self._base_msec = event.start_msec
    # Invalidate our index rather than trying to be fancy and incrementally
    # update.
    self._interval_tree = None

  def Categories(self):
    """Returns the set of categories in this trace."""
    return self._categories

  def GetFirstEventMillis(self):
    """Find the canonical start time for this track.

    Returns:
      The millisecond timestamp of the first request.
    """
    return self._base_msec

  def GetEvents(self):
    """Returns a list of tracing.Event. Not sorted."""
    return self._events

  def GetMatchingEvents(self, category, name):
    """Gets events matching |category| and |name|."""
    return [e for e in self.GetEvents() if e.Matches(category, name)]

  def GetMatchingMainFrameEvents(self, category, name):
    """Gets events matching |category| and |name| that occur in the main frame.

    Events without a 'frame' key in their |args| are discarded.
    """
    matching_events = self.GetMatchingEvents(category, name)
    return [e for e in matching_events
        if 'frame' in e.args and e.args['frame'] == self.GetMainFrameID()]

  def GetMainFrameRoutingID(self):
    """Returns the main frame routing ID."""
    for event in self.GetMatchingEvents(
        'navigation', 'RenderFrameImpl::OnNavigate'):
      return event.args['id']
    assert False

  def GetMainFrameID(self):
    """Returns the main frame ID."""
    if not self._main_frame_id:
      navigation_start_events = self.GetMatchingEvents(
          'blink.user_timing', 'navigationStart')
      first_event = min(navigation_start_events, key=lambda e: e.start_msec)
      self._main_frame_id = first_event.args['frame']

    return self._main_frame_id

  def SetMainFrameID(self, frame_id):
    """Set the main frame ID. Normally this is used only for testing."""
    self._main_frame_id = frame_id

  def EventsAt(self, msec):
    """Gets events active at a timestamp.

    Args:
      msec: tracing milliseconds to query. Tracing milliseconds appears to be
        since chrome startup (ie, arbitrary epoch).

    Returns:
      List of events active at that timestamp. Instantaneous (ie, instant,
      sample and counter) events are never included. Event end times are
      exclusive, so that an event ending at the usec parameter will not be
      returned.
    """
    self._IndexEvents()
    return self._interval_tree.EventsAt(msec)

  def Filter(self, pid=None, tid=None, categories=None):
    """Returns a new TracingTrack with a subset of the events.

    Args:
      pid: (int or None) Selects events from this PID.
      tid: (int or None) Selects events from this TID.
      categories: (set([str]) or None) Selects events belonging to one of the
                  categories.
    """
    events = self._events
    if pid is not None:
      events = filter(lambda e : e.tracing_event['pid'] == pid, events)
    if tid is not None:
      events = filter(lambda e : e.tracing_event['tid'] == tid, events)
    if categories is not None:
      events = filter(
          lambda e : set(e.category.split(',')).intersection(categories),
          events)
    tracing_track = TracingTrack(None, clovis_constants.DEFAULT_CATEGORIES)
    tracing_track._events = events
    tracing_track._categories = self._categories
    if categories is not None:
      tracing_track._categories = self._categories.intersection(categories)
    return tracing_track

  def ToJsonDict(self):
    return {'categories': list(self._categories),
            'events': [e.ToJsonDict() for e in self._events]}

  @classmethod
  def FromJsonDict(cls, json_dict):
    if not json_dict:
      return None
    assert 'events' in json_dict
    events = [Event(e) for e in json_dict['events']]
    tracing_track = TracingTrack(None, clovis_constants.DEFAULT_CATEGORIES)
    tracing_track._categories = set(json_dict.get('categories', []))
    tracing_track._events = events
    tracing_track._base_msec = events[0].start_msec if events else 0
    for e in events[1:]:
      if e.type == 'M':
        continue  # No timestamp for metadata events.
      assert e.start_msec > 0
      if e.start_msec < tracing_track._base_msec:
        tracing_track._base_msec = e.start_msec
    return tracing_track

  def OverlappingEvents(self, start_msec, end_msec):
    self._IndexEvents()
    return self._interval_tree.OverlappingEvents(start_msec, end_msec)

  def EventsEndingBetween(self, start_msec, end_msec):
    """Gets the list of events ending within an interval.

    Args:
      start_msec: the start of the range to query, in milliseconds, inclusive.
      end_msec: the end of the range to query, in milliseconds, inclusive.

    Returns:
      See OverlappingEvents() above.
    """
    overlapping_events = self.OverlappingEvents(start_msec, end_msec)
    return [e for e in overlapping_events
            if start_msec <= e.end_msec <= end_msec]

  def EventFromStep(self, step_event):
    """Returns the Event associated with a step event, or None.

    Args:
      step_event: (Event) Step event.

    Returns:
      an Event that matches the step event, or None.
    """
    self._IndexEvents()
    assert 'step' in step_event.args and step_event.tracing_event['ph'] == 'T'
    candidates = self._interval_tree.EventsAt(step_event.start_msec)
    for event in candidates:
      # IDs are only unique within a process (often they are pointers).
      if (event.pid == step_event.pid and event.tracing_event['ph'] != 'T'
          and event.name == step_event.name and event.id == step_event.id):
        return event
    return None

  def _IndexEvents(self, strict=False):
    if self._interval_tree:
      return
    complete_events = []
    spanning_events = self._SpanningEvents()
    for event in self._events:
      if not event.IsIndexable():
        continue
      if event.IsComplete():
        complete_events.append(event)
        continue
      matched_event = spanning_events.Match(event, strict)
      if matched_event is not None:
        complete_events.append(matched_event)
    self._interval_tree = _IntervalTree.FromEvents(complete_events)

    if strict and spanning_events.HasPending():
      raise devtools_monitor.DevToolsConnectionException(
          'Pending spanning events: %s' %
          '\n'.join([str(e) for e in spanning_events.PendingEvents()]))

  def _GetEvents(self):
    self._IndexEvents()
    return self._interval_tree.GetEvents()

  def HasLoadingSucceeded(self):
    """Returns whether the loading has succeed at recording time."""
    main_frame_id = self.GetMainFrameRoutingID()
    for event in self.GetMatchingEvents(
        'navigation', 'RenderFrameImpl::didFailProvisionalLoad'):
      if event.args['id'] == main_frame_id:
        return False
    for event in self.GetMatchingEvents(
        'navigation', 'RenderFrameImpl::didFailLoad'):
      if event.args['id'] == main_frame_id:
        return False
    return True

  class _SpanningEvents(object):
    def __init__(self):
      self._duration_stack = []
      self._async_stacks = {}
      self._objects = {}
      self._MATCH_HANDLER = {
          'B': self._DurationBegin,
          'E': self._DurationEnd,
          'b': self._AsyncStart,
          'e': self._AsyncEnd,
          'S': self._AsyncStart,
          'F': self._AsyncEnd,
          'N': self._ObjectCreated,
          'D': self._ObjectDestroyed,
          'M': self._Ignore,
          'X': self._Ignore,
          'R': self._Ignore,
          'p': self._Ignore,
          '(': self._Ignore, # Context events.
          ')': self._Ignore, # Ditto.
          None: self._Ignore,
          }

    def Match(self, event, strict=False):
      return self._MATCH_HANDLER.get(
          event.type, self._Unsupported)(event, strict)

    def HasPending(self):
      return (self._duration_stack or
              self._async_stacks or
              self._objects)

    def PendingEvents(self):
      return itertools.chain(
          (e for e in self._duration_stack),
          (o for o in self._objects),
          itertools.chain.from_iterable((
              (e for e in s) for s in self._async_stacks.itervalues())))

    def _AsyncKey(self, event, _):
      return (event.tracing_event['cat'], event.id)

    def _Ignore(self, _event, _):
      return None

    def _Unsupported(self, event, _):
      raise devtools_monitor.DevToolsConnectionException(
          'Unsupported spanning event type: %s' % event)

    def _DurationBegin(self, event, _):
      self._duration_stack.append(event)
      return None

    def _DurationEnd(self, event, _):
      if not self._duration_stack:
        raise devtools_monitor.DevToolsConnectionException(
            'Unmatched duration end: %s' % event)
      start = self._duration_stack.pop()
      start.SetClose(event)
      return start

    def _AsyncStart(self, event, strict):
      key = self._AsyncKey(event, strict)
      self._async_stacks.setdefault(key, []).append(event)
      return None

    def _AsyncEnd(self, event, strict):
      key = self._AsyncKey(event, strict)
      if key not in self._async_stacks:
        message = 'Unmatched async end %s: %s' % (key, event)
        if strict:
          raise devtools_monitor.DevToolsConnectionException(message)
        else:
          logging.warning(message)
        return None
      stack = self._async_stacks[key]
      start = stack.pop()
      if not stack:
        del self._async_stacks[key]
      start.SetClose(event)
      return start

    def _ObjectCreated(self, event, _):
      # The tracing event format has object deletion timestamps being exclusive,
      # that is the timestamp for a deletion my equal that of the next create at
      # the same address. This asserts that does not happen in practice as it is
      # inconvenient to handle that correctly here.
      if event.id in self._objects:
        raise devtools_monitor.DevToolsConnectionException(
            'Multiple objects at same address: %s, %s' %
            (event, self._objects[event.id]))
      self._objects[event.id] = event
      return None

    def _ObjectDestroyed(self, event, _):
      if event.id not in self._objects:
        raise devtools_monitor.DevToolsConnectionException(
            'Missing object creation for %s' % event)
      start = self._objects[event.id]
      del self._objects[event.id]
      start.SetClose(event)
      return start


class Event(object):
  """Wraps a tracing event."""
  CLOSING_EVENTS = {'E': 'B',
                    'e': 'b',
                    'F': 'S',
                    'D': 'N'}
  __slots__ = ('_tracing_event', 'start_msec', 'end_msec', '_synthetic')
  def __init__(self, tracing_event, synthetic=False):
    """Creates Event.

    Intended to be created only by TracingTrack.

    Args:
      tracing_event: JSON tracing event, as defined in https://goo.gl/Qabkqk.
      synthetic: True if the event is synthetic. This is only used for indexing
        internal to TracingTrack.
    """
    if not synthetic and tracing_event['ph'] in ['s', 't', 'f']:
      raise devtools_monitor.DevToolsConnectionException(
          'Unsupported event: %s' % tracing_event)

    self._tracing_event = tracing_event
    # Note tracing event times are in microseconds.
    self.start_msec = tracing_event['ts'] / 1000.0
    self.end_msec = None
    self._synthetic = synthetic
    if self.type == 'X':
      # Some events don't have a duration.
      duration = (tracing_event['dur']
                  if 'dur' in tracing_event else tracing_event['tdur'])
      self.end_msec = self.start_msec + duration / 1000.0

  @property
  def type(self):
    if self._synthetic:
      return None
    return self._tracing_event['ph']

  @property
  def category(self):
    return self._tracing_event['cat']

  @property
  def pid(self):
    return self._tracing_event['pid']

  @property
  def args(self):
    return self._tracing_event.get('args', {})

  @property
  def id(self):
    return self._tracing_event.get('id')

  @property
  def name(self):
    return self._tracing_event['name']

  @property
  def tracing_event(self):
    return self._tracing_event

  @property
  def synthetic(self):
    return self._synthetic

  def __str__(self):
    return ''.join([str(self._tracing_event),
                    '[%s,%s]' % (self.start_msec, self.end_msec)])

  def Matches(self, category, name):
    """Match tracing events.

    Args:
      category: a tracing category (event['cat']).
      name: the tracing event name (event['name']).

    Returns:
      True if the event matches and False otherwise.
    """
    if name != self.name:
      return False
    categories = self.category.split(',')
    return category in categories

  def IsIndexable(self):
    """True iff the event can be indexed by time."""
    return self._synthetic or self.type not in [
        'I', 'P', 'c', 'C',
        'n', 'T', 'p',  # TODO(mattcary): ?? instant types of async events.
        'O',            # TODO(mattcary): ?? object snapshot
        'M'             # Metadata
        ]

  def IsComplete(self):
    return self.type == 'X'

  def Synthesize(self):
    """Expand into synthetic events.

    Returns:
      A list of events, possibly some synthetic, whose start times are all
      interesting for purposes of indexing. If the event is not indexable the
      set may be empty.
    """
    if not self.IsIndexable():
      return []
    if self.IsComplete():
      # Tracing event timestamps are microseconds!
      return [self, Event({'ts': self.end_msec * 1000}, synthetic=True)]
    return [self]

  def SetClose(self, closing):
    """Close a spanning event.

    Args:
      closing: The closing event.

    Raises:
      devtools_monitor.DevToolsConnectionException if closing can't property
      close this event.
    """
    if self.type != self.CLOSING_EVENTS.get(closing.type):
      raise devtools_monitor.DevToolsConnectionException(
        'Bad closing: %s --> %s' % (self, closing))
    if self.type in ['b', 'S'] and (
        self.tracing_event['cat'] != closing.tracing_event['cat'] or
        self.id != closing.id):
      raise devtools_monitor.DevToolsConnectionException(
        'Bad async closing: %s --> %s' % (self, closing))
    self.end_msec = closing.start_msec
    if 'args' in closing.tracing_event:
      self.tracing_event.setdefault(
          'args', {}).update(closing.tracing_event['args'])

  def ToJsonDict(self):
    return self._tracing_event

  @classmethod
  def FromJsonDict(cls, json_dict):
    return Event(json_dict)


class _IntervalTree(object):
  """Simple interval tree. This is not an optimal one, as the split is done with
  an equal number of events on each side, according to start time.
  """
  _TRESHOLD = 100
  def __init__(self, start, end, events):
    """Builds an interval tree.

    Args:
      start: start timestamp of this node, in ms.
      end: end timestamp covered by this node, in ms.
      events: Iterable of objects having start_msec and end_msec fields. Has to
              be sorted by start_msec.
    """
    self.start = start
    self.end = end
    self._events = events
    self._left = self._right = None
    if len(self._events) > self._TRESHOLD:
      self._Divide()

  @classmethod
  def FromEvents(cls, events):
    """Returns an IntervalTree instance from a list of events."""
    filtered_events = [e for e in events
                       if e.start_msec is not None and e.end_msec is not None]
    filtered_events.sort(key=operator.attrgetter('start_msec'))
    start = min(event.start_msec for event in filtered_events)
    end = max(event.end_msec for event in filtered_events)
    return _IntervalTree(start, end, filtered_events)

  def OverlappingEvents(self, start, end):
    """Returns a set of events overlapping with [start, end)."""
    if min(end, self.end) - max(start, self.start) <= 0:
      return set()
    elif self._IsLeaf():
      result = set()
      for event in self._events:
        if self._Overlaps(event, start, end):
          result.add(event)
      return result
    else:
      return (self._left.OverlappingEvents(start, end)
              | self._right.OverlappingEvents(start, end))

  def EventsAt(self, timestamp):
    result = set()
    if self._IsLeaf():
      for event in self._events:
        if event.start_msec <= timestamp < event.end_msec:
          result.add(event)
    else:
      if self._left.start <= timestamp < self._left.end:
        result |= self._left.EventsAt(timestamp)
      if self._right.start <= timestamp < self._right.end:
        result |= self._right.EventsAt(timestamp)
    return result

  def GetEvents(self):
    return self._events

  def _Divide(self):
    middle = len(self._events) / 2
    left_events = self._events[:middle]
    right_events = self._events[middle:]
    left_end = max(e.end_msec for e in left_events)
    right_start = min(e.start_msec for e in right_events)
    self._left = _IntervalTree(self.start, left_end, left_events)
    self._right = _IntervalTree(right_start, self.end, right_events)

  def _IsLeaf(self):
    return self._left is None

  @classmethod
  def _Overlaps(cls, event, start, end):
    return (min(end, event.end_msec) - max(start, event.start_msec) > 0
            or start <= event.start_msec < end)  # For instant events.
