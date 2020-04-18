# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for GerritHelper."""

from __future__ import print_function

import getpass
import httplib
import os
import mock

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.cbuildbot import validation_pool
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import gerrit
from chromite.lib import git
from chromite.lib import gob_util
from chromite.lib import osutils
from chromite.lib import retry_util
from chromite.lib import timeout_util


site_config = config_lib.GetConfig()


# NOTE: The following test cases are designed to run as part of the release
# qualification process for the googlesource.com servers:
#   GerritHelperTest
# Any new test cases must be manually added to the qualification test suite.


# pylint: disable=W0212,R0904
@cros_test_lib.NetworkTest()
class GerritHelperTest(cros_test_lib.GerritTestCase):
  """Unittests for GerritHelper."""

  def _GetHelper(self, remote=site_config.params.EXTERNAL_REMOTE):
    return gerrit.GetGerritHelper(remote)

  def createPatch(self, clone_path, project, **kwargs):
    """Create a patch in the given git checkout and upload it to gerrit.

    Args:
      clone_path: The directory on disk of the git clone.
      project: The associated project.
      **kwargs: Additional keyword arguments to pass to createCommit.

    Returns:
      A GerritPatch object.
    """
    (revision, changeid) = self.createCommit(clone_path, **kwargs)
    self.uploadChange(clone_path)
    def PatchQuery():
      return self._GetHelper().QuerySingleRecord(
          change=changeid, project=project, branch='master')
    # 'RetryException' is needed because there is a race condition between
    # uploading the change and querying for the change.
    gpatch = retry_util.RetryException(
        gerrit.QueryHasNoResults,
        5,
        PatchQuery,
        sleep=1)
    self.assertEqual(gpatch.change_id, changeid)
    self.assertEqual(gpatch.revision, revision)
    return gpatch

  def test001SimpleQuery(self):
    """Create one independent and three dependent changes, then query them."""
    project = self.createProject('test001')
    clone_path = self.cloneProject(project)
    (head_sha1, head_changeid) = self.createCommit(clone_path)
    for idx in xrange(3):
      cros_build_lib.RunCommand(
          ['git', 'checkout', head_sha1], cwd=clone_path, quiet=True)
      self.createCommit(clone_path, filename='test-file-%d.txt' % idx)
      self.uploadChange(clone_path)
    helper = self._GetHelper()
    changes = helper.Query(owner='self', project=project)
    self.assertEqual(len(changes), 4)
    changes = helper.Query(head_changeid, project=project, branch='master')
    self.assertEqual(len(changes), 1)
    self.assertEqual(changes[0].change_id, head_changeid)
    self.assertEqual(changes[0].sha1, head_sha1)
    change = helper.QuerySingleRecord(
        head_changeid, project=project, branch='master')
    self.assertTrue(change)
    self.assertEqual(change.change_id, head_changeid)
    self.assertEqual(change.sha1, head_sha1)
    change = helper.GrabPatchFromGerrit(project, head_changeid, head_sha1)
    self.assertTrue(change)
    self.assertEqual(change.change_id, head_changeid)
    self.assertEqual(change.sha1, head_sha1)

  @mock.patch.object(gerrit.GerritHelper, '_GERRIT_MAX_QUERY_RETURN', 2)
  def test002GerritQueryTruncation(self):
    """Verify that we detect gerrit truncating our query, and handle it."""
    project = self.createProject('test002')
    clone_path = self.cloneProject(project)
    # Using a shell loop is markedly faster than running a python loop.
    num_changes = 5
    cmd = ('for ((i=0; i<%i; i=i+1)); do '
           'echo "Another day, another dollar." > test-file-$i.txt; '
           'git add test-file-$i.txt; '
           'git commit -m "Test commit $i."; '
           'done' % num_changes)
    cros_build_lib.RunCommand(cmd, shell=True, cwd=clone_path, quiet=True)
    self.uploadChange(clone_path)
    helper = self._GetHelper()
    changes = helper.Query(project=project)
    self.assertEqual(len(changes), num_changes)

  def test003IsChangeCommitted(self):
    """Tests that we can parse a json to check if a change is committed."""
    project = self.createProject('test003')
    clone_path = self.cloneProject(project)
    gpatch = self.createPatch(clone_path, project)
    helper = self._GetHelper()
    helper.SetReview(gpatch.gerrit_number, labels={'Code-Review':'+2'})
    helper.SubmitChange(gpatch)
    self.assertTrue(helper.IsChangeCommitted(gpatch.gerrit_number))

    gpatch = self.createPatch(clone_path, project)
    self.assertFalse(helper.IsChangeCommitted(gpatch.gerrit_number))

  def test004GetLatestSHA1ForBranch(self):
    """Verifies that we can query the tip-of-tree commit in a git repository."""
    project = self.createProject('test004')
    clone_path = self.cloneProject(project)
    for _ in xrange(5):
      (master_sha1, _) = self.createCommit(clone_path)
    self.pushBranch(clone_path, 'master')
    for _ in xrange(5):
      (testbranch_sha1, _) = self.createCommit(clone_path)
    self.pushBranch(clone_path, 'testbranch')
    helper = self._GetHelper()
    self.assertEqual(
        helper.GetLatestSHA1ForBranch(project, 'master'),
        master_sha1)
    self.assertEqual(
        helper.GetLatestSHA1ForBranch(project, 'testbranch'),
        testbranch_sha1)

  def _ChooseReviewers(self):
    all_reviewers = set(['dborowitz@google.com', 'sop@google.com',
                         'jrn@google.com'])
    ret = list(all_reviewers.difference(['%s@google.com' % getpass.getuser()]))
    self.assertGreaterEqual(len(ret), 2)
    return ret

  def test005SetReviewers(self):
    """Verify that we can set reviewers on a CL."""
    project = self.createProject('test005')
    clone_path = self.cloneProject(project)
    gpatch = self.createPatch(clone_path, project)
    emails = self._ChooseReviewers()
    helper = self._GetHelper()
    helper.SetReviewers(gpatch.gerrit_number, add=(
        emails[0], emails[1]))
    reviewers = gob_util.GetReviewers(helper.host, gpatch.gerrit_number)
    self.assertEqual(len(reviewers), 2)
    self.assertItemsEqual(
        [r['email'] for r in reviewers],
        [emails[0], emails[1]])
    helper.SetReviewers(gpatch.gerrit_number,
                        remove=(emails[0],))
    reviewers = gob_util.GetReviewers(helper.host, gpatch.gerrit_number)
    self.assertEqual(len(reviewers), 1)
    self.assertEqual(reviewers[0]['email'], emails[1])

  def test006PatchNotFound(self):
    """Test case where ChangeID isn't found on the server."""
    changeids = ['I' + ('deadbeef' * 5), 'I' + ('beadface' * 5)]
    self.assertRaises(gerrit.GerritException, gerrit.GetGerritPatchInfo,
                      changeids)
    self.assertRaises(gerrit.GerritException, gerrit.GetGerritPatchInfo,
                      ['*' + cid for cid in changeids])
    # Change ID sequence starts at 1000.
    gerrit_numbers = ['123', '543']
    self.assertRaises(gerrit.GerritException, gerrit.GetGerritPatchInfo,
                      gerrit_numbers)
    self.assertRaises(gerrit.GerritException, gerrit.GetGerritPatchInfo,
                      ['*' + num for num in gerrit_numbers])

  def test007VagueQuery(self):
    """Verify GerritHelper complains if an ID matches multiple changes."""
    project = self.createProject('test007')
    clone_path = self.cloneProject(project)
    (sha1, _) = self.createCommit(clone_path)
    (_, changeid) = self.createCommit(clone_path)
    self.uploadChange(clone_path, 'master')
    cros_build_lib.RunCommand(
        ['git', 'checkout', sha1], cwd=clone_path, quiet=True)
    self.createCommit(clone_path)
    self.pushBranch(clone_path, 'testbranch')
    self.createCommit(
        clone_path, msg='Test commit.\n\nChange-Id: %s' % changeid)
    self.uploadChange(clone_path, 'testbranch')
    self.assertRaises(gerrit.GerritException, gerrit.GetGerritPatchInfo,
                      [changeid])

  def test008Queries(self):
    """Verify assorted query operations."""
    project = self.createProject('test008')
    clone_path = self.cloneProject(project)
    gpatch = self.createPatch(clone_path, project)
    helper = self._GetHelper()

    # Multi-queries with one valid and one invalid term should raise.
    invalid_change_id = 'I1234567890123456789012345678901234567890'
    self.assertRaises(gerrit.GerritException, gerrit.GetGerritPatchInfo,
                      [invalid_change_id, gpatch.change_id])
    self.assertRaises(gerrit.GerritException, gerrit.GetGerritPatchInfo,
                      [gpatch.change_id, invalid_change_id])
    self.assertRaises(gerrit.GerritException, gerrit.GetGerritPatchInfo,
                      ['9876543', gpatch.gerrit_number])
    self.assertRaises(gerrit.GerritException, gerrit.GetGerritPatchInfo,
                      [gpatch.gerrit_number, '9876543'])

    # Simple query by project/changeid/sha1.
    patch_info = helper.GrabPatchFromGerrit(gpatch.project, gpatch.change_id,
                                            gpatch.sha1)
    self.assertEqual(patch_info.gerrit_number, gpatch.gerrit_number)
    self.assertEqual(patch_info.remote, site_config.params.EXTERNAL_REMOTE)

    # Simple query by gerrit number to external remote.
    patch_info = gerrit.GetGerritPatchInfo([gpatch.gerrit_number])
    self.assertEqual(patch_info[0].gerrit_number, gpatch.gerrit_number)
    self.assertEqual(patch_info[0].remote, site_config.params.EXTERNAL_REMOTE)

    # Simple query by gerrit number to internal remote.
    patch_info = gerrit.GetGerritPatchInfo(['*' + gpatch.gerrit_number])
    self.assertEqual(patch_info[0].gerrit_number, gpatch.gerrit_number)
    self.assertEqual(patch_info[0].remote, site_config.params.INTERNAL_REMOTE)

    # Query to external server by gerrit number and change-id which refer to
    # the same change should return one result.
    fq_changeid = '~'.join((gpatch.project, 'master', gpatch.change_id))
    patch_info = gerrit.GetGerritPatchInfo([gpatch.gerrit_number, fq_changeid])
    self.assertEqual(len(patch_info), 1)
    self.assertEqual(patch_info[0].gerrit_number, gpatch.gerrit_number)
    self.assertEqual(patch_info[0].remote, site_config.params.EXTERNAL_REMOTE)

    # Query to internal server by gerrit number and change-id which refer to
    # the same change should return one result.
    patch_info = gerrit.GetGerritPatchInfo(
        ['*' + gpatch.gerrit_number, '*' + fq_changeid])
    self.assertEqual(len(patch_info), 1)
    self.assertEqual(patch_info[0].gerrit_number, gpatch.gerrit_number)
    self.assertEqual(patch_info[0].remote, site_config.params.INTERNAL_REMOTE)

  def test009SubmitOutdatedCommit(self):
    """Tests that we can parse a json to check if a change is committed."""
    project = self.createProject('test009')
    clone_path = self.cloneProject(project, 'p1')

    # Create a change.
    gpatch1 = self.createPatch(clone_path, project)

    # Update the change.
    gpatch2 = self.createPatch(clone_path, project, amend=True)

    # Make sure we're talking about the same change.
    self.assertEqual(gpatch1.change_id, gpatch2.change_id)

    # Try submitting the out-of-date change.
    helper = self._GetHelper()
    helper.SetReview(gpatch1.gerrit_number, labels={'Code-Review':'+2'})
    with self.assertRaises(gob_util.GOBError) as ex:
      helper.SubmitChange(gpatch1)
    self.assertEqual(ex.exception.http_status, httplib.CONFLICT)
    self.assertFalse(helper.IsChangeCommitted(gpatch1.gerrit_number))

    # Try submitting the up-to-date change.
    helper.SubmitChange(gpatch2)
    helper.IsChangeCommitted(gpatch2.gerrit_number)

  def test010SubmitBatchUsingGit(self):
    project = self.createProject('test012')

    helper = self._GetHelper()
    repo = self.cloneProject(project, 'p1')
    initial_patch = self.createPatch(repo, project, msg='Init')
    helper.SetReview(initial_patch.gerrit_number, labels={'Code-Review':'+2'})
    helper.SubmitChange(initial_patch)
    # GoB does not guarantee that the change will be in "merged" state
    # atomically after the /Submit endpoint is called.
    timeout_util.WaitForReturnTrue(
        lambda: helper.IsChangeCommitted(initial_patch.gerrit_number),
        timeout=60)

    patchA = self.createPatch(repo, project,
                              msg='Change A',
                              filename='a.txt')

    osutils.WriteFile(os.path.join(repo, 'aoeu.txt'), 'asdf')
    git.RunGit(repo, ['add', 'aoeu.txt'])
    git.RunGit(repo, ['commit', '--amend', '--reuse-message=HEAD'])
    sha1 = git.RunGit(repo,
                      ['rev-list', '-n1', 'HEAD']).output.strip()

    patchA.sha1 = sha1
    patchA.revision = sha1

    patchB = self.createPatch(repo, project,
                              msg='Change B',
                              filename='b.txt')

    pool = validation_pool.ValidationPool(
        overlays=constants.PUBLIC,
        build_root='',
        build_number=0,
        builder_name='',
        is_master=False,
        dryrun=False)

    reason = "Testing submitting changes in batch via Git."
    by_repo = {repo: {patchA: reason, patchB: reason}}
    pool.SubmitLocalChanges(by_repo)

    self.assertTrue(helper.IsChangeCommitted(patchB.gerrit_number))
    self.assertTrue(helper.IsChangeCommitted(patchA.gerrit_number))
    for patch in [patchA, patchB]:
      self.assertTrue(helper.IsChangeCommitted(patch.gerrit_number))

  def test011ResetReviewLabels(self):
    """Tests that we can remove a code review label."""
    project = self.createProject('test011')
    helper = self._GetHelper()
    clone_path = self.cloneProject(project, 'p1')
    gpatch = self.createPatch(clone_path, project, msg='Init')
    helper.SetReview(gpatch.gerrit_number, labels={'Code-Review':'+2'})
    gob_util.ResetReviewLabels(helper.host, gpatch.gerrit_number,
                               label='Code-Review', notify='OWNER')

  def test012ApprovalTime(self):
    """Approval timestamp should be reset when a new patchset is created."""
    # Create a change.
    project = self.createProject('test013')
    helper = self._GetHelper()
    clone_path = self.cloneProject(project, 'p1')
    gpatch = self.createPatch(clone_path, project, msg='Init')
    helper.SetReview(gpatch.gerrit_number, labels={'Code-Review':'+2'})

    # Update the change.
    new_msg = 'New %s' % gpatch.commit_message
    cros_build_lib.RunCommand(
        ['git', 'commit', '--amend', '-m', new_msg], cwd=clone_path, quiet=True)
    self.uploadChange(clone_path)
    gpatch2 = self._GetHelper().QuerySingleRecord(
        change=gpatch.change_id, project=gpatch.project, branch='master')
    self.assertNotEqual(gpatch2.approval_timestamp, 0)
    self.assertNotEqual(gpatch2.commit_timestamp, 0)
    self.assertEqual(gpatch2.approval_timestamp, gpatch2.commit_timestamp)


