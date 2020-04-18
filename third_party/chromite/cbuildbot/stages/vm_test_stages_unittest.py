# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for vm_test_stages."""

from __future__ import print_function

import os
import mock

from chromite.cbuildbot import cbuildbot_unittest
from chromite.cbuildbot import commands
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.cbuildbot.stages import vm_test_stages
from chromite.lib import cgroups
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_logging
from chromite.lib import cros_test_lib
from chromite.lib import failures_lib
from chromite.lib import gs
from chromite.lib import moblab_vm
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import results_lib


# pylint: disable=too-many-ancestors

class GCETestStageTest(generic_stages_unittest.AbstractStageTestCase,
                       cbuildbot_unittest.SimpleBuilderTestCase):
  """Tests for the GCETest stage."""

  BOT_ID = 'betty-full'
  RELEASE_TAG = ''

  def setUp(self):
    for cmd in ('CreateTestRoot', 'GenerateStackTraces', 'ArchiveFile',
                'UploadArchivedFile', 'BuildAndArchiveTestResultsTarball'):
      self.PatchObject(commands, cmd, autospec=True)
    for cmd in ('RunTestSuite', 'ArchiveTestResults', 'ArchiveVMFiles',
                'RunDevModeTest', 'RunCrosVMTest',
                'ListTests', 'GetTestResultsDir',):
      self.PatchObject(vm_test_stages, cmd, autospec=True)
    self.PatchObject(vm_test_stages.VMTestStage, '_NoTestResults',
                     autospec=True, return_value=False)
    self.PatchObject(osutils, 'RmDir', autospec=True)
    self.PatchObject(cgroups, 'SimpleContainChildren', autospec=True)
    self._Prepare()

    # Simulate breakpad symbols being ready.
    board_runattrs = self._run.GetBoardRunAttrs(self._current_board)
    board_runattrs.SetParallel('breakpad_symbols_generated', True)

  def ConstructStage(self):
    # pylint: disable=W0212
    self._run.GetArchive().SetupArchivePath()
    stage = vm_test_stages.GCETestStage(self._run, self._current_board)
    image_dir = stage.GetImageDirSymlink()
    osutils.Touch(os.path.join(image_dir, constants.TEST_KEY_PRIVATE),
                  makedirs=True)
    return stage

  def testGceTests(self):
    """Verifies that GCE_SUITE_TEST_TYPE tests are run on GCE."""
    self._run.config['gce_tests'] = [
        config_lib.GCETestConfig(constants.GCE_SUITE_TEST_TYPE,
                                 test_suite='gce-smoke')
    ]
    gce_tarball = constants.TEST_IMAGE_GCE_TAR

    # pylint: disable=unused-argument
    def _MockRunTestSuite(buildroot, board, image_path, results_dir,
                          test_config, *args, **kwargs):
      test_type = test_config.test_type
      self.assertEndsWith(image_path, gce_tarball)
      self.assertEqual(test_type, constants.GCE_SUITE_TEST_TYPE)
      self.assertEqual(test_config.test_suite, 'gce-smoke')
    # pylint: enable=unused-argument

    vm_test_stages.RunTestSuite.side_effect = _MockRunTestSuite

    self.RunStage()

    self.assertTrue(vm_test_stages.RunTestSuite.called and
                    vm_test_stages.RunTestSuite.call_count == 1)


