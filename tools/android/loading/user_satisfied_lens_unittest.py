# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import request_track
import test_utils
import user_satisfied_lens


class TraceCreator(object):
  def __init__(self):
    self._request_index = 1

  def RequestAt(self, timestamp_msec, duration=1):
    timestamp_sec = float(timestamp_msec) / 1000
    rq = request_track.Request.FromJsonDict({
        'url': 'http://bla-%s-.com' % timestamp_msec,
        'request_id': '0.%s' % self._request_index,
        'frame_id': '123.%s' % timestamp_msec,
        'initiator': {'type': 'other'},
        'timestamp': timestamp_sec,
        'timing': {'request_time': timestamp_sec,
                   'loading_finished': duration}
        })
    self._request_index += 1
    return rq

  def CreateTrace(self, requests, events, main_frame_id):
    loading_trace = test_utils.LoadingTraceFromEvents(
        requests, trace_events=events)
    loading_trace.tracing_track.SetMainFrameID(main_frame_id)
    loading_trace.url = 'http://www.dummy.com'
    return loading_trace


class UserSatisfiedLensTestCase(unittest.TestCase):
  # We track all times in milliseconds, but raw trace events are in
  # microseconds.
  MILLI_TO_MICRO = 1000

  def setUp(self):
    super(UserSatisfiedLensTestCase, self).setUp()

  def testPLTLens(self):
    MAINFRAME = 1
    trace_creator = test_utils.TraceCreator()
    requests = [trace_creator.RequestAt(1), trace_creator.RequestAt(10),
                trace_creator.RequestAt(20)]
    loading_trace = trace_creator.CreateTrace(
        requests,
        [{'ts': 5 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'devtools.timeline', 'pid': 1, 'tid': 1,
          'name': 'MarkLoad',
          'args': {'data': {'isMainFrame': True}}},
         {'ts': 10 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'devtools.timeline', 'pid': 1, 'tid': 1,
          'name': 'MarkLoad',
          'args': {'data': {'isMainFrame': True}}},
         {'ts': 20 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'devtools.timeline', 'pid': 1, 'tid': 1,
          'name': 'MarkLoad',
          'args': {'data': {'isMainFrame': False}}}], MAINFRAME)
    lens = user_satisfied_lens.PLTLens(loading_trace)
    self.assertEqual(set(['0.1']), lens.CriticalRequestIds())
    self.assertEqual(10, lens.SatisfiedMs())

  def testFirstContentfulPaintLens(self):
    MAINFRAME = 1
    SUBFRAME = 2
    trace_creator = test_utils.TraceCreator()
    requests = [trace_creator.RequestAt(1), trace_creator.RequestAt(10),
                trace_creator.RequestAt(20)]
    loading_trace = trace_creator.CreateTrace(
        requests,
        [{'ts': 0, 'ph': 'I',
          'cat': 'blink.some_other_user_timing',
          'name': 'firstContentfulPaint'},
         {'ts': 30 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink.user_timing',
          'name': 'firstDiscontentPaint'},
         {'ts': 5 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink.user_timing',
          'name': 'firstContentfulPaint',
          'args': {'frame': SUBFRAME} },
         {'ts': 12 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink.user_timing',
          'name': 'firstContentfulPaint',
          'args': {'frame': MAINFRAME}}], MAINFRAME)
    lens = user_satisfied_lens.FirstContentfulPaintLens(loading_trace)
    self.assertEqual(set(['0.1', '0.2']), lens.CriticalRequestIds())
    self.assertEqual(1, lens.PostloadTimeMsec())

  def testCantGetNoSatisfaction(self):
    MAINFRAME = 1
    trace_creator = test_utils.TraceCreator()
    requests = [trace_creator.RequestAt(1), trace_creator.RequestAt(10),
                trace_creator.RequestAt(20)]
    loading_trace = trace_creator.CreateTrace(
        requests,
        [{'ts': 0, 'ph': 'I',
          'cat': 'not_my_cat',
          'name': 'someEvent',
          'args': {'frame': MAINFRAME}}], MAINFRAME)
    loading_trace.tracing_track.SetMainFrameID(MAINFRAME)
    lens = user_satisfied_lens.FirstContentfulPaintLens(loading_trace)
    self.assertEqual(set(['0.1', '0.2', '0.3']), lens.CriticalRequestIds())
    self.assertEqual(float('inf'), lens.PostloadTimeMsec())

  def testFirstTextPaintLens(self):
    MAINFRAME = 1
    SUBFRAME = 2
    trace_creator = test_utils.TraceCreator()
    requests = [trace_creator.RequestAt(1), trace_creator.RequestAt(10),
                trace_creator.RequestAt(20)]
    loading_trace = trace_creator.CreateTrace(
        requests,
        [{'ts': 0, 'ph': 'I',
          'cat': 'blink.some_other_user_timing',
          'name': 'firstPaint'},
         {'ts': 30 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink.user_timing',
          'name': 'firstishPaint',
          'args': {'frame': MAINFRAME}},
         {'ts': 3 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink.user_timing',
          'name': 'firstPaint',
          'args': {'frame': SUBFRAME}},
         {'ts': 12 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink.user_timing',
          'name': 'firstPaint',
          'args': {'frame': MAINFRAME}}], MAINFRAME)
    loading_trace.tracing_track.SetMainFrameID(MAINFRAME)
    lens = user_satisfied_lens.FirstTextPaintLens(loading_trace)
    self.assertEqual(set(['0.1', '0.2']), lens.CriticalRequestIds())
    self.assertEqual(1, lens.PostloadTimeMsec())

  def testFirstSignificantPaintLens(self):
    MAINFRAME = 1
    trace_creator = test_utils.TraceCreator()
    requests = [trace_creator.RequestAt(1), trace_creator.RequestAt(10),
                trace_creator.RequestAt(15), trace_creator.RequestAt(20)]
    loading_trace = trace_creator.CreateTrace(
        requests,
        [{'ts': 0, 'ph': 'I',
          'cat': 'blink',
          'name': 'firstPaint'},
         {'ts': 9 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink.user_timing',
          'name': 'FrameView::paintTree'},
         {'ts': 18 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink',
          'name': 'FrameView::paintTree'},
         {'ts': 22 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink',
          'name': 'FrameView::paintTree'},
         {'ts': 5 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'foobar', 'name': 'biz',
          'args': {'counters': {
              'LayoutObjectsThatHadNeverHadLayout': 10
          } } },
         {'ts': 12 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'foobar', 'name': 'biz',
          'args': {'counters': {
              'LayoutObjectsThatHadNeverHadLayout': 12
          } } },
         {'ts': 15 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'foobar', 'name': 'biz',
          'args': {'counters': {
              'LayoutObjectsThatHadNeverHadLayout': 10
          } } } ], MAINFRAME)
    lens = user_satisfied_lens.FirstSignificantPaintLens(loading_trace)
    self.assertEqual(set(['0.1', '0.2']), lens.CriticalRequestIds())
    self.assertEqual(7, lens.PostloadTimeMsec())

  def testRequestFingerprintLens(self):
    MAINFRAME = 1
    SUBFRAME = 2
    trace_creator = test_utils.TraceCreator()
    requests = [trace_creator.RequestAt(1), trace_creator.RequestAt(10),
                trace_creator.RequestAt(20)]
    loading_trace = trace_creator.CreateTrace(
        requests,
        [{'ts': 0, 'ph': 'I',
          'cat': 'blink.some_other_user_timing',
          'name': 'firstContentfulPaint'},
         {'ts': 30 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink.user_timing',
          'name': 'firstDiscontentPaint'},
         {'ts': 5 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink.user_timing',
          'name': 'firstContentfulPaint',
          'args': {'frame': SUBFRAME} },
         {'ts': 12 * self.MILLI_TO_MICRO, 'ph': 'I',
          'cat': 'blink.user_timing',
          'name': 'firstContentfulPaint',
          'args': {'frame': MAINFRAME}}], MAINFRAME)
    lens = user_satisfied_lens.FirstContentfulPaintLens(loading_trace)
    self.assertEqual(set(['0.1', '0.2']), lens.CriticalRequestIds())
    self.assertEqual(1, lens.PostloadTimeMsec())
    request_lens = user_satisfied_lens.RequestFingerprintLens(
      loading_trace, lens.CriticalFingerprints())
    self.assertEqual(set(['0.1', '0.2']), request_lens.CriticalRequestIds())
    self.assertEqual(0, request_lens.PostloadTimeMsec())


if __name__ == '__main__':
  unittest.main()
