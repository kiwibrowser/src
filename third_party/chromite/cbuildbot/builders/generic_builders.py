# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the generic builders."""

from __future__ import print_function

import multiprocessing
import os
import sys
import tempfile
import traceback

from chromite.lib import constants
from chromite.lib import failures_lib
from chromite.cbuildbot import manifest_version
from chromite.lib import results_lib
from chromite.cbuildbot import trybot_patch_pool
from chromite.cbuildbot.stages import build_stages
from chromite.cbuildbot.stages import completion_stages
from chromite.cbuildbot.stages import report_stages
from chromite.cbuildbot.stages import sync_stages
from chromite.lib import cidb
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import parallel


class Builder(object):
  """Parent class for all builder types.

  This class functions as an abstract parent class for various build types.
  Its intended use is builder_instance.Run().

  Attributes:
    _run: The BuilderRun object for this run.
    archive_stages: Dict of BuildConfig keys to ArchiveStage values.
    patch_pool: TrybotPatchPool.
  """

  def __init__(self, builder_run):
    """Initializes instance variables. Must be called by all subclasses."""
    self._run = builder_run

    # TODO: all the fields below should not be part of the generic builder.
    # We need to restructure our SimpleBuilder and see about creating a new
    # base in there for holding them.
    if self._run.config.chromeos_official:
      os.environ['CHROMEOS_OFFICIAL'] = '1'

    self.archive_stages = {}
    self.patch_pool = trybot_patch_pool.TrybotPatchPool()
    self._build_image_lock = multiprocessing.Lock()

  def Initialize(self):
    """Runs through the initialization steps of an actual build."""
    if self._run.options.resume:
      results_lib.LoadCheckpoint(self._run.buildroot)

    self._RunStage(report_stages.BuildStartStage)

    self._RunStage(build_stages.CleanUpStage)

  def _GetStageInstance(self, stage, *args, **kwargs):
    """Helper function to get a stage instance given the args.

    Useful as almost all stages just take in builder_run.
    """
    # Normally the default BuilderRun (self._run) is used, but it can
    # be overridden with "builder_run" kwargs (e.g. for child configs).
    builder_run = kwargs.pop('builder_run', self._run)
    return stage(builder_run, *args, **kwargs)

  def _SetReleaseTag(self):
    """Sets run.attrs.release_tag from the manifest manager used in sync.

    Must be run after sync stage as syncing enables us to have a release tag,
    and must be run before any usage of attrs.release_tag.

    TODO(mtennant): Find a bottleneck place in syncing that can set this
    directly.  Be careful, as there are several kinds of syncing stages, and
    sync stages have been known to abort with sys.exit calls.
    """
    manifest_manager = getattr(self._run.attrs, 'manifest_manager', None)
    if manifest_manager:
      self._run.attrs.release_tag = manifest_manager.current_version
    else:
      self._run.attrs.release_tag = None

    logging.debug('Saved release_tag value for run: %r',
                  self._run.attrs.release_tag)

  def _RunStage(self, stage, *args, **kwargs):
    """Wrapper to run a stage.

    Args:
      stage: A BuilderStage class.
      args: args to pass to stage constructor.
      kwargs: kwargs to pass to stage constructor.

    Returns:
      Whatever the stage's Run method returns.
    """
    stage_instance = self._GetStageInstance(stage, *args, **kwargs)
    return stage_instance.Run()

  @staticmethod
  def _RunParallelStages(stage_objs):
    """Run the specified stages in parallel.

    Args:
      stage_objs: BuilderStage objects.
    """
    steps = [stage.Run for stage in stage_objs]
    try:
      parallel.RunParallelSteps(steps)
    except BaseException as ex:
      logging.error('BaseException in _RunParallelStages %s' % ex,
                    exc_info=True)
      # If a stage threw an exception, it might not have correctly reported
      # results (e.g. because it was killed before it could report the
      # results.) In this case, attribute the exception to any stages that
      # didn't report back correctly (if any).
      for stage in stage_objs:
        for name in stage.GetStageNames():
          if not results_lib.Results.StageHasResults(name):
            results_lib.Results.Record(
                name, ex, str(ex), prefix=stage.StageNamePrefix())

        if cidb.CIDBConnectionFactory.IsCIDBSetup():
          db = cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder()
          for build_stage_id in stage.GetBuildStageIDs():
            stage_status = db.GetBuildStage(build_stage_id)

            # If no failures for this stage found in failureTable, and the stage
            # has no status or has non-failure status in buildStageTable,
            # report failures to failureTable for this stage.
            if (not db.HasFailureMsgForStage(build_stage_id) and
                (stage_status is None or stage_status['status']
                 not in constants.BUILDER_NON_FAILURE_STATUSES)):
              failures_lib.ReportStageFailure(
                  db, build_stage_id, ex, build_config=stage.build_config)

            # If this stage has non_completed status in buildStageTable, mark
            # the stage as 'fail' status in buildStageTable.
            if (stage_status is not None and stage_status['status'] not in
                constants.BUILDER_COMPLETED_STATUSES):
              db.FinishBuildStage(build_stage_id,
                                  constants.BUILDER_STATUS_FAILED)

      raise

  def _RunSyncStage(self, sync_instance):
    """Run given |sync_instance| stage and be sure attrs.release_tag set."""
    try:
      sync_instance.Run()
    finally:
      self._SetReleaseTag()

  def SetVersionInfo(self):
    """Sync the builder's version info with the buildbot runtime."""
    self._run.attrs.version_info = self.GetVersionInfo()

  def GetVersionInfo(self):
    """Returns a manifest_version.VersionInfo object for this build.

    Chrome OS Subclasses must override this method. Site specific builds which
    don't use Chrome OS versioning should leave this alone.
    """
    # Placeholder version for non-Chrome OS builds.
    return manifest_version.VersionInfo('1.0.0')

  def GetSyncInstance(self):
    """Returns an instance of a SyncStage that should be run.

    Subclasses must override this method.
    """
    raise NotImplementedError()

  def GetCompletionInstance(self):
    """Returns the MasterSlaveSyncCompletionStage for this build.

    Subclasses may override this method.

    Returns:
      None
    """
    return None

  def RunStages(self):
    """Subclasses must override this method.  Runs the appropriate code."""
    raise NotImplementedError()

  def _ReExecuteInBuildroot(self, sync_instance):
    """Reexecutes self in buildroot and returns True if build succeeds.

    This allows the buildbot code to test itself when changes are patched for
    buildbot-related code.  This is a no-op if the buildroot == buildroot
    of the running chromite checkout.

    Args:
      sync_instance: Instance of the sync stage that was run to sync.

    Returns:
      True if the Build succeeded.
    """
    if not self._run.options.resume:
      results_lib.WriteCheckpoint(self._run.options.buildroot)

    args = sync_stages.BootstrapStage.FilterArgsForTargetCbuildbot(
        self._run.options.buildroot, constants.PATH_TO_CBUILDBOT,
        self._run.options)

    # Specify a buildroot explicitly (just in case, for local trybot).
    # Suppress any timeout options given from the commandline in the
    # invoked cbuildbot; our timeout will enforce it instead.
    args += ['--resume', '--timeout', '0', '--notee', '--nocgroups',
             '--buildroot', os.path.abspath(self._run.options.buildroot)]

    # Set --version. Note that --version isn't legal without --buildbot.
    if (self._run.options.buildbot and
        hasattr(self._run.attrs, 'manifest_manager')):
      ver = self._run.attrs.manifest_manager.current_version
      args += ['--version', ver]

    pool = getattr(sync_instance, 'pool', None)
    if pool:
      filename = os.path.join(self._run.options.buildroot,
                              'validation_pool.dump')
      pool.Save(filename)
      args += ['--validation_pool', filename]

    # Reset the cache dir so that the child will calculate it automatically.
    if not self._run.options.cache_dir_specified:
      commandline.BaseParser.ConfigureCacheDir(None)

    with tempfile.NamedTemporaryFile(prefix='metadata') as metadata_file:
      metadata_file.write(self._run.attrs.metadata.GetJSON())
      metadata_file.flush()
      args += ['--metadata_dump', metadata_file.name]

      # Re-run the command in the buildroot.
      # Finally, be generous and give the invoked cbuildbot 30s to shutdown
      # when something occurs.  It should exit quicker, but the sigterm may
      # hit while the system is particularly busy.
      return_obj = cros_build_lib.RunCommand(
          args, cwd=self._run.options.buildroot, error_code_ok=True,
          kill_timeout=30)
      return return_obj.returncode == 0

  def _InitializeTrybotPatchPool(self):
    """Generate patch pool from patches specified on the command line.

    Do this only if we need to patch changes later on.
    """
    changes_stage = sync_stages.PatchChangesStage.StageNamePrefix()
    check_func = results_lib.Results.PreviouslyCompletedRecord
    if not check_func(changes_stage) or self._run.options.bootstrap:
      options = self._run.options
      self.patch_pool = trybot_patch_pool.TrybotPatchPool.FromOptions(
          gerrit_patches=options.gerrit_patches,
          sourceroot=options.sourceroot,
          remote_patches=options.remote_patches)

  def _GetBootstrapStage(self):
    """Constructs and returns the BootStrapStage object.

    We return None when there are no chromite patches to test, and
    --test-bootstrap wasn't passed in.
    """
    stage = None

    patches_needed = sync_stages.BootstrapStage.BootstrapPatchesNeeded(
        self._run, self.patch_pool)

    chromite_branch = git.GetChromiteTrackingBranch()

    if (patches_needed or
        self._run.options.test_bootstrap or
        chromite_branch != self._run.options.branch):
      stage = sync_stages.BootstrapStage(self._run, self.patch_pool)
    return stage

  def Run(self):
    """Main runner for this builder class.  Runs build and prints summary.

    Returns:
      Whether the build succeeded.
    """
    self._InitializeTrybotPatchPool()

    if self._run.options.bootstrap:
      bootstrap_stage = self._GetBootstrapStage()
      if bootstrap_stage:
        # BootstrapStage blocks on re-execution of cbuildbot.
        bootstrap_stage.Run()
        return bootstrap_stage.returncode == 0

    print_report = True
    exception_thrown = False
    success = True
    sync_instance = None
    try:
      self.Initialize()
      sync_instance = self.GetSyncInstance()
      self._RunSyncStage(sync_instance)

      if self._run.ShouldPatchAfterSync():
        # Filter out patches to manifest, since PatchChangesStage can't handle
        # them.  Manifest patches are patched in the BootstrapStage.
        non_manifest_patches = self.patch_pool.FilterManifest(negate=True)
        if non_manifest_patches:
          self._RunStage(sync_stages.PatchChangesStage, non_manifest_patches)

      # Now that we have a fully synced & patched tree, we can let the builder
      # extract version information from the sources for this particular build.
      self.SetVersionInfo()
      if self._run.ShouldReexecAfterSync():
        print_report = False
        success = self._ReExecuteInBuildroot(sync_instance)
      else:
        self._RunStage(report_stages.BuildReexecutionFinishedStage)
        self._RunStage(report_stages.ConfigDumpStage)
        self.RunStages()

    except Exception as ex:
      if isinstance(ex, failures_lib.ExitEarlyException):
        # One stage finished and exited early, not a failure.
        raise

      exception_thrown = True
      build_id, db = self._run.GetCIDBHandle()
      if results_lib.Results.BuildSucceededSoFar(db, build_id):
        # If the build is marked as successful, but threw exceptions, that's a
        # problem. Print the traceback for debugging.
        if isinstance(ex, failures_lib.CompoundFailure):
          print(str(ex))

        traceback.print_exc(file=sys.stdout)
        raise

      if not (print_report and isinstance(ex, failures_lib.StepFailure)):
        # If the failed build threw a non-StepFailure exception, we
        # should raise it.
        raise

    finally:
      if print_report:
        results_lib.WriteCheckpoint(self._run.options.buildroot)
        completion_instance = self.GetCompletionInstance()
        self._RunStage(report_stages.ReportStage, completion_instance)
        build_id, db = self._run.GetCIDBHandle()
        success = results_lib.Results.BuildSucceededSoFar(db, build_id)
        if exception_thrown and success:
          success = False
          logging.PrintBuildbotStepWarnings()
          print("""\
Exception thrown, but all stages marked successful. This is an internal error,
because the stage that threw the exception should be marked as failing.""")

    return success


