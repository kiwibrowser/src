# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the simple builders."""

from __future__ import print_function

import collections
import traceback

from chromite.cbuildbot import afdo
from chromite.cbuildbot import manifest_version
from chromite.cbuildbot.builders import generic_builders
from chromite.cbuildbot.stages import afdo_stages
from chromite.cbuildbot.stages import android_stages
from chromite.cbuildbot.stages import artifact_stages
from chromite.cbuildbot.stages import build_stages
from chromite.cbuildbot.stages import chrome_stages
from chromite.cbuildbot.stages import completion_stages
from chromite.cbuildbot.stages import generic_stages
from chromite.cbuildbot.stages import handle_changes_stages
from chromite.cbuildbot.stages import release_stages
from chromite.cbuildbot.stages import report_stages
from chromite.cbuildbot.stages import scheduler_stages
from chromite.cbuildbot.stages import sync_stages
from chromite.cbuildbot.stages import tast_test_stages
from chromite.cbuildbot.stages import test_stages
from chromite.cbuildbot.stages import vm_test_stages
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import failures_lib
from chromite.lib import patch as cros_patch
from chromite.lib import parallel
from chromite.lib import results_lib


# TODO: SimpleBuilder needs to be broken up big time.


BoardConfig = collections.namedtuple('BoardConfig', ['board', 'name'])


