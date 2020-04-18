# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for the chrome_chromeos_lkgm program."""

from __future__ import print_function

import os

from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import partial_mock
from chromite.scripts import chrome_chromeos_lkgm


# pylint: disable=protected-access
class ChromeLKGMCommitterTester(cros_test_lib.RunCommandTestCase,
                                cros_test_lib.MockTempDirTestCase):
  """Test cros_chromeos_lkgm.Committer."""

  class Args(object):
    """Class for ChromeLKGMComitter args."""
    def __init__(self, workdir, lkgm):
      self.workdir = workdir
      self.lkgm = lkgm
      self.dryrun = False
      self.user_email = 'user@test.org'


  def setUp(self):
    """Common set up method for all tests."""
    self.committer = chrome_chromeos_lkgm.ChromeLKGMCommitter(
        ChromeLKGMCommitterTester.Args(self.tempdir, '1001.0.0'))
    self.lkgm_file = os.path.join(self.tempdir, constants.PATH_TO_CHROME_LKGM)
    self.old_lkgm = None

  def _createOldLkgm(self, *args, **kwargs):  # pylint: disable=unused-argument
    # Write out an old lkgm file as if we got it from a git fetch.
    osutils.SafeMakedirs(os.path.join(self.tempdir, '.git', 'info'))
    osutils.SafeMakedirs(os.path.dirname(self.lkgm_file))
    osutils.WriteFile(self.lkgm_file, self.old_lkgm)

  def testCheckoutChromeLKGM(self):
    "Tests that we can read/obtain the old LKGM from mocked out git."
    self.old_lkgm = '1234.0.0'
    self.rc.AddCmdResult(partial_mock.In('remote'), returncode=0,
                         side_effect=self._createOldLkgm)
    self.committer.CheckoutChrome()

    self.assertEqual(self.committer.lkgm_file, self.lkgm_file)
    self.assertEqual(osutils.ReadFile(self.lkgm_file), self.old_lkgm)

  def testCommitNewLKGM(self):
    """Tests that we can commit a new LKGM file."""
    self.old_lkgm = '999.0.0'
    self.rc.AddCmdResult(partial_mock.In('remote'), returncode=0,
                         side_effect=self._createOldLkgm)
    self.committer.CheckoutChrome()

    self.assertEqual(self.committer.lkgm_file, self.lkgm_file)

    self.committer.UpdateLKGM()
    self.committer.CommitNewLKGM()

    # Check the file was actually written out correctly.
    self.assertEqual(osutils.ReadFile(self.lkgm_file), self.committer._lkgm)
    self.assertCommandContains(['git', 'commit'])
    self.assertEqual(self.committer._old_lkgm, self.old_lkgm)

  def testOlderLKGMFails(self):
    """Tests that trying to update to an older lkgm version fails."""
    self.old_lkgm = '1002.0.0'
    self.rc.AddCmdResult(partial_mock.In('remote'), returncode=0,
                         side_effect=self._createOldLkgm)

    self.committer.CheckoutChrome()

    self.assertRaises(chrome_chromeos_lkgm.LKGMNotValid,
                      self.committer.UpdateLKGM)
    self.assertEqual(self.committer._old_lkgm, self.old_lkgm)
    self.assertEqual(self.committer._lkgm, '1001.0.0')
    self.assertEqual(osutils.ReadFile(self.lkgm_file), '1002.0.0')

  def testVersionWithChromeBranch(self):
    """Tests passing a version with a chrome branch strips the branch."""
    self.committer = chrome_chromeos_lkgm.ChromeLKGMCommitter(
        ChromeLKGMCommitterTester.Args(self.tempdir, '1003.0.0-rc2'))

    self.old_lkgm = '1002.0.0'
    self.rc.AddCmdResult(partial_mock.In('remote'), returncode=0,
                         side_effect=self._createOldLkgm)

    self.committer.CheckoutChrome()
    self.committer.UpdateLKGM()
    self.committer.CommitNewLKGM()

    # Check the file was actually written out correctly.
    stripped_lkgm = '1003.0.0'
    self.assertEqual(osutils.ReadFile(self.lkgm_file), stripped_lkgm)
    self.assertEqual(self.committer._old_lkgm, self.old_lkgm)
