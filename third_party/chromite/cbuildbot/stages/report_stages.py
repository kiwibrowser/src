# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the report stages."""


from __future__ import print_function

import datetime
import os
import sys

from infra_libs import ts_mon

from chromite.cbuildbot import cbuildbot_run
from chromite.cbuildbot import commands
from chromite.cbuildbot import goma_util
from chromite.cbuildbot import validation_pool
from chromite.cbuildbot.stages import completion_stages
from chromite.cbuildbot.stages import generic_stages
from chromite.lib.const import waterfall
from chromite.lib import cidb
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import clactions
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import failures_lib
from chromite.lib import git
from chromite.lib import gs
from chromite.lib import metadata_lib
from chromite.lib import metrics
from chromite.lib import osutils
from chromite.lib import patch as cros_patch
from chromite.lib import portage_util
from chromite.lib import results_lib
from chromite.lib import retry_stats
from chromite.lib import risk_report
from chromite.lib import toolchain
from chromite.lib import tree_status
from chromite.lib import triage_lib


site_config = config_lib.GetConfig()


def WriteBasicMetadata(builder_run):
  """Writes basic metadata that should be known at start of execution.

  This method writes to |build_run|'s metadata instance the basic metadata
  values that should be known at the beginning of the first cbuildbot
  execution, prior to any reexecutions.

  In particular, this method does not write any metadata values that depend
  on the builder config, as the config may be modified by patches that are
  applied before the final reexectuion. (exception: the config's name itself)

  This method is safe to run more than once (for instance, once per cbuildbot
  execution) because it will write the same data each time.

  Args:
    builder_run: The BuilderRun instance for this build.
  """
  start_time = results_lib.Results.start_time
  start_time_stamp = cros_build_lib.UserDateTimeFormat(timeval=start_time)

  metadata = {
      # Data for this build.
      'bot-hostname': cros_build_lib.GetHostName(fully_qualified=True),
      'build-number': builder_run.buildnumber,
      'builder-name': builder_run.GetBuilderName(),
      # This is something like https://uberchromegw.corp.google.com/i/chromeos/
      # Note that we are phasing out using the buildbot UI, transitioning
      # instead to luci-milo.
      # Once we phase out completely, we can get rid of this metadata entry.
      'buildbot-url': os.environ.get('BUILDBOT_BUILDBOTURL', ''),
      'buildbot-master-name':
          os.environ.get('BUILDBOT_MASTERNAME', ''),
      'bot-config': builder_run.config['name'],
      'time': {
          'start': start_time_stamp,
      },
      'master_build_id': builder_run.options.master_build_id,
      'suite_scheduling': builder_run.config['suite_scheduling'],
  }

  builder_run.attrs.metadata.UpdateWithDict(metadata)

def WriteTagMetadata(builder_run):
  """Add a 'tags' sub-dict to metadata.

  This is a proof of concept for using tags to help find commonality
  in failures.
  """
  build_id, _ = builder_run.GetCIDBHandle()

  # Yes, these values match general metadata values, but they are just
  # proof of concept, so far.
  tags = {
      'bot_config': builder_run.config['name'],
      'bot_hostname': cros_build_lib.GetHostName(fully_qualified=True),
      'build_id': build_id,
      'build_number': builder_run.buildnumber,
      'builder_name': builder_run.GetBuilderName(),
      'buildbot_url': os.environ.get('BUILDBOT_BUILDBOTURL', ''),
      'buildbot_master_name':
          os.environ.get('BUILDBOT_MASTERNAME', ''),
      'id': ('Build', build_id),
      'master_build_id': builder_run.options.master_build_id,
      'important': builder_run.config['important'],
  }

  # Guess type of bot.
  tags['bot_type'] = 'unknown'
  if '.golo.' in tags['bot_hostname']:
    tags['bot_type'] = 'golo'
  else:
    gce_types = ['beefy', 'standard', 'wimpy']
    for t in gce_types:
      host_string = 'cros-%s' % t
      if host_string in tags['bot_hostname']:
        tags['bot_type'] = 'gce-%s' % t
        break

  # Look up the git version.
  try:
    cmd_result = cros_build_lib.RunCommand(['git', '--version'],
                                           capture_output=True)
    tags['git_version'] = cmd_result.output.strip()
  except cros_build_lib.RunCommandError:
    pass  # If we fail, just don't include the tag.

  # Look up the repo version.
  try:
    cmd_result = cros_build_lib.RunCommand(['repo', '--version'],
                                           capture_output=True)

    # Convert the following output into 'v1.12.17-cr3':
    #
    # repo version v1.12.17-cr3
    #        (from https://chromium.googlesource.com/external/repo.git)
    # repo launcher version 1.21
    #        (from /usr/local/google/home/dgarrett/sand/depot_tools/repo)
    # git version 2.8.0.rc3.226.g39d4020
    # Python 2.7.6 (default, Jun 22 2015, 17:58:13)
    # [GCC 4.8.2]
    tags['repo_version'] = cmd_result.output.splitlines()[0].split(' ')[-1]
  except (cros_build_lib.RunCommandError, IndexError):
    pass  # If we fail, just don't include the tag.

  builder_run.attrs.metadata.UpdateKeyDictWithDict(constants.METADATA_TAGS,
                                                   tags)

def GetChildConfigListMetadata(child_configs, config_status_map):
  """Creates a list for the child configs metadata.

  This creates a list of child config dictionaries from the given child
  configs, optionally adding the final status if the success map is
  specified.

  Args:
    child_configs: The list of child configs for this build.
    config_status_map: The map of config name to final build status.

  Returns:
    List of child config dictionaries, with optional final status
  """
  child_config_list = []
  for c in child_configs:
    pass_fail_status = None
    if config_status_map:
      if config_status_map[c['name']]:
        pass_fail_status = constants.BUILDER_STATUS_PASSED
      else:
        pass_fail_status = constants.BUILDER_STATUS_FAILED
    child_config_list.append({'name': c['name'],
                              'boards': c['boards'],
                              'status': pass_fail_status})
  return child_config_list


