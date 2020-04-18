# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing SDK builders."""

from __future__ import print_function

import datetime

from chromite.lib import constants
from chromite.cbuildbot.builders import simple_builders
from chromite.cbuildbot.stages import android_stages
from chromite.cbuildbot.stages import artifact_stages
from chromite.cbuildbot.stages import build_stages
from chromite.cbuildbot.stages import chrome_stages
from chromite.cbuildbot.stages import sdk_stages


class ChrootSdkBuilder(simple_builders.SimpleBuilder):
  """Build the SDK chroot."""

  def RunStages(self):
    """Runs through build process."""
    # Unlike normal CrOS builds, the SDK has no concept of pinned CrOS manifest
    # or specific Chrome version.  Use a datestamp instead.
    version = datetime.datetime.now().strftime('%Y.%m.%d.%H%M%S')
    self._RunStage(build_stages.UprevStage, boards=[])
    self._RunStage(build_stages.InitSDKStage)
    self._RunStage(build_stages.SetupBoardStage, constants.CHROOT_BUILDER_BOARD)
    self._RunStage(chrome_stages.SyncChromeStage)
    self._RunStage(android_stages.UprevAndroidStage)
    self._RunStage(sdk_stages.SDKBuildToolchainsStage)
    self._RunStage(sdk_stages.SDKPackageStage, version=version)
    # Note: This produces badly named toolchain tarballs.  Before
    # we re-enable it, make sure we fix that first.  For example:
    # gs://chromiumos-sdk/2015/06/...
    #   .../cros-sdk-overlay-toolchains-aarch64-cros-linux-gnu-$VER.tar.xz
    #self._RunStage(sdk_stages.SDKPackageToolchainOverlaysStage,
    #               version=version)

    self._RunStage(sdk_stages.SDKTestStage)
    # manojgupta: The comment in para below is not valid right now. Need to
    # test SDK before making artifacts available to users because of
    # https://crbug.com/798617.
    # Move it after UploadPrebuilts again once the bug is fixed.
    #
    # Upload artifacts before tests. Testing takes several hours, so during
    # that, goma server will find the new archive and stage it. So that,
    # goma can be used on bots just after uprev.
    # TODO(hidehiko): We may want to run upload prebuilts and test in
    # parallel in future, if it becomes performance bottleneck.
    self._RunStage(artifact_stages.UploadPrebuiltsStage,
                   constants.CHROOT_BUILDER_BOARD, version=version)

    self._RunStage(sdk_stages.SDKUprevStage, version=version)
