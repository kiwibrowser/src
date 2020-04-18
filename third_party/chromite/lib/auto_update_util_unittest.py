# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for the auto_update_util module.
"""

from __future__ import print_function

import unittest

from chromite.lib import auto_update_util

class VersionMatchUnittest(unittest.TestCase):
  """Test version_match function."""

  def testVersionMatch(self):
    """Test version_match function."""
    canary_build = 'lumpy-release/R43-6803.0.0'
    canary_release = '6803.0.0'
    cq_build = 'lumpy-release/R43-6803.0.0-rc1'
    cq_release = '6803.0.0-rc1'
    trybot_paladin_build = 'trybot-lumpy-paladin/R43-6803.0.0-b123'
    trybot_paladin_release = '6803.0.2015_03_12_2103'
    trybot_pre_cq_build = 'trybot-wifi-pre-cq/R43-7000.0.0-b36'
    trybot_pre_cq_release = '7000.0.2016_03_12_2103'
    trybot_toolchain_build = 'trybot-sentry-llvm-toolchain/R56-8885.0.0-b943'
    trybot_toolchain_release = '8885.0.2016_10_10_1432'


    builds = [canary_build, cq_build, trybot_paladin_build,
              trybot_pre_cq_build, trybot_toolchain_build]
    releases = [canary_release, cq_release, trybot_paladin_release,
                trybot_pre_cq_release, trybot_toolchain_release]
    for i in range(len(builds)):
      for j in range(len(releases)):
        self.assertEqual(
            auto_update_util.VersionMatch(builds[i], releases[j]), i == j,
            'Build version %s should%s match release version %s.' %
            (builds[i], '' if i == j else ' not', releases[j]))
