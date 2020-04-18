# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing unit tests for generic_builders."""

from __future__ import print_function

import mock

from chromite.cbuildbot.builders import generic_builders
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import parallel
from chromite.lib import cidb
from chromite.lib import fake_cidb
from chromite.lib import results_lib


# pylint: disable=protected-access
class BuilderTest(cros_test_lib.MockTestCase):
  """Test cases for Builder."""

  def test_RunParallelStages(self):
    """test _RunParallelStages."""
    fake_db = fake_cidb.FakeCIDBConnection()
    build_id = fake_db.InsertBuild(
        'test_build', 'waterfall', 1, 'test_build', 'hostname')
    cidb.CIDBConnectionFactory.SetupMockCidb(mock_cidb=fake_db)
    parallel_ex = parallel.UnexpectedException('run parallel exception')
    self.PatchObject(parallel, 'RunParallelSteps', side_effect=parallel_ex)

    results_lib.Results.Record('stage_0', results_lib.Results.SKIPPED)
    fake_db.InsertBuildStage(build_id, 'stage_0',
                             status=constants.BUILDER_STATUS_SKIPPED)
    results_lib.Results.Record('stage_1', results_lib.Results.FORGIVEN)
    fake_db.InsertBuildStage(build_id, 'stage_1',
                             status=constants.BUILDER_STATUS_FORGIVEN)
    results_lib.Results.Record('stage_2', results_lib.Results.SUCCESS)
    fake_db.InsertBuildStage(build_id, 'stage_2',
                             status=constants.BUILDER_STATUS_PASSED)
    # build stage status for stage_3 is failed but no entry in failureTable
    fake_db.InsertBuildStage(build_id, 'stage_3',
                             status=constants.BUILDER_STATUS_FAILED)
    # build stage status for stage_4 is not in completed status
    fake_db.InsertBuildStage(build_id, 'stage_4',
                             status=constants.BUILDER_STATUS_INFLIGHT)
    # no build stage status found for stage_5

    stage_objs = []
    for i in range(0, 6):
      stage_mock = mock.Mock()
      stage_mock.GetStageNames.return_value = ['stage_%s' % i]
      stage_mock.GetBuildStageIDs.return_value = [i]
      stage_mock.StageNamePrefix.return_value = 'stage_prefix'
      stage_objs.append(stage_mock)

    self.assertRaises(parallel.UnexpectedException,
                      generic_builders.Builder._RunParallelStages, stage_objs)
    self.assertTrue(results_lib.Results.StageHasResults('stage_3'))
    self.assertTrue(results_lib.Results.StageHasResults('stage_4'))
    for r in results_lib.Results.Get():
      if r.name in ('stage_3', 'stage_4'):
        self.assertEqual(r.prefix, 'stage_prefix')

    for i in range(0, 3):
      self.assertFalse(fake_db.HasFailureMsgForStage(i))
    for i in range(3, 6):
      self.assertTrue(fake_db.HasFailureMsgForStage(i))

    self.assertEqual(fake_db.GetBuildStage(0)['status'],
                     constants.BUILDER_STATUS_SKIPPED)
    self.assertEqual(fake_db.GetBuildStage(1)['status'],
                     constants.BUILDER_STATUS_FORGIVEN)
    self.assertEqual(fake_db.GetBuildStage(2)['status'],
                     constants.BUILDER_STATUS_PASSED)
    self.assertEqual(fake_db.GetBuildStage(3)['status'],
                     constants.BUILDER_STATUS_FAILED)
    self.assertEqual(fake_db.GetBuildStage(4)['status'],
                     constants.BUILDER_STATUS_FAILED)
    self.assertIsNone(fake_db.GetBuildStage(5))
