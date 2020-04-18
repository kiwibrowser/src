#!/usr/bin/env python
# Copyright 2017 The PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Compares the performance of two versions of the pdfium code."""

import argparse
import functools
import glob
import json
import multiprocessing
import os
import re
import shutil
import subprocess
import sys
import tempfile

from common import GetBooleanGnArg
from common import PrintErr
from common import RunCommandPropagateErr
from githelper import GitHelper
from safetynet_conclusions import ComparisonConclusions
from safetynet_conclusions import PrintConclusionsDictHumanReadable
from safetynet_conclusions import RATING_IMPROVEMENT
from safetynet_conclusions import RATING_REGRESSION
from safetynet_image import ImageComparison


def RunSingleTestCaseParallel(this, run_label, build_dir, test_case):
  result = this.RunSingleTestCase(run_label, build_dir, test_case)
  return (test_case, result)


class CompareRun(object):
  """A comparison between two branches of pdfium."""

  def __init__(self, args):
    self.git = GitHelper()
    self.args = args
    self._InitPaths()

  def _InitPaths(self):
    if self.args.this_repo:
      self.safe_script_dir = self.args.build_dir
    else:
      self.safe_script_dir = os.path.join('testing', 'tools')

    self.safe_measure_script_path = os.path.abspath(
        os.path.join(self.safe_script_dir,
                     'safetynet_measure.py'))

    input_file_re = re.compile('^.+[.]pdf$')
    self.test_cases = []
    for input_path in self.args.input_paths:
      if os.path.isfile(input_path):
        self.test_cases.append(input_path)
      elif os.path.isdir(input_path):
        for file_dir, _, filename_list in os.walk(input_path):
          for input_filename in filename_list:
            if input_file_re.match(input_filename):
              file_path = os.path.join(file_dir, input_filename)
              if os.path.isfile(file_path):
                self.test_cases.append(file_path)

    self.after_build_dir = self.args.build_dir
    if self.args.build_dir_before:
      self.before_build_dir = self.args.build_dir_before
    else:
      self.before_build_dir = self.after_build_dir

  def Run(self):
    """Runs comparison by checking out branches, building and measuring them.

    Returns:
      Exit code for the script.
    """
    if self.args.this_repo:
      self._FreezeMeasureScript()

    if self.args.branch_after:
      if self.args.this_repo:
        before, after = self._ProfileTwoOtherBranchesInThisRepo(
            self.args.branch_before,
            self.args.branch_after)
      else:
        before, after = self._ProfileTwoOtherBranches(
            self.args.branch_before,
            self.args.branch_after)
    elif self.args.branch_before:
      if self.args.this_repo:
        before, after = self._ProfileCurrentAndOtherBranchInThisRepo(
            self.args.branch_before)
      else:
        before, after = self._ProfileCurrentAndOtherBranch(
            self.args.branch_before)
    else:
      if self.args.this_repo:
        before, after = self._ProfileLocalChangesAndCurrentBranchInThisRepo()
      else:
        before, after = self._ProfileLocalChangesAndCurrentBranch()

    conclusions = self._DrawConclusions(before, after)
    conclusions_dict = conclusions.GetOutputDict()
    conclusions_dict.setdefault('metadata', {})['profiler'] = self.args.profiler

    self._PrintConclusions(conclusions_dict)

    self._CleanUp(conclusions)

    if self.args.png_dir:
      image_comparison = ImageComparison(
          self.after_build_dir,
          self.args.png_dir,
          ('before', 'after'),
          self.args.num_workers,
          self.args.png_threshold)
      image_comparison.Run(open_in_browser=not self.args.machine_readable)

    return 0

  def _FreezeMeasureScript(self):
    """Freezes a version of the measuring script.

    This is needed to make sure we are comparing the pdfium library changes and
    not script changes that may happen between the two branches.
    """
    self.__FreezeFile(os.path.join('testing', 'tools', 'safetynet_measure.py'))
    self.__FreezeFile(os.path.join('testing', 'tools', 'common.py'))

  def __FreezeFile(self, file):
    RunCommandPropagateErr(['cp', file, self.safe_script_dir],
                           exit_status_on_error=1)

  def _ProfileTwoOtherBranchesInThisRepo(self, before_branch, after_branch):
    """Profiles two branches that are not the current branch.

    This is done in the local repository and changes may not be restored if the
    script fails or is interrupted.

    after_branch does not need to descend from before_branch, they will be
    measured the same way

    Args:
      before_branch: One branch to profile.
      after_branch: Other branch to profile.

    Returns:
      A tuple (before, after), where each of before and after is a dict
      mapping a test case name to the profiling values for that test case
      in the given branch.
    """
    branch_to_restore = self.git.GetCurrentBranchName()

    self._StashLocalChanges()

    self._CheckoutBranch(after_branch)
    self._BuildCurrentBranch(self.after_build_dir)
    after = self._MeasureCurrentBranch('after', self.after_build_dir)

    self._CheckoutBranch(before_branch)
    self._BuildCurrentBranch(self.before_build_dir)
    before = self._MeasureCurrentBranch('before', self.before_build_dir)

    self._CheckoutBranch(branch_to_restore)
    self._RestoreLocalChanges()

    return before, after

  def _ProfileTwoOtherBranches(self, before_branch, after_branch):
    """Profiles two branches that are not the current branch.

    This is done in new, cloned repositories, therefore it is safer but slower
    and requires downloads.

    after_branch does not need to descend from before_branch, they will be
    measured the same way

    Args:
      before_branch: One branch to profile.
      after_branch: Other branch to profile.

    Returns:
      A tuple (before, after), where each of before and after is a dict
      mapping a test case name to the profiling values for that test case
      in the given branch.
    """
    after = self._ProfileSeparateRepo('after',
                                      self.after_build_dir,
                                      after_branch)
    before = self._ProfileSeparateRepo('before',
                                       self.before_build_dir,
                                       before_branch)
    return before, after

  def _ProfileCurrentAndOtherBranchInThisRepo(self, other_branch):
    """Profiles the current branch (with uncommitted changes) and another one.

    This is done in the local repository and changes may not be restored if the
    script fails or is interrupted.

    The current branch does not need to descend from other_branch.

    Args:
      other_branch: Other branch to profile that is not the current.

    Returns:
      A tuple (before, after), where each of before and after is a dict
      mapping a test case name to the profiling values for that test case
      in the given branch. The current branch is considered to be "after" and
      the other branch is considered to be "before".
    """
    branch_to_restore = self.git.GetCurrentBranchName()

    self._BuildCurrentBranch(self.after_build_dir)
    after = self._MeasureCurrentBranch('after', self.after_build_dir)

    self._StashLocalChanges()

    self._CheckoutBranch(other_branch)
    self._BuildCurrentBranch(self.before_build_dir)
    before = self._MeasureCurrentBranch('before', self.before_build_dir)

    self._CheckoutBranch(branch_to_restore)
    self._RestoreLocalChanges()

    return before, after

  def _ProfileCurrentAndOtherBranch(self, other_branch):
    """Profiles the current branch (with uncommitted changes) and another one.

    This is done in new, cloned repositories, therefore it is safer but slower
    and requires downloads.

    The current branch does not need to descend from other_branch.

    Args:
      other_branch: Other branch to profile that is not the current. None will
          compare to the same branch.

    Returns:
      A tuple (before, after), where each of before and after is a dict
      mapping a test case name to the profiling values for that test case
      in the given branch. The current branch is considered to be "after" and
      the other branch is considered to be "before".
    """
    self._BuildCurrentBranch(self.after_build_dir)
    after = self._MeasureCurrentBranch('after', self.after_build_dir)

    before = self._ProfileSeparateRepo('before',
                                       self.before_build_dir,
                                       other_branch)

    return before, after

  def _ProfileLocalChangesAndCurrentBranchInThisRepo(self):
    """Profiles the current branch with and without uncommitted changes.

    This is done in the local repository and changes may not be restored if the
    script fails or is interrupted.

    Returns:
      A tuple (before, after), where each of before and after is a dict
      mapping a test case name to the profiling values for that test case
      using the given version. The current branch without uncommitted changes is
      considered to be "before" and with uncommitted changes is considered to be
      "after".
    """
    self._BuildCurrentBranch(self.after_build_dir)
    after = self._MeasureCurrentBranch('after', self.after_build_dir)

    pushed = self._StashLocalChanges()
    if not pushed and not self.args.build_dir_before:
      PrintErr('Warning: No local changes to compare')

    before_build_dir = self.before_build_dir

    self._BuildCurrentBranch(before_build_dir)
    before = self._MeasureCurrentBranch('before', before_build_dir)

    self._RestoreLocalChanges()

    return before, after

  def _ProfileLocalChangesAndCurrentBranch(self):
    """Profiles the current branch with and without uncommitted changes.

    This is done in new, cloned repositories, therefore it is safer but slower
    and requires downloads.

    Returns:
      A tuple (before, after), where each of before and after is a dict
      mapping a test case name to the profiling values for that test case
      using the given version. The current branch without uncommitted changes is
      considered to be "before" and with uncommitted changes is considered to be
      "after".
    """
    return self._ProfileCurrentAndOtherBranch(other_branch=None)

  def _ProfileSeparateRepo(self, run_label, relative_build_dir, branch):
    """Profiles a branch in a a temporary git repository.

    Args:
      run_label: String to differentiate this version of the code in output
          files from other versions.
      relative_build_dir: Path to the build dir in the current working dir to
          clone build args from.
      branch: Branch to checkout in the new repository. None will
          profile the same branch checked out in the original repo.
    Returns:
      A dict mapping each test case name to the profiling values for that
      test case.
    """
    build_dir = self._CreateTempRepo('repo_%s' % run_label,
                                     relative_build_dir,
                                     branch)

    self._BuildCurrentBranch(build_dir)
    return self._MeasureCurrentBranch(run_label, build_dir)

  def _CreateTempRepo(self, dir_name, relative_build_dir, branch):
    """Clones a temporary git repository out of the current working dir.

    Args:
      dir_name: Name for the temporary repository directory
      relative_build_dir: Path to the build dir in the current working dir to
          clone build args from.
      branch: Branch to checkout in the new repository. None will keep checked
          out the same branch as the local repo.
    Returns:
      Path to the build directory of the new repository.
    """
    cwd = os.getcwd()

    repo_dir = tempfile.mkdtemp(suffix='-%s' % dir_name)
    src_dir = os.path.join(repo_dir, 'pdfium')

    self.git.CloneLocal(os.getcwd(), src_dir)

    if branch is not None:
      os.chdir(src_dir)
      self.git.Checkout(branch)

    os.chdir(repo_dir)
    PrintErr('Syncing...')

    cmd = ['gclient', 'config', '--unmanaged',
           'https://pdfium.googlesource.com/pdfium.git']
    if self.args.cache_dir:
      cmd.append('--cache-dir=%s' % self.args.cache_dir)
    RunCommandPropagateErr(cmd, exit_status_on_error=1)

    RunCommandPropagateErr(['gclient', 'sync'], exit_status_on_error=1)

    PrintErr('Done.')

    build_dir = os.path.join(src_dir, relative_build_dir)
    os.makedirs(build_dir)
    os.chdir(src_dir)

    source_gn_args = os.path.join(cwd, relative_build_dir, 'args.gn')
    dest_gn_args = os.path.join(build_dir, 'args.gn')
    shutil.copy(source_gn_args, dest_gn_args)

    RunCommandPropagateErr(['gn', 'gen', relative_build_dir],
                           exit_status_on_error=1)

    os.chdir(cwd)

    return build_dir


  def _CheckoutBranch(self, branch):
    PrintErr("Checking out branch '%s'" % branch)
    self.git.Checkout(branch)

  def _StashLocalChanges(self):
    PrintErr('Stashing local changes')
    return self.git.StashPush()

  def _RestoreLocalChanges(self):
    PrintErr('Restoring local changes')
    self.git.StashPopAll()

  def _BuildCurrentBranch(self, build_dir):
    """Synchronizes and builds the current version of pdfium.

    Args:
      build_dir: String with path to build directory
    """
    PrintErr('Syncing...')
    RunCommandPropagateErr(['gclient', 'sync'], exit_status_on_error=1)
    PrintErr('Done.')

    PrintErr('Building...')
    cmd = ['ninja', '-C', build_dir, 'pdfium_test']
    if GetBooleanGnArg('use_goma', build_dir):
      cmd.extend(['-j', '250'])
    RunCommandPropagateErr(cmd, stdout_has_errors=True, exit_status_on_error=1)
    PrintErr('Done.')

  def _MeasureCurrentBranch(self, run_label, build_dir):
    PrintErr('Measuring...')
    if self.args.num_workers > 1 and len(self.test_cases) > 1:
      results = self._RunAsync(run_label, build_dir)
    else:
      results = self._RunSync(run_label, build_dir)
    PrintErr('Done.')

    return results

  def _RunSync(self, run_label, build_dir):
    """Profiles the test cases synchronously.

    Args:
      run_label: String to differentiate this version of the code in output
          files from other versions.
      build_dir: String with path to build directory

    Returns:
      A dict mapping each test case name to the profiling values for that
      test case.
    """
    results = {}

    for test_case in self.test_cases:
      result = self.RunSingleTestCase(run_label, build_dir, test_case)
      if result is not None:
        results[test_case] = result

    return results

  def _RunAsync(self, run_label, build_dir):
    """Profiles the test cases asynchronously.

    Uses as many workers as configured by --num-workers.

    Args:
      run_label: String to differentiate this version of the code in output
          files from other versions.
      build_dir: String with path to build directory

    Returns:
      A dict mapping each test case name to the profiling values for that
      test case.
    """
    results = {}
    pool = multiprocessing.Pool(self.args.num_workers)
    worker_func = functools.partial(
        RunSingleTestCaseParallel, self, run_label, build_dir)

    try:
      # The timeout is a workaround for http://bugs.python.org/issue8296
      # which prevents KeyboardInterrupt from working.
      one_year_in_seconds = 3600 * 24 * 365
      worker_results = (pool.map_async(worker_func, self.test_cases)
                        .get(one_year_in_seconds))
      for worker_result in worker_results:
        test_case, result = worker_result
        if result is not None:
          results[test_case] = result
    except KeyboardInterrupt:
      pool.terminate()
      sys.exit(1)
    else:
      pool.close()

    pool.join()

    return results

  def RunSingleTestCase(self, run_label, build_dir, test_case):
    """Profiles a single test case.

    Args:
      run_label: String to differentiate this version of the code in output
          files from other versions.
      build_dir: String with path to build directory
      test_case: Path to the test case.

    Returns:
      The measured profiling value for that test case.
    """
    command = [self.safe_measure_script_path, test_case,
               '--build-dir=%s' % build_dir]

    if self.args.interesting_section:
      command.append('--interesting-section')

    if self.args.profiler:
      command.append('--profiler=%s' % self.args.profiler)

    profile_file_path = self._GetProfileFilePath(run_label, test_case)
    if profile_file_path:
      command.append('--output-path=%s' % profile_file_path)

    if self.args.png_dir:
      command.append('--png')

    if self.args.pages:
      command.extend(['--pages', self.args.pages])

    output = RunCommandPropagateErr(command)

    if output is None:
      return None

    if self.args.png_dir:
      self._MoveImages(test_case, run_label)

    # Get the time number as output, making sure it's just a number
    output = output.strip()
    if re.match('^[0-9]+$', output):
      return int(output)

    return None

  def _MoveImages(self, test_case, run_label):
    png_dir = os.path.join(self.args.png_dir, run_label)
    if not os.path.exists(png_dir):
      os.makedirs(png_dir)

    test_case_dir, test_case_filename = os.path.split(test_case)
    test_case_png_matcher = '%s.*.png' % test_case_filename
    for output_png in glob.glob(os.path.join(test_case_dir,
                                             test_case_png_matcher)):
      shutil.move(output_png, png_dir)

  def _GetProfileFilePath(self, run_label, test_case):
    if self.args.output_dir:
      output_filename = ('callgrind.out.%s.%s'
                         % (test_case.replace('/', '_'),
                            run_label))
      return os.path.join(self.args.output_dir, output_filename)
    else:
      return None

  def _DrawConclusions(self, times_before_branch, times_after_branch):
    """Draws conclusions comparing results of test runs in two branches.

    Args:
      times_before_branch: A dict mapping each test case name to the
          profiling values for that test case in the branch to be considered
          as the baseline.
      times_after_branch: A dict mapping each test case name to the
          profiling values for that test case in the branch to be considered
          as the new version.

    Returns:
      ComparisonConclusions with all test cases processed.
    """
    conclusions = ComparisonConclusions(self.args.threshold_significant)

    for test_case in sorted(self.test_cases):
      before = times_before_branch.get(test_case)
      after = times_after_branch.get(test_case)
      conclusions.ProcessCase(test_case, before, after)

    return conclusions

  def _PrintConclusions(self, conclusions_dict):
    """Prints the conclusions as the script output.

    Depending on the script args, this can output a human or a machine-readable
    version of the conclusions.

    Args:
      conclusions_dict: Dict to print returned from
          ComparisonConclusions.GetOutputDict().
    """
    if self.args.machine_readable:
      print json.dumps(conclusions_dict)
    else:
      PrintConclusionsDictHumanReadable(
          conclusions_dict, colored=True, key=self.args.case_order)

  def _CleanUp(self, conclusions):
    """Removes profile output files for uninteresting cases.

    Cases without significant regressions or improvements and considered
    uninteresting.

    Args:
      conclusions: A ComparisonConclusions.
    """
    if not self.args.output_dir:
      return

    if self.args.profiler != 'callgrind':
      return

    for case_result in conclusions.GetCaseResults().values():
      if case_result.rating not in [RATING_REGRESSION, RATING_IMPROVEMENT]:
        self._CleanUpOutputFile('before', case_result.case_name)
        self._CleanUpOutputFile('after', case_result.case_name)

  def _CleanUpOutputFile(self, run_label, case_name):
    """Removes one profile output file.

    If the output file does not exist, fails silently.

    Args:
      run_label: String to differentiate a version of the code in output
          files from other versions.
      case_name: String identifying test case for which to remove the output
          file.
    """
    try:
      os.remove(self._GetProfileFilePath(run_label, case_name))
    except OSError:
      pass


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('input_paths', nargs='+',
                      help='pdf files or directories to search for pdf files '
                           'to run as test cases')
  parser.add_argument('--branch-before',
                      help='git branch to use as "before" for comparison. '
                           'Omitting this will use the current branch '
                           'without uncommitted changes as the baseline.')
  parser.add_argument('--branch-after',
                      help='git branch to use as "after" for comparison. '
                           'Omitting this will use the current branch '
                           'with uncommitted changes.')
  parser.add_argument('--build-dir', default=os.path.join('out', 'Release'),
                      help='relative path from the base source directory '
                           'to the build directory')
  parser.add_argument('--build-dir-before',
                      help='relative path from the base source directory '
                           'to the build directory for the "before" branch, if '
                           'different from the build directory for the '
                           '"after" branch')
  parser.add_argument('--cache-dir', default=None,
                      help='directory with a new or preexisting cache for '
                           'downloads. Default is to not use a cache.')
  parser.add_argument('--this-repo', action='store_true',
                      help='use the repository where the script is instead of '
                           'checking out a temporary one. This is faster and '
                           'does not require downloads, but although it '
                           'restores the state of the local repo, if the '
                           'script is killed or crashes the changes can remain '
                           'stashed and you may be on another branch.')
  parser.add_argument('--profiler', default='callgrind',
                      help='which profiler to use. Supports callgrind, '
                           'perfstat, and none. Default is callgrind.')
  parser.add_argument('--interesting-section', action='store_true',
                      help='whether to measure just the interesting section or '
                           'the whole test harness. Limiting to only the '
                           'interesting section does not work on Release since '
                           'the delimiters are optimized out')
  parser.add_argument('--pages',
                      help='selects some pages to be rendered. Page numbers '
                           'are 0-based. "--pages A" will render only page A. '
                           '"--pages A-B" will render pages A to B '
                           '(inclusive).')
  parser.add_argument('--num-workers', default=multiprocessing.cpu_count(),
                      type=int, help='run NUM_WORKERS jobs in parallel')
  parser.add_argument('--output-dir',
                      help='directory to write the profile data output files')
  parser.add_argument('--png-dir', default=None,
                      help='outputs pngs to the specified directory that can '
                           'be compared with a static html generated. Will '
                           'affect performance measurements.')
  parser.add_argument('--png-threshold', default=0.0, type=float,
                      help='Requires --png-dir. Threshold above which a png '
                           'is considered to have changed.')
  parser.add_argument('--threshold-significant', default=0.02, type=float,
                      help='variations in performance above this factor are '
                           'considered significant')
  parser.add_argument('--machine-readable', action='store_true',
                      help='whether to get output for machines. If enabled the '
                           'output will be a json with the format specified in '
                           'ComparisonConclusions.GetOutputDict(). Default is '
                           'human-readable.')
  parser.add_argument('--case-order', default=None,
                      help='what key to use when sorting test cases in the '
                           'output. Accepted values are "after", "before", '
                           '"ratio" and "rating". Default is sorting by test '
                           'case path.')

  args = parser.parse_args()

  # Always start at the pdfium src dir, which is assumed to be two level above
  # this script.
  pdfium_src_dir = os.path.join(
      os.path.dirname(__file__),
      os.path.pardir,
      os.path.pardir)
  os.chdir(pdfium_src_dir)

  git = GitHelper()

  if args.branch_after and not args.branch_before:
    PrintErr('--branch-after requires --branch-before to be specified.')
    return 1

  if args.branch_after and not git.BranchExists(args.branch_after):
    PrintErr('Branch "%s" does not exist' % args.branch_after)
    return 1

  if args.branch_before and not git.BranchExists(args.branch_before):
    PrintErr('Branch "%s" does not exist' % args.branch_before)
    return 1

  if args.output_dir:
    args.output_dir = os.path.expanduser(args.output_dir)
    if not os.path.isdir(args.output_dir):
      PrintErr('"%s" is not a directory' % args.output_dir)
      return 1

  if args.png_dir:
    args.png_dir = os.path.expanduser(args.png_dir)
    if not os.path.isdir(args.png_dir):
      PrintErr('"%s" is not a directory' % args.png_dir)
      return 1

  if args.threshold_significant <= 0.0:
    PrintErr('--threshold-significant should receive a positive float')
    return 1

  if args.png_threshold:
    if not args.png_dir:
      PrintErr('--png-threshold requires --png-dir to be specified.')
      return 1

    if args.png_threshold <= 0.0:
      PrintErr('--png-threshold should receive a positive float')
      return 1

  if args.pages:
    if not re.match(r'^\d+(-\d+)?$', args.pages):
      PrintErr('Supported formats for --pages are "--pages 7" and '
               '"--pages 3-6"')
      return 1

  run = CompareRun(args)
  return run.Run()


if __name__ == '__main__':
  sys.exit(main())
