# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Unittests for simpler builders."""

from __future__ import print_function

import copy
import mock
import os

from chromite.cbuildbot import cbuildbot_run
from chromite.cbuildbot.builders import generic_builders
from chromite.cbuildbot.builders import simple_builders
from chromite.cbuildbot.stages import completion_stages
from chromite.cbuildbot.stages import generic_stages
from chromite.cbuildbot.stages import handle_changes_stages
from chromite.cbuildbot.stages import tast_test_stages
from chromite.cbuildbot.stages import vm_test_stages
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import failures_lib
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.scripts import cbuildbot


# pylint: disable=protected-access


class SimpleBuilderTest(cros_test_lib.MockTempDirTestCase):
  """Tests for the main code paths in simple_builders.SimpleBuilder"""

  def setUp(self):
    # List of all stages that would have been called as part of this run.
    self.called_stages = []

    # Map from stage class to exception to be raised when stage is run.
    self.stage_exceptions = {}

    # VM test stages that are run by SimpleBuilder._RunVMTests.
    self.all_vm_test_stages = [vm_test_stages.VMTestStage,
                               tast_test_stages.TastVMTestStage]

    # Simple new function that redirects RunStage to record all stages to be
    # run rather than mock them completely. These can be used in a test to
    # assert something has been called.
    def run_stage(_class_instance, stage_name, *args, **_kwargs):
      # It's more useful to record the actual stage that's wrapped within
      # RepeatStage or RetryStage.
      if stage_name in [generic_stages.RepeatStage, generic_stages.RetryStage]:
        stage_name = args[1]

      self.called_stages.append(stage_name)
      if stage_name in self.stage_exceptions:
        raise self.stage_exceptions[stage_name]

    # Parallel version.
    def run_parallel_stages(_class_instance, *_args):
      # Since parallel stages are forked processes, we can't actually
      # update anything here unless we want to do interprocesses comms.
      pass

    self.buildroot = os.path.join(self.tempdir, 'buildroot')
    chroot_path = os.path.join(self.buildroot, constants.DEFAULT_CHROOT_DIR)
    osutils.SafeMakedirs(os.path.join(chroot_path, 'tmp'))

    self.PatchObject(generic_builders.Builder, '_RunStage',
                     new=run_stage)
    self.PatchObject(simple_builders.SimpleBuilder, '_RunParallelStages',
                     new=run_parallel_stages)
    self.PatchObject(cbuildbot_run._BuilderRunBase, 'GetVersion',
                     return_value='R32-1234.0.0')

    self._manager = parallel.Manager()
    self._manager.__enter__()

  def tearDown(self):
    # Mimic exiting a 'with' statement.
    self._manager.__exit__(None, None, None)

  def _initConfig(
      self, bot_id, master=False, extra_argv=None, override_hw_test_config=None,
      models=None):
    """Return normal options/build_config for |bot_id|"""
    site_config = config_lib.GetConfig()
    build_config = copy.deepcopy(site_config[bot_id])
    build_config['master'] = master
    build_config['important'] = False
    if models:
      build_config['models'] = models

    # Use the cbuildbot parser to create properties and populate default values.
    parser = cbuildbot._CreateParser()
    argv = (['-r', self.buildroot, '--buildbot', '--debug', '--nochromesdk'] +
            (extra_argv if extra_argv else []) + [bot_id])
    options = cbuildbot.ParseCommandLine(parser, argv)

    # Yikes.
    options.managed_chrome = build_config['sync_chrome']

    # Iterate through override and update HWTestConfig attributes.
    if override_hw_test_config:
      for key, val in override_hw_test_config.iteritems():
        for hw_test in build_config.hw_tests:
          setattr(hw_test, key, val)

    return cbuildbot_run.BuilderRun(
        options, site_config, build_config, self._manager)

  def _RunVMTests(self):
    """Helper method that runs VM tests and returns exceptions.

    Returns:
      List of exception classes in CompoundFailure.
    """
    board = 'betty-release'
    builder_run = self._initConfig(board)
    exception_types = []

    try:
      simple_builders.SimpleBuilder(builder_run)._RunVMTests(builder_run, board)
    except failures_lib.CompoundFailure as f:
      exception_types = [e.type for e in f.exc_infos]
    return exception_types

  def testRunStagesPreCQ(self):
    """Verify RunStages for PRE_CQ_LAUNCHER_TYPE builders"""
    builder_run = self._initConfig('pre-cq-launcher')
    simple_builders.SimpleBuilder(builder_run).RunStages()

  def testRunStagesBranchUtil(self):
    """Verify RunStages for CREATE_BRANCH_TYPE builders"""
    extra_argv = ['--branch-name', 'foo', '--version', '1234']
    builder_run = self._initConfig(constants.BRANCH_UTIL_CONFIG,
                                   extra_argv=extra_argv)
    simple_builders.SimpleBuilder(builder_run).RunStages()

  def testRunStagesChrootBuilder(self):
    """Verify RunStages for CHROOT_BUILDER_TYPE builders"""
    builder_run = self._initConfig('chromiumos-sdk')
    simple_builders.SimpleBuilder(builder_run).RunStages()

  def testRunStagesDefaultBuild(self):
    """Verify RunStages for standard board builders"""
    builder_run = self._initConfig('amd64-generic-full')
    builder_run.attrs.chrome_version = 'TheChromeVersion'
    simple_builders.SimpleBuilder(builder_run).RunStages()

  def testRunStagesDefaultBuildCompileCheck(self):
    """Verify RunStages for standard board builders (compile only)"""
    extra_argv = ['--compilecheck']
    builder_run = self._initConfig('amd64-generic-full', extra_argv=extra_argv)
    builder_run.attrs.chrome_version = 'TheChromeVersion'
    simple_builders.SimpleBuilder(builder_run).RunStages()

  def testRunStagesDefaultBuildHwTests(self):
    """Verify RunStages for boards w/hwtests"""
    extra_argv = ['--hwtest']
    builder_run = self._initConfig('eve-release', extra_argv=extra_argv)
    builder_run.attrs.chrome_version = 'TheChromeVersion'
    simple_builders.SimpleBuilder(builder_run).RunStages()

  def testThatWeScheduleHWTestsRegardlessOfBlocking(self):
    """Verify RunStages for boards w/hwtests (blocking).

    Make sure the same stages get scheduled regardless of whether their hwtest
    suites are marked blocking or not.
    """
    extra_argv = ['--hwtest']
    builder_run_without_blocking = self._initConfig(
        'eve-release', extra_argv=extra_argv,
        override_hw_test_config=dict(blocking=False))
    builder_run_with_blocking = self._initConfig(
        'eve-release', extra_argv=extra_argv,
        override_hw_test_config=dict(blocking=True))
    builder_run_without_blocking.attrs.chrome_version = 'TheChromeVersion'
    builder_run_with_blocking.attrs.chrome_version = 'TheChromeVersion'

    simple_builders.SimpleBuilder(builder_run_without_blocking).RunStages()
    without_blocking_stages = list(self.called_stages)

    self.called_stages = []
    simple_builders.SimpleBuilder(builder_run_with_blocking).RunStages()
    self.assertEqual(without_blocking_stages, self.called_stages)

  def testUnifiedBuildsRunHwTestsForAllModels(self):
    """Verify hwtests run for model fanout with unified builds"""
    extra_argv = ['--hwtest']
    unified_build = self._initConfig(
        'eve-release',
        extra_argv=extra_argv,
        models=[config_lib.ModelTestConfig('model1', 'model1'),
                config_lib.ModelTestConfig(
                    'model2', 'model2', ['sanity', 'bvt-inline'])])
    unified_build.attrs.chrome_version = 'TheChromeVersion'
    simple_builders.SimpleBuilder(unified_build).RunStages()

  def testGetHWTestStageWithPerModelFilters(self):
    """Verify hwtests are filtered correctly on a per-model basis"""
    extra_argv = ['--hwtest']
    unified_build = self._initConfig(
        'eve-release',
        extra_argv=extra_argv)
    unified_build.attrs.chrome_version = 'TheChromeVersion'

    test_phase1 = unified_build.config.hw_tests[0]
    test_phase2 = unified_build.config.hw_tests[1]

    model1 = config_lib.ModelTestConfig('model1', 'some_lab_board')
    model2 = config_lib.ModelTestConfig('model2', 'mode11', [test_phase2.suite])

    hw_stage = simple_builders.SimpleBuilder(unified_build)._GetHWTestStage(
        unified_build,
        'eve',
        model1,
        test_phase1)
    self.assertIsNotNone(hw_stage)
    self.assertEqual(hw_stage._board_name, 'some_lab_board')

    hw_stage = simple_builders.SimpleBuilder(unified_build)._GetHWTestStage(
        unified_build,
        'eve',
        model2,
        test_phase1)
    self.assertIsNone(hw_stage)

    hw_stage = simple_builders.SimpleBuilder(unified_build)._GetHWTestStage(
        unified_build,
        'eve',
        model2,
        test_phase2)
    self.assertIsNotNone(hw_stage)

  def testAllVMTestStagesSucceed(self):
    """Verify all VM test stages are run."""
    self.assertEquals([], self._RunVMTests())
    self.assertEquals(self.all_vm_test_stages, self.called_stages)

  def testAllVMTestStagesFail(self):
    """Verify failures are reported when all VM test stages fail."""
    self.stage_exceptions = {
        vm_test_stages.VMTestStage: failures_lib.InfrastructureFailure(),
        tast_test_stages.TastVMTestStage: failures_lib.TestFailure(),
    }
    self.assertEquals(
        [failures_lib.InfrastructureFailure, failures_lib.TestFailure],
        self._RunVMTests())
    self.assertEquals(self.all_vm_test_stages, self.called_stages)

  def testVMTestStageFails(self):
    """Verify TastVMTestStage is still run when VMTestStage fails."""
    self.stage_exceptions = {
        vm_test_stages.VMTestStage: failures_lib.TestFailure(),
    }
    self.assertEquals([failures_lib.TestFailure], self._RunVMTests())
    self.assertEquals(self.all_vm_test_stages, self.called_stages)

  def testTastVMTestStageFails(self):
    """Verify VMTestStage is still run when TastVMTestStage fails."""
    self.stage_exceptions = {
        tast_test_stages.TastVMTestStage: failures_lib.TestFailure(),
    }
    self.assertEquals([failures_lib.TestFailure], self._RunVMTests())
    self.assertEquals(self.all_vm_test_stages, self.called_stages)


