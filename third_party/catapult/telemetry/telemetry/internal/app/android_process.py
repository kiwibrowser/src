# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.internal.backends.chrome_inspector import devtools_client_backend
from telemetry.internal.browser import web_contents


class WebViewNotFoundException(Exception):
  pass


class AndroidProcess(object):
  """Represents a single android process."""

  def __init__(self, app_backend, pid, name):
    self._app_backend = app_backend
    self._pid = pid
    self._name = name
    # TODO(crbug.com/799415): Move forwarder into DevToolsClientBackend
    self._forwarder = None
    self._devtools_client = None

  def __del__(self):
    if self._devtools_client:
      self._devtools_client.Close()
    if self._forwarder:
      self._forwarder.Close()

  @property
  def pid(self):
    return self._pid

  @property
  def name(self):
    return self._name

  @property
  def _remote_devtools_port(self):
    return 'localabstract:webview_devtools_remote_%s' % str(self.pid)

  def _UpdateDevToolsClient(self):
    if self._devtools_client is None:
      platform_backend = self._app_backend.platform_backend
      self._forwarder = platform_backend.forwarder_factory.Create(
          local_port=0, remote_port=self._remote_devtools_port, reverse=True)
      devtools_config = devtools_client_backend.DevToolsClientConfig(
          local_port=self._forwarder.local_port,
          remote_port=self._forwarder.remote_port,
          app_backend=self._app_backend)
      if devtools_config.IsAgentReady():
        self._devtools_client = devtools_config.Create()

  def GetWebViews(self):
    webviews = []
    self._UpdateDevToolsClient()
    if self._devtools_client is not None:
      devtools_context_map = (
          self._devtools_client.GetUpdatedInspectableContexts())
      for context in devtools_context_map.contexts:
        webviews.append(web_contents.WebContents(
            devtools_context_map.GetInspectorBackend(context['id'])))
    return webviews
