# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the completion stages."""

from __future__ import print_function

from infra_libs import ts_mon

from chromite.cbuildbot import commands
from chromite.cbuildbot import prebuilts
from chromite.cbuildbot.stages import generic_stages
from chromite.cbuildbot.stages import sync_stages
from chromite.lib import buildbucket_lib
from chromite.lib import builder_status_lib
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import failures_lib
from chromite.lib import metrics
from chromite.lib import tree_status


def GetBuilderSuccessMap(builder_run, overall_success):
  """Get the pass/fail status of all builders.

  A builder is marked as passed if all of its steps ran all of the way to
  completion. We determine this by looking at whether all of the steps for
  all of the constituent boards ran to completion.

  In cases where a builder does not have any boards, or has child boards, we
  fall back and instead just look at whether the entire build was successful.

  Args:
    builder_run: The builder run we wish to get the status of.
    overall_success: The overall status of the build.

  Returns:
    A dict, mapping the builder names to whether they succeeded.
  """
  success_map = {}
  for run in [builder_run] + builder_run.GetChildren():
    if run.config.boards and not run.config.child_configs:
      success_map[run.config.name] = True
      for board in run.config.boards:
        board_runattrs = run.GetBoardRunAttrs(board)
        if not board_runattrs.HasParallel('success'):
          success_map[run.config.name] = False
    else:
      # If a builder does not have boards, or if it has child configs, we
      # will just use the overall status instead.
      success_map[run.config.name] = overall_success
  return success_map


class ManifestVersionedSyncCompletionStage(
    generic_stages.ForgivingBuilderStage):
  """Stage that records board specific results for a unique manifest file."""

  option_name = 'sync'

  def __init__(self, builder_run, sync_stage, success, **kwargs):
    super(ManifestVersionedSyncCompletionStage, self).__init__(
        builder_run, **kwargs)
    self.sync_stage = sync_stage
    self.success = success
    # Message that can be set that well be sent along with the status in
    # UpdateStatus.
    self.message = None

  def PerformStage(self):
    if not self.success:
      self.message = self.GetBuildFailureMessage()

    if not config_lib.IsPFQType(self._run.config.build_type):
      # Update the pass/fail status in the manifest-versions
      # repo. Suite scheduler checks the build status to schedule
      # suites.
      self._run.attrs.manifest_manager.UpdateStatus(
          success_map=GetBuilderSuccessMap(self._run, self.success),
          message=self.message)


class ImportantBuilderFailedException(failures_lib.StepFailure):
  """Exception thrown when an important build fails to build."""


