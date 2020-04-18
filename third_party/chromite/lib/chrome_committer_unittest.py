# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for the chrome_committer library."""

from __future__ import print_function

import os

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import chrome_committer
from chromite.lib import tree_status


class ChromeCommitterTester(cros_test_lib.RunCommandTestCase,
                            cros_test_lib.MockTempDirTestCase):
  """Test cros_chromeos_lkgm.Committer."""

  class Args(object):
    """Class for ChromeComitter args."""
    def __init__(self, workdir):
      self.workdir = workdir
      self.dryrun = False
      self.user_email = 'user@test.org'


  def setUp(self):
    """Common set up method for all tests."""
    osutils.SafeMakedirs(os.path.join(self.tempdir, '.git', 'info'))
    osutils.WriteFile(os.path.join(self.tempdir, 'OWNERS'), 'foo@chromium.org')
    osutils.SafeMakedirs(os.path.join(self.tempdir, 'chromeos'))
    osutils.WriteFile(os.path.join(self.tempdir, 'chromeos', 'BUILD.gn'),
                      'assert(is_chromeos)')
    self.committer = chrome_committer.ChromeCommitter(
        ChromeCommitterTester.Args(self.tempdir))

  def _assertCommand(self, git_cmd):
    self.assertCommandContains(git_cmd.split(' '))

  def testCheckout(self):
    "Tests checkout with mocked out git."
    self.committer.Checkout(['OWNERS'])

    self._assertCommand('git init')
    self._assertCommand('git remote add origin '
                        'https://chromium.googlesource.com/chromium/src.git')
    self._assertCommand('git config core.sparsecheckout true')
    self._assertCommand('git fetch --depth=1')
    self._assertCommand('git pull origin master')
    self._assertCommand('git checkout -B auto-commit-branch origin/master')
    self.assertEquals(osutils.ReadFile(
        os.path.join(self.tempdir, '.git', 'info', 'sparse-checkout')),
                      'OWNERS\ncodereview.settings\nWATCHLISTS')

  def testCommit(self):
    """Tests that we can commit a file."""
    self.committer.Checkout(['OWNERS'])
    self.committer.Commit(['OWNERS', 'chromeos/BUILD.gn'],
                          'Modify OWNERS and BUILD.gn')

    self._assertCommand('git add -- OWNERS')
    self._assertCommand('git add -- BUILD.gn')
    self.assertCommandContains(['git',
                                '-c', 'user.email=user@test.org',
                                '-c', 'user.name=user@test.org',
                                'commit', '-m',
                                'Automated Commit: Modify OWNERS and BUILD.gn'])

    # Non-existent file should raise.
    self.assertRaisesRegexp(chrome_committer.CommitError,
                            'Invalid path: /tmp/chromite.*/nonexistent$',
                            self.committer.Commit,
                            ['nonexistent'], 'Commit non-existent file')

  def testUpload(self):
    """Tests that we can upload a commit."""
    self.committer.Checkout(['OWNERS'])
    self.committer.Commit(['OWNERS', 'chromeos/BUILD.gn'],
                          'Modify OWNERS and BUILD.gn')

    self.PatchObject(tree_status, 'GetSheriffEmailAddresses',
                     return_value=['gardener@chromium.org'])
    self.committer.Upload()

    self.assertCommandContains(['git',
                                '-c', 'user.email=user@test.org',
                                '-c', 'user.name=user@test.org',
                                'cl', 'upload', '-v', '-m',
                                'Automated Commit: Modify OWNERS and BUILD.gn',
                                '--bypass-hooks', '-f',
                                '--tbrs', 'gardener@chromium.org',
                                '--send-mail'])
    self._assertCommand('git cl set-commit -v')

  def testUploadDryRun(self):
    """Tests that we can upload a commit with dryrun."""
    self.committer.Checkout(['OWNERS'])
    self.committer.Commit(['OWNERS', 'chromeos/BUILD.gn'],
                          'Modify OWNERS and BUILD.gn')

    self.PatchObject(tree_status, 'GetSheriffEmailAddresses',
                     return_value=['gardener@chromium.org'])
    self.committer._dryrun = True  # pylint: disable=protected-access
    self.committer.Upload()

    self.assertCommandContains(['git',
                                '-c', 'user.email=user@test.org',
                                '-c', 'user.name=user@test.org',
                                'cl', 'upload', '-v', '-m',
                                'Automated Commit: Modify OWNERS and BUILD.gn',
                                '--bypass-hooks', '-f'])
    self._assertCommand('git cl set-commit -v --dry-run')
