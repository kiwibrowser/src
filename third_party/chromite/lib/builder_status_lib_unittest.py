# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for builder_status_lib."""

from __future__ import print_function

import mock

from chromite.cbuildbot import build_status_unittest
from chromite.lib.const import waterfall
from chromite.lib import buildbucket_lib
from chromite.lib import builder_status_lib
from chromite.lib import cidb
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import fake_cidb
from chromite.lib import failure_message_lib
from chromite.lib import failure_message_lib_unittest
from chromite.lib import metadata_lib


bb_infos = build_status_unittest.BuildbucketInfos
cidb_infos = build_status_unittest.CIDBStatusInfos
failure_msg_helper = failure_message_lib_unittest.FailureMessageHelper
stage_failure_helper = failure_message_lib_unittest.StageFailureHelper


def ConstructFailureMessages(build_config):
  """Helper method to construct failure messages."""
  entry_1 = stage_failure_helper.GetStageFailure(
      build_config=build_config, failure_id=1)
  entry_2 = stage_failure_helper.GetStageFailure(
      build_config=build_config, failure_id=2, outer_failure_id=1)
  entry_3 = stage_failure_helper.GetStageFailure(
      build_config=build_config, failure_id=3, outer_failure_id=1)
  failure_entries = [entry_1, entry_2, entry_3]
  failure_messages = (
      failure_message_lib.FailureMessageManager.ConstructStageFailureMessages(
          failure_entries))

  return failure_messages


class BuilderStatusLibTests(cros_test_lib.MockTestCase):
  """Tests for builder_status_lib."""

  def testGetSlavesAbortedBySelfDestructedMaster(self):
    """Test GetSlavesAbortedBySelfDestructedMaster with aborted slaves."""
    db = fake_cidb.FakeCIDBConnection()
    cidb.CIDBConnectionFactory.SetupMockCidb(db)
    master_build_id = db.InsertBuild(
        'master', waterfall.WATERFALL_INTERNAL, 1, 'master', 'bot_hostname',
        buildbucket_id='0')

    self.assertEqual(
        set(),
        builder_status_lib.GetSlavesAbortedBySelfDestructedMaster(
            master_build_id, db))

    slave_build_id_1 = db.InsertBuild(
        'slave_1', waterfall.WATERFALL_INTERNAL, 1, 'slave_1', 'bot_hostname',
        master_build_id=master_build_id, buildbucket_id='1')
    slave_build_id_2 = db.InsertBuild(
        'slave_2', waterfall.WATERFALL_INTERNAL, 2, 'slave_2', 'bot_hostname',
        master_build_id=master_build_id, buildbucket_id='2')
    db.InsertBuild(
        'slave_3', waterfall.WATERFALL_INTERNAL, 3, 'slave_3', 'bot_hostname',
        master_build_id=master_build_id, buildbucket_id='3')
    for slave_build_id in (slave_build_id_1, slave_build_id_2):
      db.InsertBuildMessage(
          master_build_id,
          message_type=constants.MESSAGE_TYPE_IGNORED_REASON,
          message_subtype=constants.MESSAGE_SUBTYPE_SELF_DESTRUCTION,
          message_value=str(slave_build_id))
    self.assertEqual(
        {'slave_1', 'slave_2'},
        builder_status_lib.GetSlavesAbortedBySelfDestructedMaster(
            master_build_id, db))