class MasterSlaveSyncCompletionStage(ManifestVersionedSyncCompletionStage):
  """Stage that records whether we passed or failed to build/test manifest."""

  def __init__(self, *args, **kwargs):
    super(MasterSlaveSyncCompletionStage, self).__init__(*args, **kwargs)
    # TODO(nxia): rename to _build_statuses as it contains local status and
    # slave statuses for master builds
    self._slave_statuses = {}
    self._experimental_build_statuses = {}
    self._fatal = False
    self.buildbucket_client = self.GetBuildbucketClient()

  def _WaitForSlavesToComplete(self, manager, build_id, db, builders_array,
                               timeout):
    """Wait for slave builds to complete.

    Args:
      manager: An instance of BuildSpecsManager.
      build_id: The build id of the master build.
      db: An instance of cidb.CIDBConnection.
      builders_array: A list of builder names (strings) of slave builds.
      timeout: Number of seconds to wait for the results.
    """
    return manager.WaitForSlavesToComplete(
        build_id,
        db,
        builders_array,
        timeout=timeout)

  def _GetBuilderStatusesFetcher(self):
    """Construct and return the BuilderStatusesFetcher instance.

    If this build is a master, wait for slaves to complete or timeout before
    constructing and returning the BuilderStatusesFetcher instance.

    Returns:
      A instance of builder_status_lib.BuilderStatusesFetcher.
    """
    # Wait for slaves if we're a master, in production or mock-production.
    # Otherwise just look at our own status.
    build_id, db = self._run.GetCIDBHandle()
    builders_array = None
    if not self._run.config.master:
      # The slave build returns its own status.
      logging.warning('The build is not a master.')
    elif self._run.options.mock_slave_status or not self._run.options.debug:
      # The master build.
      builders = self._GetSlaveConfigs()
      builders_array = [b.name for b in builders]
      timeout = None

      if db:
        timeout = db.GetTimeToDeadline(build_id)
        logging.info('Got timeout for build_id %s', build_id)
      if timeout is None:
        # Catch-all: This could happen if cidb is not setup, or the deadline
        # query fails.
        timeout = self._run.config.build_timeout

      if self._run.options.debug:
        # For debug runs, wait for three minutes to ensure most code
        # paths are executed.
        logging.info('Waiting for 3 minutes only for debug run. '
                     'Would have waited for %s seconds.', timeout)
        timeout = 3 * 60

      manager = self._run.attrs.manifest_manager
      if sync_stages.MasterSlaveLKGMSyncStage.external_manager:
        manager = sync_stages.MasterSlaveLKGMSyncStage.external_manager

      self._WaitForSlavesToComplete(
          manager, build_id, db, builders_array, timeout)

    # Set exclude_experimental to False to fetch the BuilderStatus for
    # builds which are important in config but marked as experimental in
    # the tree status.
    builder_statuses_fetcher = builder_status_lib.BuilderStatusesFetcher(
        build_id, db, self.success, self.message, self._run.config,
        self._run.attrs.metadata, self.buildbucket_client,
        builders_array=builders_array, exclude_experimental=False,
        dry_run=self._run.options.debug)

    return builder_statuses_fetcher

  def _HandleStageException(self, exc_info):
    """Decide whether an exception should be treated as fatal."""
    # Besides the master, the completion stages also run on slaves, to report
    # their status back to the master. If the build failed, they throw an
    # exception here. For slave builders, marking this stage 'red' would be
    # redundant, since the build itself would already be red. In this case,
    # report a warning instead.
    # pylint: disable=protected-access
    exc_type = exc_info[0]
    if (issubclass(exc_type, ImportantBuilderFailedException) and
        not self._run.config.master):
      return self._HandleExceptionAsWarning(exc_info)
    else:
      # In all other cases, exceptions should be treated as fatal. To
      # implement this, we bypass ForgivingStage and call
      # generic_stages.BuilderStage._HandleStageException explicitly.
      return generic_stages.BuilderStage._HandleStageException(self, exc_info)

  def HandleSuccess(self):
    """Handle a successful build.

    This function is called whenever the cbuildbot run is successful.
    For the master, this will only be called when all slave builders
    are also successful. This function may be overridden by subclasses.
    """
    # We only promote for the pfq, not chrome pfq.
    # TODO(build): Run this logic in debug mode too.
    if (not self._run.options.debug and
        config_lib.IsPFQType(self._run.config.build_type) and
        self._run.config.master and
        self._run.manifest_branch == 'master' and
        self._run.config.build_type != constants.CHROME_PFQ_TYPE):
      self._run.attrs.manifest_manager.PromoteCandidate()
      if sync_stages.MasterSlaveLKGMSyncStage.external_manager:
        sync_stages.MasterSlaveLKGMSyncStage.external_manager.PromoteCandidate()

  def HandleFailure(self, failing, inflight, no_stat, self_destructed):
    """Handle a build failure.

    This function is called whenever the cbuildbot run fails.
    For the master, this will be called when any slave fails or times
    out. This function may be overridden by subclasses.

    Args:
      failing: The names of the failing builders.
      inflight: The names of the builders that are still running.
      no_stat: Set of builder names of slave builders that had status None.
      self_destructed: Boolean indicating whether the master build destructed
                       itself and stopped waiting completion of its slaves.
    """
    if failing or inflight or no_stat:
      logging.PrintBuildbotStepWarnings()

    if failing:
      logging.warning('\n'.join([
          'The following builders failed with this manifest:',
          ', '.join(sorted(failing)),
          'Please check the logs of the failing builders for details.']))

    if not self_destructed and inflight:
      logging.warning('\n'.join([
          'The following builders took too long to finish:',
          ', '.join(sorted(inflight)),
          'Please check the logs of these builders for details.']))

    if no_stat:
      logging.warning('\n'.join([
          'The following builders did not start or failed prematurely:',
          ', '.join(sorted(no_stat)),
          'Please check the logs of these builders for details.']))

  @staticmethod
  def _EmitImportanceMetric(master_config, important_configs,
                            experimental_configs):
    """Emit monarch metrics about which slave configs were important."""
    importance = {build_config: True for build_config in important_configs}
    for build_config in experimental_configs:
      importance[build_config] = False

    m = metrics.BooleanMetric('chromeos/cbuildbot/master/has_important_slave',
                              description='Slaves that were considered '
                                          'important by master.',
                              field_spec=[ts_mon.StringField('master_config'),
                                          ts_mon.StringField('slave_config')])
    for slave_config, is_important in importance.iteritems():
      m.set(is_important, fields={'master_config': master_config,
                                  'slave_config': slave_config})

  def PerformStage(self):
    super(MasterSlaveSyncCompletionStage, self).PerformStage()

    builder_statusess_fetcher = self._GetBuilderStatusesFetcher()
    self._slave_statuses, self._experimental_build_statuses = (
        builder_statusess_fetcher.GetBuilderStatuses())

    if self._run.config.master:
      self._EmitImportanceMetric(self._run.config.name,
                                 self._slave_statuses,
                                 self._experimental_build_statuses)

    no_stat = builder_status_lib.BuilderStatusesFetcher.GetNostatBuilds(
        self._slave_statuses)
    failing = builder_status_lib.BuilderStatusesFetcher.GetFailingBuilds(
        self._slave_statuses)
    inflight = builder_status_lib.BuilderStatusesFetcher.GetInflightBuilds(
        self._slave_statuses)

    self_destructed = self._run.attrs.metadata.GetValueWithDefault(
        constants.SELF_DESTRUCTED_BUILD, False)

    self._fatal = self._IsFailureFatal(
        failing, inflight, no_stat, self_destructed=self_destructed)

    # Always annotate unsuccessful builders.
    self._AnnotateFailingBuilders(
        failing, inflight, no_stat, self._slave_statuses,
        self._experimental_build_statuses, self_destructed)

    if self._fatal:
      self.HandleFailure(failing, inflight, no_stat, self_destructed)
      raise ImportantBuilderFailedException()
    else:
      self.HandleSuccess()

  def _IsFailureFatal(self, failing, inflight, no_stat, self_destructed=False):
    """Returns a boolean indicating whether the build should fail.

    Args:
      failing: Set of build config names of builders that failed.
      inflight: Set of build config names of builders that are inflight
      no_stat: Set of build config names of builders that had status None.
      self_destructed: Boolean indicating whether it's a master build which
        destructed itself and stopped waiting its slaves to complete.

    Returns:
      True if any of the failing, inflight or no_stat builders are not sanity
      checker builders and not ignored by self-destruction; else, False.
    """
    not_passed_builders = failing | inflight | no_stat

    if self_destructed:
      # This build must be a master build if self_destructed is True.
      self_destructed_with_success = (
          self._run.attrs.metadata.GetValueWithDefault(
              constants.SELF_DESTRUCTED_WITH_SUCCESS_BUILD, False))
      if self_destructed_with_success:
        # If the master build itself didn't pass, report fatal.
        return self._run.config.name in not_passed_builders

      build_id, db = self._run.GetCIDBHandle()
      if db:
        aborted_slaves = (
            builder_status_lib.GetSlavesAbortedBySelfDestructedMaster(
                build_id, db))
        # Ignore the slaves aborted by self-destruction.
        not_passed_builders -= aborted_slaves

    # Fatal if any not_passed_builders remain.
    return bool(not_passed_builders)

  def _PrintBuildMessage(self, text, url=None):
    """Print the build message.

    Args:
      text: Text (string) to print.
      url: URL (string) to link to the text, default to None.
    """
    if url is not None:
      logging.PrintBuildbotLink(text, url)
    else:
      logging.PrintBuildbotStepText(text)

  def _AnnotateBuildStatusFromBuildbucket(self, no_stat):
    """Annotate the build statuses fetched from the Buildbucket.

    Some builds may fail to upload statuses to GS. If the builds were
    scheduled by Buildbucket, get the build statuses and annotate the results.

    Args:
      no_stat: Config names of the slave builds with None status.
    """
    buildbucket_info_dict = buildbucket_lib.GetBuildInfoDict(
        self._run.attrs.metadata)

    for config_name in no_stat:
      if config_name in buildbucket_info_dict:
        buildbucket_id = buildbucket_info_dict[config_name].buildbucket_id
        assert buildbucket_id is not None, 'buildbucket_id is None'
        try:
          content = self.buildbucket_client.GetBuildRequest(
              buildbucket_id, self._run.options.debug)

          status = buildbucket_lib.GetBuildStatus(content)
          result = buildbucket_lib.GetBuildResult(content)

          text = '%s: [status] %s [result] %s' % (config_name, status, result)

          if result == constants.BUILDBUCKET_BUILDER_RESULT_FAILURE:
            failure_reason = buildbucket_lib.GetBuildFailureReason(content)
            if failure_reason:
              text += ' [failure_reason] %s' % failure_reason
          elif result == constants.BUILDBUCKET_BUILDER_RESULT_CANCELED:
            cancel_reason = buildbucket_lib.GetBuildCancelationReason(content)
            if cancel_reason:
              text += ' [cancelation_reason] %s' % cancel_reason

          dashboard_url = buildbucket_lib.GetBuildURL(content)
          if dashboard_url:
            logging.PrintBuildbotLink(text, dashboard_url)
          else:
            logging.PrintBuildbotStepText(text)
        except buildbucket_lib.BuildbucketResponseException as e:
          logging.error('Cannot get status for %s: %s', config_name, e)
          logging.PrintBuildbotStepText(
              'No status found for build %s buildbucket_id %s'
              % (config_name, buildbucket_id))
      else:
        logging.PrintBuildbotStepText('%s wasn\'t scheduled by master.'
                                      % config_name)

  def _AnnotateNoStatBuilders(self, no_stat):
    """Annotate the no stat builds.

    Args:
      no_stat: Set of build config names of slaves that had status None.
    """
    if config_lib.UseBuildbucketScheduler(self._run.config):
      self._AnnotateBuildStatusFromBuildbucket(no_stat)
    else:
      for build in no_stat:
        self._PrintBuildMessage('%s: did not start' % build)

  def _AnnotateFailingBuilders(self, failing, inflight, no_stat, statuses,
                               experimental_statuses,
                               self_destructed):
    """Annotate the failing, inflight and no_stat builds with text and links.

    Add text and buildbot links to build dashboards for failing builds and
    inflight builds. For master builds using Buildbucket schdeduler, add text
    and buildbot links for the no_stat builds; for other master builds, add
    step text for the no_stat builds.

    Args:
      failing: Set of builder names of slave builders that failed.
      inflight: Set of builder names of slave builders that are inflight.
      no_stat: Set of builder names of slave builders that had status None.
      statuses: A builder-name->status dictionary, which will provide
                the dashboard_url values for any links.
      experimental_statuses: A builder-name->status dictionary for all slaves
                             that were set as experimental through the tree
                             status.
      self_destructed: Boolean indicating whether the master build destructed
                       itself and stopped waiting completion of its slaves.
    """
    for build in failing:
      if statuses[build].message:
        self._PrintBuildMessage(
            '%s: %s' % (build, statuses[build].message.reason),
            statuses[build].dashboard_url)
      else:
        self._PrintBuildMessage('%s: failed due to unknown reasons' % build,
                                statuses[build].dashboard_url)

    if not self_destructed:
      for build in inflight:
        self._PrintBuildMessage('%s: timed out' % build,
                                statuses[build].dashboard_url)

      self._AnnotateNoStatBuilders(no_stat)
    else:
      logging.PrintBuildbotStepText('The master destructed itself and stopped '
                                    'waiting for the following slaves:')
      for build in inflight:
        self._PrintBuildMessage('%s: still running' % build,
                                statuses[build].dashboard_url)

      self._AnnotateNoStatBuilders(no_stat)

    for build, status in experimental_statuses.items():
      if not status.Passed():
        self._PrintBuildMessage('%s: set as experimental through tree status' %
                                build, status.dashboard_url)

  def GetSlaveStatuses(self):
    """Returns cached slave status results.

    Cached results are populated during PerformStage, so this function
    should only be called after PerformStage has returned.

    Returns:
      A dictionary from build names to builder_status_lib.BuilderStatus
      builder status objects.
    """
    return self._slave_statuses

  def GetExperimentalBuildStatuses(self):
    """Returns cached BuilderStatus of the experimental slave builds.

    Cached results are populated during PerformStage, so this function
    should only be called after PerformStage has returned.

    Returns:
      A dictionary from names of experimental slave builds to their
        builder_status_lib.BuilderStatus objects.
    """
    return self._experimental_build_statuses

  def GetFatal(self):
    """Returns whether the build (and its important slaves) failed with fatal.

    Cached results are populated during PerformStage, so this function
    should only be called after PerformStage has returned.

    Returns:
      A boolean indicating whether the build  (and its important slaves) failed
      with fatal.
    """
    return self._fatal


