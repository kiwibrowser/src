# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import mock

from dashboard.pinpoint.models.quest import run_test


_BASE_ARGUMENTS = {
    'swarming_server': 'server',
    'dimensions': [{'key': 'value'}],
}


class StartTest(unittest.TestCase):

  def testStart(self):
    quest = run_test.RunTest('server', [{'key': 'value'}], ['arg'])
    execution = quest.Start('change', 'https://isolate.server', 'isolate hash')
    self.assertEqual(execution._extra_args, ['arg'])

  # TODO: Remove after there are no more jobs running RunTest quests
  # (instead of RunTelemetryTest quests).
  def testResultsLabel(self):
    quest = run_test.RunTest('server', [{'key': 'value'}],
                             ['--results-label', ''])
    execution = quest.Start('change', 'https://isolate.server', 'isolate hash')
    self.assertEqual(execution._extra_args, ['--results-label', 'change'])


class FromDictTest(unittest.TestCase):

  def testMinimumArguments(self):
    quest = run_test.RunTest.FromDict(_BASE_ARGUMENTS)
    expected = run_test.RunTest('server', [{'key': 'value'}],
                                run_test._DEFAULT_EXTRA_ARGS)
    self.assertEqual(quest, expected)

  def testAllArguments(self):
    arguments = dict(_BASE_ARGUMENTS)
    arguments['extra_test_args'] = '["--custom-arg", "custom value"]'
    quest = run_test.RunTest.FromDict(arguments)

    extra_args = ['--custom-arg', 'custom value'] + run_test._DEFAULT_EXTRA_ARGS
    expected = run_test.RunTest('server', [{'key': 'value'}], extra_args)
    self.assertEqual(quest, expected)

  def testMissingSwarmingServer(self):
    arguments = dict(_BASE_ARGUMENTS)
    del arguments['swarming_server']
    with self.assertRaises(TypeError):
      run_test.RunTest.FromDict(arguments)

  def testMissingDimensions(self):
    arguments = dict(_BASE_ARGUMENTS)
    del arguments['dimensions']
    with self.assertRaises(TypeError):
      run_test.RunTest.FromDict(arguments)

  def testStringDimensions(self):
    arguments = dict(_BASE_ARGUMENTS)
    arguments['dimensions'] = '[{"key": "value"}]'
    quest = run_test.RunTest.FromDict(arguments)
    expected = run_test.RunTest('server', [{'key': 'value'}],
                                run_test._DEFAULT_EXTRA_ARGS)
    self.assertEqual(quest, expected)

  def testInvalidExtraTestArgs(self):
    arguments = dict(_BASE_ARGUMENTS)
    arguments['extra_test_args'] = '"this is a json-encoded string"'
    with self.assertRaises(TypeError):
      run_test.RunTest.FromDict(arguments)

  def testStringExtraTestArgs(self):
    arguments = dict(_BASE_ARGUMENTS)
    arguments['extra_test_args'] = '--custom-arg "custom value"'
    quest = run_test.RunTest.FromDict(arguments)

    extra_args = ['--custom-arg', 'custom value'] + run_test._DEFAULT_EXTRA_ARGS
    expected = run_test.RunTest('server', [{'key': 'value'}], extra_args)
    self.assertEqual(quest, expected)


class _RunTestExecutionTest(unittest.TestCase):

  def assertNewTaskHasDimensions(self, swarming_tasks_new):
    body = {
        'name': 'Pinpoint job',
        'user': 'Pinpoint',
        'priority': '100',
        'expiration_secs': '86400',
        'properties': {
            'inputs_ref': {
                'isolatedserver': 'isolate server',
                'isolated': 'input isolate hash',
            },
            'extra_args': ['arg'],
            'dimensions': [
                {'key': 'pool', 'value': 'Chrome-perf-pinpoint'},
                {'key': 'value'},
            ],
            'execution_timeout_secs': '7200',
            'io_timeout_secs': '1200',
        },
    }
    swarming_tasks_new.assert_called_with(body)

  def assertNewTaskHasBotId(self, swarming_tasks_new):
    body = {
        'name': 'Pinpoint job',
        'user': 'Pinpoint',
        'priority': '100',
        'expiration_secs': '86400',
        'properties': {
            'inputs_ref': {
                'isolatedserver': 'isolate server',
                'isolated': 'input isolate hash',
            },
            'extra_args': ['arg'],
            'dimensions': [
                {'key': 'pool', 'value': 'Chrome-perf-pinpoint'},
                {'key': 'id', 'value': 'bot id'},
            ],
            'execution_timeout_secs': '7200',
            'io_timeout_secs': '1200',
        },
    }
    swarming_tasks_new.assert_called_with(body)


