# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test cros_generate_os_release."""

from __future__ import print_function

import os

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import osutils
import cros_generate_os_release


class CrosGenerateOsReleaseTest(cros_test_lib.TempDirTestCase):
  """Tests GenerateOsRelease."""

  def setUp(self):
    # Use a fresh tempdir as the root for each test case.
    self.osrelease = os.path.join(self.tempdir, "etc", "os-release")
    self.osreleased = os.path.join(self.tempdir, "etc", "os-release.d")
    osutils.SafeMakedirs(self.osreleased)

  def testOnlyOsRelease(self):
    """Tests the script without /etc/os-release."""
    osutils.WriteFile(os.path.join(self.osreleased, "TEST"), "hello")
    cros_generate_os_release.GenerateOsRelease(self.tempdir)
    self.assertEquals("TEST=hello\n", osutils.ReadFile(self.osrelease))

  def testOnlyOsReleaseD(self):
    """Tests the script without /etc/os-release.d."""
    osutils.RmDir(self.osreleased)
    osutils.WriteFile(self.osrelease, "TEST=bonjour\n")

    cros_generate_os_release.GenerateOsRelease(self.tempdir)
    self.assertEquals("TEST=bonjour\n", osutils.ReadFile(self.osrelease))

  def testFailOnDuplicate(self):
    """Tests with a field set both in os-release and os-release.d/."""
    osutils.WriteFile(os.path.join(self.osreleased, "TEST"), "hello")
    osutils.WriteFile(self.osrelease, "TEST=bonjour")

    self.assertRaises(cros_build_lib.DieSystemExit,
                      cros_generate_os_release.GenerateOsRelease, self.tempdir)

  def testNormal(self):
    """Normal scenario: both os-release and os-release.d are present."""
    osutils.WriteFile(os.path.join(self.osreleased, "TEST1"), "hello")
    osutils.WriteFile(self.osrelease, "TEST2=bonjour")

    default_params = {"TEST1": "hello2",
                      "TEST3": "hola"}

    cros_generate_os_release.GenerateOsRelease(self.tempdir,
                                               default_params=default_params)
    output = osutils.ReadFile(self.osrelease).splitlines()
    output.sort()
    self.assertEquals(["TEST1=hello",
                       "TEST2=bonjour",
                       "TEST3=hola"],
                      output)
