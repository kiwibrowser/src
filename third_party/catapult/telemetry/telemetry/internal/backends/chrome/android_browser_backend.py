# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from telemetry.core import exceptions
from telemetry.internal.platform import android_platform_backend as \
  android_platform_backend_module
from telemetry.internal.backends.chrome import chrome_browser_backend
from telemetry.internal.browser import user_agent

from devil.android import app_ui
from devil.android import device_signal
from devil.android.sdk import intent


class AndroidBrowserBackend(chrome_browser_backend.ChromeBrowserBackend):
  """The backend for controlling a browser instance running on Android."""
  def __init__(self, android_platform_backend, browser_options,
               browser_directory, profile_directory, backend_settings):
    assert isinstance(android_platform_backend,
                      android_platform_backend_module.AndroidPlatformBackend)
    super(AndroidBrowserBackend, self).__init__(
        android_platform_backend,
        browser_options=browser_options,
        browser_directory=browser_directory,
        profile_directory=profile_directory,
        supports_extensions=False,
        supports_tab_control=backend_settings.supports_tab_control)
    self._backend_settings = backend_settings

    # Initialize fields so that an explosion during init doesn't break in Close.
    self._saved_sslflag = ''
    self._app_ui = None

    # Set the debug app if needed.
    self.platform_backend.SetDebugApp(self._backend_settings.package)

  @property
  def log_file_path(self):
    return None

  @property
  def device(self):
    return self.platform_backend.device

  @property
  def supports_app_ui_interactions(self):
    return True

  def GetAppUi(self):
    if self._app_ui is None:
      self._app_ui = app_ui.AppUi(self.device, package=self.package)
    return self._app_ui

  def _StopBrowser(self):
    # Note: it's important to stop and _not_ kill the browser app, since
    # stopping also clears the app state in Android's activity manager.
    self.platform_backend.StopApplication(self._backend_settings.package)

  def Start(self, startup_args, startup_url=None):
    assert not startup_args, (
        'Startup arguments for Android should be set during '
        'possible_browser.SetUpEnvironment')
    user_agent_dict = user_agent.GetChromeUserAgentDictFromType(
        self.browser_options.browser_user_agent_type)
    self.device.StartActivity(
        intent.Intent(package=self._backend_settings.package,
                      activity=self._backend_settings.activity,
                      action=None, data=startup_url, category=None,
                      extras=user_agent_dict),
        blocking=True)
    try:
      self.BindDevToolsClient()
    except:
      self.Close()
      raise

  def BindDevToolsClient(self):
    super(AndroidBrowserBackend, self).BindDevToolsClient()
    package = self.devtools_client.GetVersion().get('Android-Package')
    if package is None:
      logging.warning('Could not determine package name from DevTools client.')
    elif package == self._backend_settings.package:
      logging.info('Successfully connected to %s DevTools client', package)
    else:
      raise exceptions.BrowserGoneException(
          self.browser, 'Expected connection to %s but got %s.' % (
              self._backend_settings.package, package))

  def _FindDevToolsPortAndTarget(self):
    devtools_port = self._backend_settings.GetDevtoolsRemotePort(self.device)
    browser_target = None  # Use default
    return devtools_port, browser_target

  def Foreground(self):
    package = self._backend_settings.package
    activity = self._backend_settings.activity
    self.device.StartActivity(
        intent.Intent(package=package,
                      activity=activity,
                      action=None,
                      flags=[intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED]),
        blocking=False)
    # TODO(crbug.com/601052): The following waits for any UI node for the
    # package launched to appear on the screen. When the referenced bug is
    # fixed, remove this workaround and just switch blocking above to True.
    try:
      app_ui.AppUi(self.device).WaitForUiNode(package=package)
    except Exception:
      raise exceptions.BrowserGoneException(
          self.browser,
          'Timed out waiting for browser to come back foreground.')

  def Background(self):
    package = 'org.chromium.push_apps_to_background'
    activity = package + '.PushAppsToBackgroundActivity'
    self.device.StartActivity(
        intent.Intent(
            package=package,
            activity=activity,
            action=None,
            flags=[intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED]),
        blocking=True)

  def GetBrowserStartupUrl(self):
    # TODO(crbug.com/787834): Move to the corresponding possible-browser class.
    if self.browser_options.startup_url:
      return self.browser_options.startup_url
    elif self.browser_options.profile_dir:
      return None
    else:
      # If we have no existing tabs start with a blank page since default
      # startup with the NTP can lead to race conditions with Telemetry
      return 'about:blank'

  def ForceJavaHeapGarbageCollection(self):
    # Send USR1 signal to force GC on Chrome processes forked from Zygote.
    # (c.f. crbug.com/724032)
    self.device.KillAll(
        self._backend_settings.package,
        exact=False,  # Send signal to children too.
        signum=device_signal.SIGUSR1)

  @property
  def processes(self):
    try:
      zygotes = self.device.ListProcesses('zygote')
      zygote_pids = set(p.pid for p in zygotes)
      assert zygote_pids, 'No Android zygote found'
      processes = self.device.ListProcesses(self._backend_settings.package)
      return [p for p in processes if p.ppid in zygote_pids]
    except Exception as exc:
      # Re-raise as an AppCrashException to get further diagnostic information.
      # In particular we also get the values of all local variables above.
      raise exceptions.AppCrashException(
          self.browser, 'Error getting browser PIDs: %s' % exc)

  @property
  def pid(self):
    package = self._backend_settings.package
    browser_processes = [p for p in self.processes if p.name == package]
    assert len(browser_processes) <= 1, (
        'Found too many browsers: %r' % browser_processes)
    if not browser_processes:
      raise exceptions.BrowserGoneException(self.browser)
    return browser_processes[0].pid

  @property
  def package(self):
    return self._backend_settings.package

  @property
  def activity(self):
    return self._backend_settings.activity

  def __del__(self):
    self.Close()

  def Close(self):
    super(AndroidBrowserBackend, self).Close()
    self._StopBrowser()

  def IsBrowserRunning(self):
    return self.platform_backend.IsAppRunning(self._backend_settings.package)

  def GetStandardOutput(self):
    return self.platform_backend.GetStandardOutput()

  def GetStackTrace(self):
    return self.platform_backend.GetStackTrace()

  def GetMostRecentMinidumpPath(self):
    return None

  def GetAllMinidumpPaths(self):
    return None

  def GetAllUnsymbolizedMinidumpPaths(self):
    return None

  def SymbolizeMinidump(self, minidump_path):
    return None