@mock.patch('dashboard.services.swarming.Tasks.New')
@mock.patch('dashboard.services.swarming.Task.Result')
class RunTestFullTest(_RunTestExecutionTest):

  def testSuccess(self, swarming_task_result, swarming_tasks_new):
    # Goes through a full run of two Executions.

    # Call RunTest.Start() to create an Execution.
    quest = run_test.RunTest('server', [{'key': 'value'}], ['arg'])
    execution = quest.Start('change_1', 'isolate server', 'input isolate hash')

    swarming_task_result.assert_not_called()
    swarming_tasks_new.assert_not_called()

    # Call the first Poll() to start the swarming task.
    swarming_tasks_new.return_value = {'task_id': 'task id'}
    execution.Poll()

    swarming_task_result.assert_not_called()
    self.assertEqual(swarming_tasks_new.call_count, 1)
    self.assertNewTaskHasDimensions(swarming_tasks_new)
    self.assertFalse(execution.completed)
    self.assertFalse(execution.failed)

    # Call subsequent Poll()s to check the task status.
    swarming_task_result.return_value = {'state': 'PENDING'}
    execution.Poll()

    self.assertFalse(execution.completed)
    self.assertFalse(execution.failed)

    swarming_task_result.return_value = {
        'bot_id': 'bot id',
        'exit_code': 0,
        'failure': False,
        'outputs_ref': {
            'isolatedserver': 'output isolate server',
            'isolated': 'output isolate hash',
        },
        'state': 'COMPLETED',
    }
    execution.Poll()

    self.assertTrue(execution.completed)
    self.assertFalse(execution.failed)
    self.assertEqual(execution.result_values, ())
    self.assertEqual(execution.result_arguments, {
        'isolate_server': 'output isolate server',
        'isolate_hash': 'output isolate hash',
    })
    self.assertEqual(execution.AsDict(), {
        'completed': True,
        'exception': None,
        'details': {
            'bot_id': 'bot id',
            'task_id': 'task id',
        },
        'result_arguments': {
            'isolate_server': 'output isolate server',
            'isolate_hash': 'output isolate hash',
        },
    })

    # Start a second Execution on another Change. It should use the bot_id
    # from the first execution.
    execution = quest.Start('change_2', 'isolate server', 'input isolate hash')
    execution.Poll()

    self.assertNewTaskHasBotId(swarming_tasks_new)

    # Start an Execution on the same Change. It should use a new bot_id.
    execution = quest.Start('change_2', 'isolate server', 'input isolate hash')
    execution.Poll()

    self.assertNewTaskHasDimensions(swarming_tasks_new)


