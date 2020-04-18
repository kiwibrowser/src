# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing config related builders."""

from __future__ import print_function

from chromite.cbuildbot.builders import simple_builders
from chromite.cbuildbot.stages import build_stages
from chromite.cbuildbot.stages import config_stages

class UpdateConfigBuilder(simple_builders.SimpleBuilder):
  """Create config updater builders."""

  def RunStages(self):
    """Run through the stages of a config-updater build."""
    self._RunStage(build_stages.InitSDKStage)
    self._RunStage(config_stages.CheckTemplateStage)
