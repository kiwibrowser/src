# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for completion stages."""

from __future__ import print_function

import mock
import sys

from chromite.cbuildbot import cbuildbot_run
from chromite.cbuildbot import commands
from chromite.cbuildbot import manifest_version
from chromite.cbuildbot import prebuilts
from chromite.cbuildbot.stages import completion_stages
from chromite.cbuildbot.stages import generic_stages
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.cbuildbot.stages import sync_stages_unittest
from chromite.cbuildbot.stages import sync_stages
from chromite.lib.const import waterfall
from chromite.lib import alerts
from chromite.lib import auth
from chromite.lib import buildbucket_lib
from chromite.lib import builder_status_lib
from chromite.lib import build_failure_message
from chromite.lib import cidb
from chromite.lib import cros_logging as logging
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import tree_status


# pylint: disable=protected-access


class ManifestVersionedSyncCompletionStageTest(
    sync_stages_unittest.ManifestVersionedSyncStageTest):
  """Tests the ManifestVersionedSyncCompletion stage."""

  # pylint: disable=abstract-method

  BOT_ID = 'x86-mario-release'


  def testManifestVersionedSyncCompletedSuccess(self):
    """Tests basic ManifestVersionedSyncStageCompleted on success"""
    board_runattrs = self._run.GetBoardRunAttrs('x86-mario')
    board_runattrs.SetParallel('success', True)
    update_status_mock = self.PatchObject(
        manifest_version.BuildSpecsManager, 'UpdateStatus')

    stage = completion_stages.ManifestVersionedSyncCompletionStage(
        self._run, self.sync_stage, success=True)

    stage.Run()
    update_status_mock.assert_called_once_with(
        message=None, success_map={self.BOT_ID: True})

  def testManifestVersionedSyncCompletedFailure(self):
    """Tests basic ManifestVersionedSyncStageCompleted on failure"""
    stage = completion_stages.ManifestVersionedSyncCompletionStage(
        self._run, self.sync_stage, success=False)
    message = 'foo'
    get_msg_mock = self.PatchObject(
        generic_stages.BuilderStage, 'GetBuildFailureMessage',
        return_value=message)
    update_status_mock = self.PatchObject(
        manifest_version.BuildSpecsManager, 'UpdateStatus')

    stage.Run()
    update_status_mock.assert_called_once_with(
        message='foo', success_map={self.BOT_ID: False})
    get_msg_mock.assert_called_once_with()


  def testManifestVersionedSyncCompletedIncomplete(self):
    """Tests basic ManifestVersionedSyncStageCompleted on incomplete build."""
    stage = completion_stages.ManifestVersionedSyncCompletionStage(
        self._run, self.sync_stage, success=False)
    stage.Run()

  def testGetBuilderSuccessMap(self):
    """Tests that the builder success map is properly created."""
    board_runattrs = self._run.GetBoardRunAttrs('x86-mario')
    board_runattrs.SetParallel('success', True)
    builder_success_map = completion_stages.GetBuilderSuccessMap(
        self._run, True)
    expected_map = {self.BOT_ID: True}
    self.assertEqual(expected_map, builder_success_map)


class MasterSlaveSyncCompletionStageMockConfigTest(
    generic_stages_unittest.AbstractStageTestCase):
  """Tests MasterSlaveSyncCompletionStage with ManifestVersionedSyncStage."""
  BOT_ID = 'master'

  def setUp(self):
    self.source_repo = 'ssh://source/repo'
    self.manifest_version_url = 'fake manifest url'
    self.branch = 'master'
    self.build_type = constants.PFQ_TYPE

    # Use our mocked out SiteConfig for all tests.
    self.test_config = self._GetTestConfig()
    self._Prepare(site_config=self.test_config)

  def ConstructStage(self):
    sync_stage = sync_stages.ManifestVersionedSyncStage(self._run)
    return completion_stages.MasterSlaveSyncCompletionStage(
        self._run, sync_stage, success=True)

  def _GetTestConfig(self):
    test_config = config_lib.SiteConfig()
    test_config.Add(
        'master',
        config_lib.BuildConfig(),
        boards=[],
        build_type=self.build_type,
        master=True,
        slave_configs=['test3', 'test5'],
        manifest_version=True,
        active_waterfall=waterfall.WATERFALL_INTERNAL,
    )
    test_config.Add(
        'test1',
        config_lib.BuildConfig(),
        boards=['amd64-generic'],
        manifest_version=True,
        build_type=constants.PFQ_TYPE,
        overlays='public',
        important=False,
        chrome_rev=None,
        branch=False,
        internal=False,
        master=False,
        active_waterfall=waterfall.WATERFALL_INTERNAL,
    )
    test_config.Add(
        'test2',
        config_lib.BuildConfig(),
        boards=['amd64-generic'],
        manifest_version=False,
        build_type=constants.PFQ_TYPE,
        overlays='public',
        important=True,
        chrome_rev=None,
        branch=False,
        internal=False,
        master=False,
        active_waterfall=waterfall.WATERFALL_INTERNAL,
    )
    test_config.Add(
        'test3',
        config_lib.BuildConfig(),
        boards=['amd64-generic'],
        manifest_version=True,
        build_type=constants.PFQ_TYPE,
        overlays='both',
        important=True,
        chrome_rev=None,
        branch=False,
        internal=True,
        master=False,
        active_waterfall=waterfall.WATERFALL_INTERNAL,
    )
    test_config.Add(
        'test4',
        config_lib.BuildConfig(),
        boards=['amd64-generic'],
        manifest_version=True,
        build_type=constants.PFQ_TYPE,
        overlays='both',
        important=True,
        chrome_rev=None,
        branch=True,
        internal=True,
        master=False,
        active_waterfall=waterfall.WATERFALL_INTERNAL,
    )
    test_config.Add(
        'test5',
        config_lib.BuildConfig(),
        boards=['amd64-generic'],
        manifest_version=True,
        build_type=constants.PFQ_TYPE,
        overlays='public',
        important=True,
        chrome_rev=None,
        branch=False,
        internal=False,
        master=False,
        active_waterfall=waterfall.WATERFALL_INTERNAL,
    )
    return test_config

  def testGetSlavesForMaster(self):
    """Tests that we get the slaves for a fake unified master configuration."""
    self.maxDiff = None
    stage = self.ConstructStage()
    p = stage._GetSlaveConfigs()
    self.assertEqual([self.test_config['test3'], self.test_config['test5']], p)


