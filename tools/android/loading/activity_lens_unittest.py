# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import copy
import unittest

from activity_lens import (ActivityLens, _EventsTree)
import clovis_constants
import test_utils
import tracing_track


class ActivityLensTestCase(unittest.TestCase):
  @classmethod
  def _EventsFromRawEvents(cls, raw_events):
    track = tracing_track.TracingTrack(None,
        clovis_constants.DEFAULT_CATEGORIES)
    track.Handle(
        'Tracing.dataCollected', {'params': {'value': raw_events}})
    return track.GetEvents()

  def setUp(self):
    self.track = tracing_track.TracingTrack(None,
        clovis_constants.DEFAULT_CATEGORIES)

  def testGetRendererMainThread(self):
    first_renderer_tid = 12345
    second_renderer_tid = 123456
    raw_events =  [
        {u'args': {u'name': u'CrBrowserMain'},
         u'cat': u'__metadata',
         u'name': u'thread_name',
         u'ph': u'M',
         u'pid': 1,
         u'tid': 123,
         u'ts': 0},
        {u'args': {u'name': u'CrRendererMain'},
         u'cat': u'__metadata',
         u'name': u'thread_name',
         u'ph': u'M',
         u'pid': 1,
         u'tid': first_renderer_tid,
         u'ts': 0},
        {u'args': {u'name': u'CrRendererMain'},
         u'cat': u'__metadata',
         u'name': u'thread_name',
         u'ph': u'M',
         u'pid': 1,
         u'tid': second_renderer_tid,
         u'ts': 0}]
    raw_events += [
        {u'args': {u'data': {}},
         u'cat': u'devtools.timeline,v8',
         u'name': u'FunctionCall',
         u'ph': u'X',
         u'pid': 1,
         u'tdur': 0,
         u'tid': first_renderer_tid,
         u'ts': 251427174674,
         u'tts': 5107725}] * 100
    raw_events += [
        {u'args': {u'data': {}},
         u'cat': u'devtools.timeline,v8',
         u'name': u'FunctionCall',
         u'ph': u'X',
         u'pid': 1,
         u'tdur': 0,
         u'tid': second_renderer_tid,
         u'ts': 251427174674,
         u'tts': 5107725}] * 150
    # There are more events from first_renderer_tid when (incorrectly) ignoring
    # the PID.
    raw_events += [
        {u'args': {u'data': {}},
         u'cat': u'devtools.timeline,v8',
         u'name': u'FunctionCall',
         u'ph': u'X',
         u'pid': 12,
         u'tdur': 0,
         u'tid': first_renderer_tid,
         u'ts': 251427174674,
         u'tts': 5107725}] * 100
    events = self._EventsFromRawEvents(raw_events)
    self.assertEquals((1, second_renderer_tid),
                      ActivityLens._GetRendererMainThreadId(events))

  def testThreadBusyness(self):
    raw_events =  [
        {u'args': {},
         u'cat': u'toplevel',
         u'dur': 200 * 1000,
         u'name': u'MessageLoop::RunTask',
         u'ph': u'X',
         u'pid': 123,
         u'tid': 123,
         u'ts': 0,
         u'tts': 56485},
        {u'args': {},
         u'cat': u'toplevel',
         u'dur': 8 * 200,
         u'name': u'MessageLoop::NestedSomething',
         u'ph': u'X',
         u'pid': 123,
         u'tid': 123,
         u'ts': 0,
         u'tts': 0}]
    events = self._EventsFromRawEvents(raw_events)
    self.assertEquals(200, ActivityLens._ThreadBusyness(events, 0, 1000))
    # Clamping duration.
    self.assertEquals(100, ActivityLens._ThreadBusyness(events, 0, 100))
    self.assertEquals(50, ActivityLens._ThreadBusyness(events, 25, 75))

  def testScriptExecuting(self):
    url = u'http://example.com/script.js'
    raw_events = [
        {u'args': {u'data': {u'scriptName': url}},
         u'cat': u'devtools.timeline,v8',
         u'dur': 250 * 1000,
         u'name': u'FunctionCall',
         u'ph': u'X',
         u'pid': 123,
         u'tdur': 247,
         u'tid': 123,
         u'ts': 0,
         u'tts': 0},
        {u'args': {u'data': {}},
         u'cat': u'devtools.timeline,v8',
         u'dur': 350 * 1000,
         u'name': u'EvaluateScript',
         u'ph': u'X',
         u'pid': 123,
         u'tdur': 247,
         u'tid': 123,
         u'ts': 0,
         u'tts': 0}]
    events = self._EventsFromRawEvents(raw_events)
    self.assertEquals(2, len(ActivityLens._ScriptsExecuting(events, 0, 1000)))
    self.assertTrue(None in ActivityLens._ScriptsExecuting(events, 0, 1000))
    self.assertEquals(
        350, ActivityLens._ScriptsExecuting(events, 0, 1000)[None])
    self.assertTrue(url in ActivityLens._ScriptsExecuting(events, 0, 1000))
    self.assertEquals(250, ActivityLens._ScriptsExecuting(events, 0, 1000)[url])
    # Aggreagates events.
    raw_events.append({u'args': {u'data': {}},
                       u'cat': u'devtools.timeline,v8',
                       u'dur': 50 * 1000,
                       u'name': u'EvaluateScript',
                       u'ph': u'X',
                       u'pid': 123,
                       u'tdur': 247,
                       u'tid': 123,
                       u'ts': 0,
                       u'tts': 0})
    events = self._EventsFromRawEvents(raw_events)
    self.assertEquals(
        350 + 50, ActivityLens._ScriptsExecuting(events, 0, 1000)[None])

  def testParsing(self):
    css_url = u'http://example.com/style.css'
    html_url = u'http://example.com/yeah.htnl'
    raw_events = [
        {u'args': {u'data': {u'styleSheetUrl': css_url}},
         u'cat': u'blink,devtools.timeline',
         u'dur': 400 * 1000,
         u'name': u'ParseAuthorStyleSheet',
         u'ph': u'X',
         u'pid': 32723,
         u'tdur': 49721,
         u'tid': 32738,
         u'ts': 0,
         u'tts': 216148},
        {u'args': {u'beginData': {u'url': html_url}},
         u'cat': u'devtools.timeline',
         u'dur': 42 * 1000,
         u'name': u'ParseHTML',
         u'ph': u'X',
         u'pid': 32723,
         u'tdur': 49721,
         u'tid': 32738,
         u'ts': 0,
         u'tts': 5032310},]
    events = self._EventsFromRawEvents(raw_events)
    self.assertEquals(2, len(ActivityLens._Parsing(events, 0, 1000)))
    self.assertTrue(css_url in ActivityLens._Parsing(events, 0, 1000))
    self.assertEquals(400, ActivityLens._Parsing(events, 0, 1000)[css_url])
    self.assertTrue(html_url in ActivityLens._Parsing(events, 0, 1000))
    self.assertEquals(42, ActivityLens._Parsing(events, 0, 1000)[html_url])

  def testBreakdownEdgeActivityByInitiator(self):
    requests = [test_utils.MakeRequest(0, 1, 10, 20, 30),
                test_utils.MakeRequest(0, 1, 50, 60, 70)]
    raw_events = [
        {u'args': {u'beginData': {u'url': requests[0].url}},
         u'cat': u'devtools.timeline',
         u'dur': 12 * 1000,
         u'name': u'ParseHTML',
         u'ph': u'X',
         u'pid': 1,
         u'tid': 1,
         u'ts': 25 * 1000},
        {u'args': {u'data': {'scriptName': requests[0].url}},
         u'cat': u'devtools.timeline,v8',
         u'dur': 0,
         u'name': u'EvaluateScript',
         u'ph': u'X',
         u'pid': 1,
         u'tid': 1,
         u'ts': 0},
        {u'cat': u'toplevel',
         u'dur': 100 * 1000,
         u'name': u'MessageLoop::RunTask',
         u'ph': u'X',
         u'pid': 1,
         u'tid': 1,
         u'ts': 0},
        {u'args': {u'name': u'CrRendererMain'},
         u'cat': u'__metadata',
         u'name': u'thread_name',
         u'ph': u'M',
         u'pid': 1,
         u'tid': 1,
         u'ts': 0}]
    activity = self._ActivityLens(requests, raw_events)
    dep = (requests[0], requests[1], 'parser')
    self.assertEquals(
        {'unrelated_work': 18, 'idle': 0, 'script': 0, 'parsing': 12,
         'other_url': 0, 'unknown_url': 0},
        activity.BreakdownEdgeActivityByInitiator(dep))
    dep = (requests[0], requests[1], 'other')
    # Truncating the event from the parent request end.
    self.assertEquals(
        {'unrelated_work': 13, 'idle': 0, 'script': 0, 'parsing': 7,
         'other_url': 0, 'unknown_url': 0},
        activity.BreakdownEdgeActivityByInitiator(dep))
    # Unknown URL
    raw_events[0]['args']['beginData']['url'] = None
    activity = self._ActivityLens(requests, raw_events)
    dep = (requests[0], requests[1], 'parser')
    self.assertEquals(
        {'unrelated_work': 18, 'idle': 0, 'script': 0, 'parsing': 0,
         'other_url': 0, 'unknown_url': 12},
        activity.BreakdownEdgeActivityByInitiator(dep))
    # Script
    raw_events[1]['ts'] = 40 * 1000
    raw_events[1]['dur'] = 6 * 1000
    activity = self._ActivityLens(requests, raw_events)
    dep = (requests[0], requests[1], 'script')
    self.assertEquals(
        {'unrelated_work': 7, 'idle': 0, 'script': 6, 'parsing': 0,
         'other_url': 0, 'unknown_url': 7},
        activity.BreakdownEdgeActivityByInitiator(dep))
    # Other URL
    raw_events[1]['args']['data']['scriptName'] = 'http://other.com/url'
    activity = self._ActivityLens(requests, raw_events)
    self.assertEquals(
        {'unrelated_work': 7, 'idle': 0, 'script': 0., 'parsing': 0.,
         'other_url': 6., 'unknown_url': 7.},
        activity.BreakdownEdgeActivityByInitiator(dep))

  def testMainRendererThreadBusyness(self):
    raw_events =  [
        {u'args': {u'name': u'CrRendererMain'},
         u'cat': u'__metadata',
         u'name': u'thread_name',
         u'ph': u'M',
         u'pid': 1,
         u'tid': 12,
         u'ts': 0},
        {u'args': {},
         u'cat': u'toplevel',
         u'dur': 200 * 1000,
         u'name': u'MessageLoop::RunTask',
         u'ph': u'X',
         u'pid': 1,
         u'tid': 12,
         u'ts': 0,
         u'tts': 56485},
        {u'args': {},
         u'cat': u'toplevel',
         u'dur': 8 * 200,
         u'name': u'MessageLoop::NestedSomething',
         u'ph': u'X',
         u'pid': 1,
         u'tid': 12,
         u'ts': 0,
         u'tts': 0},
        {u'args': {},
         u'cat': u'toplevel',
         u'dur': 500 * 1000,
         u'name': u'MessageLoop::RunTask',
         u'ph': u'X',
         u'pid': 12,
         u'tid': 12,
         u'ts': 0,
         u'tts': 56485}]
    lens = self._ActivityLens([], raw_events)
    # Ignore events from another PID.
    self.assertEquals(200, lens.MainRendererThreadBusyness(0, 1000))
    # Clamping duration.
    self.assertEquals(100, lens.MainRendererThreadBusyness(0, 100))
    self.assertEquals(50, lens.MainRendererThreadBusyness(25, 75))
    # Other PID.
    raw_events[0]['pid'] = 12
    lens = self._ActivityLens([], raw_events)
    self.assertEquals(500, lens.MainRendererThreadBusyness(0, 1000))

  def _ActivityLens(self, requests, raw_events):
    loading_trace = test_utils.LoadingTraceFromEvents(
        requests, None, raw_events)
    return ActivityLens(loading_trace)


