# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing unit tests for build_status."""

from __future__ import print_function

import datetime
import mock
import time

from chromite.cbuildbot import build_status
from chromite.cbuildbot import relevant_changes
from chromite.cbuildbot import validation_pool_unittest
from chromite.lib.const import waterfall
from chromite.lib import buildbucket_lib
from chromite.lib import builder_status_lib
from chromite.lib import build_requests
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import fake_cidb
from chromite.lib import metadata_lib
from chromite.lib import patch_unittest
from chromite.lib import tree_status


site_config = config_lib.GetConfig()


# pylint: disable=protected-access
class BuildbucketInfos(object):
  """Helper methods to build BuildbucketInfo."""

  @staticmethod
  def GetScheduledBuild(bb_id='scheduled_id_1', retry=0, url=None):
    return buildbucket_lib.BuildbucketInfo(
        buildbucket_id=bb_id,
        retry=retry,
        created_ts=1,
        status=constants.BUILDBUCKET_BUILDER_STATUS_SCHEDULED,
        result=None,
        url=url
    )

  @staticmethod
  def GetStartedBuild(bb_id='started_id_1', retry=0, url=None):
    return buildbucket_lib.BuildbucketInfo(
        buildbucket_id=bb_id,
        retry=retry,
        created_ts=1,
        status=constants.BUILDBUCKET_BUILDER_STATUS_STARTED,
        result=None,
        url=url
    )

  @staticmethod
  def GetSuccessBuild(bb_id='success_id_1', retry=0, url=None):
    return buildbucket_lib.BuildbucketInfo(
        buildbucket_id=bb_id,
        retry=retry,
        created_ts=1,
        status=constants.BUILDBUCKET_BUILDER_STATUS_COMPLETED,
        result=constants.BUILDBUCKET_BUILDER_RESULT_SUCCESS,
        url=url
    )

  @staticmethod
  def GetFailureBuild(bb_id='failure_id_1', retry=0, url=None):
    return buildbucket_lib.BuildbucketInfo(
        buildbucket_id=bb_id,
        retry=retry,
        created_ts=1,
        status=constants.BUILDBUCKET_BUILDER_STATUS_COMPLETED,
        result=constants.BUILDBUCKET_BUILDER_RESULT_FAILURE,
        url=url
    )

  @staticmethod
  def GetCanceledBuild(bb_id='canceled_id_1', retry=0, url=None):
    return buildbucket_lib.BuildbucketInfo(
        buildbucket_id=bb_id,
        retry=retry,
        created_ts=1,
        status=constants.BUILDBUCKET_BUILDER_STATUS_COMPLETED,
        result=constants.BUILDBUCKET_BUILDER_RESULT_CANCELED,
        url=url
    )

  @staticmethod
  def GetMissingBuild(bb_id='missing_id_1', retry=0, url=None):
    return buildbucket_lib.BuildbucketInfo(
        buildbucket_id=bb_id,
        retry=retry,
        created_ts=1,
        status=None,
        result=None,
        url=url
    )

  @staticmethod
  def GetFullBuildbucketInfoDict(exclude_builds=None):
    buildbucket_info_dict = {
        'scheduled': BuildbucketInfos.GetScheduledBuild(),
        'started': BuildbucketInfos.GetStartedBuild(),
        'completed_success': BuildbucketInfos.GetSuccessBuild(),
        'completed_failure': BuildbucketInfos.GetFailureBuild(),
        'completed_canceled': BuildbucketInfos.GetCanceledBuild()
    }

    if exclude_builds:
      for exclude_build in exclude_builds:
        buildbucket_info_dict.pop(exclude_build, None)

    return buildbucket_info_dict


class CIDBStatusInfos(object):
  """Helper methods to build CIDBStatusInfo."""

  @staticmethod
  def GetInflightBuild(build_id=1, build_number=1):
    return builder_status_lib.CIDBStatusInfo(
        build_id=build_id,
        status=constants.BUILDER_STATUS_INFLIGHT,
        build_number=build_number)

  @staticmethod
  def GetPassedBuild(build_id=2, build_number=2):
    return builder_status_lib.CIDBStatusInfo(
        build_id=build_id,
        status=constants.BUILDER_STATUS_PASSED,
        build_number=build_number)

  @staticmethod
  def GetFailedBuild(build_id=3, build_number=3):
    return builder_status_lib.CIDBStatusInfo(
        build_id=build_id,
        status=constants.BUILDER_STATUS_FAILED,
        build_number=build_number)

  @staticmethod
  def GetPlannedBuild(build_id=4, build_number=4):
    return builder_status_lib.CIDBStatusInfo(
        build_id=build_id,
        status=constants.BUILDER_STATUS_PLANNED,
        build_number=build_number)

  @staticmethod
  def GetForgivenBuild(build_id=5, build_number=5):
    return builder_status_lib.CIDBStatusInfo(
        build_id=build_id,
        status=constants.BUILDER_STATUS_FORGIVEN,
        build_number=build_number)

  @staticmethod
  def GetAbortedBuild(build_id=6, build_number=6):
    return builder_status_lib.CIDBStatusInfo(
        build_id=build_id,
        status=constants.BUILDER_STATUS_ABORTED,
        build_number=build_number)

  @staticmethod
  def GetMissingBuild(build_id=7, build_number=7):
    return builder_status_lib.CIDBStatusInfo(
        build_id=build_id,
        status=constants.BUILDER_STATUS_MISSING,
        build_number=build_number)

  @staticmethod
  def GetSkippedBuild(build_id=8, build_number=8):
    return builder_status_lib.CIDBStatusInfo(
        build_id=build_id,
        status=constants.BUILDER_STATUS_SKIPPED,
        build_number=build_number)

  @staticmethod
  def GetFullCIDBStatusInfo(exclude_builds=None):
    cidb_status = {
        'started': CIDBStatusInfos.GetInflightBuild(build_id=1),
        'completed_success': CIDBStatusInfos.GetPassedBuild(build_id=2),
        'completed_failure': CIDBStatusInfos.GetFailedBuild(build_id=3),
        'completed_canceled': CIDBStatusInfos.GetInflightBuild(build_id=4)
    }

    if exclude_builds:
      for exclude_build in exclude_builds:
        cidb_status.pop(exclude_build, None)

    return cidb_status


