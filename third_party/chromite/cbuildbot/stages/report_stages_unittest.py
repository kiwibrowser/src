# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for report stages."""

from __future__ import print_function

import datetime as dt
import json
import mock
import os

from chromite.cbuildbot import cbuildbot_run
from chromite.cbuildbot import cbuildbot_unittest
from chromite.cbuildbot import commands
from chromite.cbuildbot import manifest_version
from chromite.cbuildbot import topology
from chromite.cbuildbot import topology_unittest
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.cbuildbot.stages import report_stages
from chromite.lib.const import waterfall
from chromite.lib import alerts
from chromite.lib import cidb
from chromite.lib import constants
from chromite.lib import cq_config
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import fake_cidb
from chromite.lib import failures_lib
from chromite.lib import failure_message_lib_unittest
from chromite.lib import gs_unittest
from chromite.lib import metadata_lib
from chromite.lib import metrics
from chromite.lib import osutils
from chromite.lib import patch_unittest
from chromite.lib import results_lib
from chromite.lib import retry_stats
from chromite.lib import risk_report
from chromite.lib import toolchain
from chromite.lib import triage_lib


# pylint: disable=protected-access
# pylint: disable=too-many-ancestors


class BuildReexecutionStageTest(generic_stages_unittest.AbstractStageTestCase):
  """Tests that BuildReexecutionFinishedStage behaves as expected."""
  def setUp(self):
    self.fake_db = fake_cidb.FakeCIDBConnection()
    cidb.CIDBConnectionFactory.SetupMockCidb(self.fake_db)
    build_id = self.fake_db.InsertBuild(
        'builder name', 'waterfall', 1, 'build config', 'bot hostname')

    self._Prepare(build_id=build_id)

    release_tag = '4815.0.0-rc1'
    self._run.attrs.release_tag = '4815.0.0-rc1'
    fake_versioninfo = manifest_version.VersionInfo(release_tag, '39')
    self.gs_mock = self.StartPatcher(gs_unittest.GSContextMock())
    self.gs_mock.SetDefaultCmdResult()
    self.PatchObject(cbuildbot_run._BuilderRunBase, 'GetVersionInfo',
                     return_value=fake_versioninfo)
    self.PatchObject(toolchain, 'GetToolchainsForBoard')
    self.PatchObject(toolchain, 'GetToolchainTupleForBoard',
                     return_value=['i686-pc-linux-gnu', 'arm-none-eabi'])

  def tearDown(self):
    cidb.CIDBConnectionFactory.SetupMockCidb()

  def testPerformStage(self):
    """Test that a normal runs completes without error."""
    self.RunStage()
    tags = self._run.attrs.metadata.GetValue(constants.METADATA_TAGS)
    self.assertEqual(tags['version_full'], 'R39-4815.0.0-rc1')

  def testMasterSlaveVersionMismatch(self):
    """Test that master/slave version mismatch causes failure."""
    master_release_tag = '9999.0.0-rc1'
    master_build_id = self.fake_db.InsertBuild(
        'master', waterfall.WATERFALL_INTERNAL, 2, 'master config',
        'master hostname')
    master_metadata = metadata_lib.CBuildbotMetadata()
    master_metadata.UpdateKeyDictWithDict(
        'version', {'full' : 'R39-9999.0.0-rc1',
                    'milestone': '39',
                    'platform': master_release_tag})
    self._run.attrs.metadata.UpdateWithDict(
        {'master_build_id': master_build_id})
    self.fake_db.UpdateMetadata(master_build_id, master_metadata)

    stage = self.ConstructStage()
    with self.assertRaises(failures_lib.StepFailure):
      stage.Run()

  def ConstructStage(self):
    return report_stages.BuildReexecutionFinishedStage(self._run)


