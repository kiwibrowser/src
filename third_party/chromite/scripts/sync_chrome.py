# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Sync the Chrome source code used by Chrome OS to the specified directory."""

from __future__ import print_function

import os

from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gclient
from chromite.lib import osutils


def GetParser():
  """Creates the argparse parser."""
  parser = commandline.ArgumentParser(description=__doc__)

  version = parser.add_mutually_exclusive_group()
  version.add_argument('--tag', help='Sync to specified Chrome release',
                       dest='version')
  version.add_argument('--revision', help='Sync to specified git revision',
                       dest='version')

  parser.add_argument('--internal', help='Sync internal version of Chrome',
                      action='store_true', default=False)
  parser.add_argument('--reset', help='Revert local changes',
                      action='store_true', default=False)
  parser.add_argument('--gclient', help=commandline.argparse.SUPPRESS,
                      default=None)
  parser.add_argument('--gclient_template', help='Template gclient input file')
  parser.add_argument('--skip_cache', help='Skip using git cache',
                      dest='use_cache', action='store_false')
  parser.add_argument('--ignore_locks', help='Ignore git cache locks.',
                      action='store_true', default=False)
  parser.add_argument('chrome_root', help='Directory to sync chrome in')

  return parser


def SyncChrome(gclient_path, options):
  """Sync new Chrome."""
  gclient.WriteConfigFile(gclient_path, options.chrome_root,
                          options.internal, options.version,
                          options.gclient_template, options.use_cache)
  gclient.Sync(gclient_path, options.chrome_root, reset=options.reset,
               ignore_locks=options.ignore_locks)


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)

  if options.gclient == '':
    parser.error('--gclient can not be an empty string!')
  gclient_path = options.gclient or osutils.Which('gclient')
  if not gclient_path:
    gclient_path = os.path.join(constants.DEPOT_TOOLS_DIR, 'gclient')

  try:
    if options.reset:
      # Revert any lingering local changes.
      gclient.Revert(gclient_path, options.chrome_root)

    SyncChrome(gclient_path, options)
  except cros_build_lib.RunCommandError:
    # If we have an error resetting, or syncing, we clobber, and fresh sync.
    logging.warning('Chrome checkout appears corrupt. Clobbering.')
    osutils.RmDir(options.chrome_root, ignore_missing=True, sudo=True)
    osutils.SafeMakedirsNonRoot(options.chrome_root)
    SyncChrome(gclient_path, options)

  return 0
