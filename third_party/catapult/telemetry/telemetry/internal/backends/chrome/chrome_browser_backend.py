# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import pprint
import shlex
import socket
import sys

from telemetry.core import exceptions
from telemetry import decorators
from telemetry.internal.backends import browser_backend
from telemetry.internal.backends.chrome import extension_backend
from telemetry.internal.backends.chrome import tab_list_backend
from telemetry.internal.backends.chrome_inspector import devtools_client_backend
from telemetry.internal.backends.chrome_inspector import websocket
from telemetry.internal.browser import web_contents
from telemetry.testing import options_for_unittests

import py_utils


class ChromeBrowserBackend(browser_backend.BrowserBackend):
  """An abstract class for chrome browser backends. Provides basic functionality
  once a remote-debugger port has been established."""
  # It is OK to have abstract methods. pylint: disable=abstract-method

  def __init__(self, platform_backend, browser_options,
               browser_directory, profile_directory,
               supports_extensions, supports_tab_control):
    super(ChromeBrowserBackend, self).__init__(
        platform_backend=platform_backend,
        browser_options=browser_options,
        supports_extensions=supports_extensions,
        tab_list_backend=tab_list_backend.TabListBackend)
    self._browser_directory = browser_directory
    self._profile_directory = profile_directory
    self._supports_tab_control = supports_tab_control

    self._devtools_client = None
    # TODO(crbug.com/799415): Move forwarder into DevToolsClientBackend
    self._forwarder = None

    self._extensions_to_load = browser_options.extensions_to_load
    if not supports_extensions and len(self._extensions_to_load) > 0:
      raise browser_backend.ExtensionsNotSupportedException(
          'Extensions are not supported on the selected browser')

    if (self.browser_options.dont_override_profile and
        not options_for_unittests.AreSet()):
      sys.stderr.write('Warning: Not overriding profile. This can cause '
                       'unexpected effects due to profile-specific settings, '
                       'such as about:flags settings, cookies, and '
                       'extensions.\n')

  @property
  def devtools_client(self):
    return self._devtools_client

  @property
  @decorators.Cache
  def extension_backend(self):
    if not self.supports_extensions:
      return None
    return extension_backend.ExtensionBackendDict(self)

  def _ArgsNeedProxyServer(self, args):
    """Returns True if args for Chrome indicate the need for proxy server."""
    if '--enable-spdy-proxy-auth' in args:
      return True
    return [arg for arg in args if arg.startswith('--proxy-server=')]

  def GetBrowserStartupUrl(self):
    # TODO(crbug.com/787834): Move to the corresponding possible-browser class.
    return None

  def HasDevToolsConnection(self):
    return self._devtools_client and self._devtools_client.IsAlive()

  def _FindDevToolsPortAndTarget(self):
    """Clients should return a (devtools_port, browser_target) pair.

    May also raise EnvironmentError (IOError or OSError) if this information
    could not be determined; the call will be retried until it succeeds or
    a timeout is met.
    """
    raise NotImplementedError

  def _GetDevToolsClient(self):
    # If the agent does not appear to be ready, it could be because we got the
    # details of an older agent that no longer exists. It's thus important to
    # re-read and update the port and target on each retry.
    try:
      devtools_port, browser_target = self._FindDevToolsPortAndTarget()
    except EnvironmentError:
      return None  # Port information not ready, will retry.

    # Since the method may be called multiple times due to retries, we need to
    # restart the forwarder if the ports changed.
    if (self._forwarder is not None and
        self._forwarder.remote_port != devtools_port):
      self._forwarder.Close()
      self._forwarder = None

    if self._forwarder is None:
      # When running on a local platform this creates a DoNothingForwarder,
      # and by setting local_port=None we let the forwarder choose a port.
      self._forwarder = self.platform_backend.forwarder_factory.Create(
          local_port=None, remote_port=devtools_port, reverse=True)

    devtools_config = devtools_client_backend.DevToolsClientConfig(
        local_port=self._forwarder.local_port,
        remote_port=self._forwarder.remote_port,
        browser_target=browser_target,
        app_backend=self)

    logging.info('Got devtools config: %s', devtools_config)
    if not devtools_config.IsAgentReady():
      return None  # Will retry.

    return devtools_config.Create()

  def BindDevToolsClient(self):
    """Find an existing DevTools agent and bind this browser backend to it."""
    if self._devtools_client:
      # In case we are launching a second browser instance (as is done by
      # the CrOS backend), ensure that the old devtools_client is closed,
      # otherwise re-creating it will fail.
      self._devtools_client.Close()
      self._devtools_client = None

    try:
      self._devtools_client = py_utils.WaitFor(
          self._GetDevToolsClient,
          timeout=self.browser_options.browser_startup_timeout)
    except (py_utils.TimeoutException, exceptions.ProcessGoneException) as e:
      if not self.IsBrowserRunning():
        raise exceptions.BrowserGoneException(self.browser, e)
      raise exceptions.BrowserConnectionGoneException(self.browser, e)

  def _WaitForExtensionsToLoad(self):
    """ Wait for all extensions to load.
    Be sure to check whether the browser_backend supports_extensions before
    calling this method.
    """
    assert self._supports_extensions
    assert self._devtools_client, (
        'Waiting for extensions required devtool client to be initiated first')
    try:
      py_utils.WaitFor(self._AllExtensionsLoaded, timeout=60)
    except py_utils.TimeoutException:
      logging.error('ExtensionsToLoad: ' + repr(
          [e.extension_id for e in self._extensions_to_load]))
      logging.error('Extension list: ' + pprint.pformat(
          self.extension_backend, indent=4))
      raise

  def _AllExtensionsLoaded(self):
    # Extension pages are loaded from an about:blank page,
    # so we need to check that the document URL is the extension
    # page in addition to the ready state.
    for e in self._extensions_to_load:
      try:
        extension_objects = self.extension_backend[e.extension_id]
      except KeyError:
        return False
      for extension_object in extension_objects:
        try:
          res = extension_object.EvaluateJavaScript(
              """
              document.URL.lastIndexOf({{ url }}, 0) == 0 &&
              (document.readyState == 'complete' ||
               document.readyState == 'interactive')
              """,
              url='chrome-extension://%s/' % e.extension_id)
        except exceptions.EvaluateException:
          # If the inspected page is not ready, we will get an error
          # when we evaluate a JS expression, but we can just keep polling
          # until the page is ready (crbug.com/251913).
          res = None

        # TODO(tengs): We don't have full support for getting the Chrome
        # version before launch, so for now we use a generic workaround to
        # check for an extension binding bug in old versions of Chrome.
        # See crbug.com/263162 for details.
        if res and extension_object.EvaluateJavaScript(
            'chrome.runtime == null'):
          extension_object.Reload()
        if not res:
          return False
    return True

  @property
  def browser_directory(self):
    return self._browser_directory

  @property
  def profile_directory(self):
    return self._profile_directory

  @property
  def supports_tab_control(self):
    return self._supports_tab_control

  @property
  def supports_tracing(self):
    return True

  def StartTracing(self, trace_options,
                   timeout=web_contents.DEFAULT_WEB_CONTENTS_TIMEOUT):
    """
    Args:
        trace_options: An tracing_options.TracingOptions instance.
    """
    return self.devtools_client.StartChromeTracing(trace_options, timeout)

  def StopTracing(self):
    self.devtools_client.StopChromeTracing()

  def CollectTracingData(self, trace_data_builder):
    self.devtools_client.CollectChromeTracingData(trace_data_builder)

  def GetProcessName(self, cmd_line):
    """Returns a user-friendly name for the process of the given |cmd_line|."""
    if not cmd_line:
      # TODO(tonyg): Eventually we should make all of these known and add an
      # assertion.
      return 'unknown'
    if 'nacl_helper_bootstrap' in cmd_line:
      return 'nacl_helper_bootstrap'
    if ':sandboxed_process' in cmd_line:
      return 'renderer'
    if ':privileged_process' in cmd_line:
      return 'gpu-process'
    args = shlex.split(cmd_line)
    types = [arg.split('=')[1] for arg in args if arg.startswith('--type=')]
    if not types:
      return 'browser'
    return types[0]

  @staticmethod
  def GetThreadType(thread_name):
    if not thread_name:
      return 'unknown'
    if (thread_name.startswith('Chrome_ChildIO') or
        thread_name.startswith('Chrome_IO')):
      return 'io'
    if thread_name.startswith('Compositor'):
      return 'compositor'
    if thread_name.startswith('CrGpuMain'):
      return 'gpu'
    if (thread_name.startswith('ChildProcessMai') or
        thread_name.startswith('CrRendererMain')):
      return 'main'
    if thread_name.startswith('RenderThread'):
      return 'render'

  def Close(self):
    if self._devtools_client:
      self._devtools_client.Close()
      self._devtools_client = None
    if self._forwarder:
      self._forwarder.Close()
      self._forwarder = None

  @property
  def supports_system_info(self):
    return self.GetSystemInfo() != None

  def GetSystemInfo(self):
    try:
      return self.devtools_client.GetSystemInfo()
    except (websocket.WebSocketException, socket.error) as e:
      if not self.IsBrowserRunning():
        raise exceptions.BrowserGoneException(self.browser, e)
      raise exceptions.BrowserConnectionGoneException(self.browser, e)

  @property
  def supports_memory_dumping(self):
    return True

  def DumpMemory(self, timeout=None):
    return self.devtools_client.DumpMemory(timeout=timeout)

  @property
  def supports_overriding_memory_pressure_notifications(self):
    return True

  def SetMemoryPressureNotificationsSuppressed(
      self, suppressed, timeout=web_contents.DEFAULT_WEB_CONTENTS_TIMEOUT):
    self.devtools_client.SetMemoryPressureNotificationsSuppressed(
        suppressed, timeout)

  def SimulateMemoryPressureNotification(
      self, pressure_level, timeout=web_contents.DEFAULT_WEB_CONTENTS_TIMEOUT):
    self.devtools_client.SimulateMemoryPressureNotification(
        pressure_level, timeout)

  def GetDirectoryPathsToFlushOsPageCacheFor(self):
    """ Return a list of directories to purge from OS page cache.

    Will only be called when page cache clearing is necessary for a benchmark.
    The caller will then attempt to purge all files from OS page cache for each
    returned directory recursively.
    """
    paths_to_flush = []
    if self.profile_directory:
      paths_to_flush.append(self.profile_directory)
    if self.browser_directory:
      paths_to_flush.append(self.browser_directory)
    return paths_to_flush

  # TODO(crbug.com/787834): consider migrating profile_directory &
  # browser_directory out of browser_backend so we don't have to rely on
  # creating browser_backend before clearing browser caches.
  def ClearCaches(self):
    """ Clear system caches related to browser.

    This clears DNS caches, then clears OS page cache on file paths that are
    related to the browser (iff
    browser_options.clear_sytem_cache_for_browser_and_profile_on_start is True).

    Note: this is done with best effort and may have no actual effects on the
    system.
    """
    platform = self.platform_backend.platform
    platform.FlushDnsCache()
    if self.browser_options.clear_sytem_cache_for_browser_and_profile_on_start:
      paths_to_flush = self.GetDirectoryPathsToFlushOsPageCacheFor()
      if (platform.CanFlushIndividualFilesFromSystemCache() and
          paths_to_flush):
        for path in paths_to_flush:
          platform.FlushSystemCacheForDirectory(path)
      elif platform.SupportFlushEntireSystemCache():
        platform.FlushEntireSystemCache()
      else:
        logging.warning(
            'Flush system cache is not supported. Did not flush OS page cache.')

  @property
  def supports_cpu_metrics(self):
    return True

  @property
  def supports_memory_metrics(self):
    return True

  @property
  def supports_power_metrics(self):
    return True
