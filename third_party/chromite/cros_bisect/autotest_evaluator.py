# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs autotest on DUT and gets result for performance evaluation."""

from __future__ import print_function

import os
import shutil

from chromite.cros_bisect import common
from chromite.cros_bisect import evaluator
from chromite.cros_bisect import simple_chrome_builder
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import json_lib
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import remote_access


class AutotestEvaluator(evaluator.Evaluator):
  """Evaluates performance by running autotest test.

  It first try running autotest from DUT (via ssh command). If it fails to run
  (e.g. first running the test on the DUT), it then runs "test_that" inside
  CrOS's chrome_sdk to pack test package and push it to DUT to execute.

  After autotest done running, it grabs JSON report file and finds the metric
  to watch (currently support float value).
  """
  AUTOTEST_BASE = '/usr/local/autotest'
  AUTOTEST_CLIENT = os.path.join(AUTOTEST_BASE, 'bin', 'autotest_client')
  REQUIRED_ARGS = evaluator.Evaluator.REQUIRED_ARGS + (
      'board', 'chromium_dir', 'cros_dir', 'test_name', 'metric',
      'metric_take_average', 'eval_passing_only')
  CROS_DIR = 'cros'
  RESULT_FILENAME = 'results-chart.json'

  def __init__(self, options):
    """Constructor.

    Args:
      options: In addition to the flags required by the base class, need to
        specify:
        * board: CrOS board name (used for running host side autotest).
        * chromium_dir: Optional. If specified, use the chromium repo the path
          points to. Otherwise, use base_dir/chromium/src.
        * cros_dir: Optional. If specified, use it to enter CrOS chroot to
          run autotest "test_that" command. Otherwise, use base_dir/cros.
        * test_name: Autotest name to run.
        * metric: Metric to look up.
        * metric_take_average: If set, take average value of the metric.
        * eval_passing_only: If set, use existing perf result only if test was
            passing.
    """
    super(AutotestEvaluator, self).__init__(options)
    self.board = options.board
    self.test_name = options.test_name
    self.metric = options.metric
    self.metric_take_average = options.metric_take_average
    self.eval_passing_only = options.eval_passing_only
    # Used for entering chroot. Some autotest depends on CHROME_ROOT being set.
    if options.chromium_dir:
      self.chromium_dir = options.chromium_dir
    else:
      self.chromium_dir = os.path.join(self.base_dir,
                                       simple_chrome_builder.CHROMIUM_DIR)

    if options.cros_dir:
      self.cros_dir = options.cros_dir
    else:
      self.cros_dir = os.path.join(self.base_dir, self.CROS_DIR)

  def RunTestFromDut(self, remote, report_file):
    """Runs autotest from DUT.

    It runs autotest from DUT directly. It can only be used after the test was
    deployed/run using "test_that" from host.

    Args:
      remote: DUT for running test (refer lib.commandline.Device).
      report_file: Benchmark report to store (host side).

    Returns:
      False if sanity check fails, i.e. autotest control file missing.
      If autotest ran successfully, or --eval-failsafe is set, it returns
      if the test report is retrieved from DUT. Otherwise, False.
    """
    def RetrieveReport(dut, remote_report_file):
      """Retrieves report from DUT to local.

      Args:
        dut: a RemoteAccess object to access DUT.
        remote_report_file: path of the report on DUT to retrieve.

      Returns:
        True if a report is copied from DUT to local path (report_file).
      """
      logging.info('Copy report from DUT(%s:%s) to %s',
                   dut.remote_host, remote_report_file, report_file)
      scp_result = dut.ScpToLocal(remote_report_file, report_file,
                                  error_code_ok=True)
      if scp_result.returncode != 0:
        logging.error('Failed to copy report from DUT(%s:%s) to host(%s)',
                      dut.remote_host, remote_report_file, report_file)
        return False
      return True

    # TODO(deanliao): Deal with the case that test control file is not the
    #     same as below.
    test_target = os.path.join(self.AUTOTEST_BASE, 'tests', self.test_name,
                               'control')

    remote_report_file = os.path.join(
        self.AUTOTEST_BASE, 'results', 'default', self.test_name, 'results',
        self.RESULT_FILENAME)

    with osutils.TempDir() as temp_dir:
      dut = remote_access.RemoteAccess(
          remote.hostname, temp_dir, port=remote.port, username=remote.username)

      run_test_command = [self.AUTOTEST_CLIENT, test_target]
      logging.info('Run autotest from DUT %s: %s', dut.remote_host,
                   cros_build_lib.CmdToStr(run_test_command))

      # Make sure that both self.AUTOTEST_CLIENT and test_target exist.
      sanity_check_result = dut.RemoteSh(['ls'] + run_test_command,
                                         error_code_ok=True, ssh_error_ok=True)
      if sanity_check_result.returncode != 0:
        logging.info('Failed to run autotest from DUT %s: One of %s does not '
                     'exist.', dut.remote_host, run_test_command)
        return False

      run_test_result = dut.RemoteSh(run_test_command, error_code_ok=True,
                                     ssh_error_ok=True)
      run_test_returncode = run_test_result.returncode

      if run_test_returncode != 0:
        logging.info('Run failed (returncode: %d)', run_test_returncode)
        if self.eval_passing_only:
          return False
      else:
        logging.info('Ran successfully.')
      return RetrieveReport(dut, remote_report_file)

  def LookupReportFile(self):
    """Looks up autotest report file.

    It looks up results-chart.json under chroot's /tmp/test_that_latest.

    Returns:
      Path to report file. None if not found.
    """
    # Default result dir: /tmp/test_that_latest
    results_dir = self.ResolvePathFromChroot(os.path.join(
        '/tmp', 'test_that_latest', 'results-1-%s' % self.test_name))
    # Invoking "find" command is faster than using os.walkdir().
    try:
      command_result = cros_build_lib.RunCommand(
          ['find', '.', '-name', self.RESULT_FILENAME],
          cwd=results_dir, capture_output=True)
    except cros_build_lib.RunCommandError as e:
      logging.error('Failed to look up %s under %s: %s', self.RESULT_FILENAME,
                    results_dir, e)
      return None
    if not command_result.output:
      logging.error('Failed to look up %s under %s', self.RESULT_FILENAME,
                    results_dir)
      return None
    report_file_under_results_dir = (
        command_result.output.splitlines()[0].strip())
    return os.path.normpath(
        os.path.join(results_dir, report_file_under_results_dir))

  def SetupCrosRepo(self):
    """Gets the ChromeOS source code.

    It is used to enter cros_sdk to run autotest by executing "test_that"
    command.
    """
    osutils.SafeMakedirs(self.cros_dir)
    cros_build_lib.RunCommand(
        ['repo', 'init', '-u',
         'https://chromium.googlesource.com/chromiumos/manifest.git',
         '--repo-url',
         'https://chromium.googlesource.com/external/repo.git'],
        cwd=self.cros_dir)
    cros_build_lib.RunCommand(['repo', 'sync', '-j8'],
                              cwd=self.cros_dir)

  def MaySetupBoard(self):
    """Checks if /build/${board} exists. Sets it up if not.

    Returns:
      False if setup_board or build_package failed. True otherwise.
    """
    if not os.path.isdir(self.cros_dir):
      logging.notice('ChromeOS source: %s does not exist, set it up',
                     self.cros_dir)
      self.SetupCrosRepo()

    board_path = self.ResolvePathFromChroot(os.path.join('/build', self.board))
    if os.path.isdir(board_path):
      return True

    try:
      self.RunCommandInsideCrosSdk(['./setup_board', '--board', self.board])
    except cros_build_lib.RunCommandError as e:
      logging.error('Failed to setup_board for %s: %s', self.board, e)
      return False

    try:
      self.RunCommandInsideCrosSdk(['./build_packages', '--board', self.board])
    except cros_build_lib.RunCommandError as e:
      logging.error('Failed to build_package for %s: %s', self.board, e)
      return False
    return True

  def ResolvePathFromChroot(self, path_inside_chroot):
    """Resolves path from chroot.

    Args:
      path_inside_chroot: path inside chroot.

    Returns:
      Path outside chroot which points to the given path inside chroot.
    """
    return path_util.ChrootPathResolver(source_path=self.cros_dir).FromChroot(
        path_inside_chroot)

  def RunCommandInsideCrosSdk(self, command):
    """Runs command inside cros_sdk.

    The cros_sdk it used is under self.cros_dir. And its chrome_root is set to
    self.chrome_root.

    Args:
      command: command as a list of arguments.

    Returns:
      A CommandResult object.

    Raises:
      RunCommandError:  Raises exception on error with optional error_message.
    """
    # --chrome_root is needed for autotests running Telemetry.
    # --no-ns-pid is used to prevent the program receiving SIGTTIN (e.g. go to
    # background and stopped) when asking user input.
    chroot_args = ['--chrome_root', self.chromium_dir,
                   '--no-ns-pid']
    return cros_build_lib.RunCommand(command, enter_chroot=True,
                                     chroot_args=chroot_args, cwd=self.cros_dir)

  def RunTestFromHost(self, remote, report_file_to_store):
    """Runs autotest from host.

    It uses test_that tool in CrOS chroot to deploy autotest to DUT and run it.

    Args:
      remote: DUT for running test (refer lib.commandline.Device).
      report_file_to_store: Benchmark report to store.

    Returns:
      False if sanity check fails, i.e. setup_board inside chroot fails.
      If autotest ran successfully inside chroot, or --eval-failsafe is set, it
      returns if the test report is retrieved from chroot. Otherwise, False.
    """
    if not self.MaySetupBoard():
      return False

    run_autotest = ['test_that',
                    '-b', self.board,
                    '--fast',
                    '--args', 'local=True',
                    remote.raw,
                    self.test_name]

    try:
      self.RunCommandInsideCrosSdk(run_autotest)
    except cros_build_lib.RunCommandError as e:
      if self.eval_passing_only:
        logging.error('Failed to run autotest: %s', e)
        return False

    report_file_in_chroot = self.LookupReportFile()
    if not report_file_in_chroot:
      logging.error('Failed to run autotest: report file not found')
      return False

    try:
      shutil.copyfile(report_file_in_chroot, report_file_to_store)
    except Exception as e:
      logging.error('Failed to retrieve report from chroot %s to %s',
                    report_file_in_chroot, report_file_to_store)
      return False
    return True

  def GetAutotestMetricValue(self, report_file):
    """Gets metric value from autotest benchmark report.

    Report example:
      {"avg_fps_1000_fishes": {
         "summary": {
           "units": "fps",
           "type": "scalar",
           "value": 56.733810392225671,
           "improvement_direction": "up"
         },
         ...,
       },
       ...,
      }
      self.metric = "avg_fps_1000_fishes/summary/value"

    Args:
      report_file: Path to benchmark report.

    Returns:
      Metric value in benchmark report.
      None if self.metric is undefined or metric does not exist in the report.
    """
    if not self.metric:
      return None

    report = json_lib.ParseJsonFileWithComments(report_file)
    metric_value = json_lib.GetNestedDictValue(report, self.metric.split('/'))
    if metric_value is None:
      logging.error('Cannot get metric %s from %s', self.metric, report_file)
      return None
    if self.metric_take_average:
      return float(sum(metric_value)) / len(metric_value)
    return metric_value

  def GetReportPath(self, build_label, nth_eval, repeat):
    """Obtains report file path.

    Args:
      build_label: current build label to run the evaluation.
      nth_eval: n-th evaluation.
      repeat: #repeat.

    Returns:
      Report file path.
    """
    return os.path.join(
        self.report_base_dir,
        'results-chart.%s.%d-%d.json' % (build_label, nth_eval, repeat))

  def Evaluate(self, remote, build_label, repeat=1):
    """Runs autotest N-times on DUT and extracts the designated metric values.

    Args:
      remote: DUT to evaluate (refer lib.commandline.Device).
      build_label: Build label used for part of report filename and log message.
      repeat: Run test for N times. Default 1.

    Returns:
      Score object stores a list of autotest running results.
    """
    if repeat == 1:
      times_str = 'once'
    elif repeat == 2:
      times_str = 'twice'
    else:
      times_str = '%d times' % repeat
    logging.info(
        'Evaluating build %s performance on DUT %s by running autotest %s %s '
        'to get metric %s',
        build_label, remote.raw, self.test_name, times_str, self.metric)

    score_list = []
    for nth in range(repeat):
      report_file = self.GetReportPath(build_label, nth + 1, repeat)
      score = self._EvaluateOnce(remote, report_file)
      if score is None:
        return common.Score()
      logging.info(
          'Run autotest %d/%d. Got result: %s:%s = %.3f (build:%s DUT:%s).',
          nth + 1, repeat, self.test_name, self.metric, score, build_label,
          remote.raw)
      score_list.append(score)

    scores = common.Score(score_list)
    logging.info(
        'Successfully ran autotest %d times. Arithmetic mean(%s:%s) = %.3f',
        repeat, self.test_name, self.metric, scores.mean)
    return scores

  def _EvaluateOnce(self, remote, report_file):
    """Runs autotest on DUT once and extracts the designated metric value."""
    success = self.RunTestFromDut(remote, report_file)
    if not success:
      logging.info('Failed to run autotest from DUT. Failover to run autotest '
                   'from host using "test_that" command.')
      success = self.RunTestFromHost(remote, report_file)
      if not success:
        logging.error('Failed to run autotest.')
        return None
    return self.GetAutotestMetricValue(report_file)

  def CheckLastEvaluate(self, build_label, repeat=1):
    """Checks if previous evaluate report is available.

    Args:
      build_label: Build label used for part of report filename and log message.
      repeat: Run test for N times. Default 1.

    Returns:
      Score object stores a list of autotest running results if report
      available and reuse_eval is set.
      Score() otherwise.
    """
    if not self.reuse_eval:
      return common.Score()

    score_list = []
    for nth in range(repeat):
      report_file = self.GetReportPath(build_label, nth + 1, repeat)
      if not os.path.isfile(report_file):
        return common.Score()
      score = self.GetAutotestMetricValue(report_file)
      if score is None:
        return common.Score()
      score_list.append(score)
    scores = common.Score(score_list)
    logging.info(
        'Used archived autotest result. Arithmetic mean(%s:%s) = '
        '%.3f (build:%s)',
        self.test_name, self.metric, scores.mean, build_label)
    return scores
