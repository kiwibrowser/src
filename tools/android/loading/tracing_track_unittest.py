# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import copy
import logging
import operator
import unittest

import devtools_monitor

from tracing_track import (Event, TracingTrack, _IntervalTree)


class TracingTrackTestCase(unittest.TestCase):
  _MIXED_EVENTS = [
      {'ts': 3, 'ph': 'N', 'id': 1, 'args': {'name': 'A'}},
      {'ts': 5, 'ph': 'X', 'dur': 1, 'args': {'name': 'B'}},
      {'ts': 7, 'ph': 'D', 'id': 1},
      {'ts': 10, 'ph': 'B', 'args': {'name': 'D'}},
      {'ts': 10, 'ph': 'b', 'cat': 'X', 'id': 1, 'args': {'name': 'C'}},
      {'ts': 11, 'ph': 'e', 'cat': 'X', 'id': 1},
      {'ts': 12, 'ph': 'E'},
      {'ts': 12, 'ph': 'N', 'id': 1, 'args': {'name': 'E'}},
      {'ts': 13, 'ph': 'b', 'cat': 'X', 'id': 2, 'args': {'name': 'F'}},
      {'ts': 14, 'ph': 'e', 'cat': 'X', 'id': 2},
      {'ts': 15, 'ph': 'D', 'id': 1}]

  _EVENTS = [
      {'ts': 5, 'ph': 'X', 'dur': 1, 'pid': 2, 'tid': 1, 'args': {'name': 'B'}},
      {'ts': 3, 'ph': 'X', 'dur': 4, 'pid': 2, 'tid': 1, 'args': {'name': 'A'}},
      {'ts': 10, 'ph': 'X', 'dur': 1, 'pid': 2, 'tid': 2,
       'args': {'name': 'C'}},
      {'ts': 10, 'ph': 'X', 'dur': 2, 'pid': 2, 'tid': 2,
       'args': {'name': 'D'}},
      {'ts': 13, 'ph': 'X', 'dur': 1, 'pid': 2, 'tid': 1,
       'args': {'name': 'F'}},
      {'ts': 12, 'ph': 'X', 'dur': 3, 'pid': 2, 'tid': 1,
       'args': {'name': 'E'}}]

  def setUp(self):
    self.tree_threshold = _IntervalTree._TRESHOLD
    _IntervalTree._TRESHOLD = 2  # Expose more edge cases in the tree.
    self.track = TracingTrack(None, ['A', 'B', 'C', 'D'])

  def tearDown(self):
    _IntervalTree._TRESHOLD = self.tree_threshold

  def EventToMicroseconds(self, event):
    result = copy.deepcopy(event)
    if 'ts' in result:
      result['ts'] *= 1000
    if 'dur' in result:
      result['dur'] *= 1000
    return result

  def CheckTrack(self, timestamp, names):
    self.track._IndexEvents(strict=True)
    self.assertEqual(
        set((e.args['name'] for e in self.track.EventsAt(timestamp))),
        set(names))

  def CheckIntervals(self, events):
    """All tests should produce the following sequence of intervals, each
    identified by a 'name' in the event args.

    Timestamp
    3    |      A
    4    |
    5    | |    B
    6    |
    7
    ..
    10   | |    C, D
    11     |
    12   |      E
    13   | |    F
    14   |
    """
    self.track.Handle('Tracing.dataCollected',
                      {'params': {'value': [self.EventToMicroseconds(e)
                                            for e in events]}})
    self.CheckTrack(0, '')
    self.CheckTrack(2, '')
    self.CheckTrack(3, 'A')
    self.CheckTrack(4, 'A')
    self.CheckTrack(5, 'AB')
    self.CheckTrack(6, 'A')
    self.CheckTrack(7, '')
    self.CheckTrack(9, '')
    self.CheckTrack(10, 'CD')
    self.CheckTrack(11, 'D')
    self.CheckTrack(12, 'E')
    self.CheckTrack(13, 'EF')
    self.CheckTrack(14, 'E')
    self.CheckTrack(15, '')
    self.CheckTrack(100, '')

  def testComplete(self):
    # These are deliberately out of order.
    self.CheckIntervals([
        {'ts': 5, 'ph': 'X', 'dur': 1, 'args': {'name': 'B'}},
        {'ts': 3, 'ph': 'X', 'dur': 4, 'args': {'name': 'A'}},
        {'ts': 10, 'ph': 'X', 'dur': 1, 'args': {'name': 'C'}},
        {'ts': 10, 'ph': 'X', 'dur': 2, 'args': {'name': 'D'}},
        {'ts': 13, 'ph': 'X', 'dur': 1, 'args': {'name': 'F'}},
        {'ts': 12, 'ph': 'X', 'dur': 3, 'args': {'name': 'E'}}])

  def testDuration(self):
    self.CheckIntervals([
        {'ts': 3, 'ph': 'B', 'args': {'name': 'A'}},
        {'ts': 5, 'ph': 'B', 'args': {'name': 'B'}},
        {'ts': 6, 'ph': 'E'},
        {'ts': 7, 'ph': 'E'},
        # Since async intervals aren't named and must be nested, we fudge the
        # beginning of D by a tenth to ensure it's consistently detected as the
        # outermost event.
        {'ts': 9.9, 'ph': 'B', 'args': {'name': 'D'}},
        {'ts': 10, 'ph': 'B', 'args': {'name': 'C'}},
        {'ts': 11, 'ph': 'E'},
        # End of D. As end times are exclusive this should not conflict with the
        # start of E.
        {'ts': 12, 'ph': 'E'},
        {'ts': 12, 'ph': 'B', 'args': {'name': 'E'}},
        {'ts': 13, 'ph': 'B', 'args': {'name': 'F'}},
        {'ts': 14, 'ph': 'E'},
        {'ts': 15, 'ph': 'E'}])

  def testBadDurationExtraBegin(self):
    self.assertRaises(devtools_monitor.DevToolsConnectionException,
                      self.CheckIntervals,
                      [{'ts': 3, 'ph': 'B'},
                       {'ts': 4, 'ph': 'B'},
                       {'ts': 5, 'ph': 'E'}])

  def testBadDurationExtraEnd(self):
    self.assertRaises(devtools_monitor.DevToolsConnectionException,
                      self.CheckIntervals,
                      [{'ts': 3, 'ph': 'B'},
                       {'ts': 4, 'ph': 'E'},
                       {'ts': 5, 'ph': 'E'}])

  def testAsync(self):
    self.CheckIntervals([
        # A, B and F have the same category/id (so that A & B nest); C-E do not.
        {'ts': 3, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'A'}},
        {'ts': 5, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'B'}},
        # Not indexable.
        {'ts': 4, 'ph': 'n', 'cat': 'A', 'id': 1, 'args': {'name': 'A'}},
        {'ts': 6, 'ph': 'e', 'cat': 'A', 'id': 1},
        {'ts': 7, 'ph': 'e', 'cat': 'A', 'id': 1},
        {'ts': 10, 'ph': 'b', 'cat': 'B', 'id': 2, 'args': {'name': 'D'}},
        {'ts': 10, 'ph': 'b', 'cat': 'B', 'id': 3, 'args': {'name': 'C'}},
        {'ts': 11, 'ph': 'e', 'cat': 'B', 'id': 3},
        {'ts': 12, 'ph': 'e', 'cat': 'B', 'id': 2},
        {'ts': 12, 'ph': 'b', 'cat': 'A', 'id': 2, 'args': {'name': 'E'}},
        {'ts': 13, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'F'}},
        {'ts': 14, 'ph': 'e', 'cat': 'A', 'id': 1},
        {'ts': 15, 'ph': 'e', 'cat': 'A', 'id': 2}])

  def testBadAsyncIdMismatch(self):
    self.assertRaises(
        devtools_monitor.DevToolsConnectionException,
        self.CheckIntervals,
        [{'ts': 3, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'A'}},
         {'ts': 5, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'B'}},
         {'ts': 6, 'ph': 'e', 'cat': 'A', 'id': 2},
         {'ts': 7, 'ph': 'e', 'cat': 'A', 'id': 1}])

  def testBadAsyncExtraBegin(self):
    self.assertRaises(
        devtools_monitor.DevToolsConnectionException,
        self.CheckIntervals,
        [{'ts': 3, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'A'}},
         {'ts': 5, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'B'}},
         {'ts': 6, 'ph': 'e', 'cat': 'A', 'id': 1}])

  def testBadAsyncExtraEnd(self):
    self.assertRaises(
        devtools_monitor.DevToolsConnectionException,
        self.CheckIntervals,
        [{'ts': 3, 'ph': 'b', 'cat': 'A', 'id': 1, 'args': {'name': 'A'}},
         {'ts': 5, 'ph': 'e', 'cat': 'A', 'id': 1},
         {'ts': 6, 'ph': 'e', 'cat': 'A', 'id': 1}])

  def testObject(self):
    # A and E share ids, which is okay as their scopes are disjoint.
    self.CheckIntervals([
        {'ts': 3, 'ph': 'N', 'id': 1, 'args': {'name': 'A'}},
        {'ts': 5, 'ph': 'N', 'id': 2, 'args': {'name': 'B'}},
        {'ts': 6, 'ph': 'D', 'id': 2},
        {'ts': 6, 'ph': 'O', 'id': 2},  #  Ignored.
        {'ts': 7, 'ph': 'D', 'id': 1},
        {'ts': 10, 'ph': 'N', 'id': 3, 'args': {'name': 'D'}},
        {'ts': 10, 'ph': 'N', 'id': 4, 'args': {'name': 'C'}},
        {'ts': 11, 'ph': 'D', 'id': 4},
        {'ts': 12, 'ph': 'D', 'id': 3},
        {'ts': 12, 'ph': 'N', 'id': 1, 'args': {'name': 'E'}},
        {'ts': 13, 'ph': 'N', 'id': 5, 'args': {'name': 'F'}},
        {'ts': 14, 'ph': 'D', 'id': 5},
        {'ts': 15, 'ph': 'D', 'id': 1}])

  def testMixed(self):
    # A and E are objects, B complete, D a duration, and C and F async.
    self.CheckIntervals(self._MIXED_EVENTS)

  def testEventSerialization(self):
    for e in self._MIXED_EVENTS:
      event = Event(e)
      json_dict = event.ToJsonDict()
      deserialized_event = Event.FromJsonDict(json_dict)
      self.assertEquals(
          event.tracing_event, deserialized_event.tracing_event)

  def testTracingTrackSerialization(self):
    self._HandleEvents(self._MIXED_EVENTS)
    json_dict = self.track.ToJsonDict()
    self.assertTrue('events' in json_dict)
    deserialized_track = TracingTrack.FromJsonDict(json_dict)
    self.assertEquals(
        len(self.track._events), len(deserialized_track._events))
    for (e1, e2) in zip(self.track._events, deserialized_track._events):
      self.assertEquals(e1.tracing_event, e2.tracing_event)

  def testEventsEndingBetween(self):
    self._HandleEvents(self._EVENTS)
    self.assertEqual(set('ABCDEF'),
                     set([e.args['name']
                          for e in self.track.EventsEndingBetween(0, 100)]))
    self.assertFalse([e.args['name']
                      for e in self.track.EventsEndingBetween(3, 5)])
    self.assertTrue('B' in set([e.args['name']
                          for e in self.track.EventsEndingBetween(3, 6)]))
    self.assertEqual(set('B'),
                     set([e.args['name']
                          for e in self.track.EventsEndingBetween(3, 6)]))

  def testOverlappingEvents(self):
    self._HandleEvents(self._EVENTS)
    self.assertEqual(set('ABCDEF'),
                     set([e.args['name']
                          for e in self.track.OverlappingEvents(0, 100)]))
    self.assertFalse([e.args['name']
                      for e in self.track.OverlappingEvents(0, 2)])
    self.assertEqual(set('BA'),
                     set([e.args['name']
                          for e in self.track.OverlappingEvents(4, 5.1)]))
    self.assertEqual(set('ACD'),
                     set([e.args['name']
                          for e in self.track.OverlappingEvents(6, 10.1)]))

  def testEventFromStep(self):
    events = [
        {'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 2, 'tid': 1, 'id': '0x123',
         'name': 'B'},
        {'ts': 5, 'ph': 'X', 'dur': 2, 'pid': 2, 'tid': 1, 'id': '0x12343',
        'name': 'A'}]
    step_events = [{'ts': 6, 'ph': 'T', 'pid': 2, 'tid': 1, 'id': '0x123',
                    'name': 'B', 'args': {'step': 'Bla'}},
                   {'ts': 4, 'ph': 'T', 'pid': 2, 'tid': 1, 'id': '0x123',
                    'name': 'B', 'args': {'step': 'Bla'}},
                   {'ts': 6, 'ph': 'T', 'pid': 12, 'tid': 1, 'id': '0x123',
                    'name': 'B', 'args': {'step': 'Bla'}},
                   {'ts': 6, 'ph': 'T', 'pid': 2, 'tid': 1, 'id': '0x1234',
                    'name': 'B', 'args': {'step': 'Bla'}},
                   {'ts': 6, 'ph': 'T', 'pid': 2, 'tid': 1, 'id': '0x123',
                    'name': 'A', 'args': {'step': 'Bla'}},
                   {'ts': 6, 'ph': 'n', 'pid': 2, 'tid': 1, 'id': '0x123',
                    'name': 'B', 'args': {'step': 'Bla'}},
                   {'ts': 6, 'ph': 'n', 'pid': 2, 'tid': 1, 'id': '0x123',
                    'name': 'B', 'args': {}}]
    self._HandleEvents(events + step_events)
    trace_events = self.track.GetEvents()
    self.assertEquals(9, len(trace_events))
    # pylint: disable=unbalanced-tuple-unpacking
    (event, _, step_event, outside, wrong_pid, wrong_id, wrong_name,
     wrong_phase, no_step) = trace_events
    self.assertEquals(event, self.track.EventFromStep(step_event))
    self.assertIsNone(self.track.EventFromStep(outside))
    self.assertIsNone(self.track.EventFromStep(wrong_pid))
    self.assertIsNone(self.track.EventFromStep(wrong_id))
    self.assertIsNone(self.track.EventFromStep(wrong_name))
    # Invalid events
    with self.assertRaises(AssertionError):
      self.track.EventFromStep(wrong_phase)
    with self.assertRaises(AssertionError):
      self.track.EventFromStep(no_step)

  def testFilterPidTid(self):
    self._HandleEvents(self._EVENTS)
    tracing_track = self.track.Filter(2, 1)
    self.assertTrue(tracing_track is not self.track)
    self.assertEquals(4, len(tracing_track.GetEvents()))
    tracing_track = self.track.Filter(2, 42)
    self.assertEquals(0, len(tracing_track.GetEvents()))

  def testGetMainFrameID(self):
    _MAIN_FRAME_ID = 0xffff
    _SUBFRAME_ID = 0xaaaa
    events = [
        {'ts': 7, 'ph': 'X', 'dur': 10, 'pid': 2, 'tid': 1, 'id': '0x123',
         'name': 'navigationStart', 'cat': 'blink.user_timing',
         'args': {'frame': _SUBFRAME_ID}},
        {'ts': 8, 'ph': 'X', 'dur': 2, 'pid': 2, 'tid': 1, 'id': '0x12343',
        'name': 'A'},
        {'ts': 3, 'ph': 'X', 'dur': 10, 'pid': 2, 'tid': 1, 'id': '0x125',
         'name': 'navigationStart', 'cat': 'blink.user_timing',
         'args': {'frame': _MAIN_FRAME_ID}},
        ]
    self._HandleEvents(events)
    self.assertEquals(_MAIN_FRAME_ID, self.track.GetMainFrameID())

  def testGetMatchingEvents(self):
    _MAIN_FRAME_ID = 0xffff
    _SUBFRAME_ID = 0xaaaa
    events = [
        {'ts': 7, 'ph': 'X', 'dur': 10, 'pid': 2, 'tid': 1, 'id': '0x123',
         'name': 'navigationStart', 'cat': 'blink.user_timing',
         'args': {'frame': _SUBFRAME_ID}},
        {'ts': 8, 'ph': 'X', 'dur': 2, 'pid': 2, 'tid': 1, 'id': '0x12343',
        'name': 'A'},
        {'ts': 3, 'ph': 'X', 'dur': 10, 'pid': 2, 'tid': 1, 'id': '0x125',
         'name': 'navigationStart', 'cat': 'blink.user_timing',
         'args': {'frame': _MAIN_FRAME_ID}},
        ]
    self._HandleEvents(events)
    matching_events = self.track.GetMatchingEvents('blink.user_timing',
                                                   'navigationStart')
    self.assertEquals(2, len(matching_events))
    self.assertListEqual([self.track.GetEvents()[0],
                         self.track.GetEvents()[2]], matching_events)

    matching_main_frame_events = self.track.GetMatchingMainFrameEvents(
        'blink.user_timing', 'navigationStart')
    self.assertEquals(1, len(matching_main_frame_events))
    self.assertListEqual([self.track.GetEvents()[2]],
                         matching_main_frame_events)

  def testFilterCategories(self):
    events = [
        {'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 2, 'tid': 1, 'cat': 'A'},
        {'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 2, 'tid': 1, 'cat': 'B'},
        {'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 2, 'tid': 1, 'cat': 'C,D'},
        {'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 2, 'tid': 1, 'cat': 'A,B,C,D'}]
    self._HandleEvents(events)
    tracing_events = self.track.GetEvents()
    self.assertEquals(4, len(tracing_events))
    filtered_events = self.track.Filter(categories=None).GetEvents()
    self.assertListEqual(tracing_events, filtered_events)
    filtered_events = self.track.Filter(categories=set(['A'])).GetEvents()
    self.assertEquals(2, len(filtered_events))
    self.assertListEqual([tracing_events[0], tracing_events[3]],
                         filtered_events)
    filtered_events = self.track.Filter(categories=set(['Z'])).GetEvents()
    self.assertEquals(0, len(filtered_events))
    filtered_events = self.track.Filter(categories=set(['B', 'C'])).GetEvents()
    self.assertEquals(3, len(filtered_events))
    self.assertListEqual(tracing_events[1:], filtered_events)
    self.assertSetEqual(
        set('A'), self.track.Filter(categories=set('A')).Categories())

  def testHasLoadingSucceeded(self):
    cat = 'navigation'
    on_navigate = 'RenderFrameImpl::OnNavigate'
    fail_provisional = 'RenderFrameImpl::didFailProvisionalLoad'
    fail_load = 'RenderFrameImpl::didFailLoad'

    track = TracingTrack.FromJsonDict({'categories': [cat], 'events': []})
    with self.assertRaises(AssertionError):
      track.HasLoadingSucceeded()

    track = TracingTrack.FromJsonDict({'categories': [cat], 'events': [
        {'cat': cat, 'name': on_navigate, 'args': {'id': 1},
            'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 1, 'tid': 1}]})
    self.assertTrue(track.HasLoadingSucceeded())

    track = TracingTrack.FromJsonDict({'categories': [cat], 'events': [
        {'cat': cat, 'name': on_navigate, 'args': {'id': 1},
            'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 1, 'tid': 1},
        {'cat': cat, 'name': on_navigate, 'args': {'id': 2},
            'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 1, 'tid': 1},
        {'cat': cat, 'name': fail_provisional, 'args': {'id': 2},
            'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 1, 'tid': 1}]})
    self.assertTrue(track.HasLoadingSucceeded())

    track = TracingTrack.FromJsonDict({'categories': [cat], 'events': [
        {'cat': cat, 'name': on_navigate, 'args': {'id': 1},
            'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 1, 'tid': 1},
        {'cat': cat, 'name': fail_provisional, 'args': {'id': 1},
            'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 1, 'tid': 1}]})
    self.assertFalse(track.HasLoadingSucceeded())

    track = TracingTrack.FromJsonDict({'categories': [cat], 'events': [
        {'cat': cat, 'name': on_navigate, 'args': {'id': 1},
            'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 1, 'tid': 1},
        {'cat': cat, 'name': fail_load, 'args': {'id': 1},
            'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 1, 'tid': 1}]})
    self.assertFalse(track.HasLoadingSucceeded())

    track = TracingTrack.FromJsonDict({'categories': [cat], 'events': [
        {'cat': cat, 'name': on_navigate, 'args': {'id': 1},
            'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 1, 'tid': 1},
        {'cat': cat, 'name': fail_load, 'args': {'id': 1},
            'ts': 5, 'ph': 'X', 'dur': 10, 'pid': 1, 'tid': 1}]})
    self.assertFalse(track.HasLoadingSucceeded())

  def _HandleEvents(self, events):
    self.track.Handle('Tracing.dataCollected', {'params': {'value': [
        self.EventToMicroseconds(e) for e in events]}})


class IntervalTreeTestCase(unittest.TestCase):
  class FakeEvent(object):
    def __init__(self, start_msec, end_msec):
      self.start_msec = start_msec
      self.end_msec = end_msec

    def __eq__(self, o):
      return self.start_msec == o.start_msec and self.end_msec == o.end_msec

  _COUNT = 1000

  def testCreateTree(self):
    events = [self.FakeEvent(100 * i, 100 * (i + 1))
              for i in range(self._COUNT)]
    tree = _IntervalTree.FromEvents(events)
    self.assertEquals(0, tree.start)
    self.assertEquals(100 * self._COUNT, tree.end)
    self.assertFalse(tree._IsLeaf())

  def testEventsAt(self):
    events = ([self.FakeEvent(100 * i, 100 * (i + 1))
               for i in range(self._COUNT)]
              + [self.FakeEvent(100 * i + 50, 100 * i + 150)
                 for i in range(self._COUNT)])
    tree = _IntervalTree.FromEvents(events)
    self.assertEquals(0, tree.start)
    self.assertEquals(100 * self._COUNT + 50, tree.end)
    self.assertFalse(tree._IsLeaf())
    for i in range(self._COUNT):
      self.assertEquals(2, len(tree.EventsAt(100 * i + 75)))
    # Add instant events, check that they are excluded.
    events += [self.FakeEvent(100 * i + 75, 100 * i + 75)
               for i in range(self._COUNT)]
    tree = _IntervalTree.FromEvents(events)
    self.assertEquals(3 * self._COUNT, len(tree._events))
    for i in range(self._COUNT):
      self.assertEquals(2, len(tree.EventsAt(100 * i + 75)))

  def testOverlappingEvents(self):
    events = ([self.FakeEvent(100 * i, 100 * (i + 1))
               for i in range(self._COUNT)]
              + [self.FakeEvent(100 * i + 50, 100 * i + 150)
                 for i in range(self._COUNT)])
    tree = _IntervalTree.FromEvents(events)
    self.assertEquals(0, tree.start)
    self.assertEquals(100 * self._COUNT + 50, tree.end)
    self.assertFalse(tree._IsLeaf())
    # 400 -> 500, 450 -> 550, 500 -> 600
    self.assertEquals(3, len(tree.OverlappingEvents(450, 550)))
    overlapping = sorted(
        tree.OverlappingEvents(450, 550), key=operator.attrgetter('start_msec'))
    self.assertEquals(self.FakeEvent(400, 500), overlapping[0])
    self.assertEquals(self.FakeEvent(450, 550), overlapping[1])
    self.assertEquals(self.FakeEvent(500, 600), overlapping[2])
    self.assertEquals(8, len(tree.OverlappingEvents(450, 800)))
    # Add instant events, check that they are included.
    events += [self.FakeEvent(500, 500) for i in range(10)]
    tree = _IntervalTree.FromEvents(events)
    self.assertEquals(3 + 10, len(tree.OverlappingEvents(450, 550)))
    self.assertEquals(8 + 10, len(tree.OverlappingEvents(450, 800)))

  def testEventMatches(self):
    event = Event({'name': 'foo',
                   'cat': 'bar',
                   'ph': 'X',
                   'ts': 0, 'dur': 0})
    self.assertTrue(event.Matches('bar', 'foo'))
    self.assertFalse(event.Matches('bar', 'biz'))
    self.assertFalse(event.Matches('biz', 'foo'))

    event = Event({'name': 'foo',
                   'cat': 'bar,baz,bizbiz',
                   'ph': 'X',
                   'ts': 0, 'dur': 0})
    self.assertTrue(event.Matches('bar', 'foo'))
    self.assertTrue(event.Matches('baz', 'foo'))
    self.assertFalse(event.Matches('bar', 'biz'))
    self.assertFalse(event.Matches('biz', 'foo'))

if __name__ == '__main__':
  unittest.main()