class DistributedBuilderTests(SimpleBuilderTest):
  """Tests for DistributedBuilder."""

  def testRunStagesCommitQueueMaster(self):
    """Verify RunStages for master-paladin builder."""
    builder_run = self._initConfig('master-paladin', master=True)
    builder = simple_builders.DistributedBuilder(builder_run)
    builder.sync_stage = mock.Mock()
    builder.completion_stage_class = mock.Mock()
    builder.RunStages()
    self.assertTrue(handle_changes_stages.CommitQueueHandleChangesStage
                    in self.called_stages)

  def testRunStagesCommitQueueMasterWithImportantBuilderFailedException(self):
    """Verify RunStages for CQ-master with ImportantBuilderFailedException."""
    builder_run = self._initConfig('master-paladin', master=True)
    builder = simple_builders.DistributedBuilder(builder_run)
    builder.sync_stage = mock.Mock()
    builder.completion_stage_class = (
        completion_stages.CommitQueueCompletionStage)
    self.PatchObject(
        completion_stages.CommitQueueCompletionStage, '__init__',
        return_value=None)
    self.PatchObject(
        completion_stages.CommitQueueCompletionStage, 'Run',
        side_effect=completion_stages.ImportantBuilderFailedException)
    self.assertRaises(completion_stages.ImportantBuilderFailedException,
                      builder.RunStages)
    self.assertTrue(handle_changes_stages.CommitQueueHandleChangesStage
                    in self.called_stages)

  def testRunStagesCommitQueueMasterWithStepFailure(self):
    """Verify RunStages for CQ-master with StepFailure."""
    builder_run = self._initConfig('master-paladin', master=True)
    builder = simple_builders.DistributedBuilder(builder_run)
    builder.sync_stage = mock.Mock()
    builder.completion_stage_class = (
        completion_stages.CommitQueueCompletionStage)
    self.PatchObject(
        completion_stages.CommitQueueCompletionStage, '__init__',
        return_value=None)
    self.PatchObject(
        completion_stages.CommitQueueCompletionStage, 'Run',
        side_effect=failures_lib.StepFailure)
    self.assertRaises(failures_lib.StepFailure, builder.RunStages)
    self.assertFalse(handle_changes_stages.CommitQueueHandleChangesStage
                     in self.called_stages)

  def testRunStagesReleaseMaster(self):
    """Verify RunStages for master-release builder."""
    builder_run = self._initConfig('master-release', master=True)
    builder = simple_builders.DistributedBuilder(builder_run)
    builder.sync_stage = mock.Mock()
    builder.completion_stage_class = mock.Mock()
    builder.RunStages()
    self.assertFalse(handle_changes_stages.CommitQueueHandleChangesStage
                     in self.called_stages)