class CanaryCompletionStage(MasterSlaveSyncCompletionStage):
  """Collect build slave statuses and handle the failures."""

  def HandleFailure(self, failing, inflight, no_stat, self_destructed):
    """Handle a build failure or timeout in the Canary builders.

    Args:
      failing: Names of the builders that failed.
      inflight: Names of the builders that timed out.
      no_stat: Set of builder names of slave builders that had status None.
      self_destructed: Boolean indicating whether the master build destructed
                       itself and stopped waiting completion of its slaves.
    """
    # Print out the status about what builds failed or not.
    MasterSlaveSyncCompletionStage.HandleFailure(
        self, failing, inflight, no_stat, self_destructed)

    if self._run.config.master:
      self.CanaryMasterHandleFailure(failing, inflight, no_stat)

  def SendCanaryFailureAlert(self, failing, inflight, no_stat):
    """Send an alert email to summarize canary failures.

    Args:
      failing: The names of the failing builders.
      inflight: The names of the builders that are still running.
      no_stat: The names of the builders that had status None.
    """
    builder_name = 'Canary Master'
    title = '%s has detected build failures:' % builder_name
    msgs = [str(x) for x in
            builder_status_lib.GetFailedMessages(
                self._slave_statuses, failing)]
    slaves = builder_status_lib.GetBuildersWithNoneMessages(
        self._slave_statuses, failing)
    msgs += ['%s failed with unknown reason.' % x for x in slaves]
    msgs += ['%s timed out' % x for x in inflight]
    msgs += ['%s did not start' % x for x in no_stat]
    msgs.insert(0, title)
    msgs.append('You can also view the summary of the slave failures from '
                'the %s stage of %s. Click on the failure message to go '
                'to an individual slave\'s build status page: %s' % (
                    self.name, builder_name, self.ConstructDashboardURL()))
    msg = '\n\n'.join(msgs)
    logging.warning(msg)
    extra_fields = {'X-cbuildbot-alert': 'canary-fail-alert'}
    tree_status.SendHealthAlert(self._run, 'Canary builder failures', msg,
                                extra_fields=extra_fields)

  def _ComposeTreeStatusMessage(self, failing, inflight, no_stat):
    """Composes a tres status message.

    Args:
      failing: Names of the builders that failed.
      inflight: Names of the builders that timed out.
      no_stat: Set of builder names of slave builders that had status None.

    Returns:
      A string.
    """
    slave_status_list = [
        ('did not start', list(no_stat)),
        ('timed out', list(inflight)),
        ('failed', list(failing)),]
    # Print maximum 2 slaves for each category to not clutter the
    # message.
    max_num = 2
    messages = []
    for status, slaves in slave_status_list:
      if not slaves:
        continue
      slaves_str = ','.join(slaves[:max_num])
      if len(slaves) <= max_num:
        messages.append('%s %s' % (slaves_str, status))
      else:
        messages.append('%s and %d others %s' % (slaves_str,
                                                 len(slaves) - max_num,
                                                 status))
    return '; '.join(messages)

  def CanaryMasterHandleFailure(self, failing, inflight, no_stat):
    """Handles the failure by sending out an alert email.

    Args:
      failing: Names of the builders that failed.
      inflight: Names of the builders that timed out.
      no_stat: Set of builder names of slave builders that had status None.
    """
    if self._run.manifest_branch == 'master':
      self.SendCanaryFailureAlert(failing, inflight, no_stat)
      # Note: We used to throttle the tree here. As of
      # https://chromium-review.googlesource.com/#/c/325821/ we no longer do.

  def _HandleStageException(self, exc_info):
    """Decide whether an exception should be treated as fatal."""
    # Canary master already updates the tree status for slave
    # failures. There is no need to mark this stage red. For slave
    # builders, the build itself would already be red. In this case,
    # report a warning instead.
    # pylint: disable=protected-access
    exc_type = exc_info[0]
    if issubclass(exc_type, ImportantBuilderFailedException):
      return self._HandleExceptionAsWarning(exc_info)
    else:
      # In all other cases, exceptions should be treated as fatal.
      return super(CanaryCompletionStage, self)._HandleStageException(exc_info)