class ConfigDumpStageTest(generic_stages_unittest.AbstractStageTestCase):
  """Tests that ConfigDumpStage runs without syntax error."""

  def ConstructStage(self):
    return report_stages.ConfigDumpStage(self._run)

  def testPerformStage(self):
    self._Prepare()
    self.RunStage()


class SlaveFailureSummaryStageTest(
    generic_stages_unittest.AbstractStageTestCase):
  """Tests that SlaveFailureSummaryStage behaves as expected."""

  def setUp(self):
    self.db = mock.MagicMock()
    cidb.CIDBConnectionFactory.SetupMockCidb(self.db)
    self._Prepare(build_id=1)

  def _Prepare(self, **kwargs):
    """Prepare stage with config['master']=True."""
    super(SlaveFailureSummaryStageTest, self)._Prepare(**kwargs)
    self._run.config['master'] = True

  def ConstructStage(self):
    return report_stages.SlaveFailureSummaryStage(self._run)

  def testPerformStage(self):
    """Tests that stage runs without syntax errors."""
    fake_failure = (
        failure_message_lib_unittest.StageFailureHelper.CreateStageFailure(
            build_id=10,
            build_stage_id=11,
            waterfall=waterfall.WATERFALL_EXTERNAL,
            builder_name='builder_name',
            build_number=12,
            build_config='build-config',
            stage_name='FailingStage',
            stage_status=constants.BUILDER_STATUS_FAILED,
            build_status=constants.BUILDER_STATUS_FAILED))
    self.PatchObject(self.db, 'GetSlaveFailures', return_value=[fake_failure])
    self.PatchObject(logging, 'PrintBuildbotLink')
    self.RunStage()
    self.assertEqual(logging.PrintBuildbotLink.call_count, 1)


class BuildStartStageTest(generic_stages_unittest.AbstractStageTestCase):
  """Tests that BuildStartStage behaves as expected."""

  def setUp(self):
    self.db = fake_cidb.FakeCIDBConnection()
    cidb.CIDBConnectionFactory.SetupMockCidb(self.db)
    retry_stats.SetupStats()
    os.environ['BUILDBOT_MASTERNAME'] = waterfall.WATERFALL_EXTERNAL

    master_build_id = self.db.InsertBuild(
        'master_build', waterfall.WATERFALL_EXTERNAL, 1,
        'master_build_config', 'bot_hostname')

    self.PatchObject(toolchain, 'GetToolchainsForBoard')
    self.PatchObject(toolchain, 'GetArchForTarget', return_value='x86')

    self._Prepare(build_id=None, master_build_id=master_build_id)

  def testPerformStage(self):
    """Test that a normal run of the stage does a database insert."""
    self.RunStage()

    build_id = self._run.attrs.metadata.GetValue('build_id')
    self.assertGreater(build_id, 0)
    self.assertEqual(self._run.attrs.metadata.GetValue('db_type'),
                     cidb.CONNECTION_TYPE_MOCK)

  def testSuiteSchedulingEqualsFalse(self):
    """Test that a run of the stage makes suite_scheduling False."""
    # Test suite_scheduling for **-paladin
    self._Prepare(bot_id='amd64-generic-paladin')
    self.RunStage()
    self.assertFalse(self._run.attrs.metadata.GetValue('suite_scheduling'))

  def testSuiteSchedulingEqualsTrue(self):
    """Test that a run of the stage makes suite_scheduling True."""
    # Test suite_scheduling for **-release
    self._Prepare(bot_id='x86-alex-release')
    self.RunStage()
    self.assertTrue(self._run.attrs.metadata.GetValue('suite_scheduling'))

  def testHandleSkipWithInstanceChange(self):
    """Test that HandleSkip disables cidb and dies when necessary."""
    # This test verifies that switching to a 'mock' database type once
    # metadata already has an id in 'previous_db_type' will fail.
    self._run.attrs.metadata.UpdateWithDict({'build_id': 31337,
                                             'db_type': 'previous_db_type'})
    stage = self.ConstructStage()
    self.assertRaises(AssertionError, stage.HandleSkip)
    self.assertEqual(cidb.CIDBConnectionFactory.GetCIDBConnectionType(),
                     cidb.CONNECTION_TYPE_INV)
    # The above test has the side effect of invalidating CIDBConnectionFactory.
    # Undo that side effect so other unit tests can run.
    cidb.CIDBConnectionFactory.SetupMockCidb()

  def testHandleSkipWithNoDbType(self):
    """Test that HandleSkip passes when db_type is missing."""
    self._run.attrs.metadata.UpdateWithDict({'build_id': 31337})
    stage = self.ConstructStage()
    stage.HandleSkip()

  def testHandleSkipWithDbType(self):
    """Test that HandleSkip passes when db_type is specified."""
    self._run.attrs.metadata.UpdateWithDict(
        {'build_id': 31337,
         'db_type': cidb.CONNECTION_TYPE_MOCK})
    stage = self.ConstructStage()
    stage.HandleSkip()

  def ConstructStage(self):
    return report_stages.BuildStartStage(self._run)


