# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import mock

from dashboard.services import swarming


class _SwarmingTest(unittest.TestCase):

  def setUp(self):
    patcher = mock.patch('dashboard.services.request.RequestJson')
    self._request_json = patcher.start()
    self.addCleanup(patcher.stop)

    self._request_json.return_value = {'content': {}}

  def _AssertCorrectResponse(self, content):
    self.assertEqual(content, {'content': {}})

  def _AssertRequestMadeOnce(self, path, *args, **kwargs):
    self._request_json.assert_called_once_with(
        'https://server/api/swarming/v1/' + path, *args, **kwargs)


class BotTest(_SwarmingTest):

  def testGet(self):
    response = swarming.Swarming('https://server').Bot('bot_id').Get()
    self._AssertCorrectResponse(response)
    self._AssertRequestMadeOnce('bot/bot_id/get')

  def testTasks(self):
    response = swarming.Swarming('https://server').Bot('bot_id').Tasks()
    self._AssertCorrectResponse(response)
    self._AssertRequestMadeOnce('bot/bot_id/tasks')


class BotsTest(_SwarmingTest):

  def testList(self):
    response = swarming.Swarming('https://server').Bots().List(
        'CkMSPWoQ', {'pool': 'Chrome-perf', 'a': 'b'}, False, 1, True)
    self._AssertCorrectResponse(response)

    path = ('bots/list')
    self._AssertRequestMadeOnce(path, cursor='CkMSPWoQ',
                                dimensions=('a:b', 'pool:Chrome-perf'),
                                is_dead=False, limit=1, quarantined=True)


class TaskTest(_SwarmingTest):

  def testCancel(self):
    response = swarming.Swarming('https://server').Task('task_id').Cancel()
    self._AssertCorrectResponse(response)
    self._AssertRequestMadeOnce('task/task_id/cancel', method='POST')

  def testRequest(self):
    response = swarming.Swarming('https://server').Task('task_id').Request()
    self._AssertCorrectResponse(response)
    self._AssertRequestMadeOnce('task/task_id/request')

  def testResult(self):
    response = swarming.Swarming('https://server').Task('task_id').Result()
    self._AssertCorrectResponse(response)
    self._AssertRequestMadeOnce('task/task_id/result')

  def testResultWithPerformanceStats(self):
    response = swarming.Swarming('https://server').Task('task_id').Result(True)
    self._AssertCorrectResponse(response)
    self._AssertRequestMadeOnce('task/task_id/result',
                                include_performance_stats=True)

  def testStdout(self):
    response = swarming.Swarming('https://server').Task('task_id').Stdout()
    self._AssertCorrectResponse(response)
    self._AssertRequestMadeOnce('task/task_id/stdout')


class TasksTest(_SwarmingTest):

  def testNew(self):
    body = {
        'name': 'name',
        'user': 'user',
        'priority': '100',
        'expiration_secs': '600',
        'properties': {
            'inputs_ref': {
                'isolated': 'isolated_hash',
            },
            'extra_args': ['--output-format=histograms'],
            'dimensions': [
                {'key': 'id', 'value': 'bot_id'},
                {'key': 'pool', 'value': 'Chrome-perf'},
            ],
            'execution_timeout_secs': '3600',
            'io_timeout_secs': '3600',
        },
        'tags': [
            'id:bot_id',
            'pool:Chrome-perf',
        ],
    }

    response = swarming.Swarming('https://server').Tasks().New(body)
    self._AssertCorrectResponse(response)
    self._AssertRequestMadeOnce('tasks/new', method='POST', body=body)
