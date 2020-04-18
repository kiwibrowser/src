# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing alert generating stages."""

from __future__ import print_function

import os
from chromite.cbuildbot.stages import generic_stages
from chromite.lib import buildbucket_lib
from chromite.lib import cidb
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import failures_lib


class SomDispatcherStage(generic_stages.BuilderStage):
  """Stage to dispatch Sheriff-o-Matic alerts. go/som/chromeos"""

  def __init__(self, builder_run, tree, **kwargs):
    """Init that requires the tree argument.

    Args:
      builder_run: See builder_run on BuilderStage.
      tree: SoM instance name. See constants.SOM_BUILDS.
    """
    suffix = ' [%s]' % tree
    super(SomDispatcherStage, self).__init__(
        builder_run, suffix=suffix, **kwargs)
    self.tree = tree

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def RunAlertsDispatcher(self, db_credentials_dir, tree):
    """Submit alerts summary to Sheriff-o-Matic.

    Args:
      db_credentials_dir: Path to CIDB database credentials.
      tree: Sheriff-o-Matic tree to submit alerts to.
    """
    dispatcher_cmd = [os.path.join(self._build_root, 'chromite', 'scripts',
                                   'som_alerts_dispatcher'),
                      '--som_tree', tree]
    if buildbucket_lib.GetServiceAccount(constants.CHROMEOS_SERVICE_ACCOUNT):
      # User the service account file if it exists.
      dispatcher_cmd.extend(['--service_acct_json',
                             constants.CHROMEOS_SERVICE_ACCOUNT])
    if tree != constants.SOM_TREE:
      dispatcher_cmd.append('--allow_experimental')
    dispatcher_cmd.append(db_credentials_dir)

    cros_build_lib.RunCommand(dispatcher_cmd)

  def PerformStage(self):
    db = cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder()
    self.RunAlertsDispatcher(db.db_credentials_dir, self.tree)
