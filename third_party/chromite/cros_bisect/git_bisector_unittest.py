# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test git_bisector module."""

from __future__ import print_function

import copy
import mock
import os

from chromite.cros_bisect import common
from chromite.cros_bisect import builder as builder_module
from chromite.cros_bisect import evaluator as evaluator_module
from chromite.cros_bisect import git_bisector
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import partial_mock
from chromite.lib import cros_logging as logging

class GitMock(partial_mock.PartialCmdMock):
  """Mocks git.RunGit.

  Usage:
    mock = GitMock('/path/to/git_repository')
    mock.AddRunGitResult(git_command, output=...)
    # call git.RunGit(...)
  """
  TARGET = 'chromite.lib.git'
  ATTRS = ('RunGit', )
  DEFAULT_ATTR = 'RunGit'

  def __init__(self, cwd):
    """Constructor.

    Args:
      cwd: default git repository.
    """
    super(GitMock, self).__init__()
    self.cwd = cwd

  def RunGit(self, *args, **kwargs):
    """Mocked RunGit."""
    try:
      return self._results['RunGit'].LookupResult(args, kwargs=kwargs)
    except cros_build_lib.RunCommandError as e:
      # Copy the logic of error_code_ok from RunCommand.
      if kwargs.get('error_code_ok'):
        return e.result
      raise e

  def AddRunGitResult(self, cmd, cwd=None, returncode=0, output='', error='',
                      kwargs=None, strict=False, side_effect=None):
    """Adds git command and results."""
    cwd = self.cwd if cwd is None else cwd
    result = self.CmdResult(returncode, output, error)
    if returncode != 0 and not side_effect:
      side_effect = cros_build_lib.RunCommandError('non-zero returncode',
                                                   result)
    self._results['RunGit'].AddResultForParams(
        (cwd, cmd,), result, kwargs=kwargs, side_effect=side_effect,
        strict=strict)