class CommitQueueCompletionStage(MasterSlaveSyncCompletionStage):
  """Commits or reports errors to CL's that failed to be validated."""

  def _IsFailureFatal(self, failing, inflight, no_stat, self_destructed=False):
    """Returns a boolean indicating whether the build should fail.

    Args:
      failing: Set of build config names of builders that failed.
      inflight: Set of build config names of builders that are inflight
      no_stat: Set of build config names of builders that had status None.
      self_destructed: Boolean indicating whether this is a master which
        self-destructed and stopped waiting for the running slaves. Default to
        False.

    Returns:
      False if this is a CQ-master and the sync_stage.validation_pool hasn't
      picked up any chump CLs or new CLs, else see the return type of
      _IsFailureFatal of the parent class MasterSlaveSyncCompletionStage.
    """
    if (config_lib.IsMasterCQ(self._run.config) and
        not self.sync_stage.pool.HasPickedUpCLs()):
      # If it's a CQ-master build and the validation pool hasn't picked up any
      # CLs, no slave CQ builds have been scheduled.
      return False

    return super(CommitQueueCompletionStage, self)._IsFailureFatal(
        failing, inflight, no_stat, self_destructed=self_destructed)

  def HandleFailure(self, failing, inflight, no_stat, self_destructed):
    """Handle a build failure or timeout in the Commit Queue.

    Runs MasterSlaveSyncCompletionStage.HandleFailure, and send out infra alerts
    (if needed) for a master build.

    Args:
      failing: Set of builder names that failed.
      inflight: Set of builder names that timed out.
      no_stat: Set of builder names that had status None.
      self_destructed: Boolean indicating whether it's a master build and it
        self-destructed and stopped waiting completion of its slaves.
    """
    # Print out the status about what builds failed or not.
    super(CommitQueueCompletionStage, self).HandleFailure(
        failing, inflight, no_stat, self_destructed)

    if self._run.config.master:
      self.SendInfraAlertIfNeeded(failing, inflight, no_stat, self_destructed)

  def _GetInfraFailMessages(self, failing):
    """Returns a list of messages containing infra failures.

    Args:
      failing: The names of the failing builders.

    Returns:
      A list of build_failure_message.BuildFailureMessage objects.
    """
    msgs = builder_status_lib.GetFailedMessages(
        self._slave_statuses, failing)
    # Filter out None messages because we cannot analyze them.
    return [x for x in msgs if x and
            x.HasExceptionCategories({constants.EXCEPTION_CATEGORY_INFRA,
                                      constants.EXCEPTION_CATEGORY_LAB})]

  def SendInfraAlertIfNeeded(self, failing, inflight, no_stat, self_destructed):
    """Send infra alerts if needed.

    Args:
      failing: The names of the failing builders.
      inflight: The names of the builders that are still running.
      no_stat: The names of the builders that had status None.
      self_destructed: Boolean indicating whether the master build destructed
                       itself and stopped waiting completion of its slaves.
    """
    msgs = [str(x) for x in self._GetInfraFailMessages(failing)]
    # Failed to report a non-None messages is an infra failure.
    slaves = builder_status_lib.GetBuildersWithNoneMessages(
        self._slave_statuses, failing)
    msgs += ['%s failed with unknown reason.' % x for x in slaves]

    if not self_destructed:
      msgs += ['%s timed out' % x for x in inflight]
      msgs += ['%s did not start' % x for x in no_stat]
    elif msgs:
      msgs += ['The master destructed itself and stopped waiting for the '
               'following slaves:']
      msgs += ['%s was still running' % x for x in inflight]
      msgs += ['%s was waiting to start' % x for x in no_stat]

    if msgs:
      builder_name = self._run.config.name
      title = '%s has encountered infra failures:' % (builder_name,)
      msgs.insert(0, title)
      msgs.append('See %s' % self.ConstructDashboardURL())
      msg = '\n\n'.join(msgs)
      subject = '%s infra failures' % (builder_name,)
      extra_fields = {'X-cbuildbot-alert': 'cq-infra-alert'}
      tree_status.SendHealthAlert(self._run, subject, msg,
                                  extra_fields=extra_fields)

  def _WaitForSlavesToComplete(self, manager, build_id, db, builders_array,
                               timeout):
    """Wait for slave builds to complete.

    Args:
      manager: An instance of BuildSpecsManager.
      build_id: The build id of the master build.
      db: An instance of cidb.CIDBConnection.
      builders_array: A list of builder names (strings) of slave builds.
      timeout: Number of seconds to wait for the results.
    """
    # CQ master build needs needs validation_pool to keep track of applied
    # changes and change dependencies.
    return manager.WaitForSlavesToComplete(
        build_id,
        db,
        builders_array,
        pool=self.sync_stage.pool,
        timeout=timeout)

  def PerformStage(self):
    """Run CommitQueueCompletionStage."""
    super(CommitQueueCompletionStage, self).PerformStage()


