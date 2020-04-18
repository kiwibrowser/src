# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing release engineering related builders."""

from __future__ import print_function

from chromite.lib import parallel

from chromite.cbuildbot.builders import simple_builders
from chromite.cbuildbot.stages import branch_stages
from chromite.cbuildbot.stages import build_stages
from chromite.cbuildbot.stages import release_stages


class CreateBranchBuilder(simple_builders.SimpleBuilder):
  """Create release branches in the manifest."""

  def RunStages(self):
    """Runs through build process."""
    self._RunStage(branch_stages.BranchUtilStage)


class GeneratePayloadsBuilder(simple_builders.SimpleBuilder):
  """Run the PaygenStage once for each board."""

  def RunStages(self):
    """Runs through build process."""
    def _RunStageWrapper(board):
      self._RunStage(build_stages.InitSDKStage)
      self._RunStage(release_stages.PaygenStage, board=board,
                     channels=self._run.options.channels)

    with parallel.BackgroundTaskRunner(_RunStageWrapper) as queue:
      for board in self._run.config.boards:
        queue.put([board])