class AbstractReportStageTestCase(
    generic_stages_unittest.AbstractStageTestCase,
    cbuildbot_unittest.SimpleBuilderTestCase):
  """Base class for testing the Report stage."""

  def setUp(self):
    for cmd in ((osutils, 'WriteFile'),
                (commands, 'UploadArchivedFile'),
                (alerts, 'SendEmail')):
      self.StartPatcher(mock.patch.object(*cmd, autospec=True))
    retry_stats.SetupStats()

    self.PatchObject(report_stages.ReportStage, '_GetBuildDuration',
                     return_value=1000)
    self.PatchObject(toolchain, 'GetToolchainsForBoard')
    self.PatchObject(toolchain, 'GetArchForTarget', return_value='x86')

    # We need to mock out the function in risk_report that calls the real
    # CL-Scanner API to avoid relying on external dependencies in the test.
    self.PatchObject(risk_report, '_GetCLRisks', return_value={'1234': 1.0})

    # Set up a general purpose cidb mock. Tests with more specific
    # mock requirements can replace this with a separate call to
    # SetupMockCidb
    self.mock_cidb = mock.MagicMock()
    cidb.CIDBConnectionFactory.SetupMockCidb(self.mock_cidb)

    # Setup topology for unittests
    keyvals = {topology.DATASTORE_WRITER_CREDS_KEY:'./foo/bar.cert'}
    topology_unittest.FakeFetchTopologyFromCIDB(keyvals=keyvals)

    self._Prepare()

  def _SetupUpdateStreakCounter(self, counter_value=-1):
    self.PatchObject(report_stages.ReportStage, '_UpdateStreakCounter',
                     autospec=True, return_value=counter_value)

  def ConstructStage(self):
    return report_stages.ReportStage(self._run, None)


