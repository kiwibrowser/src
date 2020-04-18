# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for test stages."""

from __future__ import print_function

import json
import mock
import os

from chromite.cbuildbot import cbuildbot_unittest
from chromite.cbuildbot import commands
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.cbuildbot.stages import generic_stages
from chromite.cbuildbot.stages import test_stages
from chromite.lib.const import waterfall
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import cros_test_lib
from chromite.lib import failures_lib
from chromite.lib import fake_cidb
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import timeout_util


# pylint: disable=too-many-ancestors

class UnitTestStageTest(generic_stages_unittest.AbstractStageTestCase,
                        cbuildbot_unittest.SimpleBuilderTestCase):
  """Tests for the UnitTest stage."""

  BOT_ID = 'amd64-generic-full'
  RELEASE_TAG = 'ToT.0.0'

  def setUp(self):
    self.rununittests_mock = self.PatchObject(commands, 'RunUnitTests')
    self.buildunittests_mock = self.PatchObject(
        commands, 'BuildUnitTestTarball', return_value='unit_tests.tar')
    self.uploadartifact_mock = self.PatchObject(
        generic_stages.ArchivingStageMixin, 'UploadArtifact')
    self.testauzip_mock = self.PatchObject(commands, 'TestAuZip')
    self.image_dir = os.path.join(
        self.build_root, 'src/build/images/amd64-generic/latest-cbuildbot')

    self._Prepare()

  def ConstructStage(self):
    self._run.GetArchive().SetupArchivePath()
    return test_stages.UnitTestStage(self._run, self._current_board)

  def testFullTests(self):
    """Tests if full unit and cros_au_test_harness tests are run correctly."""
    exists_mock = self.PatchObject(os.path, 'exists', return_value=True)
    makedirs_mock = self.PatchObject(osutils, 'SafeMakedirs')

    self.RunStage()
    makedirs_mock.assert_called_once_with(self._run.GetArchive().archive_path)
    exists_mock.assert_called_once_with(
        os.path.join(self.image_dir, 'au-generator.zip'))
    self.rununittests_mock.assert_called_once_with(
        self.build_root, self._current_board, blacklist=[], extra_env=mock.ANY)
    self.buildunittests_mock.assert_called_once_with(
        self.build_root, self._current_board,
        self._run.GetArchive().archive_path)
    self.uploadartifact_mock.assert_called_once_with(
        'unit_tests.tar', archive=False)
    self.testauzip_mock.assert_called_once_with(self.build_root, self.image_dir)


