# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.internal.backends.chrome_inspector import inspector_websocket
from telemetry.internal.platform import system_info
from py_utils import camel_case


class SystemInfoBackend(object):
  def __init__(self, browser_target_ws):
    self._browser_target_ws = browser_target_ws

  def GetSystemInfo(self, timeout=10):
    req = {'method': 'SystemInfo.getInfo'}
    websocket = inspector_websocket.InspectorWebsocket()
    try:
      websocket.Connect(self._browser_target_ws, timeout)
      res = websocket.SyncRequest(req, timeout)
    finally:
      websocket.Disconnect()
    if 'error' in res:
      return None
    return system_info.SystemInfo.FromDict(
        camel_case.ToUnderscore(res['result']))

  def Close(self):
    pass