class MasterSlaveSyncCompletionStageTest(
    generic_stages_unittest.AbstractStageTestCase):
  """Tests MasterSlaveSyncCompletionStage with ManifestVersionedSyncStage."""
  BOT_ID = 'amd64-generic-paladin'

  def setUp(self):
    self.source_repo = 'ssh://source/repo'
    self.manifest_version_url = 'fake manifest url'
    self.branch = 'master'

    self._Prepare()

  def _Prepare(self, bot_id=None, **kwargs):
    super(MasterSlaveSyncCompletionStageTest, self)._Prepare(bot_id, **kwargs)
    self._run.config['manifest_version'] = True

  def ConstructStage(self):
    sync_stage = sync_stages.ManifestVersionedSyncStage(self._run)
    return completion_stages.MasterSlaveSyncCompletionStage(
        self._run, sync_stage, success=True)

  def testIsFailureFatal(self):
    """Tests the correctness of the _IsFailureFatal method"""
    stage = self.ConstructStage()

    # Test behavior when there are no sanity check builders
    self.assertFalse(stage._IsFailureFatal(set(), set(), set()))
    self.assertTrue(stage._IsFailureFatal(set(['test3']), set(), set()))
    self.assertTrue(stage._IsFailureFatal(set(), set(['test5']), set()))
    self.assertTrue(stage._IsFailureFatal(set(), set(), set(['test1'])))

  def testExceptionHandler(self):
    """Verify _HandleStageException is sane."""
    stage = self.ConstructStage()
    e = ValueError('foo')
    try:
      raise e
    except ValueError:
      ret = stage._HandleStageException(sys.exc_info())
      self.assertTrue(isinstance(ret, tuple))
      self.assertEqual(len(ret), 3)
      self.assertEqual(ret[0], e)

  def testGetBuilderStatusesFetcher(self):
    """Test GetBuilderStatusesFetcher."""
    mock_fetcher = mock.Mock()
    self.PatchObject(
        builder_status_lib, 'BuilderStatusesFetcher',
        return_value=mock_fetcher)

    stage = self.ConstructStage()
    self.assertEqual(stage._GetBuilderStatusesFetcher(), mock_fetcher)

