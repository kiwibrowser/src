# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Controller objects that control the context in which chrome runs.

This is responsible for the setup necessary for launching chrome, and for
creating a DevToolsConnection. There are remote device and local
desktop-specific versions.
"""

import contextlib
import copy
import datetime
import errno
import logging
import os
import platform
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import traceback

import psutil

import chrome_cache
import chrome_setup
import common_util
import device_setup
import devtools_monitor
import emulation
from options import OPTIONS

_SRC_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..'))
_CATAPULT_DIR = os.path.join(_SRC_DIR, 'third_party', 'catapult')

sys.path.append(os.path.join(_CATAPULT_DIR, 'devil'))
from devil.android import device_errors
from devil.android import flag_changer
from devil.android.sdk import intent

sys.path.append(
    os.path.join(_CATAPULT_DIR, 'telemetry', 'third_party', 'websocket-client'))
import websocket


class ChromeControllerMetadataGatherer(object):
  """Gather metadata for the ChromeControllerBase."""

  def __init__(self):
    self._chromium_commit = None

  def GetMetadata(self):
    """Gets metadata to update in the ChromeControllerBase"""
    if self._chromium_commit is None:
      def _GitCommand(subcmd):
        return subprocess.check_output(['git', '-C', _SRC_DIR] + subcmd).strip()
      try:
        self._chromium_commit = _GitCommand(['merge-base', 'master', 'HEAD'])
        if self._chromium_commit != _GitCommand(['rev-parse', 'HEAD']):
          self._chromium_commit = 'unknown'
      except subprocess.CalledProcessError:
        self._chromium_commit = 'git_error'
    return {
      'chromium_commit': self._chromium_commit,
      'date': datetime.datetime.utcnow().isoformat(),
      'seconds_since_epoch': time.time()
    }


class ChromeControllerInternalError(Exception):
  pass


def _AllocateTcpListeningPort():
  """Allocates a TCP listening port.

  Note: The use of this function is inherently OS level racy because the
    port returned by this function might be re-used by another running process.
  """
  temp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  try:
    temp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    temp_socket.bind(('', 0))
    return temp_socket.getsockname()[1]
  finally:
    temp_socket.close()


class ChromeControllerError(Exception):
  """Chrome error with detailed log.

  Note:
    Some of these errors might be known intermittent errors that can usually be
    retried by the caller after re-doing any specific setup again.
  """
  _INTERMITTENT_WHITE_LIST = {websocket.WebSocketTimeoutException,
                              devtools_monitor.DevToolsConnectionTargetCrashed}
  _PASSTHROUGH_WHITE_LIST = (MemoryError, SyntaxError)

  def __init__(self, log):
    """Constructor

    Args:
      log: String containing the log of the running Chrome instance that was
          running. It will be interleaved with any other running Android
          package.
    """
    self.error_type, self.error_value, self.error_traceback = sys.exc_info()
    super(ChromeControllerError, self).__init__(repr(self.error_value))
    self.parent_stack = traceback.extract_stack()
    self.log = log

  def Dump(self, output):
    """Dumps the entire error's infos into file-like object."""
    output.write('-' * 60 + ' {}:\n'.format(self.__class__.__name__))
    output.write(repr(self) + '\n')
    output.write('{} is {}known as intermittent.\n'.format(
        self.error_type.__name__, '' if self.IsIntermittent() else 'NOT '))
    output.write(
        '-' * 60 + ' {}\'s full traceback:\n'.format(self.error_type.__name__))
    output.write(''.join(traceback.format_list(self.parent_stack)))
    traceback.print_tb(self.error_traceback, file=output)
    output.write('-' * 60 + ' Begin log\n')
    output.write(self.log)
    output.write('-' * 60 + ' End log\n')

  def IsIntermittent(self):
    """Returns whether the error is an known intermittent error."""
    return self.error_type in self._INTERMITTENT_WHITE_LIST

  def RaiseOriginal(self):
    """Raises the original exception that has caused <self>."""
    raise self.error_type, self.error_value, self.error_traceback