def _UploadAndLinkGomaLogIfNecessary(
    stage_name, goma_dir, goma_client_json, goma_tmp_dir):
  """Uploads the logs for goma, if needed. Also create a link to the visualizer.

  If |goma_tmp_dir| is given, |goma_dir| and |goma_client_json| must not be
  None.

  Args:
    stage_name: Name of the stage where goma is used.
    goma_dir: Path to goma installed directory.
    goma_client_json: Path to the service account json file.
    goma_tmp_dir: Goma's working directory.
  """
  if not goma_tmp_dir:
    return

  goma = goma_util.Goma(goma_dir, goma_client_json, goma_tmp_dir=goma_tmp_dir)
  # Just in case, stop the goma. E.g. In case of timeout, we do not want to
  # keep goma compiler_proxy running.
  goma.Stop()
  goma_urls = goma.UploadLogs()
  if goma_urls:
    for label, url in goma_urls:
      logging.PrintBuildbotLink('%s %s' % (stage_name, label), url)


class BuildStartStage(generic_stages.BuilderStage):
  """The first stage to run.

  This stage writes a few basic metadata values that are known at the start of
  build, and inserts the build into the database, if appropriate.
  """

  def _GetBuildTimeoutSeconds(self):
    """Get the overall build timeout to be published to cidb.

    Returns:
      Timeout in seconds. None if no sensible timeout can be inferred.
    """
    timeout_seconds = self._run.options.timeout
    if self._run.config.master:
      master_timeout = self._run.config.build_timeout
      if timeout_seconds > 0:
        master_timeout = min(master_timeout, timeout_seconds)
      return master_timeout

    return timeout_seconds if timeout_seconds > 0 else None

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    if self._run.config['doc']:
      logging.PrintBuildbotLink('Builder documentation',
                                self._run.config['doc'])

    WriteBasicMetadata(self._run)

    # This is a heuristic value for |important|, since patches that get applied
    # later in the build might change the config. We write it now anyway,
    # because in case the build fails before Sync, it is better to have this
    # heuristic value than None. In BuildReexecutionFinishedStage, we re-write
    # the definitive value.
    self._run.attrs.metadata.UpdateWithDict(
        {'important': self._run.config['important']})

    d = self._run.attrs.metadata.GetDict()

    # BuildStartStage should only run once per build. But just in case it
    # is somehow running a second time, we do not want to insert an additional
    # database entry. Detect if a database entry has been inserted already
    # and if so quit the stage.
    if 'build_id' in d:
      logging.info('Already have build_id %s, not inserting an entry.',
                   d['build_id'])
      return

    # Note: In other build stages we use self._run.GetCIDBHandle to fetch
    # a cidb handle. However, since we don't yet have a build_id, we can't
    # do that here.
    if cidb.CIDBConnectionFactory.IsCIDBSetup():
      db_type = cidb.CIDBConnectionFactory.GetCIDBConnectionType()
      db = cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder()
      if db:
        wfall = d['buildbot-master-name']
        try:
          build_id = db.InsertBuild(
              builder_name=d['builder-name'],
              waterfall=wfall,
              build_number=d['build-number'],
              build_config=d['bot-config'],
              bot_hostname=d['bot-hostname'],
              master_build_id=d['master_build_id'],
              timeout_seconds=self._GetBuildTimeoutSeconds(),
              important=d['important'],
              buildbucket_id=self._run.options.buildbucket_id)
        except Exception as e:
          logging.error('Error: %s\n If the buildbucket_id to insert is '
                        'duplicated to the buildbucket_id of an old build and '
                        'the old build was canceled because of a waterfall '
                        'master restart, please ignore this error. Else, '
                        'the error needs more investigation. More context: '
                        'crbug.com/679974 and crbug.com/685889', e)
          raise e

        self._run.attrs.metadata.UpdateWithDict({'build_id': build_id,
                                                 'db_type': db_type})
        logging.info('Inserted build_id %s into cidb database type %s.',
                     build_id, db_type)
        logging.PrintBuildbotStepText('database: %s, build_id: %s' %
                                      (db_type, build_id))

        master_build_id = d['master_build_id']
        if master_build_id is not None:
          master_build_status = db.GetBuildStatus(master_build_id)
          if master_build_status['buildbucket_id']:
            master_url = tree_status.ConstructLegolandBuildURL(
                master_build_status['buildbucket_id'])
          else:
            master_url = tree_status.ConstructDashboardURL(
                master_build_status['waterfall'],
                master_build_status['builder_name'],
                master_build_status['build_number'])
          logging.PrintBuildbotLink('Link to master build', master_url)

    # Write the tag metadata last so that a build_id is available.
    WriteTagMetadata(self._run)

  def HandleSkip(self):
    """Ensure that re-executions use the same db instance as initial db."""
    metadata_dict = self._run.attrs.metadata.GetDict()
    if 'build_id' in metadata_dict:
      db_type = cidb.CIDBConnectionFactory.GetCIDBConnectionType()
      if not 'db_type' in metadata_dict:
        # This will only execute while this CL is in the commit queue. After
        # this CL lands, this block can be removed.
        self._run.attrs.metadata.UpdateWithDict({'db_type': db_type})
        return

      if db_type != metadata_dict['db_type']:
        cidb.CIDBConnectionFactory.InvalidateCIDBSetup()
        raise AssertionError('Invalid attempt to switch from database %s to '
                             '%s.' % (metadata_dict['db_type'], db_type))


