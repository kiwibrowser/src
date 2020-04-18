# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for clactions methods."""

from __future__ import print_function

import datetime
import itertools

from chromite.lib.const import waterfall
from chromite.lib import constants
from chromite.lib import metadata_lib
from chromite.lib import fake_cidb
from chromite.lib import clactions
from chromite.lib import clactions_metrics
from chromite.lib import cros_test_lib


class TestCLActionsMetrics(cros_test_lib.TestCase):
  """Tests various methods related to CL action history."""

  def setUp(self):
    self.fake_db = fake_cidb.FakeCIDBConnection()

  def testRecordSubmissionMetrics(self):
    """Test that we correctly compute a CL's handling time."""
    change = metadata_lib.GerritPatchTuple(1, 1, False)
    launcher_id = self.fake_db.InsertBuild(
        'launcher', waterfall.WATERFALL_INTERNAL, 1,
        constants.PRE_CQ_LAUNCHER_CONFIG, 'hostname')
    trybot_id = self.fake_db.InsertBuild(
        'banana pre cq', waterfall.WATERFALL_INTERNAL, 1,
        'banana-pre-cq', 'hostname')
    master_id = self.fake_db.InsertBuild(
        'CQ master', waterfall.WATERFALL_INTERNAL, 1,
        constants.CQ_MASTER, 'hostname')
    slave_id = self.fake_db.InsertBuild(
        'banana paladin', waterfall.WATERFALL_INTERNAL, 1,
        'banana-paladin', 'hostname')

    start_time = datetime.datetime.now()
    c = itertools.count()

    def next_time():
      return start_time + datetime.timedelta(seconds=c.next())

    def a(build_id, action, reason=None):
      self._Act(build_id, change, action, reason=reason, timestamp=next_time())

    strategies = {}

    # Change is screened, picked up, and rejected by the pre-cq,
    # non-speculatively.
    a(launcher_id, constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
      reason='banana-pre-cq')
    a(launcher_id, constants.CL_ACTION_SCREENED_FOR_PRE_CQ)
    a(launcher_id, constants.CL_ACTION_TRYBOT_LAUNCHING,
      reason='banana-pre-cq')
    a(trybot_id, constants.CL_ACTION_PICKED_UP)
    a(trybot_id, constants.CL_ACTION_KICKED_OUT)

    # Change is re-marked by developer, picked up again by pre-cq, verified,
    # and marked as passed.
    a(launcher_id, constants.CL_ACTION_REQUEUED)
    a(launcher_id, constants.CL_ACTION_TRYBOT_LAUNCHING,
      reason='banana-pre-cq')
    a(trybot_id, constants.CL_ACTION_PICKED_UP)
    a(trybot_id, constants.CL_ACTION_VERIFIED)
    a(launcher_id, constants.CL_ACTION_PRE_CQ_FULLY_VERIFIED)
    a(launcher_id, constants.CL_ACTION_PRE_CQ_PASSED)

    # Change is picked up by the CQ and rejected.
    a(master_id, constants.CL_ACTION_PICKED_UP)
    a(slave_id, constants.CL_ACTION_PICKED_UP)
    a(master_id, constants.CL_ACTION_KICKED_OUT)

    # Change is re-marked, picked up by the CQ, and forgiven.
    a(launcher_id, constants.CL_ACTION_REQUEUED)
    a(master_id, constants.CL_ACTION_PICKED_UP)
    a(slave_id, constants.CL_ACTION_PICKED_UP)
    a(master_id, constants.CL_ACTION_FORGIVEN)

    # Change is re-marked, picked up by the CQ, and forgiven.
    a(master_id, constants.CL_ACTION_PICKED_UP)
    a(slave_id, constants.CL_ACTION_PICKED_UP)
    a(master_id, constants.CL_ACTION_SUBMITTED)
    strategies[change] = constants.STRATEGY_CQ_SUCCESS

    action_history = self.fake_db.GetActionsForChanges([change])
    clactions_metrics.RecordSubmissionMetrics(
        clactions.CLActionHistory(action_history), strategies)

  def _Act(self, build_id, change, action, reason=None, timestamp=None):
    self.fake_db.InsertCLActions(
        build_id,
        [clactions.CLAction.FromGerritPatchAndAction(change, action, reason)],
        timestamp=timestamp)