class VMTestStageTest(generic_stages_unittest.AbstractStageTestCase,
                      cbuildbot_unittest.SimpleBuilderTestCase):
  """Tests for the VMTest stage."""

  BOT_ID = 'betty-full'
  RELEASE_TAG = ''

  def setUp(self):
    for cmd in ('CreateTestRoot', 'GenerateStackTraces', 'ArchiveFile',
                'UploadArchivedFile', 'BuildAndArchiveTestResultsTarball'):
      self.PatchObject(commands, cmd, autospec=True)
    for cmd in ('RunTestSuite', 'ArchiveTestResults', 'ArchiveVMFiles',
                'RunDevModeTest', 'RunCrosVMTest',
                'ListTests', 'GetTestResultsDir',):
      self.PatchObject(vm_test_stages, cmd, autospec=True)
    self.PatchObject(vm_test_stages.VMTestStage, '_NoTestResults',
                     autospec=True, return_value=False)
    self.PatchObject(osutils, 'RmDir', autospec=True)
    self.PatchObject(cgroups, 'SimpleContainChildren', autospec=True)
    self._Prepare()

    # Simulate breakpad symbols being ready.
    board_runattrs = self._run.GetBoardRunAttrs(self._current_board)
    board_runattrs.SetParallel('breakpad_symbols_generated', True)

  def ConstructStage(self):
    # pylint: disable=W0212
    self._run.GetArchive().SetupArchivePath()
    stage = vm_test_stages.VMTestStage(self._run, self._current_board)
    image_dir = stage.GetImageDirSymlink()
    osutils.Touch(os.path.join(image_dir, constants.TEST_KEY_PRIVATE),
                  makedirs=True)
    return stage

  def testFullTests(self):
    """Tests if full unit and cros_au_test_harness tests are run correctly."""
    self._run.config['vm_tests'] = [
        config_lib.VMTestConfig(constants.FULL_AU_TEST_TYPE)
    ]
    self.RunStage()

  def testQuickTests(self):
    """Tests if quick unit and cros_au_test_harness tests are run correctly."""
    self._run.config['vm_tests'] = [
        config_lib.VMTestConfig(constants.SIMPLE_AU_TEST_TYPE)
    ]
    self.RunStage()

  def testFailedTest(self):
    """Tests if quick unit and cros_au_test_harness tests are run correctly."""
    self.PatchObject(vm_test_stages.VMTestStage, '_RunTest',
                     autospec=True, side_effect=Exception())
    self.assertRaises(failures_lib.StepFailure, self.RunStage)

  def testRaisesInfraFail(self):
    """Tests that a infra failures has been raised."""
    commands.BuildAndArchiveTestResultsTarball.side_effect = (
        OSError('Cannot archive'))
    stage = self.ConstructStage()
    self.assertRaises(failures_lib.InfrastructureFailure, stage.PerformStage)

  def testSkipVMTest(self):
    """Tests trybot with no vm test."""
    extra_cmd_args = ['--novmtests']
    self._Prepare(extra_cmd_args=extra_cmd_args)
    self.RunStage()

  def testReportTestResults(self):
    """Test trybot with reporting function."""
    self._run.config['vm_tests'] = [
        config_lib.VMTestConfig(constants.FULL_AU_TEST_TYPE)
    ]
    self._run.config['vm_test_report_to_dashboards'] = True
    self.RunStage()

  def testForgivingVMTest(self):
    """Test if a test is warn-only, it actually warns."""
    self._run.config['vm_tests'] = [
        config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                warn_only=True, test_suite='bvt-perbuild'),
        config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                warn_only=False, test_suite='bvt-arc')
    ]

    # pylint: disable=unused-argument
    def _MockRunTestSuite(buildroot, board, image_path, results_dir,
                          test_config, *args, **kwargs):
      # Only raise exception in one test.
      if test_config.test_suite == 'bvt-perbuild':
        raise Exception()
    # pylint: enable=unused-argument

    self.PatchObject(vm_test_stages, 'RunTestSuite',
                     autospec=True, side_effect=_MockRunTestSuite)
    results_lib.Results.Clear()
    self.RunStage()
    result = results_lib.Results.Get()[0]
    self.assertEqual(result.result, results_lib.Results.FORGIVEN)
    # Make sure that all tests were actually run.
    self.assertEqual(vm_test_stages.RunTestSuite.call_count,
                     len(self._run.config['vm_tests']))


