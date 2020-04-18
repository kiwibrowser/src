# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module that contains unittests for patch_series module."""

from __future__ import print_function

import contextlib
import mock
import mox
import os

from chromite.cbuildbot import patch_series
from chromite.cbuildbot import validation_pool_unittest
from chromite.lib import config_lib
from chromite.lib import gerrit
from chromite.lib import git
from chromite.lib import cros_test_lib
from chromite.lib import parallel
from chromite.lib import parallel_unittest
from chromite.lib import partial_mock
from chromite.lib import patch as cros_patch
from chromite.lib import patch_unittest


site_config = config_lib.GetConfig()


def FakeFetchChangesForRepo(fetched_changes, by_repo, repo):
  """Fake version of the "PatchSeries._FetchChangesForRepo" method.

  Thes does nothing to the changes and simply copies them into the output
  dict.
  """
  for c in by_repo[repo]:
    fetched_changes[c.id] = c


class FakePatch(partial_mock.PartialMock):
  """Mocks out dependency and fetch methods of GitRepoPatch.

  Usage: set FakePatch.parents, .cq and .build_roots per patch, and set
  FakePatch.assertEqual to your TestCase's assertEqual method.  The behavior of
  `GerritDependencies`, `PaladinDependencies` and `Fetch` depends on the patch
  id.
  """

  TARGET = 'chromite.lib.patch.GitRepoPatch'
  ATTRS = ('GerritDependencies', 'PaladinDependencies', 'Fetch')

  parents = {}
  cq = {}
  build_root = None
  assertEqual = None

  def PreStart(self):
    FakePatch.parents = {}
    FakePatch.cq = {}

  def PreStop(self):
    FakePatch.build_root = None
    FakePatch.assertEqual = None

  def GerritDependencies(self, patch):
    return map(cros_patch.ParsePatchDep, self.parents[patch.id])

  def PaladinDependencies(self, patch, path):
    self._assertPath(patch, path)
    return map(cros_patch.ParsePatchDep, self.cq[patch.id])

  def Fetch(self, patch, path):
    self._assertPath(patch, path)
    return patch.sha1

  def _assertPath(self, patch, path):
    self.assertEqual(path,
                     os.path.join(self.build_root, patch.project))


class FakeGerritPatch(FakePatch):
  """Mocks out the "GerritDependencies" method of GerritPatch.

  This is necessary because GerritPatch overrides the GerritDependencies method.
  """
  TARGET = 'chromite.lib.patch.GerritPatch'
  ATTRS = ('GerritDependencies',)


# pylint: disable=protected-access
# pylint: disable=too-many-ancestors
class PatchSeriesTestCase(patch_unittest.UploadedLocalPatchTestCase,
                          cros_test_lib.MoxTestCase,
                          cros_test_lib.MockTestCase):
  """Base class for tests that need to test PatchSeries."""

  @contextlib.contextmanager
  def _ValidateTransactionCall(self, _changes):
    yield

  def setUp(self):
    self.StartPatcher(parallel_unittest.ParallelMock())
    self._patch_factory = patch_unittest.MockPatchFactory()
    self.build_root = 'fakebuildroot'
    self.GetPatches = self._patch_factory.GetPatches
    self.MockPatch = self._patch_factory.MockPatch

  def MakeHelper(self, cros_internal=None, cros=None):
    # pylint: disable=W0201
    if cros_internal:
      cros_internal = self.mox.CreateMock(gerrit.GerritHelper)
      cros_internal.version = '2.2'
      cros_internal.remote = site_config.params.INTERNAL_REMOTE
    if cros:
      cros = self.mox.CreateMock(gerrit.GerritHelper)
      cros.remote = site_config.params.EXTERNAL_REMOTE
      cros.version = '2.2'
    return patch_series.HelperPool(cros_internal=cros_internal,
                                   cros=cros)

  def GetPatchSeries(self, helper_pool=None):
    if helper_pool is None:
      helper_pool = self.MakeHelper(cros_internal=True, cros=True)
    series = patch_series.PatchSeries(self.build_root, helper_pool)

    # Suppress transactions.
    series._Transaction = self._ValidateTransactionCall
    series.GetGitRepoForChange = \
        lambda change, **kwargs: os.path.join(self.build_root, change.project)

    return series

  def _ValidatePatchApplyManifest(self, value):
    self.assertTrue(isinstance(value, validation_pool_unittest.MockManifest))
    self.assertEqual(value.root, self.build_root)
    return True

  def SetPatchApply(self, patch, trivial=False):
    self.mox.StubOutWithMock(patch, 'ApplyAgainstManifest')
    return patch.ApplyAgainstManifest(
        mox.Func(self._ValidatePatchApplyManifest),
        trivial=trivial)

  def assertResults(self, series, changes, applied=(), failed_tot=(),
                    failed_inflight=(), frozen=True):
    manifest = validation_pool_unittest.MockManifest(self.build_root)
    result = series.Apply(changes, frozen=frozen, manifest=manifest)

    _GetIds = lambda seq: [x.id for x in seq]
    _GetFailedIds = lambda seq: _GetIds(x.patch for x in seq)

    applied_result = _GetIds(result[0])
    failed_tot_result, failed_inflight_result = map(_GetFailedIds, result[1:])

    applied = _GetIds(applied)
    failed_tot = _GetIds(failed_tot)
    failed_inflight = _GetIds(failed_inflight)

    self.maxDiff = None
    self.assertEqual(applied, applied_result)
    self.assertItemsEqual(failed_inflight, failed_inflight_result)
    self.assertItemsEqual(failed_tot, failed_tot_result)
    return result


