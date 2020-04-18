# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for toolchain."""

from __future__ import print_function

import mock
import os

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import toolchain


BASE_TOOLCHAIN_CONF = """# The root of all evil is money, err, this config.
base-target-name # This will become the base target.

# This toolchain is bonus!
bonus-toolchain {"a setting": "bonus value"}  # Bonus!

"""

ADDITIONAL_TOOLCHAIN_CONF = """# A helpful toolchain related comment.
extra-toolchain  # Unlikely to win any performance tests.

bonus-toolchain {"stable": true}
"""

EXPECTED_TOOLCHAINS = {
    'bonus-toolchain': {
        'sdk': True,
        'crossdev': '',
        'default': False,
        'a setting': 'bonus value',
        'stable': True,
    },
    'extra-toolchain': {'sdk': True, 'crossdev': '', 'default': False},
    'base-target-name': {'sdk': True, 'crossdev': '', 'default': True},
}


class ToolchainTest(cros_test_lib.MockTempDirTestCase):
  """Tests for lib.toolchain."""

  def testArchForToolchain(self):
    """Tests that we correctly parse crossdev's output."""
    rc_mock = cros_test_lib.RunCommandMock()

    noarch = """target=foo
category=bla
"""
    rc_mock.SetDefaultCmdResult(output=noarch)
    with rc_mock:
      self.assertEqual(None, toolchain.GetArchForTarget('fake_target'))

    amd64arch = """arch=amd64
target=foo
"""
    rc_mock.SetDefaultCmdResult(output=amd64arch)
    with rc_mock:
      self.assertEqual('amd64', toolchain.GetArchForTarget('fake_target'))

  @mock.patch('chromite.lib.toolchain.portage_util.FindOverlays')
  def testReadsBoardToolchains(self, find_overlays_mock):
    """Tests that we correctly parse toolchain configs for an overlay stack."""
    # Create some fake overlays and put toolchain confs in a subset of them.
    overlays = [os.path.join(self.tempdir, 'overlay%d' % i) for i in range(3)]
    for overlay in overlays:
      osutils.SafeMakedirs(overlay)
    for overlay, contents in [(overlays[0], BASE_TOOLCHAIN_CONF),
                              (overlays[2], ADDITIONAL_TOOLCHAIN_CONF)]:
      osutils.WriteFile(os.path.join(overlay, 'toolchain.conf'), contents)
    find_overlays_mock.return_value = overlays
    actual_targets = toolchain.GetToolchainsForBoard('board_value')
    self.assertEqual(EXPECTED_TOOLCHAINS, actual_targets)
