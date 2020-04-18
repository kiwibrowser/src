# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import json
import os
import shutil
import subprocess
import tempfile
import unittest

import loading_trace
import page_track
import sandwich_metrics as puller
import sandwich_runner
import request_track
import tracing_track


_BLINK_CAT = 'blink.user_timing'
_MEM_CAT = sandwich_runner.MEMORY_DUMP_CATEGORY
_START = 'requestStart'
_LOADS = 'loadEventStart'
_LOADE = 'loadEventEnd'
_NAVIGATION_START = 'navigationStart'
_PAINT = 'firstContentfulPaint'
_LAYOUT = 'firstLayout'

_MINIMALIST_TRACE_EVENTS = [
    {'ph': 'R', 'cat': _BLINK_CAT, 'name': _NAVIGATION_START, 'ts': 10000,
        'args': {'frame': '0'}},
    {'ph': 'R', 'cat': _BLINK_CAT, 'name': _START,  'ts': 20000,
        'args': {}},
    {'cat': _MEM_CAT,   'name': 'periodic_interval', 'pid': 1, 'ph': 'v',
        'ts': 1, 'args': {'dumps': {'allocators': {'malloc': {'attrs': {'size':{
            'units': 'bytes', 'value': '1af2', }}}}}}},
    {'ph': 'R', 'cat': _BLINK_CAT, 'name': _LAYOUT,  'ts': 24000,
        'args': {'frame': '0'}},
    {'ph': 'R', 'cat': _BLINK_CAT, 'name': _PAINT,  'ts': 31000,
        'args': {'frame': '0'}},
    {'ph': 'R', 'cat': _BLINK_CAT, 'name': _LOADS,  'ts': 35000,
        'args': {'frame': '0'}},
    {'ph': 'R', 'cat': _BLINK_CAT, 'name': _LOADE,  'ts': 40000,
        'args': {'frame': '0'}},
    {'cat': _MEM_CAT,   'name': 'periodic_interval', 'pid': 1, 'ph': 'v',
        'ts': 1, 'args': {'dumps': {'allocators': {'malloc': {'attrs': {'size':{
            'units': 'bytes', 'value': 'd704', }}}}}}},
    {'ph': 'M', 'cat': '__metadata', 'pid': 1, 'name': 'process_name', 'ts': 1,
        'args': {'name': 'Browser'}}]


def TracingTrack(events):
  return tracing_track.TracingTrack.FromJsonDict({
      'events': events,
      'categories': (sandwich_runner._TRACING_CATEGORIES +
          [sandwich_runner.MEMORY_DUMP_CATEGORY])})


def LoadingTrace(events):
  return loading_trace.LoadingTrace('http://a.com/', {},
                                    page_track.PageTrack(None),
                                    request_track.RequestTrack(None),
                                    TracingTrack(events))