class SimpleBuilder(generic_builders.Builder):
  """Builder that performs basic vetting operations."""

  def __init__(self, *args, **kwargs):
    super(SimpleBuilder, self).__init__(*args, **kwargs)
    self.sync_stage = None

  def GetSyncInstance(self):
    """Sync to lkgm or TOT as necessary.

    Returns:
      The instance of the sync stage to run.
    """
    if self._run.options.force_version:
      sync_stage = self._GetStageInstance(
          sync_stages.ManifestVersionedSyncStage)
    elif self._run.config.use_lkgm:
      sync_stage = self._GetStageInstance(sync_stages.LKGMSyncStage)
    elif self._run.config.use_chrome_lkgm:
      sync_stage = self._GetStageInstance(chrome_stages.ChromeLKGMSyncStage)
    else:
      sync_stage = self._GetStageInstance(sync_stages.SyncStage)

    self.sync_stage = sync_stage
    return sync_stage

  def GetVersionInfo(self):
    """Returns the CrOS version info from the chromiumos-overlay."""
    return manifest_version.VersionInfo.from_repo(self._run.buildroot)

  def _GetChangesUnderTest(self):
    """Returns the list of GerritPatch changes under test."""
    changes = set()

    changes_json_list = self._run.attrs.metadata.GetDict().get('changes', [])
    for change_dict in changes_json_list:
      change = cros_patch.GerritFetchOnlyPatch.FromAttrDict(change_dict)
      changes.add(change)

    # Also add the changes from PatchChangeStage, the PatchChangeStage doesn't
    # write changes into metadata.
    if self._run.ShouldPatchAfterSync():
      changes.update(set(self.patch_pool.gerrit_patches))

    return list(changes)

  def _RunHWTests(self, builder_run, board):
    """Run hwtest-related stages for the specified board.

    Args:
      builder_run: BuilderRun object for these background stages.
      board: Board name.
    """

    if not builder_run.options.archive:
      logging.warning("HWTests were requested but could not be run because "
                      "artifacts weren't uploaded. Please ensure the archive "
                      "option in the builder config is set to True.")
      return

    # For non-uni builds, we don't pass a model (just board)
    models = [config_lib.ModelTestConfig(None, board)]

    unibuild = False
    if builder_run.config.models:
      models = builder_run.config.models
      unibuild = True

    parallel_stages = []
    for suite_config in builder_run.config.hw_tests:
      # Even for blocking stages, all models can still be run in parallel since
      # it will still block the next stage from executing.
      for model in models:
        new_stage = self._GetHWTestStage(
            builder_run, board, model, suite_config)
        if new_stage:
          parallel_stages.append(new_stage)

      # Please see docstring for blocking in the HWTestConfig for more
      # information on this behavior.
      if suite_config.blocking and not unibuild:
        self._RunParallelStages(parallel_stages)
        parallel_stages = []

    if parallel_stages:
      self._RunParallelStages(parallel_stages)

  def _GetHWTestStage(self, builder_run, board, model, suite_config):
    """Gets the correct hw test stage for a given test suite and model.

    Args:
      builder_run: BuilderRun object for these background stages.
      board: board overlay name
      model: ModelTestConfig object to test against.
      suite_config: HWTestConfig object that defines the test suite.

    Returns:
      The test stage or None if the test suite was filtered for the model.
    """
    result = None

    # If test_suites doesn't exist, then there is no filter.
    # Whereas, an empty array will act as a comprehensive filter.
    if (model.test_suites is None
        or suite_config.suite in model.test_suites):
      stage_class = None
      if suite_config.async:
        stage_class = test_stages.ASyncHWTestStage
      else:
        stage_class = test_stages.HWTestStage

      result = self._GetStageInstance(stage_class,
                                      board,
                                      model.name,
                                      suite_config,
                                      lab_board_name=model.lab_board_name,
                                      builder_run=builder_run)
    return result

  def _RunVMTests(self, builder_run, board):
    """Run VM test stages for the specified board.

    Args:
      builder_run: BuilderRun object for stages.
      board: String containing board name.
    """
    config = builder_run.config
    except_infos = []

    try:
      if config.vm_test_runs > 1:
        # Run the VMTests multiple times to see if they fail.
        self._RunStage(generic_stages.RepeatStage, config.vm_test_runs,
                       vm_test_stages.VMTestStage, board,
                       builder_run=builder_run)
      else:
        # Retry VM-based tests in case failures are flaky.
        self._RunStage(generic_stages.RetryStage, constants.VM_NUM_RETRIES,
                       vm_test_stages.VMTestStage, board,
                       builder_run=builder_run)
    except Exception as e:
      except_infos.extend(
          failures_lib.CreateExceptInfo(e, traceback.format_exc()))

    # Run stages serially to avoid issues encountered when running VMs (or the
    # devserver) in parallel: https://crbug.com/779267
    if config.tast_vm_tests:
      try:
        self._RunStage(generic_stages.RetryStage, constants.VM_NUM_RETRIES,
                       tast_test_stages.TastVMTestStage, board,
                       builder_run=builder_run)
      except Exception as e:
        except_infos.extend(
            failures_lib.CreateExceptInfo(e, traceback.format_exc()))

    if except_infos:
      raise failures_lib.CompoundFailure('VM tests failed', except_infos)

  def _RunDebugSymbolStages(self, builder_run, board):
    """Run debug-related stages for the specified board.

    Args:
      builder_run: BuilderRun object for these background stages.
      board: Board name.
    """
    # These stages should run sequentially.
    self._RunStage(android_stages.DownloadAndroidDebugSymbolsStage,
                   board, builder_run=builder_run)
    self._RunStage(artifact_stages.DebugSymbolsStage, board,
                   builder_run=builder_run)

  def _RunBackgroundStagesForBoardAndMarkAsSuccessful(self, builder_run, board):
    """Run background board-specific stages for the specified board.

    After finishing the build, mark it as successful.

    Args:
      builder_run: BuilderRun object for these background stages.
      board: Board name.
    """
    self._RunBackgroundStagesForBoard(builder_run, board)
    board_runattrs = builder_run.GetBoardRunAttrs(board)
    board_runattrs.SetParallel('success', True)

  def _RunBackgroundStagesForBoard(self, builder_run, board):
    """Run background board-specific stages for the specified board.

    Used by _RunBackgroundStagesForBoardAndMarkAsSuccessful. Callers should use
    that method instead.

    Args:
      builder_run: BuilderRun object for these background stages.
      board: Board name.
    """
    config = builder_run.config

    # TODO(mtennant): This is the last usage of self.archive_stages.  We can
    # kill it once we migrate its uses to BuilderRun so that none of the
    # stages below need it as an argument.
    archive_stage = self.archive_stages[BoardConfig(board, config.name)]
    if config.afdo_generate_min:
      self._RunParallelStages([archive_stage])
      return

    # paygen can't complete without push_image.
    assert not config.paygen or config.push_image

    if config.build_packages_in_background:
      self._RunStage(build_stages.BuildPackagesStage, board,
                     update_metadata=True, builder_run=builder_run,
                     afdo_use=config.afdo_use)

    changes = self._GetChangesUnderTest()
    if changes:
      self._RunStage(report_stages.DetectRelevantChangesStage, board,
                     changes, builder_run=builder_run)

    # While this stage list is run in parallel, the order here dictates the
    # order that things will be shown in the log.  So group things together
    # that make sense when read in order.  Also keep in mind that, since we
    # gather output manually, early slow stages will prevent any output from
    # later stages showing up until it finishes.
    changes = self._GetChangesUnderTest()
    stage_list = [
        [test_stages.UnitTestStage, board],
        [test_stages.DebugInfoTestStage, board],
    ]

    # Skip most steps if we're a compilecheck builder.
    if builder_run.config.compilecheck or builder_run.options.compilecheck:
      for x in stage_list:
        self._RunStage(*x, builder_run=builder_run)
      return

    stage_list += [[chrome_stages.SimpleChromeArtifactsStage, board]]

    if config.gce_tests:
      stage_list += [[generic_stages.RetryStage, constants.VM_NUM_RETRIES,
                      vm_test_stages.GCETestStage, board]]

    if config.moblab_vm_tests:
      stage_list += [[vm_test_stages.MoblabVMTestStage, board]]

    if config.afdo_generate:
      stage_list += [[afdo_stages.AFDODataGenerateStage, board]]

    stage_list += [
        [release_stages.SignerTestStage, board, archive_stage],
        [release_stages.SigningStage, board],
        [release_stages.PaygenStage, board],
        [test_stages.ImageTestStage, board],
        [artifact_stages.UploadPrebuiltsStage, board],
        [artifact_stages.DevInstallerPrebuiltsStage, board],
        [artifact_stages.CPEExportStage, board],
        [artifact_stages.UploadTestArtifactsStage, board],
    ]

    stage_objs = [self._GetStageInstance(*x, builder_run=builder_run)
                  for x in stage_list]

    # Build the image first before running the steps.
    with self._build_image_lock:
      self._RunStage(build_stages.BuildImageStage, board,
                     builder_run=builder_run, afdo_use=config.afdo_use)

    parallel.RunParallelSteps([
        lambda: self._RunParallelStages(stage_objs + [archive_stage]),
        lambda: self._RunHWTests(builder_run, board),
        lambda: self._RunVMTests(builder_run, board),
        lambda: self._RunDebugSymbolStages(builder_run, board),
    ])

  def RunSetupBoard(self):
    """Run the SetupBoard stage for all child configs and boards."""
    for builder_run in self._run.GetUngroupedBuilderRuns():
      for board in builder_run.config.boards:
        self._RunStage(build_stages.SetupBoardStage, board,
                       builder_run=builder_run)

  def _RunMasterPaladinOrPFQBuild(self):
    """Runs through the stages of the paladin or chrome PFQ master build."""
    # If this master build uses Buildbucket scheduler, run
    # scheduler_stages.ScheduleSlavesStage to schedule slaves.
    if config_lib.UseBuildbucketScheduler(self._run.config):
      self._RunStage(scheduler_stages.ScheduleSlavesStage, self.sync_stage)
    self._RunStage(build_stages.UprevStage)
    self._RunStage(build_stages.InitSDKStage)
    # The CQ/Chrome PFQ master will not actually run the SyncChrome stage, but
    # we want the logic that gets triggered when SyncChrome stage is skipped.
    self._RunStage(chrome_stages.SyncChromeStage)
    self._RunStage(android_stages.UprevAndroidStage)
    self._RunStage(android_stages.AndroidMetadataStage)
    if self._run.config.build_type == constants.PALADIN_TYPE:
      self._RunStage(build_stages.RegenPortageCacheStage)
    self._RunStage(test_stages.BinhostTestStage)
    self._RunStage(test_stages.BranchUtilTestStage)

  def RunEarlySyncAndSetupStages(self):
    """Runs through the early sync and board setup stages."""
    # If this build is master and uses Buildbucket scheduler, run
    # scheduler_stages.ScheduleSlavesStage to schedule slaves.
    if (config_lib.UseBuildbucketScheduler(self._run.config) and
        config_lib.IsMasterBuild(self._run.config)):
      self._RunStage(scheduler_stages.ScheduleSlavesStage, self.sync_stage)
    self._RunStage(build_stages.UprevStage)
    self._RunStage(build_stages.InitSDKStage)
    self._RunStage(build_stages.RegenPortageCacheStage)
    self.RunSetupBoard()
    self._RunStage(chrome_stages.SyncChromeStage)
    self._RunStage(android_stages.UprevAndroidStage)
    self._RunStage(android_stages.AndroidMetadataStage)

  def RunBuildTestStages(self):
    """Runs through the stages to test before building."""
    self._RunStage(test_stages.BinhostTestStage)
    self._RunStage(test_stages.BranchUtilTestStage)

  def RunBuildStages(self):
    """Runs through the stages to perform the build and resulting tests."""
    # Prepare stages to run in background.  If child_configs exist then
    # run each of those here, otherwise use default config.
    builder_runs = self._run.GetUngroupedBuilderRuns()

    tasks = []
    for builder_run in builder_runs:
      # Prepare a local archive directory for each "run".
      builder_run.GetArchive().SetupArchivePath()

      for board in builder_run.config.boards:
        archive_stage = self._GetStageInstance(
            artifact_stages.ArchiveStage, board, builder_run=builder_run,
            chrome_version=self._run.attrs.chrome_version)
        board_config = BoardConfig(board, builder_run.config.name)
        self.archive_stages[board_config] = archive_stage
        tasks.append((builder_run, board))

    # Set up a process pool to run test/archive stages in the background.
    # This process runs task(board) for each board added to the queue.
    task_runner = self._RunBackgroundStagesForBoardAndMarkAsSuccessful
    with parallel.BackgroundTaskRunner(task_runner) as queue:
      for builder_run, board in tasks:
        if not builder_run.config.build_packages_in_background:
          # Run BuildPackages in the foreground, generating or using AFDO data
          # if requested.
          kwargs = {'builder_run': builder_run}
          if builder_run.config.afdo_generate_min:
            kwargs['afdo_generate_min'] = True
          elif builder_run.config.afdo_use:
            kwargs['afdo_use'] = True

          self._RunStage(build_stages.BuildPackagesStage, board,
                         update_metadata=True, **kwargs)

          if (builder_run.config.afdo_generate_min and
              afdo.CanGenerateAFDOData(board)):
            # Generate the AFDO data before allowing any other tasks to run.
            self._RunStage(build_stages.BuildImageStage, board, **kwargs)
            self._RunStage(artifact_stages.UploadTestArtifactsStage, board,
                           builder_run=builder_run,
                           suffix='[afdo_generate_min]')
            for suite in builder_run.config.hw_tests:
              self._RunStage(test_stages.HWTestStage, board, suite,
                             builder_run=builder_run)
            self._RunStage(afdo_stages.AFDODataGenerateStage, board,
                           builder_run=builder_run)

          if (builder_run.config.afdo_generate_min and
              builder_run.config.afdo_update_ebuild):
            self._RunStage(afdo_stages.AFDOUpdateChromeEbuildStage,
                           builder_run=builder_run)
            self._RunStage(afdo_stages.AFDOUpdateKernelEbuildStage,
                           builder_run=builder_run)

        # Kick off our background stages.
        queue.put([builder_run, board])

  def _RunDefaultTypeBuild(self):
    """Runs through the stages of a non-special-type build."""
    self.RunEarlySyncAndSetupStages()
    self.RunBuildTestStages()
    self.RunBuildStages()

  def RunStages(self):
    """Runs through build process."""
    # TODO(sosa): Split these out into classes.
    if self._run.config.build_type == constants.PRE_CQ_LAUNCHER_TYPE:
      self._RunStage(sync_stages.PreCQLauncherStage)
    elif ((self._run.config.build_type == constants.PALADIN_TYPE or
           self._run.config.build_type == constants.CHROME_PFQ_TYPE or
           self._run.config.build_type == constants.ANDROID_PFQ_TYPE) and
          self._run.config.master):
      self._RunMasterPaladinOrPFQBuild()
    else:
      self._RunDefaultTypeBuild()


