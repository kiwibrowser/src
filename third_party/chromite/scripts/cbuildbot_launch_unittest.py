# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for chromite.scripts.cbuildbot_launch."""

from __future__ import print_function

import mock
import os
import time

from chromite.cbuildbot import repository
from chromite.lib import build_summary
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_sdk_lib
from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.scripts import cbuildbot_launch

EXPECTED_MANIFEST_URL = 'https://chrome-internal-review.googlesource.com/chromeos/manifest-internal'  # pylint: disable=line-too-long


# It's reasonable for unittests to look at internals.
# pylint: disable=protected-access


class FakeException(Exception):
  """Test exception to raise during tests."""


class CbuildbotLaunchTest(cros_test_lib.MockTestCase):
  """Tests for cbuildbot_launch script."""

  def testPreParseArguments(self):
    """Test that we can correctly extract branch values from cbuildbot args."""
    CASES = (
        (['--buildroot', '/buildroot', 'daisy-incremental'],
         (None, '/buildroot', None)),

        (['--buildbot', '--buildroot', '/buildroot',
          '--git-cache-dir', '/git-cache',
          '-b', 'release-R57-9202.B',
          'daisy-incremental'],
         ('release-R57-9202.B', '/buildroot', '/git-cache')),

        (['--debug', '--buildbot', '--notests',
          '--buildroot', '/buildroot',
          '--git-cache-dir', '/git-cache',
          '--branch', 'release-R57-9202.B',
          'daisy-incremental'],
         ('release-R57-9202.B', '/buildroot', '/git-cache')),
    )

    for cmd_args, expected in CASES:
      expected_branch, expected_buildroot, expected_cache_dir = expected

      options = cbuildbot_launch.PreParseArguments(cmd_args)

      self.assertEqual(options.branch, expected_branch)
      self.assertEqual(options.buildroot, expected_buildroot)
      self.assertEqual(options.git_cache_dir, expected_cache_dir)

  def testInitialCheckout(self):
    """Test InitialCheckout with minimum settings."""
    mock_repo = mock.MagicMock()
    mock_repo.branch = 'branch'

    cbuildbot_launch.InitialCheckout(mock_repo)

    self.assertEqual(mock_repo.mock_calls, [
        mock.call.Sync(detach=True),
    ])

  def testConfigureGlobalEnvironment(self):
    """Ensure that we can setup our global runtime environment correctly."""

    os.environ.pop('LANG', None)
    os.environ['LC_MONETARY'] = 'bad'

    cbuildbot_launch.ConfigureGlobalEnvironment()

    # Verify umask is updated.
    self.assertEqual(os.umask(0), 0o22)

    # Verify ENVs are cleaned up.
    self.assertEqual(os.environ['LANG'], 'en_US.UTF-8')
    self.assertNotIn('LC_MONETARY', os.environ)


class RunDepotToolsEnsureBootstrap(cros_test_lib.RunCommandTestCase,
                                   cros_test_lib.TempDirTestCase):
  """Test the helper function DepotToolsEnsureBootstrap."""

  def testEnsureBootstrap(self):
    """Verify that the script is run if present."""
    script = os.path.join(self.tempdir, 'ensure_bootstrap')
    osutils.Touch(script, makedirs=True)

    cbuildbot_launch.DepotToolsEnsureBootstrap(self.tempdir)
    self.assertCommandCalled(
        [script], extra_env={'PATH': mock.ANY}, cwd=self.tempdir)

  def testEnsureBootstrapMissing(self):
    """Verify that the script is NOT run if not present."""
    cbuildbot_launch.DepotToolsEnsureBootstrap(self.tempdir)
    self.assertEqual(self.rc.call_count, 0)


