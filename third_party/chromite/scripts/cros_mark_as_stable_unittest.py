# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for cros_mark_as_stable.py."""

from __future__ import print_function

import mock
import os

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import parallel_unittest
from chromite.lib import partial_mock
from chromite.lib import portage_util
from chromite.scripts import cros_mark_as_stable


class RunGitMock(partial_mock.PartialCmdMock):
  """Partial mock for git.RunMock."""
  TARGET = 'chromite.lib.git'
  ATTRS = ('RunGit',)
  DEFAULT_ATTR = 'RunGit'

  def RunGit(self, _git_repo, cmd, _retry=True, **kwargs):
    return self._results['RunGit'].LookupResult(
        (cmd,), hook_args=(cmd,), hook_kwargs=kwargs)


class NonClassTests(cros_test_lib.MockTestCase):
  """Test the flow for pushing a change."""

  def setUp(self):
    self._branch = 'test_branch'
    self._target_manifest_branch = 'cros/master'

  def _TestPushChange(self, bad_cls):
    side_effect = Exception('unittest says this should not be called')

    git_log = 'Marking test_one as stable\nMarking test_two as stable\n'
    fake_description = 'Marking set of ebuilds as stable\n\n%s' % git_log
    self.PatchObject(git, 'DoesCommitExistInRepo', return_value=True)
    self.PatchObject(cros_mark_as_stable, '_DoWeHaveLocalCommits',
                     return_value=True)
    self.PatchObject(cros_mark_as_stable.GitBranch, 'CreateBranch',
                     side_effect=side_effect)
    self.PatchObject(cros_mark_as_stable.GitBranch, 'Exists',
                     side_effect=side_effect)

    push_mock = self.PatchObject(git, 'PushBranch')
    self.PatchObject(
        git, 'GetTrackingBranch',
        return_value=git.RemoteRef('gerrit', 'refs/remotes/gerrit/master'))
    sync_mock = self.PatchObject(git, 'SyncPushBranch')
    create_mock = self.PatchObject(git, 'CreatePushBranch')
    git_mock = self.StartPatcher(RunGitMock())

    git_mock.AddCmdResult(['checkout', self._branch])

    cmd = ['log', '--format=short', '--perl-regexp', '--author',
           '^(?!chrome-bot)', 'refs/remotes/gerrit/master..%s' % self._branch]

    if bad_cls:
      push_mock.side_effect = side_effect
      create_mock.side_effect = side_effect
      git_mock.AddCmdResult(cmd, output='Found bad stuff')
    else:
      git_mock.AddCmdResult(cmd, output='\n')
      cmd = ['log', '--format=format:%s%n%n%b',
             'refs/remotes/gerrit/master..%s' % self._branch]
      git_mock.AddCmdResult(cmd, output=git_log)
      git_mock.AddCmdResult(['merge', '--squash', self._branch])
      git_mock.AddCmdResult(['commit', '-m', fake_description])
      git_mock.AddCmdResult(['config', 'push.default', 'tracking'])

    cros_mark_as_stable.PushChange(self._branch, self._target_manifest_branch,
                                   False, '.')
    sync_mock.assert_called_with('.', 'gerrit', 'refs/remotes/gerrit/master')
    if not bad_cls:
      push_mock.assert_called_with('merge_branch', '.', dryrun=False,
                                   staging_branch=None)
      create_mock.assert_called_with('merge_branch', '.',
                                     remote_push_branch=mock.ANY)

  def testPushChange(self):
    """Verify pushing changes works."""
    self._TestPushChange(bad_cls=False)

  def testPushChangeBadCls(self):
    """Verify we do not push bad CLs."""
    self.assertRaises(AssertionError, self._TestPushChange, bad_cls=True)


class EbuildMock(object):
  """Mock portage_util.Ebuild."""

  def __init__(self, path, new_package=True):
    self.path = path
    self.package = '%s_package' % path
    self.cros_workon_vars = 'cros_workon_vars'
    self._new_package = new_package

  # pylint: disable=unused-argument
  def RevWorkOnEBuild(self, srcroot, manifest, redirect_file=None):
    if self._new_package:
      return ('%s_new_package' % self.path,
              '%s_new_ebuild' % self.path,
              '%s_old_ebuild' % self.path)


