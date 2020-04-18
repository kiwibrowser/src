# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for clactions methods."""

from __future__ import print_function

import datetime
import itertools
import random

from chromite.lib.const import waterfall
from chromite.lib import constants
from chromite.lib import metadata_lib
from chromite.cbuildbot import validation_pool
from chromite.lib import fake_cidb
from chromite.lib import clactions
from chromite.lib import cros_test_lib


class CLActionTest(cros_test_lib.TestCase):
  """Placeholder for clactions unit tests."""

  def runTest(self):
    pass


class TestCLActionHistory(cros_test_lib.TestCase):
  """Tests various methods related to CL action history."""

  def setUp(self):
    self.fake_db = fake_cidb.FakeCIDBConnection()

  def testGetCLHandlingTime(self):
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

    # Change is re-marked by developer, picked up again by pre-cq, verified, and
    # marked as passed.
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
    # Note: There are 2 ticks in the total handling time that are not accounted
    # for in the sub-times. These are the time between VALIDATION_PENDING and
    # SCREENED, and the time between FULLY_VERIFIED and PASSED.
    self.assertEqual(18, clactions.GetCLHandlingTime(change, action_history))
    self.assertEqual(7, clactions.GetPreCQTime(change, action_history))
    self.assertEqual(3, clactions.GetCQWaitTime(change, action_history))
    self.assertEqual(6, clactions.GetCQRunTime(change, action_history))
    self.assertEqual(3, clactions.GetCQAttemptsCount(change, action_history))

  def _Act(self, build_id, change, action, reason=None, timestamp=None):
    self.fake_db.InsertCLActions(
        build_id,
        [clactions.CLAction.FromGerritPatchAndAction(change, action, reason)],
        timestamp=timestamp)

  def _GetCLStatus(self, change):
    """Helper method to get a CL's pre-CQ status from fake_db."""
    action_history = self.fake_db.GetActionsForChanges([change])
    return clactions.GetCLPreCQStatus(change, action_history)

  def testGetOldPreCQBuildActions(self):
    """Test GetOldPreCQBuildActions."""
    c1 = metadata_lib.GerritPatchTuple(1, 1, False)
    c2 = metadata_lib.GerritPatchTuple(1, 2, False)
    changes = [c1, c2]

    build_id = self.fake_db.InsertBuild('n', 'w', 1, 'c', 'h')

    a1 = clactions.CLAction.FromGerritPatchAndAction(
        c1, constants.CL_ACTION_TRYBOT_LAUNCHING,
        reason='binhost-pre-cq',
        timestamp=datetime.datetime.now() - datetime.timedelta(hours=5))
    a2 = clactions.CLAction.FromGerritPatchAndAction(
        c1, constants.CL_ACTION_TRYBOT_LAUNCHING,
        reason='binhost-pre-cq',
        timestamp=datetime.datetime.now() - datetime.timedelta(hours=4),
        buildbucket_id='1')
    a3 = clactions.CLAction.FromGerritPatchAndAction(
        c1, constants.CL_ACTION_TRYBOT_LAUNCHING,
        reason='binhost-pre-cq',
        timestamp=datetime.datetime.now() - datetime.timedelta(hours=3),
        buildbucket_id='2')
    a4 = clactions.CLAction.FromGerritPatchAndAction(
        c1, constants.CL_ACTION_TRYBOT_LAUNCHING,
        reason='pbinhost-pre-cq',
        timestamp=datetime.datetime.now() - datetime.timedelta(hours=3),
        buildbucket_id='3')
    a5 = clactions.CLAction.FromGerritPatchAndAction(
        c1, constants.CL_ACTION_TRYBOT_CANCELLED,
        reason='binhost-pre-cq',
        timestamp=datetime.datetime.now() - datetime.timedelta(hours=3),
        buildbucket_id='3')
    a6 = clactions.CLAction.FromGerritPatchAndAction(
        c1, constants.CL_ACTION_TRYBOT_LAUNCHING,
        reason='binhost-pre-cq',
        timestamp=datetime.datetime.now() - datetime.timedelta(hours=1),
        buildbucket_id='4')
    a7 = clactions.CLAction.FromGerritPatchAndAction(
        c2, constants.CL_ACTION_TRYBOT_LAUNCHING,
        reason='binhost-pre-cq',
        timestamp=datetime.datetime.now(),
        buildbucket_id='5')

    cl_actions = [a1, a2, a3, a4, a5, a6, a7]

    self.fake_db.InsertCLActions(build_id, cl_actions)
    action_history = self.fake_db.GetActionsForChanges(changes)

    timestamp = datetime.datetime.now() - datetime.timedelta(hours=2)
    c1_old_actions = clactions.GetOldPreCQBuildActions(
        c1, action_history, timestamp)
    c2_old_actions = clactions.GetOldPreCQBuildActions(
        c2, action_history, timestamp)
    self.assertTrue(len(c1_old_actions) == 0)
    self.assertTrue(len(c2_old_actions) == 1)
    self.assertEqual([c.buildbucket_id for c in c2_old_actions],
                     [a6.buildbucket_id])

    c1_old_actions = clactions.GetOldPreCQBuildActions(
        c1, action_history)
    c2_old_actions = clactions.GetOldPreCQBuildActions(
        c2, action_history)
    self.assertTrue(len(c1_old_actions) == 0)
    self.assertTrue(len(c2_old_actions) == 3)
    self.assertEqual([c.buildbucket_id for c in c2_old_actions],
                     [a2.buildbucket_id, a3.buildbucket_id,
                      a6.buildbucket_id])

  def testGetCancelledPreCQBuilds(self):
    """Test GetCancelledPreCQBuilds."""
    c1 = metadata_lib.GerritPatchTuple(1, 1, False)
    build_id = self.fake_db.InsertBuild('n', 'w', 1, 'c', 'h')
    a1 = clactions.CLAction.FromGerritPatchAndAction(
        c1, constants.CL_ACTION_TRYBOT_CANCELLED,
        reason='binhost-pre-cq')
    a2 = clactions.CLAction.FromGerritPatchAndAction(
        c1, constants.CL_ACTION_TRYBOT_LAUNCHING,
        reason='binhost-pre-cq',
        buildbucket_id='1')
    a3 = clactions.CLAction.FromGerritPatchAndAction(
        c1, constants.CL_ACTION_TRYBOT_CANCELLED,
        reason='binhost-pre-cq',
        buildbucket_id='2')
    cl_actions = [a1, a2, a3]
    self.fake_db.InsertCLActions(build_id, cl_actions)
    action_history = self.fake_db.GetActionsForChanges([c1])
    builds = clactions.GetCancelledPreCQBuilds(action_history)
    self.assertEqual(len(builds), 1)
    self.assertEqual(builds.pop().buildbucket_id, '2')

  def testGetRequeuedOrSpeculative(self):
    """Tests GetRequeuedOrSpeculative function."""
    change = metadata_lib.GerritPatchTuple(1, 1, False)
    speculative_change = metadata_lib.GerritPatchTuple(2, 2, False)
    changes = [change, speculative_change]

    build_id = self.fake_db.InsertBuild('n', 'w', 1, 'c', 'h')

    # A fresh change should not be marked requeued. A fresh specualtive
    # change should be marked as speculative.
    action_history = self.fake_db.GetActionsForChanges(changes)
    a = clactions.GetRequeuedOrSpeculative(change, action_history, False)
    self.assertEqual(a, None)
    a = clactions.GetRequeuedOrSpeculative(speculative_change, action_history,
                                           True)
    self.assertEqual(a, constants.CL_ACTION_SPECULATIVE)
    self._Act(build_id, speculative_change, a)

    # After picking up either change, neither should need an additional
    # requeued or speculative action.
    self._Act(build_id, speculative_change, constants.CL_ACTION_PICKED_UP)
    self._Act(build_id, change, constants.CL_ACTION_PICKED_UP)
    action_history = self.fake_db.GetActionsForChanges(changes)
    a = clactions.GetRequeuedOrSpeculative(change, action_history, False)
    self.assertEqual(a, None)
    a = clactions.GetRequeuedOrSpeculative(speculative_change, action_history,
                                           True)
    self.assertEqual(a, None)

    # After being rejected, both changes need an action (requeued and
    # speculative accordingly).
    self._Act(build_id, speculative_change, constants.CL_ACTION_KICKED_OUT)
    self._Act(build_id, change, constants.CL_ACTION_KICKED_OUT)
    action_history = self.fake_db.GetActionsForChanges(changes)
    a = clactions.GetRequeuedOrSpeculative(change, action_history, False)
    self.assertEqual(a, constants.CL_ACTION_REQUEUED)
    self._Act(build_id, change, a)
    a = clactions.GetRequeuedOrSpeculative(speculative_change, action_history,
                                           True)
    self.assertEqual(a, constants.CL_ACTION_SPECULATIVE)
    self._Act(build_id, speculative_change, a)

    # Once a speculative change becomes un-speculative, it needs a REQUEUD
    # action.
    action_history = self.fake_db.GetActionsForChanges(changes)
    a = clactions.GetRequeuedOrSpeculative(speculative_change, action_history,
                                           False)
    self.assertEqual(a, constants.CL_ACTION_REQUEUED)
    self._Act(build_id, speculative_change, a)

  def testGetCLPreCQStatus(self):
    change = metadata_lib.GerritPatchTuple(1, 1, False)
    # Initial pre-CQ status of a change is None.
    self.assertEqual(self._GetCLStatus(change), None)

    # Builders can update the CL's pre-CQ status.
    build_id = self.fake_db.InsertBuild(
        constants.PRE_CQ_LAUNCHER_NAME, waterfall.WATERFALL_INTERNAL, 1,
        constants.PRE_CQ_LAUNCHER_CONFIG, 'bot-hostname')

    self._Act(build_id, change, constants.CL_ACTION_PRE_CQ_WAITING)
    self.assertEqual(self._GetCLStatus(change), constants.CL_STATUS_WAITING)

    self._Act(build_id, change, constants.CL_ACTION_PRE_CQ_INFLIGHT)
    self.assertEqual(self._GetCLStatus(change), constants.CL_STATUS_INFLIGHT)

    # Recording a cl action that is not a valid pre-cq status should leave
    # pre-cq status unaffected.
    self._Act(build_id, change, 'polenta')
    self.assertEqual(self._GetCLStatus(change), constants.CL_STATUS_INFLIGHT)

    self._Act(build_id, change, constants.CL_ACTION_PRE_CQ_RESET)
    self.assertEqual(self._GetCLStatus(change), None)

  def testGetCLPreCQProgress(self):
    change = metadata_lib.GerritPatchTuple(1, 1, False)
    s = lambda: clactions.GetCLPreCQProgress(
        change, self.fake_db.GetActionsForChanges([change]))

    self.assertEqual({}, s())

    # Simulate the pre-cq-launcher screening changes for pre-cq configs
    # to test with.
    launcher_build_id = self.fake_db.InsertBuild(
        constants.PRE_CQ_LAUNCHER_NAME, waterfall.WATERFALL_INTERNAL,
        1, constants.PRE_CQ_LAUNCHER_CONFIG, 'bot hostname 1')

    self._Act(launcher_build_id, change,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'pineapple-pre-cq')
    self._Act(launcher_build_id, change,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'banana-pre-cq')

    configs = ['banana-pre-cq', 'pineapple-pre-cq']

    self.assertEqual(configs, sorted(s().keys()))
    for c in configs:
      self.assertEqual(constants.CL_PRECQ_CONFIG_STATUS_PENDING,
                       s()[c][0])

    # Simulate a prior build rejecting change
    self._Act(launcher_build_id, change,
              constants.CL_ACTION_KICKED_OUT,
              'pineapple-pre-cq')
    self.assertEqual(constants.CL_PRECQ_CONFIG_STATUS_FAILED,
                     s()['pineapple-pre-cq'][0])

    # Simulate the pre-cq-launcher launching tryjobs for all pending configs.
    for c in configs:
      self._Act(launcher_build_id, change,
                constants.CL_ACTION_TRYBOT_LAUNCHING, c)
    for c in configs:
      self.assertEqual(constants.CL_PRECQ_CONFIG_STATUS_LAUNCHED,
                       s()[c][0])

    # Simulate the tryjobs launching, and picking up the changes.
    banana_build_id = self.fake_db.InsertBuild(
        'banana', waterfall.WATERFALL_TRYBOT, 12, 'banana-pre-cq',
        'banana hostname')
    pineapple_build_id = self.fake_db.InsertBuild(
        'pineapple', waterfall.WATERFALL_TRYBOT, 87, 'pineapple-pre-cq',
        'pineapple hostname')

    self._Act(banana_build_id, change, constants.CL_ACTION_PICKED_UP)
    self._Act(pineapple_build_id, change, constants.CL_ACTION_PICKED_UP)
    for c in configs:
      self.assertEqual(constants.CL_PRECQ_CONFIG_STATUS_INFLIGHT,
                       s()[c][0])

    # Simulate the changes being retried.
    self._Act(banana_build_id, change, constants.CL_ACTION_FORGIVEN)
    self._Act(launcher_build_id, change, constants.CL_ACTION_FORGIVEN,
              'pineapple-pre-cq')
    for c in configs:
      self.assertEqual(constants.CL_PRECQ_CONFIG_STATUS_PENDING,
                       s()[c][0])
    # Simulate the changes being rejected, either by the configs themselves
    # or by the pre-cq-launcher.
    self._Act(banana_build_id, change, constants.CL_ACTION_KICKED_OUT)
    self._Act(launcher_build_id, change, constants.CL_ACTION_KICKED_OUT,
              'pineapple-pre-cq')
    for c in configs:
      self.assertEqual(constants.CL_PRECQ_CONFIG_STATUS_FAILED,
                       s()[c][0])
    # Simulate the tryjobs verifying the changes.
    self._Act(banana_build_id, change, constants.CL_ACTION_VERIFIED)
    self._Act(pineapple_build_id, change, constants.CL_ACTION_VERIFIED)
    for c in configs:
      self.assertEqual(constants.CL_PRECQ_CONFIG_STATUS_VERIFIED,
                       s()[c][0])

    # Simulate the pre-cq status being reset.
    self._Act(launcher_build_id, change, constants.CL_ACTION_PRE_CQ_RESET)
    self.assertEqual({}, s())

  def testGetCLPreCQCategoriesAndPendingCLs(self):
    c1 = metadata_lib.GerritPatchTuple(1, 1, False)
    c2 = metadata_lib.GerritPatchTuple(2, 2, False)
    c3 = metadata_lib.GerritPatchTuple(3, 3, False)
    c4 = metadata_lib.GerritPatchTuple(4, 4, False)
    c5 = metadata_lib.GerritPatchTuple(5, 5, False)

    launcher_build_id = self.fake_db.InsertBuild(
        constants.PRE_CQ_LAUNCHER_NAME, waterfall.WATERFALL_INTERNAL,
        1, constants.PRE_CQ_LAUNCHER_CONFIG, 'bot hostname 1')
    pineapple_build_id = self.fake_db.InsertBuild(
        'pineapple', waterfall.WATERFALL_TRYBOT, 87, 'pineapple-pre-cq',
        'pineapple hostname')
    guava_build_id = self.fake_db.InsertBuild(
        'guava', waterfall.WATERFALL_TRYBOT, 7, 'guava-pre-cq',
        'guava hostname')

    # c1 has 3 pending verifications, but only 1 inflight and 1
    # launching, so it is not busy/inflight.
    self._Act(launcher_build_id, c1,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'pineapple-pre-cq')
    self._Act(launcher_build_id, c1,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'banana-pre-cq')
    self._Act(launcher_build_id, c1,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'guava-pre-cq')
    self._Act(launcher_build_id, c1,
              constants.CL_ACTION_TRYBOT_LAUNCHING,
              'banana-pre-cq')
    self._Act(pineapple_build_id, c1, constants.CL_ACTION_PICKED_UP)

    # c2 has 3 pending verifications, 1 inflight and 1 launching, and 1 passed,
    # so it is busy.
    self._Act(launcher_build_id, c2,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'pineapple-pre-cq')
    self._Act(launcher_build_id, c2,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'banana-pre-cq')
    self._Act(launcher_build_id, c2,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'guava-pre-cq')
    self._Act(launcher_build_id, c2, constants.CL_ACTION_TRYBOT_LAUNCHING,
              'banana-pre-cq')
    self._Act(pineapple_build_id, c2, constants.CL_ACTION_PICKED_UP)
    self._Act(guava_build_id, c2, constants.CL_ACTION_VERIFIED)

    # c3 has 2 pending verifications, both passed, so it is passed.
    self._Act(launcher_build_id, c3,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'pineapple-pre-cq')
    self._Act(launcher_build_id, c3,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'guava-pre-cq')
    self._Act(pineapple_build_id, c3, constants.CL_ACTION_VERIFIED)
    self._Act(guava_build_id, c3, constants.CL_ACTION_VERIFIED)

    # c4 has 2 pending verifications: one is inflight and the other
    # passed. It is considered inflight and busy.
    self._Act(launcher_build_id, c4,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'pineapple-pre-cq')
    self._Act(launcher_build_id, c4,
              constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
              'guava-pre-cq')
    self._Act(pineapple_build_id, c4, constants.CL_ACTION_PICKED_UP)
    self._Act(guava_build_id, c4, constants.CL_ACTION_VERIFIED)

    # c5 has not even been screened.

    changes = [c1, c2, c3, c4, c5]
    action_history = self.fake_db.GetActionsForChanges(changes)
    progress_map = clactions.GetPreCQProgressMap(changes, action_history)

    self.assertEqual(({c2, c4}, {c4}, {c3}),
                     clactions.GetPreCQCategories(progress_map))

    # Among changes c1, c2, c3, only the guava-pre-cq config is pending. The
    # other configs are either inflight, launching, or passed everywhere.
    screened_changes = set(changes).intersection(progress_map)
    self.assertEqual({'guava-pre-cq'},
                     clactions.GetPreCQConfigsToTest(screened_changes,
                                                     progress_map))


