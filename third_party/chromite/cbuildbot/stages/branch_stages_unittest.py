# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the branch stages."""

from __future__ import print_function

import os

from chromite.lib import constants
from chromite.cbuildbot import manifest_version
from chromite.cbuildbot import manifest_version_unittest
from chromite.cbuildbot.stages import branch_stages
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import git
from chromite.lib import git_unittest
from chromite.lib import osutils
from chromite.lib import parallel_unittest
from chromite.lib import partial_mock


MANIFEST_CONTENTS = """\
<?xml version="1.0" encoding="UTF-8"?>
<manifest>
  <remote fetch="https://chromium.googlesource.com"
          name="cros"
          review="chromium-review.googlesource.com"/>

  <default remote="cros" revision="refs/heads/master" sync-j="8"/>

  <project groups="minilayout,buildtools"
           name="chromiumos/chromite"
           path="chromite"
           revision="refs/heads/special-branch"/>

  <project name="chromiumos/special"
           path="src/special-new"
           revision="new-special-branch"/>

  <project name="chromiumos/special"
           path="src/special-old"
           revision="old-special-branch" />

  <!-- Test the explicitly specified branching strategy for projects. -->
  <project name="chromiumos/external-explicitly-pinned"
           path="explicit-external"
           revision="refs/heads/master">
    <annotation name="branch-mode" value="pin" />
  </project>

  <project name="chromiumos/external-explicitly-unpinned"
           path="explicit-unpinned"
           revision="refs/heads/master">
    <annotation name="branch-mode" value="tot" />
  </project>

  <project name="chromiumos/external-explicitly-pinned-sha1"
           path="explicit-external-sha1"
           revision="12345">
    <annotation name="branch-mode" value="pin" />
  </project>

  <project name="chromiumos/external-explicitly-unpinned-sha1"
           path="explicit-unpinned-sha1"
           revision="12345">
    <annotation name="branch-mode" value="tot" />
  </project>

  <!-- The next two projects test legacy heristic to determine branching
       strategy for projects -->
  <project name="faraway/external"
           path="external"
           revision="refs/heads/master" />

  <project name="faraway/unpinned"
           path="unpinned"
           revision="refs/heads/master"
           pin="False" />

</manifest>"""

CHROMITE_REVISION = "fb46d34d7cd4b9c167b74f494f2a99b68df50b18"
SPECIAL_REVISION1 = "7bc42f093d644eeaf1c77fab60883881843c3c65"
SPECIAL_REVISION2 = "6270eb3b4f78d9bffec77df50f374f5aae72b370"

VERSIONED_MANIFEST_CONTENTS = """\
<?xml version="1.0" encoding="UTF-8"?>
<manifest revision="fe72f0912776fa4596505e236e39286fb217961b">
  <remote fetch="https://chrome-internal.googlesource.com" name="chrome"/>
  <remote fetch="https://chromium.googlesource.com/" name="chromium"/>
  <remote fetch="https://chromium.googlesource.com" name="cros" \
review="chromium-review.googlesource.com"/>
  <remote fetch="https://chrome-internal.googlesource.com" name="cros-internal" \
review="https://chrome-internal-review.googlesource.com"/>
  <remote fetch="https://special.googlesource.com/" name="special" \
review="https://special.googlesource.com/"/>

  <default remote="cros" revision="refs/heads/master" sync-j="8"/>

  <project name="chromeos/manifest-internal" path="manifest-internal" \
remote="cros-internal" revision="fe72f0912776fa4596505e236e39286fb217961b" \
upstream="refs/heads/master"/>
  <project groups="minilayout,buildtools" name="chromiumos/chromite" \
path="chromite" revision="%(chromite_revision)s" \
upstream="refs/heads/master"/>
  <project name="chromiumos/manifest" path="manifest" \
revision="f24b69176b16bf9153f53883c0cc752df8e07d8b" \
upstream="refs/heads/master"/>
  <project groups="minilayout" name="chromiumos/overlays/chromiumos-overlay" \
path="src/third_party/chromiumos-overlay" \
revision="3ac713c65b5d18585e606a0ee740385c8ec83e44" \
upstream="refs/heads/master"/>
  <project name="chromiumos/special" path="src/special-new" \
revision="%(special_revision1)s" \
upstream="new-special-branch"/>
  <project name="chromiumos/special" path="src/special-old" \
revision="%(special_revision2)s" \
upstream="old-special-branch"/>
</manifest>""" % dict(chromite_revision=CHROMITE_REVISION,
                      special_revision1=SPECIAL_REVISION1,
                      special_revision2=SPECIAL_REVISION2)