class SlaveFailureSummaryStage(generic_stages.BuilderStage):
  """Stage which summarizes and links to the failures of slave builds."""

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    if not self._run.config.master:
      logging.info('This stage is only meaningful for master builds. '
                   'Doing nothing.')
      return

    build_id, db = self._run.GetCIDBHandle()

    if not db:
      logging.info('No cidb connection for this build. '
                   'Doing nothing.')
      return

    slave_buildbucket_ids = self.GetScheduledSlaveBuildbucketIds()
    slave_failures = db.GetSlaveFailures(
        build_id, buildbucket_ids=slave_buildbucket_ids)
    failures_by_build = cros_build_lib.GroupNamedtuplesByKey(
        slave_failures, 'build_id')
    for build_id, build_failures in sorted(failures_by_build.items()):
      failures_by_stage = cros_build_lib.GroupNamedtuplesByKey(
          build_failures, 'build_stage_id')
      # Surface a link to each slave stage that failed, in stage_id sorted
      # order.
      for stage_id in sorted(failures_by_stage):
        failure = failures_by_stage[stage_id][0]
        # Ignore failures that did not cause their enclosing stage to fail.
        # Ignore slave builds that are still inflight, because some stage logs
        # might not have been printed to buildbot yet.
        # TODO(akeshet) revisit this approach, if we seem to be suppressing
        # useful information as a result of it.
        if (failure.stage_status != constants.BUILDER_STATUS_FAILED or
            failure.build_status == constants.BUILDER_STATUS_INFLIGHT):
          continue
        waterfall_url = waterfall.WATERFALL_TO_DASHBOARD[failure.waterfall]
        slave_stage_url = tree_status.ConstructBuildStageURL(
            waterfall_url,
            failure.builder_name,
            failure.build_number,
            failure.stage_name)
        logging.PrintBuildbotLink('%s %s' % (failure.build_config,
                                             failure.stage_name),
                                  slave_stage_url)


class BuildReexecutionFinishedStage(generic_stages.BuilderStage,
                                    generic_stages.ArchivingStageMixin):
  """The first stage to run after the final cbuildbot reexecution.

  This stage is the first stage run after the final cbuildbot
  bootstrap/reexecution. By the time this stage is run, the sync stages
  are complete and version numbers of chromeos are known (though chrome
  version may not be known until SyncChrome).

  This stage writes metadata values that are first known after the final
  reexecution (such as those that come from the config). This stage also
  updates the build's cidb entry if appropriate.

  Where possible, metadata that is already known at this time should be
  written at this time rather than in ReportStage.
  """

  def _AbortPreviousHWTestSuites(self, milestone):
    """Abort any outstanding synchronous hwtest suites from this builder."""
    # Only try to clean up previous HWTests if this is really running on one of
    # our builders in a non-trybot build.
    debug = (self._run.options.remote_trybot or
             (not self._run.options.buildbot) or
             self._run.options.debug)
    build_id, db = self._run.GetCIDBHandle()
    if db:
      builds = db.GetBuildHistory(self._run.config.name, 2,
                                  milestone_version=milestone,
                                  ignore_build_id=build_id)
      for build in builds:
        old_version = build['full_version']
        if old_version is None:
          continue
        if build['status'] == constants.BUILDER_STATUS_PASSED:
          continue
        for suite_config in self._run.config.hw_tests:
          if not suite_config.async:
            commands.AbortHWTests(self._run.config.name, old_version,
                                  debug, suite_config.suite)

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    config = self._run.config
    build_root = self._build_root

    logging.info('Build re-executions have finished. Chromite source '
                 'will not be modified for remainder of run.')
    logging.info("config['important']=%s", config['important'])
    logging.PrintBuildbotStepText(
        "config['important']=%s" % config['important'])

    # Flat list of all child config boards. Since child configs
    # are not allowed to have children, it is not necessary to search
    # deeper than one generation.
    child_configs = GetChildConfigListMetadata(
        child_configs=config['child_configs'], config_status_map=None)

    sdk_verinfo = cros_build_lib.LoadKeyValueFile(
        os.path.join(build_root, constants.SDK_VERSION_FILE),
        ignore_missing=True)

    verinfo = self._run.GetVersionInfo()
    platform_tag = getattr(self._run.attrs, 'release_tag')
    if not platform_tag:
      platform_tag = verinfo.VersionString()

    version = {
        'full': self._run.GetVersion(),
        'milestone': verinfo.chrome_branch,
        'platform': platform_tag,
    }

    metadata = {
        # Version of the metadata format.
        'metadata-version': '2',
        'boards': config['boards'],
        'child-configs': child_configs,
        'build_type': config['build_type'],
        'important': config['important'],

        # Data for the toolchain used.
        'sdk-version': sdk_verinfo.get('SDK_LATEST_VERSION', '<unknown>'),
        'toolchain-url': sdk_verinfo.get('TC_PATH', '<unknown>'),
    }

    if len(config['boards']) == 1:
      metadata['toolchain-tuple'] = toolchain.GetToolchainTupleForBoard(
          config['boards'][0], buildroot=build_root)

    logging.info('Metadata being written: %s', metadata)
    self._run.attrs.metadata.UpdateWithDict(metadata)

    toolchains = set()
    toolchain_tuples = []
    primary_toolchains = []
    for board in config['boards']:
      toolchain_tuple = toolchain.GetToolchainTupleForBoard(
          board, buildroot=build_root)
      toolchains |= set(toolchain_tuple)
      toolchain_tuples.append(','.join(toolchain_tuple))
      if len(toolchain_tuple):
        primary_toolchains.append(toolchain_tuple[0])

    # Update 'version' separately to avoid overwriting the existing
    # entries in it (e.g. PFQ builders may have written the Chrome
    # version to uprev).
    logging.info("Metadata 'version' being written: %s", version)
    self._run.attrs.metadata.UpdateKeyDictWithDict('version', version)

    tags = {
        'boards': config['boards'],
        'child_config_names': [cc['name'] for cc in child_configs],
        'build_type': config['build_type'],
        'important': config['important'],

        # Data for the toolchain used.
        'sdk_version': sdk_verinfo.get('SDK_LATEST_VERSION', '<unknown>'),
        'toolchain_url': sdk_verinfo.get('TC_PATH', '<unknown>'),
        'toolchains': list(toolchains),
        'toolchain_tuples': toolchain_tuples,
        'primary_toolchains': primary_toolchains,
    }
    full_version = self._run.attrs.metadata.GetValue('version')
    tags.update({'version_%s' % v: full_version[v] for v in full_version})
    self._run.attrs.metadata.UpdateKeyDictWithDict(constants.METADATA_TAGS,
                                                   tags)

    # Ensure that all boards and child config boards have a per-board
    # metadata subdict.
    for b in config['boards']:
      self._run.attrs.metadata.UpdateBoardDictWithDict(b, {})

    for cc in child_configs:
      for b in cc['boards']:
        self._run.attrs.metadata.UpdateBoardDictWithDict(b, {})

    # Upload build metadata (and write it to database if necessary)
    self.UploadMetadata(filename=constants.PARTIAL_METADATA_JSON)

    # Write child-per-build and board-per-build rows to database
    build_id, db = self._run.GetCIDBHandle()
    if db:
      # TODO(akeshet): replace this with a GetValue call once crbug.com/406522
      # is resolved
      per_board_dict = self._run.attrs.metadata.GetDict()['board-metadata']
      for board, board_metadata in per_board_dict.items():
        db.InsertBoardPerBuild(build_id, board)
        if board_metadata:
          db.UpdateBoardPerBuildMetadata(build_id, board, board_metadata)
      for child_config in self._run.attrs.metadata.GetValue('child-configs'):
        db.InsertChildConfigPerBuild(build_id, child_config['name'])

      # If this build has a master build, ensure that the master full_version
      # is the same as this build's full_version. This is a sanity check to
      # avoid bugs in master-slave logic.
      master_id = self._run.attrs.metadata.GetDict().get('master_build_id')
      if master_id is not None:
        master_full_version = db.GetBuildStatus(master_id)['full_version']
        my_full_version = self._run.attrs.metadata.GetValue('version').get(
            'full')
        if master_full_version != my_full_version:
          raise failures_lib.MasterSlaveVersionMismatchFailure(
              'Master build id %s has full_version %s, while slave version is '
              '%s.' % (master_id, master_full_version, my_full_version))

    # Abort previous hw test suites. This happens after reexecution as it
    # requires chromite/third_party/swarming.client, which is not available
    # untill after reexecution.
    self._AbortPreviousHWTestSuites(version['milestone'])