class TestUploadedLocalPatch(PatchSeriesTestCase):
  """Test the interaction between uploaded local git patches and PatchSeries."""

  def testFetchChanges(self):
    """Test fetching uploaded local patches."""
    git1, git2, patch1 = self._CommonGitSetup()
    patch2 = self.CommitFile(git1, 'monkeys2', 'foon2')
    patch3 = self._MkPatch(git1, None, original_sha1=patch1.sha1)
    patch4 = self._MkPatch(git1, None, original_sha1=patch2.sha1)
    self.assertEqual(patch3.id, patch1.id)
    self.assertEqual(patch4.id, patch2.id)
    self.assertNotEqual(patch3.id, patch4.id)
    series = self.GetPatchSeries()
    series.GetGitRepoForChange = lambda change, **kwargs: git2
    patches, _ = series.FetchChanges([patch3, patch4])
    self.assertEqual(len(patches), 2)
    self.assertEqual(patches[0].id, patch3.id)
    self.assertEqual(patches[1].id, patch4.id)

  def testFetchChangesWithChangeNotInManifest(self):
    """test FetchChanges with ChangeNotInManifest."""
    # pylint: disable=unused-argument
    def raiseException(change, **kwargs):
      raise cros_patch.ChangeNotInManifest(change)

    patch_1, patch_2 = patches = self.GetPatches(2)

    series = self.GetPatchSeries()
    series.GetGitRepoForChange = raiseException

    changes, not_in_manifest = series.FetchChanges(patches)

    self.assertEqual(len(changes), 0)
    self.assertEqual(len(not_in_manifest), 2)
    self.assertEqual(not_in_manifest[0].patch, patch_1)
    self.assertEqual(not_in_manifest[1].patch, patch_2)