# pylint: disable=protected-access
class MarkAsStableCMDTest(cros_test_lib.MockTempDirTestCase):
  """Test cros_mark_as_stable commands."""

  def setUp(self):
    self._manifest = 'manifest'
    self._parser = cros_mark_as_stable.GetParser()
    self._package_list = ['pkg1']

    self._overlays = [os.path.join(self.tempdir, 'overlay_%s' % i)
                      for i in range(0, 3)]

    self._overlay_remote_ref = {
        self._overlays[0]: git.RemoteRef('remote', 'ref', 'project_1'),
        self._overlays[1]: git.RemoteRef('remote', 'ref', 'project_1'),
        self._overlays[2]: git.RemoteRef('remote', 'ref', 'project_2'),
    }

    self._git_project_overlays = {}
    self._overlay_tracking_branch = {}
    for overlay in self._overlays:
      self._git_project_overlays.setdefault(
          self._overlay_remote_ref[overlay], []).append(overlay)
      self._overlay_tracking_branch[overlay] = (
          self._overlay_remote_ref[overlay].ref)

    self.PatchObject(git, 'GetTrackingBranchViaManifest')
    self._commit_options = self._parser.parse_args(['commit'])
    self._push_options = self._parser.parse_args(['push'])

  def testWorkOnPush(self):
    """Test _WorkOnPush."""
    self.PatchObject(parallel, 'RunTasksInProcessPool')

    cros_mark_as_stable._WorkOnPush(
        self._push_options, self._overlay_tracking_branch,
        self._git_project_overlays)

  def testPushOverlays(self):
    """Test _PushOverlays."""
    self.PatchObject(os.path, 'isdir', return_value=True)
    mock_push_change = self.PatchObject(cros_mark_as_stable, 'PushChange')

    cros_mark_as_stable._PushOverlays(
        self._push_options, self._overlays, self._overlay_tracking_branch)
    self.assertEqual(mock_push_change.call_count, 3)

  def testWorkOnCommit(self):
    """Test _WorkOnCommit."""
    self.PatchObject(parallel, 'RunTasksInProcessPool')
    self.PatchObject(cros_mark_as_stable, '_CommitOverlays')
    self.PatchObject(cros_mark_as_stable, '_GetOverlayToEbuildsMap',
                     return_value={})

    cros_mark_as_stable._WorkOnCommit(
        self._commit_options, self._overlays, self._overlay_tracking_branch,
        self._git_project_overlays, self._manifest, self._package_list)

  def testGetOverlayToEbuildsMap(self):
    """Test _GetOverlayToEbuildsMap."""
    self.PatchObject(portage_util, 'GetOverlayEBuilds', return_value=['ebuild'])

    expected_overlay_dicts = {
        overlay : ['ebuild'] for overlay in self._overlays}
    overlay_ebuilds = cros_mark_as_stable._GetOverlayToEbuildsMap(
        self._commit_options, self._overlays, self._package_list)
    self.assertItemsEqual(expected_overlay_dicts, overlay_ebuilds)

  def testCommitOverlays(self):
    """Test _CommitOverlays."""
    mock_run_process_pool = self.PatchObject(parallel, 'RunTasksInProcessPool')
    self.PatchObject(os.path, 'isdir', return_value=True)
    self.PatchObject(git, 'RunGit')
    self.PatchObject(cros_mark_as_stable, '_WorkOnEbuild', return_value=None)
    self.PatchObject(git, 'GetGitRepoRevision')
    self.PatchObject(cros_mark_as_stable.GitBranch, 'CreateBranch')
    self.PatchObject(cros_mark_as_stable.GitBranch, 'Exists', return_value=True)
    self.PatchObject(portage_util.EBuild, 'CommitChange')

    overlay_ebuilds = {
        self._overlays[0]: ['ebuild_1_1', 'ebuild_1_2'],
        self._overlays[1]: ['ebuild_2_1'],
        self._overlays[2]: ['ebuild_3_1', 'ebuild_3_2'],
    }

    cros_mark_as_stable._CommitOverlays(
        self._commit_options, self._manifest, self._overlays,
        self._overlay_tracking_branch, overlay_ebuilds, list(), list())
    self.assertEqual(3, mock_run_process_pool.call_count)

  def testWorkOnEbuildWithNewPackage(self):
    """Test _WorkOnEbuild with new packages."""
    overlay = self._overlays[0]
    ebuild = EbuildMock('ebuild')

    with parallel.Manager() as manager:
      revved_packages = manager.list()
      new_package_atoms = manager.list()

      messages = manager.list()
      ebuild_paths_to_add = manager.list()
      ebuild_paths_to_remove = manager.list()

      cros_mark_as_stable._WorkOnEbuild(
          overlay, ebuild, self._manifest, self._commit_options,
          ebuild_paths_to_add, ebuild_paths_to_remove,
          messages, revved_packages, new_package_atoms)
      self.assertItemsEqual(ebuild_paths_to_add, ['ebuild_new_ebuild'])
      self.assertItemsEqual(ebuild_paths_to_remove, ['ebuild_old_ebuild'])
      self.assertItemsEqual(messages,
                            [cros_mark_as_stable._GIT_COMMIT_MESSAGE %
                             'ebuild_package'])
      self.assertItemsEqual(revved_packages, ['ebuild_package'])
      self.assertItemsEqual(new_package_atoms, ['=ebuild_new_package'])

  def testWorkOnEbuildWithoutNewPackage(self):
    """Test _WorkOnEbuild without new packages."""
    ebuild = EbuildMock('ebuild', new_package=False)
    overlay = self._overlays[0]

    with parallel.Manager() as manager:
      revved_packages = manager.list()
      new_package_atoms = manager.list()

      messages = manager.list()
      ebuild_paths_to_add = manager.list()
      ebuild_paths_to_remove = manager.list()

      cros_mark_as_stable._WorkOnEbuild(
          overlay, ebuild, self._manifest, self._commit_options,
          ebuild_paths_to_add, ebuild_paths_to_remove, messages,
          revved_packages, new_package_atoms)
      self.assertEqual(list(ebuild_paths_to_add), [])
      self.assertEqual(list(ebuild_paths_to_remove), [])
      self.assertEqual(list(messages), [])
      self.assertEqual(list(revved_packages), [])
      self.assertEqual(list(new_package_atoms), [])


