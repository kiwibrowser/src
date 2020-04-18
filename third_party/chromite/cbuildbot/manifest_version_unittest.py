# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for manifest_version. Needs to be run inside of chroot."""

from __future__ import print_function

import mock
import os
import tempfile

from chromite.cbuildbot import build_status
from chromite.cbuildbot import manifest_version
from chromite.cbuildbot import repository
from chromite.lib import builder_status_lib
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import timeout_util
from chromite.lib import tree_status


FAKE_VERSION = """
CHROMEOS_BUILD=%(build_number)s
CHROMEOS_BRANCH=%(branch_build_number)s
CHROMEOS_PATCH=%(patch_number)s
CHROME_BRANCH=%(chrome_branch)s
"""

FAKE_WHITELISTED_REMOTES = ('cros', 'chromium')
FAKE_NON_WHITELISTED_REMOTE = 'hottubtimemachine'

FAKE_VERSION_STRING = '1.2.3'
FAKE_VERSION_STRING_NEXT = '1.2.4'
CHROME_BRANCH = '13'

# Use the chromite repo to actually test git changes.
GIT_TEST_PATH = 'chromite'

MOCK_BUILD_ID = 162345


# pylint: disable=protected-access


class HelperMethodsTest(cros_test_lib.TempDirTestCase):
  """Test methods associated with methods not in a class."""

  def testCreateSymlink(self):
    """Tests that we can create symlinks and remove a previous one."""
    srcfile = os.path.join(self.tempdir, 'src')
    osutils.Touch(srcfile)
    other_dir = os.path.join(self.tempdir, 'other_dir')
    os.makedirs(other_dir)
    destfile = os.path.join(other_dir, 'dest')

    manifest_version.CreateSymlink(srcfile, destfile)
    self.assertTrue(os.path.lexists(destfile),
                    'Unable to create symlink to %s' % destfile)


class VersionInfoTest(cros_test_lib.MockTempDirTestCase):
  """Test methods testing methods in VersionInfo class."""

  @classmethod
  def WriteFakeVersionFile(cls, version_file, version=None, chrome_branch=None):
    """Helper method to write a version file from specified version number."""
    if version is None:
      version = FAKE_VERSION_STRING
    if chrome_branch is None:
      chrome_branch = CHROME_BRANCH

    osutils.SafeMakedirs(os.path.split(version_file)[0])
    info = manifest_version.VersionInfo(version, chrome_branch)
    osutils.WriteFile(version_file, FAKE_VERSION % info.__dict__)

  @classmethod
  def CreateFakeVersionFile(cls, tmpdir, version=None, chrome_branch=None):
    """Helper method to create a version file from specified version number."""
    version_file = tempfile.mktemp(dir=tmpdir)
    cls.WriteFakeVersionFile(version_file, version=version,
                             chrome_branch=chrome_branch)
    return version_file

  def testLoadFromFile(self):
    """Tests whether we can load from a version file."""
    version_file = self.CreateFakeVersionFile(self.tempdir)
    info = manifest_version.VersionInfo(version_file=version_file)
    self.assertEqual(info.VersionString(), FAKE_VERSION_STRING)

  def testLoadFromRepo(self):
    """Tests whether we can load from a source repo."""
    version_file = os.path.join(self.tempdir, constants.VERSION_FILE)
    self.WriteFakeVersionFile(version_file)
    info = manifest_version.VersionInfo.from_repo(self.tempdir)
    self.assertEqual(info.VersionString(), FAKE_VERSION_STRING)

  def testLoadFromString(self):
    """Tests whether we can load from a string."""
    info = manifest_version.VersionInfo(FAKE_VERSION_STRING, CHROME_BRANCH)
    self.assertEqual(info.VersionString(), FAKE_VERSION_STRING)

  def CommonTestIncrementVersion(self, incr_type, version, chrome_branch=None):
    """Common test increment.  Returns path to new incremented file."""
    message = 'Incrementing cuz I sed so'
    create_mock = self.PatchObject(git, 'CreateBranch')
    push_mock = self.PatchObject(manifest_version, '_PushGitChanges')
    clean_mock = self.PatchObject(git, 'CleanAndCheckoutUpstream')

    version_file = self.CreateFakeVersionFile(
        self.tempdir, version=version, chrome_branch=chrome_branch)
    info = manifest_version.VersionInfo(version_file=version_file,
                                        incr_type=incr_type)
    info.IncrementVersion()
    info.UpdateVersionFile(message, dry_run=False)

    create_mock.assert_called_once_with(
        self.tempdir, manifest_version.PUSH_BRANCH)
    push_mock.assert_called_once_with(
        self.tempdir, message, dry_run=False, push_to=None)
    clean_mock.assert_called_once_with(self.tempdir)

    return version_file

  def testIncrementVersionPatch(self):
    """Tests whether we can increment a version file by patch number."""
    version_file = self.CommonTestIncrementVersion('branch', '1.2.3')
    new_info = manifest_version.VersionInfo(version_file=version_file,
                                            incr_type='branch')
    self.assertEqual(new_info.VersionString(), '1.2.4')

  def testIncrementVersionBranch(self):
    """Tests whether we can increment a version file by branch number."""
    version_file = self.CommonTestIncrementVersion('branch', '1.2.0')
    new_info = manifest_version.VersionInfo(version_file=version_file,
                                            incr_type='branch')
    self.assertEqual(new_info.VersionString(), '1.3.0')

  def testIncrementVersionBuild(self):
    """Tests whether we can increment a version file by build number."""
    version_file = self.CommonTestIncrementVersion('build', '1.0.0')
    new_info = manifest_version.VersionInfo(version_file=version_file,
                                            incr_type='build')
    self.assertEqual(new_info.VersionString(), '2.0.0')

  def testIncrementVersionChrome(self):
    """Tests whether we can increment the chrome version."""
    version_file = self.CommonTestIncrementVersion(
        'chrome_branch', version='1.0.0', chrome_branch='29')
    new_info = manifest_version.VersionInfo(version_file=version_file)
    self.assertEqual(new_info.VersionString(), '2.0.0')
    self.assertEqual(new_info.chrome_branch, '30')