class MoblabVMTestStageTestCase(
    cros_test_lib.RunCommandTestCase,
    generic_stages_unittest.AbstractStageTestCase,
    cbuildbot_unittest.SimpleBuilderTestCase,
):
  """Does what it says above."""

  BOT_ID = 'moblab-generic-vm-paladin'
  RELEASE_TAG = ''

  def setUp(self):
    self._temp_chroot_prefix = os.path.join(self.tempdir, 'chroot')
    osutils.SafeMakedirsNonRoot(self._temp_chroot_prefix)
    self._temp_host_prefix = os.path.join(self.tempdir, 'host')
    osutils.SafeMakedirsNonRoot(self._temp_host_prefix)
    self.PatchObject(commands, 'UploadArchivedFile', autospec=True)
    self._Prepare()

  def ConstructStage(self):
    self._run.GetArchive().SetupArchivePath()
    self._run.config['moblab_vm_tests'] = [
        config_lib.MoblabVMTestConfig(constants.MOBLAB_VM_SMOKE_TEST_TYPE),
    ]
    stage = vm_test_stages.MoblabVMTestStage(self._run, self._current_board)
    image_dir = stage.GetImageDirSymlink()
    osutils.Touch(os.path.join(image_dir, constants.TEST_KEY_PRIVATE),
                  makedirs=True)
    osutils.Touch(os.path.join(image_dir, constants.TEST_IMAGE_BIN),
                  makedirs=True)
    return stage

  def _temp_chroot_path(self, suffix):
    return os.path.join(self._temp_chroot_prefix, suffix)

  def _temp_host_path(self, suffix):
    return os.path.join(self._temp_host_prefix, suffix)

  def _strip_path_prefix(self, full_path):
    """Strips the host / chroot prefix from the given path."""
    if full_path.startswith(self._temp_chroot_prefix):
      return full_path.lstrip(self._temp_chroot_prefix)
    elif full_path.startswith(self._temp_host_prefix):
      return full_path.lstrip(self._temp_host_prefix)

  def testPerformStageSuccess(self):
    mock_create_test_root = self.PatchObject(
        commands, 'CreateTestRoot', autospec=True,
        return_value=self._temp_chroot_prefix)
    self.PatchObject(
        path_util,
        'FromChrootPath',
        new=lambda x: self._temp_host_path(self._strip_path_prefix(x)),
    )
    self.PatchObject(
        path_util,
        'ToChrootPath',
        new=lambda x: self._temp_chroot_path(self._strip_path_prefix(x)),
    )

    mock_gs_context = mock.create_autospec(gs.GSContext)
    self.PatchObject(gs, 'GSContext', autospec=True,
                     return_value=mock_gs_context)
    mock_buildbot_link = self.PatchObject(cros_logging, 'PrintBuildbotLink')
    mock_generate_payloads = self.PatchObject(commands, 'GeneratePayloads')
    self.PatchObject(commands, 'BuildAutotestTarballsForHWTest')
    #self.PatchObject(vm_test_stages, 'StageArtifactsOnMoblab', autospec=True)
    mock_run_moblab_tests = self.PatchObject(vm_test_stages, 'RunMoblabTests',
                                             autospec=True)
    mock_validate_results = self.PatchObject(vm_test_stages,
                                             'ValidateMoblabTestSuccess',
                                             autospec=True)

    disk_dir = os.path.join(self.tempdir, 'moblab_mounted_disk')
    osutils.SafeMakedirsNonRoot(disk_dir)
    mock_context_manager = mock.MagicMock()
    mock_context_manager.__enter__.return_value = disk_dir
    mock_moblab_vm = mock.create_autospec(moblab_vm.MoblabVm)
    mock_moblab_vm.MountedMoblabDiskContext.return_value = mock_context_manager
    self.PatchObject(moblab_vm, 'MoblabVm', autospec=True,
                     return_value=mock_moblab_vm)

    # Prepopulate results in the results directory to test result link printing.
    osutils.SafeMakedirsNonRoot(os.path.join(
        self._temp_host_prefix, 'results',
        'results-1-moblab_DummyServerNoSspSuite',
        'moblab_RunSuite', 'sysinfo', 'var', 'log', 'bootup',
    ))
    osutils.SafeMakedirsNonRoot(os.path.join(
        self._temp_host_prefix, 'results',
        'results-1-moblab_DummyServerNoSspSuite',
        'moblab_RunSuite', 'sysinfo', 'var', 'log', 'autotest',
    ))
    osutils.SafeMakedirsNonRoot(os.path.join(
        self._temp_host_prefix, 'results',
        'results-1-moblab_DummyServerNoSspSuite',
        'moblab_RunSuite', 'sysinfo', 'var', 'log_diff', 'autotest',
    ))
    osutils.SafeMakedirsNonRoot(os.path.join(
        self._temp_host_prefix, 'results',
        'results-1-moblab_DummyServerNoSspSuite',
        'sysinfo', 'mnt', 'moblab', 'results',
    ))
    self.RunStage()

    self.assertEqual(mock_create_test_root.call_count, 1)

    mock_moblab_vm.Create.assert_called_once_with(mock.ANY, mock.ANY)
    self.assertEqual(mock_moblab_vm.Start.call_count, 1)
    self.assertEqual(mock_generate_payloads.call_count, 1)
    generate_payloads_kwargs = mock_generate_payloads.call_args_list[0][1]
    self.assertTrue(os.path.isdir(generate_payloads_kwargs['archive_dir']))
    self.assertTrue(os.path.isdir(generate_payloads_kwargs['build_root']))
    self.assertTrue(
        os.path.isfile(generate_payloads_kwargs['target_image_path']))
    self.assertEqual(mock_run_moblab_tests.call_count, 1)
    run_moblab_tests_kwargs = mock_run_moblab_tests.call_args_list[0][1]
    self.assertEqual(run_moblab_tests_kwargs['moblab_board'],
                     'moblab-generic-vm')

    self.assertEqual(mock_validate_results.call_count, 1)
    # 1 for the overall results during _Upload, 4 more for the detailed logs.
    self.assertEqual(mock_buildbot_link.call_count, 5)

    self.assertEqual(mock_moblab_vm.Stop.call_count, 1)
    self.assertEqual(mock_moblab_vm.Destroy.call_count, 1)