class ReportStageTest(AbstractReportStageTestCase):
  """Test the Report stage."""

  RELEASE_TAG = ''

  def testCheckResults(self):
    """Basic sanity check for results stage functionality"""
    stages = [
        {
            'name': 'stage1',
            'start_time': dt.datetime.now() - dt.timedelta(0, 500),
            'finish_time': dt.datetime.now() - dt.timedelta(0, 300),
            'status': constants.BUILDER_STATUS_PASSED,
        },
        {
            'name': 'stage2',
            'start_time': dt.datetime.now() - dt.timedelta(0, 500),
            'finish_time': dt.datetime.now() - dt.timedelta(0, 200),
            'status': constants.BUILDER_STATUS_PASSED,
        },
        {
            'name': 'stage3',
            'start_time': dt.datetime.now() - dt.timedelta(0, 200),
            'finish_time': dt.datetime.now() - dt.timedelta(0, 100),
            'status': constants.BUILDER_STATUS_PASSED,
        },
    ]
    statuses = [
        {
            'build_config': 'build1',
            'build_number': '64',
            'start_time': dt.datetime.now() - dt.timedelta(0, 600),
            'finish_time': dt.datetime.now() - dt.timedelta(0, 330),
            'status': constants.BUILDER_STATUS_PASSED,
        },
        {
            'build_config': 'build2',
            'build_number': '27',
            'start_time': dt.datetime.now() - dt.timedelta(0, 300),
            'finish_time': dt.datetime.now() - dt.timedelta(0, 100),
            'status': constants.BUILDER_STATUS_PASSED,
        },
        {
            'build_config': 'build3',
            'build_number': '288282',
            'start_time': dt.datetime.now() - dt.timedelta(0, 400),
            'finish_time': dt.datetime.now() - dt.timedelta(0, 200),
            'status': constants.BUILDER_STATUS_PASSED,
        },
    ]
    self.mock_cidb.GetBuildStages = mock.Mock(
        return_value=stages)
    self.mock_cidb.GetSlaveStatuses = mock.Mock(
        return_value=statuses)
    self._SetupUpdateStreakCounter()
    self.PatchObject(report_stages.ReportStage, '_LinkArtifacts')
    self.RunStage()
    filenames = (
        'LATEST-%s' % self.TARGET_MANIFEST_BRANCH,
        'LATEST-%s' % self.VERSION,
    )
    calls = [mock.call(mock.ANY, mock.ANY, 'metadata.json', False,
                       update_list=True, acl=mock.ANY)]
    calls += [mock.call(mock.ANY, mock.ANY, 'timeline-stages.html',
                        debug=False, update_list=True, acl=mock.ANY)]
    calls += [mock.call(mock.ANY, mock.ANY, 'timeline-slaves.html',
                        debug=False, update_list=True, acl=mock.ANY)]
    calls += [mock.call(mock.ANY, mock.ANY, filename, False,
                        acl=mock.ANY) for filename in filenames]

    # Verify build stages timeline contains the stages that were mocked.
    self.assertEquals(calls, commands.UploadArchivedFile.call_args_list)
    timeline_content = osutils.WriteFile.call_args_list[2][0][1]
    for s in stages:
      self.assertIn('["%s", new Date' % s['name'], timeline_content)

    # Verify slaves timeline contains the slaves that were mocked.
    self.assertEquals(calls, commands.UploadArchivedFile.call_args_list)
    timeline_content = osutils.WriteFile.call_args_list[3][0][1]
    for s in statuses:
      self.assertIn('["%s - %s", new Date' %
                    (s['build_config'], s['build_number']), timeline_content)

  def testDoNotUpdateLATESTMarkersWhenBuildFailed(self):
    """Check that we do not update the latest markers on failed build."""
    self._SetupUpdateStreakCounter()
    self.PatchObject(report_stages.ReportStage, '_LinkArtifacts')
    self.PatchObject(results_lib.Results, 'BuildSucceededSoFar',
                     return_value=False)
    stage = self.ConstructStage()
    self.PatchObject(stage, 'GetBuildFailureMessage')
    stage.Run()
    calls = [mock.call(mock.ANY, mock.ANY, 'metadata.json', False,
                       update_list=True, acl=mock.ANY)]
    calls += [mock.call(mock.ANY, mock.ANY, 'timeline-stages.html',
                        debug=False, update_list=True, acl=mock.ANY)]
    self.assertEquals(calls, commands.UploadArchivedFile.call_args_list)

  def testAlertEmail(self):
    """Send out alerts when streak counter reaches the threshold."""
    self.PatchObject(cbuildbot_run._BuilderRunBase,
                     'InEmailReportingEnvironment', return_value=True)
    self.PatchObject(cros_build_lib, 'HostIsCIBuilder', return_value=True)
    self._Prepare(extra_config={'health_threshold': 3,
                                'health_alert_recipients': ['foo@bar.org']})
    self._SetupUpdateStreakCounter(counter_value=-3)
    self.RunStage()
    # The mocking logic gets confused with SendEmail.
    # pylint: disable=no-member
    self.assertGreater(alerts.SendEmail.call_count, 0,
                       'CQ health alerts emails were not sent.')

  def testAlertEmailOnFailingStreak(self):
    """Continue sending out alerts when streak counter exceeds the threshold."""
    self.PatchObject(cbuildbot_run._BuilderRunBase,
                     'InEmailReportingEnvironment', return_value=True)
    self.PatchObject(cros_build_lib, 'HostIsCIBuilder', return_value=True)
    self._Prepare(extra_config={'health_threshold': 3,
                                'health_alert_recipients': ['foo@bar.org']})
    self._SetupUpdateStreakCounter(counter_value=-5)
    self.RunStage()
    # The mocking logic gets confused with SendEmail.
    # pylint: disable=no-member
    self.assertGreater(alerts.SendEmail.call_count, 0,
                       'CQ health alerts emails were not sent.')

  def testWriteBasicMetadata(self):
    """Test that WriteBasicMetadata writes expected keys correctly."""
    report_stages.WriteBasicMetadata(self._run)
    metadata_dict = self._run.attrs.metadata.GetDict()
    self.assertEqual(metadata_dict['build-number'],
                     generic_stages_unittest.DEFAULT_BUILD_NUMBER)
    self.assertTrue(metadata_dict.has_key('builder-name'))
    self.assertTrue(metadata_dict.has_key('bot-hostname'))

  def testWriteTagMetadata(self):
    """Test that WriteTagMetadata writes expected keys correctly."""
    self.PatchObject(cros_build_lib, 'GetHostName', return_value='cros-wimpy2')
    self._SetupUpdateStreakCounter()
    report_stages.WriteTagMetadata(self._run)
    tags_dict = self._run.attrs.metadata.GetValue(constants.METADATA_TAGS)
    self.assertEqual(tags_dict['build_number'],
                     generic_stages_unittest.DEFAULT_BUILD_NUMBER)
    self.assertTrue(tags_dict.has_key('builder_name'))
    self.assertTrue(tags_dict.has_key('bot_hostname'))
    self.RunStage()
    tags_content = osutils.WriteFile.call_args_list[1][0][1]
    tags_content_dict = json.loads(tags_content)
    self.assertEqual(tags_content_dict['build_number'],
                     generic_stages_unittest.DEFAULT_BUILD_NUMBER)

  def testGetChildConfigsMetadataList(self):
    """Test that GetChildConfigListMetadata generates child config metadata."""
    child_configs = [{'name': 'config1', 'boards': ['board1']},
                     {'name': 'config2', 'boards': ['board2']}]
    config_status_map = {'config1': True,
                         'config2': False}
    expected = [{'name': 'config1', 'boards': ['board1'],
                 'status': constants.BUILDER_STATUS_PASSED},
                {'name': 'config2', 'boards': ['board2'],
                 'status': constants.BUILDER_STATUS_FAILED}]
    child_config_list = report_stages.GetChildConfigListMetadata(
        child_configs, config_status_map)
    self.assertEqual(expected, child_config_list)

  def testPerformStage(self):
    """Test PerformStage."""
    mock_sd = self.PatchObject(metrics, 'CumulativeSecondsDistribution')
    self.PatchObject(report_stages.ReportStage, 'ArchiveResults')
    stage = self.ConstructStage()
    stage.PerformStage()
    self.assertEqual(mock_sd.call_count, 1)