class PageTrackTest(unittest.TestCase):
  def testGetBrowserPID(self):
    def RunHelper(expected, events):
      self.assertEquals(expected, puller._GetBrowserPID(TracingTrack(events)))

    RunHelper(123, [
        {'ph': 'M', 'ts': 0, 'pid': 354, 'cat': 'whatever0'},
        {'ph': 'M', 'ts': 0, 'pid': 354, 'cat': 'whatever1'},
        {'ph': 'M', 'ts': 0, 'pid': 354, 'cat': '__metadata',
            'name': 'thread_name'},
        {'ph': 'M', 'ts': 0, 'pid': 354, 'cat': '__metadata',
            'name': 'process_name', 'args': {'name': 'Renderer'}},
        {'ph': 'M', 'ts': 0, 'pid': 123, 'cat': '__metadata',
            'name': 'process_name', 'args': {'name': 'Browser'}},
        {'ph': 'M', 'ts': 0, 'pid': 354, 'cat': 'whatever0'}])

    with self.assertRaises(ValueError):
      RunHelper(123, [
          {'ph': 'M', 'ts': 0, 'pid': 354, 'cat': 'whatever0'},
          {'ph': 'M', 'ts': 0, 'pid': 354, 'cat': 'whatever1'}])

  def testGetBrowserDumpEvents(self):
    NAME = 'periodic_interval'

    def RunHelper(trace_events, browser_pid):
      trace_events = copy.copy(trace_events)
      trace_events.append({
          'pid': browser_pid,
          'cat': '__metadata',
          'name': 'process_name',
          'ph': 'M',
          'ts': 0,
          'args': {'name': 'Browser'}})
      return puller._GetBrowserDumpEvents(TracingTrack(trace_events))

    TRACE_EVENTS = [
        {'pid': 354, 'ts':  1000, 'cat': _MEM_CAT, 'ph': 'v', 'name': NAME},
        {'pid': 354, 'ts':  2000, 'cat': _MEM_CAT, 'ph': 'V'},
        {'pid': 672, 'ts':  3000, 'cat': _MEM_CAT, 'ph': 'v', 'name': NAME},
        {'pid': 123, 'ts':  4000, 'cat': _MEM_CAT, 'ph': 'v', 'name': 'foo'},
        {'pid': 123, 'ts':  5000, 'cat': _MEM_CAT, 'ph': 'v', 'name': NAME},
        {'pid': 123, 'ts':  6000, 'cat': _MEM_CAT, 'ph': 'V'},
        {'pid': 672, 'ts':  7000, 'cat': _MEM_CAT, 'ph': 'v', 'name': NAME},
        {'pid': 354, 'ts':  8000, 'cat': _MEM_CAT, 'ph': 'v', 'name': 'foo'},
        {'pid': 123, 'ts':  9000, 'cat': 'whatever1', 'ph': 'v', 'name': NAME},
        {'pid': 123, 'ts': 10000, 'cat': _MEM_CAT, 'ph': 'v', 'name': NAME},
        {'pid': 354, 'ts': 11000, 'cat': 'whatever0', 'ph': 'R'},
        {'pid': 672, 'ts': 12000, 'cat': _MEM_CAT, 'ph': 'v', 'name': NAME}]

    bump_events = RunHelper(TRACE_EVENTS, 123)
    self.assertEquals(2, len(bump_events))
    self.assertEquals(5, bump_events[0].start_msec)
    self.assertEquals(10, bump_events[1].start_msec)

    bump_events = RunHelper(TRACE_EVENTS, 354)
    self.assertEquals(1, len(bump_events))
    self.assertEquals(1, bump_events[0].start_msec)

    bump_events = RunHelper(TRACE_EVENTS, 672)
    self.assertEquals(3, len(bump_events))
    self.assertEquals(3, bump_events[0].start_msec)
    self.assertEquals(7, bump_events[1].start_msec)
    self.assertEquals(12, bump_events[2].start_msec)

    with self.assertRaises(ValueError):
      RunHelper(TRACE_EVENTS, 895)

  def testGetWebPageTrackedEvents(self):
    trace_events = puller._GetWebPageTrackedEvents(TracingTrack([
        {'ph': 'R', 'ts':  0000, 'args': {},             'cat': 'whatever',
            'name': _START},
        {'ph': 'R', 'ts':  1000, 'args': {'frame': '0'}, 'cat': 'whatever',
            'name': _LOADS},
        {'ph': 'R', 'ts':  2000, 'args': {'frame': '0'}, 'cat': 'whatever',
            'name': _LOADE},
        {'ph': 'R', 'ts':  3000, 'args': {},             'cat': _BLINK_CAT,
            'name': _START},
        {'ph': 'R', 'ts':  4000, 'args': {'frame': '0'}, 'cat': _BLINK_CAT,
            'name': _LOADS},
        {'ph': 'R', 'ts':  5000, 'args': {'frame': '0'}, 'cat': _BLINK_CAT,
            'name': _LOADE},
        {'ph': 'R', 'ts':  7000, 'args': {},             'cat': _BLINK_CAT,
            'name': _START},
        {'ph': 'R', 'ts':  8000, 'args': {'frame': '0'}, 'cat': _BLINK_CAT,
            'name': _LOADS},
        {'ph': 'R', 'ts':  9000, 'args': {'frame': '0'}, 'cat': _BLINK_CAT,
            'name': _LOADE},
        {'ph': 'R', 'ts': 11000, 'args': {'frame': '0'}, 'cat': 'whatever',
            'name': _START},
        {'ph': 'R', 'ts': 12000, 'args': {'frame': '0'}, 'cat': 'whatever',
            'name': _LOADS},
        {'ph': 'R', 'ts': 13000, 'args': {'frame': '0'}, 'cat': 'whatever',
            'name': _LOADE},
        {'ph': 'R', 'ts': 14000, 'args': {},             'cat': _BLINK_CAT,
            'name': _START},
        {'ph': 'R', 'ts': 10000, 'args': {'frame': '0'}, 'cat': _BLINK_CAT,
            'name': _NAVIGATION_START}, # Event out of |start_msec| order.
        {'ph': 'R', 'ts':  6000, 'args': {'frame': '0'}, 'cat': 'whatever',
            'name': _NAVIGATION_START},
        {'ph': 'R', 'ts': 15000, 'args': {},             'cat': _BLINK_CAT,
            'name': _START},
        {'ph': 'R', 'ts': 16000, 'args': {'frame': '1'}, 'cat': _BLINK_CAT,
            'name': _LOADS},
        {'ph': 'R', 'ts': 17000, 'args': {'frame': '0'}, 'cat': _BLINK_CAT,
            'name': _LOADS},
        {'ph': 'R', 'ts': 18000, 'args': {'frame': '1'}, 'cat': _BLINK_CAT,
            'name': _LOADE},
        {'ph': 'R', 'ts': 19000, 'args': {'frame': '0'}, 'cat': _BLINK_CAT,
            'name': _LOADE},
        {'ph': 'R', 'ts': 20000, 'args': {},             'cat': 'whatever',
            'name': _START},
        {'ph': 'R', 'ts': 21000, 'args': {'frame': '0'}, 'cat': 'whatever',
            'name': _LOADS},
        {'ph': 'R', 'ts': 22000, 'args': {'frame': '0'}, 'cat': 'whatever',
            'name': _LOADE},
        {'ph': 'R', 'ts': 23000, 'args': {},             'cat': _BLINK_CAT,
            'name': _START},
        {'ph': 'R', 'ts': 24000, 'args': {'frame': '0'}, 'cat': _BLINK_CAT,
            'name': _LOADS},
        {'ph': 'R', 'ts': 25000, 'args': {'frame': '0'}, 'cat': _BLINK_CAT,
            'name': _LOADE}]))

    self.assertEquals(3, len(trace_events))
    self.assertEquals(14, trace_events['requestStart'].start_msec)
    self.assertEquals(17, trace_events['loadEventStart'].start_msec)
    self.assertEquals(19, trace_events['loadEventEnd'].start_msec)

  def testExtractDefaultMetrics(self):
    metrics = puller._ExtractDefaultMetrics(LoadingTrace(
        _MINIMALIST_TRACE_EVENTS))
    self.assertEquals(4, len(metrics))
    self.assertEquals(20, metrics['total_load'])
    self.assertEquals(5, metrics['js_onload_event'])
    self.assertEquals(4, metrics['first_layout'])
    self.assertEquals(11, metrics['first_contentful_paint'])

  def testExtractDefaultMetricsBestEffort(self):
    metrics = puller._ExtractDefaultMetrics(LoadingTrace([
        {'ph': 'R', 'ts': 10000, 'args': {'frame': '0'}, 'cat': _BLINK_CAT,
            'name': _NAVIGATION_START},
        {'ph': 'R', 'ts': 11000, 'args': {'frame': '0'}, 'cat': 'whatever',
            'name': _START}]))
    self.assertEquals(4, len(metrics))
    self.assertEquals(puller._FAILED_CSV_VALUE, metrics['total_load'])
    self.assertEquals(puller._FAILED_CSV_VALUE, metrics['js_onload_event'])
    self.assertEquals(puller._FAILED_CSV_VALUE, metrics['first_layout'])
    self.assertEquals(puller._FAILED_CSV_VALUE,
                      metrics['first_contentful_paint'])

  def testExtractMemoryMetrics(self):
    metrics = puller._ExtractMemoryMetrics(LoadingTrace(
        _MINIMALIST_TRACE_EVENTS))
    self.assertEquals(2, len(metrics))
    self.assertEquals(30971, metrics['browser_malloc_avg'])
    self.assertEquals(55044, metrics['browser_malloc_max'])

  def testComputeSpeedIndex(self):
    def point(time, frame_completeness):
      return puller.CompletenessPoint(time=time,
                                      frame_completeness=frame_completeness)
    completness_record = [
      point(0, 0.0),
      point(120, 0.4),
      point(190, 0.75),
      point(280, 1.0),
      point(400, 1.0),
    ]
    self.assertEqual(120 + 70 * 0.6 + 90 * 0.25,
                     puller._ComputeSpeedIndex(completness_record))

    completness_record = [
      point(70, 0.0),
      point(150, 0.3),
      point(210, 0.6),
      point(220, 0.9),
      point(240, 1.0),
    ]
    self.assertEqual(80 + 60 * 0.7 + 10 * 0.4 + 20 * 0.1,
                     puller._ComputeSpeedIndex(completness_record))

    completness_record = [
      point(90, 0.0),
      point(200, 0.6),
      point(150, 0.3),
      point(230, 1.0),
    ]
    with self.assertRaises(ValueError):
      puller._ComputeSpeedIndex(completness_record)


if __name__ == '__main__':
  unittest.main()