class TestCLStatusCounter(cros_test_lib.TestCase):
  """Tests that GetCLActionCount behaves as expected."""

  def setUp(self):
    self.fake_db = fake_cidb.FakeCIDBConnection()

  def testGetCLActionCount(self):
    c1p1 = metadata_lib.GerritPatchTuple(1, 1, False)
    c1p2 = metadata_lib.GerritPatchTuple(1, 2, False)
    precq_build_id = self.fake_db.InsertBuild(
        constants.PRE_CQ_LAUNCHER_NAME, waterfall.WATERFALL_INTERNAL, 1,
        constants.PRE_CQ_LAUNCHER_CONFIG, 'bot-hostname')
    melon_build_id = self.fake_db.InsertBuild(
        'melon builder name', waterfall.WATERFALL_INTERNAL, 1,
        'melon-config-name', 'grape-bot-hostname')

    # Count should be zero before any actions are recorded.

    action_history = self.fake_db.GetActionsForChanges([c1p1])
    self.assertEqual(
        0,
        clactions.GetCLActionCount(
            c1p1, validation_pool.CQ_PIPELINE_CONFIGS,
            constants.CL_ACTION_KICKED_OUT, action_history))

    # Record 3 failures for c1p1, and some other actions. Only count the
    # actions from builders in validation_pool.CQ_PIPELINE_CONFIGS.
    self.fake_db.InsertCLActions(
        precq_build_id,
        [clactions.CLAction.FromGerritPatchAndAction(
            c1p1, constants.CL_ACTION_KICKED_OUT)])
    self.fake_db.InsertCLActions(
        precq_build_id,
        [clactions.CLAction.FromGerritPatchAndAction(
            c1p1, constants.CL_ACTION_PICKED_UP)])
    self.fake_db.InsertCLActions(
        precq_build_id,
        [clactions.CLAction.FromGerritPatchAndAction(
            c1p1, constants.CL_ACTION_KICKED_OUT)])
    self.fake_db.InsertCLActions(
        melon_build_id,
        [clactions.CLAction.FromGerritPatchAndAction(
            c1p1, constants.CL_ACTION_KICKED_OUT)])

    action_history = self.fake_db.GetActionsForChanges([c1p1])
    self.assertEqual(
        2,
        clactions.GetCLActionCount(
            c1p1, validation_pool.CQ_PIPELINE_CONFIGS,
            constants.CL_ACTION_KICKED_OUT, action_history))

    # Record a failure for c1p2. Now the latest patches failure count should be
    # 1 (true weather we pass c1p1 or c1p2), whereas the total failure count
    # should be 3.
    self.fake_db.InsertCLActions(
        precq_build_id,
        [clactions.CLAction.FromGerritPatchAndAction(
            c1p2, constants.CL_ACTION_KICKED_OUT)])

    action_history = self.fake_db.GetActionsForChanges([c1p1])
    self.assertEqual(
        1,
        clactions.GetCLActionCount(
            c1p1, validation_pool.CQ_PIPELINE_CONFIGS,
            constants.CL_ACTION_KICKED_OUT, action_history))
    self.assertEqual(
        1,
        clactions.GetCLActionCount(
            c1p2, validation_pool.CQ_PIPELINE_CONFIGS,
            constants.CL_ACTION_KICKED_OUT, action_history))
    self.assertEqual(
        3,
        clactions.GetCLActionCount(
            c1p2, validation_pool.CQ_PIPELINE_CONFIGS,
            constants.CL_ACTION_KICKED_OUT, action_history,
            latest_patchset_only=False))


