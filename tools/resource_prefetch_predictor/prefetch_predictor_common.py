# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Common utils for the speculative resource prefetch evaluation."""

import os
import sys

_SRC_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir))

sys.path.append(os.path.join(_SRC_PATH, 'third_party', 'catapult', 'devil'))
from devil.android import device_utils

sys.path.append(os.path.join(_SRC_PATH, 'tools', 'android', 'loading'))
import controller
import device_setup
from options import OPTIONS


def FindDevice(device_id):
  """Returns a device matching |device_id| or the first one if None, or None."""
  devices = device_utils.DeviceUtils.HealthyDevices()
  if device_id is None:
    return devices[0]
  return device_setup.GetDeviceFromSerial(device_id)


def Setup(device, additional_flags=None):
  """Sets up a |device| and returns an instance of RemoteChromeController.

  Args:
    device: (Device) As returned by FindDevice().
    additional_flags: ([str] or None) Chrome flags to add.
  """
  if not device.HasRoot():
    device.EnableRoot()
  chrome_controller = controller.RemoteChromeController(device)
  device.ForceStop(OPTIONS.ChromePackage().package)
  if additional_flags is not None:
    chrome_controller.AddChromeArguments(additional_flags)
  chrome_controller.ResetBrowserState()
  return chrome_controller


def DatabaseDevicePath():
  return ('/data/user/0/%s/app_chrome/Default/Network Action Predictor' %
          OPTIONS.ChromePackage().package)