class TestGitBisector(cros_test_lib.MockTempDirTestCase):
  """Tests GitBisector class."""

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

  CULPRIT_COMMIT_SHA1 = '12345abcde'
  CULPRIT_COMMIT_TIMESTAMP = 1486530000
  CULPRIT_COMMIT_SCORE = common.Score([81])
  CULPRIT_COMMIT_INFO = common.CommitInfo(
      sha1=CULPRIT_COMMIT_SHA1, timestamp=CULPRIT_COMMIT_TIMESTAMP, title='bad',
      score=CULPRIT_COMMIT_SCORE)

  THRESHOLD_SPLITTER = 95  # Score between good and bad, closer to good side.
  THRESHOLD = 5  # Distance between good score and splitter.

  REPEAT = 3

  def setUp(self):
    """Sets up test case."""
    self.options = cros_test_lib.EasyAttr(
        base_dir=self.tempdir, board=self.BOARD, reuse_repo=True,
        good=self.GOOD_COMMIT_SHA1, bad=self.BAD_COMMIT_SHA1, remote=self.DUT,
        eval_repeat=self.REPEAT, auto_threshold=False, reuse_eval=False,
        eval_raise_on_error=False, skip_failed_commit=False)

    self.evaluator = evaluator_module.Evaluator(self.options)
    self.builder = builder_module.Builder(self.options)
    self.bisector = git_bisector.GitBisector(self.options, self.builder,
                                             self.evaluator)
    self.repo_dir = os.path.join(self.tempdir,
                                 builder_module.Builder.DEFAULT_REPO_DIR)

  def setDefaultCommitInfo(self):
    """Sets up default commit info."""
    self.bisector.good_commit_info = copy.deepcopy(self.GOOD_COMMIT_INFO)
    self.bisector.bad_commit_info = copy.deepcopy(self.BAD_COMMIT_INFO)

  def testInit(self):
    """Tests GitBisector() to expect default data members."""
    self.assertEqual(self.GOOD_COMMIT_SHA1, self.bisector.good_commit)
    self.assertEqual(self.BAD_COMMIT_SHA1, self.bisector.bad_commit)
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
    """Tests that GitBisector raises error when missing required arguments."""
    options = cros_test_lib.EasyAttr()
    with self.assertRaises(common.MissingRequiredOptionsException) as cm:
      git_bisector.GitBisector(options, self.builder, self.evaluator)
    exception_message = cm.exception.message
    self.assertTrue('Missing command line' in exception_message)
    self.assertTrue('GitBisector' in exception_message)
    for arg in git_bisector.GitBisector.REQUIRED_ARGS:
      self.assertTrue(arg in exception_message)

  def testCheckCommitFormat(self):
    """Tests CheckCommitFormat()."""
    sha1 = '900d900d'
    self.assertEqual(sha1, git_bisector.GitBisector.CheckCommitFormat(sha1))
    not_sha1 = 'bad_sha1'
    self.assertIsNone(git_bisector.GitBisector.CheckCommitFormat(not_sha1))

  def testSetUp(self):
    """Tests that SetUp() calls builder.SetUp()."""
    with mock.patch.object(builder_module.Builder, 'SetUp') as builder_mock:
      self.bisector.SetUp()
      builder_mock.assert_called_with()

  def testGit(self):
    """Tests that Git() invokes git.RunGit() as expected."""
    git_mock = self.StartPatcher(GitMock(self.repo_dir))
    git_mock.AddRunGitResult(['ls'])
    self.bisector.Git(['ls'])

  def testUpdateCurrentCommit(self):
    """Tests that UpdateCurrentCommit() updates self.current_commit."""
    git_mock = self.StartPatcher(GitMock(self.repo_dir))
    git_mock.AddRunGitResult(
        partial_mock.In('show'),
        output='4fcbdaf6 1493010050 Add "cros bisect" command.\n')
    self.bisector.UpdateCurrentCommit()
    current_commit = self.bisector.current_commit
    self.assertEqual('4fcbdaf6', current_commit.sha1)
    self.assertEqual(1493010050, current_commit.timestamp)
    self.assertEqual('Add "cros bisect" command.', current_commit.title)

  def testUpdateCurrentCommitFail(self):
    """Tests UpdateCurrentCommit() when 'git show' returns nonzero."""
    git_mock = self.StartPatcher(GitMock(self.repo_dir))
    git_mock.AddRunGitResult(partial_mock.In('show'), returncode=128)
    self.bisector.UpdateCurrentCommit()
    self.assertTrue(not self.bisector.current_commit)

  def testUpdateCurrentCommitFailUnexpectedOutput(self):
    """Tests UpdateCurrentCommit() when 'git show' gives unexpected output."""
    git_mock = self.StartPatcher(GitMock(self.repo_dir))
    git_mock.AddRunGitResult(partial_mock.In('show'), output='Not expected')
    self.bisector.UpdateCurrentCommit()
    self.assertTrue(not self.bisector.current_commit)

  def MockOutBuildDeployEvaluateForSanityCheck(self):
    """Mocks out BuildDeployEvaluate behavior for SaintyCheck test.

    It mocks UpdateCurrentCommit() to emit good and bad commits. And mocks
    CheckLastEvaluate() to return good and bad commit score.
    """
    commit_info_list = [
        common.CommitInfo(sha1=self.GOOD_COMMIT_SHA1, title='good',
                          timestamp=self.GOOD_COMMIT_TIMESTAMP),
        common.CommitInfo(sha1=self.BAD_COMMIT_SHA1, title='bad',
                          timestamp=self.BAD_COMMIT_TIMESTAMP)]
    # This mock is problematic. The side effect should modify "self" when
    # the member method is called.
    def UpdateCurrentCommitSideEffect():
      self.bisector.current_commit = commit_info_list.pop(0)
    self.PatchObject(
        git_bisector.GitBisector, 'UpdateCurrentCommit',
        side_effect=UpdateCurrentCommitSideEffect)

    self.PatchObject(
        evaluator_module.Evaluator, 'CheckLastEvaluate',
        side_effect=[self.GOOD_COMMIT_SCORE, self.BAD_COMMIT_SCORE])

  def testGetCommitTimestamp(self):
    """Tests GetCommitTimeStamp by mocking git.RunGit."""
    git_mock = self.StartPatcher(GitMock(self.repo_dir))

    # Mock git result for GetCommitTimestamp.
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', self.GOOD_COMMIT_SHA1]),
        output=str(self.GOOD_COMMIT_TIMESTAMP))

    self.assertEqual(self.GOOD_COMMIT_TIMESTAMP,
                     self.bisector.GetCommitTimestamp(self.GOOD_COMMIT_SHA1))

  def testGetCommitTimestampNotFound(self):
    """Tests GetCommitTimeStamp when the commit is not found."""
    git_mock = self.StartPatcher(GitMock(self.repo_dir))
    nonexist_sha1 = '12345'
    # Mock git result for GetCommitTimestamp.
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', nonexist_sha1]),
        output='commit does not exist')

    self.assertIsNone(self.bisector.GetCommitTimestamp(nonexist_sha1))

  def testSanityCheck(self):
    """Tests SanityCheck().

    It tests by mocking out git commands called by SanityCheck().
    """
    git_mock = self.StartPatcher(GitMock(self.repo_dir))
    # Mock git result for DoesCommitExistInRepo.
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['rev-list', self.GOOD_COMMIT_SHA1]))
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['rev-list', self.BAD_COMMIT_SHA1]))

    # Mock git result for GetCommitTimestamp.
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', self.GOOD_COMMIT_SHA1]),
        output=str(self.GOOD_COMMIT_TIMESTAMP))
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', self.BAD_COMMIT_SHA1]),
        output=str(self.BAD_COMMIT_TIMESTAMP))

    self.assertTrue(self.bisector.SanityCheck())

  def testSanityCheckGoodCommitNotExist(self):
    """Tests SanityCheck() that good commit does not exist."""
    git_mock = self.StartPatcher(GitMock(self.repo_dir))
    sync_to_head_mock = self.PatchObject(builder_module.Builder, 'SyncToHead')
    # Mock git result for DoesCommitExistInRepo to return False.
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['rev-list', self.GOOD_COMMIT_SHA1]),
        returncode=128)

    # Mock invalid result for GetCommitTimestamp to return None.
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', self.GOOD_COMMIT_SHA1]),
        output='invalid commit')
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', self.BAD_COMMIT_SHA1]),
        output='invalid commit')

    self.assertFalse(self.bisector.SanityCheck())

    # SyncToHead() called because DoesCommitExistInRepo() returns False for
    # good commit.
    sync_to_head_mock.assert_called()

  def testSanityCheckSyncToHeadWorks(self):
    """Tests SanityCheck() that good and bad commit do not exist.

    As good and bad commit do not exist, it calls SyncToHead().
    """
    git_mock = self.StartPatcher(GitMock(self.repo_dir))
    sync_to_head_mock = self.PatchObject(builder_module.Builder, 'SyncToHead')
    # Mock git result for DoesCommitExistInRepo to return False.
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['rev-list', self.GOOD_COMMIT_SHA1]),
        returncode=128)

    # Mock git result for GetCommitTimestamp.
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', self.GOOD_COMMIT_SHA1]),
        output=str(self.GOOD_COMMIT_TIMESTAMP))
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', self.BAD_COMMIT_SHA1]),
        output=str(self.BAD_COMMIT_TIMESTAMP))

    self.assertTrue(self.bisector.SanityCheck())

    # After SyncToHead, found both bad and good commit.
    sync_to_head_mock.assert_called()

  def testSanityCheckWrongTimeOrder(self):
    """Tests SanityCheck() that good and bad commit have wrong time order."""
    git_mock = self.StartPatcher(GitMock(self.repo_dir))
    # Mock git result for DoesCommitExistInRepo.
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['rev-list', self.GOOD_COMMIT_SHA1]))
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['rev-list', self.BAD_COMMIT_SHA1]))

    # Mock git result for GetCommitTimestamp, but swap timestamp.
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', self.GOOD_COMMIT_SHA1]),
        output=str(self.BAD_COMMIT_TIMESTAMP))
    git_mock.AddRunGitResult(
        partial_mock.InOrder(['show', self.BAD_COMMIT_SHA1]),
        output=str(self.GOOD_COMMIT_TIMESTAMP))

    self.assertFalse(self.bisector.SanityCheck())

  def testObtainBisectBoundaryScoreImpl(self):
    """Tests ObtainBisectBoundaryScoreImpl()."""
    git_mock = self.StartPatcher(GitMock(self.repo_dir))
    git_mock.AddRunGitResult(['checkout', self.GOOD_COMMIT_SHA1])
    git_mock.AddRunGitResult(['checkout', self.BAD_COMMIT_SHA1])

    build_deploy_eval_mock = self.PatchObject(
        git_bisector.GitBisector, 'BuildDeployEval',
        side_effect=[self.GOOD_COMMIT_SCORE, self.BAD_COMMIT_SCORE])

    self.assertEqual(self.GOOD_COMMIT_SCORE,
                     self.bisector.ObtainBisectBoundaryScoreImpl(True))
    self.assertEqual(self.BAD_COMMIT_SCORE,
                     self.bisector.ObtainBisectBoundaryScoreImpl(False))

    self.assertEqual(
        [mock.call(self.repo_dir, ['checkout', self.GOOD_COMMIT_SHA1],
                   error_code_ok=True),
         mock.call(self.repo_dir, ['checkout', self.BAD_COMMIT_SHA1],
                   error_code_ok=True)],
        git_mock.call_args_list)
    build_deploy_eval_mock.assert_called()

  def testObtainBisectBoundaryScore(self):
    """Tests ObtainBisectBoundaryScore(). Normal case."""

    def MockedObtainBisectBoundaryScoreImpl(good_side):
      if good_side:
        self.bisector.current_commit = copy.deepcopy(self.GOOD_COMMIT_INFO)
      else:
        self.bisector.current_commit = copy.deepcopy(self.BAD_COMMIT_INFO)
      return self.bisector.current_commit.score

    obtain_score_mock = self.PatchObject(
        git_bisector.GitBisector, 'ObtainBisectBoundaryScoreImpl',
        side_effect=MockedObtainBisectBoundaryScoreImpl)

    self.assertTrue(self.bisector.ObtainBisectBoundaryScore())
    self.assertEqual(self.GOOD_COMMIT_SCORE,
                     self.bisector.good_commit_info.score)
    self.assertEqual('last-known-good  ', self.bisector.good_commit_info.label)
    self.assertEqual(self.BAD_COMMIT_SCORE, self.bisector.bad_commit_info.score)
    self.assertEqual('last-known-bad   ', self.bisector.bad_commit_info.label)
    obtain_score_mock.assert_called()

  def testObtainBisectBoundaryScoreBadScoreUnavailable(self):
    """Tests ObtainBisectBoundaryScore(). Bad score unavailable."""

    def UpdateCurrentCommitSideEffect(good_side):
      if good_side:
        self.bisector.current_commit = copy.deepcopy(self.GOOD_COMMIT_INFO)
      else:
        self.bisector.current_commit = copy.deepcopy(self.BAD_COMMIT_INFO)
        self.bisector.current_commit.score = None
      return self.bisector.current_commit.score

    obtain_score_mock = self.PatchObject(
        git_bisector.GitBisector, 'ObtainBisectBoundaryScoreImpl',
        side_effect=UpdateCurrentCommitSideEffect)

    self.assertFalse(self.bisector.ObtainBisectBoundaryScore())
    self.assertEqual(self.GOOD_COMMIT_SCORE,
                     self.bisector.good_commit_info.score)
    self.assertEqual('last-known-good  ', self.bisector.good_commit_info.label)
    self.assertIsNone(self.bisector.bad_commit_info.score)
    self.assertEqual('last-known-bad   ', self.bisector.bad_commit_info.label)
    obtain_score_mock.assert_called()

  def testGetThresholdFromUser(self):
    """Tests GetThresholdFromUser()."""
    logging.notice('testGetThresholdFromUser')
    self.setDefaultCommitInfo()
    input_mock = self.PatchObject(cros_build_lib, 'GetInput',
                                  return_value=self.THRESHOLD_SPLITTER)
    self.assertTrue(self.bisector.GetThresholdFromUser())
    self.assertEqual(self.THRESHOLD, self.bisector.threshold)
    input_mock.assert_called()

  def testGetThresholdFromUserAutoPick(self):
    """Tests GetThresholdFromUser()."""
    self.setDefaultCommitInfo()
    self.bisector.auto_threshold = True

    self.assertTrue(self.bisector.GetThresholdFromUser())
    self.assertEqual(10, self.bisector.threshold)

  def testGetThresholdFromUserOutOfBoundFail(self):
    """Tests GetThresholdFromUser() with out-of-bound input."""
    self.setDefaultCommitInfo()
    input_mock = self.PatchObject(cros_build_lib, 'GetInput',
                                  side_effect=['0', '1000', '-10'])
    self.assertFalse(self.bisector.GetThresholdFromUser())
    self.assertIsNone(self.bisector.threshold)
    self.assertEqual(3, input_mock.call_count)

  def testGetThresholdFromUserRetrySuccess(self):
    """Tests GetThresholdFromUser() with retry."""
    self.setDefaultCommitInfo()
    input_mock = self.PatchObject(
        cros_build_lib, 'GetInput',
        side_effect=['not_a_number', '1000', self.THRESHOLD_SPLITTER])
    self.assertTrue(self.bisector.GetThresholdFromUser())
    self.assertEqual(self.THRESHOLD, self.bisector.threshold)
    self.assertEqual(3, input_mock.call_count)

  def testBuildDeploy(self):
    """Tests BuildDeploy()."""
    # Inject this as UpdateCurrentCommit's side effect.
    self.bisector.current_commit = copy.deepcopy(self.GOOD_COMMIT_INFO)

    build_to_deploy = '/build/to/deploy'
    build_mock = self.PatchObject(builder_module.Builder, 'Build',
                                  return_value=build_to_deploy)
    deploy_mock = self.PatchObject(builder_module.Builder, 'Deploy')

    self.assertTrue(self.bisector.BuildDeploy())

    build_label = self.GOOD_COMMIT_INFO.sha1
    build_mock.assert_called_with(build_label)
    deploy_mock.assert_called_with(self.DUT, build_to_deploy, build_label)

  def testBuildDeployBuildFail(self):
    """Tests BuildDeploy() with Build() failure."""
    # Inject this as UpdateCurrentCommit's side effect.
    self.bisector.current_commit = copy.deepcopy(self.GOOD_COMMIT_INFO)

    # Build() failed.
    build_mock = self.PatchObject(builder_module.Builder, 'Build',
                                  return_value=None)
    deploy_mock = self.PatchObject(builder_module.Builder, 'Deploy')

    self.assertFalse(self.bisector.BuildDeploy())
    build_mock.assert_called()
    deploy_mock.assert_not_called()

  def PatchObjectForBuildDeployEval(self):
    """Returns a dict of patch objects.

    The patch objects are to mock:
      git_bisector.UpdateCurrentCommit()
      evaluator.CheckLastEvaluate()
      git_bisector.BuildDeploy()
      evaluator.Evaluate()
    """
    return {
        'UpdateCurrentCommit': self.PatchObject(git_bisector.GitBisector,
                                                'UpdateCurrentCommit'),
        'CheckLastEvaluate': self.PatchObject(evaluator_module.Evaluator,
                                              'CheckLastEvaluate'),
        'BuildDeploy': self.PatchObject(git_bisector.GitBisector, 'BuildDeploy',
                                        return_value=True),
        'Evaluate': self.PatchObject(evaluator_module.Evaluator, 'Evaluate')}

  def testBuildDeployEvalShortcutCheckLastEvaluate(self):
    """Tests BuildDeployEval() with CheckLastEvaluate() found last score."""
    mocks = self.PatchObjectForBuildDeployEval()

    # Inject this as UpdateCurrentCommit's side effect.
    self.bisector.current_commit = copy.deepcopy(self.GOOD_COMMIT_INFO)

    mocks['CheckLastEvaluate'].return_value = self.GOOD_COMMIT_SCORE

    score = self.bisector.BuildDeployEval()
    self.assertEqual(self.GOOD_COMMIT_SCORE, score)
    self.assertEqual(self.GOOD_COMMIT_SCORE, self.bisector.current_commit.score)
    for method_called in ['UpdateCurrentCommit', 'CheckLastEvaluate']:
      mocks[method_called].assert_called()
    mocks['CheckLastEvaluate'].assert_called_with(self.GOOD_COMMIT_SHA1,
                                                  self.REPEAT)

    for method_called in ['BuildDeploy', 'Evaluate']:
      mocks[method_called].assert_not_called()

  def AssertBuildDeployEvalMocksAllCalled(self, mocks):
    for method_called in ['UpdateCurrentCommit', 'CheckLastEvaluate',
                          'BuildDeploy', 'Evaluate']:
      mocks[method_called].assert_called()
    mocks['CheckLastEvaluate'].assert_called_with(self.GOOD_COMMIT_SHA1,
                                                  self.REPEAT)
    mocks['Evaluate'].assert_called_with(self.DUT, self.GOOD_COMMIT_SHA1,
                                         self.REPEAT)

  def testBuildDeployEvalNoCheckLastEvaluate(self):
    """Tests BuildDeployEval() without last score."""
    mocks = self.PatchObjectForBuildDeployEval()

    # Inject this as UpdateCurrentCommit's side effect.
    self.bisector.current_commit = copy.deepcopy(self.GOOD_COMMIT_INFO)

    mocks['CheckLastEvaluate'].return_value = common.Score()
    mocks['Evaluate'].return_value = self.GOOD_COMMIT_SCORE

    self.assertEqual(self.GOOD_COMMIT_SCORE, self.bisector.BuildDeployEval())
    self.assertEqual(self.GOOD_COMMIT_SCORE, self.bisector.current_commit.score)
    self.AssertBuildDeployEvalMocksAllCalled(mocks)

  def testBuildDeployEvalBuildFail(self):
    """Tests BuildDeployEval() with BuildDeploy failure."""
    mocks = self.PatchObjectForBuildDeployEval()

    # Inject this as UpdateCurrentCommit's side effect.
    self.bisector.current_commit = copy.deepcopy(self.GOOD_COMMIT_INFO)

    mocks['CheckLastEvaluate'].return_value = common.Score()
    mocks['BuildDeploy'].return_value = False

    score = self.bisector.BuildDeployEval()
    self.assertFalse(score)
    self.assertFalse(self.bisector.current_commit.score)

    for method_called in ['UpdateCurrentCommit', 'CheckLastEvaluate',
                          'BuildDeploy']:
      mocks[method_called].assert_called()
    mocks['CheckLastEvaluate'].assert_called_with(self.GOOD_COMMIT_SHA1,
                                                  self.REPEAT)

    mocks['Evaluate'].assert_not_called()

  def testBuildDeployEvalNoCheckLastEvaluateSpecifyEvalLabel(self):
    """Tests BuildDeployEval() with eval_label specified."""
    mocks = self.PatchObjectForBuildDeployEval()

    # Inject this as UpdateCurrentCommit's side effect.
    self.bisector.current_commit = copy.deepcopy(self.GOOD_COMMIT_INFO)

    mocks['CheckLastEvaluate'].return_value = common.Score()
    mocks['Evaluate'].return_value = self.GOOD_COMMIT_SCORE

    eval_label = 'customized_label'
    self.assertEqual(self.GOOD_COMMIT_SCORE,
                     self.bisector.BuildDeployEval(eval_label=eval_label))
    self.assertEqual(self.GOOD_COMMIT_SCORE, self.bisector.current_commit.score)

    for method_called in ['UpdateCurrentCommit', 'CheckLastEvaluate',
                          'BuildDeploy', 'Evaluate']:
      mocks[method_called].assert_called()
    # Use given label instead of SHA1 as eval label.
    mocks['CheckLastEvaluate'].assert_called_with(eval_label, self.REPEAT)
    mocks['Evaluate'].assert_called_with(self.DUT, eval_label, self.REPEAT)

  @staticmethod
  def _DummyMethod():
    """A dummy method for test to call and mock."""
    pass

  def testBuildDeployEvalNoCheckLastEvaluateSpecifyBuildDeploy(self):
    """Tests BuildDeployEval() with customize_build_deploy specified."""
    mocks = self.PatchObjectForBuildDeployEval()

    # Inject this as UpdateCurrentCommit's side effect.
    self.bisector.current_commit = copy.deepcopy(self.GOOD_COMMIT_INFO)

    mocks['CheckLastEvaluate'].return_value = common.Score()
    mocks['Evaluate'].return_value = self.GOOD_COMMIT_SCORE

    dummy_method = self.PatchObject(
        TestGitBisector, '_DummyMethod', return_value=True)

    eval_label = 'customized_label'
    self.assertEqual(self.GOOD_COMMIT_SCORE,
                     self.bisector.BuildDeployEval(
                         eval_label=eval_label,
                         customize_build_deploy=TestGitBisector._DummyMethod))
    self.assertEqual(self.GOOD_COMMIT_SCORE, self.bisector.current_commit.score)

    for method_called in ['UpdateCurrentCommit', 'CheckLastEvaluate',
                          'Evaluate']:
      mocks[method_called].assert_called()
    mocks['BuildDeploy'].assert_not_called()
    dummy_method.assert_called()
    # Use given label instead of SHA1 as eval label.
    mocks['CheckLastEvaluate'].assert_called_with(eval_label, self.REPEAT)
    mocks['Evaluate'].assert_called_with(self.DUT, eval_label, self.REPEAT)

  def testBuildDeployEvalRaiseNoScore(self):
    """Tests BuildDeployEval() without score with eval_raise_on_error set."""
    self.options.eval_raise_on_error = True
    self.bisector = git_bisector.GitBisector(self.options, self.builder,
                                             self.evaluator)

    mocks = self.PatchObjectForBuildDeployEval()

    # Inject this as UpdateCurrentCommit's side effect.
    self.bisector.current_commit = copy.deepcopy(self.GOOD_COMMIT_INFO)

    mocks['CheckLastEvaluate'].return_value = common.Score()
    mocks['Evaluate'].return_value = common.Score()

    self.assertRaises(Exception, self.bisector.BuildDeployEval)
    self.assertFalse(self.bisector.current_commit.score)
    self.AssertBuildDeployEvalMocksAllCalled(mocks)

  def testBuildDeployEvalSuppressRaiseNoScore(self):
    """Tests BuildDeployEval() without score with eval_raise_on_error unset."""
    mocks = self.PatchObjectForBuildDeployEval()

    # Inject this as UpdateCurrentCommit's side effect.
    self.bisector.current_commit = copy.deepcopy(self.GOOD_COMMIT_INFO)

    mocks['CheckLastEvaluate'].return_value = common.Score()
    mocks['Evaluate'].return_value = common.Score()

    self.assertFalse(self.bisector.BuildDeployEval())
    self.assertFalse(self.bisector.current_commit.score)
    self.AssertBuildDeployEvalMocksAllCalled(mocks)

  def testLabelBuild(self):
    """Tests LabelBuild()."""
    # Inject good(100), bad(80) score and threshold.
    self.setDefaultCommitInfo()
    self.bisector.threshold = self.THRESHOLD
    good = 'good'
    bad = 'bad'

    # Worse than given bad score.
    self.assertEqual(bad, self.bisector.LabelBuild(common.Score([70])))

    # Better than bad score, but not good enough.
    self.assertEqual(bad, self.bisector.LabelBuild(common.Score([85])))
    self.assertEqual(bad, self.bisector.LabelBuild(common.Score([90])))

    # On the margin.
    self.assertEqual(good, self.bisector.LabelBuild(common.Score([95])))

    # Better than the margin.
    self.assertEqual(good, self.bisector.LabelBuild(common.Score([98])))

    # Better than given good score.
    self.assertEqual(good, self.bisector.LabelBuild(common.Score([110])))

    # No score, default bad.
    self.assertEqual(bad, self.bisector.LabelBuild(None))
    self.assertEqual(bad, self.bisector.LabelBuild(common.Score()))

  def testLabelBuildSkipNoScore(self):
    """Tests LabelBuild()."""
    self.options.skip_failed_commit = True
    self.bisector = git_bisector.GitBisector(self.options, self.builder,
                                             self.evaluator)

    # Inject good(100), bad(80) score and threshold.
    self.setDefaultCommitInfo()
    self.bisector.threshold = self.THRESHOLD

    # No score, skip.
    self.assertEqual('skip', self.bisector.LabelBuild(None))
    self.assertEqual('skip', self.bisector.LabelBuild(common.Score()))


  def testLabelBuildLowerIsBetter(self):
    """Tests LabelBuild() in lower-is-better condition."""
    # Reverse good(80) and bad(100) score (lower is better), same threshold.
    self.bisector.good_commit_info = copy.deepcopy(self.BAD_COMMIT_INFO)
    self.bisector.bad_commit_info = copy.deepcopy(self.GOOD_COMMIT_INFO)
    self.bisector.threshold = self.THRESHOLD
    good = 'good'
    bad = 'bad'

    # Better than given good score.
    self.assertEqual(good, self.bisector.LabelBuild(common.Score([70])))

    # Worse than good score, but still better than  margin.
    self.assertEqual(good, self.bisector.LabelBuild(common.Score([80])))
    self.assertEqual(good, self.bisector.LabelBuild(common.Score([82])))

    # On the margin.
    self.assertEqual(good, self.bisector.LabelBuild(common.Score([85])))

    # Worse than the margin.
    self.assertEqual(bad, self.bisector.LabelBuild(common.Score([88])))
    self.assertEqual(bad, self.bisector.LabelBuild(common.Score([90])))
    self.assertEqual(bad, self.bisector.LabelBuild(common.Score([95])))

    # Worse than given bad score.
    self.assertEqual(bad, self.bisector.LabelBuild(common.Score([110])))

  def testGitBisect(self):
    """Tests GitBisect()."""
    git_mock = self.PatchObject(
        git_bisector.GitBisector, 'Git',
        return_value=cros_build_lib.CommandResult(
            cmd=['git', 'bisect', 'reset'], output='We are not bisecting.',
            returncode=0))

    result, done = self.bisector.GitBisect(['reset'])
    git_mock.assert_called_with(['bisect', 'reset'])
    self.assertFalse(done)
    self.assertEqual('We are not bisecting.', result.output)
    self.assertListEqual(['git', 'bisect', 'reset'], result.cmd)

  def testGitBisectDone(self):
    """Tests GitBisect() when culprit is found."""
    git_mock = self.PatchObject(
        git_bisector.GitBisector, 'Git',
        return_value=cros_build_lib.CommandResult(
            cmd=['git', 'bisect', 'bad'],
            output='abcedf is the first bad commit\ncommit abcdef',
            returncode=0))

    result, done = self.bisector.GitBisect(['bad'])
    git_mock.assert_called_with(['bisect', 'bad'])
    self.assertTrue(done)
    self.assertListEqual(['git', 'bisect', 'bad'], result.cmd)

  def testRun(self):
    """Tests Run()."""
    bisector_mock = self.StartPatcher(GitBisectorMock())
    bisector_mock.good_commit_info = copy.deepcopy(self.GOOD_COMMIT_INFO)
    bisector_mock.bad_commit_info = copy.deepcopy(self.BAD_COMMIT_INFO)
    bisector_mock.threshold = self.THRESHOLD
    bisector_mock.git_bisect_args_result = [
        (['reset'], (None, False)),
        (['start'], (None, False)),
        (['bad', self.BAD_COMMIT_SHA1], (None, False)),
        (['good', self.GOOD_COMMIT_SHA1], (None, False)),
        (['bad'],
         (cros_build_lib.CommandResult(
             cmd=['git', 'bisect', 'bad'],
             output='%s is the first bad commit\ncommit %s' % (
                 self.CULPRIT_COMMIT_SHA1, self.CULPRIT_COMMIT_SHA1),
             returncode=0),
          True)),         # bisect bad (assume it found the first bad commit).
        (['log'], (None, False)),
        (['reset'], (None, False))]
    bisector_mock.build_deploy_eval_current_commit = [self.CULPRIT_COMMIT_INFO]
    bisector_mock.build_deploy_eval_result = [self.CULPRIT_COMMIT_SCORE]
    bisector_mock.label_build_result = ['bad']

    run_result = self.bisector.Run()

    self.assertTrue(bisector_mock.patched['PrepareBisect'].called)
    self.assertEqual(7, bisector_mock.patched['GitBisect'].call_count)
    self.assertTrue(bisector_mock.patched['BuildDeployEval'].called)
    self.assertTrue(bisector_mock.patched['LabelBuild'].called)
    self.assertEqual(self.CULPRIT_COMMIT_SHA1, run_result)


class GitBisectorMock(partial_mock.PartialMock):
  """Partial mock GitBisector to test GitBisector.Run()."""

  TARGET = 'chromite.cros_bisect.git_bisector.GitBisector'
  ATTRS = ('PrepareBisect', 'GitBisect', 'BuildDeployEval', 'LabelBuild')

  def __init__(self):
    super(GitBisectorMock, self).__init__()
    self.good_commit_info = None
    self.bad_commit_info = None
    self.threshold = None
    self.git_bisect_args_result = []
    self.build_deploy_eval_current_commit = []
    self.build_deploy_eval_result = []
    self.label_build_result = None

  def PrepareBisect(self, this):
    this.good_commit_info = self.good_commit_info
    this.bad_commit_info = self.bad_commit_info
    this.threshold = self.threshold
    return True

  def GitBisect(self, _, bisect_op):
    (expected_op, result) = self.git_bisect_args_result.pop(0)
    assert cmp(expected_op, bisect_op) == 0
    return result

  def BuildDeployEval(self, this):
    this.current_commit = self.build_deploy_eval_current_commit.pop(0)
    return self.build_deploy_eval_result.pop(0)

  def LabelBuild(self, _, unused_score):
    return self.label_build_result.pop(0)