class ConfigDumpStage(generic_stages.BuilderStage):
  """Stage that dumps the current build config to the build log.

  This stage runs immediately after BuildReexecutionFinishedStage, at which
  point the build is finalized.
  """

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    """Dump the running config to info logs."""
    config = self._run.config
    logging.info('The current build config is dumped below:\n%s',
                 config_lib.PrettyJsonDict(config))


class ReportStage(generic_stages.BuilderStage,
                  generic_stages.ArchivingStageMixin):
  """Summarize all the builds."""

  _STATS_HISTORY_DAYS = 7

  def __init__(self, builder_run, completion_instance, **kwargs):
    super(ReportStage, self).__init__(builder_run, **kwargs)

    # TODO(mtennant): All these should be retrieved from builder_run instead.
    # Or, more correctly, the info currently retrieved from these stages should
    # be stored and retrieved from builder_run instead.
    self._completion_instance = completion_instance
    self._post_completion = False

  def _UpdateRunStreak(self, builder_run, final_status):
    """Update the streak counter for this builder, if applicable, and notify.

    Update the pass/fail streak counter for the builder.  If the new
    streak should trigger a notification email then send it now.

    Args:
      builder_run: BuilderRun for this run.
      final_status: Final status string for this run.
    """
    if builder_run.InEmailReportingEnvironment():
      streak_value = self._UpdateStreakCounter(
          final_status=final_status, counter_name=builder_run.config.name,
          dry_run=self._run.debug)
      verb = 'passed' if streak_value > 0 else 'failed'
      logging.info('Builder %s has %s %s time(s) in a row.',
                   builder_run.config.name, verb, abs(streak_value))
      # See if updated streak should trigger a notification email.
      if (builder_run.config.health_alert_recipients and
          builder_run.config.health_threshold > 0 and
          streak_value <= -builder_run.config.health_threshold):
        logging.info('Builder failed %i consecutive times, sending health '
                     'alert email to %s.', -streak_value,
                     builder_run.config.health_alert_recipients)

        subject = '%s health alert' % builder_run.config.name
        body = self._HealthAlertMessage(-streak_value)
        extra_fields = {'X-cbuildbot-alert': 'cq-health'}
        tree_status.SendHealthAlert(builder_run, subject, body,
                                    extra_fields=extra_fields)

  def _UpdateStreakCounter(self, final_status, counter_name,
                           dry_run=False):
    """Update the given streak counter based on the final status of build.

    A streak counter counts the number of consecutive passes or failures of
    a particular builder. Consecutive passes are indicated by a positive value,
    consecutive failures by a negative value.

    Args:
      final_status: String indicating final status of build,
                    constants.BUILDER_STATUS_PASSED indicating success.
      counter_name: Name of counter to increment, typically the name of the
                    build config.
      dry_run: Pretend to update counter only. Default: False.

    Returns:
      The new value of the streak counter.
    """
    gs_ctx = gs.GSContext(dry_run=dry_run)
    counter_url = os.path.join(site_config.params.MANIFEST_VERSIONS_GS_URL,
                               constants.STREAK_COUNTERS,
                               counter_name)
    gs_counter = gs.GSCounter(gs_ctx, counter_url)

    if final_status == constants.BUILDER_STATUS_PASSED:
      streak_value = gs_counter.StreakIncrement()
    else:
      streak_value = gs_counter.StreakDecrement()

    return streak_value

  def _HealthAlertMessage(self, fail_count):
    """Returns the body of a health alert email message."""
    return 'The builder named %s has failed %i consecutive times. See %s' % (
        self._run.config['name'], fail_count, self.ConstructDashboardURL())

  def _SendPreCQInfraAlertMessageIfNeeded(self):
    """Send alerts on Pre-CQ infra failures."""
    msg = self.GetBuildFailureMessage()
    pre_cq = self._run.config.pre_cq
    if (pre_cq and
        msg.HasExceptionCategories(
            {constants.EXCEPTION_CATEGORY_INFRA,
             constants.EXCEPTION_CATEGORY_LAB})):
      name = self._run.config.name
      title = 'pre-cq infra failures'
      body = ['%s failed on %s' % (name, cros_build_lib.GetHostName()),
              '%s' % msg]
      extra_fields = {'X-cbuildbot-alert': 'pre-cq-infra-alert'}
      tree_status.SendHealthAlert(self._run, title, '\n\n'.join(body),
                                  extra_fields=extra_fields)

  def _LinkArtifacts(self, builder_run):
    """Upload an HTML index and uploaded.json for artifacts.

    If there are no artifacts in the archive then do nothing.

    Args:
      builder_run: BuilderRun object for this run.
    """
    archive = builder_run.GetArchive()
    archive_path = archive.archive_path

    boards = builder_run.config.boards
    if boards:
      board_names = ' '.join(boards)
    else:
      boards = [None]
      board_names = '<no board>'

    # See if there are any artifacts found for this run.
    uploaded = os.path.join(archive_path, commands.UPLOADED_LIST_FILENAME)
    if not os.path.exists(uploaded):
      # UPLOADED doesn't exist.  Normal if Archive stage never ran, which
      # is possibly normal.  Regardless, no archive index is needed.
      logging.info('No archived artifacts found for %s run (%s)',
                   builder_run.config.name, board_names)
      return

    logging.PrintBuildbotSetBuildProperty('artifact_link', archive.upload_url)

    uploaded_json = 'uploaded.json'
    commands.GenerateUploadJSON(os.path.join(archive_path, uploaded_json),
                                archive_path, uploaded)
    commands.UploadArchivedFile(
        archive_path, [archive.upload_url], uploaded_json,
        debug=self._run.debug, update_list=True, acl=self.acl)

    if builder_run.config.internal:
      # Internal builds simply link to pantheon directories, which require
      # authenticated access that most Googlers should have.
      artifacts_url = archive.download_url

    else:
      # External builds must allow unauthenticated access to build artifacts.
      # GS doesn't let unauthenticated users browse selected locations without
      # being able to browse everything (which would expose secret stuff).
      # So, we upload an index.html file and link to it instead of the
      # directory.
      title = 'Artifacts Index: %(board)s / %(version)s (%(config)s config)' % {
          'board': board_names,
          'config': builder_run.config.name,
          'version': builder_run.GetVersion(),
      }

      files = osutils.ReadFile(uploaded).splitlines() + [
          '.|Google Storage Index',
          '..|',
      ]

      index = os.path.join(archive_path, 'index.html')

      # TODO (sbasi) crbug.com/362776: Rework the way we do uploading to
      # multiple buckets. Currently this can only be done in the Archive Stage
      # therefore index.html will only end up in the normal Chrome OS bucket.
      commands.GenerateHtmlIndex(index, files, title=title,
                                 url_base=gs.GsUrlToHttp(archive.upload_url))
      commands.UploadArchivedFile(
          archive_path, [archive.upload_url], os.path.basename(index),
          debug=self._run.debug, acl=self.acl)

      artifacts_url = os.path.join(archive.download_url_file, 'index.html')

    links_build_description = '%s/%s' % (builder_run.config.name,
                                         archive.version)
    logging.PrintBuildbotLink('Artifacts[%s]' % links_build_description,
                              artifacts_url)

  def _UploadBuildStagesTimeline(self, builder_run, build_id, db):
    """Upload an HTML timeline for the build stages at remote archive location.

    Args:
      builder_run: BuilderRun object for this run.
      build_id: CIDB id for the current build.
      db: CIDBConnection instance.

    Returns:
      If an index file is uploaded then a dict is returned where each value
        is the same (the URL for the uploaded HTML index) and the keys are
        the boards it applies to, including None if applicable.  If no index
        file is uploaded then this returns None.
    """
    archive = builder_run.GetArchive()
    archive_path = archive.archive_path

    config = builder_run.config
    boards = config.boards
    if boards:
      board_names = ' '.join(boards)
    else:
      boards = [None]
      board_names = '<no board>'

    timeline_file = 'timeline-stages.html'
    timeline = os.path.join(archive_path, timeline_file)

    # Gather information about this build from CIDB.
    stages = db.GetBuildStages(build_id)
    # Many stages are started in parallel after the build finishes. Stages are
    # sorted by start_time first bceause it shows that progression most
    # clearly. Sort by finish_time secondarily to display those paralllel
    # stages cleanly.
    epoch = datetime.datetime.fromtimestamp(0)
    stages.sort(key=lambda stage: (stage['start_time'] or epoch,
                                   stage['finish_time'] or epoch))
    rows = ((s['name'], s['start_time'], s['finish_time']) for s in stages)

    # Prepare html head.
    title = ('Build Stages Timeline: %s / %s (%s config)' %
             (board_names, builder_run.GetVersion(), config.name))

    commands.GenerateHtmlTimeline(timeline, rows, title=title)
    commands.UploadArchivedFile(
        archive_path, [archive.upload_url], os.path.basename(timeline),
        debug=self._run.debug, update_list=True, acl=self.acl)
    return os.path.join(archive.download_url_file, timeline_file)

  def _UploadSlavesTimeline(self, builder_run, build_id, db):
    """Upload an HTML timeline for the slaves at remote archive location.

    Args:
      builder_run: BuilderRun object for this run.
      build_id: CIDB id for the master build.
      db: CIDBConnection instance.

    Returns:
      The URL of the timeline is returned if slave builds exists.  If no
        slave builds exists then this returns None.
    """
    archive = builder_run.GetArchive()
    archive_path = archive.archive_path

    config = builder_run.config
    boards = config.boards
    if boards:
      board_names = ' '.join(boards)
    else:
      boards = [None]
      board_names = '<no board>'

    timeline_file = 'timeline-slaves.html'
    timeline = os.path.join(archive_path, timeline_file)

    # Gather information about this build from CIDB.
    statuses = db.GetSlaveStatuses(build_id)
    if statuses is None or len(statuses) == 0:
      return None
    # Slaves may be started at slightly different times, but what matters most
    # is which slave is the bottleneck - namely, which slave finishes last.
    # Therefore, sort primarily by finish_time.
    epoch = datetime.datetime.fromtimestamp(0)
    statuses.sort(key=lambda stage: (stage['finish_time'] or epoch,
                                     stage['start_time'] or epoch))
    rows = (('%s - %s' % (s['build_config'], s['build_number']),
             s['start_time'], s['finish_time']) for s in statuses)

    # Prepare html head.
    title = ('Slave Builds Timeline: %s / %s (%s config)' %
             (board_names, builder_run.GetVersion(), config.name))

    commands.GenerateHtmlTimeline(timeline, rows, title=title)
    commands.UploadArchivedFile(
        archive_path, [archive.upload_url], os.path.basename(timeline),
        debug=self._run.debug, update_list=True, acl=self.acl)
    return os.path.join(archive.download_url_file, timeline_file)

  def GetReportMetadata(self, config=None, stage=None, final_status=None,
                        completion_instance=None):
    """Generate ReportStage metadata.

    Args:
      config: The build config for this run.  Defaults to self._run.config.
      stage: The stage name that this metadata file is being uploaded for.
      final_status: Whether the build passed or failed. If None, the build
        will be treated as still running.
      completion_instance: The stage instance that was used to wait for slave
        completion. Used to add slave build information to master builder's
        metadata. If None, no such status information will be included. It not
        None, this should be a derivative of MasterSlaveSyncCompletionStage.

    Returns:
      A JSON-able dictionary representation of the metadata object.
    """
    builder_run = self._run
    config = config or builder_run.config

    get_statuses_from_slaves = (
        config['master'] and
        completion_instance and
        isinstance(completion_instance,
                   completion_stages.MasterSlaveSyncCompletionStage)
    )

    child_configs_list = GetChildConfigListMetadata(
        child_configs=config['child_configs'],
        config_status_map=completion_stages.GetBuilderSuccessMap(self._run,
                                                                 final_status))

    return metadata_lib.CBuildbotMetadata.GetReportMetadataDict(
        builder_run, get_statuses_from_slaves,
        config, stage, final_status, completion_instance,
        child_configs_list)

  def ArchiveResults(self, final_status, build_id, db):
    """Archive our build results.

    Args:
      final_status: constants.BUILDER_STATUS_PASSED or
                    constants.BUILDER_STATUS_FAILED
      build_id: CIDB id for the current build.
      db: CIDBConnection instance.
    """
    # Make sure local archive directory is prepared, if it was not already.
    if not os.path.exists(self.archive_path):
      self.archive.SetupArchivePath()

    # Upload metadata, and update the pass/fail streak counter for the main
    # run only. These aren't needed for the child builder runs.
    self.UploadMetadata(export=True)
    self._UpdateRunStreak(self._run, final_status)

    # Alert if the Pre-CQ has infra failures.
    if final_status == constants.BUILDER_STATUS_FAILED:
      self._SendPreCQInfraAlertMessageIfNeeded()

    # Iterate through each builder run, whether there is just the main one
    # or multiple child builder runs.
    for builder_run in self._run.GetUngroupedBuilderRuns():
      if db is not None:
        timeline = self._UploadBuildStagesTimeline(builder_run, build_id, db)
        logging.PrintBuildbotLink('Build stages timeline', timeline)

        timeline = self._UploadSlavesTimeline(builder_run, build_id, db)
        if timeline is not None:
          logging.PrintBuildbotLink('Slaves timeline', timeline)

      if build_id is not None:
        details_link = tree_status.ConstructViceroyBuildDetailsURL(build_id)
        logging.PrintBuildbotLink('Build details', details_link)
        suite_details_link = tree_status.ConstructGoldenEyeSuiteDetailsURL(
            build_id=build_id)
        logging.PrintBuildbotLink('Build details', details_link)
        logging.PrintBuildbotLink('Suite details', suite_details_link)

      # Generate links to archived artifacts if there are any.  All the
      # archived artifacts for one run/config are in one location, so the link
      # is only specific to each run/config.  In theory multiple boards could
      # share that archive, but in practice it is usually one board.  A
      # run/config without a board will also usually not have artifacts to
      # archive, but that restriction is not assumed here.
      self._LinkArtifacts(builder_run)

      # Check if the builder_run is tied to any boards and if so get all
      # upload urls.
      if final_status == constants.BUILDER_STATUS_PASSED:
        # Update the LATEST files if the build passed.
        try:
          upload_urls = self._GetUploadUrls(
              'LATEST-*', builder_run=builder_run)
        except portage_util.MissingOverlayException as e:
          # If the build failed prematurely, some overlays might be
          # missing. Ignore them in this stage.
          logging.warning(e)
        else:
          if upload_urls:
            archive = builder_run.GetArchive()
            archive.UpdateLatestMarkers(builder_run.manifest_branch,
                                        builder_run.debug,
                                        upload_urls=upload_urls)

  def PerformStage(self):
    """Perform the actual work for this stage.

    This includes final metadata archival, and update CIDB with our final status
    as well as producting a logged build result summary.
    """
    build_id, db = self._run.GetCIDBHandle()
    if results_lib.Results.BuildSucceededSoFar(db, build_id, self.name):
      final_status = constants.BUILDER_STATUS_PASSED
    else:
      final_status = constants.BUILDER_STATUS_FAILED

    if not hasattr(self._run.attrs, 'release_tag'):
      # If, for some reason, sync stage was not completed and
      # release_tag was not set. Set it to None here because
      # ArchiveResults() depends the existence of this attr.
      self._run.attrs.release_tag = None

    # Set up our report metadata.
    self._run.attrs.metadata.UpdateWithDict(
        self.GetReportMetadata(final_status=final_status,
                               completion_instance=self._completion_instance))

    # Add tags for the arches and statuses of the build.
    # arches requires crossdev which isn't available at the early part of the
    # build.
    arches = []
    for board in self._run.config['boards']:
      toolchains = toolchain.GetToolchainsForBoard(
          board, buildroot=self._build_root)
      default = toolchain.FilterToolchains(toolchains, 'default', True).keys()
      if len(default):
        try:
          arches.append(toolchain.GetArchForTarget(default[0]))
        except cros_build_lib.RunCommandError as e:
          logging.warning(
              'Unable to retrieve arch for board %s default toolchain %s: %s' %
              (board, default, e))
    tags = {
        'arches': arches,
        'status': final_status,
    }
    results = self._run.attrs.metadata.GetValue('results')
    for stage in results:
      tags['stage_status:%s' % stage['name']] = stage['status']
      tags['stage_summary:%s' % stage['name']] = stage['summary']
    self._run.attrs.metadata.UpdateKeyDictWithDict(constants.METADATA_TAGS,
                                                   tags)

    # Some operations can only be performed if a valid version is available.
    try:
      self._run.GetVersionInfo()
      self.ArchiveResults(final_status, build_id, db)
      metadata_url = os.path.join(self.upload_url, constants.METADATA_JSON)
    except cbuildbot_run.VersionNotSetError:
      logging.error('A valid version was never set for this run. '
                    'Can not archive results.')
      metadata_url = ''

    results_lib.Results.Report(
        sys.stdout, current_version=(self._run.attrs.release_tag or ''))

    # Upload goma log if used for BuildPackage and TestSimpleChrome.
    _UploadAndLinkGomaLogIfNecessary(
        'BuildPackages',
        self._run.options.goma_dir,
        self._run.options.goma_client_json,
        self._run.attrs.metadata.GetValueWithDefault('goma_tmp_dir'))
    _UploadAndLinkGomaLogIfNecessary(
        'TestSimpleChromeWorkflow',
        self._run.options.goma_dir,
        self._run.options.goma_client_json,
        self._run.attrs.metadata.GetValueWithDefault(
            'goma_tmp_dir_for_simple_chrome'))

    if db:
      status_for_db = final_status

      child_metadatas = self._run.attrs.metadata.GetDict().get(
          'child-configs', [])
      for child_metadata in child_metadatas:
        db.FinishChildConfig(build_id, child_metadata['name'],
                             child_metadata['status'])

      # TODO(pprabhu): After BuildData and CBuildbotMetdata are merged, remove
      # this extra temporary object creation.
      # XXX:HACK We're creating a BuildData with an empty URL. Don't try to
      # MarkGathered this object.
      build_data = metadata_lib.BuildData("",
                                          self._run.attrs.metadata.GetDict())
      # TODO(akeshet): Find a clearer way to get the "primary upload url" for
      # the metadata.json file. One alternative is _GetUploadUrls(...)[0].
      # Today it seems that element 0 of its return list is the primary upload
      # url, but there is no guarantee or unit test coverage of that.
      db.FinishBuild(build_id, status=status_for_db,
                     summary=build_data.failure_message,
                     metadata_url=metadata_url)

      duration = self._GetBuildDuration()

      mon_fields = {'status': status_for_db,
                    'build_config': self._run.config.name,
                    'important': self._run.config.important}
      metrics.Counter(constants.MON_BUILD_COMP_COUNT).increment(
          fields=mon_fields)
      metrics.CumulativeSecondsDistribution(constants.MON_BUILD_DURATION).add(
          duration, fields=mon_fields)

      if self._run.options.sanity_check_build:
        metrics.Counter(constants.MON_BUILD_SANITY_COMP_COUNT).increment(
            fields=mon_fields)
        metrics.Gauge(
            constants.MON_BUILD_SANITY_ID,
            description=("The build number of the latest sanity build. Used "
                         "for recovering the link to the latest failing build "
                         "in the alert when a sanity build fails."),
            field_spec=[ts_mon.StringField('status'),
                        ts_mon.StringField('build_config'),
                        ts_mon.StringField('builder_name'),
                        ts_mon.BooleanField('important')]
        ).set(self._run.buildnumber,
              fields=dict(mon_fields, builder_name=self._run.GetBuilderName()))

      if config_lib.IsMasterCQ(self._run.config):
        self._RunRiskReport()
        self_destructed = self._run.attrs.metadata.GetValueWithDefault(
            constants.SELF_DESTRUCTED_BUILD, False)
        mon_fields = {'status': status_for_db,
                      'self_destructed': self_destructed}
        metrics.CumulativeSecondsDistribution(
            constants.MON_CQ_BUILD_DURATION).add(duration, fields=mon_fields)
        annotator_link = tree_status.ConstructAnnotatorURL(build_id)
        logging.PrintBuildbotLink('Build annotator', annotator_link)

      # From this point forward, treat all exceptions as warnings.
      self._post_completion = True

      # Dump report about things we retry.
      retry_stats.ReportStats(sys.stdout)

  def _RunRiskReport(self):
    """Fetches the CL-Scanner risk report and prints step text and links."""
    build_id, _ = self._run.GetCIDBHandle()
    report = risk_report.GetCLRiskReport(build_id)
    for link_text, url in sorted(report.iteritems()):
      logging.PrintBuildbotLink(link_text, url)

  def _GetBuildDuration(self):
    """Fetches the duration of this build in seconds, from cidb.

    This method should be called only after the build has been Finished in
    cidb.
    """
    build_id, db = self._run.GetCIDBHandle()
    if db:
      build_info = db.GetBuildStatus(build_id)
      duration = (build_info['finish_time'] -
                  build_info['start_time']).total_seconds()
      return duration
    return 0

  def _HandleStageException(self, exc_info):
    """Override and don't set status to FAIL but FORGIVEN instead."""
    if self._post_completion:
      # If we've already reported the stage completion, treat exceptions as
      # warnings so we keep reported success in-line with waterfall displayed
      # results.
      return self._HandleExceptionAsWarning(exc_info)

    return super(ReportStage, self)._HandleStageException(exc_info)


