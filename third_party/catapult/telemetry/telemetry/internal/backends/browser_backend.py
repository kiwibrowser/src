# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import uuid
import sys

from py_utils import cloud_storage  # pylint: disable=import-error

from telemetry import decorators
from telemetry.core import exceptions
from telemetry.internal.backends import app_backend
from telemetry.internal.browser import web_contents


class ExtensionsNotSupportedException(Exception):
  pass


class BrowserBackend(app_backend.AppBackend):
  """A base class for browser backends."""

  def __init__(self, platform_backend, browser_options,
               supports_extensions, tab_list_backend):
    assert browser_options.browser_type
    super(BrowserBackend, self).__init__(browser_options.browser_type,
                                         platform_backend)
    self.browser_options = browser_options
    self._supports_extensions = supports_extensions
    self._tab_list_backend_class = tab_list_backend

  def SetBrowser(self, browser):
    super(BrowserBackend, self).SetApp(app=browser)

  @property
  def log_file_path(self):
    # Specific browser backend is responsible for overriding this properly.
    raise NotImplementedError

  def GetLogFileContents(self):
    if not self.log_file_path:
      return 'No log file'
    with file(self.log_file_path) as f:
      return f.read()

  def UploadLogsToCloudStorage(self):
    """ Uploading log files produce by this browser instance to cloud storage.

    Check supports_uploading_logs before calling this method.
    """
    assert self.supports_uploading_logs
    remote_path = (self.browser_options.logs_cloud_remote_path or
                   'log_%s' % uuid.uuid4())
    cloud_url = cloud_storage.Insert(
        bucket=self.browser_options.logs_cloud_bucket,
        remote_path=remote_path,
        local_path=self.log_file_path)
    sys.stderr.write('Uploading browser log to %s\n' % cloud_url)

  @property
  def browser(self):
    return self.app

  @property
  def browser_type(self):
    return self.app_type

  @property
  def supports_uploading_logs(self):
    # Specific browser backend is responsible for overriding this properly.
    return False

  @property
  def supports_extensions(self):
    """True if this browser backend supports extensions."""
    return self._supports_extensions

  @property
  def supports_tab_control(self):
    raise NotImplementedError()

  @property
  @decorators.Cache
  def tab_list_backend(self):
    return self._tab_list_backend_class(self)

  @property
  def supports_tracing(self):
    raise NotImplementedError()

  @property
  def supports_app_ui_interactions(self):
    return False

  @property
  def supports_system_info(self):
    return False

  def StartTracing(self,
                   trace_options,
                   timeout=web_contents.DEFAULT_WEB_CONTENTS_TIMEOUT):
    raise NotImplementedError()

  def StopTracing(self):
    raise NotImplementedError()

  def CollectTracingData(self, trace_data_builder):
    raise NotImplementedError()

  def Start(self, startup_args, startup_url=None):
    raise NotImplementedError()

  def IsBrowserRunning(self):
    raise NotImplementedError()

  def IsAppRunning(self):
    return self.IsBrowserRunning()

  def GetStandardOutput(self):
    raise NotImplementedError()

  def GetStackTrace(self):
    raise NotImplementedError()

  def GetMostRecentMinidumpPath(self):
    raise NotImplementedError()

  def GetAllMinidumpPaths(self):
    raise NotImplementedError()

  def GetAllUnsymbolizedMinidumpPaths(self):
    raise NotImplementedError()

  def SymbolizeMinidump(self, minidump_path):
    raise NotImplementedError()

  def GetSystemInfo(self):
    raise NotImplementedError()

  @property
  def supports_memory_dumping(self):
    return False

  def DumpMemory(self, timeout=None):
    raise NotImplementedError()

# pylint: disable=invalid-name
  @property
  def supports_overriding_memory_pressure_notifications(self):
    return False

  def SetMemoryPressureNotificationsSuppressed(
      self, suppressed, timeout=web_contents.DEFAULT_WEB_CONTENTS_TIMEOUT):
    raise NotImplementedError()

  def SimulateMemoryPressureNotification(
      self, pressure_level, timeout=web_contents.DEFAULT_WEB_CONTENTS_TIMEOUT):
    raise NotImplementedError()

  @property
  def supports_cpu_metrics(self):
    raise NotImplementedError()

  @property
  def supports_memory_metrics(self):
    raise NotImplementedError()

  @property
  def supports_power_metrics(self):
    raise NotImplementedError()

  @property
  def supports_overview_mode(self): # pylint: disable=invalid-name
    return False

  def EnterOverviewMode(self, timeout): # pylint: disable=unused-argument
    raise exceptions.StoryActionError('Overview mode is not supported')

  def ExitOverviewMode(self, timeout): # pylint: disable=unused-argument
    raise exceptions.StoryActionError('Overview mode is not supported')
