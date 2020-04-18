# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Bisect culprit commit that causes regression."""

from __future__ import print_function

import argparse

from chromite.cros_bisect import autotest_evaluator
from chromite.cros_bisect import chrome_on_cros_bisector
from chromite.cros_bisect import manual_evaluator
from chromite.cros_bisect import simple_chrome_builder
from chromite.cli import command
from chromite.lib import commandline
from chromite.lib import cros_logging as logging
from chromite.lib import remote_access


def GoodBadCommitType(value):
  """Checks if the input is legal last known good/bad string.

  Currently, the legal formats are:
  1. SHA1 (for Chromium commit)
  2. CrOS version, e.g. R60-9531.0.0 or 60.9531.0.0

  Args:
    value: string to represent last known good/bad commit/version.

  Returns:
    Normalized commit/version value.

  Raises:
    argparse.ArgumentTypeError if input is illegal.
  """
  normalized_value = (
      chrome_on_cros_bisector.ChromeOnCrosBisector.CheckCommitFormat(value))
  if not normalized_value:
    raise argparse.ArgumentTypeError(
        '%s is neither commit SHA1 or CrOS version.' % value)
  return normalized_value


@command.CommandDecorator('bisect')
class BisectCommand(command.CliCommand):
  """Bisect commits to find the culprit responsible for regression."""

  EPILOG = """
Bisects commits of the given repository within given range, builds Chromium for
CrOS, deploys to DUT (device under test) and run the verifier to decide if the
commit is good or bad to find the culprit commit. Right now, supported
repository is Chromium; supported verifier is Autotest.
Note that before starting "cros bisect" please install an OS image on the DUT
that is close to the failure point, in particular from the corresponding branch.
"""

  REPO = ('chromium', )
  BUILDER = {'chromium': simple_chrome_builder.SimpleChromeBuilder}
  EVALUATOR = {'autotest': autotest_evaluator.AutotestEvaluator,
               'manual': manual_evaluator.ManualEvaluator}

  @classmethod
  def AddParser(cls, parser):
    super(BisectCommand, cls).AddParser(parser)
    parser.add_argument(
        '-G', '--good', metavar='good_commit', required=True,
        type=GoodBadCommitType,
        help='A good revision (commit SHA) or CrOS image (e.g. R60-9531.0.0) '
             'to start bisection. Should be earlier than the bad revision.')
    parser.add_argument(
        '-B', '--bad', metavar='bad_commit', required=True,
        type=GoodBadCommitType,
        help='A bad revision (commit SHA) or CrOS image (e.g. R60-9532.0.0) '
             'to start bisection. Should be later than the good revision.')
    parser.add_argument(
        '-r', '--remote', metavar='IP address / hostname', required=True,
        type=commandline.DeviceParser(commandline.DEVICE_SCHEME_SSH),
        help='The IP address/hostname of remote device which is going to test.')
    parser.add_argument(
        '-b', '--board', metavar='board_name',
        help='The board name of the ChromeOS device under test. Obtained from '
             'DUT if not assigned.')
    parser.add_argument(
        '--repo', default='chromium', choices=cls.REPO,
        help='Repository to bisect. Now it supports chormium git repository. '
             'Later it will support catapult git repository and even ChromeOS '
             'repositories.')
    parser.add_argument(
        '--evaluator', default='autotest', choices=cls.EVALUATOR.keys(),
        help='Evaluator used to determine if a commit is good or bad. Now it '
             'supports autotest. Later it will support telemetry.')
    parser.add_argument(
        '-d', '--base-dir', required=True, type='path',
        help='Base directory to store repo and results. Existing checkout can '
             'be used, see --reuse-repo.')
    parser.add_argument(
        '--chromium-dir', type='path',
        help='If specified, use it as chromium repository. Otherwise, use '
             '"[base-dir]/chromium".')
    parser.add_argument(
        '--cros-dir', type='path',
        help='If specified, use it to enter CrOS chroot environment. '
             'Otherwise, use "[base-dir]/cros".')
    parser.add_argument(
        '--build-dir', type='path',
        help='If specified, use it to store build results. Otherwise, use '
             '"[base-dir]/build".')
    parser.add_argument(
        '--reuse-repo', action='store_true',
        help='If set, reuse repository if it exists.')
    parser.add_argument(
        '--reuse-build', action='store_true',
        help='If set, reuse build if available.')
    parser.add_argument(
        '--reuse-eval', action='store_true',
        help='If set, reuse evaluation result if available.')
    parser.add_argument(
        '--reuse', action='store_true',
        help='If set, set reuse-repo, reuse-build, reuse-eval')
    parser.add_argument(
        '--no-archive-build', dest='archive_build', default=True,
        action='store_false',
        help='If set, do not archive the build.')
    parser.add_argument(
        '--auto-threshold', action='store_true',
        help='If set, set threshold in the middle between good and bad '
             'score instead of prompting user to set it.')
    parser.add_argument(
        '--test-name', help='Test name to run against')
    parser.add_argument(
        '--metric',
        help='Metric of test result to look at. For autotest, metric name is '
             'hierarchical, divided by "/". E.g., "avg_fps_1000_fishes/summary/'
             'value" for graphics_WebGLAquarium, or "charts/Total/Score/value" '
             'for telemetry_Benchmarks.octane autotest.')
    parser.add_argument(
        '--metric-take-average', action='store_true',
        help='If set, treat metric value as a list of numbers and calculate '
             'arithmetic average of them.')
    parser.add_argument(
        '--eval-repeat', type=int, default=3,
        help='Repeat evaluate commit for N times to calculate mean and '
             'standard deviation. Default 3.')
    parser.add_argument(
        '--cros-flash-retry', type=int, default=3,
        help='Max #retry for "cros flash" command. Default 3.')
    parser.add_argument(
        '--cros-flash-sleep', type=int, default=60,
        help='Wait #seconds before retry. See cros-flash-backoff for detail.')
    parser.add_argument(
        '--cros-flash-backoff', type=float, default=1,
        help='Backoff factor for sleep between "cros flash" retry. If backoff '
             'factor is 1, sleep_duration = sleep * num_retry. Otherwise, '
             'sleep_duration = sleep * (backoff_factor) ** (num_retry - 1)')
    parser.add_argument(
        '--eval-passing-only', action='store_true',
        help='If set, use existing perf result only if test was passing.')
    parser.add_argument(
        '--eval-raise-on-error', action='store_true',
        help='If set, stop bisect if it fails to evaluate a commit. '
             'Otherwise, the failed commit is labeled as bad.')
    parser.add_argument(
        '--skip-failed-commit', action='store_true',
        help='If set, skip the failed commit (build failed / no result) rather '
             'than marking it as bad commit.')

  def ProcessOptions(self):
    """Process self.options.

    It sets reuse_repo, reuse_build and reuse_eval if self.options.reuse is set.
    It also resolves self.options.board from DUT if it is not assigned.
    """
    if self.options.reuse:
      self.options.reuse_repo = True
      self.options.reuse_build = True
      self.options.reuse_eval = True
    if not self.options.board:
      dut = remote_access.ChromiumOSDevice(self.options.remote.hostname)
      self.options.board = dut.board
      if not self.options.board:
        raise Exception('Unable to obtain board name from DUT.')
      else:
        logging.info('Obtained board name "%s" from DUT.', self.options.board)

  def Run(self):
    self.ProcessOptions()
    # Note that for the objects consuming command line options, please evaluate
    # its SanityCheckOptions() in testAddParser in cros_bisect_unittest.
    builder = self.BUILDER[self.options.repo](self.options)
    evaluator = self.EVALUATOR[self.options.evaluator](self.options)
    bisector = chrome_on_cros_bisector.ChromeOnCrosBisector(
        self.options, builder, evaluator)
    bisector.SetUp()
    bisector.Run()
