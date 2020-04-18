#!/usr/bin/python
#
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Loads a set of web pages several times on a device, and extracts the
predictor database.
Also generates a WPR archive for another page.
"""

import argparse
import logging
import os
import sys
import time

_SRC_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir))
sys.path.append(os.path.join(_SRC_PATH, 'third_party', 'catapult', 'devil'))

from devil.android import device_utils

sys.path.append(os.path.join(_SRC_PATH, 'build', 'android'))
import devil_chromium

sys.path.append(os.path.join(_SRC_PATH, 'tools', 'android', 'loading'))
import device_setup
from options import OPTIONS
import page_track

import prefetch_predictor_common


_PAGE_LOAD_TIMEOUT = 40
_LEARNING_FLAGS = [
    '--force-fieldtrials=trial/group',
    '--force-fieldtrial-params=trial.group:mode/learning',
    '--enable-features=SpeculativeResourcePrefetching<trial']


def _CreateArgumentParser():
  """Creates and returns the argument parser."""
  parser = argparse.ArgumentParser(
      ('Loads a set of web pages several times on a device, and extracts the '
       'predictor database.'), parents=[OPTIONS.GetParentParser()])
  parser.add_argument('--device', help='Device ID')
  parser.add_argument('--urls_filename', help='File containing a list of URLs '
                      '(one per line). URLs can be repeated.')
  parser.add_argument('--output_filename',
                      help='File to store the database in.')
  parser.add_argument('--test_url', help='URL to record an archive of.')
  parser.add_argument('--wpr_archive', help='WPR archive path.')
  parser.add_argument('--url_repeat',
                      help=('Number of times each URL in the input '
                            'file is loaded.'), default=3)
  return parser


def _GenerateDatabase(chrome_controller, urls_filename, output_filename,
                      repeats):
  urls = []
  with open(urls_filename) as f:
    urls = [line.strip() for line in f.readlines()]

  with chrome_controller.Open() as connection:
    for repeat in range(repeats):
      logging.info('Repeat #%d', repeat)
      for url in urls:
        logging.info('\tLoading %s', url)
        page_track.PageTrack(connection)  # Registers the listeners.
        connection.MonitorUrl(url, timeout_seconds=_PAGE_LOAD_TIMEOUT,
                              stop_delay_multiplier=1.5)
        time.sleep(2)  # Reduces flakiness.

  device = chrome_controller.GetDevice()
  device.ForceStop(OPTIONS.ChromePackage().package)
  device.PullFile(prefetch_predictor_common.DatabaseDevicePath(),
                  output_filename)


def _GenerateWprArchive(device, url, archive_path):
  with device_setup.RemoteWprHost(device, archive_path, record=True) as wpr:
    chrome_controller = prefetch_predictor_common.Setup(
        device, wpr.chrome_args)
    with chrome_controller.Open() as connection:
      page_track.PageTrack(connection)  # Registers the listeners.
      connection.MonitorUrl(url, timeout_seconds=_PAGE_LOAD_TIMEOUT,
                            stop_delay_multiplier=1.5)


def main():
  devil_chromium.Initialize()
  logging.basicConfig(level=logging.INFO)

  parser = _CreateArgumentParser()
  args = parser.parse_args()
  OPTIONS.SetParsedArgs(args)

  device = prefetch_predictor_common.FindDevice(args.device)
  if device is None:
    logging.error('Could not find device: %s.', args.device)
    sys.exit(1)

  chrome_controller = prefetch_predictor_common.Setup(device, _LEARNING_FLAGS)
  _GenerateDatabase(chrome_controller, args.urls_filename,
                    args.output_filename, int(args.url_repeat))
  _GenerateWprArchive(device, args.test_url, args.wpr_archive)


if __name__ == '__main__':
  main()