class ChromeControllerBase(object):
  """Base class for all controllers.

  Defines common operations but should not be created directly.
  """
  METADATA_GATHERER = ChromeControllerMetadataGatherer()
  DEVTOOLS_CONNECTION_ATTEMPTS = 10
  DEVTOOLS_CONNECTION_ATTEMPT_INTERVAL_SECONDS = 1

  def __init__(self):
    self._chrome_args = chrome_setup.CHROME_ARGS + [
        # Tests & dev-tools related stuff.
        '--enable-test-events',
        '--remote-debugging-port=%d' % OPTIONS.devtools_port,

        # Detailed log.
        '--enable-logging=stderr',
        '--v=1',
    ]
    self._wpr_attributes = None
    self._metadata = {}
    self._emulated_device = None
    self._network_name = None
    self._slow_death = False

  def AddChromeArguments(self, args):
    """Add command-line arguments to the chrome execution."""
    self._chrome_args.extend(args)

  @contextlib.contextmanager
  def Open(self):
    """Context that returns a connection/chrome instance.

    Returns:
      DevToolsConnection instance for which monitoring has been set up but not
      started.
    """
    raise NotImplementedError

  def ChromeMetadata(self):
    """Return metadata such as emulation information.

    Returns:
      Metadata as JSON dictionary.
    """
    return self._metadata

  def GetDevice(self):
    """Returns an android device, or None if chrome is local."""
    return None

  def SetDeviceEmulation(self, device_name):
    """Set device emulation.

    Args:
      device_name: (str) Key from --devices_file.
    """
    devices = emulation.LoadEmulatedDevices(file(OPTIONS.devices_file))
    self._emulated_device = devices[device_name]

  def SetNetworkEmulation(self, network_name):
    """Set network emulation.

    Args:
      network_name: (str) Key from emulation.NETWORK_CONDITIONS or None to
        disable network emulation.
    """
    assert network_name in emulation.NETWORK_CONDITIONS or network_name is None
    self._network_name = network_name

  def ResetBrowserState(self):
    """Resets the chrome's browser state."""
    raise NotImplementedError

  def PushBrowserCache(self, cache_path):
    """Pushes the HTTP chrome cache to the profile directory.

    Caution:
      The chrome cache backend type differ according to the platform. On
      desktop, the cache backend type is `blockfile` versus `simple` on Android.
      This method assumes that your are pushing a cache with the correct backend
      type, and will NOT verify for you.

    Args:
      cache_path: The directory's path containing the cache locally.
    """
    raise NotImplementedError

  def PullBrowserCache(self):
    """Pulls the HTTP chrome cache from the profile directory.

    Returns:
      Temporary directory containing all the browser cache. Caller will need to
      remove this directory manually.
    """
    raise NotImplementedError

  def SetSlowDeath(self, slow_death=True):
    """Set to pause before final kill of chrome.

    Gives time for caches to write.

    Args:
      slow_death: (bool) True if you want that which comes to all who live, to
        be slow.
    """
    self._slow_death = slow_death

  @contextlib.contextmanager
  def OpenWprHost(self, wpr_archive_path, record=False,
                  network_condition_name=None,
                  disable_script_injection=False,
                  out_log_path=None):
    """Opens a Web Page Replay host context.

    Args:
      wpr_archive_path: host sided WPR archive's path.
      record: Enables or disables WPR archive recording.
      network_condition_name: Network condition name available in
          emulation.NETWORK_CONDITIONS.
      disable_script_injection: Disable JavaScript file injections that is
        fighting against resources name entropy.
      out_log_path: Path of the WPR host's log.
    """
    raise NotImplementedError

  def _StartConnection(self, connection):
    """This should be called after opening an appropriate connection."""
    if self._emulated_device:
      self._metadata.update(emulation.SetUpDeviceEmulationAndReturnMetadata(
          connection, self._emulated_device))
    if self._network_name:
      network_condition = emulation.NETWORK_CONDITIONS[self._network_name]
      logging.info('Set up network emulation %s (latency=%dms, down=%d, up=%d)'
          % (self._network_name, network_condition['latency'],
              network_condition['download'], network_condition['upload']))
      emulation.SetUpNetworkEmulation(connection, **network_condition)
      self._metadata['network_emulation'] = copy.copy(network_condition)
      self._metadata['network_emulation']['name'] = self._network_name
    else:
      self._metadata['network_emulation'] = \
          {k: 'disabled' for k in ['name', 'download', 'upload', 'latency']}
    self._metadata.update(self.METADATA_GATHERER.GetMetadata())
    logging.info('Devtools connection success')

  def _GetChromeArguments(self):
    """Get command-line arguments for the chrome execution."""
    chrome_args = self._chrome_args[:]
    if self._wpr_attributes:
      chrome_args.extend(self._wpr_attributes.chrome_args)
    return chrome_args