class MasterSlaveSyncCompletionStageTestWithMasterPaladin(
    generic_stages_unittest.AbstractStageTestCase):
  """Tests MasterSlaveSyncCompletionStage with master-paladin."""
  BOT_ID = 'master-paladin'

  def setUp(self):
    self.source_repo = 'ssh://source/repo'
    self.manifest_version_url = 'fake manifest url'
    self.branch = 'master'

    self.PatchObject(buildbucket_lib, 'GetServiceAccount',
                     return_value=True)
    self.PatchObject(auth.AuthorizedHttp, '__init__',
                     return_value=None)
    self.PatchObject(buildbucket_lib.BuildbucketClient,
                     'SendBuildbucketRequest',
                     return_value=None)
    self.PatchObject(buildbucket_lib.BuildbucketClient,
                     '_GetHost',
                     return_value=buildbucket_lib.BUILDBUCKET_TEST_HOST)
    self.mock_handle_failure = self.PatchObject(
        completion_stages.MasterSlaveSyncCompletionStage, 'HandleFailure')
    self.PatchObject(builder_status_lib.BuilderStatusesFetcher, '__init__',
                     return_value=None)

    self._Prepare()

  def tearDown(self):
    cidb.CIDBConnectionFactory.ClearMock()

  def ConstructStage(self):
    sync_stage = sync_stages.ManifestVersionedSyncStage(self._run)

    scheduled_slaves_list = {
        ('build_1', 'buildbucket_id_1', 1),
        ('build_2', 'buildbucket_id_2', 2)
    }
    self._run.attrs.metadata.ExtendKeyListWithList(
        constants.METADATA_SCHEDULED_IMPORTANT_SLAVES, scheduled_slaves_list)
    return completion_stages.MasterSlaveSyncCompletionStage(
        self._run, sync_stage, success=True)

  def testGetBuilderStatusesFetcher(self):
    """Test GetBuilderStatusesFetcher."""
    mock_fetcher = mock.Mock()
    self.PatchObject(
        builder_status_lib, 'BuilderStatusesFetcher',
        return_value=mock_fetcher)
    mock_wait = self.PatchObject(
        completion_stages.MasterSlaveSyncCompletionStage,
        '_WaitForSlavesToComplete')
    stage = self.ConstructStage()
    stage._run.attrs.manifest_manager = mock.Mock()

    self.assertEqual(stage._GetBuilderStatusesFetcher(), mock_fetcher)
    self.assertEqual(mock_wait.call_count, 1)

  def testPerformStageWithFatalFailure(self):
    """Test PerformStage on master-paladin."""
    stage = self.ConstructStage()

    stage._run.attrs.manifest_manager = mock.MagicMock()

    statuses = {
        'build_1': builder_status_lib.BuilderStatus(
            constants.BUILDER_STATUS_INFLIGHT, None),
        'build_2': builder_status_lib.BuilderStatus(
            constants.BUILDER_STATUS_MISSING, None)
    }

    self.PatchObject(builder_status_lib.BuilderStatusesFetcher,
                     'GetBuilderStatuses', return_value=(statuses, {}))
    mock_annotate = self.PatchObject(
        completion_stages.MasterSlaveSyncCompletionStage,
        '_AnnotateFailingBuilders')

    with self.assertRaises(completion_stages.ImportantBuilderFailedException):
      stage.PerformStage()
    mock_annotate.assert_called_once_with(
        set(), {'build_1'}, {'build_2'}, statuses, {}, False)
    self.mock_handle_failure.assert_called_once_with(
        set(), {'build_1'}, {'build_2'}, False)

    mock_annotate.reset_mock()
    self.mock_handle_failure.reset_mock()
    stage._run.attrs.metadata.UpdateWithDict(
        {constants.SELF_DESTRUCTED_BUILD: True})
    with self.assertRaises(completion_stages.ImportantBuilderFailedException):
      stage.PerformStage()
    mock_annotate.assert_called_once_with(
        set(), {'build_1'}, {'build_2'}, statuses, {}, True)
    self.mock_handle_failure.assert_called_once_with(
        set(), {'build_1'}, {'build_2'}, True)

  def testStageRunWithImportantBuilderFailedException(self):
    """Test stage.Run on master-paladin with ImportantBuilderFailedException."""
    stage = self.ConstructStage()
    stage._run.attrs.manifest_manager = mock.MagicMock()
    statuses = {
        'build_1': builder_status_lib.BuilderStatus(
            constants.BUILDER_STATUS_INFLIGHT, None),
        'build_2': builder_status_lib.BuilderStatus(
            constants.BUILDER_STATUS_MISSING, None)
    }
    self.PatchObject(builder_status_lib.BuilderStatusesFetcher,
                     'GetBuilderStatuses', return_value=(statuses, {}))

    with self.assertRaises(completion_stages.ImportantBuilderFailedException):
      stage.Run()

  def testAnnotateBuildStatusFromBuildbucket(self):
    """Test AnnotateBuildStatusFromBuildbucket"""
    stage = self.ConstructStage()

    scheduled_slaves_list = {
        ('build_1', 'buildbucket_id_1', 1),
        ('build_2', 'buildbucket_id_2', 2)
    }
    stage.buildbucket_info_dict = (
        buildbucket_lib.GetScheduledBuildDict(scheduled_slaves_list))

    mock_logging_link = self.PatchObject(
        logging, 'PrintBuildbotLink',
        side_effect=logging.PrintBuildbotLink)
    mock_logging_text = self.PatchObject(
        logging, 'PrintBuildbotStepText',
        side_effect=logging.PrintBuildbotStepText)

    no_stat = set(['not_scheduled_build_1'])
    stage._AnnotateBuildStatusFromBuildbucket(no_stat)
    mock_logging_text.assert_called_once_with(
        '%s wasn\'t scheduled by master.' % 'not_scheduled_build_1')

    build_content = {
        'build': {
            'status': 'COMPLETED',
            'result': 'FAILURE',
            'url': 'dashboard_url',
            "failure_reason": "BUILD_FAILURE",
        }
    }
    self.PatchObject(buildbucket_lib.BuildbucketClient,
                     'GetBuildRequest',
                     return_value=build_content)
    no_stat = set(['build_1'])
    stage._AnnotateBuildStatusFromBuildbucket(no_stat)
    mock_logging_link.assert_called_once_with(
        '%s: [status] %s [result] %s [failure_reason] %s' %
        ('build_1', 'COMPLETED', 'FAILURE', 'BUILD_FAILURE'),
        'dashboard_url')

    mock_logging_link.reset_mock()
    build_content = {
        'build': {
            'status': 'COMPLETED',
            'result': 'CANCELED',
            'url': 'dashboard_url',
            "cancelation_reason": "CANCELED_EXPLICITLY",
        }
    }
    self.PatchObject(buildbucket_lib.BuildbucketClient,
                     'GetBuildRequest',
                     return_value=build_content)
    no_stat = set(['build_1'])
    stage._AnnotateBuildStatusFromBuildbucket(no_stat)
    mock_logging_link.assert_called_once_with(
        '%s: [status] %s [result] %s [cancelation_reason] %s' %
        ('build_1', 'COMPLETED', 'CANCELED', 'CANCELED_EXPLICITLY'),
        'dashboard_url')


    mock_logging_text.reset_mock()
    self.PatchObject(buildbucket_lib.BuildbucketClient,
                     'GetBuildRequest',
                     side_effect=buildbucket_lib.BuildbucketResponseException)
    no_stat = set(['build_1'])
    stage._AnnotateBuildStatusFromBuildbucket(no_stat)
    mock_logging_text.assert_called_once_with(
        'No status found for build %s buildbucket_id %s' %
        ('build_1', 'buildbucket_id_1'))

  def test_AnnotateNoStatBuildersWithBuildbucketSchedulder(self):
    """Tests _AnnotateNoStatBuilders with master using Buildbucket scheduler."""
    stage = self.ConstructStage()

    annotate_mock = self.PatchObject(
        completion_stages.MasterSlaveSyncCompletionStage,
        '_AnnotateBuildStatusFromBuildbucket')

    no_stat = {'no_stat_1', 'no_stat_2'}
    stage._AnnotateNoStatBuilders(no_stat)
    annotate_mock.assert_called_once_with(no_stat)

  def testAnnotateFailingBuilders(self):
    """Tests that _AnnotateFailingBuilders is free of syntax errors."""
    stage = self.ConstructStage()

    annotate_mock = self.PatchObject(
        completion_stages.MasterSlaveSyncCompletionStage,
        '_AnnotateNoStatBuilders')

    failing = {'failing_build'}
    inflight = {'inflight_build'}
    no_stat = {'no_stat_build'}
    failed_msg = build_failure_message.BuildFailureMessage(
        'message', [], True, 'reason', 'bot')
    failed_status = builder_status_lib.BuilderStatus(
        'failed', failed_msg, 'url')
    inflight_status = builder_status_lib.BuilderStatus('inflight', None, 'url')
    statuses = {'failing_build' : failed_status,
                'inflight_build': inflight_status}

    stage._AnnotateFailingBuilders(failing, inflight, set(), statuses, {},
                                   False)
    self.assertEqual(annotate_mock.call_count, 1)

    stage._AnnotateFailingBuilders(failing, inflight, no_stat, statuses, {},
                                   False)
    self.assertEqual(annotate_mock.call_count, 2)

    stage._AnnotateFailingBuilders(failing, inflight, no_stat, statuses, {},
                                   True)
    self.assertEqual(annotate_mock.call_count, 3)

  def testAnnotateFailingExperimentalBuilders(self):
    """Tests _AnnotateFailingBuilders with experimental builders."""
    stage = self.ConstructStage()

    print_build_message_mock = self.PatchObject(
        completion_stages.MasterSlaveSyncCompletionStage,
        '_PrintBuildMessage')

    failed_msg = build_failure_message.BuildFailureMessage(
        'message', [], True, 'reason', 'bot')
    experimental_statuses = {
        'passed_experimental' : builder_status_lib.BuilderStatus(
            constants.BUILDER_STATUS_PASSED, None, 'url'),
        'failing_experimental' : builder_status_lib.BuilderStatus(
            constants.BUILDER_STATUS_FAILED, failed_msg, 'url'),
        'inflight_experimental': builder_status_lib.BuilderStatus(
            constants.BUILDER_STATUS_INFLIGHT, None, 'url')
    }

    stage._AnnotateFailingBuilders(set(), set(), set(), {},
                                   experimental_statuses, False)
    # Build message should not be printed for the passed builder.
    self.assertEqual(print_build_message_mock.call_count, 2)

  def testPerformStageWithFailedExperimentalBuilder(self):
    """Test PerformStage with a failed experimental builder."""
    stage = self.ConstructStage()
    stage._run.attrs.manifest_manager = mock.MagicMock()
    status = {
        'build_1': builder_status_lib.BuilderStatus(
            constants.BUILDER_STATUS_PASSED, None)
    }
    experimental_status = {
        'build_2': builder_status_lib.BuilderStatus(
            constants.BUILDER_STATUS_FAILED, None)
    }
    self.PatchObject(builder_status_lib.BuilderStatusesFetcher,
                     'GetBuilderStatuses',
                     return_value=(status, experimental_status))
    mock_annotate = self.PatchObject(
        completion_stages.MasterSlaveSyncCompletionStage,
        '_AnnotateFailingBuilders')

    stage._run.attrs.metadata.UpdateWithDict(
        {constants.METADATA_EXPERIMENTAL_BUILDERS: ['build_2']})
    stage.PerformStage()
    mock_annotate.assert_called_once_with(
        set(), set(), set(), status, experimental_status, False)

