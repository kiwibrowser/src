#!/usr/bin/python
#
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Loads a web page with speculative prefetch, and collects loading metrics."""

import argparse
import logging
import os
import random
import sys
import time

_SRC_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir))

sys.path.append(os.path.join(
    _SRC_PATH, 'tools', 'android', 'customtabs_benchmark', 'scripts'))
import customtabs_benchmark
import chrome_setup
import device_setup

sys.path.append(os.path.join(_SRC_PATH, 'tools', 'android', 'loading'))
import controller
from options import OPTIONS

sys.path.append(os.path.join(_SRC_PATH, 'build', 'android'))
import devil_chromium

sys.path.append(os.path.join(_SRC_PATH, 'third_party', 'catapult', 'devil'))
from devil.android import flag_changer
from devil.android.sdk import intent

import prefetch_predictor_common


def _CreateArgumentParser():
  """Creates and returns the argument parser."""
  parser = argparse.ArgumentParser(
      ('Loads a URL with the resource_prefetch_predictor and prints loading '
       'metrics.'), parents=[OPTIONS.GetParentParser()])
  parser.add_argument('--device', help='Device ID')
  parser.add_argument('--database',
                      help=('File containing the predictor database, as '
                            'obtained from generate_database.py.'))
  parser.add_argument('--url', help='URL to load.')
  parser.add_argument('--prefetch_delays_ms',
                      help='List of prefetch delays in ms. -1 to disable '
                      'prefetch. Runs will randomly select one delay in the '
                      'list.')
  parser.add_argument('--output_filename',
                      help='CSV file to append the result to.')
  parser.add_argument('--network_condition',
                      help='Network condition for emulation.')
  parser.add_argument('--wpr_archive', help='WPR archive path.')
  parser.add_argument('--once', help='Only run once.', action='store_true')
  return parser


def _Setup(device, database_filename):
  """Sets up a device and returns an instance of RemoteChromeController."""
  chrome_controller = prefetch_predictor_common.Setup(device)
  chrome_package = OPTIONS.ChromePackage()
  device.ForceStop(chrome_package.package)
  chrome_controller.ResetBrowserState()
  device_database_filename = prefetch_predictor_common.DatabaseDevicePath()
  owner = group = None

  # Make sure that the speculative prefetch predictor is enabled to ensure
  # that the disk database is re-created.
  with flag_changer.CustomCommandLineFlags(
      device, chrome_package.cmdline_file, ['--disable-fre']):
    # Launch Chrome for the first time to recreate the local state.
    launch_intent = intent.Intent(
        action='android.intent.action.MAIN',
        package=chrome_package.package,
        activity=chrome_package.activity)
    device.StartActivity(launch_intent, blocking=True)
    time.sleep(5)
    device.ForceStop(chrome_package.package)
    assert device.FileExists(device_database_filename)
    stats = device.StatPath(device_database_filename)
    owner = stats['st_owner']
    group = stats['st_group']
  # Now push the database. Needs to be done after the first launch, otherwise
  # the profile directory is owned by root. Also change the owner of the
  # database, since adb push sets it to root.
  database_content = open(database_filename, 'r').read()
  device.WriteFile(device_database_filename, database_content, force_push=True)
  device.RunShellCommand(
      ['chown', '%s:%s' % (owner, group), device_database_filename],
      as_root=True, check_return=True)


def _RunOnce(device, database_filename, url, prefetch_delay_ms,
             output_filename, wpr_archive, network_condition):
  _Setup(device, database_filename)

  disable_prefetch = prefetch_delay_ms == -1
  # Startup tracing to ease debugging.
  chrome_args = (chrome_setup.CHROME_ARGS
                 + ['--trace-startup', '--trace-startup-duration=20'])
  # Speculative Prefetch is enabled through an experiment.
  chrome_args.extend([
      '--force-fieldtrials=trial/group',
      '--force-fieldtrial-params=trial.group:mode/external-prefetching',
      '--enable-features=SpeculativeResourcePrefetching<trial'])

  chrome_controller = controller.RemoteChromeController(device)
  device.ForceStop(OPTIONS.ChromePackage().package)
  chrome_controller.AddChromeArguments(chrome_args)

  with device_setup.RemoteWprHost(
      device, wpr_archive, record=False,
      network_condition_name=network_condition) as wpr:
    logging.info('WPR arguments: ' +  ' '.join(wpr.chrome_args))
    chrome_args += wpr.chrome_args
    prefetch_mode = 'disabled' if disable_prefetch else 'speculative_prefetch'
    result = customtabs_benchmark.RunOnce(
        device, url, warmup=True, speculation_mode=prefetch_mode,
        delay_to_may_launch_url=2000,
        delay_to_launch_url=prefetch_delay_ms, cold=False,
        chrome_args=chrome_args, reset_chrome_state=False)
  data_point = customtabs_benchmark.ParseResult(result)

  with open(output_filename, 'a') as f:
    f.write(','.join(str(x) for x in data_point) + '\n')


def main():
  logging.basicConfig(level=logging.INFO)
  devil_chromium.Initialize()

  parser = _CreateArgumentParser()
  args = parser.parse_args()
  OPTIONS.SetParsedArgs(args)

  if os.path.exists(args.output_filename):
    logging.error('Output file %s already exists.' % args.output_filename)
    sys.exit(1)

  device = prefetch_predictor_common.FindDevice(args.device)
  if device is None:
    logging.error('Could not find device: %s.', args.device)
    sys.exit(1)

  delays = [int(x) for x in args.prefetch_delays_ms.split(',')]

  with open(args.output_filename, 'w') as f:
    f.write(','.join(customtabs_benchmark.RESULT_FIELDS) + '\n')

  while True:
    delay = delays[random.randint(0, len(delays) - 1)]
    _RunOnce(device, args.database, args.url, delay, args.output_filename,
             args.wpr_archive, args.network_condition)
    if args.once:
      return


if __name__ == '__main__':
  main()
