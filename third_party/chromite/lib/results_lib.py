# -*- coding: utf-8 -*-
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes for collecting results of our BuildStages as they run."""

from __future__ import print_function

import collections
import datetime
import math
import os

from chromite.lib import constants
from chromite.lib import failures_lib
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging

def _GetCheckpointFile(buildroot):
  return os.path.join(buildroot, '.completed_stages')


def WriteCheckpoint(buildroot):
  """Drops a completed stages file with current state."""
  completed_stages_file = _GetCheckpointFile(buildroot)
  with open(completed_stages_file, 'w+') as save_file:
    Results.SaveCompletedStages(save_file)


def LoadCheckpoint(buildroot):
  """Restore completed stage info from checkpoint file."""
  completed_stages_file = _GetCheckpointFile(buildroot)
  if not os.path.exists(completed_stages_file):
    logging.warning('Checkpoint file not found in buildroot %s' % buildroot)
    return

  with open(completed_stages_file, 'r') as load_file:
    Results.RestoreCompletedStages(load_file)


class RecordedTraceback(object):
  """This class represents a traceback recorded in the list of results."""

  def __init__(self, failed_stage, failed_prefix, exception, traceback):
    """Construct a RecordedTraceback object.

    Args:
      failed_stage: The stage that failed during the build. E.g., HWTest [bvt]
      failed_prefix: The prefix of the stage that failed. E.g., HWTest
      exception: The raw exception object.
      traceback: The full stack trace for the failure, as a string.
    """
    self.failed_stage = failed_stage
    self.failed_prefix = failed_prefix
    self.exception = exception
    self.traceback = traceback


_result_fields = ['name', 'result', 'description', 'prefix', 'board', 'time']
Result = collections.namedtuple('Result', _result_fields)


class _Results(object):
  """Static class that collects the results of our BuildStages as they run."""

  SUCCESS = "Stage was successful"
  FORGIVEN = "Stage failed but was optional"
  SKIPPED = "Stage was skipped"
  NON_FAILURE_TYPES = (SUCCESS, FORGIVEN, SKIPPED)

  SPLIT_TOKEN = r'\_O_/'

  def __init__(self):
    # List of results for all stages that's built up as we run. Members are of
    #  the form ('name', SUCCESS | FORGIVEN | Exception, None | description)
    self._results_log = []

    # A list of instances of failure_message_lib.StageFailureMessage to present
    # the exceptions threw by failed stages.
    self._failure_message_results = []

    # Stages run in a previous run and restored. Stored as a dictionary of
    # names to previous records.
    self._previous = {}

    self.start_time = datetime.datetime.now()

  def Clear(self):
    """Clear existing stage results."""
    self.__init__()

  def PreviouslyCompletedRecord(self, name):
    """Check to see if this stage was previously completed.

    Returns:
      A boolean showing the stage was successful in the previous run.
    """
    return self._previous.get(name)

  def BuildSucceededSoFar(self, db=None, build_id=None, name=None):
    """Return true if all stages so far have passing states.

    This method returns true if all was successful or forgiven or skipped.

    Args:
      db: cidb connection isinstance.
      build_id: build_id of the build to check.
      name: stage name of current stage.
    """
    build_succeess = all(entry.result in self.NON_FAILURE_TYPES
                         for entry in self._results_log)

    # When timeout happens and background tasks are killed, the statuses
    # of the background stage tasks may get lost. BuildSucceededSoFar may
    # still return build_succeess = True when the killed stage tasks were
    # failed. Add one more verification step in _BuildSucceededFromCIDB to
    # check the stage status in CIDB.
    return (build_succeess and
            self._BuildSucceededFromCIDB(db=db, build_id=build_id, name=name))

  def _BuildSucceededFromCIDB(self, db=None, build_id=None, name=None):
    """Return True if all stages recorded in CIDB have passing states.

    Args:
      db: cidb connection isinstance.
      build_id: build_id of the build to check.
      name: stage name of current stage.
    """
    if db is not None and build_id is not None:
      stages = db.GetBuildStages(build_id)
      for stage in stages:
        if name is not None and stage['name'] == name:
          logging.info("Ignore status of %s as it's the current stage.",
                       stage['name'])
          continue
        if stage['status'] not in constants.BUILDER_NON_FAILURE_STATUSES:
          logging.warning('Failure in previous stage %s with status %s.',
                          stage['name'], stage['status'])
          return False

    return True

  def StageHasResults(self, name):
    """Return true if stage has posted results."""
    return name in [entry.name for entry in self._results_log]

  def _RecordStageFailureMessage(self, name, exception, prefix=None,
                                 build_stage_id=None):
    self._failure_message_results.append(
        failures_lib.GetStageFailureMessageFromException(
            name, build_stage_id, exception, stage_prefix_name=prefix))

  def Record(self, name, result, description=None, prefix=None, board='',
             time=0, build_stage_id=None):
    """Store off an additional stage result.

    Args:
      name: The name of the stage (e.g. HWTest [bvt])
      result:
        Result should be one of:
          Results.SUCCESS if the stage was successful.
          Results.SKIPPED if the stage was skipped.
          Results.FORGIVEN if the stage had warnings.
          Otherwise, it should be the exception stage errored with.
      description:
        The textual backtrace of the exception, or None
      prefix: The prefix of the stage (e.g. HWTest). Defaults to
        the value of name.
      board: The board associated with the stage, if any. Defaults to ''.
      time: How long the result took to complete.
      build_stage_id: The id of the failed build stage to record, default to
        None.
    """
    if prefix is None:
      prefix = name

    # Convert exception to stage_failure_message and record it.
    if isinstance(result, BaseException):
      self._RecordStageFailureMessage(name, result, prefix=prefix,
                                      build_stage_id=build_stage_id)

    result = Result(name, result, description, prefix, board, time)
    self._results_log.append(result)

  def GetStageFailureMessage(self):
    return self._failure_message_results

  def Get(self):
    """Fetch stage results.

    Returns:
      A list with one entry per stage run with a result.
    """
    return self._results_log

  def GetPrevious(self):
    """Fetch stage results.

    Returns:
      A list of stages names that were completed in a previous run.
    """
    return self._previous

  def SaveCompletedStages(self, out):
    """Save the successfully completed stages to the provided file |out|."""
    for entry in self._results_log:
      if entry.result != self.SUCCESS:
        break
      out.write(self.SPLIT_TOKEN.join(map(str, entry)) + '\n')

  def RestoreCompletedStages(self, out):
    """Load the successfully completed stages from the provided file |out|."""
    # Read the file, and strip off the newlines.
    for line in out:
      record = line.strip().split(self.SPLIT_TOKEN)
      if len(record) != len(_result_fields):
        logging.warning('State file does not match expected format, ignoring.')
        # Wipe any partial state.
        self._previous = {}
        break

      self._previous[record[0]] = Result(*record)

  def GetTracebacks(self):
    """Get a list of the exceptions that failed the build.

    Returns:
       A list of RecordedTraceback objects.
    """
    tracebacks = []
    for entry in self._results_log:
      # If entry.result is not in NON_FAILURE_TYPES, then the stage failed, and
      # entry.result is the exception object and entry.description is a string
      # containing the full traceback.
      if entry.result not in self.NON_FAILURE_TYPES:
        traceback = RecordedTraceback(entry.name, entry.prefix, entry.result,
                                      entry.description)
        tracebacks.append(traceback)
    return tracebacks

  def Report(self, out, current_version=None):
    """Generate a user friendly text display of the results data.

    Args:
      out: Output stream to write to (e.g. sys.stdout).
      current_version: Chrome OS version associated with this report.
    """
    results = self._results_log

    line = '*' * 60 + '\n'
    edge = '*' * 2

    if current_version:
      out.write(line)
      out.write(edge +
                ' RELEASE VERSION: ' +
                current_version +
                '\n')

    out.write(line)
    out.write(edge + ' Stage Results\n')
    warnings = False

    for entry in results:
      name, result, run_time = (entry.name, entry.result, entry.time)
      timestr = datetime.timedelta(seconds=math.ceil(run_time))

      # Don't print data on skipped stages.
      if result == self.SKIPPED:
        continue

      out.write(line)
      details = ''
      if result == self.SUCCESS:
        status = 'PASS'
      elif result == self.FORGIVEN:
        status = 'FAILED BUT FORGIVEN'
        warnings = True
      else:
        status = 'FAIL'
        if isinstance(result, cros_build_lib.RunCommandError):
          # If there was a RunCommand error, give just the command that
          # failed, not its full argument list, since those are usually
          # too long.
          details = ' in %s' % result.result.cmd[0]
        elif isinstance(result, failures_lib.BuildScriptFailure):
          # BuildScriptFailure errors publish a 'short' name of the
          # command that failed.
          details = ' in %s' % result.shortname
        else:
          # There was a normal error. Give the type of exception.
          details = ' with %s' % type(result).__name__

      out.write('%s %s %s (%s)%s\n' % (edge, status, name, timestr, details))

    out.write(line)

    for x in self.GetTracebacks():
      if x.failed_stage and x.traceback:
        out.write('\nFailed in stage %s:\n\n' % x.failed_stage)
        out.write(x.traceback)
        out.write('\n')

    if warnings:
      logging.PrintBuildbotStepWarnings(out)


Results = _Results()