class CanaryCompletionStageTest(
    generic_stages_unittest.AbstractStageTestCase):
  """Tests how canary master handles failures in CanaryCompletionStage."""
  BOT_ID = 'master-release'

  def _Prepare(self, bot_id=BOT_ID, **kwargs):
    super(CanaryCompletionStageTest, self)._Prepare(bot_id, **kwargs)

  def setUp(self):
    self.build_type = constants.CANARY_TYPE
    self._Prepare()

  def ConstructStage(self):
    """Returns a CanaryCompletionStage object."""
    sync_stage = sync_stages.ManifestVersionedSyncStage(self._run)
    return completion_stages.CanaryCompletionStage(
        self._run, sync_stage, success=True)

  def testComposeTreeStatusMessage(self):
    """Tests that the status message is constructed as expected."""
    failing = ['foo1', 'foo2', 'foo3', 'foo4', 'foo5']
    inflight = ['bar']
    no_stat = []
    stage = self.ConstructStage()
    self.assertEqual(
        stage._ComposeTreeStatusMessage(failing, inflight, no_stat),
        'bar timed out; foo1,foo2 and 3 others failed')

  def testGetBuilderStatusesFetcher(self):
    """Test GetBuilderStatusesFetcher."""
    mock_fetcher = mock.Mock()
    self.PatchObject(
        builder_status_lib, 'BuilderStatusesFetcher',
        return_value=mock_fetcher)
    mock_wait = self.PatchObject(
        completion_stages.MasterSlaveSyncCompletionStage,
        '_WaitForSlavesToComplete')
    stage = self.ConstructStage()
    stage._run.attrs.manifest_manager = mock.Mock()

    self.assertEqual(stage._GetBuilderStatusesFetcher(), mock_fetcher)
    self.assertEqual(mock_wait.call_count, 1)


