# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing builders for infra."""

from __future__ import print_function

from chromite.cbuildbot import manifest_version
from chromite.cbuildbot.builders import generic_builders
from chromite.cbuildbot.stages import build_stages
from chromite.cbuildbot.stages import infra_stages
from chromite.cbuildbot.stages import sync_stages


class InfraGoBuilder(generic_builders.Builder):
  """Builder that builds infra Go binaries."""

  def GetVersionInfo(self):
    """Returns the CrOS version info from the chromiumos-overlay."""
    return manifest_version.VersionInfo.from_repo(self._run.buildroot)

  def GetSyncInstance(self):
    """Returns an instance of a SyncStage that should be run."""
    return self._GetStageInstance(sync_stages.ManifestVersionedSyncStage)

  def RunStages(self):
    """Build and upload infra Go binaries."""
    self._RunStage(build_stages.UprevStage)
    self._RunStage(build_stages.InitSDKStage)
    self._RunStage(infra_stages.EmergeInfraGoBinariesStage)
    self._RunStage(infra_stages.PackageInfraGoBinariesStage)
    self._RunStage(infra_stages.RegisterInfraGoPackagesStage)


class PuppetPreCqBuilder(generic_builders.PreCqBuilder):
  """Builder that tests Puppet config."""

  def GetVersionInfo(self):
    """Returns the CrOS version info from the chromiumos-overlay."""
    return manifest_version.VersionInfo.from_repo(self._run.buildroot)

  def RunTestStages(self):
    """Run Puppet RSpec tests."""
    self._RunStage(build_stages.UprevStage)
    self._RunStage(build_stages.InitSDKStage)
    self._RunStage(infra_stages.TestPuppetSpecsStage)


class InfraUnittestsPreCqBuilder(generic_builders.PreCqBuilder):
  """Builder that runs infra projects' venved unittests."""

  def GetVersionInfo(self):
    """Returns the CrOS version info from the chromiumos-overlay."""
    return manifest_version.VersionInfo.from_repo(self._run.buildroot)

  def RunTestStages(self):
    """Run various infra venv package unittsets."""
    self._RunStage(build_stages.UprevStage)
    self._RunStage(build_stages.InitSDKStage)
    self._RunStage(infra_stages.TestVenvPackagesStage)