class RunTests(cros_test_lib.RunCommandTestCase):
  """Tests for cbuildbot_launch script."""

  ARGS_BASE = ['--buildroot', '/buildroot']
  EXPECTED_ARGS_BASE = ['--buildroot', '/cbuildbot_buildroot']
  ARGS_GIT_CACHE = ['--git-cache-dir', '/git-cache']
  ARGS_CONFIG = ['config']
  CMD = ['/cbuildbot_buildroot/chromite/bin/cbuildbot']

  def verifyCbuildbot(self, args, expected_cmd, version):
    """Ensure we invoke cbuildbot correctly."""
    self.PatchObject(
        cros_build_lib, 'GetTargetChromiteApiVersion', autospec=True,
        return_value=version)

    cbuildbot_launch.Cbuildbot('/cbuildbot_buildroot', '/depot_tools', args)

    self.assertCommandCalled(
        expected_cmd, extra_env={'PATH': mock.ANY},
        cwd='/cbuildbot_buildroot', error_code_ok=True)

  def testCbuildbotSimple(self):
    """Ensure we invoke cbuildbot correctly."""
    self.verifyCbuildbot(
        self.ARGS_BASE + self.ARGS_CONFIG,
        self.CMD + self.ARGS_CONFIG + self.EXPECTED_ARGS_BASE,
        (0, 4))

  def testCbuildbotNotFiltered(self):
    """Ensure we invoke cbuildbot correctly."""
    self.verifyCbuildbot(
        self.ARGS_BASE + self.ARGS_CONFIG + self.ARGS_GIT_CACHE,
        (self.CMD + self.ARGS_CONFIG + self.EXPECTED_ARGS_BASE +
         self.ARGS_GIT_CACHE),
        (0, 4))

  def testCbuildbotFiltered(self):
    """Ensure we invoke cbuildbot correctly."""
    self.verifyCbuildbot(
        self.ARGS_BASE + self.ARGS_CONFIG + self.ARGS_GIT_CACHE,
        self.CMD + self.ARGS_CONFIG + self.EXPECTED_ARGS_BASE,
        (0, 2))

  def testMainMin(self):
    """Test a minimal set of command line options."""
    self.PatchObject(osutils, 'SafeMakedirs', autospec=True)
    self.PatchObject(cros_build_lib, 'GetTargetChromiteApiVersion',
                     autospec=True, return_value=(constants.REEXEC_API_MAJOR,
                                                  constants.REEXEC_API_MINOR))
    mock_repo = mock.MagicMock()
    mock_repo.branch = 'master'
    mock_repo.directory = '/root/repository'

    mock_repo_create = self.PatchObject(repository, 'RepoRepository',
                                        autospec=True, return_value=mock_repo)
    mock_clean = self.PatchObject(cbuildbot_launch, 'CleanBuildRoot',
                                  autospec=True)
    mock_checkout = self.PatchObject(cbuildbot_launch, 'InitialCheckout',
                                     autospec=True)
    mock_cleanup_chroot = self.PatchObject(cbuildbot_launch, 'CleanupChroot',
                                           autospec=True)
    mock_set_last_build_state = self.PatchObject(
        cbuildbot_launch, 'SetLastBuildState', autospec=True)

    expected_build_state = build_summary.BuildSummary(
        build_number=0, master_build_id=0, status=mock.ANY,
        buildroot_layout=2, branch='master')

    cbuildbot_launch._main(['-r', '/root', 'config'])

    # Did we create the repo instance correctly?
    self.assertEqual(mock_repo_create.mock_calls,
                     [mock.call(EXPECTED_MANIFEST_URL, '/root/repository',
                                git_cache_dir=None, branch='master')])

    # Ensure we clean, as expected.
    self.assertEqual(mock_clean.mock_calls, [
        mock.call('/root', mock_repo,
                  {
                      'branch_name': 'master',
                      'tryjob': False,
                      'build_config': 'config',
                  },
                  expected_build_state)])

    # Ensure we checkout, as expected.
    self.assertEqual(mock_checkout.mock_calls,
                     [mock.call(mock_repo)])

    # Ensure we invoke cbuildbot, as expected.
    self.assertCommandCalled(
        [
            '/root/repository/chromite/bin/cbuildbot',
            'config',
            '-r', '/root/repository',
            '--ts-mon-task-num', '1',
        ],
        extra_env={'PATH': mock.ANY},
        cwd='/root/repository',
        error_code_ok=True)

    # Ensure we saved the final state, as expected.
    self.assertEqual(expected_build_state.status,
                     constants.BUILDER_STATUS_PASSED)
    self.assertEqual(mock_set_last_build_state.mock_calls, [
        mock.call('/root', expected_build_state)])

    # Ensure we clean the chroot, as expected.
    self.assertEqual(mock_cleanup_chroot.mock_calls, [
        mock.call('/root/repository')])

  def testMainMax(self):
    """Test a larger set of command line options."""
    self.PatchObject(osutils, 'SafeMakedirs', autospec=True)
    self.PatchObject(cros_build_lib, 'GetTargetChromiteApiVersion',
                     autospec=True, return_value=(constants.REEXEC_API_MAJOR,
                                                  constants.REEXEC_API_MINOR))
    mock_repo = mock.MagicMock()
    mock_repo.branch = 'branch'
    mock_repo.directory = '/root/repository'

    mock_summary = build_summary.BuildSummary(
        build_number=313,
        master_build_id=123123123,
        status=constants.BUILDER_STATUS_FAILED,
        buildroot_layout=cbuildbot_launch.BUILDROOT_BUILDROOT_LAYOUT,
        branch='branch')

    mock_get_last_build_state = self.PatchObject(
        cbuildbot_launch, 'GetLastBuildState', autospec=True,
        return_value=mock_summary)
    mock_repo_create = self.PatchObject(repository, 'RepoRepository',
                                        autospec=True, return_value=mock_repo)
    mock_clean = self.PatchObject(cbuildbot_launch, 'CleanBuildRoot',
                                  autospec=True)
    mock_checkout = self.PatchObject(cbuildbot_launch, 'InitialCheckout',
                                     autospec=True)
    mock_cleanup_chroot = self.PatchObject(cbuildbot_launch, 'CleanupChroot',
                                           autospec=True)
    mock_set_last_build_state = self.PatchObject(
        cbuildbot_launch, 'SetLastBuildState', autospec=True)

    cbuildbot_launch._main(['--buildroot', '/root',
                            '--branch', 'branch',
                            '--git-cache-dir', '/git-cache',
                            '--remote-trybot',
                            '--master-build-id', '123456789',
                            '--buildnumber', '314',
                            'config'])

    # Did we create the repo instance correctly?
    self.assertEqual(mock_repo_create.mock_calls,
                     [mock.call(EXPECTED_MANIFEST_URL, '/root/repository',
                                git_cache_dir='/git-cache', branch='branch')])

    # Ensure we look up the previous status.
    self.assertEqual(mock_get_last_build_state.mock_calls, [
        mock.call('/root')])

    # Ensure we clean, as expected.
    self.assertEqual(mock_clean.mock_calls, [
        mock.call('/root',
                  mock_repo,
                  {
                      'branch_name': 'branch',
                      'tryjob': True,
                      'build_config': 'config',
                  },
                  build_summary.BuildSummary(
                      build_number=314,
                      master_build_id=123456789,
                      status=mock.ANY,
                      branch='branch',
                      buildroot_layout=2
                  ))])

    # Ensure we checkout, as expected.
    self.assertEqual(mock_checkout.mock_calls,
                     [mock.call(mock_repo)])

    # Ensure we invoke cbuildbot, as expected.
    self.assertCommandCalled(
        [
            '/root/repository/chromite/bin/cbuildbot',
            'config',
            '--buildroot', '/root/repository',
            '--branch', 'branch',
            '--git-cache-dir', '/git-cache',
            '--remote-trybot',
            '--master-build-id', '123456789',
            '--buildnumber', '314',
            '--previous-build-state',
            'eyJzdGF0dXMiOiAiZmFpbCIsICJtYXN0ZXJfYnVpbGRfaWQiOiAxMjMxMjMxMj'
            'MsICJidWlsZF9udW1iZXIiOiAzMTMsICJidWlsZHJvb3RfbGF5b3V0IjogMiwg'
            'ImJyYW5jaCI6ICJicmFuY2gifQ==',
            '--ts-mon-task-num', '1',
        ],
        extra_env={'PATH': mock.ANY},
        cwd='/root/repository',
        error_code_ok=True)

    # Ensure we write the final build state, as expected.
    final_state = build_summary.BuildSummary(
        build_number=314,
        master_build_id=123456789,
        status=constants.BUILDER_STATUS_PASSED,
        buildroot_layout=2,
        branch='branch')
    self.assertEqual(mock_set_last_build_state.mock_calls, [
        mock.call('/root', final_state)])

    # Ensure we clean the chroot, as expected.
    self.assertEqual(mock_cleanup_chroot.mock_calls, [
        mock.call('/root/repository')])