class BaseCommitQueueCompletionStageTest(
    generic_stages_unittest.AbstractStageTestCase):
  """Tests how CQ handles changes in CommitQueueCompletionStage."""

  def setUp(self):
    self.build_type = constants.PFQ_TYPE
    self._Prepare()

    self.partial_submit_changes = ['C', 'D']
    self.other_changes = ['A', 'B']
    self.changes = self.other_changes + self.partial_submit_changes

    self.alert_email_mock = self.PatchObject(alerts, 'SendEmail')
    self.PatchObject(cbuildbot_run._BuilderRunBase,
                     'InEmailReportingEnvironment', return_value=True)
    self.PatchObject(completion_stages.MasterSlaveSyncCompletionStage,
                     'HandleFailure')
    self.PatchObject(builder_status_lib.BuilderStatusesFetcher, '__init__',
                     return_value=None)
    self.PatchObject(builder_status_lib, 'GetFailedMessages')

  def ConstructStage(self):
    """Returns a CommitQueueCompletionStage object."""
    sync_stage = sync_stages.CommitQueueSyncStage(self._run)
    sync_stage.pool = mock.MagicMock()
    sync_stage.pool.applied = self.changes

    return completion_stages.CommitQueueCompletionStage(
        self._run, sync_stage, success=True)

  def VerifyStage(self, failing=(), inflight=(), no_stat=(), alert=False,
                  stage=None, build_passed=False):
    """Runs and Verifies PerformStage.

    Args:
      failing: The names of the builders that failed.
      inflight: The names of the buiders that timed out.
      no_stat: The names of the builders that had status None.
      alert: If True, sends out an alert email for infra failures.
      stage: If set, use this constructed stage, otherwise create own.
      build_passed: Whether the build passed or failed.
    """
    if not stage:
      stage = self.ConstructStage()

    stage._run.attrs.manifest_manager = mock.Mock()

    # Setup the stage to look at the specified configs.
    all_slaves = set(failing) | set(inflight) | set(no_stat)
    configs = [config_lib.BuildConfig(name=x) for x in all_slaves]
    self.PatchObject(stage, '_GetSlaveConfigs', return_value=configs)

    statuses = {}
    for x in failing:
      statuses[x] = builder_status_lib.BuilderStatus(
          constants.BUILDER_STATUS_FAILED, message=None)
    for x in inflight:
      statuses[x] = builder_status_lib.BuilderStatus(
          constants.BUILDER_STATUS_INFLIGHT, message=None)
    for x in no_stat:
      statuses[x] = builder_status_lib.BuilderStatus(
          constants.BUILDER_STATUS_MISSING, message=None)

    self.PatchObject(builder_status_lib.BuilderStatusesFetcher,
                     'GetBuilderStatuses', return_value=(statuses, {}))

    # Track whether 'HandleSuccess' is called.
    success_mock = self.PatchObject(stage, 'HandleSuccess')

    # Actually run the stage.
    if build_passed:
      stage.PerformStage()
    else:
      with self.assertRaises(completion_stages.ImportantBuilderFailedException):
        stage.PerformStage()

    # Verify the calls.
    self.assertEqual(success_mock.called, build_passed)

    if not build_passed and self._run.config.master:
      if alert:
        self.alert_email_mock.assert_called_once_with(
            mock.ANY, mock.ANY, server=mock.ANY, message=mock.ANY,
            extra_fields=mock.ANY)