class HWTestStageTest(generic_stages_unittest.AbstractStageTestCase,
                      cbuildbot_unittest.SimpleBuilderTestCase):
  """Tests for the HWTest stage."""

  BOT_ID = 'x86-mario-release'
  VERSION = 'R36-5760.0.0'
  RELEASE_TAG = ''

  def setUp(self):
    self.run_suite_mock = self.PatchObject(commands, 'RunHWTestSuite')
    self.warning_mock = self.PatchObject(
        logging, 'PrintBuildbotStepWarnings')
    self.failure_mock = self.PatchObject(
        logging, 'PrintBuildbotStepFailure')

    self.suite_config = None
    self.suite = None
    self.version = None

    self._Prepare()

  def _Prepare(self, bot_id=None, version=None, warn_only=False, **kwargs):
    super(HWTestStageTest, self)._Prepare(bot_id, **kwargs)

    self.version = version or self.VERSION
    self._run.options.log_dir = '/b/cbuild/mylogdir'
    self.suite_config = self.GetHWTestSuite()
    self.suite_config.warn_only = warn_only
    self.suite = self.suite_config.suite

  def ConstructStage(self):
    self._run.GetArchive().SetupArchivePath()
    board_runattrs = self._run.GetBoardRunAttrs(self._current_board)
    board_runattrs.SetParallelDefault('test_artifacts_uploaded', True)
    return test_stages.HWTestStage(
        self._run, self._current_board, self._model, self.suite_config)

  def _RunHWTestSuite(self, debug=False, fails=False, warns=False,
                      cmd_fail_mode=None):
    """Verify the stage behavior in various circumstances.

    Args:
      debug: Whether the HWTest suite should be run in debug mode.
      fails: Whether the stage should fail.
      warns: Whether the stage should warn.
      cmd_fail_mode: How commands.RunHWTestSuite() should fail.
        If None, don't fail.
    """
    # We choose to define these mocks in setUp() because they are
    # useful for tests that do not call this method. However, this
    # means we have to reset the mocks before each run.
    self.run_suite_mock.reset_mock()
    self.warning_mock.reset_mock()
    self.failure_mock.reset_mock()

    mock_report = self.PatchObject(
        test_stages.HWTestStage, 'ReportHWTestResults')

    to_raise = None

    if cmd_fail_mode == None:
      to_raise = None
    elif cmd_fail_mode == 'timeout':
      to_raise = timeout_util.TimeoutError('Timed out')
    elif cmd_fail_mode == 'suite_timeout':
      to_raise = failures_lib.SuiteTimedOut('Suite timed out')
    elif cmd_fail_mode == 'board_not_available':
      to_raise = failures_lib.BoardNotAvailable('Board not available')
    elif cmd_fail_mode == 'lab_fail':
      to_raise = failures_lib.TestLabFailure('Test lab failure')
    elif cmd_fail_mode == 'test_warn':
      to_raise = failures_lib.TestWarning('Suite passed with warnings')
    elif cmd_fail_mode == 'test_fail':
      to_raise = failures_lib.TestFailure('HWTest failed.')
    else:
      raise ValueError('cmd_fail_mode %s not supported' % cmd_fail_mode)

    if cmd_fail_mode == 'timeout':
      self.run_suite_mock.side_effect = to_raise
    else:
      self.run_suite_mock.return_value = commands.HWTestSuiteResult(
          to_raise, None)

    if fails:
      self.assertRaises(failures_lib.StepFailure, self.RunStage)
    else:
      self.RunStage()

    self.run_suite_mock.assert_called_once()
    self.assertEqual(self.run_suite_mock.call_args[1].get('debug'), debug)
    self.assertEqual(self.run_suite_mock.call_args[1].get('model'), self._model)

    # Make sure we print the buildbot failure/warning messages correctly.
    if fails:
      self.failure_mock.assert_called_once()
    else:
      self.failure_mock.assert_not_called()

    if warns:
      self.warning_mock.assert_called_once()
    else:
      self.warning_mock.assert_not_called()

    mock_report.assert_not_called()

  def testRemoteTrybotWithHWTest(self):
    """Test remote trybot with hw test enabled"""
    cmd_args = ['--remote-trybot', '-r', self.build_root, '--hwtest',
                self.BOT_ID]
    self._Prepare(cmd_args=cmd_args)
    self._RunHWTestSuite()

  def testRemoteTrybotNoHWTest(self):
    """Test remote trybot with no hw test"""
    cmd_args = ['--remote-trybot', '-r', self.build_root, self.BOT_ID]
    self._Prepare(cmd_args=cmd_args)
    self._RunHWTestSuite(debug=True)

  def testWithSuite(self):
    """Test if run correctly with a test suite."""
    self._RunHWTestSuite()

  def testHandleTestWarning(self):
    """Tests that we pass the build on test warning."""
    # CQ passes.
    self._Prepare('x86-alex-paladin')
    self._RunHWTestSuite(warns=True, cmd_fail_mode='test_warn')

    # PFQ passes.
    self._Prepare('falco-chrome-pfq')
    self._RunHWTestSuite(warns=True, cmd_fail_mode='test_warn')

    # Canary passes.
    self._Prepare('x86-alex-release')
    self._RunHWTestSuite(warns=True, cmd_fail_mode='test_warn')

  def testHandleLabFail(self):
    """Tests that we handle lab failures correctly."""
    # CQ fails.
    self._Prepare('x86-alex-paladin')
    self._RunHWTestSuite(fails=True, cmd_fail_mode='lab_fail')

    # PFQ fails.
    self._Prepare('falco-chrome-pfq')
    self._RunHWTestSuite(fails=True, cmd_fail_mode='lab_fail')

    # Canary fails.
    self._Prepare('x86-alex-release')
    self._RunHWTestSuite(fails=True, cmd_fail_mode='lab_fail')

  def testWithSuiteWithFatalFailure(self):
    """Tests that we fail on test failure."""
    self._RunHWTestSuite(fails=True, cmd_fail_mode='test_fail')

  def testWithSuiteWithFatalFailureWarnFlag(self):
    """Tests that we don't fail if HWTestConfig warn_only is True."""
    self._Prepare('x86-alex-release', warn_only=True)
    self._RunHWTestSuite(warns=True, cmd_fail_mode='test_fail')

  def testHandleSuiteTimeout(self):
    """Tests that we handle suite timeout correctly ."""
    # Canary fails.
    self._Prepare('x86-alex-release')
    self._RunHWTestSuite(fails=True, cmd_fail_mode='suite_timeout')

    # CQ fails.
    self._Prepare('x86-alex-paladin')
    self._RunHWTestSuite(fails=True, cmd_fail_mode='suite_timeout')

    # PFQ fails.
    self._Prepare('falco-chrome-pfq')
    self._RunHWTestSuite(fails=True, cmd_fail_mode='suite_timeout')

  def testHandleBoardNotAvailable(self):
    """Tests that we handle board not available correctly."""
    # Canary passes.
    self._Prepare('x86-alex-release')
    self._RunHWTestSuite(warns=True, cmd_fail_mode='board_not_available')

    # CQ fails.
    self._Prepare('x86-alex-paladin')
    self._RunHWTestSuite(fails=True, cmd_fail_mode='board_not_available')

    # PFQ fails.
    self._Prepare('falco-chrome-pfq')
    self._RunHWTestSuite(fails=True, cmd_fail_mode='board_not_available')

  def testHandleTimeout(self):
    """Tests that we handle timeout exceptions correctly."""
    # Canary fails.
    self._Prepare('x86-alex-release')
    self._RunHWTestSuite(fails=True, cmd_fail_mode='timeout')

    # CQ fails.
    self._Prepare('x86-alex-paladin')
    self._RunHWTestSuite(fails=True, cmd_fail_mode='timeout')

    # PFQ fails.
    self._Prepare('falco-chrome-pfq')
    self._RunHWTestSuite(fails=True, cmd_fail_mode='timeout')

  def testPayloadsNotGenerated(self):
    """Test that we exit early if payloads are not generated."""
    board_runattrs = self._run.GetBoardRunAttrs(self._current_board)
    board_runattrs.SetParallel('test_artifacts_uploaded', False)

    self.RunStage()

    # Make sure we make the stage orange.
    self.warning_mock.assert_called_once()
    # We exit early, so commands.RunHWTestSuite should not have been
    # called.
    self.assertFalse(self.run_suite_mock.called)

  def testReportHWTestResults(self):
    """Test ReportHWTestResults."""
    stage = self.ConstructStage()
    json_str = """
{
  "tests":{
    "Suite job":{
       "status":"FAIL"
    },
    "cheets_CTS.com.android.cts.dram":{
       "status":"FAIL"
    },
    "cheets_ContainerSmokeTest":{
       "status":"GOOD"
    },
    "cheets_DownloadsFilesystem":{
       "status":"ABORT"
    },
    "cheets_KeyboardTest":{
       "status":"UNKNOWN"
    }
  }
}
"""
    json_dump_dict = json.loads(json_str)
    db = fake_cidb.FakeCIDBConnection()
    build_id = db.InsertBuild('build_1', waterfall.WATERFALL_INTERNAL, 1,
                              'build_1', 'bot_hostname')

    # When json_dump_dict is None
    self.assertIsNone(stage.ReportHWTestResults(None, build_id, db))

    # When db is None
    self.assertIsNone(stage.ReportHWTestResults(json_dump_dict, build_id, None))

    # When results are successfully reported
    stage.ReportHWTestResults(json_dump_dict, build_id, db)
    results = db.GetHWTestResultsForBuilds([build_id])
    result_dict = {x.test_name: x.status for x in results}

    expect_dict = {
        'cheets_DownloadsFilesystem': constants.HWTEST_STATUS_ABORT,
        'cheets_KeyboardTest': constants.HWTEST_STATUS_OTHER,
        'Suite job': constants.HWTEST_STATUS_FAIL,
        'cheets_CTS.com.android.cts.dram': constants.HWTEST_STATUS_FAIL,
        'cheets_ContainerSmokeTest': constants.HWTEST_STATUS_PASS
    }
    self.assertItemsEqual(expect_dict, result_dict)
    self.assertEqual(len(results), 5)

  def testPerformStageOnCQ(self):
    """Test PerformStage on CQ."""
    self._Prepare('eve-paladin')
    stage = self.ConstructStage()
    mock_report = self.PatchObject(
        test_stages.HWTestStage, 'ReportHWTestResults')
    cmd_result = mock.Mock(to_raise=None)
    self.PatchObject(commands, 'RunHWTestSuite', return_value=cmd_result)
    stage.PerformStage()

    mock_report.assert_called_once_with(mock.ANY, mock.ANY, mock.ANY)


