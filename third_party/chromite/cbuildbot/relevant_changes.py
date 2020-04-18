# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for tracking relevant changes (i.e. CLs) to validate."""

from __future__ import print_function

import datetime

from chromite.cbuildbot.stages import artifact_stages
from chromite.lib import builder_status_lib
from chromite.lib import clactions
from chromite.lib import constants
from chromite.lib import config_lib
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import patch as cros_patch
from chromite.lib import triage_lib


# Limit (hours) for looking back cl actions in the history for history-aware
# submission.
CQ_HISTORY_LOOKBACK_LIMIT_HOUR = 48


site_config = config_lib.GetConfig()


class RelevantChanges(object):
  """Class that quries and tracks relevant changes."""

  @classmethod
  def _GetSlaveMappingAndCLActions(cls, master_build_id, db, config, changes,
                                   slave_buildbucket_ids, include_master=False):
    """Query CIDB to for slaves and CL actions.

    Args:
      master_build_id: Build id of this master build.
      db: Instance of cidb.CIDBConnection.
      config: Instance of config_lib.BuildConfig of this build.
      changes: A list of GerritPatch instances to examine.
      slave_buildbucket_ids: A list of buildbucket_ids (strings) of slave builds
                             scheduled by Buildbucket.
      include_master: Boolean indicating whether to include the master build in
                      the config_map. Default to False.

    Returns:
      A tuple of (config_map, action_history). The config_map is a dictionary
      mapping build_id to config name for all slaves in this run. If
      include_master is True, the config_map also includes master build. The
      action_history is a list of all CL actions associated with |changes|.
    """
    assert db, 'No database connection to use.'
    assert config.master, 'This is not a master build.'

    slave_list = db.GetSlaveStatuses(
        master_build_id, buildbucket_ids=slave_buildbucket_ids)

    # TODO(akeshet): We are getting the full action history for all changes that
    # were in this CQ run. It would make more sense to only get the actions from
    # build_ids of this master and its slaves.
    action_history = db.GetActionsForChanges(changes)

    config_map = dict()

    for d in slave_list:
      config_map[d['id']] = d['build_config']

    if include_master:
      config_map[master_build_id] = config.name

    return config_map, action_history

  @classmethod
  def GetRelevantChangesForSlaves(cls, master_build_id, db, config, changes,
                                  builds_not_passed_sync_stage,
                                  slave_buildbucket_ids,
                                  include_master=False):
    """Compile a set of relevant changes for each slave.

    Args:
      master_build_id: Build id of this master build.
      db: Instance of cidb.CIDBConnection.
      config: Instance of config_lib.BuildConfig of this build.
      changes: A list of GerritPatch instances to examine.
      builds_not_passed_sync_stage: Set of build config names of slaves that
        didn't pass the sync stages.
      slave_buildbucket_ids: A list of buildbucket_ids (strings) of slave builds
                             scheduled by Buildbucket.
      include_master: Boolean indicating whether to include the master build in
                      the config_map. Default to False.

    Returns:
      A dictionary mapping a slave config name to a set of relevant changes
      (as GerritPatch instances). If include_master is True, the dictionary
      includes the master build config and its relevant changes.
    """
    # Retrieve the slaves and clactions from CIDB.
    config_map, action_history = cls._GetSlaveMappingAndCLActions(
        master_build_id, db, config, changes, slave_buildbucket_ids,
        include_master=include_master)
    changes_by_build_id = clactions.GetRelevantChangesForBuilds(
        changes, action_history, config_map.keys())

    # Convert index from build_ids to config names.
    changes_by_config = dict()
    for k, v in changes_by_build_id.iteritems():
      changes_by_config[config_map[k]] = v

    for config in builds_not_passed_sync_stage:
      # If a slave did not passed the sync stage, it means that the slave never
      # finished applying the changes in the sync stage. Hence the CL pickup
      # actions for this slave may be inaccurate. Conservatively assume all
      # changes are relevant.
      changes_by_config[config] = set(changes)

    return changes_by_config

  @classmethod
  def GetSubsysResultForSlaves(cls, master_build_id, db):
    """Get the pass/fail HWTest subsystems results for each slave.

    Returns:
      A dictionary mapping a slave config name to a dictionary of the pass/fail
      subsystems. E.g.
      {'foo-paladin': {'pass_subsystems':{'A', 'B'},
                       'fail_subsystems':{'C'}}}
    """
    assert db, 'No database connection to use.'
    slave_msgs = db.GetSlaveBuildMessages(master_build_id)
    slave_subsys_msgs = ([m for m in slave_msgs
                          if m['message_type'] == constants.SUBSYSTEMS])
    subsys_by_config = dict()
    group_msg_by_config = cros_build_lib.GroupByKey(slave_subsys_msgs,
                                                    'build_config')
    for config, dict_list in group_msg_by_config.iteritems():
      d = subsys_by_config.setdefault(config, {})
      subsys_groups = cros_build_lib.GroupByKey(dict_list, 'message_subtype')
      for k, v in subsys_groups.iteritems():
        if k == constants.SUBSYSTEM_PASS:
          d['pass_subsystems'] = set([x['message_value'] for x in v])
        if k == constants.SUBSYSTEM_FAIL:
          d['fail_subsystems'] = set([x['message_value'] for x in v])
        # If message_subtype==subsystem_unused, keep d as an empty dict.
    return subsys_by_config

  @classmethod
  def GetPreviouslyPassedSlavesForChanges(
      cls, master_build_id, db, changes, change_relevant_slaves_dict,
      history_lookback_limit=CQ_HISTORY_LOOKBACK_LIMIT_HOUR):
    """Get slaves passed in history (not from current run) for changes.

    If a previous slave build:
    1) inserted constants.CL_ACTION_RELEVANT_TO_SLAVE cl action for a change;
    2) is a passed build;
    3) is a relevant slave of the change
    this slave is considered as a previously passed slave.

    Args:
      master_build_id: The build id of current master to get current slaves.
      db: An instance of cidb.CIDBConnection.
      changes: A list of cros_patch.GerritPatch instance to check.
      change_relevant_slaves_dict: A dict mapping changes to their relevant
        slaves in current run.
      history_lookback_limit: Limit (hours) for looking back cl actions in the
        histor. If it's None, do not force the limit.
        Default to CQ_HISTORY_LOOKBACK_LIMIT_HOUR.

    Returns:
      A dict mapping changes (cros_patch.GerritPatch instances) to sets of
      of build config name (strings) of their relevant slaves which passed in
      history.
    """
    assert db, 'No database connection to use.'
    current_slaves = db.GetSlaveStatuses(master_build_id)
    current_slave_build_ids = [x['id'] for x in current_slaves]

    valid_configs = set()
    for relevant_slaves in change_relevant_slaves_dict.values():
      valid_configs.update(relevant_slaves)

    changes_dict = {clactions.GerritPatchTuple(int(change.gerrit_number),
                                               int(change.patch_number),
                                               change.internal):
                    change for change in changes}

    start_time = None
    if history_lookback_limit is not None:
      start_time = (datetime.datetime.now() -
                    datetime.timedelta(hours=history_lookback_limit))

    actions = db.GetActionsForChanges(
        changes,
        ignore_patch_number=False,
        status=constants.BUILDER_STATUS_PASSED,
        action=constants.CL_ACTION_RELEVANT_TO_SLAVE,
        start_time=start_time)

    change_passed_slaves_dict = {}
    for action in actions:
      if (action.build_config in valid_configs and
          action.build_id not in current_slave_build_ids):
        change = changes_dict.get(action.patch)
        if change:
          change_passed_slaves_dict.setdefault(change, set()).add(
              action.build_config)

    return change_passed_slaves_dict


