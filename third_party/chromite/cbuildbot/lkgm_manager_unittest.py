# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for lkgm_manager"""

from __future__ import print_function

import contextlib
import mock
import os
import tempfile
from xml.dom import minidom

from chromite.cbuildbot import lkgm_manager
from chromite.cbuildbot import manifest_version
from chromite.cbuildbot import repository
from chromite.cbuildbot import validation_pool
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import cros_test_lib
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import patch as cros_patch

site_config = config_lib.GetConfig()


FAKE_VERSION_STRING = '1.2.4-rc3'
FAKE_VERSION_STRING_NEXT = '1.2.4-rc4'
CHROME_BRANCH = '13'

FAKE_VERSION = """
CHROMEOS_BUILD=1
CHROMEOS_BRANCH=2
CHROMEOS_PATCH=4
CHROME_BRANCH=13
"""


# pylint: disable=protected-access


class LKGMCandidateInfoTest(cros_test_lib.TestCase):
  """Test methods testing methods in _LKGMCandidateInfo class."""

  def testLoadFromString(self):
    """Tests whether we can load from a string."""
    info = lkgm_manager._LKGMCandidateInfo(version_string=FAKE_VERSION_STRING,
                                           chrome_branch=CHROME_BRANCH)
    self.assertEqual(info.VersionString(), FAKE_VERSION_STRING)

  def testIncrementVersionPatch(self):
    """Tests whether we can increment a lkgm info."""
    info = lkgm_manager._LKGMCandidateInfo(version_string=FAKE_VERSION_STRING,
                                           chrome_branch=CHROME_BRANCH)
    info.IncrementVersion()
    self.assertEqual(info.VersionString(), FAKE_VERSION_STRING_NEXT)

  def testVersionCompare(self):
    """Tests whether our comparision method works."""
    info0 = lkgm_manager._LKGMCandidateInfo('5.2.3-rc100')
    info1 = lkgm_manager._LKGMCandidateInfo('1.2.3-rc1')
    info2 = lkgm_manager._LKGMCandidateInfo('1.2.3-rc2')
    info3 = lkgm_manager._LKGMCandidateInfo('1.2.200-rc1')
    info4 = lkgm_manager._LKGMCandidateInfo('1.4.3-rc1')

    self.assertGreater(info0, info1)
    self.assertGreater(info0, info2)
    self.assertGreater(info0, info3)
    self.assertGreater(info0, info4)
    self.assertGreater(info2, info1)
    self.assertGreater(info3, info1)
    self.assertGreater(info3, info2)
    self.assertGreater(info4, info1)
    self.assertGreater(info4, info2)
    self.assertGreater(info4, info3)
    self.assertEqual(info0, info0)
    self.assertEqual(info1, info1)
    self.assertEqual(info2, info2)
    self.assertEqual(info3, info3)
    self.assertEqual(info4, info4)
    self.assertNotEqual(info0, info1)
    self.assertNotEqual(info0, info2)
    self.assertNotEqual(info0, info3)
    self.assertNotEqual(info0, info4)
    self.assertNotEqual(info1, info0)
    self.assertNotEqual(info1, info2)
    self.assertNotEqual(info1, info3)
    self.assertNotEqual(info1, info4)
    self.assertNotEqual(info2, info0)
    self.assertNotEqual(info2, info1)
    self.assertNotEqual(info2, info3)
    self.assertNotEqual(info2, info4)
    self.assertNotEqual(info3, info0)
    self.assertNotEqual(info3, info1)
    self.assertNotEqual(info3, info2)
    self.assertNotEqual(info3, info4)
    self.assertNotEqual(info4, info0)
    self.assertNotEqual(info4, info1)
    self.assertNotEqual(info4, info1)
    self.assertNotEqual(info4, info3)


@contextlib.contextmanager
def TemporaryManifest():
  with tempfile.NamedTemporaryFile() as f:
    # Create fake but empty manifest file.
    new_doc = minidom.getDOMImplementation().createDocument(
        None, 'manifest', None)
    print(new_doc.toxml())
    new_doc.writexml(f)
    f.flush()
    yield f


