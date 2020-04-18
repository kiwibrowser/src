# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for creating fuzzer builds."""

from __future__ import print_function

import functools

from chromite.lib import cros_logging as logging
from chromite.lib import parallel

from chromite.cbuildbot import manifest_version
from chromite.cbuildbot.builders import simple_builders
from chromite.cbuildbot.stages import android_stages
from chromite.cbuildbot.stages import artifact_stages
from chromite.cbuildbot.stages import build_stages
from chromite.cbuildbot.stages import chrome_stages
from chromite.cbuildbot.stages import sync_stages
from chromite.cbuildbot.stages import test_stages


class FuzzerBuilder(simple_builders.SimpleBuilder):
  """Builder that creates builds for fuzzing Chrome OS."""

  def RunStages(self):
    """Run stages for fuzzer builder."""
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
    self._RunStage(artifact_stages.GenerateSysrootStage, board)