class TriageRelevantChanges(object):
  """Class to triage relevant changes within a CQ run..

  This class keeps track of relevant_changes of a list slave builds of given a
  master build. With the build information fetched from Buildbucket and CIDB,
  it performs relevant change triages, and returns a ShouldWait flag indicating
  whether it's still meaningful for the master build to wait for the slave
  builds. The triages include anaylizing whether the failed slave builds have
  passed the critial sync stage, whether the failures in failed slave builds
  are ignorable for changes, classifying changes into will_submit, might_submit
  and will_not_submit sets, and so on.
  More context: go/self-destructed-commit-queue
  """

  # Accepted statues of the critical stages
  ACCEPTED_STATUSES = {
      constants.BUILDER_STATUS_PASSED,
      constants.BUILDER_STATUS_SKIPPED
  }

  # TODO(nxia): crbug.com/694749
  # Get stage names from stage classes instead of duplicating them here.
  COMMIT_QUEUE_SYNC = 'CommitQueueSync'
  MASTER_SLAVE_LKGM_SYNC = 'MasterSlaveLKGMSync'
  STAGE_SYNC = {COMMIT_QUEUE_SYNC, MASTER_SLAVE_LKGM_SYNC}

  STAGE_UPLOAD_PREBUILTS = (
      artifact_stages.UploadPrebuiltsStage.StageNamePrefix())

  def __init__(self, master_build_id, db, builders_array, config, metadata,
               version, build_root, changes, buildbucket_info_dict,
               cidb_status_dict, completed_builds, dependency_map,
               buildbucket_client, dry_run=True):
    """Initialize an instance of TriageRelevantChanges.

    Args:
      master_build_id: The build_id of the master build.
      db: An instance of cidb.CIDBConnection to fetch data from CIDB.
      builders_array: A list of expected slave build config names (strings).
      config: An instance of config_lib.BuildConfig. Config dict of this build.
      metadata: Instance of metadata_lib.CBuildbotMetadata. Metadata of this
                build.
      version: Current manifest version string. See the return type of
        VersionInfo.VersionString().
      build_root: Path to the build root directory.
      changes: A list of changes (GerritPatch instances) which have been applied
        to this build.
      buildbucket_info_dict: A dict mapping all slave build config names to
        their BuildbucketInfos (See SlaveBuilerStatus.GetAllSlaveBuildbucketInfo
        for details).
      cidb_status_dict: A dict mapping all slave build config names to their
        CIDBStatusInfos (See SlaveBuilerStatus.GetAllSlaveCIDBStatusInfo for
        details)
      completed_builds: A set of slave build config names (strings) which
        have completed and will not be retried.
      dependency_map: A dict mapping a change (patch.GerritPatch instance) to a
        set of changes (patch.GerritPatch instances) depending on this change.
        (See ValidationPool.GetDependMapForChanges for details.)
      buildbucket_client: Instance of buildbucket_lib.buildbucket_client.
      dry_run: Boolean indicating whether it's a dry run. Default to True.
    """
    self.master_build_id = master_build_id
    self.db = db
    self.builders_array = builders_array
    self.config = config
    self.metadata = metadata
    self.version = version
    self.buildbucket_info_dict = buildbucket_info_dict
    self.cidb_status_dict = cidb_status_dict
    self.completed_builds = completed_builds
    self.build_root = build_root
    self.changes = changes
    self.dependency_map = dependency_map
    self.buildbucket_client = buildbucket_client
    self.dry_run = dry_run

    # Dict mapping slave config names to a list of stages
    self.slave_stages_dict = None
    # Dict mapping slave config names to relevant change sets.
    self.slave_changes_dict = None
    # Dict mapping slave config names to subsys sets.
    self.slave_subsys_dict = None

    # A set of changes which will be submitted by the master.
    self.will_submit = set()
    # A set of changes which are being tested by the slaves.
    self.might_submit = set(self.changes)
    # A set of chagnes which won't be submitted by the master.
    self.will_not_submit = set()

    # A dict mapping build config name to a set of changes which can ignore the
    # failures in the build.
    self.build_ignorable_changes_dict = {}

    # A dict mapping changes to their relevant slaves (build_configs).
    self.change_relevant_slaves_dict = None

    # A dict mapping changes to their passed slaves (build_configs) in history.
    self.change_passed_slaves_dict = None

    self._UpdateSlaveInfo()

  def _UpdateSlaveInfo(self):
    """Update slave infomation with stages, relevant_changes, and subsys."""
    self.slave_stages_dict = self.GetSlaveStages(
        self.master_build_id, self.db, self.buildbucket_info_dict)
    self.slave_changes_dict = self._GetRelevantChanges(
        self.slave_stages_dict)
    self.slave_subsys_dict = RelevantChanges.GetSubsysResultForSlaves(
        self.master_build_id, self.db)
    self.change_relevant_slaves_dict = cros_build_lib.InvertDictionary(
        self.slave_changes_dict)
    self.change_passed_slaves_dict = (
        RelevantChanges.GetPreviouslyPassedSlavesForChanges(
            self.master_build_id, self.db, self.changes,
            self.change_relevant_slaves_dict))

  @staticmethod
  def GetDependChanges(changes, dependency_map):
    """Get a set of changes depending on the given changes.

    Args:
      changes: A set of changes to get the dependent change set.
      dependency_map: A dict mapping a change (patch.GerritPatch instance) to a
        set of changes (patch.GerritPatch instances) directly or indirectly
        depending on this change. (See ValidationPool.GetDependMapForChanges for
        details.)

    Returns:
      A set of all changes directly or indirectly depending on the given
      changes.
    """
    return set().union(*[dependency_map.get(c, set()) for c in changes])

  # TODO(nxia): Consolidate with completion_stages._ShouldSubmitPartialPool
  @staticmethod
  def GetSlaveStages(master_build_id, db, buildbucket_info_dict):
    """Get slave stages from CIDB.

    Args:
      master_build_id: The build_id of the master build.
      db: An instance of cidb.CIDBConnection to fetch data from CIDB.
      buildbucket_info_dict: A dict mapping all slave build config names to
        their BuildbucketInfos (See SlaveStatus.GetAllSlaveBuildbucketInfo
        for details).

    Returns:
      A dict mapping all slave config names (strings) to their stages (a list
        of dicts, see cidb.CIDBConnection.GetSlaveStages for details.)
    """
    assert db, 'No database connection to use.'

    slave_stages_dict = {}
    slave_buildbucket_ids = []

    if buildbucket_info_dict is not None:
      for slave_config, buildbucket_info in buildbucket_info_dict.iteritems():
        # Set default value for all slaves, some may not have stages in CIDB.
        slave_stages_dict.setdefault(slave_config, [])
        slave_buildbucket_ids.append(buildbucket_info.buildbucket_id)

    stages = db.GetSlaveStages(master_build_id,
                               buildbucket_ids=slave_buildbucket_ids)
    for stage in stages:
      slave_stages_dict[stage['build_config']].append(stage)

    return slave_stages_dict

  @classmethod
  def PassedAnyOfStages(cls, stages, desired_stages):
    """Check if the stages have passed any stage from desired_stages.

    Args:
      stages: A list of stages (see the type of slave_stages_dict value part)
        to check.
      desired_stages: A set of desired stages (strings).

    Returns:
      True if the accepted stages in the given stages cover any stage in
      the desired_stages set; else, False.
    """
    accepted_stages = {stage['name'] for stage in stages
                       if stage['status'] in cls.ACCEPTED_STATUSES}

    return accepted_stages.intersection(desired_stages)

  @classmethod
  def GetBuildsPassedAnyOfStages(cls, build_stages_dict, desired_stages):
    """Get builds which have passed any stage from desired_stages.

    Args:
      build_stages_dict: A dict mapping build config names to their stage lists.
      desired_stages: A set of desired stages (strings).

    Returns:
      A set of build config names (strings) which have passed any stage in
      desired_stages.
    """
    return set(build_config
               for build_config, stages in build_stages_dict.iteritems()
               if cls.PassedAnyOfStages(stages, desired_stages))

  def _GetRelevantChanges(self, slave_stages_dict):
    """Get relevant changes for slave builds.

    Args:
      slave_stages_dict: A dict mapping slaves config names (strings) to their
        stage lists. (see GetSlaveStages for details).

    Returns:
      A dict mapping all slave config names (strings) to sets of changes which
      are relevant to the slave builds. If a build has passed the STAGE_SYNC
      stage, it has recorded the CLs it picked up in the CIDB, it's mapped to
      its relevant change set. If a build failed to pass the STAGE_SYNC stage,
      we assume it's relevant to all changes, so it's mapped to the change set
      containing all the applied changes.
    """
    # If a build passed the sync stage, the picked up change stats are recorded.
    builds_passed_sync_stage = self.GetBuildsPassedAnyOfStages(
        slave_stages_dict, self.STAGE_SYNC)
    builds_not_passed_sync_stage = (set(self.buildbucket_info_dict.keys()) -
                                    builds_passed_sync_stage)
    slave_buildbucket_ids = [bb_info.buildbucket_id
                             for bb_info in self.buildbucket_info_dict.values()]
    slave_changes_dict = RelevantChanges.GetRelevantChangesForSlaves(
        self.master_build_id, self.db, self.config, self.changes,
        builds_not_passed_sync_stage, slave_buildbucket_ids)

    # Some slaves may not pick up any changes, update the value to set()
    for slave_config in self.buildbucket_info_dict:
      slave_changes_dict.setdefault(slave_config, set())

    return slave_changes_dict

  def _GetIgnorableChanges(self, build_config, builder_status,
                           relevant_changes):
    """Get changes that can ignore failures in BuilderStatus.

    Some projects are configured with ignored-stages in COMMIT_QUEUE.ini. The CQ
    can still submit changes from these projects if all failed statges are
    listed in ignored-stages. Please refer to
    cq_config.CQConfigParser.GetStagesToIgnore for more details.

    1) if the builder_status is in 'pass' status, it means the build uploaded a
    'pass' builder_status but failed other steps in or after the completion
    stage. This is rare but still possible, and it should not blame any changes
    as the build has finishes its testing. Returns all changes in
    relevant_changes in this case.
    2) else if the builder_status is in 'fail' with failure messages, it
    calculates and returns all ignorable changes given the failure messages.
    3) else, the builder_status is either in 'fail' status without failure
    messages or in one of the 'inflight' and 'missing' statuses. It cannot
    calculate ignorable changes without any failure message so should just
    return an empty set.

    Args:
      build_config: The config name (string) of the build.
      builder_status: An instance of build_status.BuilderStatus.
      relevant_changes: A set of relevant changes for triage to get the
        ignorable changes.

    Returns:
      A set of ignorable changes (GerritPatch instances).
    """
    if builder_status.Passed():
      return relevant_changes
    elif builder_status.Failed() and builder_status.message:
      ignoreable_changes = set()
      for change in relevant_changes:
        ignore_result = triage_lib.CalculateSuspects.CanIgnoreFailures(
            [builder_status.message], change, self.build_root,
            self.slave_subsys_dict)

        if ignore_result[0]:
          logging.debug('change %s is ignoreable for failures of %s.',
                        cros_patch.GetChangesAsString([change]), build_config)
          ignoreable_changes.add(change)
      return ignoreable_changes
    else:
      return set()

  def _UpdateWillNotSubmitChanges(self, will_not_submit):
    """Update will_not_submit change set.

    Args:
      will_not_submit: A set of changes (GerritPatch instances) to add to
        will_not_submit.
    """
    self.will_not_submit.update(will_not_submit)
    self.might_submit.difference_update(will_not_submit)

  def _GetWillNotSubmitChanges(self, build_config, changes):
    """Get changes which will not be submitted because of the failed build.

    Get a list of changes which will not be submitted because the build_config
    is relevant to the changes, and the build of this build_config didn't pass
    in current run and the failures are not ignorable, and there's no passed
    build of this build_config in history.

    Args:
      build_config: The build config name (string) of the failed build.
      changes: A list of cros_patch.GerritPatch instances to check.

    Returns:
      A list of changes (cros_patch.GerritPatch) which will not be submitted.
    """
    will_not_submit_changes = set()
    for change in changes:
      if (build_config in self.change_relevant_slaves_dict.get(change, set())
          and build_config not in self.change_passed_slaves_dict.get(
              change, set())):
        will_not_submit_changes.add(change)

    return will_not_submit_changes

  def _ProcessCompletedBuilds(self):
    """Process completed and not retriable builds.

    This method goes through all the builds which completed without SUCCESS
    result and will not be retried.
    1) if the failed build didn't pass the sync stage, iterate all changes,
    move the changes which didn't pass this build config in history to
    will_not_submit (as well as their dependencies).
    2) else, get BuilderStatus for the build (if there's no BuilderStatus
    pickle file in GS, a BuilderStatus with 'missing' status will be returned).
    Find not ignorable changes given the BuilderStatus, iterate the changes in
    not ignorable changes, move the changes which didn't pass this build config
    in history to will_not_submit (as well as their dependencies).
    """
    # TODO(nxia): Improve SlaveBuilderStatus to take buildbucket_info_dict
    # and cidb_status_dict as arguments to avoid extra queries.
    slave_builder_statuses = builder_status_lib.SlaveBuilderStatus(
        self.master_build_id, self.db, self.config, self.metadata,
        self.buildbucket_client, self.builders_array, self.dry_run)

    for build_config, bb_info in self.buildbucket_info_dict.iteritems():
      if (build_config in self.completed_builds and
          bb_info.status == constants.BUILDBUCKET_BUILDER_STATUS_COMPLETED and
          bb_info.result != constants.BUILDBUCKET_BUILDER_RESULT_SUCCESS):
        # This build didn't succeed and cannot be retried.
        logging.info('Processing relevant changes of build %s status %s '
                     'result %s', build_config, bb_info.status, bb_info.result)

        stages = self.slave_stages_dict[build_config]
        if not self.PassedAnyOfStages(stages, self.STAGE_SYNC):
          # The build_config didn't pass any of the sync stages. Find changes
          # which don't have valid passed builds of this build_config in
          # history. Move the changes and their dependencies to will_not_submit
          # set.
          will_not_submit_changes = self._GetWillNotSubmitChanges(
              build_config, self.changes)
          depend_changes = self.GetDependChanges(
              will_not_submit_changes, self.dependency_map)
          will_not_submit_changes |= depend_changes

          if will_not_submit_changes:
            self._UpdateWillNotSubmitChanges(will_not_submit_changes)
            logging.info('Build %s didn\'t pass any stage in %s. Will not'
                         ' submit changes: %s', build_config, self.STAGE_SYNC,
                         cros_patch.GetChangesAsString(will_not_submit_changes))
        else:
          # The build passed the required sync stage. Get builder_status and
          # get not ignorable changes based on the builder_status. Find changes
          # in the not ignorable changes don't have valid passed builds of this
          # build_config hi history. Move the changes and their dependencies to
          # will_not_submit set.
          relevant_changes = self.slave_changes_dict[build_config]
          builder_status = slave_builder_statuses.GetBuilderStatusForBuild(
              build_config)
          ignorable_changes = self._GetIgnorableChanges(
              build_config, builder_status, relevant_changes)
          self.build_ignorable_changes_dict[build_config] = ignorable_changes
          not_ignorable_changes = relevant_changes - ignorable_changes

          will_not_submit_changes = self._GetWillNotSubmitChanges(
              build_config, not_ignorable_changes)
          depend_changes = self.GetDependChanges(
              will_not_submit_changes, self.dependency_map)
          will_not_submit_changes = will_not_submit_changes | depend_changes

          if will_not_submit_changes:
            self._UpdateWillNotSubmitChanges(will_not_submit_changes)
            logging.info('Build %s failed with not ignorable failures, will not'
                         ' submit changes: %s', build_config,
                         cros_patch.GetChangesAsString(will_not_submit_changes))

        if not self.might_submit:
          # No need to process other completed builds, might_submit is empty.
          return

  def _ChangeVerifiedByCurrentBuild(self, change, build_config,
                                    buildbucket_info_dict,
                                    build_ignorable_changes_dict):
    """Whether the change can by verified by the current build of build_config.

    A change can be verified by the build if it satisfies the conditions:
    1) the build successfully completed; OR
    2) the build failed but its failures can be ignored by the change.

    Args:
      change: An instance of cros_patch.GerritPatch to check.
      build_config: The build config name (string) (relevant to the change) to
        verify.
      buildbucket_info_dict: A dict mapping all slave build config names to
        their BuildbucketInfos (See SlaveBuilerStatus.GetAllSlaveBuildbucketInfo
        for details).
      build_ignorable_changes_dict: A dict mapping build config name (string) to
        a set of changes (GerritPatch instances) which can ignore the failures
        in the build.

    Returns:
      True if the change can be verified by the current build of build_config;
      else, False.
    """
    bb_info = buildbucket_info_dict.get(build_config)
    ignorable_changes = build_ignorable_changes_dict.get(build_config, set())
    if bb_info.status != constants.BUILDBUCKET_BUILDER_STATUS_COMPLETED:
      return False
    if bb_info.result != constants.BUILDBUCKET_BUILDER_RESULT_SUCCESS:
      #If the build uploaded 'passed' BuilderStatus pickle or the build
      # only contains failures which can be ignored by this change, change is
      # in the value set for build_config in build_ignorable_changes_dict.
      if change not in ignorable_changes:
        return False

    return True

  def _ChangeCanBeSubmitted(self, change, relevant_slave_configs,
                            buildbucket_info_dict, build_ignorable_changes_dict,
                            change_passed_slaves_dict):
    """Whether the change can be submitted by current cq or cq history.

    A change can NOT be submitted if at least one of its relevant slaves
    satisfies the conditions:
    1) the relevant build_config failed in current build with not ignorable
      failures; AND
    2) no passed build for the relevant build_config in cq history.

    Args:
      change: A change (GerritPatch instance) to check.
      relevant_slave_configs: A list of relevant slave config names (string) of
        this change.
      buildbucket_info_dict: A dict mapping all slave build config names to
        their BuildbucketInfos (See SlaveBuilerStatus.GetAllSlaveBuildbucketInfo
        for details).
      build_ignorable_changes_dict: A dict mapping build config name (string) to
        a set of changes (GerritPatch instances) which can ignore the failures
        in the build.
      change_passed_slaves_dict: A dict mapping changes (cros_patch.GerritPatch)
        to their passed slaves (build_config name strings) in history.

    Returns:
      True if the change can be submitted given the statues of its relevant
      slaves; else, False.
    """
    for build_config in relevant_slave_configs:
      if (not self._ChangeVerifiedByCurrentBuild(change, build_config,
                                                 buildbucket_info_dict,
                                                 build_ignorable_changes_dict)
          and build_config not in change_passed_slaves_dict.get(change, set())):
        return False

    return True

  def _ProcessMightSubmitChanges(self):
    """Process changes in might_submit set.

    This method goes through all the changes in current might_submit set. For
    each change, get a set of its relevant slaves. If all the relevant slaves
    can either be verified by current CQ or passed in CQ history, move the
    change to will_submit set.
    """
    if not self.might_submit:
      return

    logging.info('Processing changes in might submit set.')
    will_submit_changes = set()
    for change in self.might_submit:
      relevant_slaves = self.change_relevant_slaves_dict.get(change, set())
      if self._ChangeCanBeSubmitted(
          change, relevant_slaves, self.buildbucket_info_dict,
          self.build_ignorable_changes_dict, self.change_passed_slaves_dict):
        will_submit_changes.add(change)

    if will_submit_changes:
      self.will_submit.update(will_submit_changes)
      self.might_submit.difference_update(will_submit_changes)
      logging.info('Moving %s to will_submit set, because their relevant builds'
                   ' completed successfully or all failures are ignorable or '
                   'passed in CQ history.',
                   cros_patch.GetChangesAsString(will_submit_changes))

  def _AllCompletedSlavesPassedUploadPrebuiltsStage(self):
    """Check if all completed slaves have passed the UploadPrebuiltsStage."""
    for build_config in self.completed_builds:
      # compilecheck builds don't run UploadPrebuiltsStage
      if (not site_config[build_config].compilecheck and
          not self.PassedAnyOfStages(self.slave_stages_dict[build_config],
                                     {self.STAGE_UPLOAD_PREBUILTS})):
        logging.info("Completed build %s didn't pass stage %s. "
                     "The master can't publish uprevs now.",
                     build_config, self.STAGE_UPLOAD_PREBUILTS)
        return False

    return True

  def _AllUncompletedSlavesPassedUploadPrebuiltsStage(self):
    """Check if all uncompleted slaves have passed the UploadPrebuiltsStage."""
    for build_config in set(self.builders_array) - self.completed_builds:
      # compilecheck builds don't run UploadPrebuiltsStage
      if (not site_config[build_config].compilecheck and
          not self.PassedAnyOfStages(self.slave_stages_dict[build_config],
                                     {self.STAGE_UPLOAD_PREBUILTS})):
        logging.info("Uncompleted build %s haven't passed stage %s. "
                     "The master can't publish uprevs now.",
                     build_config, self.STAGE_UPLOAD_PREBUILTS)
        return False

    return True

  def ShouldSelfDestruct(self):
    """Process builds and relevant changes, decide whether to self-destruct.

    Returns:
      A tuple of (boolean indicating if the master should self-destruct,
                  boolean indicating if the master should self-destruct with
                  with success)
    """
    self._ProcessCompletedBuilds()
    self._ProcessMightSubmitChanges()

    logging.info('will_submit set contains %d changes: [%s]\n'
                 'might_submit set contains %d changes: [%s]\n'
                 'will_not_submit set contains %d changes: [%s]\n',
                 len(self.will_submit),
                 cros_patch.GetChangesAsString(self.will_submit),
                 len(self.might_submit),
                 cros_patch.GetChangesAsString(self.might_submit),
                 len(self.will_not_submit),
                 cros_patch.GetChangesAsString(self.will_not_submit))

    # The master should wait for all the necessary slaves to pass the
    # UploadPrebuiltsStage so the master can publish prebuilts after
    # self-destruction with success. More context: crbug.com/703819
    all_completed_slaves_passed = (
        self._AllCompletedSlavesPassedUploadPrebuiltsStage())
    all_uncompleted_slaves_passed = (
        self._AllUncompletedSlavesPassedUploadPrebuiltsStage())
    should_self_destruct = (bool(not self.might_submit) and
                            (not all_completed_slaves_passed or
                             all_uncompleted_slaves_passed))
    should_self_destruct_with_success = (bool(not self.might_submit) and
                                         bool(not self.will_not_submit) and
                                         all_completed_slaves_passed and
                                         all_uncompleted_slaves_passed)

    return should_self_destruct, should_self_destruct_with_success