class ReportStageForMasterCQTest(AbstractReportStageTestCase):
  """Test the Report stage for master-paladin."""

  RELEASE_TAG = ''
  BOT_ID = 'master-paladin'

  def testPerformStage(self):
    """Test PerformStage."""
    mock_sd = self.PatchObject(metrics, 'CumulativeSecondsDistribution')
    self.PatchObject(report_stages.ReportStage, 'ArchiveResults')
    stage = self.ConstructStage()
    stage.PerformStage()
    self.assertEqual(mock_sd.call_count, 2)


class ReportStageNoSyncTest(AbstractReportStageTestCase):
  """Test the Report stage if SyncStage didn't complete.

  If SyncStage doesn't complete, we don't know the release tag, and can't
  archive results.
  """
  RELEASE_TAG = None

  def testCommitQueueResults(self):
    """Check that we can run with a RELEASE_TAG of None."""
    self._SetupUpdateStreakCounter()
    self.RunStage()


class DetectRelevantChangesStageTest(
    generic_stages_unittest.AbstractStageTestCase):
  """Test the DetectRelevantChangesStage."""

  def setUp(self):
    self._patch_factory = patch_unittest.MockPatchFactory()
    self.changes = self._patch_factory.GetPatches(how_many=2)

    self._Prepare()

    self.fake_db = fake_cidb.FakeCIDBConnection()
    cidb.CIDBConnectionFactory.SetupMockCidb(self.fake_db)
    build_id = self.fake_db.InsertBuild(
        'test-paladin', 'chromeos', 1, 'test-paladin', 'bot_hostname')
    self._run.attrs.metadata.UpdateWithDict({'build_id': build_id})

  def testGetSubsystemsWithoutEmptyEntry(self):
    """Tests the logic of GetSubsystemTobeTested() under normal case."""
    relevant_changes = self.changes
    self.PatchObject(cq_config.CQConfigParser, 'GetCommonConfigFileForChange')
    self.PatchObject(triage_lib, 'GetTestSubsystemForChange',
                     side_effect=[['light'], ['light', 'power']])

    expected = {'light', 'power'}
    stage = self.ConstructStage()
    results = stage.GetSubsystemToTest(relevant_changes)
    self.assertEqual(results, expected)

  def testGetSubsystemsWithEmptyEntry(self):
    """Tests whether return empty set when have empty entry in subsystems."""
    relevant_changes = self.changes
    self.PatchObject(cq_config.CQConfigParser, 'GetCommonConfigFileForChange')
    self.PatchObject(triage_lib, 'GetTestSubsystemForChange',
                     side_effect=[['light'], []])

    expected = set()
    stage = self.ConstructStage()
    results = stage.GetSubsystemToTest(relevant_changes)
    self.assertEqual(results, expected)

  def ConstructStage(self):
    return report_stages.DetectRelevantChangesStage(self._run,
                                                    self._current_board,
                                                    self.changes)

  def testRecordActionForChangesWithIrrelevantAction(self):
    """Test _RecordActionForChanges with irrelevant action.."""
    stage = self.ConstructStage()
    stage._RecordActionForChanges(
        self.changes, constants.CL_ACTION_IRRELEVANT_TO_SLAVE)
    action_history = self.fake_db.GetActionHistory()
    self.assertEqual(len(action_history), 2)
    for action in action_history:
      self.assertEqual(action.action, constants.CL_ACTION_IRRELEVANT_TO_SLAVE)

  def testRecordActionForChangesWithEmptySet(self):
    """Test _RecordActionForChanges with an empty changes set."""
    stage = self.ConstructStage()
    stage._RecordActionForChanges(
        set(), constants.CL_ACTION_IRRELEVANT_TO_SLAVE)
    action_history = self.fake_db.GetActionHistory()
    self.assertEqual(len(action_history), 0)

  def testRecordActionForChangesWithRelevantAction(self):
    """Test _RecordActionForChanges with relevant action."""
    stage = self.ConstructStage()
    stage._RecordActionForChanges(
        self.changes, constants.CL_ACTION_RELEVANT_TO_SLAVE)
    action_history = self.fake_db.GetActionHistory()
    self.assertEqual(len(action_history), 2)
    for action in action_history:
      self.assertEqual(action.action, constants.CL_ACTION_RELEVANT_TO_SLAVE)