class SlaveStatusTest(cros_test_lib.MockTestCase):
  """Test methods testing methods in SlaveStatus class."""

  def setUp(self):
    self.time_now = datetime.datetime.now()
    self.master_build_id = 0
    self.master_test_config = config_lib.BuildConfig(
        name='master-test', master=True,
        active_waterfall=waterfall.WATERFALL_INTERNAL)
    self.master_cq_config = site_config['master-paladin']
    self.master_canary_config = site_config['master-release']
    self.metadata = metadata_lib.CBuildbotMetadata()
    self.db = fake_cidb.FakeCIDBConnection()
    self.buildbucket_client = mock.Mock()
    # TODO(nxia): crbug.com/791592, remove the hack after switching all master
    # builds to use Buildbucket to schedule slave builds.
    self.PatchObject(config_lib, 'UseBuildbucketScheduler', return_value=True)
    self.PatchObject(tree_status, 'GetExperimentalBuilders', return_value=[])
    self._patch_factory = patch_unittest.MockPatchFactory()

  def _GetSlaveStatus(self, start_time=None, builders_array=None,
                      master_build_id=None, db=None, config=None,
                      metadata=None, buildbucket_client=None, version=None,
                      pool=None, dry_run=True):
    if start_time is None:
      start_time = self.time_now
    if builders_array is None:
      builders_array = []
    if master_build_id is None:
      master_build_id = self.master_build_id
    if db is None:
      db = self.db
    if metadata is None:
      metadata = self.metadata
    if buildbucket_client is None:
      buildbucket_client = self.buildbucket_client

    return build_status.SlaveStatus(
        start_time, builders_array, master_build_id, db,
        config=config,
        metadata=metadata,
        buildbucket_client=buildbucket_client,
        version=version,
        pool=pool,
        dry_run=dry_run)

  def _Mock_GetSlaveStatusesFromCIDB(self, cidb_status=None):
    return self.PatchObject(build_status.SlaveStatus,
                            '_GetNewSlaveCIDBStatusInfo',
                            return_value=cidb_status)

  def _MockGetAllSlaveCIDBStatusInfo(self, cidb_status=None):
    return self.PatchObject(builder_status_lib.SlaveBuilderStatus,
                            'GetAllSlaveCIDBStatusInfo',
                            return_value=cidb_status)

  def _Mock_GetSlaveStatusesFromBuildbucket(self, buildbucket_info_dict=None,
                                            mock_GetBuildInfoDict=True):
    self.PatchObject(builder_status_lib.SlaveBuilderStatus,
                     'GetAllSlaveBuildbucketInfo')
    self.PatchObject(build_status.SlaveStatus,
                     '_GetNewSlaveBuildbucketInfo',
                     return_value=buildbucket_info_dict)
    if mock_GetBuildInfoDict:
      self._MockGetBuildInfoDict(buildbucket_info_dict)

  def _MockGetBuildInfoDict(self, buildbucket_info_dict):
    self.PatchObject(buildbucket_lib, 'GetBuildInfoDict',
                     return_value=buildbucket_info_dict)

  def _MockGetAllSlaveBuildbucketInfo(self, buildbucket_info=mock.Mock()):
    self.PatchObject(builder_status_lib.SlaveBuilderStatus,
                     'GetAllSlaveBuildbucketInfo',
                     return_value=buildbucket_info)

  def _Mock_GetRetriableBuilds(self, builds=None):
    return self.PatchObject(build_status.SlaveStatus,
                            '_GetRetriableBuilds',
                            return_value=builds)

  def _Mock_RetryBuilds(self, builds=None):
    if builds is None:
      builds = set()
    self.PatchObject(build_status.SlaveStatus, '_RetryBuilds',
                     return_value=builds)

  def _GetFullBuildConfigs(self, exclude_builds=None):
    build_config_list = ['scheduled', 'started', 'completed_success',
                         'completed_failure', 'completed_canceled']

    if exclude_builds:
      for exclude_build in exclude_builds:
        build_config_list.remove(exclude_build)

    return build_config_list

  def _GetCompletedBuildInfoDict(self):
    return {
        'completed_success': BuildbucketInfos.GetSuccessBuild(),
        'completed_failure': BuildbucketInfos.GetFailureBuild(),
        'completed_canceled': BuildbucketInfos.GetCanceledBuild()
    }

  def _GetCompletedAllSet(self):
    return set(['completed_success',
                'completed_failure',
                'completed_canceled'])

  def testGetSlaveStatusWithValidationPool(self):
    """Test build SlaveStatus with ValidationPool."""
    patch_mock = self.StartPatcher(
        validation_pool_unittest.MockPatchSeries())
    p = self._patch_factory.GetPatches(how_many=3)
    pool = validation_pool_unittest.MakePool(applied=p)

    patch_mock.SetGerritDependencies(p[0], [])
    patch_mock.SetGerritDependencies(p[1], [])
    patch_mock.SetGerritDependencies(p[2], [])

    patch_mock.SetCQDependencies(p[1], [p[0]])
    patch_mock.SetCQDependencies(p[2], [p[0]])

    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config,
        pool=pool)

    expected_map = {
        p[0]: {p[1], p[2]},
    }

    self.assertDictEqual(expected_map, slave_status.dependency_map)

  def testGetExpectedBuilders(self):
    """Tests _GetExpectedBuilders does not return experimental builders."""
    slave_status = self._GetSlaveStatus(builders_array=['build1', 'build2'])
    self.assertItemsEqual(slave_status._GetExpectedBuilders(),
                          ['build1', 'build2'])

    self.metadata.UpdateWithDict({
        constants.METADATA_EXPERIMENTAL_BUILDERS: ['build1']
    })
    self.assertItemsEqual(slave_status._GetExpectedBuilders(), ['build2'])

  def testGetMissingBuilds(self):
    """Tests GetMissingBuilds returns the missing builders."""
    cidb_status = {
        'build1': CIDBStatusInfos.GetFailedBuild(),
        'build2': CIDBStatusInfos.GetInflightBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    slave_status = self._GetSlaveStatus(
        builders_array=['build1', 'build2', 'missing_builder'])

    self.assertEqual(slave_status._GetMissingBuilds(), set(['missing_builder']))

  def testGetMissingBuildsWithBuildbucket(self):
    """Tests GetMissingBuilds returns the missing builders with Buildbucket."""
    cidb_status = {
        'started': CIDBStatusInfos.GetInflightBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'scheduled': BuildbucketInfos.GetScheduledBuild(),
        'started': BuildbucketInfos.GetStartedBuild(),
        'missing': BuildbucketInfos.GetMissingBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    slave_status = self._GetSlaveStatus(
        builders_array=['scheduled', 'started', 'missing'],
        config=self.master_cq_config)

    self.assertEqual(slave_status._GetMissingBuilds(), set(['missing']))

  def testGetMissingBuildsNone(self):
    """Tests GetMissingBuilds returns None."""
    cidb_status = {
        'build1': CIDBStatusInfos.GetFailedBuild(),
        'build2': CIDBStatusInfos.GetInflightBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    slave_status = self._GetSlaveStatus(
        builders_array=['build1', 'build2'])

    self.assertEqual(slave_status._GetMissingBuilds(), set())

  def testGetMissingBuildsNoneWithBuildbucket(self):
    """Tests GetMissing returns None with Buildbucket."""
    cidb_status = {
        'started': CIDBStatusInfos.GetInflightBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'scheduled': BuildbucketInfos.GetScheduledBuild(),
        'started': BuildbucketInfos.GetStartedBuild()
    }
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    slave_status = self._GetSlaveStatus(
        builders_array=['build1', 'build2'],
        config=self.master_cq_config)

    self.assertEqual(slave_status._GetMissingBuilds(), set())

  def testGetCompletedBuilds(self):
    """Tests GetCompletedBuilds returns the completed builds."""
    cidb_status = {
        'passed': CIDBStatusInfos.GetPassedBuild(),
        'failed': CIDBStatusInfos.GetFailedBuild(),
        'aborted': CIDBStatusInfos.GetAbortedBuild(),
        'skipped': CIDBStatusInfos.GetSkippedBuild(),
        'forgiven': CIDBStatusInfos.GetForgivenBuild(),
        'inflight': CIDBStatusInfos.GetInflightBuild(),
        'missing': CIDBStatusInfos.GetMissingBuild(),
        'planned': CIDBStatusInfos.GetPlannedBuild(),
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    slave_status = self._GetSlaveStatus(
        builders_array=['passed', 'failed', 'aborted', 'skipped', 'forgiven',
                        'inflight', 'missing', 'planning'])

    self.assertEqual(slave_status._GetCompletedBuilds(),
                     set(['passed', 'failed', 'aborted', 'skipped',
                          'forgiven']))

  def testGetUncompletedBuilds(self):
    """Tests _GetUncompletedBuilds"""
    self._Mock_GetSlaveStatusesFromCIDB(CIDBStatusInfos.GetFullCIDBStatusInfo())
    self._Mock_GetSlaveStatusesFromBuildbucket(
        BuildbucketInfos.GetFullBuildbucketInfoDict())

    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs())

    completed_builds = {'completed_success', 'completed_failure',
                        'completed_canceled'}
    self.assertEqual(slave_status._GetUncompletedBuilds(completed_builds),
                     {'scheduled', 'started'})

    completed_builds = {'completed_success', 'completed_failure'}
    self.assertEqual(slave_status._GetUncompletedBuilds(completed_builds),
                     {'scheduled', 'started', 'completed_canceled'})

  def testLastSlavesToComplete(self):
    """Tests _LastSlavesToComplete."""
    history = []
    self.assertEqual(set(),
                     build_status.SlaveStatus._LastSlavesToComplete(history))

    history = [['foo']]
    self.assertEqual({'foo'},
                     build_status.SlaveStatus._LastSlavesToComplete(history))

    history = [['foo'], ['bar', 'foo', 'qux']]
    self.assertEqual({'bar', 'qux'},
                     build_status.SlaveStatus._LastSlavesToComplete(history))


  def testGetRetriableBuildsReturnsNone(self):
    """GetRetriableBuilds returns no build to retry."""
    self._Mock_GetSlaveStatusesFromCIDB(CIDBStatusInfos.GetFullCIDBStatusInfo())
    self._Mock_GetSlaveStatusesFromBuildbucket(
        BuildbucketInfos.GetFullBuildbucketInfoDict())

    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_canary_config)
    self.assertEqual(
        slave_status._GetRetriableBuilds(self._GetCompletedAllSet()),
        set())

    self.db.InsertBuildStage(3, 'CommitQueueSync',
                             status=constants.BUILDER_STATUS_PASSED)
    self.db.InsertBuildStage(4, 'MasterSlaveLKGMSync',
                             status=constants.BUILDER_STATUS_PASSED)
    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config)
    self.assertEqual(
        slave_status._GetRetriableBuilds(self._GetCompletedAllSet()),
        set())

  def _MockForGetRetriableBuildsTests(self, exclude_cidb_builds=None):
    """Helper method for GetRetriableBuilds tests."""
    self._Mock_GetSlaveStatusesFromCIDB(CIDBStatusInfos.GetFullCIDBStatusInfo(
        exclude_builds=exclude_cidb_builds))
    self._Mock_GetSlaveStatusesFromBuildbucket(
        BuildbucketInfos.GetFullBuildbucketInfoDict())

  def testGetRetriableBuildsNotRetryOnStartedBuilds(self):
    """test _GetRetriableBuilds for master not retrying started builds."""
    self._MockForGetRetriableBuildsTests(
        exclude_cidb_builds=['completed_canceled'])

    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_canary_config)
    self.assertEqual(
        slave_status._GetRetriableBuilds(self._GetCompletedAllSet()),
        set(['completed_canceled']))

  def testGetRetriableBuildsRetryOnStartedBuilds(self):
    """Retry the slave if it fails to pass the critical stage."""
    self._MockForGetRetriableBuildsTests()

    self.db.InsertBuildStage(3, 'CommitQueueSync',
                             status=constants.BUILDER_STATUS_FAILED)
    self.db.InsertBuildStage(4, 'MasterSlaveLKGMSync',
                             status=constants.BUILDER_STATUS_FAILED)
    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config)
    self.assertEqual(
        slave_status._GetRetriableBuilds(self._GetCompletedAllSet()),
        set(['completed_failure', 'completed_canceled']))

  def testGetRetriableBuildsRetryOnStartedBuilds_2(self):
    """Retry the slave if it fails to pass the critical stage."""
    self._MockForGetRetriableBuildsTests()

    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config)
    self.assertEqual(
        slave_status._GetRetriableBuilds(self._GetCompletedAllSet()),
        set(['completed_failure', 'completed_canceled']))

  def testGetRetriableBuildsRetryOnStartedBuilds_3(self):
    """Retry the slave if it fails to pass the critical stage."""
    self._MockForGetRetriableBuildsTests()

    self.db.InsertBuildStage(3, 'CommitQueueSync',
                             status=constants.BUILDER_STATUS_PLANNED)
    self.db.InsertBuildStage(4, 'MasterSlaveLKGMSync',
                             status=constants.BUILDER_STATUS_PLANNED)
    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config)
    self.assertEqual(
        slave_status._GetRetriableBuilds(self._GetCompletedAllSet()),
        set(['completed_failure', 'completed_canceled']))

  def testGetRetriableBuildsExceedsLimit(self):
    """Do not return builds which have exceeded the retry_limit."""
    cidb_status = {
        'started': CIDBStatusInfos.GetInflightBuild(),
        'completed_success': CIDBStatusInfos.GetPassedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'scheduled': BuildbucketInfos.GetScheduledBuild(),
        'started': BuildbucketInfos.GetStartedBuild(),
        'completed_success': BuildbucketInfos.GetSuccessBuild(),
        'completed_failure': BuildbucketInfos.GetFailureBuild(
            retry=constants.BUILDBUCKET_BUILD_RETRY_LIMIT),
        'completed_canceled': BuildbucketInfos.GetCanceledBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config)

    self.assertEqual(
        slave_status._GetRetriableBuilds(self._GetCompletedAllSet()),
        set(['completed_canceled']))

  def testGetCompletedBuildsWithBuildbucket(self):
    """Tests GetCompletedBuilds with Buildbucket"""
    self._Mock_GetSlaveStatusesFromCIDB(CIDBStatusInfos.GetFullCIDBStatusInfo())
    self._Mock_GetSlaveStatusesFromBuildbucket(
        BuildbucketInfos.GetFullBuildbucketInfoDict())

    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_canary_config)

    self.assertEqual(slave_status._GetCompletedBuilds(),
                     set(['completed_success', 'completed_failure',
                          'completed_canceled']))

  def testGetCompletedBuildsWithBuildbucketAndRetriableBuilds(self):
    """Tests GetCompletedBuilds with Buildbucket and retriable builds."""
    self._Mock_GetSlaveStatusesFromCIDB(CIDBStatusInfos.GetFullCIDBStatusInfo())
    self._Mock_GetSlaveStatusesFromBuildbucket(
        BuildbucketInfos.GetFullBuildbucketInfoDict())
    self._Mock_GetRetriableBuilds(builds=set(['completed_failure']))
    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config)

    self.assertEqual(slave_status._GetCompletedBuilds(),
                     set(['completed_success', 'completed_canceled']))

  def testGetCompletedBuildsWithBuildbucketOnlyCompletedInCIDB(self):
    """Tests when builds only completed in CIDB not Buildbucket."""
    self._Mock_GetSlaveStatusesFromCIDB(CIDBStatusInfos.GetFullCIDBStatusInfo())
    buildbucket_info_dict = {
        'scheduled': BuildbucketInfos.GetScheduledBuild(),
        'started': BuildbucketInfos.GetStartedBuild(),
        'completed_success': BuildbucketInfos.GetSuccessBuild(),
        'completed_failure': BuildbucketInfos.GetStartedBuild(),
        'completed_canceled': BuildbucketInfos.GetCanceledBuild()
    }

    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)
    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config)

    self.assertEqual(slave_status._GetCompletedBuilds(),
                     set(['completed_success', 'completed_failure']))

  def testGetBuildsToRetry(self):
    """Test GetBuildsToRetry."""
    cidb_status = {
        'started': CIDBStatusInfos.GetInflightBuild(),
        'completed_success': CIDBStatusInfos.GetPassedBuild(),
        'completed_canceled': CIDBStatusInfos.GetInflightBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs())
    self.assertEqual(slave_status._GetBuildsToRetry(), None)

  def testGetBuildsToRetryWithBuildbucket(self):
    """Test GetBuildsToRetry with Buildbucket."""
    self._Mock_GetSlaveStatusesFromCIDB(CIDBStatusInfos.GetFullCIDBStatusInfo(
        exclude_builds=['completed_failure']))
    self._Mock_GetSlaveStatusesFromBuildbucket(
        BuildbucketInfos.GetFullBuildbucketInfoDict())

    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_canary_config)

    self.assertEqual(slave_status._GetBuildsToRetry(),
                     set(['completed_failure']))

  def testCompleted(self):
    """Tests Completed returns proper bool."""
    builders_array = ['build1', 'build2']
    statusNotCompleted = {
        'build1': CIDBStatusInfos.GetFailedBuild(),
        'build2': CIDBStatusInfos.GetInflightBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(statusNotCompleted)
    slaveStatusNotCompleted = self._GetSlaveStatus(
        builders_array=builders_array)
    self.assertFalse(slaveStatusNotCompleted._Completed())

    statusCompleted = {
        'build1': CIDBStatusInfos.GetFailedBuild(),
        'build2': CIDBStatusInfos.GetPassedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(statusCompleted)
    slaveStatusCompleted = self._GetSlaveStatus(
        builders_array=builders_array)
    self.assertTrue(slaveStatusCompleted._Completed())

  def testCompletedWithBuildbucket(self):
    """Tests Completed returns proper bool with Buildbucket."""
    builders_array = ['started', 'failure', 'missing']
    status_not_completed = {
        'started': CIDBStatusInfos.GetInflightBuild(),
        'failure': CIDBStatusInfos.GetFailedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(status_not_completed)

    buildbucket_info_dict_not_completed = {
        'started': BuildbucketInfos.GetStartedBuild(),
        'failure': BuildbucketInfos.GetFailureBuild(),
        'missing': BuildbucketInfos.GetMissingBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(
        buildbucket_info_dict_not_completed)

    slaveStatusNotCompleted = self._GetSlaveStatus(
        builders_array=builders_array,
        config=self.master_cq_config)

    self.assertFalse(slaveStatusNotCompleted._Completed())

    status_completed = {
        'success': CIDBStatusInfos.GetPassedBuild(),
        'failure': CIDBStatusInfos.GetFailedBuild(),
    }
    self._Mock_GetSlaveStatusesFromCIDB(status_completed)

    buildbucket_info_dict_complted = {
        'success': BuildbucketInfos.GetSuccessBuild(),
        'failure': BuildbucketInfos.GetFailureBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(
        buildbucket_info_dict_complted)

    builders_array = ['success', 'failure']
    slaveStatusCompleted = self._GetSlaveStatus(
        builders_array=builders_array,
        config=self.master_canary_config)
    self.assertTrue(slaveStatusCompleted._Completed())

    self._Mock_GetRetriableBuilds(builds=set())
    slaveStatusCompleted = self._GetSlaveStatus(
        builders_array=builders_array,
        config=self.master_cq_config)
    self.assertTrue(slaveStatusCompleted._Completed())

  def testCompletedWithNoSlaveScheduledByBuildbucket(self):
    """Tests Completed returns True when no slaves are scheduled."""
    self._Mock_GetSlaveStatusesFromCIDB({})
    self._Mock_GetSlaveStatusesFromBuildbucket({})
    slaveStatusCompleted = self._GetSlaveStatus(
        builders_array=['build1', 'build2'],
        config=self.master_cq_config)
    self.assertTrue(slaveStatusCompleted._Completed())

  def testShouldFailForBuilderStartTimeoutTrue(self):
    """Tests that ShouldFailForBuilderStartTimeout says fail when it should."""
    cidb_status = {
        'build1': CIDBStatusInfos.GetFailedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    start_time = datetime.datetime.now()
    slave_status = self._GetSlaveStatus(
        builders_array=['build1', 'build2'])
    check_time = start_time + datetime.timedelta(
        minutes=slave_status.BUILD_START_TIMEOUT_MIN + 1)

    self.assertTrue(slave_status._ShouldFailForBuilderStartTimeout(check_time))

  def testShouldFailForBuilderStartTimeoutTrueWithBuildbucket(self):
    """Tests that ShouldFailForBuilderStartTimeout says fail when it should."""
    cidb_status = {
        'success': CIDBStatusInfos.GetPassedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'success': BuildbucketInfos.GetSuccessBuild(),
        'scheduled': BuildbucketInfos.GetScheduledBuild()
    }
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    start_time = datetime.datetime.now()
    slave_status = self._GetSlaveStatus(
        start_time=start_time,
        builders_array=['success', 'scheduled'],
        config=self.master_cq_config)
    check_time = start_time + datetime.timedelta(
        minutes=slave_status.BUILD_START_TIMEOUT_MIN + 1)

    self.assertTrue(slave_status._ShouldFailForBuilderStartTimeout(check_time))

  def testShouldFailForBuilderStartTimeoutFalseTooEarly(self):
    """Tests that ShouldFailForBuilderStartTimeout doesn't fail.

    Make sure that we don't fail if there are missing builders but we're
    checking before the timeout and the other builders have completed.
    """
    cidb_status = {
        'build1': CIDBStatusInfos.GetFailedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    start_time = datetime.datetime.now()
    slave_status = self._GetSlaveStatus(
        start_time=start_time,
        builders_array=['build1', 'build2'])

    self.assertFalse(slave_status._ShouldFailForBuilderStartTimeout(start_time))

  def testShouldFailForBuilderStartTimeoutFalseTooEarlyWithBuildbucket(self):
    """Tests that ShouldFailForBuilderStartTimeout doesn't fail.

    With Buildbucket, make sure that we don't fail if there are missing
    builders but we're checking before the timeout and the other builders
    have completed.
    """
    cidb_status = {
        'success': CIDBStatusInfos.GetPassedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'success': BuildbucketInfos.GetSuccessBuild(),
        'missing': BuildbucketInfos.GetMissingBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    start_time = datetime.datetime.now()
    slave_status = self._GetSlaveStatus(
        builders_array=['success', 'missing'],
        config=self.master_cq_config)

    self.assertFalse(slave_status._ShouldFailForBuilderStartTimeout(start_time))

  def testShouldFailForBuilderStartTimeoutFalseNotCompleted(self):
    """Tests that ShouldFailForBuilderStartTimeout doesn't fail.

    Make sure that we don't fail if there are missing builders and we're
    checking after the timeout but the other builders haven't completed.
    """
    cidb_status = {
        'build1': CIDBStatusInfos.GetInflightBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    start_time = datetime.datetime.now()
    slave_status = self._GetSlaveStatus(
        start_time=start_time,
        builders_array=['build1', 'build2'],
        config=self.master_cq_config,
        metadata=self.metadata,
        buildbucket_client=self.buildbucket_client)
    check_time = start_time + datetime.timedelta(
        minutes=slave_status.BUILD_START_TIMEOUT_MIN + 1)

    self.assertFalse(slave_status._ShouldFailForBuilderStartTimeout(check_time))

  def testShouldFailForStartTimeoutFalseNotCompletedWithBuildbucket(self):
    """Tests that ShouldWait doesn't fail with Buildbucket.

    With Buildbucket, make sure that we don't fail if there are missing builders
    and we're checking after the timeout but the other builders haven't
    completed.
    """
    cidb_status = {
        'started': CIDBStatusInfos.GetInflightBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'started': BuildbucketInfos.GetStartedBuild(),
        'missing': BuildbucketInfos.GetMissingBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    start_time = datetime.datetime.now()
    slave_status = self._GetSlaveStatus(
        start_time=start_time,
        builders_array=['started', 'missing'],
        config=self.master_cq_config)
    check_time = start_time + datetime.timedelta(
        minutes=slave_status.BUILD_START_TIMEOUT_MIN + 1)

    self.assertFalse(slave_status._ShouldFailForBuilderStartTimeout(check_time))

  def testShouldWaitAllBuildersCompleted(self):
    """Tests that ShouldWait says no waiting because all builders finished."""
    cidb_status = {
        'build1': CIDBStatusInfos.GetFailedBuild(),
        'build2': CIDBStatusInfos.GetPassedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    slave_status = self._GetSlaveStatus(
        builders_array=['build1', 'build2'])

    self.assertFalse(slave_status.ShouldWait())

  def testShouldWaitAllBuildersCompletedWithBuildbucket(self):
    """ShouldWait says no because all builders finished with Buildbucket."""
    cidb_status = {
        'failure': CIDBStatusInfos.GetFailedBuild(),
        'success': CIDBStatusInfos.GetPassedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'failure': BuildbucketInfos.GetFailureBuild(),
        'success': BuildbucketInfos.GetSuccessBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    slave_status = self._GetSlaveStatus(
        builders_array=['failure', 'success'],
        config=self.master_canary_config)

    self.assertFalse(slave_status.ShouldWait())

  def testShouldWaitImportantBuildersCompleted(self):
    """Tests that ShouldWait says no waiting when all important builders done.

    If all important builds are finished, though some experimental builders
    are not, ShouldWait should say to stop.
    """
    cidb_status = {
        'build1': CIDBStatusInfos.GetFailedBuild(),
        'build2': CIDBStatusInfos.GetPassedBuild(),
        'build3': CIDBStatusInfos.GetInflightBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)
    self.PatchObject(tree_status, 'GetExperimentalBuilders',
                     return_value=['build3'])

    slave_status = self._GetSlaveStatus(
        builders_array=['build1', 'build2', 'build3'])
    slave_status.UpdateSlaveStatus()

    self.assertFalse(slave_status.ShouldWait())

  def testShouldWaitCancelsSlowExperimentalIfImportantBuildersCompleted(self):
    """Tests that ShouldWait cancels experimental if important builds are done.

    If all important builds are finished, but some experimental builders
    are not, ShouldWait should cancel the experimental builders.
    """
    cidb_status = {
        'build1': CIDBStatusInfos.GetFailedBuild(),
        'build2': CIDBStatusInfos.GetPassedBuild(),
        'build3': CIDBStatusInfos.GetInflightBuild()
    }
    important_build_names = ['build1', 'build2']
    experimental_builds = [('build3', 'build3', 0)]
    experimental_build_ids = ['build3']

    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)
    self._MockGetAllSlaveBuildbucketInfo({})
    self.metadata.UpdateWithDict({
        constants.METADATA_SCHEDULED_EXPERIMENTAL_SLAVES: experimental_builds})
    mock_cancel_builds = self.PatchObject(builder_status_lib, "CancelBuilds")

    slave_status = self._GetSlaveStatus(
        builders_array=important_build_names)

    self.assertFalse(slave_status.ShouldWait())
    mock_cancel_builds.assert_called_with(
        experimental_build_ids, mock.ANY, mock.ANY, mock.ANY)

  def testShouldWaitMissingBuilder(self):
    """Tests that ShouldWait says no waiting because a builder is missing."""
    cidb_status = {
        'build1': CIDBStatusInfos.GetFailedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    slave_status = self._GetSlaveStatus(
        start_time=datetime.datetime.now() - datetime.timedelta(hours=1),
        builders_array=['build1', 'build2'])

    self.assertFalse(slave_status.ShouldWait())

  def testShouldWaitScheduledBuilderWithBuildbucket(self):
    """Test ShouldWait on canary-master with scheduled builds."""
    cidb_status = {
        'failure': CIDBStatusInfos.GetFailedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'failure': BuildbucketInfos.GetFailureBuild(),
        'scheduled': BuildbucketInfos.GetScheduledBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    slave_status = self._GetSlaveStatus(
        start_time=datetime.datetime.now() - datetime.timedelta(hours=1),
        builders_array=['failure', 'scheduled'],
        config=self.master_canary_config)
    self.assertFalse(slave_status.ShouldWait())

  def testShouldWaitScheduledBuilderWithBuildbucket_2(self):
    """Test ShouldWait on CQ-master with scheduled builds."""
    cidb_status = {
        'failure': CIDBStatusInfos.GetFailedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'failure': BuildbucketInfos.GetFailureBuild(),
        'scheduled': BuildbucketInfos.GetScheduledBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    self._Mock_GetRetriableBuilds(set())
    slave_status = self._GetSlaveStatus(
        start_time=datetime.datetime.now() - datetime.timedelta(hours=1),
        builders_array=['failure', 'scheduled'],
        config=self.master_cq_config)
    self.assertFalse(slave_status.ShouldWait())

    self._Mock_GetRetriableBuilds(set(['failure']))
    self._Mock_RetryBuilds()
    slave_status = self._GetSlaveStatus(
        start_time=datetime.datetime.now() - datetime.timedelta(hours=1),
        builders_array=['failure', 'scheduled'],
        config=self.master_cq_config)
    self.assertTrue(slave_status.ShouldWait())

  def testShouldWaitNoScheduledBuilderWithBuildbucket(self):
    """Test ShouldWait on canary-master without scheduled builds."""
    cidb_status = {
        'failure': CIDBStatusInfos.GetFailedBuild(),
        'success': CIDBStatusInfos.GetPassedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'failure': BuildbucketInfos.GetFailureBuild(),
        'success': BuildbucketInfos.GetSuccessBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    slave_status = self._GetSlaveStatus(
        start_time=datetime.datetime.now() - datetime.timedelta(hours=1),
        builders_array=['failure', 'success'],
        config=self.master_canary_config)

    self.assertFalse(slave_status.ShouldWait())

  def testShouldWaitNoScheduledBuilderWithBuildbucket_2(self):
    """Test ShouldWait on cq-master without scheduled builds."""
    cidb_status = {
        'failure': CIDBStatusInfos.GetFailedBuild(),
        'success': CIDBStatusInfos.GetPassedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'failure': BuildbucketInfos.GetFailureBuild(),
        'success': BuildbucketInfos.GetSuccessBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    self._Mock_GetRetriableBuilds(set())
    slave_status = self._GetSlaveStatus(
        start_time=datetime.datetime.now() - datetime.timedelta(hours=1),
        builders_array=['failure', 'success'],
        config=self.master_cq_config)
    self.assertFalse(slave_status.ShouldWait())

    self._Mock_GetRetriableBuilds(set(['failure']))
    self._Mock_RetryBuilds()
    slave_status = self._GetSlaveStatus(
        start_time=datetime.datetime.now() - datetime.timedelta(hours=1),
        builders_array=['failure', 'success'],
        config=self.master_cq_config)
    self.assertTrue(slave_status.ShouldWait())

  def testShouldWaitMissingBuilderWithBuildbucket(self):
    """Test ShouldWait says yes waiting because one build status is missing."""
    cidb_status = {
        'failure': CIDBStatusInfos.GetFailedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'failure': BuildbucketInfos.GetFailureBuild(),
        'missing': BuildbucketInfos.GetMissingBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    slave_status = self._GetSlaveStatus(
        start_time=datetime.datetime.now() - datetime.timedelta(hours=1),
        builders_array=['failure', 'missing'],
        config=self.master_canary_config)

    self.assertTrue(slave_status.ShouldWait())

  def testShouldWaitBuildersStillBuilding(self):
    """Tests that ShouldWait says to wait because builders still building."""
    cidb_status = {
        'build1': CIDBStatusInfos.GetInflightBuild(),
        'build2': CIDBStatusInfos.GetFailedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    slave_status = self._GetSlaveStatus(
        builders_array=['build1', 'build2'])

    self.assertTrue(slave_status.ShouldWait())

  def testShouldWaitExperimentalBuildersStillBuilding(self):
    """Tests that ShouldWait says not to wait on experimental builders."""
    cidb_status = {
        'build1': CIDBStatusInfos.GetInflightBuild(),
        'build2': CIDBStatusInfos.GetFailedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    slave_status = self._GetSlaveStatus(
        builders_array=['build1', 'build2'])

    self.assertTrue(slave_status.ShouldWait())

    self.metadata.UpdateWithDict({
        constants.METADATA_EXPERIMENTAL_BUILDERS: ['build1']
    })
    self.assertFalse(slave_status.ShouldWait())

  def test_ShouldWaitInvokesCancelBuildsWithListOfIDs(self):
    """Tests that _ShouldWait sends a serializable list of build IDs."""
    mock_cancel_builds = self.PatchObject(builder_status_lib, "CancelBuilds")
    cidb_status = {
        'build1': CIDBStatusInfos.GetFailedBuild(),
        'build2': CIDBStatusInfos.GetPlannedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    slave_status = self._GetSlaveStatus(
        builders_array=['build1', 'build2'])
    self.PatchObject(build_status.SlaveStatus,
                     '_ShouldFailForBuilderStartTimeout',
                     return_value=True)

    slave_status._ShouldWait()
    (ids, _, _, _), _ = mock_cancel_builds.call_args
    self.assertTrue(isinstance(ids, list))

  def testUpdateSlaveStatusWithoutExperimentalBuilders(self):
    """UpdateSlaveStatus without experimental builders."""
    self._Mock_GetSlaveStatusesFromCIDB(CIDBStatusInfos.GetFullCIDBStatusInfo())
    self._Mock_GetSlaveStatusesFromBuildbucket(
        BuildbucketInfos.GetFullBuildbucketInfoDict())

    scheduled_slave_builds = [(build, 'bb_id', None)
                              for build in self._GetFullBuildConfigs()]
    self.metadata.UpdateWithDict({
        constants.METADATA_SCHEDULED_IMPORTANT_SLAVES: scheduled_slave_builds})

    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config)

    slave_status.UpdateSlaveStatus()
    self.assertItemsEqual(slave_status.metadata.GetValueWithDefault(
        constants.METADATA_EXPERIMENTAL_BUILDERS, []), [])
    self.assertItemsEqual(slave_status.all_builders,
                          self._GetFullBuildConfigs())

  def testUpdateSlaveStatusRemovesScheduledExperimentalBuilders(self):
    """UpdateSlaveStatus removes experimental from scheduled builds."""
    cidb_status = {
        'build1': CIDBStatusInfos.GetInflightBuild(),
        'build2': CIDBStatusInfos.GetInflightBuild(),
        'build3': CIDBStatusInfos.GetFailedBuild(),
        'build4': CIDBStatusInfos.GetFailedBuild(),
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'build1': BuildbucketInfos.GetStartedBuild(),
        'build2': BuildbucketInfos.GetStartedBuild(),
        'build3': BuildbucketInfos.GetFailureBuild(),
        'build4': BuildbucketInfos.GetFailureBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(
        buildbucket_info_dict, mock_GetBuildInfoDict=False)

    scheduled_slave_builds = [(build, 'bb_id', None) for build in
                              ('build1', 'build2', 'build3', 'build4')]
    self.metadata.UpdateWithDict({
        constants.METADATA_SCHEDULED_IMPORTANT_SLAVES: scheduled_slave_builds})

    slave_status = self._GetSlaveStatus(
        builders_array=['build1', 'build2', 'build3', 'build4'],
        config=self.master_cq_config)

    self.PatchObject(tree_status, 'GetExperimentalBuilders',
                     return_value=['build1', 'build3'])
    slave_status.UpdateSlaveStatus()
    self.assertItemsEqual(slave_status.metadata.GetValueWithDefault(
        constants.METADATA_EXPERIMENTAL_BUILDERS, []), ['build1', 'build3'])
    self.assertItemsEqual(slave_status.all_builders, ['build2', 'build4'])

  def testUpdateSlaveStatusRemovesCompletedExperimentalBuilders(self):
    """UpdateSlaveStatus removes experimental builds from completed_builds."""
    cidb_status = {
        'build1': CIDBStatusInfos.GetInflightBuild(),
        'build2': CIDBStatusInfos.GetInflightBuild(),
        'build3': CIDBStatusInfos.GetFailedBuild(),
        'build4': CIDBStatusInfos.GetFailedBuild(),
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'build1': BuildbucketInfos.GetStartedBuild(),
        'build2': BuildbucketInfos.GetStartedBuild(),
        'build3': BuildbucketInfos.GetFailureBuild(),
        'build4': BuildbucketInfos.GetFailureBuild()}
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    slave_status = self._GetSlaveStatus(
        builders_array=['build1', 'build2', 'build3', 'build4'],
        config=self.master_cq_config)
    slave_status.completed_builds = {'build3', 'build4'}

    self.PatchObject(tree_status, 'GetExperimentalBuilders',
                     return_value=['build1', 'build3'])
    slave_status.UpdateSlaveStatus()
    self.assertItemsEqual(slave_status.metadata.GetValueWithDefault(
        constants.METADATA_EXPERIMENTAL_BUILDERS, []), ['build1', 'build3'])
    self.assertItemsEqual(slave_status.completed_builds, {'build4'})

  def testShouldWaitBuildersStillBuildingWithBuildbucket(self):
    """ShouldWait says yes because builders still in started status."""
    cidb_status = {
        'started': CIDBStatusInfos.GetInflightBuild(),
        'failure': CIDBStatusInfos.GetFailedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'started': BuildbucketInfos.GetStartedBuild(),
        'failure': BuildbucketInfos.GetFailureBuild()
    }
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    slave_status = self._GetSlaveStatus(
        builders_array=['started', 'failure'],
        config=self.master_canary_config)

    self.assertTrue(slave_status.ShouldWait())
    self.assertEqual(slave_status.builds_to_retry, set())

  def testShouldWaitBuildersStillBuildingWithBuildbucket_2(self):
    """ShouldWait says yes because builders still in started status."""
    cidb_status = {
        'started': CIDBStatusInfos.GetInflightBuild(),
        'failure': CIDBStatusInfos.GetFailedBuild()
    }
    self._Mock_GetSlaveStatusesFromCIDB(cidb_status)

    buildbucket_info_dict = {
        'started': BuildbucketInfos.GetStartedBuild(),
        'failure': BuildbucketInfos.GetFailureBuild()
    }
    self._Mock_GetSlaveStatusesFromBuildbucket(buildbucket_info_dict)

    self._Mock_GetRetriableBuilds(set(['failure']))
    self._Mock_RetryBuilds()
    slave_status = self._GetSlaveStatus(
        builders_array=['started', 'failure'],
        config=self.master_canary_config)

    self.assertTrue(slave_status.ShouldWait())
    self.assertEqual(slave_status.builds_to_retry, set(['failure']))

  def testShouldWaitSetsBuildsToRetry(self):
    """ShouldWait should update slave_status.builds_to_retry."""
    self.PatchObject(build_status.SlaveStatus,
                     '_Completed',
                     return_value=False)
    self.PatchObject(build_status.SlaveStatus,
                     '_ShouldFailForBuilderStartTimeout',
                     return_value=False)
    slave_status = self._GetSlaveStatus(
        builders_array=['slave1', 'slave2'],
        config=self.master_cq_config)
    slave_status.builds_to_retry = set(['slave1', 'slave2'])

    self.PatchObject(build_status.SlaveStatus, '_RetryBuilds',
                     return_value=set(['slave1']))
    slave_status.ShouldWait()
    self.assertEqual(slave_status.builds_to_retry, set(['slave2']))

    self.PatchObject(build_status.SlaveStatus, '_RetryBuilds',
                     return_value=set(['slave2']))
    slave_status.ShouldWait()
    self.assertEqual(slave_status.builds_to_retry, set([]))

  def testShouldWaitWithTriageRelevantChangesShouldWaitFalse(self):
    """Test ShouldWait with TriageRelevantChanges.ShouldWait is False."""
    self._Mock_GetSlaveStatusesFromCIDB(CIDBStatusInfos.GetFullCIDBStatusInfo())
    self._MockGetAllSlaveCIDBStatusInfo(CIDBStatusInfos.GetFullCIDBStatusInfo())
    self._Mock_GetSlaveStatusesFromBuildbucket(
        BuildbucketInfos.GetFullBuildbucketInfoDict())

    relevant_changes.TriageRelevantChanges.__init__ = mock.Mock(
        return_value=None)
    self.PatchObject(relevant_changes.TriageRelevantChanges,
                     'ShouldSelfDestruct', return_value=(True, False))
    self.PatchObject(build_status.SlaveStatus,
                     '_Completed', return_value=False)
    self.PatchObject(build_status.SlaveStatus,
                     '_ShouldFailForBuilderStartTimeout', return_value=False)
    self.PatchObject(build_status.SlaveStatus, '_RetryBuilds')
    pool = validation_pool_unittest.MakePool(applied=[])
    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config,
        version='9289.0.0-rc2',
        pool=pool)

    self.assertFalse(slave_status.ShouldWait())
    self.assertTrue(slave_status.metadata.GetValueWithDefault(
        constants.SELF_DESTRUCTED_BUILD, False))
    self.assertFalse(slave_status.metadata.GetValueWithDefault(
        constants.SELF_DESTRUCTED_WITH_SUCCESS_BUILD, False))

    build_messages = self.db.GetBuildMessages(self.master_build_id)
    self.assertEqual(len(build_messages), 3)
    for m in build_messages:
      self.assertEqual(m['message_type'],
                       constants.MESSAGE_TYPE_IGNORED_REASON)
      self.assertEqual(m['message_subtype'],
                       constants.MESSAGE_SUBTYPE_SELF_DESTRUCTION)
      self.assertTrue(m['message_value'] in ('1', '3', '4'))

  def testShouldWaitWithTriageRelevantChangesShouldWaitTrue(self):
    """Test ShouldWait with TriageRelevantChanges.ShouldWait is True."""
    relevant_changes.TriageRelevantChanges.__init__ = mock.Mock(
        return_value=None)
    self.PatchObject(relevant_changes.TriageRelevantChanges,
                     'ShouldSelfDestruct', return_value=(False, False))
    self.PatchObject(build_status.SlaveStatus,
                     '_Completed',
                     return_value=False)
    self.PatchObject(build_status.SlaveStatus,
                     '_ShouldFailForBuilderStartTimeout',
                     return_value=False)
    pool = validation_pool_unittest.MakePool(applied=[])
    slave_status = self._GetSlaveStatus(
        builders_array=['slave1', 'slave2'],
        config=self.master_cq_config,
        version='9289.0.0-rc2',
        pool=pool)

    self.assertTrue(slave_status.ShouldWait())
    self.assertFalse(slave_status.metadata.GetValueWithDefault(
        constants.SELF_DESTRUCTED_BUILD, False))
    self.assertFalse(slave_status.metadata.GetValueWithDefault(
        constants.SELF_DESTRUCTED_WITH_SUCCESS_BUILD, False))

  def testRetryBuilds(self):
    """Test RetryBuilds."""
    self._Mock_GetSlaveStatusesFromCIDB()
    self.PatchObject(build_status.SlaveStatus, 'UpdateSlaveStatus')
    metadata = metadata_lib.CBuildbotMetadata()
    slaves = [('failure', 'id_1', time.time()),
              ('canceled', 'id_2', time.time())]
    metadata.ExtendKeyListWithList(
        constants.METADATA_SCHEDULED_IMPORTANT_SLAVES, slaves)

    slave_status = self._GetSlaveStatus(
        builders_array=['failure', 'canceled'],
        config=self.master_cq_config,
        metadata=metadata,
        buildbucket_client=self.buildbucket_client)

    builds_to_retry = set(['failure', 'canceled'])
    updated_buildbucket_info_dict = {
        'failure': BuildbucketInfos.GetFailureBuild(),
        'canceled': BuildbucketInfos.GetCanceledBuild(),
    }
    slave_status.new_buildbucket_info_dict = updated_buildbucket_info_dict
    content = {
        'build':{
            'status': 'SCHEDULED',
            'id': 'retry_id',
            'retry_of': 'id',
            'created_ts': time.time()
        }
    }
    slave_status.buildbucket_client.RetryBuildRequest.return_value = content

    retried_builds = slave_status._RetryBuilds(builds_to_retry)
    self.assertEqual(retried_builds, builds_to_retry)
    buildbucket_info_dict = buildbucket_lib.GetBuildInfoDict(
        slave_status.metadata)
    self.assertEqual(buildbucket_info_dict['failure'].buildbucket_id,
                     'retry_id')
    self.assertEqual(buildbucket_info_dict['failure'].retry, 1)
    requests = slave_status.db.GetBuildRequestsForBuildConfigs('failure')
    self.assertEqual(len(requests), 1)
    self.assertEqual(requests[0].request_build_config, 'failure')
    self.assertEqual(requests[0].request_buildbucket_id, 'retry_id')
    self.assertEqual(requests[0].request_reason,
                     build_requests.REASON_IMPORTANT_CQ_SLAVE)

    retried_builds = slave_status._RetryBuilds(builds_to_retry)
    self.assertEqual(retried_builds, builds_to_retry)
    buildbucket_info_dict = buildbucket_lib.GetBuildInfoDict(
        slave_status.metadata)
    self.assertEqual(buildbucket_info_dict['canceled'].buildbucket_id,
                     'retry_id')
    self.assertEqual(buildbucket_info_dict['canceled'].retry, 2)
    requests = slave_status.db.GetBuildRequestsForBuildConfigs('canceled')
    self.assertEqual(len(requests), 2)
    for req in requests:
      self.assertEqual(req.request_build_config, 'canceled')
      self.assertEqual(req.request_buildbucket_id, 'retry_id')
      self.assertEqual(req.request_reason,
                       build_requests.REASON_IMPORTANT_CQ_SLAVE)

  def test_GetNewSlaveBuildbucketInfo(self):
    """Test _GetNewSlaveBuildbucketInfo."""
    all_buildbucket_info_dict = self._GetCompletedBuildInfoDict()
    completed = {'completed_success'}

    slave_status = self._GetSlaveStatus(
        builders_array=[self._GetCompletedAllSet()],
        config=self.master_cq_config)

    new_buildbucket_info = slave_status._GetNewSlaveBuildbucketInfo(
        all_buildbucket_info_dict, completed)

    self.assertItemsEqual(new_buildbucket_info.keys(),
                          ['completed_canceled', 'completed_failure'])

  def test_GetNewSlaveCIDBStatusInfoWithCompletedBuilds(self):
    """test _GetNewSlaveCIDBStatusInfo with completed_builds."""
    all_cidb_status_dict = CIDBStatusInfos.GetFullCIDBStatusInfo()
    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config)
    cidb_status_dict = slave_status._GetNewSlaveCIDBStatusInfo(
        all_cidb_status_dict, set(['completed_success']))

    self.assertItemsEqual(
        cidb_status_dict.keys(),
        ['started', 'completed_failure', 'completed_canceled'])

  def test_GetNewSlaveCIDBStatusInfoWithEmptyCompletedBuilds(self):
    """Test _GetNewSlaveCIDBStatusInfo with empty completed_builds."""
    all_cidb_status_dict = CIDBStatusInfos.GetFullCIDBStatusInfo()
    slave_status = self._GetSlaveStatus(
        builders_array=self._GetFullBuildConfigs(),
        config=self.master_cq_config)
    cidb_status_dict = slave_status._GetNewSlaveCIDBStatusInfo(
        all_cidb_status_dict, set())

    self.assertDictEqual(cidb_status_dict, all_cidb_status_dict)

  def test_GetUncompletedExperimentalBuildNamesWithCIDB(self):
    """Test _GetUncompletedExperimentalBuildNames acknowledges CIDB.

    It should exclude builds marked completed in CIDB from its returns.
    """
    slave_ids = ['build1', 'build2', 'build3', 'build4']
    cidb_status = [
        {'id': 'build1', 'build_config': 'build1', 'build_number': 1,
         'status': constants.BUILDER_STATUS_FAILED},
        {'id': 'build2', 'build_config': 'build2', 'build_number': 2,
         'status': constants.BUILDER_STATUS_PASSED},
        {'id': 'build3', 'build_config': 'build3', 'build_number': 3,
         'status': constants.BUILDER_STATUS_INFLIGHT},
        {'id': 'build4', 'build_config': 'build4', 'build_number': 4,
         'status': constants.BUILDER_STATUS_INFLIGHT},
    ]
    cidb_statuses = {
        'build1': CIDBStatusInfos.GetFailedBuild(build_id=1),
        'build2': CIDBStatusInfos.GetPassedBuild(build_id=2),
        'build3': CIDBStatusInfos.GetInflightBuild(build_id=3),
        'build4': CIDBStatusInfos.GetInflightBuild(build_id=4)
    }
    experimental_builds = [
        ('build1', 'build1', 0),
        ('build2', 'build2', 0),
        ('build3', 'build3', 0)
    ]
    self.PatchObject(self.db, 'GetSlaveStatuses',
                     return_value=cidb_status)
    self._MockGetAllSlaveCIDBStatusInfo(cidb_statuses)
    self._MockGetAllSlaveBuildbucketInfo({})

    slave_status = self._GetSlaveStatus(builders_array=slave_ids)
    slave_status.metadata.UpdateWithDict({
        constants.METADATA_SCHEDULED_EXPERIMENTAL_SLAVES: experimental_builds})

    self.assertEqual(slave_status._GetUncompletedExperimentalBuildbucketIDs(),
                     set(['build3']))

  def test_GetUncompletedExperimentalBuildNamesWithBuildBucket(self):
    """Test _GetUncompletedExperimentalBuildNames acknowledges Buildbucket.

    It should exclude builds marked completed in Buildbucket from its returns.
    """
    slave_names = ['build1', 'build2', 'build3', 'build4']
    experimental_builds = [
        ('build1', 'build1', 0),
        ('build2', 'build2', 0),
        ('build3', 'build3', 0)
    ]
    bb_statuses = {
        'build1': BuildbucketInfos.GetFailureBuild(bb_id=1),
        'build2': BuildbucketInfos.GetSuccessBuild(bb_id=2),
        'build3': BuildbucketInfos.GetStartedBuild(bb_id=3),
        'build4': BuildbucketInfos.GetStartedBuild(bb_id=4)
    }
    self.PatchObject(self.db, 'GetSlaveStatuses', return_value=[])
    self._MockGetAllSlaveCIDBStatusInfo(dict())
    self._MockGetAllSlaveBuildbucketInfo(bb_statuses)

    slave_status = self._GetSlaveStatus(builders_array=slave_names)
    slave_status.metadata.UpdateWithDict({
        constants.METADATA_SCHEDULED_EXPERIMENTAL_SLAVES: experimental_builds})

    self.assertEqual(slave_status._GetUncompletedExperimentalBuildbucketIDs(),
                     set(['build3']))