class CleanBuildRootTest(cros_test_lib.MockTempDirTestCase):
  """Tests for CleanBuildRoot method."""

  def setUp(self):
    """Create standard buildroot contents for cleanup."""
    self.root = os.path.join(self.tempdir)
    self.previous_build_state = os.path.join(
        self.root, '.cbuildbot_build_state.json')
    self.buildroot = os.path.join(self.root, 'buildroot')
    self.repo = os.path.join(self.buildroot, '.repo/repo')
    self.chroot = os.path.join(self.buildroot, 'chroot/chroot')
    self.general = os.path.join(self.buildroot, 'general/general')
    self.cache = os.path.join(self.buildroot, '.cache')
    self.distfiles = os.path.join(self.cache, 'distfiles')

    self.mock_repo = mock.MagicMock()
    self.mock_repo.directory = self.buildroot

    self.metrics = {}

  def populateBuildroot(self, previous_build_state=None):
    """Create standard buildroot contents for cleanup."""
    if previous_build_state:
      osutils.SafeMakedirs(self.root)
      osutils.WriteFile(self.previous_build_state, previous_build_state)

    # Create files.
    for f in (self.repo, self.chroot, self.general, self.distfiles):
      osutils.Touch(f, makedirs=True)

  def testNoBuildroot(self):
    """Test CleanBuildRoot with no history."""
    self.mock_repo.branch = 'master'

    build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_INFLIGHT,
        buildroot_layout=cbuildbot_launch.BUILDROOT_BUILDROOT_LAYOUT,
        branch='master')
    cbuildbot_launch.CleanBuildRoot(
        self.root, self.mock_repo, self.metrics, build_state)

    new_summary = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(new_summary.buildroot_layout, 2)
    self.assertEqual(new_summary.branch, 'master')
    self.assertIsNotNone(new_summary.distfiles_ts)
    self.assertEqual(new_summary, build_state)

    self.assertExists(self.previous_build_state)

  def testBuildrootNoState(self):
    """Test CleanBuildRoot with no state information."""
    self.populateBuildroot()
    self.mock_repo.branch = 'master'

    build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_INFLIGHT,
        buildroot_layout=cbuildbot_launch.BUILDROOT_BUILDROOT_LAYOUT,
        branch='master')
    cbuildbot_launch.CleanBuildRoot(
        self.root, self.mock_repo, self.metrics, build_state)

    new_summary = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(new_summary.buildroot_layout, 2)
    self.assertEqual(new_summary.branch, 'master')
    self.assertIsNotNone(new_summary.distfiles_ts)
    self.assertEqual(new_summary, build_state)

    self.assertNotExists(self.repo)
    self.assertNotExists(self.chroot)
    self.assertNotExists(self.general)
    self.assertNotExists(self.distfiles)
    self.assertExists(self.previous_build_state)

  def testBuildrootFormatMismatch(self):
    """Test CleanBuildRoot with buildroot layout mismatch."""
    old_build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_PASSED,
        buildroot_layout=1,
        branch='master')
    self.populateBuildroot(previous_build_state=old_build_state.to_json())
    self.mock_repo.branch = 'master'

    build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_INFLIGHT,
        buildroot_layout=cbuildbot_launch.BUILDROOT_BUILDROOT_LAYOUT,
        branch='master')
    cbuildbot_launch.CleanBuildRoot(
        self.root, self.mock_repo, self.metrics, build_state)

    new_summary = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(new_summary.buildroot_layout, 2)
    self.assertEqual(new_summary.branch, 'master')
    self.assertIsNotNone(new_summary.distfiles_ts)
    self.assertEqual(new_summary, build_state)

    self.assertNotExists(self.repo)
    self.assertNotExists(self.chroot)
    self.assertNotExists(self.general)
    self.assertNotExists(self.distfiles)
    self.assertExists(self.previous_build_state)

  def testBuildrootBranchChange(self):
    """Test CleanBuildRoot with a change in branches."""
    old_build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_PASSED,
        buildroot_layout=2,
        branch='branchA')
    self.populateBuildroot(previous_build_state=old_build_state.to_json())
    self.mock_repo.branch = 'branchB'
    m = self.PatchObject(cros_sdk_lib, 'CleanupChrootMount')

    build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_INFLIGHT,
        buildroot_layout=cbuildbot_launch.BUILDROOT_BUILDROOT_LAYOUT,
        branch='branchB')
    cbuildbot_launch.CleanBuildRoot(
        self.root, self.mock_repo, self.metrics, build_state)

    new_summary = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(new_summary.buildroot_layout, 2)
    self.assertEqual(new_summary.branch, 'branchB')
    self.assertIsNotNone(new_summary.distfiles_ts)
    self.assertEqual(new_summary, build_state)

    self.assertExists(self.repo)
    self.assertNotExists(self.chroot)
    self.assertExists(self.general)
    self.assertNotExists(self.distfiles)
    self.assertExists(self.previous_build_state)
    m.assert_called()

  def testBuildrootBranchMatch(self):
    """Test CleanBuildRoot with no change in branch."""
    old_build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_PASSED,
        buildroot_layout=2,
        branch='branchA')
    self.populateBuildroot(previous_build_state=old_build_state.to_json())
    self.mock_repo.branch = 'branchA'

    build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_INFLIGHT,
        buildroot_layout=cbuildbot_launch.BUILDROOT_BUILDROOT_LAYOUT,
        branch='branchA')
    cbuildbot_launch.CleanBuildRoot(
        self.root, self.mock_repo, self.metrics, build_state)

    new_summary = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(new_summary.buildroot_layout, 2)
    self.assertEqual(new_summary.branch, 'branchA')
    self.assertIsNotNone(new_summary.distfiles_ts)
    self.assertEqual(new_summary, build_state)

    self.assertExists(self.repo)
    self.assertExists(self.chroot)
    self.assertExists(self.general)
    self.assertExists(self.distfiles)
    self.assertExists(self.previous_build_state)

  def testBuildrootDistfilesRecentCache(self):
    """Test CleanBuildRoot does not delete distfiles when cache is recent."""
    seed_distfiles_ts = time.time() - 60
    old_build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_PASSED,
        buildroot_layout=2,
        branch='branchA',
        distfiles_ts=seed_distfiles_ts)
    self.populateBuildroot(previous_build_state=old_build_state.to_json())
    self.mock_repo.branch = 'branchA'

    build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_INFLIGHT,
        buildroot_layout=cbuildbot_launch.BUILDROOT_BUILDROOT_LAYOUT,
        branch='branchA')
    cbuildbot_launch.CleanBuildRoot(
        self.root, self.mock_repo, self.metrics, build_state)

    new_summary = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(new_summary.buildroot_layout, 2)
    self.assertEqual(new_summary.branch, 'branchA')
    # Same cache creation timestamp is rewritten to state.
    self.assertEqual(new_summary.distfiles_ts, seed_distfiles_ts)
    self.assertEqual(new_summary, build_state)

    self.assertExists(self.repo)
    self.assertExists(self.chroot)
    self.assertExists(self.general)
    self.assertExists(self.distfiles)
    self.assertExists(self.previous_build_state)

  def testBuildrootDistfilesCacheExpired(self):
    """Test CleanBuildRoot when the distfiles cache is too old."""
    old_build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_PASSED,
        buildroot_layout=2,
        branch='branchA',
        distfiles_ts=100.0)
    self.populateBuildroot(previous_build_state=old_build_state.to_json())
    self.mock_repo.branch = 'branchA'

    build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_INFLIGHT,
        buildroot_layout=cbuildbot_launch.BUILDROOT_BUILDROOT_LAYOUT,
        branch='branchA')
    cbuildbot_launch.CleanBuildRoot(
        self.root, self.mock_repo, self.metrics, build_state)

    new_summary = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(new_summary.buildroot_layout, 2)
    self.assertEqual(new_summary.branch, 'branchA')
    self.assertIsNotNone(new_summary.distfiles_ts)
    self.assertEqual(new_summary, build_state)

    self.assertExists(self.repo)
    self.assertExists(self.chroot)
    self.assertExists(self.general)
    self.assertNotExists(self.distfiles)
    self.assertExists(self.previous_build_state)

  def testBuildrootRepoCleanFailure(self):
    """Test CleanBuildRoot with repo checkout failure."""
    old_build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_PASSED,
        buildroot_layout=1,
        branch='branchA')
    self.populateBuildroot(previous_build_state=old_build_state.to_json())
    self.mock_repo.branch = 'branchA'
    self.mock_repo.BuildRootGitCleanup.side_effect = Exception

    build_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_INFLIGHT,
        buildroot_layout=cbuildbot_launch.BUILDROOT_BUILDROOT_LAYOUT,
        branch='branchA')
    cbuildbot_launch.CleanBuildRoot(
        self.root, self.mock_repo, self.metrics, build_state)

    new_summary = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(new_summary.buildroot_layout, 2)
    self.assertEqual(new_summary.branch, 'branchA')
    self.assertIsNotNone(new_summary.distfiles_ts)
    self.assertEqual(new_summary, build_state)

    self.assertNotExists(self.repo)
    self.assertNotExists(self.chroot)
    self.assertNotExists(self.general)
    self.assertNotExists(self.distfiles)
    self.assertExists(self.previous_build_state)

  def testGetCurrentBuildStateNoArgs(self):
    """Tests GetCurrentBuildState without arguments."""
    options = cbuildbot_launch.PreParseArguments([
        '--buildroot', self.root, 'config'
    ])
    state = cbuildbot_launch.GetCurrentBuildState(options, 'master')

    expected_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_INFLIGHT,
        buildroot_layout=2,
        branch='master')
    self.assertEqual(state, expected_state)

  def testGetCurrentBuildStateHasArgs(self):
    """Tests GetCurrentBuildState with arguments."""
    options = cbuildbot_launch.PreParseArguments([
        '--buildroot', self.root,
        '--buildnumber', '20',
        '--master-build-id', '50',
        'config'
    ])
    state = cbuildbot_launch.GetCurrentBuildState(options, 'branchA')

    expected_state = build_summary.BuildSummary(
        build_number=20,
        master_build_id=50,
        status=constants.BUILDER_STATUS_INFLIGHT,
        buildroot_layout=2,
        branch='branchA')
    self.assertEqual(state, expected_state)

  def testGetCurrentBuildStateLayout(self):
    """Test that GetCurrentBuildState uses the current buildroot layout."""
    # Change to a future version.
    self.PatchObject(cbuildbot_launch, 'BUILDROOT_BUILDROOT_LAYOUT', 22)

    options = cbuildbot_launch.PreParseArguments([
        '--buildroot', self.root, 'config'
    ])
    state = cbuildbot_launch.GetCurrentBuildState(options, 'branchA')

    expected_state = build_summary.BuildSummary(
        status=constants.BUILDER_STATUS_INFLIGHT,
        buildroot_layout=22,
        branch='branchA')
    self.assertEqual(state, expected_state)

  def testGetLastBuildStateNoFile(self):
    """Tests GetLastBuildState if the file is missing."""
    osutils.SafeMakedirs(self.root)
    state = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(state, build_summary.BuildSummary())

  def testGetLastBuildStateBadFile(self):
    """Tests GetLastBuildState if the file contains invalid JSON."""
    osutils.SafeMakedirs(self.root)
    osutils.WriteFile(self.previous_build_state, '}}')
    state = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(state, build_summary.BuildSummary())

  def testGetLastBuildStateMissingBuildStatus(self):
    """Tests GetLastBuildState if the file doesn't have a valid status."""
    osutils.SafeMakedirs(self.root)
    osutils.WriteFile(self.previous_build_state, '{"build_number": "3"}')
    state = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(state, build_summary.BuildSummary())

  def testGetLastBuildStateGoodFile(self):
    """Tests GetLastBuildState on a good file."""
    osutils.SafeMakedirs(self.root)
    osutils.WriteFile(
        self.previous_build_state,
        '{"build_number": 1, "master_build_id": 3, "status": "pass"}')
    state = cbuildbot_launch.GetLastBuildState(self.root)
    self.assertEqual(
        state,
        build_summary.BuildSummary(
            build_number=1, master_build_id=3, status='pass'))

  def testSetLastBuildState(self):
    """Verifies that SetLastBuildState writes to the expected file."""
    osutils.SafeMakedirs(self.root)
    old_state = build_summary.BuildSummary(
        build_number=314,
        master_build_id=2178,
        status=constants.BUILDER_STATUS_PASSED)
    cbuildbot_launch.SetLastBuildState(self.root, old_state)

    saved_state = osutils.ReadFile(self.previous_build_state)
    new_state = build_summary.BuildSummary()
    new_state.from_json(saved_state)

    self.assertEqual(old_state, new_state)
