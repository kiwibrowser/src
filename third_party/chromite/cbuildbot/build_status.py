# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for tracking and querying build status."""

from __future__ import print_function

import collections
import datetime

from chromite.cbuildbot import relevant_changes
from chromite.lib import buildbucket_lib
from chromite.lib import builder_status_lib
from chromite.lib import build_requests
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import metrics
from chromite.lib import timeout_util
from chromite.lib import tree_status


# TODO(nxia): Rename this module to slave_status, since this module is for
# a master build which has slave builds and there is builder_status_lib for
# managing the status of an indivudual build.
class SlaveStatus(object):
  """Keep track of statuses of all slaves from CIDB and Buildbucket(optional).

  For the master build scheduling slave builds through Buildbucket, it will
  interpret slave statuses by querying CIDB and Buildbucket; otherwise,
  it will only interpret slave statuses by querying CIDB.
  """

  BUILD_START_TIMEOUT_MIN = 5

  ACCEPTED_STATUSES = (constants.BUILDER_STATUS_PASSED,
                       constants.BUILDER_STATUS_SKIPPED,)

  def __init__(self, start_time, builders_array, master_build_id, db,
               config=None, metadata=None, buildbucket_client=None,
               version=None, pool=None, dry_run=True):
    """Initializes a SlaveStatus instance.

    Args:
      start_time: datetime.datetime object of when the build started.
      builders_array: List of the expected slave builds.
      master_build_id: The build_id of the master build.
      db: An instance of cidb.CIDBConnection to fetch data from CIDB.
      config: Instance of config_lib.BuildConfig. Config dict of this build.
      metadata: Instance of metadata_lib.CBuildbotMetadata. Metadata of this
                build.
      buildbucket_client: Instance of buildbucket_lib.buildbucket_client.
      version: Current manifest version string. See the return type of
               VersionInfo.VersionString().
      pool: An instance of ValidationPool.validation_pool used by sync stage
            to apply changes.
      dry_run: Boolean indicating whether it's a dry run. Default to True.
    """
    self.start_time = start_time
    self.all_builders = builders_array
    self.master_build_id = master_build_id
    self.db = db
    self.config = config
    self.metadata = metadata
    self.buildbucket_client = buildbucket_client
    self.version = version
    self.pool = pool
    self.dry_run = dry_run

    # A set of completed builds which will not be retried any more.
    self.completed_builds = set()
    # Dict mapping config names of slaves not in self.completed_builds to
    # their new CIDBStatusInfo. Everytime UpdateSlaveStatus is called,
    # new (current) status will be pulled from CIDB.
    self.new_cidb_status_dict = None
    # Dict mapping all slave config names to CIDBStatusInfo.
    self.all_cidb_status_dict = None
    self.missing_builds = None
    self.scheduled_builds = None
    self.builds_to_retry = None
    # Dict mapping config names of slaves not in self.completed_builds to
    # their new BuildbucketInfo. Everytime UpdateSlaveStatus is called,
    # new (current) status will be pulled from Buildbucket.
    # TODO(jkop): The code uses 'is not None' checks to determine if it's using
    # Buildbucket. Initialize this to a dict for simplicity when that's been
    # refactored.
    self.new_buildbucket_info_dict = None
    # Dict mapping all slave config names to BuildbucketInfo
    self.all_buildbucket_info_dict = {}
    self.status_buildset_dict = {}

    # Records history (per-tick) of self.completed_builds. Keep only the most
    # recent 2 entries of history. Used only for metrics purposes, not used for
    # any decision logic.
    self._completed_build_history = collections.deque([], 2)

    self.dependency_map = None

    if self.pool is not None:
      # Pre-compute dependency map for applied changes.
      self.dependency_map = self.pool.GetDependMapForChanges(
          self.pool.applied, self.pool.GetAppliedPatches())
    self.UpdateSlaveStatus()

  def _GetNewSlaveCIDBStatusInfo(self, all_cidb_status_dict, completed_builds):
    """Get new build status information for slaves not in completed_builds.

    Args:
      all_cidb_status_dict: A dict mapping all build config names to their
        information fetched from CIDB (in the format of CIDBStatusInfo).
      completed_builds: A set of slave build configs (strings) completed before.

    Returns:
      A dict mapping the build config names of slave builds which are not in
      the completed_builds to their CIDBStatusInfos.
    """
    return {build_config: status_info
            for build_config, status_info in all_cidb_status_dict.iteritems()
            if build_config not in completed_builds}

  def _GetNewSlaveBuildbucketInfo(self, all_buildbucket_info_dict,
                                  completed_builds):
    """Get new buildbucket info for slave builds not in completed_builds.

    Args:
      all_buildbucket_info_dict: A dict mapping all slave build config names
        to their BuildbucketInfos.
      completed_builds: A set of slave build configs (strings) completed before.

    Returns:
       A dict mapping config names of slave builds which are not in the
       completed_builds set to their BuildbucketInfos.
    """
    completed_builds = completed_builds or {}
    return {k: v for k, v in all_buildbucket_info_dict.iteritems()
            if k not in completed_builds}

  def _SetStatusBuildsDict(self):
    """Set status_buildset_dict by sorting the builds into their status set."""
    self.status_buildset_dict = {}
    for build, info in self.new_buildbucket_info_dict.iteritems():
      if info.status is not None:
        self.status_buildset_dict.setdefault(info.status, set())
        self.status_buildset_dict[info.status].add(build)

  def UpdateSlaveStatus(self):
    """Update slave statuses by querying CIDB and Buildbucket(if supported)."""
    logging.info('Updating slave status...')

    # Fetch experimental builders from tree status and update experimental
    # builders in metedata before querying and updating any slave status.
    if self.metadata is not None:
      try:
        experimental_builders = tree_status.GetExperimentalBuilders()
        self.metadata.UpdateWithDict({
            constants.METADATA_EXPERIMENTAL_BUILDERS: experimental_builders
        })
      except timeout_util.TimeoutError:
        logging.error('Timeout getting experimental builders from the tree'
                      'status. Not updating metadata.')

      # If a slave build was important in previous loop and got added to the
      # completed_builds because it completed, but in the current loop it's
      # marked as experimental, take it out from the completed_builds list.
      self.completed_builds = set([build for build in self.completed_builds
                                   if build not in experimental_builders])

    if (self.config is not None and
        self.metadata is not None and
        config_lib.UseBuildbucketScheduler(self.config)):
      scheduled_buildbucket_info_dict = buildbucket_lib.GetBuildInfoDict(
          self.metadata)
      # It's possible that CQ-master has a list of important slaves configured
      # but doesn't schedule any slaves as no CLs were picked up in SyncStage.
      # These are set to include only important builds.
      self.all_builders = scheduled_buildbucket_info_dict.keys()
      self.all_buildbucket_info_dict = (
          builder_status_lib.SlaveBuilderStatus.GetAllSlaveBuildbucketInfo(
              self.buildbucket_client, scheduled_buildbucket_info_dict,
              dry_run=self.dry_run))
      self.new_buildbucket_info_dict = self._GetNewSlaveBuildbucketInfo(
          self.all_buildbucket_info_dict, self.completed_builds)
      self._SetStatusBuildsDict()

    self.all_cidb_status_dict = (
        builder_status_lib.SlaveBuilderStatus.GetAllSlaveCIDBStatusInfo(
            self.db, self.master_build_id, self.all_buildbucket_info_dict))
    self.new_cidb_status_dict = self._GetNewSlaveCIDBStatusInfo(
        self.all_cidb_status_dict, self.completed_builds)

    self.missing_builds = self._GetMissingBuilds()
    self.scheduled_builds = self._GetScheduledBuilds()
    self.builds_to_retry = self._GetBuildsToRetry()
    self.completed_builds = self._GetCompletedBuilds()



  def GetBuildbucketBuilds(self, build_status):
    """Get the buildbucket builds which are in the build_status status.

    Args:
      build_status: The status of the builds to get. The status must
                    be a member of constants.BUILDBUCKET_BUILDER_STATUSES.

    Returns:
      A set of builds in build_status status.
    """
    if build_status not in constants.BUILDBUCKET_BUILDER_STATUSES:
      raise ValueError(
          '%s is not a member of %s '
          % (build_status, constants.BUILDBUCKET_BUILDER_STATUSES))

    return self.status_buildset_dict.get(build_status, set())

  def _GetExpectedBuilders(self):
    """Returns the list of expected slave build configs.

    This list includes all important slave build configs that are not currently
    marked as experimental through the tree status.

    Returns:
      A list of build slave config names.
    """
    experimental_builders = []
    if self.metadata:
      experimental_builders = self.metadata.GetValueWithDefault(
          constants.METADATA_EXPERIMENTAL_BUILDERS, [])
    return [
        builder for builder in self.all_builders
        if builder not in experimental_builders
    ]

  def _GetMissingBuilds(self):
    """Returns the missing builds.

    For builds scheduled by Buildbucket, missing refers to builds without
    'status' from Buildbucket.
    For builds not scheduled by Buildbucket, missing refers builds without
    reporting status to CIDB.

    Returns:
      A set of the config names of missing builds.
    """
    if self.new_buildbucket_info_dict is not None:
      return set(build for build, info in
                 self.new_buildbucket_info_dict.iteritems()
                 if info.status is None)
    else:
      return (set(self._GetExpectedBuilders()) -
              set(self.new_cidb_status_dict.keys()) -
              self.completed_builds)

  def _GetScheduledBuilds(self):
    """Returns the scheduled builds.

    Returns:
      For builds scheduled by Buildbucket, a set of config names of builds
      with 'SCHEDULED' status in Buildbucket;
      For other builds, None.
    """
    if self.new_buildbucket_info_dict is not None:
      return self.GetBuildbucketBuilds(
          constants.BUILDBUCKET_BUILDER_STATUS_SCHEDULED)
    else:
      return None

  def _GetRetriableBuilds(self, completed_builds):
    """Get retriable builds from completed builds.

    Args:
      completed_builds: a set of builds with 'COMPLETED' status in Buildbucket.

    Returns:
      A set of config names of retriable builds.
    """
    builds_to_retry = set()

    for build in completed_builds:
      build_result = self.new_buildbucket_info_dict[build].result
      if build_result == constants.BUILDBUCKET_BUILDER_RESULT_SUCCESS:
        logging.info('Not retriable build %s completed with result %s.',
                     build, build_result)
        continue

      build_retry = self.new_buildbucket_info_dict[build].retry
      if build_retry >= constants.BUILDBUCKET_BUILD_RETRY_LIMIT:
        logging.info('Not retriable build %s reached the build retry limit %d.',
                     build, constants.BUILDBUCKET_BUILD_RETRY_LIMIT)
        continue

      # If build is in self.status, it means a build tuple has been
      # inserted into CIDB buildTable.
      if build in self.new_cidb_status_dict:
        if not config_lib.RetryAlreadyStartedSlaves(self.config):
          logging.info('Not retriable build %s started already.', build)
          continue

        assert self.db is not None

        build_stages = self.db.GetBuildStages(
            self.new_cidb_status_dict[build].build_id)
        accepted_stages = {stage['name'] for stage in build_stages
                           if stage['status'] in self.ACCEPTED_STATUSES}

        # A failed build is not retriable if it passed the critical stage.
        if config_lib.GetCriticalStageForRetry(self.config).intersection(
            accepted_stages):
          continue

      builds_to_retry.add(build)

    return builds_to_retry

  def _GetBuildsToRetry(self):
    """Get the config names of the builds to retry.

    Returns:
      A set config names of builds to be retried.
    """
    if self.new_buildbucket_info_dict is not None:
      return self._GetRetriableBuilds(
          self.GetBuildbucketBuilds(
              constants.BUILDBUCKET_BUILDER_STATUS_COMPLETED))
    else:
      return None

  def _GetCompletedBuilds(self):
    """Returns the builds that have completed and will not be retried.

    Returns:
      A set of config names of completed and not retriable builds.
    """
    # current completed builds (not in self.completed_builds) from CIDB
    current_completed = set(
        b for b, s in self.new_cidb_status_dict.iteritems()
        if s.status in constants.BUILDER_COMPLETED_STATUSES and
        b in self._GetExpectedBuilders())

    if self.new_buildbucket_info_dict is not None:
      assert self.builds_to_retry is not None

      # current completed builds (not in self.completed_builds) from Buildbucket
      current_completed_buildbucket = self.GetBuildbucketBuilds(
          constants.BUILDBUCKET_BUILDER_STATUS_COMPLETED)
      current_completed = ((current_completed | current_completed_buildbucket) -
                           self.builds_to_retry)

    for build in current_completed:
      cidb_status = (self.new_cidb_status_dict[build].status if
                     build in self.new_cidb_status_dict else None)
      status_output = ('Build config %s completed: CIDB status: %s.' %
                       (build, cidb_status))
      if self.new_buildbucket_info_dict is not None:
        status_output += (' Buildbucket status %s result %s.' %
                          (self.new_buildbucket_info_dict[build].status,
                           self.new_buildbucket_info_dict[build].result))
      logging.info(status_output)

    completed_builds = self.completed_builds | current_completed

    return completed_builds

  def _Completed(self):
    """Returns a bool if all builds have completed successfully.

    Returns:
      A bool of True if all builds successfully completed, False otherwise.
    """
    return len(self.completed_builds) == len(self._GetExpectedBuilders())


  def _GetUncompletedBuilds(self, completed_builds):
    """Get uncompleted important builds.

    Args:
      completed_builds: a set of config names (strings) of completed builds.

    Returns:
      A set of config names (strings) of uncompleted important builds.
    """
    return set(self._GetExpectedBuilders()) - completed_builds

  def _GetUncompletedExperimentalBuildbucketIDs(self):
    """Get buildbucket_ids for uncompleted experimental builds.

    Returns:
      A set of Buildbucket IDs (strings) of uncompleted experimental builds.
    """
    flagged_experimental_builders = self.metadata.GetValueWithDefault(
        constants.METADATA_EXPERIMENTAL_BUILDERS, [])
    experimental_slaves = self.metadata.GetValueWithDefault(
        constants.METADATA_SCHEDULED_EXPERIMENTAL_SLAVES, [])
    important_slaves = self.metadata.GetValueWithDefault(
        constants.METADATA_SCHEDULED_IMPORTANT_SLAVES, [])
    experimental_slaves += [
        (name, bb_id, time) for (name, bb_id, time) in important_slaves
        if name in flagged_experimental_builders
    ]

    all_experimental_bb_info_dict = (
        builder_status_lib.SlaveBuilderStatus.GetAllSlaveBuildbucketInfo(
            self.buildbucket_client,
            buildbucket_lib.GetScheduledBuildDict(experimental_slaves),
            self.dry_run
        )
    )
    all_experimental_cidb_status_dict = (
        builder_status_lib.SlaveBuilderStatus.GetAllSlaveCIDBStatusInfo(
            self.db, self.master_build_id, all_experimental_bb_info_dict)
    )

    completed_experimental_builds = set(
        name for name, info in all_experimental_bb_info_dict.iteritems() if
        info.status == constants.BUILDBUCKET_BUILDER_STATUS_COMPLETED
    )
    completed_experimental_builds |= set(
        name for name, info in all_experimental_cidb_status_dict.iteritems()
        if info.status in constants.BUILDER_COMPLETED_STATUSES
    )

    return set([bb_id for (name, bb_id, time) in experimental_slaves
                if name not in completed_experimental_builds])

  def _ShouldFailForBuilderStartTimeout(self, current_time):
    """Decides if we should fail if a build hasn't started within 5 mins.

    If a build hasn't started within BUILD_START_TIMEOUT_MIN and the rest of
    the builds have finished, let the caller know that we should fail.

    Args:
      current_time: A datetime.datetime object letting us know the current time.

    Returns:
      A bool saying True that we should fail, False otherwise.
    """
    # Check that we're at least past the start timeout.
    builder_start_deadline = datetime.timedelta(
        minutes=self.BUILD_START_TIMEOUT_MIN)
    past_deadline = current_time - self.start_time > builder_start_deadline

    # Check that we have missing builders and logging who they are.
    for builder in self.missing_builds:
      logging.error('No status found for build config %s.', builder)

    if self.new_buildbucket_info_dict is not None:
      # All scheduled builds added in new_buildbucket_info_dict are
      # either in completed status or still in scheduled status.
      other_builders_completed = (
          len(self.scheduled_builds) + len(self.completed_builds) ==
          len(self._GetExpectedBuilders()))

      for builder in self.scheduled_builds:
        logging.error('Builder not started %s.', builder)

      return (past_deadline and other_builders_completed and
              self.scheduled_builds)
    else:
      # Check that aside from the missing builders the rest have completed.
      other_builders_completed = (
          len(self.missing_builds) + len(self.completed_builds) ==
          len(self._GetExpectedBuilders()))

      return (past_deadline and other_builders_completed and
              self.missing_builds)

  def _RetryBuilds(self, builds):
    """Retry builds with Buildbucket.

    Args:
      builds: config names of the builds to retry with Buildbucket.

    Returns:
      A set of retried builds.
    """
    assert builds is not None

    new_scheduled_important_slaves = []
    new_scheduled_build_reqs = []
    for build in builds:
      try:
        buildbucket_id = self.new_buildbucket_info_dict[build].buildbucket_id
        build_retry = self.new_buildbucket_info_dict[build].retry

        logging.info('Going to retry build %s buildbucket_id %s '
                     'with retry # %d',
                     build, buildbucket_id, build_retry + 1)

        if not self.dry_run:
          fields = {'build_type': self.config.build_type,
                    'build_name': self.config.name}
          metrics.Counter(constants.MON_BB_RETRY_BUILD_COUNT).increment(
              fields=fields)

        content = self.buildbucket_client.RetryBuildRequest(
            buildbucket_id, dryrun=self.dry_run)

        new_buildbucket_id = buildbucket_lib.GetBuildId(content)
        new_created_ts = buildbucket_lib.GetBuildCreated_ts(content)

        new_scheduled_important_slaves.append(
            (build, new_buildbucket_id, new_created_ts))
        new_scheduled_build_reqs.append(build_requests.BuildRequest(
            None, self.master_build_id, build, None, new_buildbucket_id,
            build_requests.REASON_IMPORTANT_CQ_SLAVE, None))

        logging.info('Retried build %s buildbucket_id %s created_ts %s',
                     build, new_buildbucket_id, new_created_ts)
      except buildbucket_lib.BuildbucketResponseException as e:
        logging.error('Failed to retry build %s buildbucket_id %s: %s',
                      build, buildbucket_id, e)

    if config_lib.IsMasterCQ(self.config) and new_scheduled_build_reqs:
      self.db.InsertBuildRequests(new_scheduled_build_reqs)

    if new_scheduled_important_slaves:
      self.metadata.ExtendKeyListWithList(
          constants.METADATA_SCHEDULED_IMPORTANT_SLAVES,
          new_scheduled_important_slaves)

    return set([build for build, _, _ in new_scheduled_important_slaves])

  @staticmethod
  def _LastSlavesToComplete(completed_builds_history):
    """Given a |completed_builds_history|, find the last to complete.

    Returns:
      A set of build_configs that were the last to complete.
    """
    if not completed_builds_history:
      return set()
    elif len(completed_builds_history) == 1:
      return set(completed_builds_history[0])
    else:
      return (set(completed_builds_history[-1]) -
              set(completed_builds_history[-2]))

  def ShouldWait(self):
    """Decides if we should continue to wait for the builds to finish.

    This will be the retry function for timeout_util.WaitForSuccess, basically
    this function will return False if all builds finished or we see a problem
    with the builds. Otherwise it returns True to continue polling
    for the builds statuses. If the slave builds are scheduled by Buildbucket
    and there're builds to retry, call RetryBuilds on those builds.

    Returns:
      A bool of True if we should continue to wait and False if we should not.
    """
    retval, slaves_remain, long_pole = self._ShouldWait()

    # If we're no longer waiting, record last-slave-to-complete metrics.
    if not retval and long_pole:
      m = metrics.CumulativeMetric(constants.MON_LAST_SLAVE)
      slaves = self._LastSlavesToComplete(self._completed_build_history)
      if slaves and self.config:
        increment = 1.0 / len(slaves)
        for s in slaves:
          m.increment_by(increment, fields={'master_config': self.config.name,
                                            'last_slave_config': s,
                                            'slaves_remain': slaves_remain})

    return retval

  def _ShouldWait(self):
    """Private helper with all the main logic of ShouldWait.

    Returns:
      A tuple of (bool indicating if we should wait,
                  bool indicating if slaves remain,
                  bool indicating if the final slave(s) to complete should
                  be considered the long-pole reason for terminating)
    """
    self._completed_build_history.append(list(self.completed_builds))

    uncompleted_experimental_build_buildbucket_ids = (
        self._GetUncompletedExperimentalBuildbucketIDs())

    # Check if all builders completed.
    if self._Completed():
      builder_status_lib.CancelBuilds(
          list(uncompleted_experimental_build_buildbucket_ids),
          self.buildbucket_client,
          self.dry_run,
          self.config)
      return False, False, True

    current_time = datetime.datetime.now()

    uncompleted_important_builds = self._GetUncompletedBuilds(
        self.completed_builds)
    uncompleted_important_build_buildbucket_ids = set(
        v.buildbucket_id
        for k, v in self.all_buildbucket_info_dict.iteritems() if k in
        uncompleted_important_builds)
    uncompleted_build_buildbucket_ids = list(
        uncompleted_important_build_buildbucket_ids |
        uncompleted_experimental_build_buildbucket_ids)

    if self._ShouldFailForBuilderStartTimeout(current_time):
      logging.error('Ending build since at least one builder has not started '
                    'within 5 mins.')
      builder_status_lib.CancelBuilds(uncompleted_build_buildbucket_ids,
                                      self.buildbucket_client,
                                      self.dry_run,
                                      self.config)
      return False, False, False

    if self.pool is not None:
      triage_relevant_changes = relevant_changes.TriageRelevantChanges(
          self.master_build_id, self.db, self._GetExpectedBuilders(),
          self.config, self.metadata, self.version, self.pool.build_root,
          self.pool.applied, self.all_buildbucket_info_dict,
          self.all_cidb_status_dict, self.completed_builds, self.dependency_map,
          self.buildbucket_client, dry_run=self.dry_run)

      should_self_destruct, should_self_destruct_with_success = (
          triage_relevant_changes.ShouldSelfDestruct())
      if should_self_destruct:
        logging.warning('This build will self-destruct given the results of '
                        'relevant change triages.')

        if should_self_destruct_with_success:
          logging.info('This build will self-destruct with success.')

        self.metadata.UpdateWithDict({
            constants.SELF_DESTRUCTED_BUILD: True,
            constants.SELF_DESTRUCTED_WITH_SUCCESS_BUILD:
            should_self_destruct_with_success})

        fields = {
            'build_config': self.config.name,
            'self_destructed_with_success': should_self_destruct_with_success}
        metrics.Counter(constants.MON_CQ_SELF_DESTRUCTION_COUNT).increment(
            fields=fields)

        # For every uncompleted build, the master build will insert an
        # ignored_reason message into the buildMessageTable.
        for build in uncompleted_important_builds:
          if build in self.all_cidb_status_dict:
            self.db.InsertBuildMessage(
                self.master_build_id,
                message_type=constants.MESSAGE_TYPE_IGNORED_REASON,
                message_subtype=constants.MESSAGE_SUBTYPE_SELF_DESTRUCTION,
                message_value=str(self.all_cidb_status_dict[build].build_id))
        builder_status_lib.CancelBuilds(uncompleted_build_buildbucket_ids,
                                        self.buildbucket_client,
                                        self.dry_run,
                                        self.config)
        return False, False, True

    # We got here which means no problems, we should still wait.
    logging.info('Still waiting for the following builds to complete: %r',
                 sorted(set(self._GetExpectedBuilders()) -
                        self.completed_builds))

    if self.builds_to_retry:
      retried_builds = self._RetryBuilds(self.builds_to_retry)
      self.builds_to_retry -= retried_builds

    return True, True, False
