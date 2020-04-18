# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the stages to handle changes."""

from __future__ import print_function

from chromite.cbuildbot import relevant_changes
from chromite.cbuildbot.stages import generic_stages
from chromite.lib import builder_status_lib
from chromite.lib import clactions
from chromite.lib import clactions_metrics
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import hwtest_results
from chromite.lib import metrics
from chromite.lib import timeout_util
from chromite.lib import tree_status


class CommitQueueHandleChangesStage(generic_stages.BuilderStage):
  """Stage that handles changes (CLs) for the Commit Queue master build.

  This stage handles changes which are applied by the CommitQueueSyncStage by
  analyzing the BuilderStatus of the master CQ and the slave CQs which are
  collected by the CommitQueueCompletionStage.
  """
  def __init__(self, builder_run, sync_stage, completion_stage, **kwargs):
    """Initialize CommitQueueHandleChangesStage."""
    super(CommitQueueHandleChangesStage, self).__init__(builder_run, **kwargs)
    assert config_lib.IsMasterCQ(self._run.config)
    self.sync_stage = sync_stage
    self.completion_stage = completion_stage

  def _RecordSubmissionMetrics(self, success=False):
    """Record CL handling statistics for submitted changes in monarch.

    Args:
      success: bool indicating whether the CQ was a success.
    """
    build_id, db = self._run.GetCIDBHandle()
    if db:
      my_actions = db.GetActionsForBuild(build_id)
      my_submit_actions = [m for m in my_actions
                           if m.action == constants.CL_ACTION_SUBMITTED]
      # A dictionary mapping from every change that was submitted to the
      # submission reason.
      submitted_change_strategies = {m.patch : m.reason
                                     for m in my_submit_actions}
      submitted_changes_all_actions = db.GetActionsForChanges(
          submitted_change_strategies.keys())

      action_history = clactions.CLActionHistory(submitted_changes_all_actions)
      logging.info('Recording submission metrics about %s CLs to monarch.',
                   len(submitted_change_strategies))
      clactions_metrics.RecordSubmissionMetrics(action_history,
                                                submitted_change_strategies)

      # Record CQ wall-clock metric.
      submitted_any = len(submitted_change_strategies) > 0
      bi = db.GetBuildStatus(build_id)
      current_time = db.GetTime()
      elapsed_seconds = int((current_time - bi['start_time']).total_seconds())
      self_destructed = self._run.attrs.metadata.GetValueWithDefault(
          constants.SELF_DESTRUCTED_BUILD, False)
      fields = {'success': success,
                'submitted_any': submitted_any,
                'self_destructed': self_destructed}

      m = metrics.Counter(constants.MON_CQ_WALL_CLOCK_SECS)
      m.increment_by(elapsed_seconds, fields=fields)

  def _GetBuildsPassedSyncStage(self, build_id, db, slave_buildbucket_ids):
    """Get builds which passed the sync stages.

    Args:
      build_id: The build id of the master build.
      db: An instance of cidb.CIDBConnection.
      slave_buildbucket_ids: A list of buildbucket_ids of the slave builds.

    Returns:
      A list of the builds (master + slaves) which passed the sync stage (See
      relevant_changes.TriageRelevantChanges.STAGE_SYNC)
    """
    assert db, 'No database connection to use.'
    build_stages_dict = {}

    # Get slave stages.
    slave_stages = db.GetSlaveStages(
        build_id, buildbucket_ids=slave_buildbucket_ids)
    for stage in slave_stages:
      build_stages_dict.setdefault(stage['build_config'], []).append(stage)

    # Get master stages.
    master_stages = db.GetBuildStages(build_id)
    for stage in master_stages:
      build_stages_dict.setdefault(self._run.config.name, []).append(stage)

    triage_relevant_changes = relevant_changes.TriageRelevantChanges
    builds_passed_sync_stage = (
        triage_relevant_changes.GetBuildsPassedAnyOfStages(
            build_stages_dict, triage_relevant_changes.STAGE_SYNC))
    return builds_passed_sync_stage

  def _CheckToTSanity(self):
    """Check and return whether Top-of-tree is healthy and the tree is opened.

    Returns:
      A boolean indicating whether ToT is sane.
    """
    tot_sanity = True

    # If the tree was not open when we acquired a pool, do not assume that
    # tot was sane.
    if not self.sync_stage.pool.tree_was_open:
      logging.info('The tree was not open when changes were acquired so we are '
                   'attributing failures to the broken tree rather than the '
                   'changes.')
      tot_sanity = False

    if tot_sanity:
      try:
        status = tree_status.WaitForTreeStatus(
            period=tree_status.DEFAULT_WAIT_FOR_TREE_STATUS_SLEEP,
            timeout=tree_status.DEFAULT_WAIT_FOR_TREE_STATUS_TIMEOUT,
            throttled_ok=True)
        tot_sanity = (status == constants.TREE_OPEN)
      except timeout_util.TimeoutError:
        logging.warning('Timed out waiting for getting tree status in %s(s).',
                        tree_status.DEFAULT_WAIT_FOR_TREE_STATUS_TIMEOUT)
        tot_sanity = False

      if not tot_sanity:
        logging.info('The tree is not open now, so we are attributing '
                     'failures to the broken tree rather than the changes.')
    return tot_sanity

  def _HandleCommitQueueFailure(self, failing, inflight, no_stat,
                                self_destructed):
    """Handle changes in the validation pool upon build failure or timeout.

    This function determines CLs to partially submit based on the passed CQs,
    also determines CLs to reject based on the category of the failures
    and whether the sanity check builder(s) passed.

    Args:
      failing: A set of build config names of builds that failed.
      inflight: A set of build config names of builds that timed out.
      no_stat: A set of build config names of builds that had status None.
      self_destructed: Boolean indicating whether the master build destructed
                       itself and stopped waiting completion of its slaves.
    """
    slave_buildbucket_ids = self.GetScheduledSlaveBuildbucketIds()

    messages = builder_status_lib.GetFailedMessages(
        self.completion_stage.GetSlaveStatuses(), failing)
    changes = self.sync_stage.pool.applied

    build_id, db = self._run.GetCIDBHandle()

    if db:
      builds_passed_sync_stage = self._GetBuildsPassedSyncStage(
          build_id, db, slave_buildbucket_ids)
      builds_not_passed_sync_stage = failing.union(inflight).union(
          no_stat).difference(builds_passed_sync_stage)
      changes_by_config = (
          relevant_changes.RelevantChanges.GetRelevantChangesForSlaves(
              build_id, db, self._run.config, changes,
              builds_not_passed_sync_stage,
              slave_buildbucket_ids, include_master=True))
      subsys_by_config = (
          relevant_changes.RelevantChanges.GetSubsysResultForSlaves(
              build_id, db))

      changes_by_slaves = changes_by_config.copy()
      # Exclude master build
      changes_by_slaves.pop(self._run.config.name, None)
      slaves_by_change = cros_build_lib.InvertDictionary(changes_by_slaves)
      passed_in_history_slaves_by_change = (
          relevant_changes.RelevantChanges.GetPreviouslyPassedSlavesForChanges(
              build_id, db, changes, slaves_by_change))

      # Even if some slaves didn't pass the critical stages, we can still submit
      # some changes based on CQ history.
      # Even if there was a failure, we can submit the changes that indicate
      # that they don't care about this failure.
      changes = self.sync_stage.pool.SubmitPartialPool(
          changes, messages, changes_by_config, subsys_by_config,
          passed_in_history_slaves_by_change, failing, inflight, no_stat)

    tot_sanity = self._CheckToTSanity()

    if not self_destructed and inflight:
      # The master didn't destruct itself and some slave(s) timed out due to
      # unknown causes, so only reject infra changes (probably just chromite
      # changes).
      self.sync_stage.pool.HandleValidationTimeout(sanity=tot_sanity,
                                                   changes=changes)
      return

    failed_hwtests = None
    if db:
      slave_statuses = db.GetSlaveStatuses(
          build_id, buildbucket_ids=slave_buildbucket_ids)
      slave_build_ids = [x['id'] for x in slave_statuses]
      failed_hwtests = (
          hwtest_results.HWTestResultManager.GetFailedHWTestsFromCIDB(
              db, slave_build_ids))

    # Some builders failed, or some builders did not report stats, or
    # the intersection of both. Let HandleValidationFailure decide
    # what changes to reject.
    self.sync_stage.pool.HandleValidationFailure(
        messages, sanity=tot_sanity, changes=changes, no_stat=no_stat,
        failed_hwtests=failed_hwtests)

  def HandleCompletionFailure(self):
    """Handle CLs and record metrics when completion_stage failed with fatal."""
    self_destructed = self._run.attrs.metadata.GetValueWithDefault(
        constants.SELF_DESTRUCTED_BUILD, False)
    important_build_statuses = self.completion_stage.GetSlaveStatuses()
    no_stat = builder_status_lib.BuilderStatusesFetcher.GetNostatBuilds(
        important_build_statuses)
    failing = builder_status_lib.BuilderStatusesFetcher.GetFailingBuilds(
        important_build_statuses)
    inflight = builder_status_lib.BuilderStatusesFetcher.GetInflightBuilds(
        important_build_statuses)
    self._HandleCommitQueueFailure(failing, inflight, no_stat, self_destructed)
    self._RecordSubmissionMetrics(False)

  def HandleCompletionSuccess(self):
    """Handle CLs and record metrics when completion_stage succeeded."""
    self.sync_stage.pool.SubmitPool(reason=constants.STRATEGY_CQ_SUCCESS)
    self._RecordSubmissionMetrics(True)

  def PerformStage(self):
    super(CommitQueueHandleChangesStage, self).PerformStage()

    if self.completion_stage.GetFatal():
      self.HandleCompletionFailure()
    else:
      self.HandleCompletionSuccess()
