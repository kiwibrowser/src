#!/usr/bin/env python
# Copyright 2017 The PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Looks for performance regressions on all pushes since the last run.

Run this nightly to have a periodical check for performance regressions.

Stores the results for each run and last checkpoint in a results directory.
"""

import argparse
import datetime
import json
import os
import sys

from common import PrintWithTime
from common import RunCommandPropagateErr
from githelper import GitHelper
from safetynet_conclusions import PrintConclusionsDictHumanReadable


class JobContext(object):
  """Context for a single run, including name and directory paths."""

  def __init__(self, args):
    self.datetime = datetime.datetime.now().strftime('%Y-%m-%d-%H-%M-%S')
    self.results_dir = args.results_dir
    self.last_revision_covered_file = os.path.join(self.results_dir,
                                                   'last_revision_covered')
    self.run_output_dir = os.path.join(self.results_dir,
                                       'profiles_%s' % self.datetime)
    self.run_output_log_file = os.path.join(self.results_dir,
                                            '%s.log' % self.datetime)


class JobRun(object):
  """A single run looking for regressions since the last one."""

  def __init__(self, args, context):
    """Constructor.

    Args:
      args: Namespace with arguments passed to the script.
      context: JobContext for this run.
    """
    self.git = GitHelper()
    self.args = args
    self.context = context

  def Run(self):
    """Searches for regressions.

    Will only write a checkpoint when first run, and on all subsequent runs
    a comparison is done against the last checkpoint.

    Returns:
      Exit code for the script: 0 if no significant changes are found; 1 if
      there was an error in the comparison; 3 if there was a regression; 4 if
      there was an improvement and no regression.
    """
    pdfium_src_dir = os.path.join(
        os.path.dirname(__file__),
        os.path.pardir,
        os.path.pardir)
    os.chdir(pdfium_src_dir)

    if not self.git.IsCurrentBranchClean() and not self.args.no_checkout:
      PrintWithTime('Current branch is not clean, aborting')
      return 1

    branch_to_restore = self.git.GetCurrentBranchName()

    if not self.args.no_checkout:
      self.git.FetchOriginMaster()
      self.git.Checkout('origin/master')

    # Make sure results dir exists
    if not os.path.exists(self.context.results_dir):
      os.makedirs(self.context.results_dir)

    if not os.path.exists(self.context.last_revision_covered_file):
      result = self._InitialRun()
    else:
      with open(self.context.last_revision_covered_file) as f:
        last_revision_covered = f.read().strip()
      result = self._IncrementalRun(last_revision_covered)

    self.git.Checkout(branch_to_restore)
    return result

  def _InitialRun(self):
    """Initial run, just write a checkpoint.

    Returns:
      Exit code for the script.
    """
    current = self.git.GetCurrentBranchHash()

    PrintWithTime('Initial run, current is %s' % current)

    self._WriteCheckpoint(current)

    PrintWithTime('All set up, next runs will be incremental and perform '
                  'comparisons')
    return 0

  def _IncrementalRun(self, last_revision_covered):
    """Incremental run, compare against last checkpoint and update it.

    Args:
      last_revision_covered: String with hash for last checkpoint.

    Returns:
      Exit code for the script.
    """
    current = self.git.GetCurrentBranchHash()

    PrintWithTime('Incremental run, current is %s, last is %s'
                  % (current, last_revision_covered))

    if not os.path.exists(self.context.run_output_dir):
      os.makedirs(self.context.run_output_dir)

    if current == last_revision_covered:
      PrintWithTime('No changes seen, finishing job')
      output_info = {
          'metadata': self._BuildRunMetadata(last_revision_covered,
                                             current,
                                             False)}
      self._WriteRawJson(output_info)
      return 0

    # Run compare
    cmd = ['testing/tools/safetynet_compare.py',
           '--this-repo',
           '--machine-readable',
           '--branch-before=%s' % last_revision_covered,
           '--output-dir=%s' % self.context.run_output_dir]
    cmd.extend(self.args.input_paths)

    json_output = RunCommandPropagateErr(cmd)

    if json_output is None:
      return 1

    output_info = json.loads(json_output)

    run_metadata = self._BuildRunMetadata(last_revision_covered,
                                          current,
                                          True)
    output_info.setdefault('metadata', {}).update(run_metadata)
    self._WriteRawJson(output_info)

    PrintConclusionsDictHumanReadable(output_info,
                                      colored=(not self.args.output_to_log
                                               and not self.args.no_color),
                                      key='after')

    status = 0

    if output_info['summary']['improvement']:
      PrintWithTime('Improvement detected.')
      status = 4

    if output_info['summary']['regression']:
      PrintWithTime('Regression detected.')
      status = 3

    if status == 0:
      PrintWithTime('Nothing detected.')

    self._WriteCheckpoint(current)

    return status

  def _WriteRawJson(self, output_info):
    json_output_file = os.path.join(self.context.run_output_dir, 'raw.json')
    with open(json_output_file, 'w') as f:
      json.dump(output_info, f)

  def _BuildRunMetadata(self, revision_before, revision_after,
                        comparison_performed):
    return {
        'datetime': self.context.datetime,
        'revision_before': revision_before,
        'revision_after': revision_after,
        'comparison_performed': comparison_performed,
    }

  def _WriteCheckpoint(self, checkpoint):
    if not self.args.no_checkpoint:
      with open(self.context.last_revision_covered_file, 'w') as f:
        f.write(checkpoint + '\n')


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('results_dir',
                      help='where to write the job results')
  parser.add_argument('input_paths', nargs='+',
                      help='pdf files or directories to search for pdf files '
                           'to run as test cases')
  parser.add_argument('--no-checkout', action='store_true',
                      help='whether to skip checking out origin/master. Use '
                           'for script debugging.')
  parser.add_argument('--no-checkpoint', action='store_true',
                      help='whether to skip writing the new checkpoint. Use '
                           'for script debugging.')
  parser.add_argument('--no-color', action='store_true',
                      help='whether to write output without color escape '
                           'codes.')
  parser.add_argument('--output-to-log', action='store_true',
                      help='whether to write output to a log file')
  args = parser.parse_args()

  job_context = JobContext(args)

  if args.output_to_log:
    log_file = open(job_context.run_output_log_file, 'w')
    sys.stdout = log_file
    sys.stderr = log_file

  run = JobRun(args, job_context)
  result = run.Run()

  if args.output_to_log:
    log_file.close()

  return result


if __name__ == '__main__':
  sys.exit(main())

