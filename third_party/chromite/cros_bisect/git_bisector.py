# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Invokes git bisect to find culprit commit."""

from __future__ import print_function

import datetime

from chromite.cros_bisect import common
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git


class GitBisectorException(Exception):
  """Exception raised for GitBisector error."""
  pass


class GitBisector(common.OptionsChecker):
  """Bisects suspicious commits in a git repository.

  Before bisect, it does sanity check for the given good and bad commit. Then
  it extracts metric scores for both commits as baseline to determine the
  upcoming bisect candidate's status.

  It finds first bad commit within given good and bad commit using "git bisect"
  command. For each commit, it uses builder to build package to verify and
  deploy it to DUT (device under test). And asks evaluator to extract score
  of the commit. It is treated as good commit i the score is closer to user
  specified good commit.

  Finally, it outputs git bisect result.
  """

  REQUIRED_ARGS = ('good', 'bad', 'remote', 'eval_repeat', 'auto_threshold',
                   'board', 'eval_raise_on_error', 'skip_failed_commit')

  def __init__(self, options, builder, evaluator):
    """Constructor.

    Args:
      options: An argparse.Namespace to hold command line arguments. Should
        contain:
        * good: Last known good commit.
        * bad: Last known bad commit.
        * remote: DUT (refer lib.commandline.Device).
        * eval_repeat: Run test for N times. None means 1.
        * auto_threshold: True to set threshold automatically without prompt.
        * board: board name of the DUT.
        * eval_raise_on_error: If set, raises if evaluation failed.
        * skip_failed_commit: If set, mark the failed commit (build failed /
            no result) as 'skip' instead of 'bad'.
      builder: Builder to build/deploy image. Should contain repo_dir.
      evaluator: Evaluator to get score
    """
    super(GitBisector, self).__init__(options)
    self.good_commit = options.good
    self.bad_commit = options.bad
    self.remote = options.remote
    self.eval_repeat = options.eval_repeat
    self.builder = builder
    self.evaluator = evaluator
    self.repo_dir = self.builder.repo_dir
    self.auto_threshold = options.auto_threshold
    self.board = options.board
    self.eval_raise_on_error = options.eval_raise_on_error
    self.skip_failed_commit = options.skip_failed_commit

    # Initialized in ObtainBisectBoundaryScore().
    self.good_commit_info = None
    self.bad_commit_info = None

    # Record bisect log (list of CommitInfo).
    self.bisect_log = []

    # If the distance between current commit's score and given good commit is
    # shorter than threshold, treat the commit as good one. Its default value
    # comes from evaluator's THRESHOLD constant. If it is None (undefined),
    # the bisector will prompt user to input threshold after obtaining both
    # sides' score.
    self.threshold = evaluator.THRESHOLD

    # Set auto_threshold to True if the threshold is preset.
    if self.threshold is not None:
      self.auto_threshold = True

    # Init with empty CommitInfo. Will update in UpdateCurrentCommit().
    self.current_commit = common.CommitInfo()

  def SetUp(self):
    """Sets up environment to bisect."""
    logging.info('Set up builder environment.')
    self.builder.SetUp()

  @staticmethod
  def CheckCommitFormat(commit):
    """Checks if commit is the acceptable format.

    For git_bisector, commit should be SHA1. Child class may override it.

    Args:
      commit: commit string.

    Returns:
      Normalized commit. None if the format is unacceptable.
    """
    if git.IsSHA1(commit, full=False):
      return commit
    return None

  def Git(self, command):
    """Wrapper of git.RunGit.

    It passes self.repo_dir as git_repo to git.RunGit. Also, it
    sets error_code_ok=True so when git encounters an error, instead of
    raising RunCommandError, it returns CommandResult with non-zero return code.

    Args:
      command: git command.

    Returns:
      A CommandResult object.
    """
    return git.RunGit(self.repo_dir, command, error_code_ok=True)

  def UpdateCurrentCommit(self):
    """Updates self.current_commit."""
    result = self.Git(['show', '--no-patch', '--format=%h %ct %s'])
    if result.returncode != 0:
      logging.error('Failed to update current commit info.')
      return

    fields = result.output.strip().split()
    if len(fields) < 3:
      logging.error('Failed to update current commit info.')
      return
    sha1 = fields[0]
    timestamp = int(fields[1])
    title = ' '.join(fields[2:])
    self.current_commit = common.CommitInfo(sha1=sha1, title=title,
                                            timestamp=timestamp)

  def GetCommitTimestamp(self, sha1):
    """Obtains given commit's timestamp.

    Args:
      sha1: Commit's SHA1 to look up.

    Returns:
      timestamp in integer. None if the commit does not exist.
    """
    result = self.Git(['show', '--no-patch', '--format=%ct', sha1])
    if result.returncode == 0:
      try:
        return int(result.output.strip())
      except ValueError:
        pass
    return None

  def SanityCheck(self):
    """Checks if both last-known-good and bad commits exists.

    Also, last-known-good commit should be earlier than bad one.

    Returns:
      True if both last-known-good and bad commits exist.
    """
    logging.info('Perform sanity check on good commit %s and bad commit %s',
                 self.good_commit, self.bad_commit)
    # Builder is lazy syncing to HEAD. Sync to HEAD if good_commit or bad_commit
    # does not exist in repo.
    if (not git.DoesCommitExistInRepo(self.repo_dir, self.good_commit) or
        not git.DoesCommitExistInRepo(self.repo_dir, self.bad_commit)):
      logging.info(
          'Either good commit (%s) or bad commit (%s) not found in repo. '
          'Try syncing to HEAD first.', self.good_commit, self.bad_commit)
      self.builder.SyncToHead()
    else:
      logging.info(
          'Both good commit (%s) and bad commit (%s) are found in repo. '
          'No need to update repo.', self.good_commit, self.bad_commit)

    good_commit_timestamp = self.GetCommitTimestamp(self.good_commit)
    if not good_commit_timestamp:
      logging.error('Sanity check failed: good commit %s does not exist.',
                    self.good_commit)
      return False
    bad_commit_timestamp = self.GetCommitTimestamp(self.bad_commit)
    if not bad_commit_timestamp:
      logging.error('Sanity check failed: bad commit %s does not exist.',
                    self.bad_commit)
      return False

    # TODO(deanliao): Consider the case that we want to find a commit that
    #     fixed a problem.
    if bad_commit_timestamp < good_commit_timestamp:
      logging.error(
          'Sanity check failed: good commit (%s) timestamp: %s should be '
          'earlier than bad commit (%s): %s',
          self.good_commit, good_commit_timestamp,
          self.bad_commit, bad_commit_timestamp)
      return False
    return True

  def ObtainBisectBoundaryScoreImpl(self, good_side):
    """The worker of obtaining score of either last-known-good or bad commit.

    It is used to be overridden by derived class without re-implement
    ObtainBisectBoundaryScore().

    Args:
      good_side: True if it evaluates score for last-known-good. False for
          last-known-bad commit.

    Returns:
      Evaluated score.
    """
    commit = self.good_commit if good_side else self.bad_commit
    commit_label = 'good' if good_side else 'bad'
    logging.notice('Obtaining score of %s commit: %s', commit_label, commit)
    self.Git(['checkout', commit])
    return self.BuildDeployEval()

  def ObtainBisectBoundaryScore(self):
    """Evaluates last known good and bad commit.

    In order to give user a reference when setting threshold, evaluates
    designated benchmark on both last-known-good and last-known-bad commits.
    It builds, deploys and evaluates both good and bad images on the DUT.
    """
    good_score = self.ObtainBisectBoundaryScoreImpl(True)
    self.good_commit_info = self.current_commit
    self.good_commit_info.label = 'last-known-good  '
    if good_score is None:
      logging.error('Unable to obtain last-known-good score.')
      return False

    bad_score = self.ObtainBisectBoundaryScoreImpl(False)
    self.bad_commit_info = self.current_commit
    self.bad_commit_info.label = 'last-known-bad   '
    if bad_score is None:
      logging.error('Unable to obtain last-known-bad score.')
      return False

    if good_score == bad_score:
      logging.error(
          'Last-known-good %s should be different from last-known-bad %s',
          good_score, bad_score)
      return False

    return True

  def GetThresholdFromUser(self):
    """Gets threshold that decides good/bad commit.

    It gives user the measured last known good and bad score's statistics to
    help user determine a reasonable threshold.

    If self.auto_threshold is set, instead of prompting user to pick threshold,
    it uses half of distance between good and bad commit score as threshold.
    """
    # If threshold is assigned, skip it.
    if self.threshold is not None:
      return True

    good_score = self.good_commit_info.score
    bad_score = self.bad_commit_info.score
    logging.notice('Good commit score mean: %.3f  STD: %.3f\n'
                   'Bad commit score mean: %.3f  STD: %.3f',
                   good_score.mean, good_score.std,
                   bad_score.mean, bad_score.std)
    if self.auto_threshold:
      splitter = (good_score.mean + bad_score.mean) / 2.0
      self.threshold = abs(good_score.mean - splitter)
      logging.notice('Automatically pick threshold: %.3f', self.threshold)
      return True

    ref_score_min = min(good_score.mean, bad_score.mean)
    ref_score_max = max(good_score.mean, bad_score.mean)
    success = False
    retry = 3
    for _ in range(retry):
      try:
        splitter = float(cros_build_lib.GetInput(
            'Please give a threshold that tell apart good and bad commit '
            '(within range [%.3f, %.3f]: ' % (ref_score_min, ref_score_max)))
      except ValueError:
        logging.error('Threshold should be a floating number.')
        continue

      if ref_score_min <= splitter <= ref_score_max:
        self.threshold = abs(good_score.mean - splitter)
        success = True
        break
      else:
        logging.error('Threshold should be within range [%.3f, %.3f]',
                      ref_score_min, ref_score_max)
    else:
      logging.error('Wrong threshold input for %d times. Terminate.', retry)

    return success

  def BuildDeploy(self):
    """Builds with current commit and deploys to DUT.

    It is the default behavior of BuildDeployEval().

    Returns:
      True if the builder successfully builds and deploys to DUT.
      False otherwise.
    """
    # TODO(deanliao): Handle build/deploy fail case.
    build_label = self.current_commit.sha1
    build_to_deploy = self.builder.Build(build_label)
    if not build_to_deploy:
      return False
    return self.builder.Deploy(self.remote, build_to_deploy, build_label)

  def BuildDeployEval(self, eval_label=None, customize_build_deploy=None):
    """Builds the image, deploys to DUT and evaluates performance.

    Args:
      build_deploy: If set, builds current commit and deploys it to DUT.
      eval_label: Label for the evaluation. Default: current commit SHA1.
      customize_build_deploy: Method object if specified, call it instead of
          default self.BuildDeploy().

    Returns:
      Evaluation result.

    Raises:
      GitBisectorException if self.eval_raise_on_error and score is empty.
    """
    self.UpdateCurrentCommit()
    if not eval_label:
      eval_label = self.current_commit.sha1

    score = self.evaluator.CheckLastEvaluate(eval_label, self.eval_repeat)
    if len(score) > 0:
      logging.info('Found last evaluated result for %s: %s', eval_label,
                   score)
      self.current_commit.score = score
    else:
      if customize_build_deploy:
        build_deploy_result = customize_build_deploy()
      else:
        build_deploy_result = self.BuildDeploy()
      if build_deploy_result:
        self.current_commit.score = self.evaluator.Evaluate(
            self.remote, eval_label, self.eval_repeat)
      else:
        logging.error('Builder fails to build/deploy for %s', eval_label)
        self.current_commit.score = common.Score()

    self.bisect_log.append(self.current_commit)
    if not self.current_commit.score and self.eval_raise_on_error:
      raise GitBisectorException('Unable to obtain evaluation score')
    return self.current_commit.score

  def LabelBuild(self, score):
    """Determines if a build is good.

    Args:
      score: evaluation score of the build.

    Returns:
      'good' if the score is closer to given good one; otherwise (including
      score is None), 'bad'.
      Note that if self.skip_failed_commit is set, returns 'skip' for empty
      score.
    """
    label = 'bad'
    if not score:
      if self.skip_failed_commit:
        label = 'skip'
      logging.error('No score. Marked as %s', label)
      return label

    good_score = self.good_commit_info.score
    bad_score = self.bad_commit_info.score
    ref_score_min = min(good_score.mean, bad_score.mean)
    ref_score_max = max(good_score.mean, bad_score.mean)
    if ref_score_min < score.mean < ref_score_max:
      # Within (good_score, bad_score)
      if abs(good_score.mean - score.mean) <= self.threshold:
        label = 'good'
    else:
      dist_good = abs(good_score.mean - score.mean)
      dist_bad = abs(bad_score.mean - score.mean)
      if dist_good < dist_bad:
        label = 'good'
    logging.info('Score %.3f marked as %s', score.mean, label)
    return label

  def GitBisect(self, bisect_op):
    """Calls git bisect and logs result.

    Args:
      bisect_op: git bisect subcommand list.

    Returns:
      A CommandResult object.
      done: True if bisect ends.
    """
    command = ['bisect'] + bisect_op
    command_str = cros_build_lib.CmdToStr(['git'] + command)
    result = self.Git(command)
    done = False
    log_msg = [command_str]
    if result.output:
      log_msg.append('(output): %s' % result.output.strip())
    if result.error:
      log_msg.append('(error): %s' % result.error.strip())
    if result.returncode != 0:
      log_msg.append('(returncode): %d' % result.returncode)

    log_msg = '\n'.join(log_msg)
    if result.error or result.returncode != 0:
      logging.error(log_msg)
    else:
      logging.info(log_msg)

    if result.output:
      if 'is the first bad commit' in result.output.splitlines()[0]:
        done = True
    return result, done

  @staticmethod
  def CommitInfoToStr(info):
    """Converts CommitInfo to string.

    Args:
      info: A CommitInfo object.

    Returns:
      Stringfied CommitInfo.
    """
    timestamp = datetime.datetime.fromtimestamp(info.timestamp).isoformat(' ')
    score = 'Score(mean: %.3f std: %.3f)' % (info.score.mean, info.score.std)
    return ' '.join([info.label, timestamp, score, info.sha1, info.title])

  def PrepareBisect(self):
    """Performs sanity checks and obtains bisect boundary score before bisect.

    Returns:
      False if there's something wrong.
    """
    return (self.SanityCheck() and
            self.ObtainBisectBoundaryScore() and
            self.GetThresholdFromUser())

  def Run(self):
    """Bisects culprit commit.

    Returns:
      Culprit commit's SHA1. None if culprit not found.
    """
    if not self.PrepareBisect():
      return None

    self.GitBisect(['reset'])
    bisect_done = False
    culprit_commit = None
    nth_bisect = 0
    try:
      with cros_build_lib.TimedSection() as timer:
        self.GitBisect(['start'])
        self.GitBisect(['bad', self.bad_commit])
        self.GitBisect(['good', self.good_commit])
        while not bisect_done:
          metric_score = self.BuildDeployEval()
          bisect_op = self.LabelBuild(metric_score)
          nth_bisect += 1
          self.current_commit.label = '%2d-th-bisect-%-4s' % (nth_bisect,
                                                              bisect_op)
          logging.notice('Mark %s as %s. Score: %s', self.current_commit.sha1,
                         bisect_op, metric_score)
          bisect_result, bisect_done = self.GitBisect([bisect_op])
          if bisect_done:
            culprit_commit = bisect_result.output.splitlines()[1:]
      if bisect_done:
        logging.info('git bisect done. Elapsed time: %s', timer.delta)
    finally:
      self.GitBisect(['log'])
      self.GitBisect(['reset'])

    # Emit bisect report.
    self.bisect_log.sort(key=lambda x: x.timestamp)
    logging.notice(
        'Bisect log (sort by time):\n' +
        '\n'.join(map(self.CommitInfoToStr, self.bisect_log)))

    if culprit_commit:
      logging.notice('\n'.join(['Culprit commit:'] + culprit_commit))
      # First line: commit <SHA>
      return culprit_commit[0].split()[1]
    else:
      logging.notice('Culprit not found.')
      return None
