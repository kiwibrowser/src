# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for scheduler stages."""

from __future__ import print_function

import mock

from chromite.cbuildbot.stages import generic_stages
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.cbuildbot.stages import scheduler_stages
from chromite.lib.const import waterfall
from chromite.lib import auth
from chromite.lib import buildbucket_lib
from chromite.lib import build_requests
from chromite.lib import cidb
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import fake_cidb


class HelperMethodTests(cros_test_lib.MockTestCase):
  """Test cases for helper methods in scheduler_stages."""

  def testBuilderName(self):
    """Test BuilderName."""
    builder_name = scheduler_stages.BuilderName(
        'parrot-release', waterfall.WATERFALL_INTERNAL, 'master-release')
    self.assertEqual(builder_name, 'parrot-release')

    builder_name = scheduler_stages.BuilderName(
        'parrot-release', waterfall.WATERFALL_RELEASE,
        'master-release release-R62-9901.B')
    self.assertEqual(builder_name, 'parrot-release release-R62-9901.B')


class ScheduleSalvesStageTest(generic_stages_unittest.AbstractStageTestCase):
  """Unit tests for ScheduleSalvesStage."""

  BOT_ID = 'master-paladin'

  def setUp(self):
    self.PatchObject(buildbucket_lib, 'GetServiceAccount',
                     return_value='server_account')
    self.PatchObject(auth.AuthorizedHttp, '__init__',
                     return_value=None)
    self.PatchObject(buildbucket_lib.BuildbucketClient,
                     '_GetHost',
                     return_value=buildbucket_lib.BUILDBUCKET_TEST_HOST)
    self.PatchObject(buildbucket_lib.BuildbucketClient,
                     'SendBuildbucketRequest',
                     return_value=None)
    # Create and set up a fake cidb instance.
    self.fake_db = fake_cidb.FakeCIDBConnection()
    cidb.CIDBConnectionFactory.SetupMockCidb(self.fake_db)

    self.sync_stage = mock.Mock()
    self._Prepare()

  def ConstructStage(self):
    return scheduler_stages.ScheduleSlavesStage(self._run, self.sync_stage)

  def testPerformStage(self):
    """Test PerformStage."""
    stage = self.ConstructStage()
    self.PatchObject(buildbucket_lib.BuildbucketClient,
                     '_GetHost',
                     return_value=buildbucket_lib.BUILDBUCKET_TEST_HOST)

    stage.PerformStage()

  def testScheduleImportantSlaveBuildsFailure(self):
    """Test ScheduleSlaveBuilds with important slave failures."""
    stage = self.ConstructStage()
    self.PatchObject(scheduler_stages.ScheduleSlavesStage,
                     'PostSlaveBuildToBuildbucket',
                     side_effect=buildbucket_lib.BuildbucketResponseException)

    slave_config_map_1 = {
        'slave_external': config_lib.BuildConfig(
            important=True, active_waterfall=waterfall.WATERFALL_EXTERNAL)}
    self.PatchObject(generic_stages.BuilderStage, '_GetSlaveConfigMap',
                     return_value=slave_config_map_1)
    self.assertRaises(
        buildbucket_lib.BuildbucketResponseException,
        stage.ScheduleSlaveBuildsViaBuildbucket,
        important_only=False, dryrun=True)

    slave_config_map_2 = {
        'slave_internal': config_lib.BuildConfig(
            important=True, active_waterfall=waterfall.WATERFALL_INTERNAL)}
    self.PatchObject(generic_stages.BuilderStage, '_GetSlaveConfigMap',
                     return_value=slave_config_map_2)
    self.assertRaises(
        buildbucket_lib.BuildbucketResponseException,
        stage.ScheduleSlaveBuildsViaBuildbucket,
        important_only=False, dryrun=True)

  def testScheduleUnimportantSlaveBuildsFailure(self):
    """Test ScheduleSlaveBuilds with unimportant slave failures."""
    stage = self.ConstructStage()
    self.PatchObject(scheduler_stages.ScheduleSlavesStage,
                     'PostSlaveBuildToBuildbucket',
                     side_effect=buildbucket_lib.BuildbucketResponseException)

    slave_config_map = {
        'slave_external': config_lib.BuildConfig(
            important=False, active_waterfall=waterfall.WATERFALL_EXTERNAL),
        'slave_internal': config_lib.BuildConfig(
            important=False, active_waterfall=waterfall.WATERFALL_INTERNAL),}
    self.PatchObject(generic_stages.BuilderStage, '_GetSlaveConfigMap',
                     return_value=slave_config_map)
    stage.ScheduleSlaveBuildsViaBuildbucket(important_only=False, dryrun=True)

    scheduled_slaves = self._run.attrs.metadata.GetValue(
        constants.METADATA_SCHEDULED_IMPORTANT_SLAVES)
    self.assertEqual(len(scheduled_slaves), 0)
    unscheduled_slaves = self._run.attrs.metadata.GetValue(
        constants.METADATA_UNSCHEDULED_SLAVES)
    self.assertEqual(len(unscheduled_slaves), 2)

  def testScheduleSlaveBuildsFailure(self):
    """Test ScheduleSlaveBuilds with mixed slave failures."""
    stage = self.ConstructStage()
    self.PatchObject(scheduler_stages.ScheduleSlavesStage,
                     'PostSlaveBuildToBuildbucket',
                     side_effect=buildbucket_lib.BuildbucketResponseException)

    slave_config_map = {
        'slave_external': config_lib.BuildConfig(
            important=False, active_waterfall=waterfall.WATERFALL_EXTERNAL),
        'slave_internal': config_lib.BuildConfig(
            important=True, active_waterfall=waterfall.WATERFALL_INTERNAL)}
    self.PatchObject(generic_stages.BuilderStage, '_GetSlaveConfigMap',
                     return_value=slave_config_map)
    self.assertRaises(
        buildbucket_lib.BuildbucketResponseException,
        stage.ScheduleSlaveBuildsViaBuildbucket,
        important_only=False, dryrun=True)

  def testScheduleSlaveBuildsSuccess(self):
    """Test ScheduleSlaveBuilds with success."""
    stage = self.ConstructStage()

    self.PatchObject(scheduler_stages.ScheduleSlavesStage,
                     'PostSlaveBuildToBuildbucket',
                     return_value=('buildbucket_id', None))

    slave_config_map = {
        'slave_external': config_lib.BuildConfig(
            important=False, active_waterfall=waterfall.WATERFALL_EXTERNAL),
        'slave_internal': config_lib.BuildConfig(
            important=True, active_waterfall=waterfall.WATERFALL_INTERNAL)}
    self.PatchObject(generic_stages.BuilderStage, '_GetSlaveConfigMap',
                     return_value=slave_config_map)

    stage.ScheduleSlaveBuildsViaBuildbucket(important_only=False, dryrun=True)

    scheduled_slaves = self._run.attrs.metadata.GetValue(
        constants.METADATA_SCHEDULED_IMPORTANT_SLAVES)
    self.assertEqual(len(scheduled_slaves), 1)
    unscheduled_slaves = self._run.attrs.metadata.GetValue(
        constants.METADATA_UNSCHEDULED_SLAVES)
    self.assertEqual(len(unscheduled_slaves), 0)

  def testNoScheduledSlaveBuilds(self):
    """Test no slave builds are scheduled."""
    stage = self.ConstructStage()
    schedule_mock = self.PatchObject(
        scheduler_stages.ScheduleSlavesStage,
        'ScheduleSlaveBuildsViaBuildbucket')
    self.sync_stage.pool.HasPickedUpCLs.return_value = False

    stage.PerformStage()
    self.assertFalse(schedule_mock.called)

  def testScheduleSlaveBuildsWithCLs(self):
    """Test no slave builds are scheduled."""
    stage = self.ConstructStage()
    schedule_mock = self.PatchObject(
        scheduler_stages.ScheduleSlavesStage,
        'ScheduleSlaveBuildsViaBuildbucket')
    self.sync_stage.pool.HasPickedUpCLs.return_value = True

    stage.PerformStage()
    self.assertTrue(schedule_mock.called)

  def testPostSlaveBuildToBuildbucketOnSingleBoardBuild(self):
    """Test PostSlaveBuildToBuildbucket on builds with a single board."""
    content = {'build':{'id':'bb_id_1', 'created_ts':1}}
    self.PatchObject(buildbucket_lib.BuildbucketClient, 'PutBuildRequest',
                     return_value=content)
    slave_config = config_lib.BuildConfig(
        name='slave',
        important=True, active_waterfall=waterfall.WATERFALL_INTERNAL,
        display_label='cq',
        boards=['board_A'], build_type='paladin')

    stage = self.ConstructStage()
    buildbucket_id, created_ts = stage.PostSlaveBuildToBuildbucket(
        'slave', slave_config, 0, 'master_bb_id', 'buildset_tag', dryrun=True)

    self.assertEqual(buildbucket_id, 'bb_id_1')
    self.assertEqual(created_ts, 1)

  def testPostSlaveBuildToBuildbucketOnMultiBoardsBuild(self):
    """Test PostSlaveBuildToBuildbucket on builds with muiltiple boards."""
    content = {'build':{'id':'bb_id_1', 'created_ts':1}}
    self.PatchObject(buildbucket_lib.BuildbucketClient, 'PutBuildRequest',
                     return_value=content)
    slave_config = config_lib.BuildConfig(
        name='slave',
        important=True, active_waterfall=waterfall.WATERFALL_INTERNAL,
        display_label='cq',
        boards=['board_A', 'board_B'], build_type='paladin')

    stage = self.ConstructStage()
    buildbucket_id, created_ts = stage.PostSlaveBuildToBuildbucket(
        'slave', slave_config, 0, 'master_bb_id', 'buildset_tag', dryrun=True)

    self.assertEqual(buildbucket_id, 'bb_id_1')
    self.assertEqual(created_ts, 1)

  # pylint: disable=protected-access
  def testScheduleSlaveBuildsViaBuildbucket(self):
    """Test ScheduleSlaveBuildsViaBuildbucket."""
    self.PatchObject(scheduler_stages.ScheduleSlavesStage,
                     'PostSlaveBuildToBuildbucket',
                     side_effect=(('bb_id_1', None), ('bb_id_2', None)))
    slave_config_map = {
        'important_external': config_lib.BuildConfig(
            important=True, active_waterfall=waterfall.WATERFALL_EXTERNAL),
        'experimental_external': config_lib.BuildConfig(
            important=False, active_waterfall=waterfall.WATERFALL_EXTERNAL)}
    self.PatchObject(generic_stages.BuilderStage, '_GetSlaveConfigMap',
                     return_value=slave_config_map)

    stage = self.ConstructStage()
    stage.ScheduleSlaveBuildsViaBuildbucket(important_only=False, dryrun=True)

    results = self.fake_db.GetBuildRequestsForBuildConfig('important_external')
    self.assertEqual(len(results), 1)
    self.assertEqual(results[0].request_build_config, 'important_external')
    self.assertEqual(results[0].request_buildbucket_id, 'bb_id_1')
    self.assertEqual(results[0].request_reason,
                     build_requests.REASON_IMPORTANT_CQ_SLAVE)

    results = self.fake_db.GetBuildRequestsForBuildConfig(
        'experimental_external')
    self.assertEqual(len(results), 1)
    self.assertEqual(results[0].request_build_config, 'experimental_external')
    self.assertEqual(results[0].request_buildbucket_id, 'bb_id_2')
    self.assertEqual(results[0].request_reason,
                     build_requests.REASON_EXPERIMENTAL_CQ_SLAVE)

    scheduled_important_builds = stage._run.attrs.metadata.GetValue(
        constants.METADATA_SCHEDULED_IMPORTANT_SLAVES)
    self.assertEqual(len(scheduled_important_builds), 1)
    self.assertEqual(scheduled_important_builds[0],
                     ('important_external', 'bb_id_1', None))

    scheduled_experimental_builds = stage._run.attrs.metadata.GetValue(
        constants.METADATA_SCHEDULED_EXPERIMENTAL_SLAVES)
    self.assertEqual(len(scheduled_experimental_builds), 1)
    self.assertEqual(scheduled_experimental_builds[0],
                     ('experimental_external', 'bb_id_2', None))