class DetectRelevantChangesStage(generic_stages.BoardSpecificBuilderStage):
  """Stage to detect irrelevant changes for slave per board base.

  This stage will get the irrelevant changes for the current board of the build,
  and record the irrelevant changes and the subsystem of the relevant changes
  test to board_metadata.

  Changes relevant to this build will be logged to create links to them
  in the builder output.
  """

  def __init__(self, builder_run, board, changes, suffix=None, **kwargs):
    super(DetectRelevantChangesStage, self).__init__(builder_run, board,
                                                     suffix=suffix, **kwargs)
    # changes is a list of GerritPatch instances.
    self.changes = changes

  def _GetIrrelevantChangesBoardBase(self, changes):
    """Calculates irrelevant changes to the current board.

    Returns:
      A subset of |changes| which are irrelevant to current board.
    """
    manifest = git.ManifestCheckout.Cached(self._build_root)
    packages = self._GetPackagesUnderTestForCurrentBoard()

    irrelevant_changes = triage_lib.CategorizeChanges.GetIrrelevantChanges(
        changes, self._run.config, self._build_root, manifest, packages)
    return irrelevant_changes

  def _GetPackagesUnderTestForCurrentBoard(self):
    """Get a list of packages used in this build for current board.

    Returns:
      A set of packages used in this build. E.g.,
      set(['chromeos-base/chromite-0.0.1-r1258']); returns None if
      the information is missing for any board in the current config.
    """
    packages_under_test = set()

    for run in [self._run] + self._run.GetChildren():
      board_runattrs = run.GetBoardRunAttrs(self._current_board)
      if not board_runattrs.HasParallel('packages_under_test'):
        logging.warning('Packages under test were not recorded correctly')
        return None
      packages_under_test.update(
          board_runattrs.GetParallel('packages_under_test'))

    return packages_under_test

  def GetSubsystemToTest(self, relevant_changes):
    """Get subsystems from relevant cls for current board, write to BOARD_ATTRS.

    Args:
      relevant_changes: A set of changes that are relevant to current board.

    Returns:
      A set of the subsystems. An empty set indicates that all subsystems should
      be tested.
    """
    # Go through all the relevant changes, collect subsystem info from them. If
    # there exists a change without subsystem info, we assume it affects all
    # subsystems. Then set the superset of all the subsystems to be empty, which
    # means that need to test all subsystems.
    subsystem_set = set()
    for change in relevant_changes:
      sys_lst = triage_lib.GetTestSubsystemForChange(self._build_root, change)
      if sys_lst:
        subsystem_set = subsystem_set.union(sys_lst)
      else:
        subsystem_set = set()
        break

    return subsystem_set

  def _RecordActionForChanges(self, changes, action):
    """Records |changes| action to the slave build into cidb.

    Args:
      builder_run: BuilderRun instance for this build.
      changes: A set of changes to record.
      action: The action for the changes to record (must be one of
        constants.CL_ACTIONS).
    """
    build_id, db = self._run.GetCIDBHandle()
    if db:
      cl_actions = [clactions.CLAction.FromGerritPatchAndAction(
          change, action) for change in changes]
      db.InsertCLActions(build_id, cl_actions)

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    """Run DetectRelevantChangesStage."""
    irrelevant_changes = set()
    relevant_changes = set(self.changes)

    if not self._run.config.master:
      # Slave writes the irrelevant changes to current board to metadata.
      irrelevant_changes = self._GetIrrelevantChangesBoardBase(self.changes)
      relevant_changes = relevant_changes - irrelevant_changes
      change_dict_list = [c.GetAttributeDict() for c in irrelevant_changes]
      change_dict_list = sorted(change_dict_list,
                                key=lambda x: (x[cros_patch.ATTR_GERRIT_NUMBER],
                                               x[cros_patch.ATTR_PATCH_NUMBER],
                                               x[cros_patch.ATTR_REMOTE]))

      self._run.attrs.metadata.UpdateBoardDictWithDict(
          self._current_board, {'irrelevant_changes': change_dict_list})

      if (not self._run.config.do_not_apply_cq_patches and
          config_lib.IsCQType(self._run.config.build_type)):
        # As each CQ build config has only one board, it's safe to record
        # irrelevant changes in this stage. For builds with multiple board
        # configurations, irrelevant changes have to be sorted and reocrded in
        # Completion stage which means each board has detected and recorded its
        # irrelevant changes to metadata in the DetectRelevantChanges stage.

        # Record the irrelevant changes to CIDB.
        self._RecordActionForChanges(
            irrelevant_changes, constants.CL_ACTION_IRRELEVANT_TO_SLAVE)

        if not irrelevant_changes:
          logging.info('No changes are considered irrelevant to this build.')
        else:
          logging.info('The following changes are irrelevant to this build: %s',
                       cros_patch.GetChangesAsString(irrelevant_changes))

        # Record the relevant changes to CIDB.
        self._RecordActionForChanges(
            relevant_changes, constants.CL_ACTION_RELEVANT_TO_SLAVE)

    if relevant_changes:
      logging.info('Below are the relevant changes for board: %s.',
                   self._current_board)
      validation_pool.ValidationPool.PrintLinksToChanges(
          list(relevant_changes))
    else:
      logging.info('No changes are relevant for board: %s.',
                   self._current_board)

    subsystem_set = self.GetSubsystemToTest(relevant_changes)
    logging.info('Subsystems need to be tested: %s. Empty set represents '
                 'testing all subsystems.', subsystem_set)
    # Record subsystems to metadata
    self._run.attrs.metadata.UpdateBoardDictWithDict(
        self._current_board, {'subsystems_to_test': list(subsystem_set)})
