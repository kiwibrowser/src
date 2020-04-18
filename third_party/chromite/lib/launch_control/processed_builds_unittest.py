# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test processed_builds."""

from __future__ import print_function

import os

from chromite.lib.launch_control import processed_builds
from chromite.lib import cros_test_lib


# Unitests often need access to internals of the thing they test.
# pylint: disable=protected-access

class ProcessedBuildsStorageTest(cros_test_lib.TempDirTestCase):
  """Test our helper library for storing processed build ids."""

  def setUp(self):
    self.testfile = os.path.join(self.tempdir, 'testfile.json')

  def testStartStop(self):
    with processed_builds.ProcessedBuildsStorage(self.testfile):
      pass

    self.assertFileContents(self.testfile, '{}')

  def testFetchEmpty(self):
    with processed_builds.ProcessedBuildsStorage(self.testfile) as ps:
      self.assertEqual(ps.GetProcessedBuilds('branch', 'target'), [])

    self.assertFileContents(self.testfile, '{"branch": {"target": []}}')

  def testPurgeEmpty(self):
    with processed_builds.ProcessedBuildsStorage(self.testfile) as ps:
      ps.PurgeOldBuilds('branch', 'target', [1, 2, 3])

    self.assertFileContents(self.testfile, '{"branch": {"target": []}}')

  def testAddEmpty(self):
    with processed_builds.ProcessedBuildsStorage(self.testfile) as ps:
      ps.AddProcessedBuild('branch', 'target', 1)

    self.assertFileContents(self.testfile, '{"branch": {"target": [1]}}')

  def testMultipleUses(self):
    with processed_builds.ProcessedBuildsStorage(self.testfile) as ps:
      ps.AddProcessedBuild('branch', 'target', 1)
      ps.AddProcessedBuild('branch', 'target', 2)

    self.assertFileContents(self.testfile, '{"branch": {"target": [1, 2]}}')

    with processed_builds.ProcessedBuildsStorage(self.testfile) as ps:
      # Try adding twice, should only happen once.
      ps.AddProcessedBuild('branch', 'target', 3)
      ps.AddProcessedBuild('branch', 'target', 3)

    self.assertFileContents(self.testfile, '{"branch": {"target": [1, 2, 3]}}')

    with processed_builds.ProcessedBuildsStorage(self.testfile) as ps:
      ps.PurgeOldBuilds('branch', 'target', [2, 3])
      ps.AddProcessedBuild('branch', 'target', 4)

    self.assertFileContents(self.testfile, '{"branch": {"target": [2, 3, 4]}}')

    with processed_builds.ProcessedBuildsStorage(self.testfile) as ps:
      self.assertEqual(ps.GetProcessedBuilds('branch', 'target'), [2, 3, 4])

  def testAddMultipleBranchTargets(self):
    with processed_builds.ProcessedBuildsStorage(self.testfile) as ps:
      ps.AddProcessedBuild('branch1', 'target', 1)
      ps.AddProcessedBuild('branch2', 'target', 1)
      ps.AddProcessedBuild('branch2', 'target', 2)
      ps.AddProcessedBuild('branch2', 'target2', 3)

      self.assertEqual(ps.GetProcessedBuilds('branch2', 'target'),
                       [1, 2])

    self.assertFileContents(
        self.testfile,
        '{"branch1": {"target": [1]},'
        ' "branch2": {"target": [1, 2], "target2": [3]}}')
