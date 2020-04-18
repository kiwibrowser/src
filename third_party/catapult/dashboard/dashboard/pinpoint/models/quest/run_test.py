# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Quest and Execution for running a test in Swarming.

This is the only Quest/Execution where the Execution has a reference back to
modify the Quest.
"""

import collections
import copy
import json
import shlex

from dashboard.pinpoint.models.quest import execution as execution_module
from dashboard.pinpoint.models.quest import quest
from dashboard.services import swarming


_DEFAULT_EXTRA_ARGS = [
    '--isolated-script-test-output', '${ISOLATED_OUTDIR}/output.json',
    '--isolated-script-test-chartjson-output',
    '${ISOLATED_OUTDIR}/chartjson-output.json',
]


class RunTestError(Exception):

  pass


class SwarmingExpiredError(StandardError):

  def __init__(self, task_id):
    self.task_id = task_id
    super(SwarmingExpiredError, self).__init__(
        'The swarming task %s expired. The bots are likely overloaded or dead, '
        'or may be misconfigured.' % self.task_id)

  def __reduce__(self):
    # http://stackoverflow.com/a/36342588
    return SwarmingExpiredError, (self.task_id,)


class SwarmingTaskError(RunTestError):

  def __init__(self, task_id, state):
    self.task_id = task_id
    self.state = state
    super(SwarmingTaskError, self).__init__(
        'The swarming task %s failed with state "%s".' %
        (self.task_id, self.state))

  def __reduce__(self):
    # http://stackoverflow.com/a/36342588
    return SwarmingTaskError, (self.task_id, self.state)


class SwarmingTestError(RunTestError):

  def __init__(self, task_id, exit_code):
    self.task_id = task_id
    self.exit_code = exit_code
    super(SwarmingTestError, self).__init__(
        'The swarming task %s failed. The test exited with code %s.' %
        (self.task_id, self.exit_code))

  def __reduce__(self):
    # http://stackoverflow.com/a/36342588
    return SwarmingTestError, (self.task_id, self.exit_code)


class RunTest(quest.Quest):

  def __init__(self, swarming_server, dimensions, extra_args):
    self._swarming_server = swarming_server
    self._dimensions = dimensions
    self._extra_args = extra_args

    # We want subsequent executions use the same bot as the first one.
    self._canonical_executions = []
    self._execution_counts = collections.defaultdict(int)

  def __eq__(self, other):
    return (isinstance(other, type(self)) and
            self._swarming_server == other._swarming_server and
            self._dimensions == other._dimensions and
            self._extra_args == other._extra_args and
            self._canonical_executions == other._canonical_executions and
            self._execution_counts == other._execution_counts)


  def __str__(self):
    return 'Test'

  def Start(self, change, isolate_server, isolate_hash):
    return self._Start(change, isolate_server, isolate_hash, self._extra_args)

  def _Start(self, change, isolate_server, isolate_hash, extra_args):
    # TODO: Remove after there are no more jobs running RunTest quests
    # (instead of RunTelemetryTest quests).
    try:
      results_label_index = extra_args.index('--results-label')
      extra_args = copy.copy(extra_args)
      extra_args[results_label_index+1] = str(change)
    except ValueError:
      # If it's not there, this is probably a gtest
      pass

    index = self._execution_counts[change]
    self._execution_counts[change] += 1

    if not hasattr(self, '_swarming_server'):
      # TODO: Remove after data migration. crbug.com/822008
      self._swarming_server = 'https://chromium-swarm.appspot.com'
    if len(self._canonical_executions) <= index:
      execution = _RunTestExecution(self._swarming_server, self._dimensions,
                                    extra_args, isolate_server, isolate_hash)
      self._canonical_executions.append(execution)
    else:
      execution = _RunTestExecution(
          self._swarming_server, self._dimensions, extra_args, isolate_server,
          isolate_hash, previous_execution=self._canonical_executions[index])

    return execution

  @classmethod
  def FromDict(cls, arguments):
    return cls._FromDict(arguments, [])

  @classmethod
  def _FromDict(cls, arguments, swarming_extra_args):
    swarming_server = arguments.get('swarming_server')
    if not swarming_server:
      raise TypeError('Missing a "swarming_server" argument.')

    dimensions = arguments.get('dimensions')
    if not dimensions:
      raise TypeError('Missing a "dimensions" argument.')
    if isinstance(dimensions, basestring):
      dimensions = json.loads(dimensions)

    extra_test_args = arguments.get('extra_test_args')
    if extra_test_args:
      # We accept a json list or a string. If it can't be loaded as json, we
      # fall back to assuming it's a string argument.
      try:
        extra_test_args = json.loads(extra_test_args)
      except ValueError:
        extra_test_args = shlex.split(extra_test_args)
      if not isinstance(extra_test_args, list):
        raise TypeError('extra_test_args must be a list: %s' % extra_test_args)
      swarming_extra_args += extra_test_args

    return cls(swarming_server, dimensions,
               swarming_extra_args + _DEFAULT_EXTRA_ARGS)


class _RunTestExecution(execution_module.Execution):

  def __init__(self, swarming_server, dimensions, extra_args,
               isolate_server, isolate_hash, previous_execution=None):
    super(_RunTestExecution, self).__init__()
    self._swarming_server = swarming_server
    self._dimensions = dimensions
    self._extra_args = extra_args
    self._isolate_server = isolate_server
    self._isolate_hash = isolate_hash
    self._previous_execution = previous_execution

    self._task_id = None
    self._bot_id = None

  @property
  def bot_id(self):
    return self._bot_id

  def _AsDict(self):
    return {
        'bot_id': self._bot_id,
        'task_id': self._task_id,
    }

  def _Poll(self):
    if not self._task_id:
      self._StartTask()
      return

    if not hasattr(self, '_swarming_server'):
      # TODO: Remove after data migration. crbug.com/822008
      self._swarming_server = 'https://chromium-swarm.appspot.com'
    result = swarming.Swarming(
        self._swarming_server).Task(self._task_id).Result()

    if 'bot_id' in result:
      # Set bot_id to pass the info back to the Quest.
      self._bot_id = result['bot_id']

    if result['state'] == 'PENDING' or result['state'] == 'RUNNING':
      return

    if result['state'] == 'EXPIRED':
      raise SwarmingExpiredError(self._task_id)

    if result['state'] != 'COMPLETED':
      raise SwarmingTaskError(self._task_id, result['state'])

    if result['failure']:
      raise SwarmingTestError(self._task_id, result['exit_code'])

    result_arguments = {
        'isolate_server': result['outputs_ref']['isolatedserver'],
        'isolate_hash': result['outputs_ref']['isolated'],
    }
    self._Complete(result_arguments=result_arguments)


  def _StartTask(self):
    """Kick off a Swarming task to run a test."""
    if self._previous_execution and not self._previous_execution.bot_id:
      if self._previous_execution.failed:
        # If the previous Execution fails before it gets a bot ID, it's likely
        # it couldn't find any device to run on. Subsequent Executions probably
        # wouldn't have any better luck, and failing fast is less complex than
        # handling retries.
        raise RunTestError('There are no bots available to run the test.')
      else:
        return

    pool_dimension = None
    for dimension in self._dimensions:
      if dimension['key'] == 'pool':
        pool_dimension = dimension

    if self._previous_execution:
      dimensions = [
          # TODO: Remove fallback after data migration. crbug.com/822008
          pool_dimension or {'key': 'pool', 'value': 'Chrome-perf-pinpoint'},
          {'key': 'id', 'value': self._previous_execution.bot_id}
      ]
    else:
      dimensions = self._dimensions
      if not pool_dimension:
        # TODO: Remove after data migration. crbug.com/822008
        dimensions.insert(0, {'key': 'pool', 'value': 'Chrome-perf-pinpoint'})

    if not hasattr(self, '_isolate_server'):
      # TODO: Remove after data migration. crbug.com/822008
      self._isolate_server = 'https://isolateserver.appspot.com'
    body = {
        'name': 'Pinpoint job',
        'user': 'Pinpoint',
        'priority': '100',
        'expiration_secs': '86400',  # 1 day.
        'properties': {
            'inputs_ref': {
                'isolatedserver': self._isolate_server,
                'isolated': self._isolate_hash,
            },
            'extra_args': self._extra_args,
            'dimensions': dimensions,
            'execution_timeout_secs': '7200',  # 2 hours.
            'io_timeout_secs': '1200',  # 20 minutes, to match the perf bots.
        },
    }
    if not hasattr(self, '_swarming_server'):
      # TODO: Remove after data migration. crbug.com/822008
      self._swarming_server = 'https://chromium-swarm.appspot.com'
    response = swarming.Swarming(self._swarming_server).Tasks().New(body)

    self._task_id = response['task_id']