class PreCQCompletionStage(generic_stages.BuilderStage):
  """Reports the status of a trybot run to Google Storage and Gerrit."""

  def __init__(self, builder_run, sync_stage, success, **kwargs):
    super(PreCQCompletionStage, self).__init__(builder_run, **kwargs)
    self.sync_stage = sync_stage
    self.success = success

  def PerformStage(self):
    # Update Gerrit and Google Storage with the Pre-CQ status.
    if not self.sync_stage.pool:
      logging.warning('No validation pool available. Skipping PreCQCompletion.')
      return

    if self.success:
      self.sync_stage.pool.HandlePreCQPerConfigSuccess()
    else:
      message = self.GetBuildFailureMessage()
      self.sync_stage.pool.HandleValidationFailure([message])


class UpdateChromeosLKGMStage(generic_stages.BuilderStage):
  """Update the CHROMEOS_LKGM file in the chromium repository."""

  def __init__(self, builder_run, **kwargs):
    """Constructor.

    Args:
      builder_run: BuilderRun object.
    """
    super(UpdateChromeosLKGMStage, self).__init__(builder_run, **kwargs)

  def PerformStage(self):
    manager = self._run.attrs.manifest_manager
    cmd = ['chrome_chromeos_lkgm', '--lkgm=%s' % manager.current_version]
    # Always do a dryrun for now so that we can check the output and ensure it
    # is doing the correct thing.
    if self._run.options.debug:
      cmd.append('--dryrun')
    commands.RunBuildScript(self._build_root, cmd, chromite_cmd=True)