class EventsTreeTestCase(unittest.TestCase):
  FakeEvent = collections.namedtuple(
      'FakeEvent', ('name', 'start_msec', 'end_msec'))
  _ROOT_EVENT = FakeEvent('-1', 0, 20)
  _EVENTS = [
      FakeEvent('0', 2, 4), FakeEvent('1', 1, 5),
      FakeEvent('2', 6, 9),
      FakeEvent('3', 13, 14), FakeEvent('4', 14, 17), FakeEvent('5', 12, 18)]

  def setUp(self):
    self.tree = _EventsTree(self._ROOT_EVENT, copy.deepcopy(self._EVENTS))

  def testEventsTreeConstruction(self):
    self.assertEquals(self._ROOT_EVENT, self.tree.event)
    self.assertEquals(3, len(self.tree.children))
    self.assertEquals(self._EVENTS[1], self.tree.children[0].event)
    self.assertEquals(self._EVENTS[0], self.tree.children[0].children[0].event)
    self.assertEquals(self._EVENTS[2], self.tree.children[1].event)
    self.assertEquals([], self.tree.children[1].children)
    self.assertEquals(self._EVENTS[5], self.tree.children[2].event)
    self.assertEquals(2, len(self.tree.children[2].children))

  def testDominatingEventsWithNames(self):
    self.assertListEqual(
        [self._ROOT_EVENT], self.tree.DominatingEventsWithNames(('-1')))
    self.assertListEqual(
        [self._ROOT_EVENT], self.tree.DominatingEventsWithNames(('-1', '0')))
    self.assertListEqual(
        [self._EVENTS[1], self._EVENTS[5]],
        self.tree.DominatingEventsWithNames(('1', '5')))


if __name__ == '__main__':
  unittest.main()