# pylint: disable=too-many-ancestors
class SlaveCommitQueueCompletionStageTest(BaseCommitQueueCompletionStageTest):
  """Tests how CQ a slave handles changes in CommitQueueCompletionStage."""
  BOT_ID = 'x86-mario-paladin'

  def testSuccess(self):
    """Test the slave succeeding."""
    self.VerifyStage(build_passed=True)

  def testFail(self):
    """Test the slave failing."""
    self.VerifyStage(failing=['foo'], build_passed=False)

  def testTimeout(self):
    """Test the slave timing out."""
    self.VerifyStage(inflight=['foo'], build_passed=False)


class MasterCommitQueueCompletionStageTest(BaseCommitQueueCompletionStageTest):
  """Tests how CQ master handles changes in CommitQueueCompletionStage."""
  BOT_ID = 'master-paladin'

  def _Prepare(self, bot_id=BOT_ID, **kwargs):
    super(MasterCommitQueueCompletionStageTest, self)._Prepare(bot_id, **kwargs)
    self.assertTrue(self._run.config['master'])

  def tearDown(self):
    cidb.CIDBConnectionFactory.ClearMock()

  def testImportantBuildFailing(self):
    """Test stage when important builds in failing."""
    self.VerifyStage(failing=['foo'])

  def testImportantBuildInflight(self):
    """Test stage when important builds in inflight."""
    self.VerifyStage(inflight=['foo'])

  def testImportantBuildNostat(self):
    """Test stage when important builds in no_stat."""
    self.VerifyStage(no_stat=['foo'])

  def testFailingBuildersWithInfraFail(self):
    """Alert when the failing builders have infra failure messages."""
    self.PatchObject(completion_stages.CommitQueueCompletionStage,
                     '_GetInfraFailMessages', return_value=['msg'])
    self.PatchObject(builder_status_lib,
                     'GetBuildersWithNoneMessages', return_value=[])

    # An alert is sent, since there are infra failures.
    self.VerifyStage(failing=['foo'], alert=True)

  def testAlertFailingBuildersWithNoneFailureMessages(self):
    """Alert when failing builders don't have failure messages."""
    self.PatchObject(completion_stages.CommitQueueCompletionStage,
                     '_GetInfraFailMessages', return_value=[])
    self.PatchObject(builder_status_lib,
                     'GetBuildersWithNoneMessages', return_value=['foo'])

    # An alert is sent, since NonType messages are considered infra failures.
    self.VerifyStage(failing=['foo'], alert=True)

  def testAlertInflightBuildersWithNoneInfraFail(self):
    """Alert when inflight builders don't have failure messages."""
    self.PatchObject(completion_stages.CommitQueueCompletionStage,
                     '_GetInfraFailMessages', return_value=[])
    self.PatchObject(builder_status_lib,
                     'GetBuildersWithNoneMessages', return_value=[])

    # An alert is sent, since we have an inflight build still.
    self.VerifyStage(inflight=['foo'], alert=True)

  def testAlertNostatBuildersWithNoneInfraFail(self):
    """Alert when no_stat builders don't have failure messages."""
    self.PatchObject(completion_stages.CommitQueueCompletionStage,
                     '_GetInfraFailMessages', return_value=[])
    self.PatchObject(builder_status_lib,
                     'GetBuildersWithNoneMessages', return_value=[])

    # An alert is sent, since we have an inflight build still.
    self.VerifyStage(no_stat=['no_stat'], alert=True)

  def test_IsFailureFatalWithCLs(self):
    """Test _IsFailureFatal with CLs."""
    stage = self.ConstructStage()
    stage.sync_stage.pool.HasPickedUpCLs.return_value = True

    self.assertFalse(stage._IsFailureFatal(set(), set(), set()))
    self.assertTrue(stage._IsFailureFatal(set(['test3']), set(), set()))

  def test_IsFailureFatalWithoutCLs(self):
    """Test _IsFailureFatal without CLs."""
    stage = self.ConstructStage()
    stage.sync_stage.pool.HasPickedUpCLs.return_value = False

    self.assertFalse(stage._IsFailureFatal(set(), set(), set()))
    self.assertFalse(stage._IsFailureFatal(set(['test3']), set(), set()))

  def testIsFailureFatalWithSelfDestruction(self):
    """test _IsFailureFatal with self-destruction."""
    mock_cidb = mock.MagicMock()
    cidb.CIDBConnectionFactory.SetupMockCidb(mock_cidb)
    self.PatchObject(builder_status_lib,
                     'GetSlavesAbortedBySelfDestructedMaster',
                     return_value={'slave-1', 'slave-2', 'slave-3'})
    stage = self.ConstructStage()

    self.assertFalse(stage._IsFailureFatal(
        {'slave-1'}, {'slave-2'}, {'slave-3'}, True))

    self.assertTrue(stage._IsFailureFatal(
        {'slave-1', 'slave-4'}, {'slave-2'}, {'slave-3'}, True))

  def testIsFailureFatalWithSelfDestructionWithSuccess(self):
    """test _IsFailureFatal with self-destruction-with-success."""
    mock_cidb = mock.MagicMock()
    cidb.CIDBConnectionFactory.SetupMockCidb(mock_cidb)
    self.PatchObject(builder_status_lib,
                     'GetSlavesAbortedBySelfDestructedMaster',
                     return_value={'slave-1', 'slave-2', 'slave-3'})
    stage = self.ConstructStage()
    stage._run.attrs.metadata.UpdateWithDict(
        {constants.SELF_DESTRUCTED_WITH_SUCCESS_BUILD: True})

    self.assertFalse(stage._IsFailureFatal(
        {'slave-1', 'slave-4'}, {'slave-2'}, {'slave-3'}, True))

    self.assertTrue(stage._IsFailureFatal(
        {stage._run.config.name}, {'slave-2'}, {'slave-3'}, True))

  def testSendInfraAlertIfNeededWithAlerts(self):
    """Test SendInfraAlertIfNeeded which sends alerts."""
    mock_send_alert = self.PatchObject(tree_status, 'SendHealthAlert')
    self.PatchObject(completion_stages.CommitQueueCompletionStage,
                     '_GetInfraFailMessages', return_value=['failure_message'])
    self.PatchObject(builder_status_lib,
                     'GetBuildersWithNoneMessages', return_value=[])
    stage = self.ConstructStage()
    failing = {'failing_build'}
    inflight = {'inflight_build'}
    no_stat = {'no_stat_build'}

    stage.SendInfraAlertIfNeeded(failing, inflight, no_stat, False)
    self.assertEqual(mock_send_alert.call_count, 1)
    stage.SendInfraAlertIfNeeded(failing, inflight, no_stat, True)
    self.assertEqual(mock_send_alert.call_count, 2)

  def testSendInfraAlertIfNeededWithoutAlerts(self):
    """Test SendInfraAlertIfNeeded which doesn't send alerts."""
    mock_send_alert = self.PatchObject(tree_status, 'SendHealthAlert')
    self.PatchObject(completion_stages.CommitQueueCompletionStage,
                     '_GetInfraFailMessages', return_value=[])
    self.PatchObject(builder_status_lib,
                     'GetBuildersWithNoneMessages', return_value=[])
    stage = self.ConstructStage()
    inflight = {'inflight_build'}
    no_stat = {'no_stat_build'}

    stage.SendInfraAlertIfNeeded({}, inflight, no_stat, True)
    self.assertEqual(mock_send_alert.call_count, 0)


