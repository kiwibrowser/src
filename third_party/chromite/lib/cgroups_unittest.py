# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for cgroups.py."""

from __future__ import print_function

from chromite.lib import cgroups
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import parallel
from chromite.lib import sudo


class TestCreateGroups(cros_test_lib.TestCase):
  """Unittests for creating groups."""

  def _CrosSdk(self):
    cmd = ['cros_sdk', '--', 'sleep', '0.001']
    cros_build_lib.RunCommand(cmd)

  def testCreateGroups(self):
    """Run many cros_sdk processes in parallel to test for race conditions."""
    with sudo.SudoKeepAlive():
      with cgroups.SimpleContainChildren('example', sigterm_timeout=5):
        parallel.RunTasksInProcessPool(self._CrosSdk, [[]] * 20,
                                       processes=10)
