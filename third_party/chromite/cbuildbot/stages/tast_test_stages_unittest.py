# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for tast_test_stages."""

from __future__ import print_function

import json
import os

from chromite.cbuildbot import cbuildbot_unittest
from chromite.cbuildbot import commands
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.cbuildbot.stages import tast_test_stages
from chromite.lib import cgroups
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import failures_lib
from chromite.lib import osutils
from chromite.lib import results_lib


class TastVMTestStageTest(generic_stages_unittest.AbstractStageTestCase,
                          cbuildbot_unittest.SimpleBuilderTestCase):
  """Tests for tast_test_stages.TastVMTestStage."""

  BOT_ID = 'amd64-generic-full'
  RELEASE_TAG = ''

  # Directory relative to the chroot containing per-suite results dir.
  RESULTS_CHROOT_PATH = '/tmp/tast_vm_results'

  # Results file and content to write within per-suite results dir.
  RESULTS_OUT_BASENAME = 'results.txt'
  RESULTS_OUT_DATA = 'example output'

  def setUp(self):
    # TastVMTestStage being tested.
    self._stage = None

    # String test suite set in the TastVMTestConfig.
    self._exp_test_suite = None

    # List of string test expressions expected to be passed to the tast command.
    self._exp_test_exprs = []

    # Array of dicts to be written to tast_test_stages.RESULTS_FILENAME as test
    # results. Not written if None. If a string is specified, it will be written
    # directly.
    self._test_results_data = []

    # Integer exit code to be returned by _FakeRunCommand.
    self._run_command_exit_code = 0

    # Optional exception to be raised by UploadArtifact.
    self._artifact_exception = None

    # Note that autospec=True instructs the mock library to verify that methods
    # that are called on the mock object actually exist and are passed valid
    # args. Without autospec=True, calls like mocked_object.nonexistent_method()
    # will succeed and return new mock objects.
    self._test_root = os.path.join(
        self.build_root, constants.DEFAULT_CHROOT_DIR,
        TastVMTestStageTest.RESULTS_CHROOT_PATH.lstrip('/'))
    os.makedirs(self._test_root)
    self._mock_create_test_root = self.PatchObject(commands, 'CreateTestRoot',
                                                   autospec=True)
    self._mock_create_test_root.return_value = \
        TastVMTestStageTest.RESULTS_CHROOT_PATH

    self._mock_run_command = self.PatchObject(cros_build_lib, 'RunCommand',
                                              autospec=True)
    self._mock_run_command.side_effect = self._FakeRunCommand

    # Mock out functions that make calls to cros_build_lib.RunCommand that we
    # don't want to see.
    self.PatchObject(osutils, 'RmDir', autospec=True)
    self.PatchObject(cgroups, 'SimpleContainChildren', autospec=True)

    # Define mocked functions that can only be created once we've created the
    # stage in ConstructStage.
    self._mock_upload_artifact = None
    self._mock_print_download_link = None

    self._Prepare()

  def ConstructStage(self):
    # pylint: disable=W0212
    self._run.GetArchive().SetupArchivePath()
    self._stage = tast_test_stages.TastVMTestStage(self._run,
                                                   self._current_board)
    self._stage._attempt = 1
    image_dir = self._stage.GetImageDirSymlink()
    osutils.Touch(os.path.join(image_dir, constants.TEST_KEY_PRIVATE),
                  makedirs=True)

    # Mock out some of the methods that TastVMTestStage inherits from
    # generic_stages. This is gross, but slightly less gross than mocking out
    # everything called by generic_stages.
    self._mock_upload_artifact = \
        self.PatchObject(self._stage, 'UploadArtifact', autospec=True)
    self._mock_upload_artifact.side_effect = self._artifact_exception
    self._mock_print_download_link = \
        self.PatchObject(self._stage, 'PrintDownloadLink', autospec=True)

    return self._stage

  def _FakeRunCommand(self, cmd, **kwargs):
    """Fake implemenation of cros_build_lib.RunCommand."""
    # pylint: disable=unused-argument
    # Just check positional args and tricky flags. Checking all args is an
    # exercise in verifying that we're capable of typing the same thing twice.
    pos = [a for a in cmd if not a.startswith('--')]
    self.assertGreater(len(pos), 0)
    self.assertEqual(pos[0], tast_test_stages.TastVMTestStage.SCRIPT_PATH)
    self.assertEqual(pos[1:], self._exp_test_exprs)

    # The passed results dir should be relative to the chroot and should contain
    # the test suite.
    results_arg = '--results_dir=' + \
        os.path.join(TastVMTestStageTest.RESULTS_CHROOT_PATH,
                     self._exp_test_suite)
    self.assertIn(results_arg, cmd)

    if self._test_results_data is not None:
      self._WriteResultsFile(self._test_results_data)
    return cros_build_lib.CommandResult(returncode=self._run_command_exit_code)

  def _GetResultsFilePath(self):
    """Returns the path to the results file."""
    results_dir = os.path.join(self._test_root, self._exp_test_suite)
    if not os.path.isdir(results_dir):
      os.makedirs(results_dir)
    return os.path.join(results_dir, tast_test_stages.RESULTS_FILENAME)

  def _WriteResultsFile(self, data):
    """Writes a results file within the suite's results dir."""
    with open(self._GetResultsFilePath(), 'w') as f:
      if isinstance(data, str):
        f.write(data)
      else:
        json.dump(data, f)

  def _SetSuite(self, suite_name, test_exprs):
    """Configures the test framework to run a given suite."""
    self._exp_test_suite = suite_name
    self._exp_test_exprs = test_exprs
    self._run.config['tast_vm_tests'] = \
        [config_lib.TastVMTestConfig(suite_name, list(test_exprs))]

  def _VerifyArtifacts(self):
    """Verifies that results were archived and queued to be uploaded."""
    # pylint: disable=W0212
    archive_dir = constants.TAST_VM_TEST_RESULTS % \
        {'attempt': self._stage._attempt}
    self.assertEqual(os.listdir(self._stage.archive_path), [archive_dir])
    archived_results_path = os.path.join(self._stage.archive_path, archive_dir,
                                         self._exp_test_suite,
                                         tast_test_stages.RESULTS_FILENAME)
    self.assertTrue(os.path.isfile(archived_results_path))
    self._mock_upload_artifact.assert_called_once_with(
        archive_dir, archive=False, strict=False)

    # There should be a download link for results and for each failed test.
    self._mock_print_download_link.assert_any_call(
        archive_dir, tast_test_stages.RESULTS_LINK_PREFIX)
    num_failed_tests = 0
    with open(archived_results_path, 'r') as f:
      for test in json.load(f):
        if test[tast_test_stages.RESULTS_ERRORS_KEY]:
          num_failed_tests += 1
          flaky = tast_test_stages.RESULTS_FLAKY_ATTR in \
              test.get(tast_test_stages.RESULTS_ATTR_KEY, [])

          name = test[tast_test_stages.RESULTS_NAME_KEY]
          test_url = os.path.join(archive_dir, self._exp_test_suite,
                                  tast_test_stages.RESULTS_TESTS_DIR, name)
          desc = tast_test_stages.FLAKY_PREFIX + name if flaky else name
          self._mock_print_download_link.assert_any_call(
              test_url, text_to_display=desc)
    self.assertEqual(self._mock_print_download_link.call_count,
                     num_failed_tests + 1)

  def _VerifyStageResult(self, result, description):
    """Verifies that the stage reported the expected result.

    Args:
      result: Either a string result constant from results_lib.Results
              (e.g. SUCCESS, FORGIVEN, SKIPPED) or (in the case of a failure)
              the exception class thrown by the test (e.g.
              failures_lib.TestFailure).
      description: String exactly matching description in results_lib.Results().
    """
    self.assertEqual(
        [(r.name,
          r.result.__class__ if isinstance(r.result, Exception) else r.result,
          r.description) for r in results_lib.Results.Get()],
        [('TastVMTest', result, description)])

  def testSuccess(self):
    """Perform a full test suite run."""
    self._SetSuite('good_test_suite', ['(bvt && chrome)', '(bvt && arc)'])
    self._test_results_data = [{'name': 'example.Pass', 'errors': None}]
    self.RunStage()
    self._VerifyStageResult(results_lib.Results.SUCCESS, None)

    self._mock_create_test_root.assert_called_once_with(self.build_root)
    self.assertEquals(self._mock_run_command.call_count, 1)
    self._VerifyArtifacts()

  def testNonZeroExitCode(self):
    """Tests that internal errors from the tast command are reported."""
    self._SetSuite('non_zero_exit_code_test_suite', [])
    self._test_results_data = [
        {'name': 'example.Fail', 'errors': [{'reason': 'Failed!'}]},
    ]
    self._run_command_exit_code = 1

    self.assertRaises(failures_lib.TestFailure, self.RunStage)
    self._VerifyStageResult(failures_lib.TestFailure,
                            tast_test_stages.FAILURE_EXIT_CODE % 1)

    self._mock_create_test_root.assert_called_once_with(self.build_root)
    self.assertEquals(self._mock_run_command.call_count, 1)
    self._VerifyArtifacts()

  def testFailedTest(self):
    """Tests that test failures are reported."""
    self._SetSuite('failed_test_suite', [])
    self._test_results_data = [
        {'name': 'example.Pass', 'errors': None},
        {'name': 'example.Fail', 'errors': [{'reason': 'Failed!'}]},
    ]

    self.assertRaises(failures_lib.TestFailure, self.RunStage)
    self._VerifyStageResult(failures_lib.TestFailure,
                            tast_test_stages.FAILURE_TESTS_FAILED % 1)

    self._mock_create_test_root.assert_called_once_with(self.build_root)
    self.assertEquals(self._mock_run_command.call_count, 1)
    self._VerifyArtifacts()

  def testFlakyTest(self):
    """Tests that errors in flaky tests don't fail the stage."""
    self._SetSuite('flaky_test_suite', ['(flaky)'])
    self._test_results_data = [
        {
            'name': 'example.Flaky',
            'attr': ['flaky'],
            'errors': [{'reason': 'Failed!'}],
        },
    ]

    self.RunStage()
    self._VerifyStageResult(results_lib.Results.SUCCESS, None)
    self._VerifyArtifacts()

  def testMissingResultsDir(self):
    """Tests that an error is returned if the results dir is missing."""
    self._SetSuite('missing_results_test_suite', [])
    self._test_results_data = None

    self.assertRaises(failures_lib.TestFailure, self.RunStage)
    self._VerifyStageResult(failures_lib.TestFailure,
                            tast_test_stages.FAILURE_NO_RESULTS %
                            self._test_root)

  def testBadResultsFile(self):
    """Tests that an error is returned if the results file is unreadable."""
    self._SetSuite('bad_results_test_suite', [])
    self._test_results_data = 'bogus'

    self.assertRaises(failures_lib.TestFailure, self.RunStage)
    self._VerifyStageResult(failures_lib.TestFailure,
                            tast_test_stages.FAILURE_BAD_RESULTS %
                            (self._GetResultsFilePath(),
                             'No JSON object could be decoded'))

  def testFailedArchive(self):
    """Tests that archive failures raise InfrastructureFailure."""
    self._SetSuite('failed_archive_test_suite', [])
    self._artifact_exception = Exception('upload failed')
    self.ConstructStage()
    self.assertRaises(failures_lib.InfrastructureFailure,
                      self._stage.PerformStage)