class BuildSpecsManagerTest(cros_test_lib.MockTempDirTestCase):
  """Tests for the BuildSpecs manager."""

  def setUp(self):
    os.makedirs(os.path.join(self.tempdir, '.repo'))
    self.source_repo = 'ssh://source/repo'
    self.manifest_repo = 'ssh://manifest/repo'
    self.version_file = 'version-file.sh'
    self.branch = 'master'
    self.build_names = ['amd64-generic-paladin']
    self.incr_type = 'branch'
    # Change default to something we clean up.
    self.tmpmandir = os.path.join(self.tempdir, 'man')
    osutils.SafeMakedirs(self.tmpmandir)
    self.manager = None

    self.PatchObject(builder_status_lib.SlaveBuilderStatus,
                     '_InitSlaveInfo')

    self.db = mock.Mock()
    self.buildbucket_client_mock = mock.Mock()

    self.PatchObject(tree_status, 'GetExperimentalBuilders', return_value=[])

  def BuildManager(self, config=None, metadata=None, db=None,
                   buildbucket_client=None):
    db = db or self.db
    repo = repository.RepoRepository(
        self.source_repo, self.tempdir, self.branch)
    manager = manifest_version.BuildSpecsManager(
        repo, self.manifest_repo, self.build_names, self.incr_type, False,
        branch=self.branch, dry_run=True, config=config, metadata=metadata,
        db=db, buildbucket_client=buildbucket_client)
    manager.manifest_dir = self.tmpmandir
    # Shorten the sleep between attempts.
    manager.SLEEP_TIMEOUT = 1

    return manager

  def testPublishManifestCommitMessageWithBuildId(self):
    """Tests that PublishManifest writes a build id."""
    self.manager = self.BuildManager()
    expected_message = ('Automatic: Start amd64-generic-paladin master 1\n'
                        'CrOS-Build-Id: %s' % MOCK_BUILD_ID)
    push_mock = self.PatchObject(self.manager, 'PushSpecChanges')

    info = manifest_version.VersionInfo(
        FAKE_VERSION_STRING, CHROME_BRANCH, incr_type='branch')

    # Create a fake manifest file.
    m = os.path.join(self.tmpmandir, '1.xml')
    osutils.Touch(m)
    self.manager.InitializeManifestVariables(info)

    self.manager.PublishManifest(m, '1', build_id=MOCK_BUILD_ID)

    push_mock.assert_called_once_with(expected_message)

  def testPublishManifestCommitMessageWithNegativeBuildId(self):
    """Tests that PublishManifest doesn't write a negative build_id"""
    self.manager = self.BuildManager()
    expected_message = 'Automatic: Start amd64-generic-paladin master 1'
    push_mock = self.PatchObject(self.manager, 'PushSpecChanges')

    info = manifest_version.VersionInfo(
        FAKE_VERSION_STRING, CHROME_BRANCH, incr_type='branch')

    # Create a fake manifest file.
    m = os.path.join(self.tmpmandir, '1.xml')
    osutils.Touch(m)
    self.manager.InitializeManifestVariables(info)

    self.manager.PublishManifest(m, '1', build_id=-1)

    push_mock.assert_called_once_with(expected_message)

  def testPublishManifestCommitMessageWithNoneBuildId(self):
    """Tests that PublishManifest doesn't write a non-existant build_id"""
    self.manager = self.BuildManager()
    expected_message = 'Automatic: Start amd64-generic-paladin master 1'
    push_mock = self.PatchObject(self.manager, 'PushSpecChanges')

    info = manifest_version.VersionInfo(
        FAKE_VERSION_STRING, CHROME_BRANCH, incr_type='branch')

    # Create a fake manifest file.
    m = os.path.join(self.tmpmandir, '1.xml')
    osutils.Touch(m)
    self.manager.InitializeManifestVariables(info)

    self.manager.PublishManifest(m, '1')

    push_mock.assert_called_once_with(expected_message)

  def _buildManifest(self):
    mpath = os.path.join(self.manager.manifest_dir, 'buildspecs', CHROME_BRANCH)
    manifest_paths = [os.path.join(mpath, '1.2.%d.xml' % x)
                      for x in [2, 3, 4, 5]]
    # Create fake buildspecs.
    osutils.SafeMakedirs(os.path.join(mpath))
    for m in manifest_paths:
      osutils.Touch(m)

    return manifest_paths

  def testInitializeManifestVariablesWithUnprocessedBuild(self):
    """Test InitializeManifestVariables with unprocessed build."""
    self.manager = self.BuildManager()
    info = manifest_version.VersionInfo(
        FAKE_VERSION_STRING, CHROME_BRANCH, incr_type='branch')
    for_build = os.path.join(self.manager.manifest_dir, 'build-name',
                             self.build_names[0])

    m1, m2, _, _ = self._buildManifest()
    # Fail 1, pass 2, leave 3,4 unprocessed.
    manifest_version.CreateSymlink(m1, os.path.join(
        for_build, 'fail', CHROME_BRANCH, os.path.basename(m1)))
    manifest_version.CreateSymlink(m1, os.path.join(
        for_build, 'pass', CHROME_BRANCH, os.path.basename(m2)))

    self.manager.db.GetBuildHistory.return_value = None
    self.manager.InitializeManifestVariables(info)
    self.assertEqual(self.manager.latest_unprocessed, '1.2.5')
    self.assertIsNone(self.manager._latest_build)

  def testInitializeManifestVariablesWithPassedBuild(self):
    """Test InitializeManifestVariables with passed build."""
    self.manager = self.BuildManager()
    info = manifest_version.VersionInfo(
        FAKE_VERSION_STRING, CHROME_BRANCH, incr_type='branch')
    for_build = os.path.join(self.manager.manifest_dir, 'build-name',
                             self.build_names[0])

    m1, m2, m3, m4 = self._buildManifest()
    # Fail 1, pass 2, pass 3, pass 4
    manifest_version.CreateSymlink(m1, os.path.join(
        for_build, 'fail', CHROME_BRANCH, os.path.basename(m1)))
    for m in [m2, m3, m4]:
      manifest_version.CreateSymlink(m, os.path.join(
          for_build, 'pass', CHROME_BRANCH, os.path.basename(m)))

    latest_builds = [{'build_config': self.build_names[0],
                      'status':'pass',
                      'platform_version':'1.2.5'}]
    self.manager.db.GetBuildHistory.return_value = latest_builds
    self.manager.InitializeManifestVariables(info)
    self.assertIsNone(self.manager.latest_unprocessed)
    self.assertEqual(self.manager._latest_build, latest_builds[0])

  def testLatestSpecFromDir(self):
    """Tests whether we can get sorted specs correctly from a directory."""
    self.manager = self.BuildManager()
    self.PatchObject(repository, 'CloneGitRepo', side_effect=Exception())
    info = manifest_version.VersionInfo(
        '99.1.2', CHROME_BRANCH, incr_type='branch')

    specs_dir = os.path.join(self.manager.manifest_dir, 'buildspecs',
                             CHROME_BRANCH)
    m1, m2, m3, m4 = [os.path.join(specs_dir, x)
                      for x in ['100.0.0.xml', '99.3.3.xml', '99.1.10.xml',
                                '99.1.5.xml']]

    # Create fake buildspecs.
    osutils.SafeMakedirs(specs_dir)
    for m in [m1, m2, m3, m4]:
      osutils.Touch(m)

    spec = self.manager._LatestSpecFromDir(info, specs_dir)
    # Should be the latest on the 99.1 branch.
    self.assertEqual(spec, '99.1.10')

  def testGetNextVersionNoIncrement(self):
    """Tests whether we can get the next version to be built correctly.

    Tests without pre-existing version in manifest dir.
    """
    self.manager = self.BuildManager()
    info = manifest_version.VersionInfo(
        FAKE_VERSION_STRING, CHROME_BRANCH, incr_type='branch')

    self.manager.latest = None
    version = self.manager.GetNextVersion(info)
    self.assertEqual(FAKE_VERSION_STRING, version)

  def testGetNextVersionIncrement(self):
    """Tests that we create a new version if a previous one exists."""
    self.manager = self.BuildManager()
    m = self.PatchObject(manifest_version.VersionInfo, 'UpdateVersionFile')
    version_file = VersionInfoTest.CreateFakeVersionFile(self.tempdir)
    info = manifest_version.VersionInfo(version_file=version_file,
                                        incr_type='branch')

    self.manager.latest = FAKE_VERSION_STRING
    version = self.manager.GetNextVersion(info)
    self.assertEqual(FAKE_VERSION_STRING_NEXT, version)
    m.assert_called_once_with(
        'Automatic: %s - Updating to a new version number from %s' % (
            self.build_names[0], FAKE_VERSION_STRING), dry_run=True)

  def testGetNextBuildSpec(self):
    """End-to-end test of updating the manifest."""
    self.manager = self.BuildManager()
    my_info = manifest_version.VersionInfo('1.2.3', chrome_branch='4')
    self.PatchObject(manifest_version.BuildSpecsManager,
                     'GetCurrentVersionInfo', return_value=my_info)
    self.PatchObject(repository.RepoRepository, 'Sync')
    self.PatchObject(repository.RepoRepository, 'ExportManifest',
                     return_value='<manifest />')
    rc = self.StartPatcher(cros_test_lib.RunCommandMock())
    rc.SetDefaultCmdResult()

    self.manager.GetNextBuildSpec(retries=0)
    self.manager.UpdateStatus({self.build_names[0]: True})

  def testDidLastBuildFailReturnsFalse(self):
    """Test DidLastBuildFail returns False."""
    self.manager = self.BuildManager()
    self.assertFalse(self.manager.DidLastBuildFail())

  # pylint: disable=attribute-defined-outside-init
  def testDidLastBuildFailReturnsTrue(self):
    """Test DidLastBuildFailReturns True."""
    self.manager = self.BuildManager()
    self._latest_build = {'build_config': self.build_names[0],
                          'status':'fail',
                          'platform_version':'1.2.5'}
    self.assertFalse(self.manager.DidLastBuildFail())

  def testWaitForSlavesToCompleteWithEmptyBuildersArray(self):
    """Test WaitForSlavesToComplete with an empty builders_array."""
    self.manager = self.BuildManager()
    self.manager.WaitForSlavesToComplete(1, self.db, [])

  def testWaitForSlavesToComplete(self):
    """Test WaitForSlavesToComplete."""
    self.PatchObject(build_status.SlaveStatus, 'UpdateSlaveStatus')
    self.PatchObject(build_status.SlaveStatus, 'ShouldWait', return_value=False)
    self.manager = self.BuildManager()
    self.manager.WaitForSlavesToComplete(1, self.db, ['build_1', 'build_2'])

  def testWaitForSlavesToCompleteWithTimeout(self):
    """Test WaitForSlavesToComplete raises timeout."""
    self.PatchObject(build_status.SlaveStatus, 'UpdateSlaveStatus')
    self.PatchObject(build_status.SlaveStatus, 'ShouldWait', return_value=True)
    self.manager = self.BuildManager()
    self.assertRaises(
        timeout_util.TimeoutError,
        self.manager.WaitForSlavesToComplete,
        1, self.db, ['build_1', 'build_2'], timeout=1,
        ignore_timeout_exception=False)