class ImageTestStageTest(generic_stages_unittest.AbstractStageTestCase,
                         cros_test_lib.RunCommandTestCase,
                         cbuildbot_unittest.SimpleBuilderTestCase):
  """Test image test stage."""

  BOT_ID = 'x86-mario-release'
  RELEASE_TAG = 'ToT.0.0'

  def setUp(self):
    self._test_root = os.path.join(self.build_root, 'tmp/results_dir')
    self.PatchObject(commands, 'CreateTestRoot', autospec=True,
                     return_value='/tmp/results_dir')
    self.PatchObject(path_util, 'ToChrootPath',
                     side_effect=lambda x: x)
    self._Prepare()

  def _Prepare(self, bot_id=None, **kwargs):
    super(ImageTestStageTest, self)._Prepare(bot_id, **kwargs)
    self._run.GetArchive().SetupArchivePath()

  def ConstructStage(self):
    return test_stages.ImageTestStage(self._run, self._current_board)

  def testPerformStage(self):
    """Tests that we correctly run test-image script."""
    stage = self.ConstructStage()
    stage.PerformStage()
    cmd = [
        'sudo', '--',
        os.path.join(self.build_root, 'chromite', 'bin', 'test_image'),
        '--board', self._current_board,
        '--test_results_root',
        path_util.ToChrootPath(os.path.join(self._test_root,
                                            'image_test_results')),
        path_util.ToChrootPath(stage.GetImageDirSymlink()),
    ]
    self.assertCommandContains(cmd)