class MainTests(cros_test_lib.RunCommandTestCase,
                cros_test_lib.MockTempDirTestCase):
  """Tests for cros_mark_as_stable.main()."""

  def setUp(self):
    self.PatchObject(git.ManifestCheckout, 'Cached', return_value='manifest')
    self.mock_work_on_push = self.PatchObject(
        cros_mark_as_stable, '_WorkOnPush')
    self.mock_work_on_commit = self.PatchObject(
        cros_mark_as_stable, '_WorkOnCommit')

    self._overlays = []
    remote_refs = []
    self._overlay_tracking_branch = {}
    self._git_project_overlays = {}
    for i in range(0, 3):
      overlay = os.path.join(self.tempdir, 'overlay_%s' % i)
      osutils.SafeMakedirs(overlay)
      self._overlays.append(overlay)

      remote_ref = git.RemoteRef('remote', 'ref', 'project_%s' % i)
      remote_refs.append(remote_ref)

      self._overlay_tracking_branch[overlay] = remote_ref.ref
      self._git_project_overlays[remote_ref.project_name] = [overlay]

    self.PatchObject(git, 'GetTrackingBranchViaManifest',
                     side_effect=remote_refs)

  def testMainWithCommit(self):
    """Test Main with Commit options."""
    cros_mark_as_stable.main(
        ['commit', '--all', '--overlays', ':'.join(self._overlays)])
    self.mock_work_on_commit.assert_called_once_with(
        mock.ANY, self._overlays, self._overlay_tracking_branch,
        self._git_project_overlays, 'manifest', None)

  def testMainWithPush(self):
    """Test Main with Push options."""
    cros_mark_as_stable.main(
        ['push', '--all', '--overlays', ':'.join(self._overlays)])
    self.mock_work_on_push.assert_called_once_with(
        mock.ANY, self._overlay_tracking_branch, self._git_project_overlays)


class CleanStalePackagesTest(cros_test_lib.RunCommandTestCase):
  """Tests for cros_mark_as_stable.CleanStalePackages."""

  def setUp(self):
    self.PatchObject(osutils, 'FindMissingBinaries', return_value=[])

  def testNormalClean(self):
    """Clean up boards/packages with normal success"""
    cros_mark_as_stable.CleanStalePackages('.', ('board1', 'board2'),
                                           ['cow', 'car'])

  def testNothingToUnmerge(self):
    """Clean up packages that don't exist (portage will exit 1)"""
    self.rc.AddCmdResult(partial_mock.In('emerge'), returncode=1)
    cros_mark_as_stable.CleanStalePackages('.', (), ['no/pkg'])

  def testUnmergeError(self):
    """Make sure random exit errors are not ignored"""
    self.rc.AddCmdResult(partial_mock.In('emerge'), returncode=123)
    with parallel_unittest.ParallelMock():
      with self.assertRaises(cros_build_lib.RunCommandError):
        cros_mark_as_stable.CleanStalePackages('.', (), ['no/pkg'])


class GitBranchTest(cros_test_lib.MockTestCase):
  """Tests for cros_mark_as_stable.GitBranch."""

  def setUp(self):
    # Always stub RunCommmand out as we use it in every method.
    self.rc_mock = self.PatchObject(cros_build_lib, 'RunCommand')

    self._branch_name = 'test_branch'
    self._target_manifest_branch = 'cros/test'
    self._branch = cros_mark_as_stable.GitBranch(
        branch_name=self._branch_name,
        tracking_branch=self._target_manifest_branch,
        cwd='.')

  def testCheckoutCreate(self):
    """Test init with no previous branch existing."""
    self.PatchObject(self._branch, 'Exists', return_value=False)
    cros_mark_as_stable.GitBranch.Checkout(self._branch)
    self.rc_mock.assert_called_with(
        ['repo', 'start', self._branch_name, '.'],
        print_cmd=False, cwd='.', capture_output=True)

  def testCheckoutNoCreate(self):
    """Test init with previous branch existing."""
    self.PatchObject(self._branch, 'Exists', return_value=True)
    cros_mark_as_stable.GitBranch.Checkout(self._branch)
    self.rc_mock.assert_called_with(
        ['git', 'checkout', '-f', self._branch_name],
        print_cmd=False, cwd='.', capture_output=True)

  def testExists(self):
    """Test if branch exists that is created."""
    result = cros_build_lib.CommandResult(output=self._branch_name + '\n')
    self.PatchObject(git, 'RunGit', return_value=result)
    self.assertTrue(self._branch.Exists())