class TestCLActionHistorySmoke(cros_test_lib.TestCase):
  """A basic test for the simpler aggregating API for CLActionHistory."""

  def setUp(self):

    self.cl1 = clactions.GerritChangeTuple(11111, True)
    self.cl1_patch1 = clactions.GerritPatchTuple(
        self.cl1.gerrit_number, 1, self.cl1.internal)
    self.cl1_patch2 = clactions.GerritPatchTuple(
        self.cl1.gerrit_number, 2, self.cl1.internal)

    self.cl2 = clactions.GerritChangeTuple(22222, True)
    self.cl2_patch1 = clactions.GerritPatchTuple(
        self.cl2.gerrit_number, 1, self.cl2.internal)
    self.cl2_patch2 = clactions.GerritPatchTuple(
        self.cl2.gerrit_number, 2, self.cl2.internal)

    self.cl3 = clactions.GerritChangeTuple(33333, True)
    self.cl3_patch1 = clactions.GerritPatchTuple(
        self.cl3.gerrit_number, 2, self.cl3.internal)

    # Expected actions in chronological order, most recent first.
    self.action1 = clactions.CLAction.FromGerritPatchAndAction(
        self.cl1_patch2, constants.CL_ACTION_SUBMITTED,
        timestamp=self._NDaysAgo(1))
    self.action2 = clactions.CLAction.FromGerritPatchAndAction(
        self.cl1_patch2, constants.CL_ACTION_KICKED_OUT,
        timestamp=self._NDaysAgo(2))
    self.action3 = clactions.CLAction.FromGerritPatchAndAction(
        self.cl2_patch2, constants.CL_ACTION_SUBMITTED,
        timestamp=self._NDaysAgo(3))
    self.action4 = clactions.CLAction.FromGerritPatchAndAction(
        self.cl1_patch1, constants.CL_ACTION_SUBMIT_FAILED,
        timestamp=self._NDaysAgo(4))
    self.action5 = clactions.CLAction.FromGerritPatchAndAction(
        self.cl1_patch1, constants.CL_ACTION_KICKED_OUT,
        timestamp=self._NDaysAgo(5))
    self.action6 = clactions.CLAction.FromGerritPatchAndAction(
        self.cl3_patch1, constants.CL_ACTION_SUBMITTED,
        reason=constants.STRATEGY_NONMANIFEST,
        timestamp=self._NDaysAgo(6))

    # CLActionHistory does not require the history to be given in chronological
    # order, so we provide them in reverse order, and expect them to be sorted
    # as appropriate.
    self.cl_action_stats = clactions.CLActionHistory([
        self.action1, self.action2, self.action3, self.action4, self.action5,
        self.action6])

  def _NDaysAgo(self, num_days):
    return datetime.datetime.today() - datetime.timedelta(num_days)

  def testAffected(self):
    """Tests that the Affected* methods DTRT."""
    self.assertEqual(set([self.cl1, self.cl2, self.cl3]),
                     self.cl_action_stats.affected_cls)
    self.assertEqual(
        set([self.cl1_patch1, self.cl1_patch2, self.cl2_patch2,
             self.cl3_patch1]),
        self.cl_action_stats.affected_patches)

  def testActions(self):
    """Tests that different types of actions are listed correctly."""
    self.assertEqual([self.action5, self.action2],
                     self.cl_action_stats.reject_actions)
    self.assertEqual([self.action6, self.action3, self.action1],
                     self.cl_action_stats.submit_actions)
    self.assertEqual([self.action4],
                     self.cl_action_stats.submit_fail_actions)

  def testSubmitted(self):
    """Tests that the list of submitted objects is correct."""
    self.assertEqual(set([self.cl1, self.cl2]),
                     self.cl_action_stats.GetSubmittedCLs())
    self.assertEqual(set([self.cl1, self.cl2, self.cl3]),
                     self.cl_action_stats.GetSubmittedCLs(False))
    self.assertEqual(set([self.cl1_patch2, self.cl2_patch2]),
                     self.cl_action_stats.GetSubmittedPatches())
    self.assertEqual(set([self.cl1_patch2, self.cl2_patch2, self.cl3_patch1]),
                     self.cl_action_stats.GetSubmittedPatches(False))