class PublishUprevChangesStage(generic_stages.BuilderStage):
  """Makes CQ uprev changes live for developers.

  Push local commits for uprevs, binhost, and portage cache. We resync to the
  latest version of repos as they exist in GoB.  We can't rely on the commits
  we pulled originally because our CL submit stage might have failed in some way
  (GoB sometimes flakes), or we don't want to submit all the CLs (we pushed some
  repos, but rejected others based on CQ repo settings). There might also be
  commits pushed independently (chumped by sheriffs or the precq submitted).
  """

  def __init__(self, builder_run, sync_stage, success, stage_push=False,
               **kwargs):
    """Constructor.

    Args:
      builder_run: BuilderRun object.
      sync_stage: An instance of sync stage.
      success: Boolean indicating whether the build succeeded.
      stage_push: Indicating whether to stage the push instead of pushing
                  it to master, default to False.
    """
    super(PublishUprevChangesStage, self).__init__(builder_run, **kwargs)
    self.sync_stage = sync_stage
    self.success = success
    self.stage_push = stage_push

  def CheckMasterBinhostTest(self, db, build_id):
    """Check whether the master builder has passed BinhostTest stage.

    Args:
      db: cidb.CIDBConnection object.
      build_id: build_id of the master build to check for.

    Returns:
      True if the status of the master build BinhostTest stage is 'pass';
      else, False.
    """
    stage_name = 'BinhostTest'

    if self._build_stage_id is not None and db is not None:
      stages = db.GetBuildStages(build_id)

      # No stages found. BinhostTest stage didn't start or got skipped,
      # in both case we don't need to push commits to the temp pfq branch.
      if not stages:
        logging.warning('no %s stage found in build %s' % (
            stage_name, build_id))
        return False

      stage_status = [s for s in stages if (
          s['name'] == stage_name and
          s['status'] == constants.BUILDER_STATUS_PASSED)]
      if stage_status:
        logging.info('build %s passed stage %s with %s' % (
            build_id, stage_name, stage_status))
        return True
      else:
        logging.warning('build %s stage %s result %s' % (
            build_id, stage_name, stage_status))
        return False

    logging.warning('Not valid build_stage_id %s or db %s or no %s found' % (
        self._build_stage_id, db, stage_name))
    return False

  def CheckSlaveUploadPrebuiltsTest(self, db, build_id):
    """Check if the slaves have passed UploadPrebuilts stage.

    Given the master build id, check if all the important slaves have passed
    the UploadPrebuilts stage.

    Args:
      db: cidb.CIDBConnection object.
      build_id: build_id of the master build to check for.

    Returns:
      True if all the important slaves have passed the stage;
      True if it's in debug environment;
      else, False.
    """
    stage_name = 'UploadPrebuilts'

    if not self._run.config.master:
      logging.warning('The build is not a master')
      return False
    elif self._run.options.buildbot and self._run.options.debug:
      # If it's in debug environment, no slave builds would be triggered,
      # in order to cover the testing on pushing commits to a remote
      # temp branch, return True.
      logging.info('In debug environment, return CheckSlaveUploadPrebuiltsTest'
                   'as True')
      return True
    elif self._build_stage_id is not None and db is not None:
      slave_configs = self._GetSlaveConfigs()
      important_set = set([slave['name'] for slave in slave_configs])

      slave_buildbucket_ids = self.GetScheduledSlaveBuildbucketIds()
      stages = db.GetSlaveStages(
          build_id, buildbucket_ids=slave_buildbucket_ids)

      passed_set = set([s['build_config'] for s in stages if (
          s['name'] == stage_name and
          s['status'] == constants.BUILDER_STATUS_PASSED)])

      if passed_set.issuperset(important_set):
        logging.info('All the important slaves passed %s' % stage_name)
        return True
      else:
        remaining_set = important_set.difference(passed_set)
        logging.warning('slave %s didn\'t pass %s' % (
            remaining_set, stage_name))
        return False
    else:
      logging.warning('Not valid build_stage_id %s or db %s ' % (
          self._build_stage_id, db))
      return False

  def PerformStage(self):
    if (config_lib.IsMasterCQ(self._run.config) and
        not self.sync_stage.pool.HasPickedUpCLs()):
      logging.info('No CLs have been picked up and no slaves have been '
                   'scheduled in this run. Will not publish uprevs.')
      return

    overlays, push_overlays = self._ExtractOverlays()

    staging_branch = None
    if self.stage_push:
      if not config_lib.IsMasterChromePFQ(self._run.config):
        raise ValueError('This build must be a master chrome PFQ build '
                         'when stage_push is True.')
      build_id, db = self._run.GetCIDBHandle()

      # If the master passed BinHostTest and all the important slaves passed
      # UploadPrebuiltsTest, push uprev commits to a staging_branch.
      if (self.CheckMasterBinhostTest(db, build_id) and
          self.CheckSlaveUploadPrebuiltsTest(db, build_id)):
        staging_branch = ('refs/' + constants.PFQ_REF + '/' +
                          constants.STAGING_PFQ_BRANCH_PREFIX + str(build_id))

    assert push_overlays, 'push_overlays must be set to run this stage'

    # If we're a commit queue, we should clean out our local changes, resync,
    # and reapply our uprevs. This is necessary so that 1) we are sure to point
    # at the remote SHA1s, not our local SHA1s; 2) we can avoid doing a
    # rebase; 3) in the case of failure and staging_branch is None, we don't
    # submit the changes that were committed locally.
    #
    # If we're not a commit queue and the build succeeded, we can skip the
    # cleanup here. This is a cheap trick so that the Chrome PFQ pushes its
    # earlier uprev from the SyncChrome stage (it would be a bit tricky to
    # replicate the uprev here, so we'll leave it alone).

    # If we're not a commit queue and staging_branch is not None, we can skip
    # the cleanup here. When staging_branch is not None, we're going to push
    # the local commits generated in AFDOUpdateEbuild stage to the
    # staging_branch, cleaning up repository here will wipe out the local
    # commits.
    if (config_lib.IsCQType(self._run.config.build_type) or
        not (self.success or staging_branch is not None)):
      repo = self.GetRepoRepository()

      # Clean up our root and sync down the latest changes that were
      # submitted.
      repo.BuildRootGitCleanup(self._build_root)

      # Sync down the latest changes we have submitted.
      if self._run.options.sync:
        next_manifest = self._run.config.manifest
        repo.Sync(next_manifest)

      # Commit uprev and portage cache regeneration locally.
      if self._run.options.uprev and self._run.config.uprev:
        commands.UprevPackages(self._build_root, self._boards, overlays)
        commands.RegenPortageCache(push_overlays)

    # When prebuilts is True, if it's a successful run or staging_branch is
    # not None for a master-chrome-pfq run, update binhost conf
    if (self._run.config.prebuilts and
        (self.success or staging_branch is not None)):
      confwriter = prebuilts.BinhostConfWriter(self._run)
      confwriter.Perform()

    # Push the uprev, portage cache, and binhost commits.
    commands.UprevPush(self._build_root, push_overlays,
                       self._run.options.debug,
                       staging_branch=staging_branch)
    if config_lib.IsMasterChromePFQ(self._run.config) and self.success:
      self._run.attrs.metadata.UpdateWithDict({'UprevvedChrome': True})
    if config_lib.IsMasterAndroidPFQ(self._run.config) and self.success:
      self._run.attrs.metadata.UpdateWithDict({'UprevvedAndroid': True})
