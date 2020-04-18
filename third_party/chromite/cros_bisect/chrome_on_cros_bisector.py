# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Invokes git bisect to find culprit commit inside Chromium repository."""

from __future__ import print_function

import json
import re

from chromite.cli import flash
from chromite.cros_bisect import git_bisector
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import gs
from chromite.lib import retry_util


REGEX_CROS_VERSION = re.compile(r'[Rr]?(\d+)[.-](\d+)\.(\d+)\.(\d+)$')


class ChromeOnCrosBisector(git_bisector.GitBisector):
  """Bisects offending commit in Chromium repository.

  Before bisect, it extracts metric scores for both last-know-good and bad
  commits, for verifying regression and for setting threshold telling good from
  bad commit. If last-known-good and last-known-bad CrOS version is given, it
  first checks if offending commit is in Chrome by deploying last-known-bad
  CrOS version's Chrome to DUT with last-known-good CrOS image installed. If it
  doesn't have regression, then the offending commit is not on Chrome side.

  It finds first bad commit within given good and bad commit using "git bisect"
  command. For each commit, it uses builder to build package to verify and
  deploy it to DUT (device under test). And asks evaluator to extract score
  of the commit. It is treated as good commit i the score is closer to user
  specified good commit.

  Finally, it outputs git bisect result.
  """

  def __init__(self, options, builder, evaluator):
    """Constructor.

    Args:
      options: An argparse.Namespace to hold command line arguments. Should
        contain:
        * cros_flash_retry: Max retry for "cros flash" command.
        * cros_flash_sleep: #seconds to wait between retry.
        * cros_flash_backoff: backoff factor. Must be >=1. If backoff factor
            is 1, sleep_duration = sleep * num_retry. Otherwise,
            sleep_duration = sleep * (backoff_factor) ** (num_retry - 1)
      builder: Builder to build/deploy image. Should contain repo_dir.
      evaluator: Evaluator to get score
    """
    super(ChromeOnCrosBisector, self).__init__(options, builder, evaluator)
    self.cros_flash_retry = max(0, options.cros_flash_retry)
    self.cros_flash_sleep = max(0, options.cros_flash_sleep)
    self.cros_flash_backoff = max(1, options.cros_flash_backoff)

    self.good_cros_version = None
    self.bad_cros_version = None
    self.bisect_between_cros_version = False
    if not (git.IsSHA1(options.good, full=False) and
            git.IsSHA1(options.bad, full=False)):
      # Postpone commit resolution to Run().
      self.good_commit = None
      self.bad_commit = None
      self.good_cros_version = options.good
      self.bad_cros_version = options.bad
      self.bisect_between_cros_version = True

    # Used to access gs://. Lazy initialization.
    self.gs_ctx = None

  @staticmethod
  def CheckCommitFormat(commit):
    """Checks if commit is the acceptable format.

    It accepts either SHA1 or CrOS version.

    Args:
      commit: commit string.

    Returns:
      Normalized commit. None if the format is unacceptable.
    """
    if git_bisector.GitBisector.CheckCommitFormat(commit):
      return commit

    match_obj = REGEX_CROS_VERSION.match(commit)
    if match_obj:
      return 'R%s-%s.%s.%s' % match_obj.groups()
    return None

  def ObtainBisectBoundaryScoreImpl(self, good_side):
    """The worker of obtaining score of either last-known-good or bad commit.

    Instead of deploying Chrome for good/bad commit, it deploys good/bad
    CrOS image if self.bisect_between_cros_version is set.

    Args:
      good_side: True if it evaluates score for last-known-good. False for
          last-known-bad commit.

    Returns:
      Evaluated score.
    """
    commit = self.good_commit if good_side else self.bad_commit
    commit_label = 'good' if good_side else 'bad'
    # Though bisect_between_cros_version uses archived image directly without
    # building Chrome, it is necessary because BuildDeployEval() will update
    # self.current_commit.
    self.Git(['checkout', commit])
    eval_label = None
    customize_build_deploy = None
    if self.bisect_between_cros_version:
      cros_version = (self.good_cros_version if good_side else
                      self.bad_cros_version)
      logging.notice('Obtaining score of %s CrOS version: %s', commit_label,
                     cros_version)
      eval_label = 'cros_%s' % cros_version
      customize_build_deploy = lambda: self.FlashCrosImage(
          self.GetCrosXbuddyPath(cros_version))
    else:
      logging.notice('Obtaining score of %s commit: %s', commit_label, commit)

    return self.BuildDeployEval(eval_label=eval_label,
                                customize_build_deploy=customize_build_deploy)

  def GetCrosXbuddyPath(self, version):
    """Composes xbuddy path.

    Args:
      version: CrOS version to get.

    Returns:
      xbuddy path of the CrOS image for board.
    """
    return 'xbuddy://remote/%s/%s/test' % (self.board, version)

  def ExchangeChromeSanityCheck(self):
    """Exchanges Chrome between good and bad CrOS.

    It deploys last-known-good Chrome to last-known-bad CrOS DUT and vice
    versa to see if regression culprit is in Chrome.
    """
    def FlashBuildDeploy(cros_version):
      """Flashes DUT first then builds/deploys Chrome."""
      self.FlashCrosImage(self.GetCrosXbuddyPath(cros_version))
      return self.BuildDeploy()

    def Evaluate(cros_version, chromium_commit):
      self.Git(['checkout', chromium_commit])
      score = self.BuildDeployEval(
          eval_label='cros_%s_cr_%s' % (cros_version, chromium_commit),
          customize_build_deploy=lambda: FlashBuildDeploy(cros_version))
      label = self.LabelBuild(score)
      logging.notice('Score(mean: %.3f std: %.3f). Marked as %s',
                     score.mean, score.std, label)
      return label

    logging.notice('Sanity check: exchange Chrome between good and bad CrOS '
                   'version.')
    # Expect bad result if culprit commit is inside Chrome.
    logging.notice('Obtaining score of good CrOS %s with bad Chrome %s',
                   self.good_cros_version, self.bad_commit)
    bad_chrome_on_good_cros_label = Evaluate(self.good_cros_version,
                                             self.bad_commit)
    self.current_commit.label = 'good_cros_bad_chrome'

    # Expect bad result if culprit commit is inside Chrome.
    logging.notice('Obtaining score of bad CrOS %s with good Chrome %s',
                   self.bad_cros_version, self.good_commit)
    good_chrome_on_bad_cros_label = Evaluate(self.bad_cros_version,
                                             self.good_commit)
    self.current_commit.label = 'bad_cros_good_chrome'

    if (bad_chrome_on_good_cros_label != 'bad' or
        good_chrome_on_bad_cros_label != 'good'):
      logging.error(
          'After exchanging Chrome between good/bad CrOS image, found that '
          'culprit commit should not be in Chrome repository.')
      logging.notice(
          'Bisect log:\n' +
          '\n'.join(map(self.CommitInfoToStr, self.bisect_log)))
      return False
    return True

  def FlashCrosImage(self, xbuddy_path):
    """Flashes CrOS image to DUT.

    It returns True when it successfully flashes image to DUT. Raises exception
    when it fails after retry.

    Args:
      xbuddy_path: xbuddy path to CrOS image to flash.

    Returns:
      True

    Raises:
      FlashError: An unrecoverable error occured.
    """
    logging.notice('cros flash %s', xbuddy_path)
    @retry_util.WithRetry(
        self.cros_flash_retry, log_all_retries=True,
        sleep=self.cros_flash_sleep,
        backoff_factor=self.cros_flash_backoff)
    def flash_with_retry():
      flash.Flash(self.remote, xbuddy_path, board=self.board,
                  clobber_stateful=True, disable_rootfs_verification=True)

    flash_with_retry()
    return True

  def CrosVersionToChromeCommit(self, cros_version):
    """Resolves head commit of the Chrome used by the CrOS version.

    Args:
      cros_version: ChromeOS version, e.g. R60-9531.0.0.

    Returns:
      Chrome SHA. None if the ChromeOS version is not found.
    """
    metadata_url = ('gs://chromeos-image-archive/%s-release/%s/'
                    'partial-metadata.json') % (self.board, cros_version)
    try:
      metadata_content = self.gs_ctx.Cat(metadata_url)
    except gs.GSCommandError as e:
      logging.error('Cannot load %s: %s', metadata_url, e)
      return None

    try:
      metadata = json.loads(metadata_content)
    except ValueError:
      logging.error('Unable to parse %s', metadata_url)
      return None
    if (not metadata or 'version' not in metadata or
        'chrome' not in metadata['version']):
      logging.error('metadata["version"]["chrome"] does not exist in %s',
                    metadata_url)
      return None

    chrome_version = metadata['version']['chrome']

    # Commit just before the branch point.
    # Second line, first field.
    result = self.Git(['log', '--oneline', '-n', '2', chrome_version])
    if result.returncode != 0:
      logging.error('Failed to run "git log %s": error: %s  returncode:%s',
                    chrome_version, result.error, result.returncode)
      return None

    return result.output.splitlines()[1].split()[0]

  def ResolveChromeBisectRangeFromCrosVersion(self):
    """Resolves Chrome bisect range given good and bad CrOS versions.

    It sets up self.good_commit and self.bad_commit, which are derived from
    self.good_cros_version and self.bad_cros_version, respectively.

    Returns:
      False if either good_commit or bad_commit failed to resolve. Otherwise,
      True.
    """
    self.good_commit = self.CrosVersionToChromeCommit(self.good_cros_version)
    if self.good_commit:
      logging.info('Latest Chrome commit of good CrOS version %s: %s',
                   self.good_cros_version, self.good_commit)
    else:
      logging.error('Cannot find metadata for CrOS version: %s',
                    self.good_cros_version)
      return False

    self.bad_commit = self.CrosVersionToChromeCommit(self.bad_cros_version)
    if self.bad_commit:
      logging.info('Latest Chrome commit of bad CrOS version %s: %s',
                   self.bad_cros_version, self.bad_commit)
    else:
      logging.error('Cannot find metadata for CrOS version: %s',
                    self.bad_cros_version)
      return False
    return True

  def PrepareBisect(self):
    """Performs sanity checks and obtains bisect boundary score before bisect.

    Returns:
      False if there's something wrong.
    """
    if self.bisect_between_cros_version:
      # Lazy initialization.
      self.gs_ctx = gs.GSContext()
      self.builder.SyncToHead(fetch_tags=True)
      if not self.ResolveChromeBisectRangeFromCrosVersion():
        return None

    if not (self.SanityCheck() and
            self.ObtainBisectBoundaryScore() and
            self.GetThresholdFromUser()):
      return False

    if self.bisect_between_cros_version:
      if not self.ExchangeChromeSanityCheck():
        return False
    return True
