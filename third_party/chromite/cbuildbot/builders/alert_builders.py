# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing alert generating builders."""

from __future__ import print_function

from chromite.cbuildbot.builders import simple_builders
from chromite.cbuildbot.stages import alert_stages
from chromite.lib import constants

class SomDispatcherBuilder(simple_builders.SimpleBuilder):
  """Dispatch alerts to Sheriff o Matic."""

  def RunStages(self):
    """Run through the stages."""
    for tree in constants.SOM_BUILDS.keys():
      self._RunStage(alert_stages.SomDispatcherStage, tree=tree)