class LKGMManagerTest(cros_test_lib.MockTempDirTestCase):
  """Tests for the BuildSpecs manager."""

  def setUp(self):
    self.push_mock = self.PatchObject(git, 'CreatePushBranch')

    self.source_repo = 'ssh://source/repo'
    self.manifest_repo = 'ssh://manifest/repo'
    self.version_file = 'version-file.sh'
    self.branch = 'master'
    self.build_name = 'amd64-generic'
    self.incr_type = 'branch'

    # Create tmp subdirs based on the one provided TempDirMixin.
    self.tmpdir = os.path.join(self.tempdir, "base")
    osutils.SafeMakedirs(self.tmpdir)
    self.tmpmandir = os.path.join(self.tempdir, "man")
    osutils.SafeMakedirs(self.tmpmandir)

    repo = repository.RepoRepository(
        self.source_repo, self.tmpdir, self.branch, depth=1)
    self.manager = lkgm_manager.LKGMManager(
        repo, self.manifest_repo, self.build_name, constants.PFQ_TYPE, 'branch',
        force=False, branch=self.branch, dry_run=True)
    self.manager.manifest_dir = self.tmpmandir
    self.manager.lkgm_path = os.path.join(
        self.tmpmandir, constants.LKGM_MANIFEST)

    self.manager.all_specs_dir = '/LKGM/path'
    manifest_dir = self.manager.manifest_dir
    self.manager.specs_for_builder = os.path.join(manifest_dir,
                                                  self.manager.rel_working_dir,
                                                  'build-name', '%(builder)s')
    self.manager.SLEEP_TIMEOUT = 0

  def _GetPathToManifest(self, info):
    return os.path.join(self.manager.all_specs_dir, '%s.xml' %
                        info.VersionString())

  def testCreateNewCandidate(self):
    """Tests that we can create a new candidate and uprev an old rc."""
    # Let's stub out other LKGMManager calls cause they're already
    # unit tested.

    my_info = lkgm_manager._LKGMCandidateInfo('1.2.3')
    most_recent_candidate = lkgm_manager._LKGMCandidateInfo('1.2.3-rc12')
    self.manager.latest = most_recent_candidate.VersionString()

    new_candidate = lkgm_manager._LKGMCandidateInfo('1.2.3-rc13')
    new_manifest = 'some_manifest'

    build_id = 59271

    # Patch out our RepoRepository to make sure we don't corrupt real repo.
    cros_source_mock = self.PatchObject(self.manager, 'cros_source')
    cros_source_mock.branch = 'master'
    cros_source_mock.directory = '/foo/repo'

    self.PatchObject(lkgm_manager.LKGMManager, 'CheckoutSourceCode')
    self.PatchObject(lkgm_manager.LKGMManager, 'CreateManifest',
                     return_value=new_manifest)
    self.PatchObject(lkgm_manager.LKGMManager, 'HasCheckoutBeenBuilt',
                     return_value=False)

    # Do manifest refresh work.
    self.PatchObject(lkgm_manager.LKGMManager, 'RefreshManifestCheckout')
    self.PatchObject(lkgm_manager.LKGMManager, 'GetCurrentVersionInfo',
                     return_value=my_info)
    init_mock = self.PatchObject(lkgm_manager.LKGMManager,
                                 'InitializeManifestVariables')

    self.PatchObject(lkgm_manager.LKGMManager,
                     'GenerateBlameListSinceLKGM',
                     return_value=True)
    self.PatchObject(lkgm_manager.LKGMManager,
                     '_AdjustRepoCheckoutToLocalManifest')
    self.PatchObject(lkgm_manager.LKGMManager,
                     '_AddPatchesToManifest')
    mock_pool = self.PatchObject(
        validation_pool, 'ValidationPool',
        build_root=self.manager.cros_source.directory,
        has_chump_cls=False)

    # For _AdjustRepoCheckoutToLocalManifest.
    self.PatchObject(repository, 'CloneGitRepo')
    self.PatchObject(git, 'CreateBranch')

    # Publish new candidate.
    publish_mock = self.PatchObject(lkgm_manager.LKGMManager, 'PublishManifest')

    candidate_path = self.manager.CreateNewCandidate(
        build_id=build_id, validation_pool=mock_pool)
    self.assertEqual(candidate_path, self._GetPathToManifest(new_candidate))

    publish_mock.assert_called_once_with(new_manifest,
                                         new_candidate.VersionString(),
                                         build_id=build_id)
    init_mock.assert_called_once_with(my_info)
    self.push_mock.assert_called_once_with(mock.ANY, mock.ANY, sync=False)
    self.assertTrue(mock_pool.has_chump_cls)

  def testCreateFromManifest(self):
    """Tests that we can create a new candidate from another manifest."""
    # Let's stub out other LKGMManager calls cause they're already
    # unit tested.

    version = '2010.0.0-rc7'
    my_info = lkgm_manager._LKGMCandidateInfo('2010.0.0')
    new_candidate = lkgm_manager._LKGMCandidateInfo(version)
    manifest = ('/tmp/manifest-versions-internal/paladin/buildspecs/'
                '20/%s.xml' % version)
    new_manifest = '/path/to/tmp/file.xml'

    build_id = 20162

    # Patch out our RepoRepository to make sure we don't corrupt real repo.
    self.PatchObject(self.manager, 'cros_source')
    filter_mock = self.PatchObject(manifest_version, 'FilterManifest',
                                   return_value=new_manifest)

    # Do manifest refresh work.
    self.PatchObject(lkgm_manager.LKGMManager, 'GetCurrentVersionInfo',
                     return_value=my_info)
    self.PatchObject(lkgm_manager.LKGMManager, 'RefreshManifestCheckout')
    init_mock = self.PatchObject(lkgm_manager.LKGMManager,
                                 'InitializeManifestVariables')

    # Publish new candidate.
    publish_mock = self.PatchObject(lkgm_manager.LKGMManager, 'PublishManifest')

    candidate_path = self.manager.CreateFromManifest(manifest,
                                                     build_id=build_id)
    self.assertEqual(candidate_path, self._GetPathToManifest(new_candidate))
    self.assertEqual(self.manager.current_version, version)

    filter_mock.assert_called_once_with(
        manifest, whitelisted_remotes=site_config.params.EXTERNAL_REMOTES)
    publish_mock.assert_called_once_with(new_manifest, version,
                                         build_id=build_id)
    init_mock.assert_called_once_with(my_info)
    self.push_mock.assert_called_once_with(mock.ANY, mock.ANY, sync=False)

  def testCreateNewCandidateReturnNoneIfNoWorkToDo(self):
    """Tests that we return nothing if there is nothing to create."""
    new_manifest = 'some_manifest'
    my_info = lkgm_manager._LKGMCandidateInfo('1.2.3')

    # Patch out our RepoRepository to make sure we don't corrupt real repo.
    cros_source_mock = self.PatchObject(self.manager, 'cros_source')
    cros_source_mock.branch = 'master'
    cros_source_mock.directory = '/foo/repo'

    self.PatchObject(lkgm_manager.LKGMManager, 'CheckoutSourceCode')
    self.PatchObject(lkgm_manager.LKGMManager, 'CreateManifest',
                     return_value=new_manifest)
    self.PatchObject(lkgm_manager.LKGMManager, 'RefreshManifestCheckout')
    self.PatchObject(lkgm_manager.LKGMManager, 'GetCurrentVersionInfo',
                     return_value=my_info)
    init_mock = self.PatchObject(lkgm_manager.LKGMManager,
                                 'InitializeManifestVariables')
    self.PatchObject(lkgm_manager.LKGMManager, 'HasCheckoutBeenBuilt',
                     return_value=True)

    # For _AdjustRepoCheckoutToLocalManifest.
    self.PatchObject(repository, 'CloneGitRepo')
    self.PatchObject(git, 'CreateBranch')

    candidate = self.manager.CreateNewCandidate()
    self.assertEqual(candidate, None)
    init_mock.assert_called_once_with(my_info)

  def _CreateManifest(self):
    """Returns a created test manifest in tmpdir with its dir_pfx."""
    self.manager.current_version = '1.2.4-rc21'
    dir_pfx = CHROME_BRANCH
    manifest = os.path.join(self.manager.manifest_dir,
                            self.manager.rel_working_dir, 'buildspecs',
                            dir_pfx, '1.2.4-rc21.xml')
    osutils.Touch(manifest)
    return manifest, dir_pfx

  def _MockParseGitLog(self, fake_git_log, project):
    exists_mock = self.PatchObject(os.path, 'exists', return_value=True)
    link_mock = self.PatchObject(logging, 'PrintBuildbotLink')
    fake_project_handler = mock.Mock(spec=git.Manifest)
    fake_project_handler.checkouts_by_path = {project['path']: project}
    self.PatchObject(git, 'Manifest', return_value=fake_project_handler)

    fake_result = cros_build_lib.CommandResult(output=fake_git_log)
    self.PatchObject(git, 'RunGit', return_value=fake_result)

    return exists_mock, link_mock

  def testGenerateBlameListSinceLKGM(self):
    """Tests that we can generate a blamelist from two commit messages.

    This test tests the functionality of generating a blamelist for a git log.
    Note in this test there are two commit messages, one commited by the
    Commit Queue and another from Non-Commit Queue.  We test the correct
    handling in both cases.
    """
    fake_git_log = """Author: Sammy Sosa <fake@fake.com>
    Commit: Chris Sosa <sosa@chromium.org>

    Date:   Mon Aug 8 14:52:06 2011 -0700

    Add in a test for cbuildbot

    TEST=So much testing
    BUG=chromium-os:99999

    Change-Id: Ib72a742fd2cee3c4a5223b8easwasdgsdgfasdf
    Reviewed-on: https://chromium-review.googlesource.com/1234
    Reviewed-by: Fake person <fake@fake.org>
    Tested-by: Sammy Sosa <fake@fake.com>
    Author: Sammy Sosa <fake@fake.com>
    Commit: Gerrit <chrome-bot@chromium.org>

    Date:   Mon Aug 8 14:52:06 2011 -0700

    Add in a test for cbuildbot

    TEST=So much testing
    BUG=chromium-os:99999

    Change-Id: Ib72a742fd2cee3c4a5223b8easwasdgsdgfasdf
    Reviewed-on: https://chromium-review.googlesource.com/1235
    Reviewed-by: Fake person <fake@fake.org>
    Tested-by: Sammy Sosa <fake@fake.com>
    """
    project = {
        'name': 'fake/repo',
        'path': 'fake/path',
        'revision': '1234567890',
    }
    self.manager.incr_type = 'build'
    self.PatchObject(cros_build_lib, 'RunCommand', side_effect=Exception())
    exists_mock, link_mock = self._MockParseGitLog(fake_git_log, project)
    self.manager.GenerateBlameListSinceLKGM()

    exists_mock.assert_called_once_with(
        os.path.join(self.tmpdir, project['path']))
    link_mock.assert_has_calls([
        mock.call('CHUMP | repo | fake | 1234',
                  'https://chromium-review.googlesource.com/1234'),
        mock.call('repo | fake | 1235',
                  'https://chromium-review.googlesource.com/1235'),
    ])

  def testGenerateBlameListHasChumpCL(self):
    """Test GenerateBlameList with chump CLs."""
    fake_git_log = """
    Author: Sammy Sosa <fake@fake.com>
    Commit: Chris Sosa <sosa@chromium.org>

    Date:   Mon Aug 8 14:52:06 2011 -0700

    Add in a test for cbuildbot

    TEST=So much testing
    BUG=chromium-os:99999

    Change-Id: Ib72a742fd2cee3c4a5223b8easwasdgsdgfasdf
    Reviewed-on: https://chromium-review.googlesource.com/1234
    Reviewed-by: Fake person <fake@fake.org>
    Tested-by: Sammy Sosa <fake@fake.com>
    Author: Sammy Sosa <fake@fake.com>
    Commit: Gerrit <chrome-bot@chromium.org>
    """
    project = {
        'name': 'fake/repo',
        'path': 'fake/path',
        'revision': '1234567890',
    }
    _, link_mock = self._MockParseGitLog(fake_git_log, project)
    has_chump_cls = lkgm_manager.GenerateBlameList(
        self.manager.cros_source, self.manager.lkgm_path)

    self.assertTrue(has_chump_cls)
    link_mock.assert_has_calls([
        mock.call('CHUMP | repo | fake | 1234',
                  'https://chromium-review.googlesource.com/1234')])

  def testGenerateBlameListNoChumpCL(self):
    """Test GenerateBlameList without chump CLs."""
    fake_git_log = """Author: Sammy Sosa <fake@fake.com>
    Commit: Gerrit <chrome-bot@chromium.org>

    Date:   Mon Aug 8 14:52:06 2011 -0700

    Add in a test for cbuildbot

    TEST=So much testing
    BUG=chromium-os:99999

    Change-Id: Ib72a742fd2cee3c4a5223b8easwasdgsdgfasdf
    Reviewed-on: https://chromium-review.googlesource.com/1235
    Reviewed-by: Fake person <fake@fake.org>
    Tested-by: Sammy Sosa <fake@fake.com>
    """
    project = {
        'name': 'fake/repo',
        'path': 'fake/path',
        'revision': '1234567890',
    }
    _, link_mock = self._MockParseGitLog(fake_git_log, project)
    has_chump_cl = lkgm_manager.GenerateBlameList(
        self.manager.cros_source, self.manager.lkgm_path)

    self.assertFalse(has_chump_cl)
    link_mock.assert_has_calls([
        mock.call('repo | fake | 1235',
                  'https://chromium-review.googlesource.com/1235')])

  def testAddChromeVersionToManifest(self):
    """Tests whether we can write the chrome version to the manifest file."""
    with TemporaryManifest() as f:
      chrome_version = '35.0.1863.0'
      # Write the chrome element to manifest.
      self.manager._AddChromeVersionToManifest(f.name, chrome_version)

      # Read the manifest file.
      new_doc = minidom.parse(f.name)
      elements = new_doc.getElementsByTagName(lkgm_manager.CHROME_ELEMENT)
      self.assertEqual(len(elements), 1)
      self.assertEqual(
          elements[0].getAttribute(lkgm_manager.CHROME_VERSION_ATTR),
          chrome_version)

  def testAddLKGMToManifest(self, present=True):
    """Tests whether we can write the LKGM version to the manifest file."""
    with TemporaryManifest() as f:
      # Set up LGKM symlink.
      if present:
        lkgm_version = '6377.0.0-rc1'
        os.makedirs(os.path.dirname(self.manager.lkgm_path))
        os.symlink('../foo/%s.xml' % lkgm_version, self.manager.lkgm_path)

      # Write the chrome element to manifest.
      self.manager._AddLKGMToManifest(f.name)

      # Read the manifest file.
      new_doc = minidom.parse(f.name)
      elements = new_doc.getElementsByTagName(lkgm_manager.LKGM_ELEMENT)
      if present:
        self.assertEqual(len(elements), 1)
        self.assertEqual(
            elements[0].getAttribute(lkgm_manager.LKGM_VERSION_ATTR),
            lkgm_version)
      else:
        self.assertEqual(len(elements), 0)

  def testAddLKGMToManifestWithMissingFile(self):
    """Tests writing the LKGM version when LKGM.xml is missing."""
    self.testAddLKGMToManifest(present=False)

  def _MockValidationPool(self, gerrit_patchs):
    mock_pool = mock.Mock()
    mock_pool.applied = gerrit_patchs
    return mock_pool

  def testAddPatchesToManifest(self):
    """Tests whether we can add a fake patch to an empty manifest file.

    This test creates an empty xml file with just manifest/ tag in it then
    runs the AddPatchesToManifest with one mocked out GerritPatch and ensures
    the newly generated manifest has the correct patch information afterwards.
    """
    with TemporaryManifest() as f:
      gerrit_patch = cros_patch.GerritFetchOnlyPatch(
          'https://host/chromite/tacos',
          'chromite/tacos',
          'refs/changes/11/12345/4',
          'master',
          'cros-internal',
          '7181e4b5e182b6f7d68461b04253de095bad74f9',
          'I47ea30385af60ae4cc2acc5d1a283a46423bc6e1',
          '12345',
          '4',
          'foo@chromium.org',
          1,
          1,
          3)

      mock_pool = self._MockValidationPool([gerrit_patch])
      self.manager._AddPatchesToManifest(f.name, mock_pool)

      new_doc = minidom.parse(f.name)
      element = new_doc.getElementsByTagName(
          lkgm_manager.PALADIN_COMMIT_ELEMENT)[0]

      self.assertEqual(element.getAttribute(
          cros_patch.ATTR_CHANGE_ID), gerrit_patch.change_id)
      self.assertEqual(element.getAttribute(
          cros_patch.ATTR_COMMIT), gerrit_patch.commit)
      self.assertEqual(element.getAttribute(cros_patch.ATTR_PROJECT),
                       gerrit_patch.project)
      self.assertEqual(element.getAttribute(cros_patch.ATTR_REMOTE),
                       gerrit_patch.remote)
      self.assertEqual(element.getAttribute(cros_patch.ATTR_BRANCH),
                       gerrit_patch.tracking_branch)
      self.assertEqual(element.getAttribute(cros_patch.ATTR_REF),
                       gerrit_patch.ref)
      self.assertEqual(
          element.getAttribute(cros_patch.ATTR_OWNER_EMAIL),
          gerrit_patch.owner_email)
      self.assertEqual(
          element.getAttribute(cros_patch.ATTR_PROJECT_URL),
          gerrit_patch.project_url)
      self.assertEqual(
          element.getAttribute(cros_patch.ATTR_PATCH_NUMBER),
          gerrit_patch.patch_number)
      self.assertEqual(
          element.getAttribute(cros_patch.ATTR_FAIL_COUNT),
          str(gerrit_patch.fail_count))
      self.assertEqual(
          element.getAttribute(cros_patch.ATTR_PASS_COUNT),
          str(gerrit_patch.pass_count))
      self.assertEqual(
          element.getAttribute(cros_patch.ATTR_TOTAL_FAIL_COUNT),
          str(gerrit_patch.total_fail_count))

  def testAddPatchesToManifestWithUnicode(self):
    """Tests to add a fake patch with unicode to an empty manifest file.

    Test whether _AddPatchesToManifest can add to a patch with unicode to
    manifest file without any UnicodeError exception and that the decoded
    manifest has the original unicode string.
    """
    with TemporaryManifest() as f:
      gerrit_patch = cros_patch.GerritFetchOnlyPatch(
          'https://host/chromite/tacos',
          'chromite/tacos',
          'refs/changes/11/12345/4',
          'master',
          'cros-internal',
          '7181e4b5e182b6f7d68461b04253de095bad74f9',
          'I47ea30385af60ae4cc2acc5d1a283a46423bc6e1',
          '12345',
          '4',
          u'foo\xe9@chromium.org',
          1,
          1,
          3)

      mock_pool = self._MockValidationPool([gerrit_patch])
      self.manager._AddPatchesToManifest(f.name, mock_pool)

      new_doc = minidom.parse(f.name)
      element = new_doc.getElementsByTagName(
          lkgm_manager.PALADIN_COMMIT_ELEMENT)[0]

      self.assertEqual(
          element.getAttribute(cros_patch.ATTR_OWNER_EMAIL),
          gerrit_patch.owner_email)

  def testAddPatchesToManifestWithInvalidTokens(self):
    """Tests to add a fake patch with invalid tokens to a manifest.

    Test whether _AddPatchesToManifest will skip commits with invalid tokens.
    """
    with TemporaryManifest() as f:
      gerrit_patch = cros_patch.GerritFetchOnlyPatch(
          'https://host/chromite/tacos',
          'chromite/tacos',
          'refs/changes/11/12345/4',
          'master',
          'cros-internal',
          '7181e4b5e182b6f7d68461b04253de095bad74f9',
          'I47ea30385af60ae4cc2acc5d1a283a46423bc6e1',
          '12345',
          '4',
          'foo@chromium.org',
          1,
          1,
          3,
          #Invalid tokens
          '…')

      mock_pool = self._MockValidationPool([gerrit_patch])
      self.manager._AddPatchesToManifest(f.name, mock_pool)

      new_doc = minidom.parse(f.name)
      self.assertEqual(0, len(new_doc.getElementsByTagName(
          lkgm_manager.PALADIN_COMMIT_ELEMENT)))

      self.assertEqual(1, mock_pool.SendNotification.call_count)
      self.assertEqual(1, mock_pool.RemoveReady.call_count)