class DistributedBuilder(SimpleBuilder):
  """Build class that has special logic to handle distributed builds.

  These builds sync using git/manifest logic in manifest_versions.  In general
  they use a non-distributed builder code for the bulk of the work.
  """

  def __init__(self, *args, **kwargs):
    """Initializes a buildbot builder.

    Extra variables:
      completion_stage_class:  Stage used to complete a build.  Set in the Sync
        stage.
    """
    super(DistributedBuilder, self).__init__(*args, **kwargs)
    self.completion_stage_class = None
    self.sync_stage = None
    self._completion_stage = None

  def GetSyncInstance(self):
    """Syncs the tree using one of the distributed sync logic paths.

    Returns:
      The instance of the sync stage to run.
    """
    # Determine sync class to use.  CQ overrides PFQ bits so should check it
    # first.
    if self._run.config.pre_cq:
      sync_stage = self._GetStageInstance(sync_stages.PreCQSyncStage,
                                          self.patch_pool.gerrit_patches)
      self.completion_stage_class = completion_stages.PreCQCompletionStage
      self.patch_pool.gerrit_patches = []
    elif config_lib.IsCQType(self._run.config.build_type):
      if self._run.config.do_not_apply_cq_patches:
        sync_stage = self._GetStageInstance(
            sync_stages.MasterSlaveLKGMSyncStage)
      else:
        sync_stage = self._GetStageInstance(sync_stages.CommitQueueSyncStage)
      self.completion_stage_class = completion_stages.CommitQueueCompletionStage
    elif config_lib.IsPFQType(self._run.config.build_type):
      sync_stage = self._GetStageInstance(sync_stages.MasterSlaveLKGMSyncStage)
      self.completion_stage_class = (
          completion_stages.MasterSlaveSyncCompletionStage)
    elif config_lib.IsCanaryType(self._run.config.build_type):
      sync_stage = self._GetStageInstance(
          sync_stages.ManifestVersionedSyncStage)
      self.completion_stage_class = (
          completion_stages.CanaryCompletionStage)
    elif self._run.config.build_type == constants.TOOLCHAIN_TYPE:
      sync_stage = self._GetStageInstance(sync_stages.MasterSlaveLKGMSyncStage)
      self.completion_stage_class = (
          completion_stages.MasterSlaveSyncCompletionStage)
    else:
      sync_stage = self._GetStageInstance(
          sync_stages.ManifestVersionedSyncStage)
      self.completion_stage_class = (
          completion_stages.ManifestVersionedSyncCompletionStage)

    self.sync_stage = sync_stage
    return self.sync_stage

  def GetCompletionInstance(self):
    """Returns the completion_stage_class instance that was used for this build.

    Returns:
      None if the completion_stage instance was not yet created (this
      occurs during Publish).
    """
    return self._completion_stage

  def Complete(self, was_build_successful, build_finished):
    """Completes build by publishing any required information.

    Args:
      was_build_successful: Whether the build succeeded.
      build_finished: Whether the build completed. A build can be successful
        without completing if it raises ExitEarlyException.
    """
    self._completion_stage = self._GetStageInstance(
        self.completion_stage_class, self.sync_stage, was_build_successful)
    completion_successful = False
    try:
      self._completion_stage.Run()
      self._HandleChanges()
      completion_successful = True
    except failures_lib.StepFailure as e:
      if isinstance(e, completion_stages.ImportantBuilderFailedException):
        # When ImportantBuilderFailedException is the only exception, the master
        # build can still submit partial changes (CLs).
        self._HandleChanges()

      raise
    finally:
      self._Publish(was_build_successful, build_finished, completion_successful)

  def _HandleChanges(self):
    """Handle changes picked up by the validation_pool in the sync stage."""
    if config_lib.IsMasterCQ(self._run.config):
      self._RunStage(handle_changes_stages.CommitQueueHandleChangesStage,
                     self.sync_stage, self._completion_stage)

  def _Publish(self, was_build_successful, build_finished,
               completion_successful):
    """Updates and publishes uprevs.

    Args:
      was_build_successful: Whether the build succeeded.
      build_finished: Whether the build completed. A build can be successful
        without completing if it raises ExitEarlyException.
      completion_successful: Whether the compeletion_stage succeeded.
    """
    is_master_chrome_pfq = config_lib.IsMasterChromePFQ(self._run.config)

    updateEbuild_successful = False
    try:
      # When (afdo_update_ebuild and not afdo_generate_min) is True,
      # if completion_stage passed, need to run
      # AFDOUpdateChromeEbuildStage to prepare for pushing commits to masters;
      # if it's a master_chrome_pfq build and compeletion_stage failed,
      # need to run AFDOUpdateChromeEbuildStage to prepare for pushing commits
      # to a staging branch.
      if ((completion_successful or is_master_chrome_pfq) and
          self._run.config.afdo_update_ebuild and
          not self._run.config.afdo_generate_min):
        self._RunStage(afdo_stages.AFDOUpdateChromeEbuildStage)
        self._RunStage(afdo_stages.AFDOUpdateKernelEbuildStage)
        updateEbuild_successful = True
    finally:
      if self._run.config.master:
        self._RunStage(report_stages.SlaveFailureSummaryStage)
      if self._run.config.push_overlays:
        publish = (was_build_successful and completion_successful and
                   build_finished)
        if is_master_chrome_pfq:
          if publish:
            self._RunStage(completion_stages.UpdateChromeosLKGMStage)
          else:
            logging.info('Skipping UpdateChromeosLKGMStage, '
                         'build_successful=%d completion_successful=%d '
                         'build_finished=%d', was_build_successful,
                         completion_successful, build_finished)
        # If this build is master chrome pfq, completion_stage failed,
        # AFDOUpdateChromeEbuildStage passed, and the necessary build stages
        # passed, it means publish is False and we need to stage the
        # push to another branch instead of master.
        stage_push = (is_master_chrome_pfq and
                      not completion_successful and
                      updateEbuild_successful and
                      was_build_successful and
                      build_finished)
        self._RunStage(completion_stages.PublishUprevChangesStage,
                       self.sync_stage, publish, stage_push)

  def RunStages(self):
    """Runs simple builder logic and publishes information to overlays."""
    was_build_successful = False
    build_finished = False
    try:
      super(DistributedBuilder, self).RunStages()
      build_id, db = self._run.GetCIDBHandle()
      was_build_successful = results_lib.Results.BuildSucceededSoFar(
          db, build_id)
      build_finished = True
    except failures_lib.ExitEarlyException as ex:
      # If a stage throws ExitEarlyException, it's exiting with success,
      # so that means we should mark ourselves as successful.
      logging.info('Detected exception %s', ex)
      was_build_successful = True
      raise
    finally:
      self.Complete(was_build_successful, build_finished)
