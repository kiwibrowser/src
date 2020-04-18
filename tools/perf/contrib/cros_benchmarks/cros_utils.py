# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import subprocess

import py_utils

from telemetry.core import exceptions
from telemetry.value import histogram_util


def _RunRemoteCommand(dut_ip, cmd):
  return os.system('ssh root@%s %s' % (dut_ip, cmd))


def _GetTabSwitchHistogram(browser):
  """Gets MPArch.RWH_TabSwitchPaintDuration histogram.

  Catches exceptions to handle devtools context lost cases.

  Any tab context can be used to get the TabSwitchPaintDuration histogram.
  browser.tabs[-1] is the last valid context. Ex: If the browser opens
  A, B, ..., I, J tabs, E, F, G tabs have valid contexts (not discarded),
  then browser.tab[0] is tab E, browser.tab[-1] is tab G.

  In the tab switching benchmark, the tabs are opened and switched to in
  1, 2, ..., n order. The tabs are discarded in roughly 1, 2, ..., n order
  (LRU order). The chance of discarding the last valid context is lower
  than discarding the first valid context.

  Args:
    browser: Gets histogram from this browser.

  Returns:
    A json serialization of a histogram or None if get histogram failed.
  """
  histogram_name = 'MPArch.RWH_TabSwitchPaintDuration'
  histogram_type = histogram_util.BROWSER_HISTOGRAM
  try:
    return histogram_util.GetHistogram(
        histogram_type, histogram_name, browser.tabs[-1])
  except (exceptions.DevtoolsTargetCrashException, KeyError):
    logging.warning('GetHistogram: Devtools context lost.')
  except exceptions.TimeoutException:
    logging.warning('GetHistogram: Timed out getting histogram.')
  return None


def GetTabSwitchHistogramRetry(browser):
  """Retries getting histogram as it may fail when a context was discarded.

  Args:
    browser: Gets histogram from this browser.

  Returns:
    A json serialization of a histogram.

  Raises:
    py_utils.TimeoutException: There is no valid histogram in 10 seconds.
  """
  return py_utils.WaitFor(lambda: _GetTabSwitchHistogram(browser), 10)


def WaitTabSwitching(browser, prev_histogram):
  """Waits for tab switching completion.

  It's done by checking browser histogram to see if
  RWH_TabSwitchPaintDuration count increases.

  Args:
    browser: Gets histogram from this browser.
    prev_histogram: Checks histogram change against this histogram.
  """
  def _IsDone():
    cur_histogram = _GetTabSwitchHistogram(browser)
    if not cur_histogram:
      return False

    diff_histogram = histogram_util.SubtractHistogram(
        cur_histogram, prev_histogram)
    diff_histogram_count = json.loads(diff_histogram).get('count', 0)
    return diff_histogram_count > 0

  try:
    py_utils.WaitFor(_IsDone, 10)
  except py_utils.TimeoutException:
    logging.warning('Timed out waiting for histogram count increasing.')


class KeyboardEmulator(object):
  """Sets up a remote emulated keyboard and sends key events to switch tab.

  Example usage:
    with KeyboardEmulator(DUT_IP) as kbd:
      for i in range(5):
        kbd.SwitchTab()
  """
  REMOTE_LOG_KEY_FILENAME = '/usr/local/tmp/log_key_tab_switch'

  def __init__(self, dut_ip):
    """Inits KeyboardEmulator.

    Args:
      dut_ip: DUT IP or hostname.
    """
    self._dut_ip = dut_ip
    self._root_dut_ip = 'root@' + dut_ip
    self._key_device_name = None

  def _StartRemoteKeyboardEmulator(self):
    """Starts keyboard emulator on the DUT.

    Returns:
      The device name of the emulated keyboard.

    Raises:
      RuntimeError: Keyboard emulation failed.
    """
    kbd_prop_filename = '/usr/local/autotest/cros/input_playback/keyboard.prop'

    ret = _RunRemoteCommand(self._dut_ip, 'test -e %s' % kbd_prop_filename)
    if ret != 0:
      raise RuntimeError('Keyboard property file does not exist.')

    cmd = ['ssh', self._root_dut_ip, 'evemu-device', kbd_prop_filename]
    ssh_process = subprocess.Popen(cmd, stdout=subprocess.PIPE)

    # The evemu-device output format:
    # Emulated Keyboard: /dev/input/event10
    output = ssh_process.stdout.readline()
    # The remote process would live when the ssh process was terminated.
    ssh_process.kill()
    if not output.startswith('Emulated Keyboard:'):
      raise RuntimeError('Keyboard emulation failed.')
    key_device_name = output.split()[2]
    return key_device_name

  def _SetupKeyDispatch(self):
    """Uploads the script to send key to switch tabs."""
    cur_dir = os.path.dirname(os.path.abspath(__file__))
    log_key_filename = os.path.join(cur_dir, 'data', 'log_key_tab_switch')
    os.system('scp -q %s %s:%s' %
              (log_key_filename, self._root_dut_ip,
               KeyboardEmulator.REMOTE_LOG_KEY_FILENAME))

  def __enter__(self):
    self._key_device_name = self._StartRemoteKeyboardEmulator()
    self._SetupKeyDispatch()
    return self

  def SwitchTab(self):
    """Sending Ctrl-tab key to trigger tab switching."""
    cmd = ('"evemu-play --insert-slot0 %s < %s"' %
           (self._key_device_name,
            KeyboardEmulator.REMOTE_LOG_KEY_FILENAME))
    _RunRemoteCommand(self._dut_ip, cmd)

  def __exit__(self, exc_type, exc_value, traceback):
    # Kills the remote emulator process explicitly.
    _RunRemoteCommand(self._dut_ip, 'pkill evemu-device')


def NoScreenOff(dut_ip):
  """Sets screen always on for 1 hour.

  Args:
    dut_ip: DUT IP or hostname.
  """
  _RunRemoteCommand(dut_ip, 'set_power_policy --ac_screen_off_delay=3600')
  _RunRemoteCommand(dut_ip, 'set_power_policy --ac_screen_dim_delay=3600')