class TestPatchSeries(PatchSeriesTestCase):
  """Tests resolution and applying logic of validation_pool.ValidationPool."""

  def setUp(self):
    self.StartPatcher(FakePatch())
    self.PatchObject(FakePatch, 'assertEqual', new=self.assertEqual)
    self.PatchObject(FakePatch, 'build_root', new=self.build_root)
    self.PatchObject(patch_series, '_FetchChangesForRepo',
                     new=FakeFetchChangesForRepo)
    self.StartPatcher(FakeGerritPatch())

  def SetPatchDeps(self, patch, parents=(), cq=()):
    """Set the dependencies of |patch|.

    Args:
      patch: The patch to process.
      parents: A set of strings to set as parents of |patch|.
      cq: A set of strings to set as paladin dependencies of |patch|.
    """
    FakePatch.parents[patch.id] = parents
    FakePatch.cq[patch.id] = cq

  def testApplyWithDeps(self):
    """Test that we can apply changes correctly and respect deps.

    This tests a simple out-of-order change where change1 depends on change2
    but tries to get applied before change2.  What should happen is that
    we should notice change2 is a dep of change1 and apply it first.
    """
    series = self.GetPatchSeries()

    patch1, patch2 = patches = self.GetPatches(2)

    self.SetPatchDeps(patch2)
    self.SetPatchDeps(patch1, [patch2.id])

    self.SetPatchApply(patch2)
    self.SetPatchApply(patch1)

    self.mox.ReplayAll()
    self.assertResults(series, patches, [patch2, patch1])
    self.mox.VerifyAll()

  def testSha1Deps(self):
    """Test that we can apply changes correctly and respect sha1 deps.

    This tests a simple out-of-order change where change1 depends on change2
    but tries to get applied before change2.  What should happen is that
    we should notice change2 is a dep of change1 and apply it first.
    """
    series = self.GetPatchSeries()

    patch1, patch2, patch3 = patches = self.GetPatches(3)
    patch3.remote = site_config.params.INTERNAL_REMOTE

    self.SetPatchDeps(patch1, [patch2.sha1])
    self.SetPatchDeps(patch2, ['*%s' % patch3.sha1])
    self.SetPatchDeps(patch3)

    self.SetPatchApply(patch2)
    self.SetPatchApply(patch3)
    self.SetPatchApply(patch1)

    self.mox.ReplayAll()
    self.assertResults(series, patches, [patch3, patch2, patch1])
    self.mox.VerifyAll()

  def testGerritNumberDeps(self):
    """Test that we can apply CQ-DEPEND changes in the right order."""
    series = self.GetPatchSeries()

    patch1, patch2, patch3 = patches = self.GetPatches(3)

    self.SetPatchDeps(patch1, cq=[patch2.id])
    self.SetPatchDeps(patch2, cq=[patch3.gerrit_number])
    self.SetPatchDeps(patch3, cq=[patch1.gerrit_number])

    self.SetPatchApply(patch1)
    self.SetPatchApply(patch2)
    self.SetPatchApply(patch3)

    self.mox.ReplayAll()
    self.assertResults(series, patches, patches[::-1])
    self.mox.VerifyAll()

  def testGerritLazyMapping(self):
    """Given a patch lacking a gerrit number, via gerrit, map it to that change.

    Literally, this ensures that local patches pushed up- lacking a gerrit
    number- are mapped back to a changeid via asking gerrit for that number,
    then the local matching patch is used if available.
    """
    series = self.GetPatchSeries()

    patch1 = self.MockPatch()
    self.PatchObject(patch1, 'LookupAliases', return_value=[patch1.id])
    patch2 = self.MockPatch(change_id=int(patch1.change_id[1:]))
    patch3 = self.MockPatch()

    self.SetPatchDeps(patch3, cq=[patch2.gerrit_number])
    self.SetPatchDeps(patch2)
    self.SetPatchDeps(patch1)

    self.SetPatchApply(patch1)
    self.SetPatchApply(patch3)

    self._SetQuery(series, patch2, query=patch2.gerrit_number).AndReturn(patch2)

    self.mox.ReplayAll()
    applied = self.assertResults(series, [patch1, patch3], [patch1, patch3])[0]
    self.assertTrue(applied[0] is patch1)
    self.assertTrue(applied[1] is patch3)
    self.mox.VerifyAll()

  def testCrosGerritDeps(self, cros_internal=True):
    """Test that we can apply changes correctly and respect deps.

    This tests a simple out-of-order change where change1 depends on change3
    but tries to get applied before it.  What should happen is that
    we should notice the dependency and apply change3 first.
    """
    helper_pool = self.MakeHelper(cros_internal=cros_internal, cros=True)
    series = self.GetPatchSeries(helper_pool=helper_pool)

    patch1 = self.MockPatch(remote=site_config.params.EXTERNAL_REMOTE)
    patch2 = self.MockPatch(remote=site_config.params.INTERNAL_REMOTE)
    patch3 = self.MockPatch(remote=site_config.params.EXTERNAL_REMOTE)
    patches = [patch1, patch2, patch3]
    if cros_internal:
      applied_patches = [patch3, patch2, patch1]
    else:
      applied_patches = [patch3, patch1]

    self.SetPatchDeps(patch1, [patch3.id])
    self.SetPatchDeps(patch2)
    self.SetPatchDeps(patch3, cq=[patch2.id])

    if cros_internal:
      self.SetPatchApply(patch2)
    self.SetPatchApply(patch1)
    self.SetPatchApply(patch3)

    self.mox.ReplayAll()
    self.assertResults(series, patches, applied_patches)
    self.mox.VerifyAll()

  def testExternalCrosGerritDeps(self):
    """Test that we exclude internal deps on external trybot."""
    self.testCrosGerritDeps(cros_internal=False)

  @staticmethod
  def _SetQuery(series, change, query=None):
    helper = series._helper_pool.GetHelper(change.remote)
    query = change.id if query is None else query
    return helper.QuerySingleRecord(query, must_match=True)

  def testApplyMissingDep(self):
    """Test that we don't try to apply a change without met dependencies.

    Patch2 is in the validation pool that depends on Patch1 (which is not)
    Nothing should get applied.
    """
    series = self.GetPatchSeries()

    patch1, patch2 = self.GetPatches(2)

    self.SetPatchDeps(patch2, [patch1.id])
    self._SetQuery(series, patch1).AndReturn(patch1)

    self.mox.ReplayAll()
    self.assertResults(series, [patch2],
                       [], [patch2])
    self.mox.VerifyAll()

  def testApplyWithCommittedDeps(self):
    """Test that we apply a change with dependency already committed."""
    series = self.GetPatchSeries()

    # Use for basic commit check.
    patch1 = self.GetPatches(1, is_merged=True)
    patch2 = self.GetPatches(1)

    self.SetPatchDeps(patch2, [patch1.id])
    self._SetQuery(series, patch1).AndReturn(patch1)
    self.SetPatchApply(patch2)

    # Used to ensure that an uncommitted change put in the lookup cache
    # isn't invalidly pulled into the graph...
    patch3, patch4, patch5 = self.GetPatches(3)

    self._SetQuery(series, patch3).AndReturn(patch3)
    self.SetPatchDeps(patch4, [patch3.id])
    self.SetPatchDeps(patch5, [patch3.id])

    self.mox.ReplayAll()
    self.assertResults(series, [patch2, patch4, patch5], [patch2],
                       [patch4, patch5])
    self.mox.VerifyAll()

  def testCyclicalDeps(self):
    """Verify that the machinery handles cycles correctly."""
    series = self.GetPatchSeries()

    patch1, patch2, patch3 = patches = self.GetPatches(3)

    self.SetPatchDeps(patch1, [patch2.id])
    self.SetPatchDeps(patch2, cq=[patch3.id])
    self.SetPatchDeps(patch3, [patch1.id])

    self.SetPatchApply(patch1)
    self.SetPatchApply(patch2)
    self.SetPatchApply(patch3)

    self.mox.ReplayAll()
    self.assertResults(series, patches, [patch2, patch1, patch3])
    self.mox.VerifyAll()

  def testApplyWithNotInManifestException(self):
    """Test Apply with NotInManifest Exception."""
    series = self.GetPatchSeries()

    patch1, patch2, patch3 = patches = self.GetPatches(3)
    self.SetPatchDeps(patch1, [])
    self.SetPatchDeps(patch2, [])
    self.SetPatchDeps(patch3, [])
    self.SetPatchApply(patch1)
    self.SetPatchApply(patch2)

    not_in_manifest = [cros_patch.ChangeNotInManifest(patch3)]
    series.FetchChanges = lambda changes: ([patch1, patch2], not_in_manifest)

    self.mox.ReplayAll()
    self.assertResults(series, patches, applied=[patch1, patch2],
                       failed_tot=[patch3])
    self.mox.VerifyAll()

  def testComplexCyclicalDeps(self, fail=False):
    """Verify handling of two interdependent cycles."""
    series = self.GetPatchSeries()

    # Create two cyclically interdependent patch chains.
    # Example: Two patch series A1<-A2<-A3<-A4 and B1<-B2<-B3<-B4. A1 has a
    # CQ-DEPEND on B4 and B1 has a CQ-DEPEND on A4, so all of the patches must
    # be committed together.
    chain1, chain2 = chains = self.GetPatches(4), self.GetPatches(4)
    for chain in chains:
      (other_chain,) = [x for x in chains if x != chain]
      self.SetPatchDeps(chain[0], [], cq=[other_chain[-1].id])
      for i in range(1, len(chain)):
        self.SetPatchDeps(chain[i], [chain[i-1].id])

    # Apply the second-last patch first, so that the last patch in the series
    # will be pulled in via the CQ-DEPEND on the other patch chain.
    to_apply = [chain1[-2]] + [x for x in (chain1 + chain2) if x != chain1[-2]]

    # Mark all the patches but the last ones as applied successfully.
    for patch in chain1 + chain2[:-1]:
      self.SetPatchApply(patch)

    if fail:
      # Pretend that chain2[-1] failed to apply.
      res = self.SetPatchApply(chain2[-1])
      res.AndRaise(cros_patch.ApplyPatchException(chain1[-1]))
      applied = []
      failed_tot = to_apply
    else:
      # We apply the patches in this order since the last patch in chain1
      # is pulled in via CQ-DEPEND.
      self.SetPatchApply(chain2[-1])
      applied = chain1[:2] + chain2[:-1] + chain1[2:] + chain2[-1:]
      failed_tot = []

    self.mox.ReplayAll()
    self.assertResults(series, to_apply, applied=applied, failed_tot=failed_tot)
    self.mox.VerifyAll()

  def testFailingComplexCyclicalDeps(self):
    """Verify handling of failing interlocked cycles."""
    self.testComplexCyclicalDeps(fail=True)

  def testApplyPartialFailures(self):
    """Test that can apply changes correctly when one change fails to apply.

    This tests a simple change order where 1 depends on 2 and 1 fails to apply.
    Only 1 should get tried as 2 will abort once it sees that 1 can't be
    applied.  3 with no dependencies should go through fine.

    Since patch1 fails to apply, we should also get a call to handle the
    failure.
    """
    series = self.GetPatchSeries()

    patch1, patch2, patch3, patch4 = patches = self.GetPatches(4)

    self.SetPatchDeps(patch1)
    self.SetPatchDeps(patch2, [patch1.id])
    self.SetPatchDeps(patch3)
    self.SetPatchDeps(patch4)

    self.SetPatchApply(patch1).AndRaise(
        cros_patch.ApplyPatchException(patch1))

    self.SetPatchApply(patch3)
    self.SetPatchApply(patch4).AndRaise(
        cros_patch.ApplyPatchException(patch1, inflight=True))

    self.mox.ReplayAll()
    self.assertResults(series, patches,
                       [patch3], [patch2, patch1], [patch4])
    self.mox.VerifyAll()

  def testComplexApply(self):
    """More complex deps test.

    This tests a total of 2 change chains where the first change we see
    only has a partial chain with the 3rd change having the whole chain i.e.
    1->2, 3->1->2.  Since we get these in the order 1,2,3,4,5 the order we
    should apply is 2,1,3,4,5.

    This test also checks the patch order to verify that Apply re-orders
    correctly based on the chain.
    """
    series = self.GetPatchSeries()

    patch1, patch2, patch3, patch4, patch5 = patches = self.GetPatches(5)

    self.SetPatchDeps(patch1, [patch2.id])
    self.SetPatchDeps(patch2)
    self.SetPatchDeps(patch3, [patch1.id, patch2.id])
    self.SetPatchDeps(patch4, cq=[patch5.id])
    self.SetPatchDeps(patch5)

    for patch in (patch2, patch1, patch3, patch4, patch5):
      self.SetPatchApply(patch)

    self.mox.ReplayAll()
    self.assertResults(
        series, patches, [patch2, patch1, patch3, patch5, patch4])
    self.mox.VerifyAll()

  def testApplyStandalonePatches(self):
    """Simple apply of two changes with no dependent CL's."""
    series = self.GetPatchSeries()

    patches = self.GetPatches(3)

    for patch in patches:
      self.SetPatchDeps(patch)

    for patch in patches:
      self.SetPatchApply(patch)

    self.mox.ReplayAll()
    self.assertResults(series, patches, patches)
    self.mox.VerifyAll()

  def testResetCheckouts(self):
    """Tests resetting git repositories to origin."""
    series = self.GetPatchSeries()

    repo_path, _, _ = self._CommonGitSetup()
    self.CommitFile(repo_path, 'aoeu', 'asdf')

    def _GetHeadAndRemote():
      head = git.RunGit(repo_path, ['log', 'HEAD', '-n1']).output
      remote = git.RunGit(repo_path, ['log', 'cros', '-n1']).output
      return head, remote

    head, remote = _GetHeadAndRemote()
    self.assertNotEqual(head, remote)

    series.manifest = mock.Mock()
    series.manifest.ListCheckouts.return_value = [mock.Mock(
        GetPath=mock.Mock(return_value=repo_path),
        __getitem__=lambda _self, k: {'tracking_branch': 'cros/master'}[k]
    )]

    def _MapStar(f, argss):
      return [f(*args) for args in argss]

    with mock.patch.object(parallel, 'RunTasksInProcessPool', new=_MapStar):
      series.ResetCheckouts('master')

    # verify that the checkout is reset.
    head, remote = _GetHeadAndRemote()
    self.assertEqual(head, remote)

  def TestCreateDisjointTransactions(self):
    """Test CreateDisjointTransactions."""
    series = self.GetPatchSeries()
    p = self.GetPatches(6)
    changes = p[0:5]
    ex = Exception('error transaction')
    # A -> B means A depends on B.
    # p0 -> (p1, p2)
    # p1 -> (p2)
    # p2 -> ()
    # p3 -> ()
    # p4 -> (p5)
    transactions = [(p[0], [p[0], p[1], p[2]], None),
                    (p[1], [p[1], p[2]], None),
                    (p[2], [p[2]], None),
                    (p[3], [p[3], p[4]], None),
                    (p[5], (), ex)]
    self.PatchObject(patch_series.PatchSeries, 'CreateTransactions',
                     return_value=transactions)
    ordered_plans, failed = series.CreateDisjointTransactions(changes)
    changes_in_plan = [change for plan in ordered_plans for change in plan]

    self.assertItemsEqual(changes_in_plan, p[0:5])
    self.assertItemsEqual(failed, [ex])
