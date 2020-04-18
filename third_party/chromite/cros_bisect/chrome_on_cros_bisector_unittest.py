# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test chrome_on_cros_bisector module."""

from __future__ import print_function

import copy
import mock
import itertools
import os

from chromite.cli import flash
from chromite.cros_bisect import common
from chromite.cros_bisect import builder as builder_module
from chromite.cros_bisect import evaluator as evaluator_module
from chromite.cros_bisect import chrome_on_cros_bisector
from chromite.cros_bisect import git_bisector_unittest
from chromite.lib import commandline
from chromite.lib import cros_test_lib
from chromite.lib import gs
from chromite.lib import gs_unittest
from chromite.lib import partial_mock


class DummyEvaluator(evaluator_module.Evaluator):
  """Evaluator which just return empty score."""

  # pylint: disable=unused-argument
  def Evaluate(self, remote, build_label, repeat):
    return common.Score()

  def CheckLastEvaluate(self, build_label, repeat=1):
    return common.Score()

class TestChromeOnCrosBisector(cros_test_lib.MockTempDirTestCase):
  """Tests ChromeOnCrosBisector class."""

  BOARD = 'samus'
  TEST_NAME = 'graphics_WebGLAquarium'
  METRIC = 'avg_fps_1000_fishes/summary/value'
  REPORT_FILE = 'reports.json'
  DUT_ADDR = '192.168.1.1'
  DUT = commandline.DeviceParser(commandline.DEVICE_SCHEME_SSH)(DUT_ADDR)

  # Be aware that GOOD_COMMIT_INFO and BAD_COMMIT_INFO should be assigned via
  # copy.deepcopy() as their users are likely to change the content.
  GOOD_COMMIT_SHA1 = '44af5c9a5505'
  GOOD_COMMIT_TIMESTAMP = 1486526594
  GOOD_COMMIT_SCORE = common.Score([100])
  GOOD_COMMIT_INFO = common.CommitInfo(
      sha1=GOOD_COMMIT_SHA1, timestamp=GOOD_COMMIT_TIMESTAMP, title='good',
      label='last-known-good  ', score=GOOD_COMMIT_SCORE)

  BAD_COMMIT_SHA1 = '6a163bb66c3e'
  BAD_COMMIT_TIMESTAMP = 1486530021
  BAD_COMMIT_SCORE = common.Score([80])
  BAD_COMMIT_INFO = common.CommitInfo(
      sha1=BAD_COMMIT_SHA1, timestamp=BAD_COMMIT_TIMESTAMP, title='bad',
      label='last-known-bad   ', score=BAD_COMMIT_SCORE)

  GOOD_CROS_VERSION = 'R60-9592.50.0'
  BAD_CROS_VERSION = 'R60-9592.51.0'

  CULPRIT_COMMIT_SHA1 = '12345abcde'
  CULPRIT_COMMIT_TIMESTAMP = 1486530000
  CULPRIT_COMMIT_SCORE = common.Score([81])
  CULPRIT_COMMIT_INFO = common.CommitInfo(
      sha1=CULPRIT_COMMIT_SHA1, timestamp=CULPRIT_COMMIT_TIMESTAMP, title='bad',
      score=CULPRIT_COMMIT_SCORE)

  THRESHOLD_SPLITTER = 95  # Score between good and bad, closer to good side.
  THRESHOLD = 5  # Distance between good score and splitter.

  REPEAT = 3

  GOOD_METADATA_CONTENT = '\n'.join([
      '{',
      '  "metadata-version": "2",',
      '  "toolchain-url": "2017/05/%(target)s-2017.05.25.101355.tar.xz",',
      '  "suite_scheduling": true,',
      '  "build_id": 1644146,',
      '  "version": {',
      '    "full": "R60-9592.50.0",',
      '    "android-branch": "git_mnc-dr-arc-m60",',
      '    "chrome": "60.0.3112.53",',
      '    "platform": "9592.50.0",',
      '    "milestone": "60",',
      '    "android": "4150402"',
      '  }',
      '}'])

  def setUp(self):
    """Sets up test case."""
    self.options = cros_test_lib.EasyAttr(
        base_dir=self.tempdir, board=self.BOARD, reuse_repo=True,
        good=self.GOOD_COMMIT_SHA1, bad=self.BAD_COMMIT_SHA1, remote=self.DUT,
        eval_repeat=self.REPEAT, auto_threshold=False, reuse_eval=False,
        cros_flash_sleep=0.01, cros_flash_retry=3, cros_flash_backoff=1,
        eval_raise_on_error=False, skip_failed_commit=False)

    self.repo_dir = os.path.join(self.tempdir,
                                 builder_module.Builder.DEFAULT_REPO_DIR)
    self.SetUpBisector()

  def SetUpBisector(self):
    """Instantiates self.bisector using self.options."""
    self.evaluator = DummyEvaluator(self.options)
    self.builder = builder_module.Builder(self.options)
    self.bisector = chrome_on_cros_bisector.ChromeOnCrosBisector(
        self.options, self.builder, self.evaluator)

  def SetUpBisectorWithCrosVersion(self):
    """Instantiates self.bisector using CrOS version as good and bad options."""
    self.options.good = self.GOOD_CROS_VERSION
    self.options.bad = self.BAD_CROS_VERSION
    self.SetUpBisector()

  def SetDefaultCommitInfo(self):
    """Sets up default commit info."""
    self.bisector.good_commit_info = copy.deepcopy(self.GOOD_COMMIT_INFO)
    self.bisector.bad_commit_info = copy.deepcopy(self.BAD_COMMIT_INFO)

  def testInit(self):
    """Tests __init__() with SHA1 as good and bad options."""
    self.assertEqual(self.GOOD_COMMIT_SHA1, self.bisector.good_commit)
    self.assertIsNone(self.bisector.good_cros_version)
    self.assertEqual(self.BAD_COMMIT_SHA1, self.bisector.bad_commit)
    self.assertIsNone(self.bisector.bad_cros_version)
    self.assertFalse(self.bisector.bisect_between_cros_version)

    self.assertEqual(self.DUT_ADDR, self.bisector.remote.raw)
    self.assertEqual(self.REPEAT, self.bisector.eval_repeat)
    self.assertEqual(self.builder, self.bisector.builder)
    self.assertEqual(self.repo_dir, self.bisector.repo_dir)

    self.assertIsNone(self.bisector.good_commit_info)
    self.assertIsNone(self.bisector.bad_commit_info)
    self.assertEqual(0, len(self.bisector.bisect_log))
    self.assertIsNone(self.bisector.threshold)
    self.assertTrue(not self.bisector.current_commit)

  def testInitCrosVersion(self):
    """Tests __init__() with CrOS version as good and bad options."""
    self.SetUpBisectorWithCrosVersion()

    self.assertEqual(self.GOOD_CROS_VERSION, self.bisector.good_cros_version)
    self.assertIsNone(self.bisector.good_commit)
    self.assertEqual(self.BAD_CROS_VERSION, self.bisector.bad_cros_version)
    self.assertIsNone(self.bisector.bad_commit)
    self.assertTrue(self.bisector.bisect_between_cros_version)

    self.assertEqual(self.DUT_ADDR, self.bisector.remote.raw)
    self.assertEqual(self.REPEAT, self.bisector.eval_repeat)
    self.assertEqual(self.builder, self.bisector.builder)
    self.assertEqual(self.repo_dir, self.bisector.repo_dir)

    self.assertIsNone(self.bisector.good_commit_info)
    self.assertIsNone(self.bisector.bad_commit_info)
    self.assertEqual(0, len(self.bisector.bisect_log))
    self.assertIsNone(self.bisector.threshold)
    self.assertTrue(not self.bisector.current_commit)

  def testInitMissingRequiredArgs(self):
    """Tests that ChromeOnCrosBisector raises for missing required argument."""
    options = cros_test_lib.EasyAttr()
    with self.assertRaises(common.MissingRequiredOptionsException) as cm:
      chrome_on_cros_bisector.ChromeOnCrosBisector(options, self.builder,
                                                   self.evaluator)
    exception_message = cm.exception.message
    self.assertTrue('Missing command line' in exception_message)
    self.assertTrue('ChromeOnCrosBisector' in exception_message)
    for arg in chrome_on_cros_bisector.ChromeOnCrosBisector.REQUIRED_ARGS:
      self.assertTrue(arg in exception_message)

  def testCheckCommitFormat(self):
    """Tests CheckCommitFormat()."""
    CheckCommitFormat = (
        chrome_on_cros_bisector.ChromeOnCrosBisector.CheckCommitFormat)
    self.assertEqual(self.GOOD_COMMIT_SHA1,
                     CheckCommitFormat(self.GOOD_COMMIT_SHA1))
    self.assertEqual(self.GOOD_CROS_VERSION,
                     CheckCommitFormat(self.GOOD_CROS_VERSION))
    self.assertEqual('R60-9592.50.0',
                     CheckCommitFormat('60.9592.50.0'))
    invalid = 'bad_sha1'
    self.assertIsNone(CheckCommitFormat(invalid))

  def testObtainBisectBoundaryScoreImpl(self):
    """Tests ObtainBisectBoundaryScoreImpl()."""
    git_mock = self.StartPatcher(git_bisector_unittest.GitMock(self.repo_dir))
    git_mock.AddRunGitResult(['checkout', self.GOOD_COMMIT_SHA1])
    git_mock.AddRunGitResult(['checkout', self.BAD_COMMIT_SHA1])

    build_deploy_eval_mock = self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector, 'BuildDeployEval')
    build_deploy_eval_mock.side_effect = [self.GOOD_COMMIT_SCORE,
                                          self.BAD_COMMIT_SCORE]

    self.assertEqual(self.GOOD_COMMIT_SCORE,
                     self.bisector.ObtainBisectBoundaryScoreImpl(True))
    self.assertEqual(self.BAD_COMMIT_SCORE,
                     self.bisector.ObtainBisectBoundaryScoreImpl(False))

    self.assertEqual(
        [mock.call(customize_build_deploy=None, eval_label=None),
         mock.call(customize_build_deploy=None, eval_label=None)],
        build_deploy_eval_mock.call_args_list)

  def testObtainBisectBoundaryScoreImplCrosVersion(self):
    """Tests ObtainBisectBoundaryScoreImpl() with CrOS version."""
    self.SetUpBisectorWithCrosVersion()
    # Inject good_commit and bad_commit as if
    # bisector.ResolveChromeBisectRangeFromCrosVersion() being run.
    self.bisector.good_commit = self.GOOD_COMMIT_SHA1
    self.bisector.bad_commit = self.BAD_COMMIT_SHA1

    git_mock = self.StartPatcher(git_bisector_unittest.GitMock(self.repo_dir))
    git_mock.AddRunGitResult(['checkout', self.GOOD_COMMIT_SHA1])
    git_mock.AddRunGitResult(['checkout', self.BAD_COMMIT_SHA1])

    self.PatchObject(chrome_on_cros_bisector.ChromeOnCrosBisector,
                     'UpdateCurrentCommit')
    evaluate_mock = self.PatchObject(DummyEvaluator, 'Evaluate')

    # Mock FlashCrosImage() to verify that customize_build_deploy is assigned
    # as expected.
    flash_cros_image_mock = self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector, 'FlashCrosImage')

    evaluate_mock.return_value = self.GOOD_COMMIT_SCORE
    self.assertEqual(self.GOOD_COMMIT_SCORE,
                     self.bisector.ObtainBisectBoundaryScoreImpl(True))
    flash_cros_image_mock.assert_called_with(
        self.bisector.GetCrosXbuddyPath(self.GOOD_CROS_VERSION))
    evaluate_mock.assert_called_with(
        self.DUT, 'cros_%s' % self.GOOD_CROS_VERSION, self.REPEAT)

    evaluate_mock.return_value = self.BAD_COMMIT_SCORE
    self.assertEqual(self.BAD_COMMIT_SCORE,
                     self.bisector.ObtainBisectBoundaryScoreImpl(False))
    flash_cros_image_mock.assert_called_with(
        self.bisector.GetCrosXbuddyPath(self.BAD_CROS_VERSION))
    evaluate_mock.assert_called_with(
        self.DUT, 'cros_%s' % self.BAD_CROS_VERSION, self.REPEAT)

  def testObtainBisectBoundaryScoreImplCrosVersionFlashError(self):
    """Tests ObtainBisectBoundaryScoreImpl() with CrOS version."""
    self.SetUpBisectorWithCrosVersion()
    # Inject good_commit and bad_commit as if
    # bisector.ResolveChromeBisectRangeFromCrosVersion() being run.
    self.bisector.good_commit = self.GOOD_COMMIT_SHA1
    self.bisector.bad_commit = self.BAD_COMMIT_SHA1

    git_mock = self.StartPatcher(git_bisector_unittest.GitMock(self.repo_dir))
    git_mock.AddRunGitResult(['checkout', self.GOOD_COMMIT_SHA1])
    git_mock.AddRunGitResult(['checkout', self.BAD_COMMIT_SHA1])

    self.PatchObject(chrome_on_cros_bisector.ChromeOnCrosBisector,
                     'UpdateCurrentCommit')
    evaluate_mock = self.PatchObject(DummyEvaluator, 'Evaluate')

    # Mock FlashCrosImage() to verify that customize_build_deploy is assigned
    # as expected.
    flash_cros_image_mock = self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector, 'FlashCrosImage')
    flash_cros_image_mock.side_effect = flash.FlashError('Flash failed.')

    with self.assertRaises(flash.FlashError):
      self.bisector.ObtainBisectBoundaryScoreImpl(True)

    flash_cros_image_mock.assert_called_with(
        self.bisector.GetCrosXbuddyPath(self.GOOD_CROS_VERSION))
    evaluate_mock.assert_not_called()

    with self.assertRaises(flash.FlashError):
      self.bisector.ObtainBisectBoundaryScoreImpl(False)
    flash_cros_image_mock.assert_called_with(
        self.bisector.GetCrosXbuddyPath(self.BAD_CROS_VERSION))
    evaluate_mock.assert_not_called()

  def testGetCrosXbuddyPath(self):
    """Tests GetCrosXbuddyPath()."""
    self.assertEqual(
        'xbuddy://remote/%s/%s/test' % (self.BOARD, self.GOOD_CROS_VERSION),
        self.bisector.GetCrosXbuddyPath(self.GOOD_CROS_VERSION))

  def testExchangeChromeSanityCheck(self):
    """Tests the flow of exchanging Chrome between good and bad CrOS."""
    self.SetUpBisectorWithCrosVersion()

    # Inject good_commit and bad_commit as if
    # bisector.ResolveChromeBisectRangeFromCrosVersion() has been run.
    self.bisector.good_commit = self.GOOD_COMMIT_SHA1
    self.bisector.bad_commit = self.BAD_COMMIT_SHA1

    # Inject commit_info and threshold as if
    # bisector.ObtainBisectBoundaryScore() and bisector.GetThresholdFromUser()
    # has been run.
    self.SetDefaultCommitInfo()
    self.bisector.threshold = self.THRESHOLD

    # Try bad Chrome first.
    git_mock = self.StartPatcher(git_bisector_unittest.GitMock(self.repo_dir))
    git_mock.AddRunGitResult(['checkout', self.BAD_COMMIT_SHA1])
    git_mock.AddRunGitResult(['checkout', self.GOOD_COMMIT_SHA1])

    self.PatchObject(chrome_on_cros_bisector.ChromeOnCrosBisector,
                     'UpdateCurrentCommit')

    evaluate_mock = self.PatchObject(DummyEvaluator, 'Evaluate')
    expected_evaluate_calls = [
        mock.call(self.DUT, x, self.REPEAT) for x in [
            'cros_%s_cr_%s' % (self.GOOD_CROS_VERSION, self.BAD_COMMIT_SHA1),
            'cros_%s_cr_%s' % (self.BAD_CROS_VERSION, self.GOOD_COMMIT_SHA1)]]

    # Mock FlashCrosImage() to verify that customize_build_deploy is assigned
    # as expected.
    flash_cros_image_mock = self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector, 'FlashCrosImage')
    expected_flash_cros_calls = [
        mock.call(self.bisector.GetCrosXbuddyPath(self.GOOD_CROS_VERSION)),
        mock.call(self.bisector.GetCrosXbuddyPath(self.BAD_CROS_VERSION))]

    # Make sure bisector.BuildDeploy() is also called.
    build_deploy_mock = self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector, 'BuildDeploy')

    # Assume culprit commit is in Chrome side, i.e. first score is bad.
    evaluate_mock.side_effect = [self.BAD_COMMIT_SCORE, self.GOOD_COMMIT_SCORE]

    self.assertTrue(self.bisector.ExchangeChromeSanityCheck())
    flash_cros_image_mock.assert_has_calls(expected_flash_cros_calls)
    evaluate_mock.assert_has_calls(expected_evaluate_calls)
    self.assertEqual(2, build_deploy_mock.call_count)

    flash_cros_image_mock.reset_mock()
    evaluate_mock.reset_mock()
    build_deploy_mock.reset_mock()

    # Assume culprit commit is not in Chrome side, i.e. first score is good.
    evaluate_mock.side_effect = [self.GOOD_COMMIT_SCORE, self.BAD_COMMIT_SCORE]

    self.assertFalse(self.bisector.ExchangeChromeSanityCheck())
    flash_cros_image_mock.assert_has_calls(expected_flash_cros_calls)
    evaluate_mock.assert_has_calls(expected_evaluate_calls)
    self.assertEqual(2, build_deploy_mock.call_count)

  def testExchangeChromeSanityCheckFlashError(self):
    """Tests the flow of exchanging Chrome between good and bad CrOS."""
    self.SetUpBisectorWithCrosVersion()

    # Inject good_commit and bad_commit as if
    # bisector.ResolveChromeBisectRangeFromCrosVersion() has been run.
    self.bisector.good_commit = self.GOOD_COMMIT_SHA1
    self.bisector.bad_commit = self.BAD_COMMIT_SHA1

    # Inject commit_info and threshold as if
    # bisector.ObtainBisectBoundaryScore() and bisector.GetThresholdFromUser()
    # has been run.
    self.SetDefaultCommitInfo()
    self.bisector.threshold = self.THRESHOLD

    # Try bad Chrome first.
    git_mock = self.StartPatcher(git_bisector_unittest.GitMock(self.repo_dir))
    git_mock.AddRunGitResult(['checkout', self.BAD_COMMIT_SHA1])
    git_mock.AddRunGitResult(['checkout', self.GOOD_COMMIT_SHA1])

    self.PatchObject(chrome_on_cros_bisector.ChromeOnCrosBisector,
                     'UpdateCurrentCommit')

    evaluate_mock = self.PatchObject(DummyEvaluator, 'Evaluate')

    # Mock FlashCrosImage() to verify that customize_build_deploy is assigned
    # as expected.
    flash_cros_image_mock = self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector, 'FlashCrosImage',
        side_effect=flash.FlashError('Flash failed.'))

    build_deploy_mock = self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector, 'BuildDeploy')

    with self.assertRaises(flash.FlashError):
      self.bisector.ExchangeChromeSanityCheck()

    evaluate_mock.assert_not_called()
    flash_cros_image_mock.assert_called()
    build_deploy_mock.assert_not_called()

  def testFlashImage(self):
    """Tests FlashImage()."""
    flash_mock = self.PatchObject(flash, 'Flash')
    xbuddy_path = self.bisector.GetCrosXbuddyPath(self.GOOD_CROS_VERSION)
    self.bisector.FlashCrosImage(xbuddy_path)
    flash_mock.assert_called_with(
        self.DUT, xbuddy_path, board=self.BOARD, clobber_stateful=True,
        disable_rootfs_verification=True)

  def testFlashImageRetry(self):
    """Tests FlashImage() with retry success."""

    flash_mock_call_counter = itertools.count()
    def flash_mock_return(*unused_args, **unused_kwargs):
      nth_call = flash_mock_call_counter.next()
      if nth_call < 3:
        raise flash.FlashError('Flash failed.')
      return

    flash_mock = self.PatchObject(flash, 'Flash')
    flash_mock.side_effect = flash_mock_return
    xbuddy_path = self.bisector.GetCrosXbuddyPath(self.GOOD_CROS_VERSION)
    self.bisector.FlashCrosImage(xbuddy_path)
    flash_mock.assert_called_with(
        self.DUT, xbuddy_path, board=self.BOARD, clobber_stateful=True,
        disable_rootfs_verification=True)

  def testFlashImageRetryFailed(self):
    """Tests FlashImage() with retry failed."""
    flash_mock = self.PatchObject(flash, 'Flash')
    flash_mock.side_effect = flash.FlashError('Flash failed.')
    xbuddy_path = self.bisector.GetCrosXbuddyPath(self.GOOD_CROS_VERSION)
    with self.assertRaises(flash.FlashError):
      self.bisector.FlashCrosImage(xbuddy_path)
    flash_mock.assert_called_with(
        self.DUT, xbuddy_path, board=self.BOARD, clobber_stateful=True,
        disable_rootfs_verification=True)

  def testCrosVersionToChromeCommit(self):
    """Tests CrosVersionToChromeCommit()."""
    metadata_url = (
        'gs://chromeos-image-archive/%s-release/%s/partial-metadata.json' %
        (self.BOARD, self.GOOD_CROS_VERSION))
    gs_mock = self.StartPatcher(gs_unittest.GSContextMock())
    gs_mock.AddCmdResult(['cat', metadata_url],
                         output=self.GOOD_METADATA_CONTENT)

    git_log_content = '\n'.join([
        '8967dd66ad72 (tag: 60.0.3112.53) Publish DEPS for Chromium '
        '60.0.3112.53',
        '27ed0cc0c2f4 Incrementing VERSION to 60.0.3112.53'])
    git_mock = self.StartPatcher(git_bisector_unittest.GitMock(self.repo_dir))
    git_mock.AddRunGitResult(['log', '--oneline', '-n', '2', '60.0.3112.53'],
                             output=git_log_content)

    self.bisector.gs_ctx = gs.GSContext()
    self.assertEqual(
        '27ed0cc0c2f4',
        self.bisector.CrosVersionToChromeCommit(self.GOOD_CROS_VERSION))

  def testCrosVersionToChromeCommitFail(self):
    """Tests failure case of CrosVersionToChromeCommit()."""
    metadata_url = (
        'gs://chromeos-image-archive/%s-release/%s/partial-metadata.json' %
        (self.BOARD, self.GOOD_CROS_VERSION))
    gs_mock = self.StartPatcher(gs_unittest.GSContextMock())
    gs_mock.AddCmdResult(['cat', metadata_url], returncode=1)

    self.bisector.gs_ctx = gs.GSContext()
    self.assertIsNone(
        self.bisector.CrosVersionToChromeCommit(self.GOOD_CROS_VERSION))

    metadata_content = 'not_a_json'
    gs_mock.AddCmdResult(['cat', metadata_url], output=metadata_content)
    self.assertIsNone(
        self.bisector.CrosVersionToChromeCommit(self.GOOD_CROS_VERSION))

    metadata_content = '\n'.join([
        '{',
        '  "metadata-version": "2",',
        '  "toolchain-url": "2017/05/%(target)s-2017.05.25.101355.tar.xz",',
        '  "suite_scheduling": true,',
        '  "build_id": 1644146,',
        '  "version": {}',
        '}'])
    gs_mock.AddCmdResult(['cat', metadata_url], output=metadata_content)
    self.assertIsNone(
        self.bisector.CrosVersionToChromeCommit(self.GOOD_CROS_VERSION))

    gs_mock.AddCmdResult(['cat', metadata_url],
                         output=self.GOOD_METADATA_CONTENT)
    git_mock = self.StartPatcher(git_bisector_unittest.GitMock(self.repo_dir))
    git_mock.AddRunGitResult(['log', '--oneline', '-n', '2', '60.0.3112.53'],
                             returncode=128)
    self.assertIsNone(
        self.bisector.CrosVersionToChromeCommit(self.GOOD_CROS_VERSION))

  def testResolveChromeBisectRangeFromCrosVersion(self):
    """Tests ResolveChromeBisectRangeFromCrosVersion()."""
    self.SetUpBisectorWithCrosVersion()
    cros_to_chrome_mock = self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector,
        'CrosVersionToChromeCommit')
    cros_to_chrome_mock.side_effect = [self.GOOD_COMMIT_SHA1,
                                       self.BAD_COMMIT_SHA1]

    self.assertTrue(self.bisector.ResolveChromeBisectRangeFromCrosVersion())
    self.assertTrue(self.GOOD_COMMIT_SHA1, self.bisector.good_commit)
    self.assertTrue(self.BAD_COMMIT_SHA1, self.bisector.bad_commit)
    cros_to_chrome_mock.assert_has_calls([mock.call(self.GOOD_CROS_VERSION),
                                          mock.call(self.BAD_CROS_VERSION)])

    cros_to_chrome_mock.reset_mock()
    cros_to_chrome_mock.side_effect = [None]
    self.assertFalse(self.bisector.ResolveChromeBisectRangeFromCrosVersion())
    cros_to_chrome_mock.assert_called_with(self.GOOD_CROS_VERSION)

    cros_to_chrome_mock.reset_mock()
    cros_to_chrome_mock.side_effect = [self.GOOD_COMMIT_SHA1, None]
    self.assertFalse(self.bisector.ResolveChromeBisectRangeFromCrosVersion())
    cros_to_chrome_mock.assert_has_calls([mock.call(self.GOOD_CROS_VERSION),
                                          mock.call(self.BAD_CROS_VERSION)])

  def testPrepareBisect(self):
    """Tests PrepareBisect()."""
    # Pass SanityCheck().
    git_mock = self.StartPatcher(git_bisector_unittest.GitMock(self.repo_dir))
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['rev-list', self.GOOD_COMMIT_SHA1]))
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['rev-list', self.BAD_COMMIT_SHA1]))
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', self.GOOD_COMMIT_SHA1]),
        output=str(self.GOOD_COMMIT_TIMESTAMP))
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', self.BAD_COMMIT_SHA1]),
        output=str(self.BAD_COMMIT_TIMESTAMP))

    # Inject score for both side.
    git_mock.AddRunGitResult(['checkout', self.GOOD_COMMIT_SHA1])
    git_mock.AddRunGitResult(['checkout', self.BAD_COMMIT_SHA1])
    build_deploy_eval_mock = self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector, 'BuildDeployEval')
    build_deploy_eval_mock.side_effect = [self.GOOD_COMMIT_SCORE,
                                          self.BAD_COMMIT_SCORE]

    # Set auto_threshold.
    self.bisector.auto_threshold = True

    self.assertTrue(self.bisector.PrepareBisect())

  def testPrepareBisectCrosVersion(self):
    """Tests PrepareBisect() with CrOS version."""
    self.SetUpBisectorWithCrosVersion()

    self.StartPatcher(gs_unittest.GSContextMock())
    self.PatchObject(builder_module.Builder, 'SyncToHead')
    self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector,
        'ResolveChromeBisectRangeFromCrosVersion').return_value = True
    self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector,
        'SanityCheck').return_value = True
    self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector,
        'ObtainBisectBoundaryScore').return_value = True
    self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector,
        'GetThresholdFromUser').return_value = True

    self.PatchObject(
        chrome_on_cros_bisector.ChromeOnCrosBisector,
        'ExchangeChromeSanityCheck').return_value = True

    self.assertTrue(self.bisector.PrepareBisect())
