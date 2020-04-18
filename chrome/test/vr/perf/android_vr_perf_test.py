# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import vr_perf_test

import logging
import os
import time

DEFAULT_SCREEN_WIDTH = 720
DEFAULT_SCREEN_HEIGHT = 1280
NUM_VR_ENTRY_ATTEMPTS = 5

class AndroidVrPerfTest(vr_perf_test.VrPerfTest):
  """Base class for VrPerfTests running on Android.

  Handles the setup and teardown portions of the test. The _Run function
  that actually runs the test is to be implemented in a subclass or in
  a separate class that a subclass also inherits from.
  """
  def __init__(self, args):
    super(AndroidVrPerfTest, self).__init__(args)
    # Swarming stuff seems to routinely kill off adbd once a minute or so,
    # which often causes adb's startup message to appear in the output. We need
    # to remove this before getting the device name.
    # TODO(bsheedy): Look into preventing adbd from being killed altogether
    # instead of working around it.
    self._device_name = self._Adb(['shell', 'getprop',
                                   'ro.product.name']).strip().split('\n')[-1]

  def _Adb(self, cmd):
    """Runs the given command via adb.

    Returns:
      A string containing the stdout and stderr of the adb command.
    """
    # TODO(bsheedy): Maybe migrate to use Devil (overkill?).
    return self._RunCommand([self._args.adb_path] + cmd)

  def _OneTimeSetup(self):
    self._Adb(['root'])

    # Install the latest VrCore and Chrome APKs.
    self._Adb(['install', '-r', '-d',
               '../../third_party/gvr-android-sdk/test-apks/vr_services'
               '/vr_services_current.apk'])
    # TODO(bsheedy): Make APK path configurable so usable with other channels.
    self._Adb(['install', '-r', 'apks/ChromePublic.apk'])

    # Force WebVR support, remove open tabs, and don't have first run
    # experience.
    flags = ['--enable-webvr', '--no-restore-state', '--disable-fre']
    if self._args.additional_flags:
      flags.extend(self._args.additional_flags.split(' '))
    self._SetChromeCommandLineFlags(flags)
    # Wake up the device and sleep, otherwise WebGL can crash on startup.
    self._Adb(['shell', 'input', 'keyevent', 'KEYCODE_WAKEUP'])
    time.sleep(1)

  def _Setup(self, url):
    # Start Chrome
    self._Adb(['shell', 'am', 'start',
               '-a', 'android.intent.action.MAIN',
               '-n', 'org.chromium.chrome/com.google.android.apps.chrome.Main',
               url])
    # TODO(bsheedy): Remove this sleep - poll for magic window GVR
    # initialization to know when page fully loaded and ready?
    time.sleep(10)

    # Tap the center of the screen to start presenting.
    # It's technically possible that the screen tap won't enter VR on the first
    # time, so try several times by checking for the logcat output from
    # entering VR.
    (width, height) = self._GetScreenResolution()
    entered_vr = False
    for _ in xrange(NUM_VR_ENTRY_ATTEMPTS):
      self._Adb(['logcat', '-c'])
      self._Adb(['shell', 'input', 'touchscreen', 'tap', str(width/2),
                 str(height/2)])
      time.sleep(5)
      output = self._Adb(['logcat', '-d'])
      if 'Initialized GVR version' in output:
        entered_vr = True
        break
      logging.warning('Failed to enter VR, retrying')
    if not entered_vr:
      raise RuntimeError('Failed to enter VR after %d attempts'
                         % NUM_VR_ENTRY_ATTEMPTS)

  def _Teardown(self):
    # Exit VR and close Chrome.
    self._Adb(['shell', 'input', 'keyevent', 'KEYCODE_BACK'])
    self._Adb(['shell', 'am', 'force-stop', 'org.chromium.chrome'])

  def _OneTimeTeardown(self):
    # Turn off the screen.
    self._Adb(['shell', 'input', 'keyevent', 'KEYCODE_POWER'])

  def _SetChromeCommandLineFlags(self, flags):
    """Sets the Chrome command line flags to the given list."""
    self._Adb(['shell', "echo 'chrome " + ' '.join(flags) + "' > "
               + '/data/local/tmp/chrome-command-line'])

  def _GetScreenResolution(self):
    """Retrieves the device's screen resolution, or a default if not found.

    Returns:
      A tuple (width, height).
    """
    output = self._Adb(['shell', 'dumpsys', 'display'])
    width = None
    height = None
    for line in output.split('\n'):
      if 'mDisplayWidth' in line:
        width = int(line.split('=')[1])
      elif 'mDisplayHeight' in line:
        height = int(line.split('=')[1])
      if width and height:
        break
    if not width:
      logging.warning('Could not get actual screen width, defaulting to %d',
                      DEFAULT_SCREEN_WIDTH)
      width = DEFAULT_SCREEN_WIDTH
    if not height:
      logging.warning('Could not get actual screen height, defaulting to %d',
                      DEFAULT_SCREEN_HEIGHT)
      height = DEFAULT_SCREEN_HEIGHT
    return (width, height)
