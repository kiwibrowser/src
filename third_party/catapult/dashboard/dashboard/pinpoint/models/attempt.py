# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Attempt(object):
  """One run of all the Quests on a Change.

  Each Change should execute at least one Attempt. The user can request more
  runs, which creates additional Attempts. The bisect algorithm may also create
  additional Attempts if it decides it needs more information to establish
  greater statistical confidence.

  Each Attempt executes the Quests in order. Quests are never skipped, even if
  they were already run in a previous Attempt. Caching (e.g. the binary was
  already built in a previous Attempt) should be handled internally to the
  Quest, and is not visible to the user.

  An Execution object is created for each Quest when it starts running. The
  Attempt is finished when the last Execution returns no result_arguments. If an
  Execution fails, it should not set its result_arguments, and the Attempt will
  have fewer Executions in the end.
  """

  def __init__(self, quests, change):
    assert quests
    self._quests = quests
    self._change = change
    self._executions = []

  @property
  def quests(self):
    return tuple(self._quests)

  @property
  def executions(self):
    return tuple(self._executions)

  @property
  def completed(self):
    """Returns True iff the Attempt is completed. Otherwise, it is in progress.

    This accessor doesn't contact external servers. Call _Poll() to update the
    Attempt's completed status.
    """
    if not self._executions:
      return not self._quests

    return self._last_execution.failed or (
        self._last_execution.completed and
        len(self._quests) == len(self._executions))

  @property
  def failed(self):
    return bool(self._executions and self._last_execution.failed)

  @property
  def _last_execution(self):
    return self._executions[-1]

  def AsDict(self):
    return {'executions': [e.AsDict() for e in self._executions]}

  def ScheduleWork(self):
    """Run this Attempt and update its status."""
    assert not self.completed

    self._StartNextExecutionIfReady()
    self._Poll()

  def _Poll(self):
    """Update the Attempt status."""
    self._last_execution.Poll()

  def _StartNextExecutionIfReady(self):
    can_start_next_execution = not self._executions or (
        self._last_execution.completed and not self.completed)
    if not can_start_next_execution:
      return

    next_quest = self._quests[len(self._executions)]
    arguments = {'change': self._change}
    if self._executions:
      arguments.update(self._last_execution.result_arguments)

    self._executions.append(next_quest.Start(**arguments))
