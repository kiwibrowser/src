# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing builders intended for testing cbuildbot behaviors."""

from __future__ import print_function


from chromite.lib import cros_logging as logging
from chromite.lib import parallel

from chromite.cbuildbot import manifest_version
from chromite.cbuildbot.builders import generic_builders
from chromite.cbuildbot.builders import simple_builders
from chromite.cbuildbot.stages import build_stages
from chromite.cbuildbot.stages import android_stages
from chromite.cbuildbot.stages import artifact_stages
from chromite.cbuildbot.stages import chrome_stages
from chromite.cbuildbot.stages import generic_stages
from chromite.cbuildbot.stages import sync_stages
from chromite.cbuildbot.stages import test_stages
from chromite.cbuildbot.stages import vm_test_stages


class SuccessStage(generic_stages.BuilderStage):
  """Build stage declares success!"""
  def PerformStage(self):
    logging.info('!!!SuccessStage, FTW!!!')


class FailStage(generic_stages.BuilderStage):
  """Build stage always fails."""
  def PerformStage(self):
    raise Exception('!!!Oh, no! A Fail Stage!!!')


class SucessBuilder(generic_builders.PreCqBuilder):
  """Very minimal builder that always passes."""
  def RunTestStages(self):
    """Run a success stage!"""
    self._RunStage(SuccessStage)


class FailBuilder(generic_builders.PreCqBuilder):
  """Very minimal builder that always fails."""
  def RunTestStages(self):
    """Run fail stage!"""
    self._RunStage(FailStage)


class ManifestVersionedSyncBuilder(generic_builders.Builder):
  """Builder that performs sync, then exits."""

  def GetVersionInfo(self):
    """Returns the CrOS version info from the chromiumos-overlay."""
    return manifest_version.VersionInfo.from_repo(self._run.buildroot)

  def GetSyncInstance(self):
    """Returns an instance of a SyncStage that should be run."""
    return self._GetStageInstance(sync_stages.ManifestVersionedSyncStage)

  def RunStages(self):
    """Run something after sync/reexec."""
    self._RunStage(SuccessStage)


class UnittestStressBuilder(generic_builders.Builder):
  """Builder that runs unittests repeatedly to reproduce flake failures."""

  TEST_CYCLES = 20

  def GetVersionInfo(self):
    """Returns the CrOS version info from the chromiumos-overlay."""
    return manifest_version.VersionInfo.from_repo(self._run.buildroot)

  def GetSyncInstance(self):
    """Returns an instance of a SyncStage that should be run."""
    return self._GetStageInstance(sync_stages.ManifestVersionedSyncStage)

  def RunStages(self):
    """Run something after sync/reexec."""
    assert len(self._run.config.boards) == 1
    board = self._run.config.boards[0]

    self._RunStage(build_stages.UprevStage)
    self._RunStage(build_stages.InitSDKStage)
    self._RunStage(build_stages.RegenPortageCacheStage)
    self._RunStage(build_stages.SetupBoardStage, board)
    self._RunStage(chrome_stages.SyncChromeStage)
    self._RunStage(android_stages.UprevAndroidStage)
    self._RunStage(android_stages.AndroidMetadataStage)
    self._RunStage(build_stages.BuildPackagesStage, board)

    for i in xrange(self.TEST_CYCLES):
      self._RunStage(test_stages.UnitTestStage, board, suffix=' - %d' % i)


class SignerTestsBuilder(generic_builders.PreCqBuilder):
  """Builder that runs the cros-signing tests, and nothing else."""
  def RunTestStages(self):
    """Run the signer tests."""
    self._RunStage(build_stages.InitSDKStage)
    self._RunStage(test_stages.CrosSigningTestStage)


class ChromiteTestsBuilder(generic_builders.PreCqBuilder):
  """Builder that runs chromite unit tests, including network."""
  def RunTestStages(self):
    """Run something after sync/reexec."""
    self._RunStage(build_stages.InitSDKStage)
    self._RunStage(test_stages.ChromiteTestStage)
    # TODO(crbug.com/820305): Enable after the flake issue is fixed.
    # self._RunStage(test_stages.CidbIntegrationTestStage)


class VMInformationalBuilder(simple_builders.SimpleBuilder):
  """Builder that runs vm test for informational purpose."""
  def _RunDebugSymbolStages(self, builder_run, board):
    self._RunStage(android_stages.DownloadAndroidDebugSymbolsStage,
                   board, builder_run=builder_run)
    self._RunStage(artifact_stages.DebugSymbolsStage, board,
                   builder_run=builder_run)

  def RunStages(self):
    assert len(self._run.config.boards) == 1
    board = self._run.config.boards[0]

    self._RunStage(build_stages.UprevStage)
    self._RunStage(build_stages.InitSDKStage)
    self._RunStage(build_stages.RegenPortageCacheStage)
    self._RunStage(build_stages.SetupBoardStage, board)
    self._RunStage(chrome_stages.SyncChromeStage)
    self._RunStage(android_stages.UprevAndroidStage)
    self._RunStage(android_stages.AndroidMetadataStage)
    self._RunStage(build_stages.BuildPackagesStage, board)
    self._RunStage(build_stages.BuildImageStage, board)

    parallel_stages = [
        lambda: self._RunDebugSymbolStages(self._run, board),
        lambda: self._RunStage(vm_test_stages.VMTestStage, board)
    ]
    parallel.RunParallelSteps(parallel_stages)
