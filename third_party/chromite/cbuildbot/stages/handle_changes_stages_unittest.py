# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the unit tests for handle_changes_stages."""

from __future__ import print_function

import itertools
import mock

from chromite.cbuildbot import relevant_changes
from chromite.cbuildbot.stages import handle_changes_stages
from chromite.cbuildbot.stages import generic_stages
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.cbuildbot.stages import sync_stages
from chromite.lib import builder_status_lib
from chromite.lib import cidb
from chromite.lib import clactions
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import fake_cidb
from chromite.lib import hwtest_results
from chromite.lib import timeout_util
from chromite.lib import tree_status
from chromite.lib.const import waterfall


# pylint: disable=protected-access


class CommitQueueHandleChangesStageTests(
    generic_stages_unittest.AbstractStageTestCase):
  """Tests for CommitQueueHandleChangesStag."""

  BOT_ID = 'master-paladin'

  def setUp(self):
    self._Prepare()

    self.partial_submit_changes = ['A', 'B']
    self.other_changes = ['C', 'D']
    self.changes = self.other_changes + self.partial_submit_changes
    self.PatchObject(builder_status_lib, 'GetFailedMessages')
    self.PatchObject(relevant_changes.RelevantChanges,
                     '_GetSlaveMappingAndCLActions',
                     return_value=(dict(), []))
    self.PatchObject(clactions, 'GetRelevantChangesForBuilds')
    self.PatchObject(tree_status, 'WaitForTreeStatus',
                     return_value=constants.TREE_OPEN)
    self.PatchObject(relevant_changes.RelevantChanges,
                     'GetPreviouslyPassedSlavesForChanges')
    self.mock_record_metrics = self.PatchObject(
        handle_changes_stages.CommitQueueHandleChangesStage,
        '_RecordSubmissionMetrics')

    self.sync_stage = self._MockSyncStage()
    self.completion_stage = mock.Mock()

  def tearDown(self):
    cidb.CIDBConnectionFactory.ClearMock()

  def _MockSyncStage(self, tree_was_open=True):
    sync_stage = sync_stages.CommitQueueSyncStage(self._run)
    sync_stage.pool = mock.MagicMock()
    sync_stage.pool.applied = self.changes
    sync_stage.pool.tree_was_open = tree_was_open

    sync_stage.pool.handle_failure_mock = self.PatchObject(
        sync_stage.pool, 'HandleValidationFailure')
    sync_stage.pool.handle_timeout_mock = self.PatchObject(
        sync_stage.pool, 'HandleValidationTimeout')
    sync_stage.pool.submit_pool_mock = self.PatchObject(
        sync_stage.pool, 'SubmitPool')

    return sync_stage

  # pylint: disable=W0221
  def ConstructStage(self, sync_stage=None, completion_stage=None):
    sync_stage = sync_stage or self.sync_stage
    completion_stage = completion_stage or self.completion_stage

    return handle_changes_stages.CommitQueueHandleChangesStage(
        self._run, sync_stage, completion_stage)

  def test_GetBuildsPassedSyncStage(self):
    """Test _GetBuildsPassedSyncStage."""
    stage = self.ConstructStage()
    mock_cidb = mock.Mock()
    mock_cidb.GetSlaveStages.return_value = [
        {'build_config': 's_1', 'status': 'pass', 'name': 'CommitQueueSync'},
        {'build_config': 's_2', 'status': 'pass', 'name': 'CommitQueueSync'},
        {'build_config': 's_3', 'status': 'fail', 'name': 'CommitQueueSync'}]
    mock_cidb.GetBuildStages.return_value = [
        {'status': 'pass', 'name': 'CommitQueueSync'}]

    builds = stage._GetBuildsPassedSyncStage(
        'build_id', mock_cidb, ['id_1', 'id_2'])
    self.assertItemsEqual(builds, ['s_1', 's_2', 'master-paladin'])

  def _MockPartialSubmit(self, stage):
    self.PatchObject(relevant_changes.RelevantChanges,
                     'GetRelevantChangesForSlaves',
                     return_value={'master-paladin': {mock.Mock()}})
    self.PatchObject(relevant_changes.RelevantChanges,
                     'GetSubsysResultForSlaves')
    self.PatchObject(handle_changes_stages.CommitQueueHandleChangesStage,
                     '_GetBuildsPassedSyncStage')
    stage.sync_stage.pool.SubmitPartialPool.return_value = self.changes

  def testHandleCommitQueueFailureWithOpenTree(self):
    """Test _HandleCommitQueueFailure with open tree."""
    stage = self.ConstructStage()
    self._MockPartialSubmit(stage)
    self.PatchObject(tree_status, 'WaitForTreeStatus',
                     return_value=constants.TREE_OPEN)
    self.PatchObject(generic_stages.BuilderStage,
                     'GetScheduledSlaveBuildbucketIds', return_value=[])

    stage._HandleCommitQueueFailure(set(['test1']), set(), set(), False)
    stage.sync_stage.pool.handle_failure_mock.assert_called_once_with(
        mock.ANY, sanity=True, no_stat=set(), changes=self.changes,
        failed_hwtests=None)

  def testHandleCommitQueueFailureWithThrottledTree(self):
    """Test _HandleCommitQueueFailure with throttled tree."""
    stage = self.ConstructStage()
    self._MockPartialSubmit(stage)
    self.PatchObject(tree_status, 'WaitForTreeStatus',
                     return_value=constants.TREE_THROTTLED)
    self.PatchObject(generic_stages.BuilderStage,
                     'GetScheduledSlaveBuildbucketIds', return_value=[])

    stage._HandleCommitQueueFailure(set(['test1']), set(), set(), False)
    stage.sync_stage.pool.handle_failure_mock.assert_called_once_with(
        mock.ANY, sanity=False, no_stat=set(), changes=self.changes,
        failed_hwtests=None)

  def testHandleCommitQueueFailureWithClosedTree(self):
    """Test _HandleCommitQueueFailure with closed tree."""
    stage = self.ConstructStage()
    self._MockPartialSubmit(stage)
    self.PatchObject(tree_status, 'WaitForTreeStatus',
                     side_effect=timeout_util.TimeoutError())
    self.PatchObject(generic_stages.BuilderStage,
                     'GetScheduledSlaveBuildbucketIds', return_value=[])

    stage._HandleCommitQueueFailure(set(['test1']), set(), set(), False)
    stage.sync_stage.pool.handle_failure_mock.assert_called_once_with(
        mock.ANY, sanity=False, no_stat=set(), changes=self.changes,
        failed_hwtests=None)

  def testHandleCommitQueueFailureWithFailedHWtests(self):
    """Test _HandleCommitQueueFailure with failed HWtests."""
    stage = self.ConstructStage()
    self._MockPartialSubmit(stage)
    master_build_id = stage._run.attrs.metadata.GetValue('build_id')
    db = fake_cidb.FakeCIDBConnection()
    slave_build_id = db.InsertBuild(
        'slave_1', waterfall.WATERFALL_INTERNAL, 1, 'slave_1', 'bot_hostname',
        master_build_id=master_build_id, buildbucket_id='123')
    cidb.CIDBConnectionFactory.SetupMockCidb(db)
    mock_failed_hwtests = mock.Mock()
    mock_get_hwtests = self.PatchObject(
        hwtest_results.HWTestResultManager,
        'GetFailedHWTestsFromCIDB', return_value=mock_failed_hwtests)
    self.PatchObject(tree_status, 'WaitForTreeStatus',
                     return_value=constants.TREE_OPEN)
    self.PatchObject(generic_stages.BuilderStage,
                     'GetScheduledSlaveBuildbucketIds', return_value=['123'])

    stage._HandleCommitQueueFailure(set(['test1']), set(), set(), False)
    stage.sync_stage.pool.handle_failure_mock.assert_called_once_with(
        mock.ANY, sanity=True, no_stat=set(), changes=self.changes,
        failed_hwtests=mock_failed_hwtests)
    mock_get_hwtests.assert_called_once_with(db, [slave_build_id])

  def VerifyStage(self, failing, inflight, no_stat, handle_failure=False,
                  handle_timeout=False, sane_tot=True, stage=None,
                  all_slaves=None, slave_stages=None, fatal=True,
                  self_destructed=False):
    """Runs and Verifies PerformStage.

    Args:
      failing: The names of the builders that failed.
      inflight: The names of the buiders that timed out.
      no_stat: The names of the builders that had no status.
      handle_failure: If True, calls HandleValidationFailure.
      handle_timeout: If True, calls HandleValidationTimeout.
      sane_tot: If not true, assumes TOT is not sane.
      stage: If set, use this constructed stage, otherwise create own.
      all_slaves: Optional set of all slave configs.
      slave_stages: Optional list of slave stages.
      fatal: Optional boolean indicating whether the completion_stage failed
        with fatal. Default to True.
      self_destructed: Optional boolean indicating whether the completion_stage
        self_destructed. Default to False.
    """
    if not stage:
      stage = self.ConstructStage()

    stage._run.attrs.metadata.UpdateWithDict(
        {constants.SELF_DESTRUCTED_BUILD: self_destructed})

    # Setup the stage to look at the specified configs.
    all_slaves = list(all_slaves or set(failing + inflight + no_stat))
    all_started_slaves = list(all_slaves or set(failing + inflight))
    configs = [config_lib.BuildConfig(name=x) for x in all_slaves]
    self.PatchObject(stage, '_GetSlaveConfigs', return_value=configs)

    statuses = {}
    for x in failing:
      statuses[x] = builder_status_lib.BuilderStatus(
          constants.BUILDER_STATUS_FAILED, message=None)
    for x in inflight:
      statuses[x] = builder_status_lib.BuilderStatus(
          constants.BUILDER_STATUS_INFLIGHT, message=None)
    for x in no_stat:
      statuses[x] = builder_status_lib.BuilderStatus(
          constants.BUILDER_STATUS_MISSING, message=None)
    self.completion_stage.GetSlaveStatuses.return_value = statuses
    self.completion_stage.GetFatal.return_value = fatal

    # Setup DB and provide list of slave stages.
    mock_cidb = mock.MagicMock()
    cidb.CIDBConnectionFactory.SetupMockCidb(mock_cidb)
    if slave_stages is None:
      slave_stages = []
      critical_stages = (
          relevant_changes.TriageRelevantChanges.STAGE_SYNC)
      for stage_name, slave in itertools.product(
          critical_stages, all_started_slaves):
        slave_stages.append({'name': stage_name,
                             'build_config': slave,
                             'status': constants.BUILDER_STATUS_PASSED})
    self.PatchObject(mock_cidb, 'GetSlaveStages', return_value=slave_stages)

    # Set up SubmitPartialPool to provide a list of changes to look at.
    self.PatchObject(stage.sync_stage.pool, 'SubmitPartialPool',
                     return_value=self.other_changes)

    # Actually run the stage.
    stage.PerformStage()

    if fatal:
      stage.sync_stage.pool.submit_pool_mock.assert_not_called()
      self.mock_record_metrics.assert_called_once_with(False)
    else:
      stage.sync_stage.pool.submit_pool_mock.assert_called_once_with(
          reason=constants.STRATEGY_CQ_SUCCESS)
      self.mock_record_metrics.assert_called_once_with(True)

    if handle_failure:
      stage.sync_stage.pool.handle_failure_mock.assert_called_once_with(
          mock.ANY, no_stat=set(no_stat), sanity=sane_tot,
          changes=self.other_changes, failed_hwtests=mock.ANY)

    if handle_timeout:
      stage.sync_stage.pool.handle_timeout_mock.assert_called_once_with(
          sanity=mock.ANY, changes=self.other_changes)

  def testCompletionSuccess(self):
    """Verify stage when the completion_stage succeeded."""
    self.VerifyStage([], [], [], fatal=False)

  def testCompletionWithInflightSlaves(self):
    """Verify stage when the completion_stage failed with inflight slaves."""
    self.VerifyStage([], ['foo'], [], handle_timeout=True)

  def testCompletionSelfDestructedWithInflightSlaves(self):
    """Verify stage when the completion_stage self_destructed with inflight."""
    self.VerifyStage([], ['foo'], [], self_destructed=True, handle_failure=True)

  def testCompletionSelfDestructedWithFailingSlaves(self):
    """Verify stage when the completion_stage self_destructed with failing."""
    self.VerifyStage(['foo'], [], [], self_destructed=True, handle_failure=True)

  def testCompletionSelfDestructedWithdNoStatSlaves(self):
    """Verify stage when the completion_stage self_destructed with no_stat."""
    self.VerifyStage([], [], ['foo'], self_destructed=True, handle_failure=True)
