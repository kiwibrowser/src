# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.core import exceptions


class InspectorStorage(object):
  def __init__(self, inspector_websocket):
    self._websocket = inspector_websocket
    self._websocket.RegisterDomain('Storage', self._OnNotification)

  def _OnNotification(self, msg):
    # TODO: track storage events
    # (https://chromedevtools.github.io/devtools-protocol/tot/Storage/)
    pass

  def ClearDataForOrigin(self, url, timeout):
    res = self._websocket.SyncRequest(
        {'method': 'Storage.clearDataForOrigin',
         'params': {
             'origin': url,
             'storageTypes': 'all',
         }}, timeout)
    if 'error' in res:
      raise exceptions.StoryActionError(res['error']['message'])