# pylint: disable=protected-access
class BuilderStatusManagerTest(cros_test_lib.MockTestCase):
  """Tests for BuilderStatusManager."""

  def setUp(self):
    self.db = fake_cidb.FakeCIDBConnection()

  def testCreateBuildFailureMessageWithMessages(self):
    """Test CreateBuildFailureMessage with stage failure messages."""
    overlays = constants.PRIVATE_OVERLAYS
    dashboard_url = 'http://fake_dashboard_url'
    slave = 'cyan-paladin'
    failure_messages = ConstructFailureMessages(slave)

    build_msg = (
        builder_status_lib.BuilderStatusManager.CreateBuildFailureMessage(
            slave, overlays, dashboard_url, failure_messages))

    self.assertTrue('stage failed' in build_msg.message_summary)
    self.assertTrue(build_msg.internal)
    self.assertEqual(build_msg.builder, slave)

  def testCreateBuildFailureMessageWithoutMessages(self):
    """Test CreateBuildFailureMessage without stage failure messages."""
    overlays = constants.PUBLIC_OVERLAYS
    dashboard_url = 'http://fake_dashboard_url'
    slave = 'cyan-paladin'

    build_msg = (
        builder_status_lib.BuilderStatusManager.CreateBuildFailureMessage(
            slave, overlays, dashboard_url, None))

    self.assertTrue('cbuildbot failed' in build_msg.message_summary)
    self.assertFalse(build_msg.internal)
    self.assertEqual(build_msg.builder, slave)

  def testCreateBuildFailureMessageWhenCanceled(self):
    """Test CreateBuildFailureMessage with no stage failure and canceled"""
    overlays = constants.PRIVATE_OVERLAYS
    dashboard_url = 'http://fake_dashboard_url'
    slave = 'cyan-paladin'

    build_msg = (
        builder_status_lib.BuilderStatusManager.CreateBuildFailureMessage(
            slave, overlays, dashboard_url, None,
            aborted_by_self_destruction=True))

    self.assertTrue('aborted by self-destruction' in build_msg.message_summary)
    self.assertFalse('cbuildbot failed' in build_msg.message_summary)
    self.assertEqual(build_msg.builder, slave)

  def testCreateBuildFailureMessageSupersedesCancellation(self):
    """Test CreateBuildFailureMessage with a stage failure when canceled"""
    overlays = constants.PRIVATE_OVERLAYS
    dashboard_url = 'http://fake_dashboard_url'
    slave = 'cyan-paladin'
    failure_messages = ConstructFailureMessages(slave)

    build_msg = (
        builder_status_lib.BuilderStatusManager.CreateBuildFailureMessage(
            slave, overlays, dashboard_url, failure_messages,
            aborted_by_self_destruction=True))

    self.assertFalse('canceled by master' in build_msg.message_summary)
    self.assertFalse('cbuildbot failed' in build_msg.message_summary)
    self.assertEqual(build_msg.builder, slave)