class RemoteChromeController(ChromeControllerBase):
  """A controller for an android device, aka remote chrome instance."""
  # An estimate of time to wait for the device to become idle after expensive
  # operations, such as opening the launcher activity.
  TIME_TO_IDLE_SECONDS = 2

  def __init__(self, device):
    """Initialize the controller.

    Caution: The browser state might need to be manually reseted.

    Args:
      device: (device_utils.DeviceUtils) an android device.
    """
    assert device is not None, 'Should you be using LocalController instead?'
    super(RemoteChromeController, self).__init__()
    self._device = device
    self._metadata['platform'] = {
        'os': 'A-' + device.build_id,
        'product_model': device.product_model
    }
    self._InitDevice()

  def GetDevice(self):
    """Overridden android device."""
    return self._device

  @contextlib.contextmanager
  def Open(self):
    """Overridden connection creation."""
    if self._wpr_attributes:
      assert self._wpr_attributes.chrome_env_override == {}, \
          'Remote controller doesn\'t support chrome environment variables.'
    package_info = OPTIONS.ChromePackage()
    self._device.ForceStop(package_info.package)
    with flag_changer.CustomCommandLineFlags(
        self._device, package_info.cmdline_file, self._GetChromeArguments()):
      self._DismissCrashDialogIfNeeded()
      start_intent = intent.Intent(
          package=package_info.package, activity=package_info.activity,
          data='about:blank')
      self._device.adb.Logcat(clear=True, dump=True)
      self._device.StartActivity(start_intent, blocking=True)
      try:
        for attempt_id in xrange(self.DEVTOOLS_CONNECTION_ATTEMPTS):
          logging.info('Devtools connection attempt %d' % attempt_id)
          # Adb forwarding does not provide a way to print the port number if
          # it is allocated atomically by the OS by passing port=0, but we need
          # dynamically allocated listening port here to handle parallel run on
          # different devices.
          host_side_port = _AllocateTcpListeningPort()
          logging.info('Allocated host sided listening port for devtools '
              'connection: %d', host_side_port)
          try:
            with device_setup.ForwardPort(
                self._device, 'tcp:%d' % host_side_port,
                'localabstract:chrome_devtools_remote'):
              try:
                connection = devtools_monitor.DevToolsConnection(
                    OPTIONS.devtools_hostname, host_side_port)
                self._StartConnection(connection)
              except socket.error as e:
                if e.errno != errno.ECONNRESET:
                  raise
                time.sleep(self.DEVTOOLS_CONNECTION_ATTEMPT_INTERVAL_SECONDS)
                continue
              yield connection
              if self._slow_death:
                self._device.adb.Shell('am start com.google.android.launcher')
                time.sleep(self.TIME_TO_IDLE_SECONDS)
              break
          except device_errors.AdbCommandFailedError as error:
            _KNOWN_ADB_FORWARDER_FAILURES = [
              'cannot bind to socket: Address already in use',
              'cannot rebind existing socket: Resource temporarily unavailable']
            for message in _KNOWN_ADB_FORWARDER_FAILURES:
              if message in error.message:
                break
            else:
              raise
            continue
        else:
          raise ChromeControllerInternalError(
              'Failed to connect to Chrome devtools after {} '
              'attempts.'.format(self.DEVTOOLS_CONNECTION_ATTEMPTS))
      except ChromeControllerError._PASSTHROUGH_WHITE_LIST:
        raise
      except Exception:
        logcat = ''.join([l + '\n' for l in self._device.adb.Logcat(dump=True)])
        raise ChromeControllerError(log=logcat)
      finally:
        self._device.ForceStop(package_info.package)
        self._DismissCrashDialogIfNeeded()

  def ResetBrowserState(self):
    """Override resetting Chrome local state."""
    logging.info('Resetting Chrome local state')
    chrome_setup.ResetChromeLocalState(self._device,
                                       OPTIONS.ChromePackage().package)


  def RebootDevice(self):
    """Reboot the remote device."""
    assert self._wpr_attributes is None, 'WPR should be closed before rebooting'
    logging.warning('Rebooting the device')
    device_setup.Reboot(self._device)
    self._InitDevice()

  def PushBrowserCache(self, cache_path):
    """Override for chrome cache pushing."""
    logging.info('Push cache from %s' % cache_path)
    chrome_cache.PushBrowserCache(self._device, cache_path)

  def PullBrowserCache(self):
    """Override for chrome cache pulling."""
    assert self._slow_death, 'Must do SetSlowDeath() before opening chrome.'
    logging.info('Pull cache from device')
    return chrome_cache.PullBrowserCache(self._device)

  @contextlib.contextmanager
  def OpenWprHost(self, wpr_archive_path, record=False,
                  network_condition_name=None,
                  disable_script_injection=False,
                  out_log_path=None):
    """Starts a WPR host, overrides Chrome flags until contextmanager exit."""
    assert not self._wpr_attributes, 'WPR is already running.'
    with device_setup.RemoteWprHost(self._device, wpr_archive_path,
        record=record,
        network_condition_name=network_condition_name,
        disable_script_injection=disable_script_injection,
        out_log_path=out_log_path) as wpr_attributes:
      self._wpr_attributes = wpr_attributes
      yield
    self._wpr_attributes = None

  def _DismissCrashDialogIfNeeded(self):
    for _ in xrange(10):
      if not self._device.DismissCrashDialogIfNeeded():
        break

  def _InitDevice(self):
    self._device.EnableRoot()