class BranchUtilStageTest(generic_stages_unittest.AbstractStageTestCase,
                          cros_test_lib.LoggingTestCase):
  """Tests for branch creation/deletion."""

  BOT_ID = constants.BRANCH_UTIL_CONFIG
  DEFAULT_VERSION = '111.0.0'
  RELEASE_BRANCH_NAME = 'release-test-branch'

  def _CreateVersionFile(self, version=None):
    if version is None:
      version = self.DEFAULT_VERSION
    version_file = os.path.join(self.build_root, constants.VERSION_FILE)
    manifest_version_unittest.VersionInfoTest.WriteFakeVersionFile(
        version_file, version=version)

  def setUp(self):
    """Setup patchers for specified bot id."""
    # Mock out methods as needed.
    self.StartPatcher(parallel_unittest.ParallelMock())
    self.StartPatcher(git_unittest.ManifestCheckoutMock())
    self._CreateVersionFile()
    self.rc_mock = self.StartPatcher(cros_test_lib.RunCommandMock())
    self.rc_mock.SetDefaultCmdResult()

    # We have a versioned manifest (generated by ManifestVersionSyncStage) and
    # the regular, user-maintained manifests.
    manifests = {
        '.repo/manifest.xml': VERSIONED_MANIFEST_CONTENTS,
        'manifest/default.xml': MANIFEST_CONTENTS,
        'manifest-internal/official.xml': MANIFEST_CONTENTS,
    }
    for m_path, m_content in manifests.iteritems():
      full_path = os.path.join(self.build_root, m_path)
      osutils.SafeMakedirs(os.path.dirname(full_path))
      osutils.WriteFile(full_path, m_content)

    self.norm_name = git.NormalizeRef(self.RELEASE_BRANCH_NAME)

  def _Prepare(self, bot_id=None, **kwargs):
    if 'cmd_args' not in kwargs:
      # Fill in cmd_args so we do not use the default, which specifies
      # --branch.  That is incompatible with some branch-util flows.
      kwargs['cmd_args'] = ['-r', self.build_root, self.BOT_ID]
    super(BranchUtilStageTest, self)._Prepare(bot_id, **kwargs)

  def ConstructStage(self):
    return branch_stages.BranchUtilStage(self._run)

  def _VerifyPush(self, new_branch, rename_from=None, delete=False):
    """Verify that |new_branch| has been created.

    Args:
      new_branch: The new remote branch to create (or delete).
      rename_from: If set, |rename_from| is being renamed to |new_branch|.
      delete: If set, |new_branch| is being deleted.
    """
    # Pushes all operate on remote branch refs.
    new_branch = git.NormalizeRef(new_branch)

    # Calculate source and destination revisions.
    suffixes = ['', '-new-special-branch', '-old-special-branch']
    if delete:
      src_revs = [''] * len(suffixes)
    elif rename_from is not None:
      rename_from = git.NormalizeRef(rename_from)
      rename_from_tracking = git.NormalizeRemoteRef('cros', rename_from)
      src_revs = [
          '%s%s' % (rename_from_tracking, suffix) for suffix in suffixes
      ]
    else:
      src_revs = [CHROMITE_REVISION, SPECIAL_REVISION1, SPECIAL_REVISION2]
    dest_revs = ['%s%s' % (new_branch, suffix) for suffix in suffixes]

    # Verify pushes happened correctly.
    for src_rev, dest_rev in zip(src_revs, dest_revs):
      cmd = ['push', '%s:%s' % (src_rev, dest_rev)]
      self.rc_mock.assertCommandContains(cmd)
      if rename_from is not None:
        cmd = ['push', ':%s' % (rename_from,)]
        self.rc_mock.assertCommandContains(cmd)

  def testRelease(self):
    """Run-through of branch creation."""
    self._Prepare(extra_cmd_args=['--buildbot',
                                  '--branch-name', self.RELEASE_BRANCH_NAME,
                                  '--version', self.DEFAULT_VERSION])
    # Simulate branch not existing.
    self.rc_mock.AddCmdResult(
        partial_mock.ListRegex('git show-ref .*%s' % self.RELEASE_BRANCH_NAME),
        returncode=1)
    # SHA1 of HEAD for pinned branches.
    self.rc_mock.AddCmdResult(
        partial_mock.ListRegex('git rev-parse HEAD'),
        output='12345')

    before = manifest_version.VersionInfo.from_repo(self.build_root)
    self.RunStage()
    after = manifest_version.VersionInfo.from_repo(self.build_root)
    # Verify Chrome version was bumped.
    self.assertEquals(int(after.chrome_branch) - int(before.chrome_branch), 1)
    self.assertEquals(int(after.build_number) - int(before.build_number), 1)

    # Verify that manifests were branched properly. Notice that external,
    # explicit-external are pinned to a SHA1, not an actual branch.
    branch_names = {
        'chromite': self.norm_name,
        'external': '12345',
        'explicit-external': '12345',
        'explicit-external-sha1': '12345',
        'src/special-new': self.norm_name + '-new-special-branch',
        'src/special-old': self.norm_name + '-old-special-branch',
        'unpinned': 'refs/heads/master',
        'explicit-unpinned': 'refs/heads/master',
        # If all we had was a sha1, there is not way to even guess what the
        # "master" branch is, so leave it pinned.
        'explicit-unpinned-sha1': '12345',
    }
    # Verify that we correctly transfer branch modes to the branched manifest.
    branch_modes = {
        'explicit-external': 'pin',
        'explicit-external-sha1': 'pin',
        'explicit-unpinned': 'tot',
        'explicit-unpinned-sha1': 'tot',
    }
    for m in ['manifest/default.xml', 'manifest-internal/official.xml']:
      manifest = git.Manifest(os.path.join(self.build_root, m))
      for project_data in manifest.checkouts_by_path.itervalues():
        path = project_data['path']
        branch_name = branch_names[path]
        msg = (
            'Branch name for %s should be %r, but got %r' %
            (path, branch_name, project_data['revision'])
        )
        self.assertEquals(project_data['revision'], branch_name, msg)
        if path in branch_modes:
          self.assertEquals(
              project_data['branch-mode'],
              branch_modes[path],
              'Branch mode for %s should be %r, but got %r' % (
                  path, branch_modes[path], project_data['branch-mode']))

    self._VerifyPush(self.norm_name)

  def testNonRelease(self):
    """Non-release branch creation."""
    self._Prepare(extra_cmd_args=['--buildbot',
                                  '--branch-name', 'refs/heads/test-branch',
                                  '--version', self.DEFAULT_VERSION])
    # Simulate branch not existing.
    self.rc_mock.AddCmdResult(
        partial_mock.ListRegex('git show-ref .*test-branch'),
        returncode=1)

    before = manifest_version.VersionInfo.from_repo(self.build_root)
    # Disable the new branch increment so that
    # IncrementVersionOnDiskForSourceBranch detects we need to bump the version.
    self.PatchObject(branch_stages.BranchUtilStage,
                     '_IncrementVersionOnDiskForNewBranch', autospec=True)
    self.RunStage()
    after = manifest_version.VersionInfo.from_repo(self.build_root)
    # Verify only branch number is bumped.
    self.assertEquals(after.chrome_branch, before.chrome_branch)
    self.assertEquals(int(after.build_number) - int(before.build_number), 1)
    self._VerifyPush(self._run.options.branch_name)

  def testDeletion(self):
    """Branch deletion."""
    self._Prepare(extra_cmd_args=['--buildbot',
                                  '--branch-name', self.RELEASE_BRANCH_NAME,
                                  '--delete-branch'])
    self.rc_mock.AddCmdResult(
        partial_mock.ListRegex('git show-ref .*release-test-branch.*'),
        output='SomeSHA1Value'
    )
    self.RunStage()
    self._VerifyPush(self.norm_name, delete=True)

  def testRename(self):
    """Branch rename."""
    self._Prepare(extra_cmd_args=['--buildbot',
                                  '--branch-name', self.RELEASE_BRANCH_NAME,
                                  '--rename-to', 'refs/heads/release-rename'])
    # Simulate source branch existing and destination branch not existing.
    self.rc_mock.AddCmdResult(
        partial_mock.ListRegex('git show-ref .*%s' % self.RELEASE_BRANCH_NAME),
        output='SomeSHA1Value')
    self.rc_mock.AddCmdResult(
        partial_mock.ListRegex('git show-ref .*release-rename'),
        returncode=1)
    self.RunStage()
    self._VerifyPush(self._run.options.rename_to, rename_from=self.norm_name)

  def testDryRun(self):
    """Verify we don't push to remote when --debug is set."""
    # Simulate branch not existing.
    self.rc_mock.AddCmdResult(
        partial_mock.ListRegex('git show-ref .*%s' % self.RELEASE_BRANCH_NAME),
        returncode=1)

    self._Prepare(extra_cmd_args=['--branch-name', self.RELEASE_BRANCH_NAME,
                                  '--debug',
                                  '--version', self.DEFAULT_VERSION])
    self.RunStage()
    self.rc_mock.assertCommandContains(('push',), expected=False)

  def _DetermineIncrForVersion(self, version):
    version_info = manifest_version.VersionInfo(version)
    stage_cls = branch_stages.BranchUtilStage
    return stage_cls.DetermineBranchIncrParams(version_info)

  def testDetermineIncrBranch(self):
    """Verify branch increment detection."""
    incr_type, _ = self._DetermineIncrForVersion(self.DEFAULT_VERSION)
    self.assertEquals(incr_type, 'branch')

  def testDetermineIncrPatch(self):
    """Verify patch increment detection."""
    incr_type, _ = self._DetermineIncrForVersion('111.1.0')
    self.assertEquals(incr_type, 'patch')

  def testDetermineBranchIncrError(self):
    """Detect unbranchable version."""
    self.assertRaises(branch_stages.BranchError, self._DetermineIncrForVersion,
                      '111.1.1')

  def _SimulateIncrementFailure(self):
    """Simulates a git push failure during source branch increment."""
    self._Prepare(extra_cmd_args=['--buildbot',
                                  '--branch-name', self.RELEASE_BRANCH_NAME,
                                  '--version', self.DEFAULT_VERSION])
    overlay_dir = os.path.join(
        self.build_root, constants.CHROMIUMOS_OVERLAY_DIR)
    self.rc_mock.AddCmdResult(partial_mock.In('push'), returncode=128)
    stage = self.ConstructStage()
    args = (overlay_dir, 'gerrit', 'refs/heads/master')
    # pylint: disable=protected-access
    stage._IncrementVersionOnDiskForSourceBranch(*args)

  def testSourceIncrementWarning(self):
    """Test the warning case for incrementing failure."""
    # Since all git commands are mocked out, the _FetchAndCheckoutTo function
    # does nothing, and leaves the chromeos_version.sh file in the bumped state,
    # so it looks like TOT version was indeed bumped by another bot.
    with cros_test_lib.LoggingCapturer() as logger:
      self._SimulateIncrementFailure()
      self.AssertLogsContain(logger, 'bumped by another')

  def testSourceIncrementFailure(self):
    """Test the failure case for incrementing failure."""
    def FetchAndCheckoutTo(*_args, **_kwargs):
      self._CreateVersionFile()

    # Simulate a git checkout of TOT.
    self.PatchObject(branch_stages.BranchUtilStage, '_FetchAndCheckoutTo',
                     side_effect=FetchAndCheckoutTo, autospec=True)
    self.assertRaises(cros_build_lib.RunCommandError,
                      self._SimulateIncrementFailure)