class RunTestSuiteTest(cros_test_lib.RunCommandTempDirTestCase):
  """Test RunTestSuite functionality."""

  TEST_BOARD = 'betty'
  BUILD_ROOT = '/fake/root'

  def _RunTestSuite(self, test_config):
    vm_test_stages.RunTestSuite(self.BUILD_ROOT, self.TEST_BOARD, self.tempdir,
                                '/tmp/taco', archive_dir='/fake/root',
                                whitelist_chrome_crashes=False,
                                test_config=test_config)
    self.assertCommandContains(['--no_graphics', '--verbose'])

  def testFull(self):
    """Test running FULL config."""
    config = config_lib.VMTestConfig(constants.FULL_AU_TEST_TYPE)
    self._RunTestSuite(config)
    self.assertCommandContains(['--quick'], expected=False)
    self.assertCommandContains(['--only_verify'], expected=False)

  def testSimple(self):
    """Test SIMPLE config."""
    config = config_lib.VMTestConfig(constants.SIMPLE_AU_TEST_TYPE)
    self._RunTestSuite(config)
    self.assertCommandContains(['--quick_update'])

  def testSmoke(self):
    """Test SMOKE config."""
    config = config_lib.VMTestConfig(
        constants.VM_SUITE_TEST_TYPE, test_suite='smoke')
    self._RunTestSuite(config)
    self.assertCommandContains(['--only_verify'])

  def testGceSmokeTestType(self):
    """Test GCE test with gce-smoke suite."""
    config = config_lib.GCETestConfig(
        constants.GCE_SUITE_TEST_TYPE, test_suite='gce-smoke')
    self._RunTestSuite(config)
    self.assertCommandContains(['--only_verify'])
    self.assertCommandContains(['--type=gce'])
    self.assertCommandContains(['--suite=gce-smoke'])

  def testGceSanityTestType(self):
    """Test GCE test with gce-sanity suite."""
    config = config_lib.GCETestConfig(
        constants.GCE_SUITE_TEST_TYPE, test_suite='gce-sanity')
    self._RunTestSuite(config)
    self.assertCommandContains(['--only_verify'])
    self.assertCommandContains(['--type=gce'])
    self.assertCommandContains(['--suite=gce-sanity'])