class SlaveBuilderStatusTest(cros_test_lib.MockTestCase):
  """Tests for SlaveBuilderStatus."""

  def setUp(self):
    self.db = fake_cidb.FakeCIDBConnection()
    self.master_build_id = 0
    self.site_config = config_lib.GetConfig()
    self.config = self.site_config['master-paladin']
    self.metadata = metadata_lib.CBuildbotMetadata()
    self.db = fake_cidb.FakeCIDBConnection()
    self.buildbucket_client = mock.Mock()
    self.slave_1 = 'cyan-paladin'
    self.slave_2 = 'auron-paladin'
    self.builders_array = [self.slave_1, self.slave_2]

  def testGetAllSlaveBuildbucketInfo(self):
    """Test GetAllSlaveBuildbucketInfo."""

    # Test completed builds.
    buildbucket_info_dict = {
        'build1': buildbucket_lib.BuildbucketInfo(
            'id_1', 1, 0, None, None, None),
        'build2': buildbucket_lib.BuildbucketInfo(
            'id_2', 1, 0, None, None, None)
    }
    self.PatchObject(buildbucket_lib, 'GetScheduledBuildDict',
                     return_value=buildbucket_info_dict)

    expected_status = 'COMPLETED'
    expected_result = 'SUCCESS'
    expected_url = 'fake_url'
    content = {
        'build': {
            'status': expected_status,
            'result': expected_result,
            'url': expected_url
        }
    }

    self.buildbucket_client.GetBuildRequest.return_value = content
    updated_buildbucket_info_dict = (
        builder_status_lib.SlaveBuilderStatus.GetAllSlaveBuildbucketInfo(
            self.buildbucket_client, buildbucket_info_dict))

    self.assertEqual(updated_buildbucket_info_dict['build1'].status,
                     expected_status)
    self.assertEqual(updated_buildbucket_info_dict['build1'].result,
                     expected_result)
    self.assertEqual(updated_buildbucket_info_dict['build1'].url,
                     expected_url)
    self.assertEqual(updated_buildbucket_info_dict['build2'].status,
                     expected_status)
    self.assertEqual(updated_buildbucket_info_dict['build2'].result,
                     expected_result)
    self.assertEqual(updated_buildbucket_info_dict['build1'].url,
                     expected_url)

    # Test started builds.
    expected_status = 'STARTED'
    expected_result = None
    content = {
        'build': {
            'status': 'STARTED'
        }
    }
    self.buildbucket_client.GetBuildRequest.return_value = content
    updated_buildbucket_info_dict = (
        builder_status_lib.SlaveBuilderStatus.GetAllSlaveBuildbucketInfo(
            self.buildbucket_client, buildbucket_info_dict))

    self.assertEqual(updated_buildbucket_info_dict['build1'].status,
                     expected_status)
    self.assertEqual(updated_buildbucket_info_dict['build1'].result,
                     expected_result)
    self.assertEqual(updated_buildbucket_info_dict['build2'].status,
                     expected_status)
    self.assertEqual(updated_buildbucket_info_dict['build2'].result,
                     expected_result)

    # Test BuildbucketResponseException failures.
    self.buildbucket_client.GetBuildRequest.side_effect = (
        buildbucket_lib.BuildbucketResponseException)
    updated_buildbucket_info_dict = (
        builder_status_lib.SlaveBuilderStatus.GetAllSlaveBuildbucketInfo(
            self.buildbucket_client, buildbucket_info_dict))
    self.assertIsNone(updated_buildbucket_info_dict['build1'].status)
    self.assertIsNone(updated_buildbucket_info_dict['build2'].status)

  def _InsertMasterSlaveBuildsToCIDB(self):
    """Insert master and slave builds into fake_cidb."""
    master = self.db.InsertBuild('master', waterfall.WATERFALL_INTERNAL, 1,
                                 'master', 'host1')
    slave1 = self.db.InsertBuild('slave1', waterfall.WATERFALL_INTERNAL, 2,
                                 'slave1', 'host1', master_build_id=0,
                                 buildbucket_id='id_1', status='fail')
    slave2 = self.db.InsertBuild('slave2', waterfall.WATERFALL_INTERNAL, 3,
                                 'slave2', 'host1', master_build_id=0,
                                 buildbucket_id='id_2', status='fail')
    return master, slave1, slave2

  def testGetAllSlaveCIDBStatusInfo(self):
    """GetAllSlaveCIDBStatusInfo without Buildbucket info."""
    _, slave1_id, slave2_id = self._InsertMasterSlaveBuildsToCIDB()

    expected_status = {
        'slave1': builder_status_lib.CIDBStatusInfo(slave1_id, 'fail', 2),
        'slave2': builder_status_lib.CIDBStatusInfo(slave2_id, 'fail', 3)
    }

    cidb_status = (
        builder_status_lib.SlaveBuilderStatus.GetAllSlaveCIDBStatusInfo(
            self.db, self.master_build_id, None))
    self.assertDictEqual(cidb_status, expected_status)

    cidb_status = (
        builder_status_lib.SlaveBuilderStatus.GetAllSlaveCIDBStatusInfo(
            self.db, self.master_build_id, None))
    self.assertDictEqual(cidb_status, expected_status)

  def testGetAllSlaveCIDBStatusInfoWithBuildbucket(self):
    """GetAllSlaveCIDBStatusInfo with Buildbucket info."""
    _, slave1_id, slave2_id = self._InsertMasterSlaveBuildsToCIDB()

    buildbucket_info_dict = {
        'slave1': build_status_unittest.BuildbucketInfos.GetStartedBuild(
            bb_id='id_1'),
        'slave2': build_status_unittest.BuildbucketInfos.GetStartedBuild(
            bb_id='id_2')
    }

    expected_status = {
        'slave1': builder_status_lib.CIDBStatusInfo(slave1_id, 'fail', 2),
        'slave2': builder_status_lib.CIDBStatusInfo(slave2_id, 'fail', 3)
    }

    cidb_status = (
        builder_status_lib.SlaveBuilderStatus.GetAllSlaveCIDBStatusInfo(
            self.db, self.master_build_id, buildbucket_info_dict))
    self.assertDictEqual(cidb_status, expected_status)

    cidb_status = (
        builder_status_lib.SlaveBuilderStatus.GetAllSlaveCIDBStatusInfo(
            self.db, self.master_build_id, buildbucket_info_dict))
    self.assertDictEqual(cidb_status, expected_status)

  def testGetAllSlaveCIDBStatusInfoWithRetriedBuilds(self):
    """GetAllSlaveCIDBStatusInfo doesn't return retried builds."""
    self._InsertMasterSlaveBuildsToCIDB()
    self.db.InsertBuild('slave1', waterfall.WATERFALL_INTERNAL, 3,
                        'slave1', 'host1', master_build_id=0,
                        buildbucket_id='id_3', status='inflight')

    buildbucket_info_dict = {
        'slave1': build_status_unittest.BuildbucketInfos.GetStartedBuild(
            bb_id='id_3'),
        'slave2': build_status_unittest.BuildbucketInfos.GetStartedBuild(
            bb_id='id_4')
    }

    cidb_status = (
        builder_status_lib.SlaveBuilderStatus.GetAllSlaveCIDBStatusInfo(
            self.db, self.master_build_id, buildbucket_info_dict))
    self.assertEqual(set(cidb_status.keys()), set(['slave1']))
    self.assertEqual(cidb_status['slave1'].status, 'inflight')


  def ConstructBuilderStatusManager(self,
                                    master_build_id=None,
                                    db=None,
                                    config=None,
                                    metadata=None,
                                    buildbucket_client=None,
                                    builders_array=None,
                                    dry_run=True):
    if master_build_id is None:
      master_build_id = self.master_build_id
    if db is None:
      db = self.db
    if config is None:
      config = self.config
    if metadata is None:
      metadata = self.metadata
    if buildbucket_client is None:
      buildbucket_client = self.buildbucket_client
    if builders_array is None:
      builders_array = self.builders_array

    return builder_status_lib.SlaveBuilderStatus(
        master_build_id, db, config, metadata, buildbucket_client,
        builders_array, dry_run)

  def testGetSlaveFailures(self):
    """Test _GetSlaveFailures."""
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_InitSlaveInfo')
    entry_1 = stage_failure_helper.GetStageFailure(
        build_config=self.slave_1, failure_id=1)
    entry_2 = stage_failure_helper.GetStageFailure(
        build_config=self.slave_1, failure_id=2, outer_failure_id=1)
    entry_3 = stage_failure_helper.GetStageFailure(
        build_config=self.slave_2, failure_id=3)
    failure_entries = [entry_1, entry_2, entry_3]
    mock_db = mock.Mock()
    mock_db.GetSlaveFailures.return_value = failure_entries
    manager = self.ConstructBuilderStatusManager(db=mock_db)
    slave_failures_dict = manager._GetSlaveFailures(None)

    self.assertItemsEqual(slave_failures_dict.keys(),
                          [self.slave_1, self.slave_2])
    self.assertEqual(len(slave_failures_dict[self.slave_1]), 1)
    self.assertEqual(len(slave_failures_dict[self.slave_2]), 1)
    self.assertTrue(isinstance(slave_failures_dict[self.slave_1][0],
                               failure_message_lib.CompoundFailureMessage))
    self.assertTrue(isinstance(slave_failures_dict[self.slave_2][0],
                               failure_message_lib.StageFailureMessage))

  def testGetSlavesAbortedBySelfDestructionReturnsEmptySet(self):
    """Test GetSlavesAbortedBySelfDestruction returns an empty set."""
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_InitSlaveInfo')
    cidb_info_dict = cidb_infos.GetFullCIDBStatusInfo()
    manager = self.ConstructBuilderStatusManager()

    aborted_slaves = manager._GetSlavesAbortedBySelfDestruction(cidb_info_dict)
    self.assertEqual(aborted_slaves, set())

  def testGetSlavesAbortedBySelfDestructionWithAbortedBuilds(self):
    """Test GetSlavesAbortedBySelfDestruction with aborted builds."""
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_InitSlaveInfo')
    cidb_info_dict = cidb_infos.GetFullCIDBStatusInfo()
    self.db.InsertBuildMessage(
        self.master_build_id,
        message_type=constants.MESSAGE_TYPE_IGNORED_REASON,
        message_subtype=constants.MESSAGE_SUBTYPE_SELF_DESTRUCTION,
        message_value=str(3))
    manager = self.ConstructBuilderStatusManager()

    aborted_slaves = manager._GetSlavesAbortedBySelfDestruction(cidb_info_dict)
    self.assertEqual(aborted_slaves, {'completed_failure'})

  def testInitSlaveInfoWithBuildbucket(self):
    """Test _InitSlaveInfo with Buildbucket info."""
    self.PatchObject(config_lib, 'UseBuildbucketScheduler', return_value=True)
    self.PatchObject(builder_status_lib.SlaveBuilderStatus,
                     '_GetSlaveFailures')
    self.PatchObject(builder_status_lib.SlaveBuilderStatus,
                     'GetAllSlaveBuildbucketInfo',
                     return_value={self.slave_1: bb_infos.GetSuccessBuild()})
    manager = self.ConstructBuilderStatusManager()

    self.assertItemsEqual(manager.builders_array, [self.slave_1])
    self.assertIsNotNone(manager.buildbucket_info_dict)

  def testInitSlaveInfoOnBuildWithoutBuildbucket(self):
    """Test _InitSlaveInfo without Buildbucket info."""
    self.PatchObject(config_lib, 'UseBuildbucketScheduler', return_value=False)
    self.PatchObject(builder_status_lib.SlaveBuilderStatus,
                     '_GetSlaveFailures')
    self.PatchObject(builder_status_lib.SlaveBuilderStatus,
                     'GetAllSlaveBuildbucketInfo',
                     return_value={self.slave_1: bb_infos.GetSuccessBuild()})
    manager = self.ConstructBuilderStatusManager()

    self.assertItemsEqual(manager.builders_array, [self.slave_1, self.slave_2])
    self.assertIsNone(manager.buildbucket_info_dict)

  def testGetStatusWithBuildbucket(self):
    """Test _GetStatus with Buildbucket info."""
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_InitSlaveInfo')
    cidb_info_dict = cidb_infos.GetFullCIDBStatusInfo()
    buildbucket_info_dict = bb_infos.GetFullBuildbucketInfoDict()
    manager = self.ConstructBuilderStatusManager()

    status_1 = manager._GetStatus(
        'scheduled', cidb_info_dict, buildbucket_info_dict)
    self.assertEqual(status_1, constants.BUILDER_STATUS_MISSING)
    status_2 = manager._GetStatus(
        'started', cidb_info_dict, buildbucket_info_dict)
    self.assertEqual(status_2, constants.BUILDER_STATUS_INFLIGHT)
    status_3 = manager._GetStatus(
        'completed_success', cidb_info_dict, buildbucket_info_dict)
    self.assertEqual(status_3, constants.BUILDER_STATUS_PASSED)
    status_4 = manager._GetStatus(
        'completed_failure', cidb_info_dict, buildbucket_info_dict)
    self.assertEqual(status_4, constants.BUILDER_STATUS_FAILED)
    status_5 = manager._GetStatus(
        'completed_canceled', cidb_info_dict, buildbucket_info_dict)
    self.assertEqual(status_5, constants.BUILDER_STATUS_FAILED)

  def testGetStatusWithoutBuildbucket(self):
    """Test _GetStatus without Buildbucket info."""
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_InitSlaveInfo')
    cidb_info_dict = cidb_infos.GetFullCIDBStatusInfo()
    manager = self.ConstructBuilderStatusManager()

    status_1 = manager._GetStatus('started', cidb_info_dict, None)
    self.assertEqual(status_1, constants.BUILDER_STATUS_INFLIGHT)
    status_2 = manager._GetStatus('completed_canceled', cidb_info_dict, None)
    self.assertEqual(status_2, constants.BUILDER_STATUS_INFLIGHT)

  def test_GetDashboardUrlWithBuildbucket(self):
    """Test _GetDashboardUrl with Buildbucket info."""
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_InitSlaveInfo')
    manager = self.ConstructBuilderStatusManager()
    cidb_info_dict = {
        self.slave_1: cidb_infos.GetPassedBuild(build_id=1, build_number=100)}
    buildbucket_info_dict = {
        self.slave_1: bb_infos.GetSuccessBuild(url='http://buildbucket_url'),
        self.slave_2:  bb_infos.GetSuccessBuild(url='http://buildbucket_url'),
    }

    dashboard_url = manager._GetDashboardUrl(
        self.slave_1, cidb_info_dict, buildbucket_info_dict)
    self.assertEqual(
        dashboard_url,
        'https://luci-milo.appspot.com/buildbot/chromeos/cyan-paladin/100')

    dashboard_url = manager._GetDashboardUrl(
        self.slave_2, cidb_info_dict, buildbucket_info_dict)
    self.assertEqual(dashboard_url, 'http://buildbucket_url')

  def testGetDashboardUrlWithoutBuildbucket(self):
    """Test _GetDashboardUrl without Buildbucket info."""
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_InitSlaveInfo')
    manager = self.ConstructBuilderStatusManager()

    dashboard_url = manager._GetDashboardUrl(self.slave_1, {}, None)
    self.assertIsNone(dashboard_url)

  def testGetMessageOnFailedBuilds(self):
    """Test _GetMessage on failed builds."""
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_InitSlaveInfo')
    mock_create_msg = self.PatchObject(builder_status_lib.BuilderStatusManager,
                                       'CreateBuildFailureMessage')
    manager = self.ConstructBuilderStatusManager()
    failure_messages = ConstructFailureMessages(self.slave_1)
    slave_failures_dict = {self.slave_1: failure_messages}
    aborted_slaves = {self.slave_1, self.slave_2}
    manager._GetMessage(
        self.slave_1, constants.BUILDER_STATUS_FAILED, 'dashboard_url',
        slave_failures_dict, aborted_slaves)

    mock_create_msg.assert_called_with(
        self.slave_1, mock.ANY, 'dashboard_url',
        slave_failures_dict.get(self.slave_1), aborted_by_self_destruction=True)

  def testGetMessageOnNotFailedBuilds(self):
    """Test _GetMessage on not failed builds."""
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_InitSlaveInfo')
    mock_create_msg = self.PatchObject(builder_status_lib.BuilderStatusManager,
                                       'CreateBuildFailureMessage')
    manager = self.ConstructBuilderStatusManager()
    manager._GetMessage(
        self.slave_1, constants.BUILDER_STATUS_PASSED, 'dashboard_url', {},
        set())

    mock_create_msg.assert_not_called()

  def testGetBuilderStatusForBuild(self):
    """Test GetBuilderStatusForBuild."""
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_InitSlaveInfo')
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_GetStatus')
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_GetDashboardUrl')
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '_GetMessage')
    manager = self.ConstructBuilderStatusManager()
    self.assertIsNotNone(manager.GetBuilderStatusForBuild(self.slave_1))

  def testCancelBuilds(self):
    """Test CancelBuilds."""
    buildbucket_id_1 = '100'
    buildbucket_id_2 = '200'
    buildbucket_ids = [buildbucket_id_1, buildbucket_id_2]

    cancel_mock = self.buildbucket_client.CancelBatchBuildsRequest

    cancel_mock.return_value = dict()
    builder_status_lib.CancelBuilds(buildbucket_ids, self.buildbucket_client)

    self.assertEqual(cancel_mock.call_count, 1)

  def testCancelNoBuilds(self):
    """Test CancelBuilds with no builds to cancel."""
    cancel_mock = self.buildbucket_client.CancelBatchBuildsRequest

    builder_status_lib.CancelBuilds([], self.buildbucket_client)
    self.assertEqual(cancel_mock.call_count, 0)

  def testCancelWithBuildbucketClient(self):
    """Test Buildbucket client cancels successfully during CancelBuilds."""
    buildbucket_id_1 = '100'
    buildbucket_id_2 = '200'
    buildbucket_ids = [buildbucket_id_1, buildbucket_id_2]

    self.PatchObject(buildbucket_lib, 'GetServiceAccount',
                     return_value=True)
    send_request_mock = self.PatchObject(buildbucket_lib.BuildbucketClient,
                                         'SendBuildbucketRequest',
                                         return_value=dict())
    buildbucket_client = buildbucket_lib.BuildbucketClient(
        mock.Mock(), mock.Mock())

    builder_status_lib.CancelBuilds(buildbucket_ids, buildbucket_client)

    self.assertEqual(send_request_mock.call_count, 1)

  def testAbortedBySelfDestruction(self):
    """Test that self-destructive aborts in CIDB are recognized."""
    slave = 'cyan-paladin'
    builder_number = 37

    slave_id = self.db.InsertBuild(slave, waterfall.WATERFALL_INTERNAL,
                                   builder_number, slave, 'bot_hostname',
                                   master_build_id=self.master_build_id)
    self.db.InsertBuildMessage(
        self.master_build_id,
        message_type=constants.MESSAGE_TYPE_IGNORED_REASON,
        message_subtype=constants.MESSAGE_SUBTYPE_SELF_DESTRUCTION,
        message_value=str(slave_id))

    self.assertTrue(
        builder_status_lib.BuilderStatusManager.AbortedBySelfDestruction(
            self.db, slave_id, self.master_build_id))

  def testNotAbortedBySelfDestruction(self):
    """Test that aborts in CIDB are only flagged if they happened."""
    slave = 'cyan-paladin'
    builder_number = 37

    slave_id = self.db.InsertBuild(slave, waterfall.WATERFALL_INTERNAL,
                                   builder_number, slave, 'bot_hostname',
                                   master_build_id=self.master_build_id)
    self.db.InsertBuildMessage(self.master_build_id,
                               message_value=slave_id)
    builder_number = 1

    self.db.InsertBuild(slave, waterfall.WATERFALL_INTERNAL,
                        builder_number, slave, 'bot_hostname')
    self.db.InsertBuildMessage(slave)

    self.assertFalse(
        builder_status_lib.BuilderStatusManager.AbortedBySelfDestruction(
            self.db, slave_id, self.master_build_id))

  def testAbortedBySelfDestructionOnBuildWithoutMaster(self):
    """AbortedBySelfDestruction returns False on builds without master."""
    self.assertFalse(
        builder_status_lib.BuilderStatusManager.AbortedBySelfDestruction(
            self.db, 1, None))