class PublishUprevChangesStageTest(
    generic_stages_unittest.AbstractStageTestCase):
  """Tests for the PublishUprevChanges stage."""
  BOT_ID = 'master-chromium-pfq'

  def _Prepare(self, bot_id=None, **kwargs):
    super(PublishUprevChangesStageTest, self)._Prepare(bot_id, **kwargs)

  def setUp(self):
    self.PatchObject(completion_stages.PublishUprevChangesStage,
                     '_GetPortageEnvVar')
    self.PatchObject(completion_stages.PublishUprevChangesStage,
                     '_ExtractOverlays', return_value=[['foo'], ['bar']])
    self.PatchObject(prebuilts.BinhostConfWriter, 'Perform')
    self.push_mock = self.PatchObject(commands, 'UprevPush')
    self.PatchObject(generic_stages.BuilderStage, 'GetRepoRepository')
    self.PatchObject(commands, 'UprevPackages')

    self._Prepare()

  def ConstructStage(self):
    sync_stage = sync_stages.ManifestVersionedSyncStage(self._run)
    sync_stage.pool = mock.MagicMock()
    return completion_stages.PublishUprevChangesStage(
        self._run, sync_stage, success=True)

  def testPush(self):
    """Test values for PublishUprevChanges."""
    self._Prepare(extra_config={'push_overlays': constants.PUBLIC_OVERLAYS},
                  extra_cmd_args=['--chrome_rev', constants.CHROME_REV_TOT])
    self._run.options.prebuilts = True
    self.RunStage()
    self.push_mock.assert_called_once_with(self.build_root, ['bar'], False,
                                           staging_branch=None)
    self.assertTrue(self._run.attrs.metadata.GetValue('UprevvedChrome'))
    metadata_dict = self._run.attrs.metadata.GetDict()
    self.assertFalse(metadata_dict.has_key('UprevvedAndroid'))

  def testCheckSlaveUploadPrebuiltsTest(self):
    """Tests for CheckSlaveUploadPrebuiltsTest."""
    stage = self.ConstructStage()
    stage._build_stage_id = 'test_build_stage_id'

    build_id = 'test_master_build_id'
    mock_cidb = mock.MagicMock()
    cidb.CIDBConnectionFactory.SetupMockCidb(mock_cidb)

    stage_name = 'UploadPrebuilts'

    slave_a = 'slave_a'
    slave_b = 'slave_b'
    slave_c = 'slave_c'

    slave_configs_a = [{'name': slave_a},
                       {'name': slave_b}]
    slave_stages_a = [{'name': stage_name,
                       'build_config': slave_a,
                       'status': constants.BUILDER_STATUS_PASSED},
                      {'name': stage_name,
                       'build_config': slave_b,
                       'status': constants.BUILDER_STATUS_PASSED}]

    self.PatchObject(completion_stages.PublishUprevChangesStage,
                     '_GetSlaveConfigs', return_value=slave_configs_a)
    self.PatchObject(mock_cidb, 'GetSlaveStages',
                     return_value=slave_stages_a)

    # All important slaves are covered
    self.assertTrue(stage.CheckSlaveUploadPrebuiltsTest(
        mock_cidb, build_id))

    slave_stages_b = [{'name': stage_name,
                       'build_config': slave_a,
                       'status': constants.BUILDER_STATUS_FAILED},
                      {'name': stage_name,
                       'build_config': slave_b,
                       'status': constants.BUILDER_STATUS_PASSED}]
    self.PatchObject(completion_stages.PublishUprevChangesStage,
                     '_GetSlaveConfigs', return_value=slave_configs_a)
    self.PatchObject(mock_cidb, 'GetSlaveStages',
                     return_value=slave_stages_b)

    # Slave_a didn't pass the stage
    self.assertFalse(stage.CheckSlaveUploadPrebuiltsTest(
        mock_cidb, build_id))

    slave_configs_b = [{'name': slave_a},
                       {'name': slave_b},
                       {'name': slave_c}]
    self.PatchObject(completion_stages.PublishUprevChangesStage,
                     '_GetSlaveConfigs', return_value=slave_configs_b)
    self.PatchObject(mock_cidb, 'GetSlaveStages',
                     return_value=slave_stages_a)

    # No stage information for slave_c
    self.assertFalse(stage.CheckSlaveUploadPrebuiltsTest(
        mock_cidb, build_id))

  def testAndroidPush(self):
    """Test values for PublishUprevChanges with Android PFQ."""
    self._Prepare(bot_id=constants.NYC_ANDROID_PFQ_MASTER,
                  extra_config={'push_overlays': constants.PUBLIC_OVERLAYS},
                  extra_cmd_args=['--android_rev',
                                  constants.ANDROID_REV_LATEST])
    self._run.options.prebuilts = True
    self.RunStage()
    self.push_mock.assert_called_once_with(self.build_root, ['bar'], False,
                                           staging_branch=None)
    self.assertTrue(self._run.attrs.metadata.GetValue('UprevvedAndroid'))
    metadata_dict = self._run.attrs.metadata.GetDict()
    self.assertFalse(metadata_dict.has_key('UprevvedChrome'))

  def testPerformStageOnChromePFQ(self):
    """Test PerformStage on ChromePFQ."""
    stage = self.ConstructStage()
    stage.sync_stage.pool.HasPickedUpCLs.return_value = True
    stage.PerformStage()
    self.push_mock.assert_called_once_with(self.build_root, ['bar'], False,
                                           staging_branch=None)

  def testPerformStageOnCQMasterWithPickedUpCLs(self):
    """Test PerformStage on CQ-master with picked up CLs."""
    self._Prepare(bot_id=constants.CQ_MASTER)
    stage = self.ConstructStage()
    stage.sync_stage.pool.HasPickedUpCLs.return_value = True
    stage.PerformStage()
    self.push_mock.assert_called_once_with(self.build_root, ['bar'], False,
                                           staging_branch=None)

  def testPerformStageOnCQMasterWithoutPickedUpCLs(self):
    """Test PerformStage on CQ-master without picked up CLs."""
    self._Prepare(bot_id=constants.CQ_MASTER)
    stage = self.ConstructStage()
    stage.sync_stage.pool.HasPickedUpCLs.return_value = False
    stage.PerformStage()
    self.push_mock.assert_not_called()
