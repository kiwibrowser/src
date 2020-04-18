# -*- coding: utf-8 -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for summarize_build_stats."""

from __future__ import print_function

import datetime
import mock
import random

from chromite.lib.const import waterfall
from chromite.lib import clactions
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import fake_cidb
from chromite.scripts import summarize_build_stats
from chromite.lib import metadata_lib
from chromite.lib import constants


CQ = constants.CQ
PRE_CQ = constants.PRE_CQ


class TestCLActionLogic(cros_test_lib.TestCase):
  """Ensures that CL action analysis logic is correct."""

  def setUp(self):
    self.fake_db = fake_cidb.FakeCIDBConnection()

  def _PopulateFakeCidbWithTestData(self, cq):
    """Generate test data and insert it in the the fake cidb object.

    Args:
      cq: Whether this is a CQ run. If False, this is a Pre-CQ run.
    """
    # Mock patches for test data.
    c1p1 = metadata_lib.GerritPatchTuple(1, 1, False)
    c2p1 = metadata_lib.GerritPatchTuple(2, 1, True)
    c2p2 = metadata_lib.GerritPatchTuple(2, 2, True)
    c3p1 = metadata_lib.GerritPatchTuple(3, 1, True)
    c3p2 = metadata_lib.GerritPatchTuple(3, 2, True)
    c4p1 = metadata_lib.GerritPatchTuple(4, 1, True)
    c4p2 = metadata_lib.GerritPatchTuple(4, 2, True)

    # Mock builder status dictionaries
    passed_status = {'status': constants.BUILDER_STATUS_PASSED}
    failed_status = {'status': constants.BUILDER_STATUS_FAILED}

    t = datetime.datetime.now()
    delta = datetime.timedelta(hours=1)
    bot_config = (constants.CQ_MASTER if cq
                  else constants.PRE_CQ_DEFAULT_CONFIGS[0])

    # pylint: disable=bad-continuation
    test_metadata = [
      # Build 1 picks up no patches.
      metadata_lib.CBuildbotMetadata(
          ).UpdateWithDict({'build-number' : 1,
                            'bot-config' : bot_config,
                            'results' : [],
                            'status' : passed_status}),
      # Build 2 picks up c1p1 and does nothing.
      metadata_lib.CBuildbotMetadata(
          ).UpdateWithDict({'build-number' : 2,
                            'bot-config' : bot_config,
                            'results' : [],
                            'status' : failed_status,
                            'changes': [c1p1._asdict()]}
          ).RecordCLAction(c1p1, constants.CL_ACTION_PICKED_UP, t+delta),
      # Build 3 picks up c1p1 and c2p1 and rejects both.
      # c3p1 is not included in the run because it fails to apply.
      metadata_lib.CBuildbotMetadata(
          ).UpdateWithDict({'build-number' : 3,
                            'bot-config' : bot_config,
                            'results' : [],
                            'status' : failed_status,
                            'changes': [c1p1._asdict(),
                                        c2p1._asdict()]}
          ).RecordCLAction(c1p1, constants.CL_ACTION_PICKED_UP, t+delta
          ).RecordCLAction(c2p1, constants.CL_ACTION_PICKED_UP, t+delta
          ).RecordCLAction(c1p1, constants.CL_ACTION_KICKED_OUT, t+delta
          ).RecordCLAction(c2p1, constants.CL_ACTION_KICKED_OUT, t+delta
          ).RecordCLAction(c3p1, constants.CL_ACTION_KICKED_OUT, t+delta),
      # Build 4 picks up c4p1 and does nothing with it.
      # c4p2 isn't picked up because it fails to apply.
      metadata_lib.CBuildbotMetadata(
          ).UpdateWithDict({'build-number' : 3,
                            'bot-config' : bot_config,
                            'results' : [],
                            'status' : failed_status,
                            'changes': [c4p1._asdict()]}
          ).RecordCLAction(c4p1, constants.CL_ACTION_PICKED_UP, t+delta
          ).RecordCLAction(c4p2, constants.CL_ACTION_KICKED_OUT, t+delta),
    ]
    if cq:
      test_metadata += [
        # Build 4 picks up c1p1, c2p2, c3p2, c4p1 and submits the first three.
        # c4p2 is submitted without being tested.
        # So  c1p1 should be detected as a 1-time rejected good patch,
        # and c2p1 should be detected as a possibly bad patch.
        metadata_lib.CBuildbotMetadata(
            ).UpdateWithDict({'build-number' : 4,
                              'bot-config' : bot_config,
                              'results' : [],
                              'status' : passed_status,
                              'changes': [c1p1._asdict(),
                                          c2p2._asdict()]}
            ).RecordCLAction(c1p1, constants.CL_ACTION_PICKED_UP, t+delta
            ).RecordCLAction(c2p2, constants.CL_ACTION_PICKED_UP, t+delta
            ).RecordCLAction(c3p2, constants.CL_ACTION_PICKED_UP, t+delta
            ).RecordCLAction(c4p1, constants.CL_ACTION_PICKED_UP, t+delta
            ).RecordCLAction(c1p1, constants.CL_ACTION_SUBMITTED, t+delta
            ).RecordCLAction(c2p2, constants.CL_ACTION_SUBMITTED, t+delta
            ).RecordCLAction(c3p2, constants.CL_ACTION_SUBMITTED, t+delta
            ).RecordCLAction(c4p2, constants.CL_ACTION_SUBMITTED, t+delta),
      ]
    else:
      test_metadata += [
        metadata_lib.CBuildbotMetadata(
            ).UpdateWithDict({'build-number' : 5,
                              'bot-config' : bot_config,
                              'results' : [],
                              'status' : failed_status,
                              'changes': [c4p1._asdict()]}
            ).RecordCLAction(c4p1, constants.CL_ACTION_PICKED_UP, t+delta
            ).RecordCLAction(c4p1, constants.CL_ACTION_KICKED_OUT, t+delta),
        metadata_lib.CBuildbotMetadata(
            ).UpdateWithDict({'build-number' : 6,
                              'bot-config' : bot_config,
                              'results' : [],
                              'status' : failed_status,
                              'changes': [c4p1._asdict()]}
            ).RecordCLAction(c1p1, constants.CL_ACTION_PICKED_UP, t+delta
            ).RecordCLAction(c1p1, constants.CL_ACTION_KICKED_OUT, t+delta)
      ]
    # pylint: enable=bad-continuation

    # test_metadata should not be guaranteed to be ordered by build number
    # so shuffle it, but use the same seed each time so that unit test is
    # deterministic.
    random.seed(0)
    random.shuffle(test_metadata)

    for m in test_metadata:
      build_id = self.fake_db.InsertBuild(
          m.GetValue('bot-config'), waterfall.WATERFALL_INTERNAL,
          m.GetValue('build-number'), m.GetValue('bot-config'),
          'bot-hostname')
      m.UpdateWithDict({'build_id': build_id})
      actions = []
      for action_metadata in m.GetDict()['cl_actions']:
        actions.append(clactions.CLAction.FromMetadataEntry(action_metadata))
      self.fake_db.InsertCLActions(build_id, actions)

  def testCLStatsEngineSummary(self):
    with cros_build_lib.ContextManagerStack() as stack:
      self._PopulateFakeCidbWithTestData(cq=False)
      self._PopulateFakeCidbWithTestData(cq=True)
      stack.Add(mock.patch.object, summarize_build_stats.CLStatsEngine,
                'GatherBuildAnnotations')
      cl_stats = summarize_build_stats.CLStatsEngine(self.fake_db)
      cl_stats.Gather(datetime.date.today(), datetime.date.today())
      cl_stats.reasons = {1: '', 2: '', 3: constants.FAILURE_CATEGORY_BAD_CL,
                          4: constants.FAILURE_CATEGORY_BAD_CL}
      cl_stats.blames = {1: '', 2: '', 3: 'crosreview.com/1',
                         4: 'crosreview.com/1'}
      summary = cl_stats.Summarize('cq')

      expected = {
          'mean_good_patch_rejections': 0.5,
          'unique_patches': 7,
          'unique_blames_change_count': 0,
          'total_cl_actions': 28,
          'good_patch_rejection_breakdown': [(0, 3), (1, 0), (2, 1)],
          'good_patch_rejection_count': {CQ: 1, PRE_CQ: 1},
          'good_patch_rejections': 2,
          'false_rejection_rate': {CQ: 20., PRE_CQ: 20., 'combined': 100. / 3},
          'long_pole_slave_counts': {},
          'submitted_patches': 4,
          'submit_fails': 0,
          'unique_cls': 4,
          'median_handling_time': -1,  # This will be ignored in comparison
          'patch_handling_time': -1,  # This will be ignored in comparison
          'bad_cl_candidates': {
              CQ: [metadata_lib.GerritChangeTuple(gerrit_number=2,
                                                  internal=True)],
              PRE_CQ: [metadata_lib.GerritChangeTuple(gerrit_number=4,
                                                      internal=True),
                       metadata_lib.GerritChangeTuple(gerrit_number=2,
                                                      internal=True)],
          },
          'rejections': 10,
          'total_builds': 5,
          'first_build_num': 1,
          'last_build_num': 4,
          'last_build_id': mock.ANY,
          'cl_handling_time_50': 0.0,
          'cl_handling_time_90': 0.0,
          'cq_time_50': 0.0,
          'cq_time_90': 0.0,
          'wait_time_50': 0.0,
          'wait_time_90': 0.0,
          'slowest_cq_slaves': [],
          'patch_flake_rejections': 1,
          'bad_cl_precq_rejected': 2,
          'false_rejection_total': 2,
          'false_rejection_pre_cq': 1,
          'false_rejection_cq': 1,
          #'build_blame_counts': {},
          #'patch_blame_counts': {},
          }
      # Ignore handling times in comparison, since these are not fully
      # reproducible from run to run of the unit test.
      summary['median_handling_time'] = expected['median_handling_time']
      summary['patch_handling_time'] = expected['patch_handling_time']
      self.maxDiff = None
      self.assertDictContainsSubset(expected, summary)
      #self.assertEqual(expected, summary)

  def testProcessBlameString(self):
    """Tests that bug and CL links are correctly parsed."""
    blame = ('some words then crbug.com/1234, then other junk and '
             'https://bugs.chromium.org/p/chromium/issues/detail?id=4321 '
             'then some stuff and other stuff and b/2345 and also '
             'https://b.corp.google.com/issue?id=5432&query=5432 '
             'and then some crosreview.com/3456 or some '
             'https://chromium-review.googlesource.com/#/c/6543/ and '
             'then crosreview.com/i/9876 followed by '
             'https://chrome-internal-review.googlesource.com/#/c/6789/ '
             'blah https://gutsv3.corp.google.com/#ticket/1234 t/4321 and '
             'https://bugs.chromium.org/p/chromium/issues/detail?id=522555#c58'
             ' and https://codereview.chromium.org/1216423002 ')
    expected = ['crbug.com/1234',
                'crbug.com/4321',
                'b/2345',
                'b/5432',
                'crosreview.com/3456',
                'crosreview.com/6543',
                'crosreview.com/i/9876',
                'crosreview.com/i/6789',
                't/1234',
                't/4321',
                'crbug.com/522555',
                'codereview.chromium.org/1216423002']
    self.assertEqual(
        summarize_build_stats.CLStatsEngine.ProcessBlameString(blame),
        expected)