class UnmockedTests(cros_test_lib.TempDirTestCase):
  """Test cases which really run tests, instead of using mocks."""

  def testListFaliedTests(self):
    """Tests if we can list failed tests."""
    test_report_1 = """
/tmp/taco/taste_tests/all/results-01-has_salsa              [  PASSED  ]
/tmp/taco/taste_tests/all/results-01-has_salsa/has_salsa    [  PASSED  ]
/tmp/taco/taste_tests/all/results-02-has_cheese             [  FAILED  ]
/tmp/taco/taste_tests/all/results-02-has_cheese/has_cheese  [  FAILED  ]
/tmp/taco/taste_tests/all/results-02-has_cheese/has_cheese   FAIL: No cheese.
"""
    test_report_2 = """
/tmp/taco/verify_tests/all/results-01-has_salsa              [  PASSED  ]
/tmp/taco/verify_tests/all/results-01-has_salsa/has_salsa    [  PASSED  ]
/tmp/taco/verify_tests/all/results-02-has_cheese             [  PASSED  ]
/tmp/taco/verify_tests/all/results-02-has_cheese/has_cheese  [  PASSED  ]
"""
    results_path = os.path.join(self.tempdir, 'tmp/taco')
    os.makedirs(results_path)
    # Create two reports with the same content to test that we don't
    # list the same test twice.
    osutils.WriteFile(
        os.path.join(results_path, 'taste_tests', 'all', 'test_report.log'),
        test_report_1, makedirs=True)
    osutils.WriteFile(
        os.path.join(results_path, 'taste_tests', 'failed', 'test_report.log'),
        test_report_1, makedirs=True)
    osutils.WriteFile(
        os.path.join(results_path, 'verify_tests', 'all', 'test_report.log'),
        test_report_2, makedirs=True)

    self.assertEquals(
        vm_test_stages.ListTests(results_path, show_passed=False),
        [('has_cheese', 'taste_tests/all/results-02-has_cheese')])

  def testArchiveTestResults(self):
    """Test if we can archive a test results dir."""
    test_results_dir = 'tmp/taco'
    results_path = os.path.join(self.tempdir, 'chroot', test_results_dir)
    archive_dir = os.path.join(self.tempdir, 'archived_taco')
    os.makedirs(results_path)
    os.makedirs(archive_dir)
    # File that should be archived.
    osutils.Touch(os.path.join(results_path, 'foo.txt'))
    # Flies that should be ignored.
    osutils.Touch(os.path.join(results_path,
                               'chromiumos_qemu_disk.bin.foo'))
    os.symlink('/src/foo', os.path.join(results_path, 'taco_link'))
    vm_test_stages.ArchiveTestResults(results_path, archive_dir)
    self.assertExists(os.path.join(archive_dir, 'foo.txt'))
    self.assertNotExists(
        os.path.join(archive_dir, 'chromiumos_qemu_disk.bin.foo'))
    self.assertNotExists(os.path.join(archive_dir, 'taco_link'))

  def testValidateMoblabTestSuccessNoLogsRaises(self):
    """ValidateMoblabTestSuccess raises when logs are missing."""
    os.makedirs(os.path.join(self.tempdir, 'debug'))
    with self.assertRaises(failures_lib.TestFailure):
      vm_test_stages.ValidateMoblabTestSuccess(self.tempdir)

  def testValidateMoblabTestSuccessTestNotRunRaises(self):
    """ValidateMoblabTestSuccess raises when logs indicate no test run."""
    os.makedirs(os.path.join(self.tempdir, 'debug'))
    osutils.WriteFile(
        os.path.join(self.tempdir, 'debug', 'test_that.INFO'),
        """
Some random stuff.
01/08 15:00:28.679 INFO  autoserv| [stderr] Suite job          [ PASSED ]
01/08 15:00:28.681 INFO  autoserv| [stderr]
01/08 15:00:28.681 INFO  autoserv| [stderr] Suite timings:"""
    )
    with self.assertRaises(failures_lib.TestFailure):
      vm_test_stages.ValidateMoblabTestSuccess(self.tempdir)

  def testValidateMoblabTestSuccessTestFailedRaises(self):
    """ValidateMoblabTestSuccess raises when logs indicate test failed."""
    os.makedirs(os.path.join(self.tempdir, 'debug'))
    osutils.WriteFile(
        os.path.join(self.tempdir, 'debug', 'test_that.INFO'),
        """
Some random stuff.
01/08 15:00:28.679 INFO  autoserv| [stderr] Suite job          [ PASSED ]
01/08 15:00:28.680 INFO  autoserv| [stderr] dummy_PassServer   [ FAILED ]
01/08 15:00:28.681 INFO  autoserv| [stderr]
01/08 15:00:28.681 INFO  autoserv| [stderr] Suite timings:"""
    )
    with self.assertRaises(failures_lib.TestFailure):
      vm_test_stages.ValidateMoblabTestSuccess(self.tempdir)

  def testValidateMoblabTestSuccessTestPassed(self):
    """ValidateMoblabTestSuccess succeeds when logs indicate test passed."""
    os.makedirs(os.path.join(self.tempdir, 'debug'))
    osutils.WriteFile(
        os.path.join(self.tempdir, 'debug', 'test_that.INFO'),
        """
Some random stuff.
01/08 15:00:28.679 INFO  autoserv| [stderr] Suite job          [ PASSED ]
01/08 15:00:28.680 INFO  autoserv| [stderr] dummy_PassServer   [ PASSED ]
01/08 15:00:28.681 INFO  autoserv| [stderr]
01/08 15:00:28.681 INFO  autoserv| [stderr] Suite timings:"""
    )
    vm_test_stages.ValidateMoblabTestSuccess(self.tempdir)
