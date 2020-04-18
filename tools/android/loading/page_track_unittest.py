# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import devtools_monitor
from page_track import PageTrack

class MockDevToolsConnection(object):
  def __init__(self):
    self.stop_has_been_called = False

  def RegisterListener(self, name, listener):
    pass

  def StopMonitoring(self):
    self.stop_has_been_called = True


class PageTrackTest(unittest.TestCase):
  _EVENTS = [{'method': 'Page.frameStartedLoading',
              'params': {'frameId': '1234.1'}},
             {'method': 'Page.frameAttached',
              'params': {'frameId': '1234.12', 'parentFrameId': '1234.1'}},
             {'method': 'Page.frameStartedLoading',
              'params': {'frameId': '1234.12'}},
             {'method': 'Page.frameStoppedLoading',
              'params': {'frameId': '1234.12'}},
             {'method': 'Page.frameStoppedLoading',
              'params': {'frameId': '1234.1'}}]
  def testAsksMonitoringToStop(self):
    devtools_connection = MockDevToolsConnection()
    page_track = PageTrack(devtools_connection)
    for msg in PageTrackTest._EVENTS[:-1]:
      page_track.Handle(msg['method'], msg)
      self.assertFalse(devtools_connection.stop_has_been_called)
    msg = PageTrackTest._EVENTS[-1]
    page_track.Handle(msg['method'], msg)
    self.assertTrue(devtools_connection.stop_has_been_called)

  def testUnknownParent(self):
    page_track = PageTrack(None)
    msg = {'method': 'Page.frameAttached',
           'params': {'frameId': '1234.12', 'parentFrameId': '1234.1'}}
    with self.assertRaises(AssertionError):
      page_track.Handle(msg['method'], msg)

  def testStopsLoadingUnknownFrame(self):
    page_track = PageTrack(None)
    msg = {'method': 'Page.frameStoppedLoading',
           'params': {'frameId': '1234.12'}}
    with self.assertRaises(AssertionError):
      page_track.Handle(msg['method'], msg)

  def testGetMainFrameId(self):
    devtools_connection = MockDevToolsConnection()
    page_track = PageTrack(devtools_connection)
    for msg in PageTrackTest._EVENTS:
      page_track.Handle(msg['method'], msg)
    self.assertEquals('1234.1', page_track.GetMainFrameId())


if __name__ == '__main__':
  unittest.main()