class CopyResultsDirTest(cros_test_lib.TempDirTestCase):
  """Tests for tast_test_stages._CopyResultsDir."""

  def setUp(self):
    self.src = os.path.join(self.tempdir, 'src')
    self.dest = os.path.join(self.tempdir, 'dest')

  def _WriteSrcFile(self, path, data):
    """Creates a file within self.src.

    Args:
      path: String containing relative path to create within self.src.
      data: String data to write to file.
    """
    full_path = os.path.join(self.src, path)
    dir_path = os.path.dirname(full_path)
    if not os.path.exists(dir_path):
      os.makedirs(dir_path)
    with open(full_path, 'w') as f:
      f.write(data)

  def _DoCopy(self):
    """Copies from src to dest directory."""
    # pylint: disable=W0212
    tast_test_stages._CopyResultsDir(self.src, self.dest)

  def testCopyAll(self):
    """Tests that files and directories are recursively copied>"""
    path1 = 'myfile.txt'
    data1 = 'foo'
    self._WriteSrcFile(path1, data1)

    path2 = 'subdir/other.txt'
    data2 = 'bar'
    self._WriteSrcFile(path2, data2)

    empty_dir = 'empty'
    os.makedirs(os.path.join(self.src, empty_dir))

    self._DoCopy()
    self.assertFileContents(os.path.join(self.dest, path1), data1)
    self.assertFileContents(os.path.join(self.dest, path2), data2)
    self.assertExists(os.path.join(self.dest, empty_dir))

  def testDestAlreadyExists(self):
    """Tests that OSError is raised if the destination dir already exists."""
    self._WriteSrcFile('myfile.txt', 'foo')
    os.makedirs(self.dest)
    self.assertRaises(OSError, self._DoCopy)

  def testSkipSymlink(self):
    """Tests that symlinks are skipped."""
    path = 'myfile.txt'
    data = 'foo'
    self._WriteSrcFile(path, data)

    link = 'symlink.txt'
    os.symlink(path, os.path.join(self.src, link))

    self._DoCopy()
    self.assertFileContents(os.path.join(self.dest, path), data)
    self.assertNotExists(os.path.join(self.dest, link))
