# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Functions for interfacing with the Chromium Swarming Server.

The Swarming Server is a task distribution service. It can be used to kick off
a test run.

API explorer: https://goo.gl/uxPUZo
"""

from dashboard.services import request


_API_PATH = 'api/swarming/v1'


class Swarming(object):

  def __init__(self, server):
    self._server = server

  def Bot(self, bot_id):
    return Bot(self._server, bot_id)

  def Bots(self):
    return Bots(self._server)

  def Task(self, task_id):
    return Task(self._server, task_id)

  def Tasks(self):
    return Tasks(self._server)


class Bot(object):

  def __init__(self, server, bot_id):
    self._server = server
    self._bot_id = bot_id

  def Get(self):
    """Returns information about a known bot.

    This includes its state and dimensions, and if it is currently running a
    task."""
    return self._Request('get')

  def Tasks(self):
    """Lists a given bot's tasks within the specified date range."""
    return self._Request('tasks')

  def _Request(self, path, **kwargs):
    url = '%s/%s/bot/%s/%s' % (self._server, _API_PATH, self._bot_id, path)
    return request.RequestJson(url, **kwargs)


class Bots(object):

  def __init__(self, server):
    self._server = server

  def List(self, cursor=None, dimensions=None, is_dead=None, limit=None,
           quarantined=None):
    """Provides list of known bots. Deleted bots will not be listed."""
    if dimensions:
      dimensions = tuple(':'.join(dimension)
                         for dimension in dimensions.iteritems())

    url = '%s/%s/bots/list' % (self._server, _API_PATH)
    return request.RequestJson(
        url, cursor=cursor, dimensions=dimensions,
        is_dead=is_dead, limit=limit, quarantined=quarantined)


class Task(object):

  def __init__(self, server, task_id):
    self._server = server
    self._task_id = task_id

  def Cancel(self):
    """Cancels a task.

    If a bot was running the task, the bot will forcibly cancel the task."""
    return self._Request('cancel', method='POST')

  def Request(self):
    """Returns the task request corresponding to a task ID."""
    return self._Request('request')

  def Result(self, include_performance_stats=False):
    """Reports the result of the task corresponding to a task ID.

    It can be a 'run' ID specifying a specific retry or a 'summary' ID hiding
    the fact that a task may have been retried transparently, when a bot reports
    BOT_DIED. A summary ID ends with '0', a run ID ends with '1' or '2'."""
    if include_performance_stats:
      return self._Request('result', include_performance_stats=True)
    else:
      return self._Request('result')

  def Stdout(self):
    """Returns the output of the task corresponding to a task ID."""
    return self._Request('stdout')

  def _Request(self, path, **kwargs):
    url = '%s/%s/task/%s/%s' % (self._server, _API_PATH, self._task_id, path)
    return request.RequestJson(url, **kwargs)


class Tasks(object):

  def __init__(self, server):
    self._server = server

  def New(self, body):
    """Creates a new task.

    The task will be enqueued in the tasks list and will be executed at the
    earliest opportunity by a bot that has at least the dimensions as described
    in the task request.
    """
    url = '%s/%s/tasks/new' % (self._server, _API_PATH)
    return request.RequestJson(url, method='POST', body=body)
