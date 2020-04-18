# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import os
import shutil
import tempfile
import textwrap

from telemetry.internal.util import binary_manager
from telemetry.util import statistics

from devil.android.sdk import version_codes

__all__ = ['BrowserIntervalProfilingController']

class BrowserIntervalProfilingController(object):
  def __init__(self, possible_browser, process_name, periods, frequency):
    process_name, _, thread_name = process_name.partition(':')
    self._process_name = process_name
    self._thread_name = thread_name
    self._periods = periods
    self._frequency = statistics.Clamp(int(frequency), 1, 4000)
    self._platform_controller = None
    if periods:
      self._platform_controller = self._CreatePlatformController(
          possible_browser)

  @staticmethod
  def _CreatePlatformController(possible_browser):
    os_name = possible_browser._platform_backend.GetOSName()
    if os_name == 'linux':
      possible_browser.AddExtraBrowserArg('--no-sandbox')
      return _LinuxController(possible_browser)
    elif os_name == 'android' and (
        possible_browser._platform_backend.device.build_version_sdk >=
        version_codes.NOUGAT):
      return _AndroidController(possible_browser)
    return None

  @contextlib.contextmanager
  def SamplePeriod(self, period, action_runner):
    if not self._platform_controller or period not in self._periods:
      yield
      return

    with self._platform_controller.SamplePeriod(
        period=period,
        action_runner=action_runner,
        process_name=self._process_name,
        thread_name=self._thread_name,
        frequency=self._frequency):
      yield

  def GetResults(self, page_name, file_safe_name, results):
    if self._platform_controller:
      self._platform_controller.GetResults(
          page_name, file_safe_name, results)


class _PlatformController(object):
  def SamplePeriod(self, period, action_runner):
    raise NotImplementedError()

  def GetResults(self, page_name, file_safe_name, results):
    raise NotImplementedError()


class _LinuxController(_PlatformController):
  def __init__(self, _):
    super(_LinuxController, self).__init__()
    self._temp_results = []

  @contextlib.contextmanager
  def SamplePeriod(self, period, action_runner, **_):
    (fd, out_file) = tempfile.mkstemp()
    os.close(fd)
    action_runner.ExecuteJavaScript(textwrap.dedent("""
        if (typeof chrome.gpuBenchmarking.startProfiling !== "undefined") {
          chrome.gpuBenchmarking.startProfiling("%s");
        }""" % out_file))
    yield
    action_runner.ExecuteJavaScript(textwrap.dedent("""
        if (typeof chrome.gpuBenchmarking.stopProfiling !== "undefined") {
          chrome.gpuBenchmarking.stopProfiling();
        }"""))
    self._temp_results.append((period, out_file))

  def GetResults(self, page_name, file_safe_name, results):
    for period, temp_file in self._temp_results:
      prefix = '%s-%s-' % (file_safe_name, period)
      with results.CreateArtifact(
          page_name, 'pprof', prefix=prefix, suffix='.profile.pb') as dest_fh:
        with open(temp_file, 'rb') as src_fh:
          shutil.copyfileobj(src_fh, dest_fh.file)
        os.remove(temp_file)
    self._temp_results = []


class _AndroidController(_PlatformController):
  DEVICE_PROFILERS_DIR = '/data/local/tmp/profilers'
  DEVICE_OUT_FILE_PATTERN = '/data/local/tmp/%s-perf.data'

  def __init__(self, possible_browser):
    super(_AndroidController, self).__init__()
    self._device = possible_browser._platform_backend.device
    self._device_simpleperf_path = self._InstallSimpleperf(possible_browser)
    self._device_results = []

  @classmethod
  def _InstallSimpleperf(cls, possible_browser):
    device = possible_browser._platform_backend.device
    package = possible_browser._backend_settings.package

    # Necessary for profiling
    # https://android-review.googlesource.com/c/platform/system/sepolicy/+/234400
    device.SetProp('security.perf_harden', '0')

    # This is the architecture of the app to be profiled, not of the device.
    package_arch = device.GetPackageArchitecture(package) or 'armeabi-v7a'
    host_path = binary_manager.FetchPath(
        'simpleperf', package_arch, 'android')
    if not host_path:
      raise Exception('Could not get path to simpleperf executable on host.')
    device_path = os.path.join(cls.DEVICE_PROFILERS_DIR,
                               package_arch,
                               'simpleperf')
    device.PushChangedFiles([(host_path, device_path)])
    return device_path

  @staticmethod
  def _ThreadsForProcess(device, pid):
    if device.build_version_sdk >= version_codes.OREO:
      pid_regex = (
          '^[[:graph:]]\\{1,\\}[[:blank:]]\\{1,\\}%s[[:blank:]]\\{1,\\}' % pid)
      ps_cmd = "ps -T -e | grep '%s'" % pid_regex
      ps_output_lines = device.RunShellCommand(
          ps_cmd, shell=True, check_return=True)
    else:
      ps_cmd = ['ps', '-p', pid, '-t']
      ps_output_lines = device.RunShellCommand(ps_cmd, check_return=True)
    result = []
    for l in ps_output_lines:
      fields = l.split()
      if fields[2] == pid:
        continue
      result.append((fields[2], fields[-1]))
    return result

  def _StartSimpleperf(
      self, browser, out_file, process_name, thread_name, frequency):
    device = browser._platform_backend.device

    processes = [p for p in browser._browser_backend.processes
                 if (browser._browser_backend.GetProcessName(p.name)
                     == process_name)]
    if len(processes) != 1:
      raise Exception('Found %d running processes with names matching "%s"' %
                      (len(processes), process_name))
    pid = processes[0].pid

    profile_cmd = [self._device_simpleperf_path, 'record',
                   '-g', # Enable call graphs based on dwarf debug frame
                   '-f', str(frequency),
                   '-o', out_file]

    if thread_name:
      threads = [t for t in self._ThreadsForProcess(device, str(pid))
                 if (browser._browser_backend.GetThreadType(t[1]) ==
                     thread_name)]
      if len(threads) != 1:
        raise Exception('Found %d threads with names matching "%s"' %
                        (len(threads), thread_name))
      profile_cmd.extend(['-t', threads[0][0]])
    else:
      profile_cmd.extend(['-p', str(pid)])
    return device.adb.StartShell(profile_cmd)

  @contextlib.contextmanager
  def SamplePeriod(self, period, action_runner, **kwargs):
    profiling_process = None
    out_file = self.DEVICE_OUT_FILE_PATTERN % period
    process_name = kwargs.get('process_name', 'renderer')
    thread_name = kwargs.get('thread_name', 'main')
    frequency = kwargs.get('frequency', 1000)
    profiling_process = self._StartSimpleperf(
        action_runner.tab.browser,
        out_file,
        process_name,
        thread_name,
        frequency)
    yield
    device = action_runner.tab.browser._platform_backend.device
    pidof_lines = device.RunShellCommand(['pidof', 'simpleperf'])
    if not pidof_lines:
      raise Exception('Could not get pid of running simpleperf process.')
    device.RunShellCommand(['kill', '-s', 'SIGINT', pidof_lines[0].strip()])
    profiling_process.wait()
    self._device_results.append((period, out_file))

  def GetResults(self, page_name, file_safe_name, results):
    for period, device_file in self._device_results:
      prefix = '%s-%s-' % (file_safe_name, period)
      with results.CreateArtifact(
          page_name, 'simpleperf', prefix=prefix, suffix='.perf.data') as fh:
        local_file = fh.name
        fh.close()
        self._device.PullFile(device_file, local_file)
    self._device_results = []