class LocalChromeController(ChromeControllerBase):
  """Controller for a local (desktop) chrome instance."""

  def __init__(self):
    """Initialize the controller.

    Caution: The browser state might need to be manually reseted.
    """
    super(LocalChromeController, self).__init__()
    if OPTIONS.no_sandbox:
      self.AddChromeArguments(['--no-sandbox'])
    self._profile_dir = OPTIONS.local_profile_dir
    self._using_temp_profile_dir = self._profile_dir is None
    if self._using_temp_profile_dir:
      self._profile_dir = tempfile.mkdtemp(suffix='.profile')
    self._chrome_env_override = {}
    self._metadata['platform'] = {
        'os': platform.system()[0] + '-' + platform.release(),
        'product_model': 'unknown'
    }

  def __del__(self):
    if self._using_temp_profile_dir:
      shutil.rmtree(self._profile_dir)

  @staticmethod
  def KillChromeProcesses():
    """Kills all the running instances of Chrome.

    Returns: (int) The number of processes that were killed.
    """
    killed_count = 0
    chrome_path = OPTIONS.LocalBinary('chrome')
    for process in psutil.process_iter():
      try:
        process_bin_path = None
        # In old versions of psutil, process.exe is a member, in newer ones it's
        # a method.
        if type(process.exe) == str:
          process_bin_path = process.exe
        else:
          process_bin_path = process.exe()
        if os.path.abspath(process_bin_path) == os.path.abspath(chrome_path):
          process.terminate()
          killed_count += 1
          try:
            process.wait(timeout=10)
          except psutil.TimeoutExpired:
            process.kill()
      except psutil.AccessDenied:
        pass
      except psutil.NoSuchProcess:
        pass
    return killed_count

  def SetChromeEnvOverride(self, env):
    """Set the environment for Chrome.

    Args:
      env: (dict) Environment.
    """
    self._chrome_env_override = env

  @contextlib.contextmanager
  def Open(self):
    """Overridden connection creation."""
    # Kill all existing Chrome instances.
    killed_count = LocalChromeController.KillChromeProcesses()
    if killed_count > 0:
      logging.warning('Killed existing Chrome instance.')

    chrome_cmd = [OPTIONS.LocalBinary('chrome')]
    chrome_cmd.extend(self._GetChromeArguments())
    # Force use of simple cache.
    chrome_cmd.append('--use-simple-cache-backend=on')
    chrome_cmd.append('--user-data-dir=%s' % self._profile_dir)
    # Navigates to about:blank for couples of reasons:
    #   - To find the correct target descriptor at devtool connection;
    #   - To avoid cache and WPR pollution by the NTP.
    chrome_cmd.append('about:blank')

    tmp_log = \
        tempfile.NamedTemporaryFile(prefix="chrome_controller_", suffix='.log')
    chrome_process = None
    try:
      chrome_env_override = self._chrome_env_override.copy()
      if self._wpr_attributes:
        chrome_env_override.update(self._wpr_attributes.chrome_env_override)

      chrome_env = os.environ.copy()
      chrome_env.update(chrome_env_override)

      # Launch Chrome.
      logging.info(common_util.GetCommandLineForLogging(chrome_cmd,
                                                        chrome_env_override))
      chrome_process = subprocess.Popen(chrome_cmd, stdout=tmp_log.file,
                                        stderr=tmp_log.file, env=chrome_env)
      # Attempt to connect to Chrome's devtools
      for attempt_id in xrange(self.DEVTOOLS_CONNECTION_ATTEMPTS):
        logging.info('Devtools connection attempt %d' % attempt_id)
        process_result = chrome_process.poll()
        if process_result is not None:
          raise ChromeControllerInternalError(
              'Unexpected Chrome exit: {}'.format(process_result))
        try:
          connection = devtools_monitor.DevToolsConnection(
              OPTIONS.devtools_hostname, OPTIONS.devtools_port)
          break
        except socket.error as e:
          if e.errno != errno.ECONNREFUSED:
            raise
          time.sleep(self.DEVTOOLS_CONNECTION_ATTEMPT_INTERVAL_SECONDS)
      else:
        raise ChromeControllerInternalError(
            'Failed to connect to Chrome devtools after {} '
            'attempts.'.format(self.DEVTOOLS_CONNECTION_ATTEMPTS))
      # Start and yield the devtool connection.
      self._StartConnection(connection)
      yield connection
      if self._slow_death:
        connection.Close()
        chrome_process.wait()
        chrome_process = None
    except ChromeControllerError._PASSTHROUGH_WHITE_LIST:
      raise
    except Exception:
      raise ChromeControllerError(log=open(tmp_log.name).read())
    finally:
      if OPTIONS.local_noisy:
        sys.stderr.write(open(tmp_log.name).read())
      del tmp_log
      if chrome_process:
        try:
          chrome_process.kill()
        except OSError:
          pass  # Chrome is already dead.

  def ResetBrowserState(self):
    """Override for chrome state reseting."""
    assert os.path.isdir(self._profile_dir)
    logging.info('Reset chrome\'s profile')
    # Don't do a rmtree(self._profile_dir) because it might be a temp directory.
    for filename in os.listdir(self._profile_dir):
      path = os.path.join(self._profile_dir, filename)
      if os.path.isdir(path):
        shutil.rmtree(path)
      else:
        os.remove(path)

  def PushBrowserCache(self, cache_path):
    """Override for chrome cache pushing."""
    self._EnsureProfileDirectory()
    profile_cache_path = self._GetCacheDirectoryPath()
    logging.info('Copy cache directory from %s to %s.' % (
        cache_path, profile_cache_path))
    chrome_cache.CopyCacheDirectory(cache_path, profile_cache_path)

  def PullBrowserCache(self):
    """Override for chrome cache pulling."""
    cache_path = tempfile.mkdtemp()
    profile_cache_path = self._GetCacheDirectoryPath()
    logging.info('Copy cache directory from %s to %s.' % (
        profile_cache_path, cache_path))
    chrome_cache.CopyCacheDirectory(profile_cache_path, cache_path)
    return cache_path

  @contextlib.contextmanager
  def OpenWprHost(self, wpr_archive_path, record=False,
                  network_condition_name=None,
                  disable_script_injection=False,
                  out_log_path=None):
    """Override for WPR context."""
    assert not self._wpr_attributes, 'WPR is already running.'
    with device_setup.LocalWprHost(wpr_archive_path,
        record=record,
        network_condition_name=network_condition_name,
        disable_script_injection=disable_script_injection,
        out_log_path=out_log_path) as wpr_attributes:
      self._wpr_attributes = wpr_attributes
      yield
    self._wpr_attributes = None

  def _EnsureProfileDirectory(self):
    if (not os.path.isdir(self._profile_dir) or
        os.listdir(self._profile_dir) == []):
      # Launch chrome so that it populates the profile directory.
      with self.Open():
        pass
    assert os.path.isdir(self._profile_dir)
    assert os.path.isdir(os.path.dirname(self._GetCacheDirectoryPath()))

  def _GetCacheDirectoryPath(self):
    return os.path.join(self._profile_dir, 'Default', 'Cache')