@mock.patch('dashboard.services.swarming.Tasks.New')
@mock.patch('dashboard.services.swarming.Task.Result')
class SwarmingTaskStatusTest(_RunTestExecutionTest):

  def testSwarmingError(self, swarming_task_result, swarming_tasks_new):
    swarming_task_result.return_value = {'state': 'BOT_DIED'}
    swarming_tasks_new.return_value = {'task_id': 'task id'}

    quest = run_test.RunTest('server', [{'key': 'value'}], ['arg'])
    execution = quest.Start(None, 'isolate server', 'input isolate hash')
    execution.Poll()
    execution.Poll()

    self.assertTrue(execution.completed)
    self.assertTrue(execution.failed)
    last_exception_line = execution.exception.splitlines()[-1]
    self.assertTrue(last_exception_line.startswith('SwarmingTaskError'))

  def testTestError(self, swarming_task_result, swarming_tasks_new):
    swarming_task_result.return_value = {
        'bot_id': 'bot id',
        'exit_code': 1,
        'failure': True,
        'state': 'COMPLETED',
    }
    swarming_tasks_new.return_value = {'task_id': 'task id'}

    quest = run_test.RunTest('server', [{'key': 'value'}], ['arg'])
    execution = quest.Start(None, 'isolate server', 'isolate_hash')
    execution.Poll()
    execution.Poll()

    self.assertTrue(execution.completed)
    self.assertTrue(execution.failed)
    last_exception_line = execution.exception.splitlines()[-1]
    self.assertTrue(last_exception_line.startswith('SwarmingTestError'))


@mock.patch('dashboard.services.swarming.Tasks.New')
@mock.patch('dashboard.services.swarming.Task.Result')
class BotIdHandlingTest(_RunTestExecutionTest):

  def testExecutionExpired(
      self, swarming_task_result, swarming_tasks_new):
    # If the Swarming task expires, the bots are overloaded or the dimensions
    # don't correspond to any bot. Raise an error that's fatal to the Job.
    swarming_tasks_new.return_value = {'task_id': 'task id'}
    swarming_task_result.return_value = {'state': 'EXPIRED'}

    quest = run_test.RunTest('server', [{'key': 'value'}], ['arg'])
    execution = quest.Start('change_1', 'isolate server', 'input isolate hash')
    execution.Poll()
    with self.assertRaises(run_test.SwarmingExpiredError):
      execution.Poll()

  def testFirstExecutionFailedWithNoBotId(
      self, swarming_task_result, swarming_tasks_new):
    # If the first Execution fails before it gets a bot ID, it's likely it
    # couldn't find any device to run on. Subsequent Executions probably
    # wouldn't have any better luck, and failing fast is less complex than
    # handling retries.
    swarming_tasks_new.return_value = {'task_id': 'task id'}
    swarming_task_result.return_value = {'state': 'CANCELED'}

    quest = run_test.RunTest('server', [{'key': 'value'}], ['arg'])
    execution = quest.Start('change_1', 'isolate server', 'input isolate hash')
    execution.Poll()
    execution.Poll()

    swarming_task_result.return_value = {
        'bot_id': 'bot id',
        'exit_code': 0,
        'failure': False,
        'outputs_ref': {
            'isolatedserver': 'output isolate server',
            'isolated': 'output isolate hash',
        },
        'state': 'COMPLETED',
    }
    execution = quest.Start('change_2', 'isolate server', 'input isolate hash')
    execution.Poll()

    self.assertTrue(execution.completed)
    self.assertTrue(execution.failed)
    last_exception_line = execution.exception.splitlines()[-1]
    self.assertTrue(last_exception_line.startswith('RunTestError'))

  def testSimultaneousExecutions(self, swarming_task_result,
                                 swarming_tasks_new):
    # Executions after the first must wait for the first execution to get a bot
    # ID. To preserve device affinity, they must use the same bot.
    quest = run_test.RunTest('server', [{'key': 'value'}], ['arg'])
    execution_1 = quest.Start('change_1', 'input isolate server',
                              'input isolate hash')
    execution_2 = quest.Start('change_2', 'input isolate server',
                              'input isolate hash')

    swarming_tasks_new.return_value = {'task_id': 'task id'}
    swarming_task_result.return_value = {'state': 'PENDING'}
    execution_1.Poll()
    execution_2.Poll()

    self.assertEqual(swarming_tasks_new.call_count, 1)

    swarming_task_result.return_value = {
        'bot_id': 'bot id',
        'exit_code': 0,
        'failure': False,
        'outputs_ref': {
            'isolatedserver': 'output isolate server',
            'isolated': 'output isolate hash',
        },
        'state': 'COMPLETED',
    }
    execution_1.Poll()
    execution_2.Poll()

    self.assertEqual(swarming_tasks_new.call_count, 2)
