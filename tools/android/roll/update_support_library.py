#!/usr/bin/env python
#
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''
Updates the Android support repository (m2repository).
'''

import argparse
import os
import logging
import sys

_SRC_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..'))
sys.path.append(os.path.join(_SRC_DIR, 'build', 'android'))

from pylib.constants import host_paths
from pylib.utils import logging_utils
from pylib.utils import maven_downloader

SUPPORT_LIB_REPO = os.path.join(host_paths.DIR_SOURCE_ROOT, 'third_party',
                                'android_tools', 'sdk', 'extras', 'android',
                                'm2repository')


def main():
  parser = argparse.ArgumentParser(description='Updates the Android support '
                                   'repository in third_party/android_tools')
  parser.add_argument('--target-repo',
                      help='Maven repo where the library will be installed.',
                      default=SUPPORT_LIB_REPO)
  parser.add_argument('--debug',
                      help='Debug mode, synchronous and with extra logging.',
                      action='store_true')
  args = parser.parse_args()

  if (args.debug):
    logging.basicConfig(level=logging.DEBUG)
  else:
    logging.basicConfig(level=logging.INFO)
  logging_utils.ColorStreamHandler.MakeDefault()

  maven_downloader.MavenDownloader(args.debug).Install(args.target_repo, [
      'android.arch.core:common:1.0.0:jar',
      'android.arch.lifecycle:common:1.0.0:jar',
      'android.arch.lifecycle:runtime:1.0.0:aar',
      'com.android.support:animated-vector-drawable:27.0.0:aar',
      'com.android.support:appcompat-v7:27.0.0:aar',
      'com.android.support:cardview-v7:27.0.0:aar',
      'com.android.support:design:27.0.0:aar',
      'com.android.support:gridlayout-v7:27.0.0:aar',
      'com.android.support:leanback-v17:27.0.0:aar',
      'com.android.support:mediarouter-v7:27.0.0:aar',
      'com.android.support:palette-v7:27.0.0:aar',
      'com.android.support:preference-leanback-v17:27.0.0:aar',
      'com.android.support:preference-v14:27.0.0:aar',
      'com.android.support:preference-v7:27.0.0:aar',
      'com.android.support:recyclerview-v7:27.0.0:aar',
      'com.android.support:support-annotations:27.0.0:jar',
      'com.android.support:support-compat:27.0.0:aar',
      'com.android.support:support-core-ui:27.0.0:aar',
      'com.android.support:support-core-utils:27.0.0:aar',
      'com.android.support:support-fragment:27.0.0:aar',
      'com.android.support:support-media-compat:27.0.0:aar',
      'com.android.support:support-v13:27.0.0:aar',
      'com.android.support:support-vector-drawable:27.0.0:aar',
      'com.android.support:transition:27.0.0:aar',
  ], include_poms=True)


if __name__ == '__main__':
  sys.exit(main())