class BuilderStatusesFetcherTests(cros_test_lib.MockTestCase):
  """Tests for BuilderStatusesFetcher."""

  def setUp(self):
    self.build_id = 0
    self.db = fake_cidb.FakeCIDBConnection()
    self.site_config = config_lib.GetConfig()
    self.config = self.site_config['master-paladin']
    self.slave_config = self.site_config['eve-paladin']
    self.message = 'build message'
    self.metadata = metadata_lib.CBuildbotMetadata()
    self.buildbucket_client = mock.Mock()
    self.slave_1 = 'cyan-paladin'
    self.slave_2 = 'auron-paladin'
    self.builders_array = [self.slave_1, self.slave_2]

  def CreateBuilderStatusesFetcher(
      self, build_id=None, db=None, success=True, message=None, config=None,
      metadata=None, buildbucket_client=None, builders_array=None,
      dry_run=True):
    build_id = build_id or self.build_id
    db = db or self.db
    message = message or self.message
    config = config or self.config
    metadata = metadata or self.metadata
    buildbucket_client = buildbucket_client or self.buildbucket_client
    builders_array = (builders_array if builders_array is not None
                      else self.builders_array)
    self.PatchObject(buildbucket_lib, 'FetchCurrentSlaveBuilders',
                     return_value=builders_array)
    builder_statuses_fetcher = builder_status_lib.BuilderStatusesFetcher(
        build_id, db, success, message, config, metadata, buildbucket_client,
        builders_array=builders_array, dry_run=dry_run)

    return builder_statuses_fetcher

  def _PatchesForGetSlaveBuilderStatus(self, status_dict):
    self.PatchObject(builder_status_lib.SlaveBuilderStatus, '__init__',
                     return_value=None)
    message_mock = mock.Mock()
    message_mock.BuildFailureMessageToStr.return_value = 'failure_message_str'
    build_statuses = {
        x: builder_status_lib.BuilderStatus(
            status_dict[x].status, message_mock) for x in status_dict
    }
    self.PatchObject(builder_status_lib.SlaveBuilderStatus,
                     'GetBuilderStatusForBuild',
                     side_effect=lambda config: build_statuses[config])

  def testFetchLocalBuilderStatus(self):
    """Test _FetchLocalBuilderStatus."""
    fetcher = self.CreateBuilderStatusesFetcher()

    local_builder_status = fetcher._FetchLocalBuilderStatus()
    self.assertTrue('master-paladin' in local_builder_status.keys())

  def testFetchSlaveBuilderStatusesWithEmptySlaveList(self):
    """Test _FetchSlaveBuilderStatuses with an empty slave list."""
    self._PatchesForGetSlaveBuilderStatus({})
    fetcher = self.CreateBuilderStatusesFetcher(builders_array=[])
    statuses = fetcher._FetchSlaveBuilderStatuses()
    self.assertEqual(statuses, {})

  def testFetchSlaveBuildersStatusesWithSlaveList(self):
    """Test _FetchSlaveBuilderStatuses with a slave list."""
    status_dict = {
        'build1': build_status_unittest.CIDBStatusInfos.GetFailedBuild(
            build_id=1),
        'build2': build_status_unittest.CIDBStatusInfos.GetPassedBuild(
            build_id=2),
        'build3': build_status_unittest.CIDBStatusInfos.GetInflightBuild(
            build_id=3)
    }
    self._PatchesForGetSlaveBuilderStatus(status_dict)
    fetcher = self.CreateBuilderStatusesFetcher(
        builders_array=['build1', 'build2', 'build3'])
    statuses = fetcher._FetchSlaveBuilderStatuses()
    self.assertTrue(statuses['build1'].Failed())
    self.assertTrue(statuses['build2'].Passed())
    self.assertTrue(statuses['build3'].Inflight())

  def testGetBuilderStatusesOnSlaves(self):
    """Test GetBuilderStatuses on slaves."""
    fetcher = self.CreateBuilderStatusesFetcher(
        config=self.slave_config, builders_array=[])

    important_statuses, experimental_statuses = fetcher.GetBuilderStatuses()
    self.assertEqual(len(important_statuses), 1)
    self.assertEqual(len(experimental_statuses), 0)
    self.assertTrue(important_statuses['eve-paladin'].Passed())

  def testGetBuilderStatusesOnMasters(self):
    """Test GetBuilderStatuses on masters."""
    status_dict = {
        'build1': build_status_unittest.CIDBStatusInfos.GetFailedBuild(
            build_id=1),
        'build2': build_status_unittest.CIDBStatusInfos.GetPassedBuild(
            build_id=2),
        'build3': build_status_unittest.CIDBStatusInfos.GetInflightBuild(
            build_id=3)
    }
    self._PatchesForGetSlaveBuilderStatus(status_dict)
    fetcher = self.CreateBuilderStatusesFetcher(
        builders_array=status_dict.keys())

    important, experimental = fetcher.GetBuilderStatuses()
    self.assertItemsEqual(['build1', 'build2', 'build3', 'master-paladin'],
                          important.keys())
    self.assertItemsEqual([], experimental.keys())

    # Update the experimental_builders in metadata
    self.metadata.UpdateWithDict({
        constants.METADATA_EXPERIMENTAL_BUILDERS: ['build1']
    })
    important, experimental = fetcher.GetBuilderStatuses()
    self.assertItemsEqual(['build2', 'build3', 'master-paladin'],
                          important.keys())
    self.assertItemsEqual(['build1'], experimental.keys())

  def _CreateBuilderStatusDict(self):
    passed = builder_status_lib.BuilderStatus(
        constants.BUILDER_STATUS_PASSED, None)
    failed = builder_status_lib.BuilderStatus(
        constants.BUILDER_STATUS_FAILED, mock.Mock())
    inflight = builder_status_lib.BuilderStatus(
        constants.BUILDER_STATUS_INFLIGHT, mock.Mock())
    missing = builder_status_lib.BuilderStatus(
        constants.BUILDER_STATUS_MISSING, None)
    return {'passed': passed,
            'failed': failed,
            'inflight': inflight,
            'missing': missing}

  def testGetFailingBuilds(self):
    """Test GetFailingBuilds."""
    statuses = self._CreateBuilderStatusDict()
    self.assertEqual(
        builder_status_lib.BuilderStatusesFetcher.GetFailingBuilds(statuses),
        {'failed'})

  def testGetInflightBuilds(self):
    """Test GetInflightBuilds."""
    statuses = self._CreateBuilderStatusDict()
    self.assertEqual(
        builder_status_lib.BuilderStatusesFetcher.GetInflightBuilds(statuses),
        {'inflight'})

  def testGetNostatBuilds(self):
    """Test GetNostatBuilds."""
    statuses = self._CreateBuilderStatusDict()
    self.assertEqual(
        builder_status_lib.BuilderStatusesFetcher.GetNostatBuilds(statuses),
        {'missing'})
