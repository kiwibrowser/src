#!/usr/bin/python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper script to launch Chrome on device and WebPageReplay on host."""

import logging
import optparse
import os
import sys
import time

_SRC_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir, os.pardir))

sys.path.append(os.path.join(_SRC_PATH, 'third_party', 'catapult', 'devil'))
from devil.android import device_utils
from devil.android import flag_changer
from devil.android.constants import chrome
from devil.android.perf import cache_control
from devil.android.sdk import intent

sys.path.append(os.path.join(_SRC_PATH, 'build', 'android'))
import devil_chromium

import chrome_setup
import device_setup


def RunChrome(device, cold, chrome_args, package_info):
  """Runs Chrome on the device.

  Args:
    device: (DeviceUtils) device to run the tests on.
    cold: (bool) Whether caches should be dropped.
    chrome_args: ([str]) List of arguments to pass to Chrome.
    package_info: (PackageInfo) Chrome package info.
  """
  if not device.HasRoot():
    device.EnableRoot()

  cmdline_file = package_info.cmdline_file
  package = package_info.package
  with flag_changer.CustomCommandLineFlags(device, cmdline_file, chrome_args):
    device.ForceStop(package)

    if cold:
      chrome_setup.ResetChromeLocalState(device, package)
      cache_control.CacheControl(device).DropRamCaches()

    start_intent = intent.Intent(package=package, data='about:blank',
                                 activity=package_info.activity)
    try:
      device.StartActivity(start_intent, blocking=True)
      print (
          '\n\n'
          '   +---------------------------------------------+\n'
          '   | Chrome launched, press Ctrl-C to interrupt. |\n'
          '   +---------------------------------------------+')
      while True:
        time.sleep(1)
    except KeyboardInterrupt:
      pass
    finally:
      device.ForceStop(package)


def _CreateOptionParser():
  description = 'Launches Chrome on a device, connected to a WebPageReplay ' \
                'instance running on the host. The WPR archive must be ' \
                'passed as parameter.'
  parser = optparse.OptionParser(description=description,
                                 usage='Usage: %prog [options] wpr_archive')

  # Device-related options.
  d = optparse.OptionGroup(parser, 'Device options')
  d.add_option('--device', help='Device ID')
  d.add_option('--cold', help='Purge all caches before running Chrome.',
               default=False, action='store_true')
  d.add_option('--chrome_package_name',
               help='Chrome package name (e.g. "chrome" or "chromium") '
               '[default: %default].', default='chrome')
  parser.add_option_group(d)

  # WebPageReplay-related options.
  w = optparse.OptionGroup(parser, 'WebPageReplay options')
  w.add_option('--record',
               help='Enable this to record a new WPR archive.',
               action='store_true', default=False)
  w.add_option('--wpr_log', help='WPR log path.')
  w.add_option('--network_condition', help='Network condition for emulation.')
  parser.add_option_group(w)

  return parser


def main():
  parser = _CreateOptionParser()
  options, args = parser.parse_args()
  if len(args) != 1:
    parser.error("Incorrect number of arguments.")
  devil_chromium.Initialize()
  devices = device_utils.DeviceUtils.HealthyDevices()
  device = devices[0]
  if len(devices) != 1 and options.device is None:
    logging.error('Several devices attached, must specify one with --device.')
    sys.exit(0)
  if options.device is not None:
    matching_devices = [d for d in devices if str(d) == options.device]
    if not matching_devices:
      logging.error('Device not found.')
      sys.exit(0)
    device = matching_devices[0]

  with device_setup.RemoteWprHost(device, args[0], options.record,
                                  options.network_condition,
                                  out_log_path=options.wpr_log) as wpr_attr:
    RunChrome(device, options.cold,
              chrome_setup.CHROME_ARGS + wpr_attr.chrome_args,
              chrome.PACKAGE_INFO[options.chrome_package_name])


if __name__ == '__main__':
  main()