@cros_test_lib.NetworkTest()
class DirectGerritHelperTest(cros_test_lib.TestCase):
  """Unittests for GerritHelper that use the real Chromium instance."""

  # A big list of real changes.
  CHANGES = ['235893', '*189165', '231790', '*190026', '231647', '234645']

  def testMultipleChangeDetail(self):
    """Test ordering of results in GetMultipleChangeDetail"""
    changes = [x for x in self.CHANGES if not x.startswith('*')]
    helper = gerrit.GetCrosExternal()
    results = list(helper.GetMultipleChangeDetail([str(x) for x in changes]))
    gerrit_numbers = [str(x['_number']) for x in results]
    self.assertEqual(changes, gerrit_numbers)

  def testQueryMultipleCurrentPatchset(self):
    """Test ordering of results in QueryMultipleCurrentPatchset"""
    changes = [x for x in self.CHANGES if not x.startswith('*')]
    helper = gerrit.GetCrosExternal()
    results = list(helper.QueryMultipleCurrentPatchset(changes))
    self.assertEqual(changes, [x.gerrit_number for _, x in results])
    self.assertEqual(changes, [x for x, _ in results])

  def testGetGerritPatchInfo(self):
    """Test ordering of results in GetGerritPatchInfo"""
    changes = self.CHANGES
    results = list(gerrit.GetGerritPatchInfo(changes))
    self.assertEqual(changes, [x.gerrit_number_str for x in results])