class TestCLActionHistoryRejections(cros_test_lib.TestCase):
  """Involved test of aggregation of rejections."""

  CQ_BUILD_CONFIG = 'lumpy-paladin'
  PRE_CQ_BUILD_CONFIG = 'pre-cq-group'

  def setUp(self):
    self._days_forward = 1
    self._build_id = 1
    self.action_history = []
    self.cl_action_stats = None

    self.cl1 = clactions.GerritChangeTuple(11111, True)
    self.cl1_patch1 = clactions.GerritPatchTuple(
        self.cl1.gerrit_number, 1, self.cl1.internal)
    self.cl1_patch2 = clactions.GerritPatchTuple(
        self.cl1.gerrit_number, 2, self.cl1.internal)

    self.cl2 = clactions.GerritChangeTuple(22222, True)
    self.cl2_patch1 = clactions.GerritPatchTuple(
        self.cl2.gerrit_number, 1, self.cl2.internal)
    self.cl2_patch2 = clactions.GerritPatchTuple(
        self.cl2.gerrit_number, 2, self.cl2.internal)

  def _AppendToHistory(self, patch, action, **kwargs):
    kwargs.setdefault('id', -1)
    kwargs.setdefault('build_id', -1)
    kwargs.setdefault('reason', '')
    kwargs.setdefault('build_config', '')
    kwargs['timestamp'] = (datetime.datetime.today() +
                           datetime.timedelta(self._days_forward))
    self._days_forward += 1
    kwargs['action'] = action
    kwargs['change_number'] = int(patch.gerrit_number)
    kwargs['patch_number'] = int(patch.patch_number)
    kwargs['change_source'] = clactions.BoolToChangeSource(patch.internal)
    kwargs['buildbucket_id'] = 'test-id'
    kwargs['status'] = None

    action = clactions.CLAction(**kwargs)
    self.action_history.append(action)
    return action

  def _PickupAndRejectPatch(self, patch, **kwargs):
    kwargs.setdefault('build_id', self._build_id)
    self._build_id += 1
    pickup_action = self._AppendToHistory(patch, constants.CL_ACTION_PICKED_UP,
                                          **kwargs)
    reject_action = self._AppendToHistory(patch, constants.CL_ACTION_KICKED_OUT,
                                          **kwargs)
    return pickup_action, reject_action

  def _CreateCLActionHistory(self):
    """Create the object under test, reordering the history.

    We reorder history in a fixed but arbitrary way, to test that order doesn't
    matter for the object under test.
    """
    random.seed(4)  # Everyone knows this is the randomest number on earth.
    random.shuffle(self.action_history)
    self.cl_action_stats = clactions.CLActionHistory(self.action_history)

  def testRejectionsNoRejection(self):
    """Tests the null case."""
    self._AppendToHistory(self.cl1_patch1, constants.CL_ACTION_SUBMITTED)
    self._CreateCLActionHistory()
    self.assertEqual({}, self.cl_action_stats.GetTrueRejections())
    self.assertEqual({}, self.cl_action_stats.GetFalseRejections())

  def testTrueRejectionsSkipApplyFailure(self):
    """Test that apply failures are not considered true rejections."""
    self._AppendToHistory(self.cl1_patch1, constants.CL_ACTION_KICKED_OUT)
    self._AppendToHistory(self.cl1_patch2, constants.CL_ACTION_SUBMITTED)
    self._CreateCLActionHistory()
    self.assertEqual({}, self.cl_action_stats.GetTrueRejections())

  def testTrueRejectionsIncludeLaterSubmitted(self):
    """Tests that we include CLs which have a patch that was later submitted."""
    _, reject_action = self._PickupAndRejectPatch(self.cl1_patch1)
    self._AppendToHistory(self.cl1_patch2, constants.CL_ACTION_SUBMITTED)
    self._CreateCLActionHistory()
    self.assertEqual({self.cl1_patch1: [reject_action]},
                     self.cl_action_stats.GetTrueRejections())

  def testTrueRejectionsMultipleRejectionsOnPatch(self):
    """Tests that we include all rejection actions on a patch."""
    _, reject_action1 = self._PickupAndRejectPatch(self.cl1_patch1)
    _, reject_action2 = self._PickupAndRejectPatch(self.cl1_patch1)
    self._AppendToHistory(self.cl1_patch2, constants.CL_ACTION_SUBMITTED)
    self._CreateCLActionHistory()
    self.assertEqual({self.cl1_patch1: [reject_action1, reject_action2]},
                     self.cl_action_stats.GetTrueRejections())

  def testTrueRejectionsByCQ(self):
    """A complex test filtering for rejections by the cq.

    For a patch that has been rejected by both the pre-cq and cq, only cq's
    actions should be reported. For a patch that has been rejected by only the
    pre-cq, the rejection should not be included at all.
    """
    _, reject_action1 = self._PickupAndRejectPatch(
        self.cl1_patch1, build_config=self.PRE_CQ_BUILD_CONFIG)
    _, reject_action2 = self._PickupAndRejectPatch(
        self.cl1_patch1, build_config=self.CQ_BUILD_CONFIG)
    self._AppendToHistory(self.cl1_patch2, constants.CL_ACTION_SUBMITTED)
    _, reject_action3 = self._PickupAndRejectPatch(
        self.cl2_patch1, build_config=self.PRE_CQ_BUILD_CONFIG)
    self._AppendToHistory(self.cl2_patch2, constants.CL_ACTION_SUBMITTED)
    self._CreateCLActionHistory()
    self.assertEqual({self.cl1_patch1: [reject_action1, reject_action2],
                      self.cl2_patch1: [reject_action3]},
                     self.cl_action_stats.GetTrueRejections())
    self.assertEqual({self.cl1_patch1: [reject_action2]},
                     self.cl_action_stats.GetTrueRejections(constants.CQ))

  def testFalseRejectionsSkipApplyFailure(self):
    """Test that apply failures are not considered false rejections."""
    self._AppendToHistory(self.cl1_patch1, constants.CL_ACTION_KICKED_OUT)
    self._AppendToHistory(self.cl1_patch1, constants.CL_ACTION_SUBMITTED)
    self._CreateCLActionHistory()
    self.assertEqual({}, self.cl_action_stats.GetTrueRejections())

  def testFalseRejectionMultiplePatchesFalselyRejected(self):
    """Test the case when we reject mulitple patches falsely."""
    _, reject_action1 = self._PickupAndRejectPatch(self.cl1_patch1)
    _, reject_action2 = self._PickupAndRejectPatch(self.cl1_patch1)
    self._AppendToHistory(self.cl1_patch1, constants.CL_ACTION_SUBMITTED)
    _, reject_action3 = self._PickupAndRejectPatch(self.cl1_patch2)
    self._AppendToHistory(self.cl1_patch2, constants.CL_ACTION_SUBMITTED)
    self._CreateCLActionHistory()
    self.assertEqual({self.cl1_patch1: [reject_action1, reject_action2],
                      self.cl1_patch2: [reject_action3]},
                     self.cl_action_stats.GetFalseRejections())

  def testFalseRejectionsByCQ(self):
    """Test that we list CQ spciefic rejections correctly."""
    self._PickupAndRejectPatch(self.cl1_patch1,
                               build_config=self.PRE_CQ_BUILD_CONFIG)
    _, reject_action1 = self._PickupAndRejectPatch(
        self.cl1_patch1, build_config=self.CQ_BUILD_CONFIG)
    self._AppendToHistory(self.cl1_patch1, action=constants.CL_ACTION_SUBMITTED)
    self._CreateCLActionHistory()
    self.assertEqual({self.cl1_patch1: [reject_action1]},
                     self.cl_action_stats.GetFalseRejections(constants.CQ))

  def testFalseRejectionsSkipsBadPreCQRun(self):
    """Test that we don't consider rejections on bad pre-cq buuilds false.

    We batch related CLs together on pre-cq runs. Rejections beause a certain
    pre-cq build failed are considered to not be false because a CL was still to
    blame.
    """
    # Use our own build_ids to tie CLs together.
    bad_build_id = 21
    # This false rejection is due to a bad build.
    self._PickupAndRejectPatch(self.cl1_patch1,
                               build_config=self.PRE_CQ_BUILD_CONFIG,
                               build_id=bad_build_id)
    # This is a true rejection, marking the pre-cq build as a bad build.
    _, reject_action1 = self._PickupAndRejectPatch(
        self.cl2_patch1,
        build_config=self.PRE_CQ_BUILD_CONFIG,
        build_id=bad_build_id)
    self._AppendToHistory(self.cl1_patch1,
                          constants.CL_ACTION_SUBMITTED)
    self._AppendToHistory(self.cl2_patch2,
                          constants.CL_ACTION_SUBMITTED)
    self._CreateCLActionHistory()
    self.assertEqual({self.cl2_patch1: [reject_action1]},
                     self.cl_action_stats.GetTrueRejections())
    self.assertEqual({}, self.cl_action_stats.GetFalseRejections())

  def testFalseRejectionsSkipsBadPreCQAction(self):
    """Test that we skip only the bad pre-cq actions when skipping bad builds.

    If a patch is rejected by a bad pre-cq run, and then rejected again by
    other builds, we should only skip the first action.
    """
    # Use our own build_ids to tie CLs together.
    bad_build_id = 21
    # This false rejection is due to a bad build.
    self._PickupAndRejectPatch(self.cl1_patch1,
                               build_config=self.PRE_CQ_BUILD_CONFIG,
                               build_id=bad_build_id)
    # This is a true rejection, marking the pre-cq build as a bad build.
    _, reject_action1 = self._PickupAndRejectPatch(
        self.cl2_patch1,
        build_config=self.PRE_CQ_BUILD_CONFIG,
        build_id=bad_build_id)
    # This is a valid false rejection.
    _, reject_action2 = self._PickupAndRejectPatch(
        self.cl1_patch1, build_config=self.PRE_CQ_BUILD_CONFIG)
    # This is also a valid false rejection.
    _, reject_action3 = self._PickupAndRejectPatch(
        self.cl1_patch1, build_config=self.CQ_BUILD_CONFIG)
    self._AppendToHistory(self.cl1_patch1,
                          constants.CL_ACTION_SUBMITTED)
    self._AppendToHistory(self.cl2_patch2,
                          constants.CL_ACTION_SUBMITTED)
    self._CreateCLActionHistory()
    self.assertEqual({self.cl2_patch1: [reject_action1]},
                     self.cl_action_stats.GetTrueRejections())
    self.assertEqual({self.cl1_patch1: [reject_action2, reject_action3]},
                     self.cl_action_stats.GetFalseRejections())
    self.assertEqual({self.cl1_patch1: [reject_action2]},
                     self.cl_action_stats.GetFalseRejections(constants.PRE_CQ))
    self.assertEqual({self.cl1_patch1: [reject_action3]},
                     self.cl_action_stats.GetFalseRejections(constants.CQ))

  def testFalseRejectionsMergeConflictByBotType(self):
    """Test the case when one bot has merge conflict.

    If pre-cq falsely rejects a patch, and CQ has a merge conflict, but later
    submits the CL, the false rejection should only show up for pre-cq.
    """
    _, reject_action1 = self._PickupAndRejectPatch(
        self.cl1_patch1,
        build_config=self.PRE_CQ_BUILD_CONFIG)
    self._AppendToHistory(self.cl1_patch1, constants.CL_ACTION_KICKED_OUT,
                          build_config=self.CQ_BUILD_CONFIG)
    self._AppendToHistory(self.cl1_patch1, constants.CL_ACTION_SUBMITTED,
                          build_config=self.CQ_BUILD_CONFIG)
    self._CreateCLActionHistory()
    self.assertEqual({self.cl1_patch1: [reject_action1]},
                     self.cl_action_stats.GetFalseRejections(constants.PRE_CQ))
    self.assertEqual({}, self.cl_action_stats.GetFalseRejections(constants.CQ))

  def testRejectionsPatchSubmittedThenUpdated(self):
    """Test the case when a patch is submitted, then updated."""
    _, reject_action1 = self._PickupAndRejectPatch(self.cl1_patch1)
    self._AppendToHistory(self.cl1_patch1, constants.CL_ACTION_SUBMITTED)
    self._AppendToHistory(self.cl1_patch2, constants.CL_ACTION_PICKED_UP)
    self._CreateCLActionHistory()
    self.assertEqual({}, self.cl_action_stats.GetTrueRejections())
    self.assertEqual({self.cl1_patch1: [reject_action1]},
                     self.cl_action_stats.GetFalseRejections())


class TestGerritChangeTuple(cros_test_lib.TestCase):
  """Tests of basic GerritChangeTuple functionality."""

  def testUnknownHostRaises(self):
    with self.assertRaises(clactions.UnknownGerritHostError):
      clactions.GerritChangeTuple.FromHostAndNumber('foobar-host', 1234)

  def testKnownHosts(self):
    self.assertEqual((31415, True),
                     clactions.GerritChangeTuple.FromHostAndNumber(
                         'gerrit-int.chromium.org', 31415))
    self.assertEqual((31415, True),
                     clactions.GerritChangeTuple.FromHostAndNumber(
                         constants.INTERNAL_GERRIT_HOST, 31415))
    self.assertEqual((31415, False),
                     clactions.GerritChangeTuple.FromHostAndNumber(
                         'gerrit.chromium.org', 31415))
    self.assertEqual((31415, False),
                     clactions.GerritChangeTuple.FromHostAndNumber(
                         constants.EXTERNAL_GERRIT_HOST, 31415))
