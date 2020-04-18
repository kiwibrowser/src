# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import devtools_monitor


class PageTrack(devtools_monitor.Track):
  """Records the events from the page track."""
  _METHODS = ('Page.frameStartedLoading', 'Page.frameStoppedLoading',
              'Page.frameAttached')
  FRAME_STARTED_LOADING = 'Page.frameStartedLoading'
  def __init__(self, connection):
    super(PageTrack, self).__init__(connection)
    self._connection = connection
    self._events = []
    self._pending_frames = set()
    self._known_frames = set()
    self._main_frame_id = None
    if self._connection:
      for method in PageTrack._METHODS:
        self._connection.RegisterListener(method, self)

  def Handle(self, method, msg):
    assert method in PageTrack._METHODS
    params = msg['params']
    frame_id = params['frameId']
    should_stop = False
    event = {'method': method, 'frame_id': frame_id}
    if method == self.FRAME_STARTED_LOADING:
      if self._main_frame_id is None:
        self._main_frame_id = params['frameId']
      self._pending_frames.add(frame_id)
      self._known_frames.add(frame_id)
    elif method == 'Page.frameStoppedLoading':
      assert frame_id in self._pending_frames
      self._pending_frames.remove(frame_id)
      if frame_id == self._main_frame_id:
        should_stop = True
    elif method == 'Page.frameAttached':
      self._known_frames.add(frame_id)
      parent_frame = params['parentFrameId']
      assert parent_frame in self._known_frames
      event['parent_frame_id'] = parent_frame
    self._events.append(event)
    if should_stop and self._connection:
      self._connection.StopMonitoring()

  def GetEvents(self):
    #TODO(lizeb): Add more checks here (child frame stops loading before parent,
    #for instance).
    return self._events

  def ToJsonDict(self):
    return {'events': [event for event in self._events]}

  def GetMainFrameId(self):
    """Returns the Id (str) of the main frame, or raises a ValueError."""
    for event in self._events:
      if event['method'] == self.FRAME_STARTED_LOADING:
        return event['frame_id']
    else:
      raise ValueError('No frame loads in the track.')

  @classmethod
  def FromJsonDict(cls, json_dict):
    assert 'events' in json_dict
    result = PageTrack(None)
    events = [event for event in json_dict['events']]
    result._events = events
    return result