class PreCqBuilder(Builder):
  """Builder that runs PreCQ tests.

  PreCq builders that need to run custom stages (build or test) should derive
  from this class. Traditional builders whose behavior is driven by
  config_lib.CONFIG_TYPE_PRECQ should derive from SimpleBuilder. The preference
  for PreCQ builders is to use this.

  Note: Override RunTestStages, NOT RunStages like a normal Builder.
  """

  def __init__(self, *args, **kwargs):
    """Initializes a buildbot builder."""
    super(PreCqBuilder, self).__init__(*args, **kwargs)
    self.sync_stage = None
    self.completion_instance = None

  def GetSyncInstance(self):
    """Returns an instance of a SyncStage that should be run."""
    self.sync_stage = self._GetStageInstance(sync_stages.PreCQSyncStage,
                                             self.patch_pool.gerrit_patches)
    self.patch_pool.gerrit_patches = []

    return self.sync_stage

  def GetCompletionInstance(self):
    """Return the completion instance.

    Returns:
      generic_stages.BuilderStage subclass, or None if completion hasn't run.
    """
    return self.completion_instance

  def RunStages(self):
    """Run something after sync/reexec."""
    try:
      self.RunTestStages()
    finally:
      build_id, db = self._run.GetCIDBHandle()
      was_build_successful = results_lib.Results.BuildSucceededSoFar(
          db, build_id)

      self.completion_instance = self._GetStageInstance(
          completion_stages.PreCQCompletionStage,
          self.sync_stage,
          was_build_successful)

      self.completion_instance.Run()

  def RunTestStages(self):
    """Subclasses must override this method. Runs the build/test stages."""
    raise NotImplementedError()
