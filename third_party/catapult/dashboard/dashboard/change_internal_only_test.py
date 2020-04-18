# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import webapp2
import webtest

from dashboard import add_point_queue
from dashboard import change_internal_only
from dashboard.common import stored_object
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import graph_data


class ChangeInternalOnlyTest(testing_common.TestCase):

  def setUp(self):
    super(ChangeInternalOnlyTest, self).setUp()
    app = webapp2.WSGIApplication([(
        '/change_internal_only',
        change_internal_only.ChangeInternalOnlyHandler)])
    self.testapp = webtest.TestApp(app)
    # Use a lower _MAX_ROWS_TO_PUT to make sure task queue is exercised.
    change_internal_only._MAX_ROWS_TO_PUT = 5
    change_internal_only._MAX_TESTS_TO_PUT = 1
    self.SetCurrentUser('foo@bar.com', is_admin=True)

  def _AddSampleData(self):
    """Adds sample data to the datastore, used in test methods below.

    All entities added here are added with internal_only=False.
    """
    testing_common.AddTests(
        ['ChromiumPerf', 'ChromiumGPU'],
        ['win7', 'mac'],
        {'scrolling': {'first_paint': {}}})
    test_paths = [
        'ChromiumPerf/win7/scrolling/first_paint',
        'ChromiumPerf/mac/scrolling/first_paint',
        'ChromiumGPU/win7/scrolling/first_paint',
        'ChromiumGPU/mac/scrolling/first_paint',
    ]
    for path in test_paths:
      testing_common.AddRows(path, [15000, 15005, 15010, 15015])
      anomaly.Anomaly(
          test=utils.TestKey(path),
          start_revision=15001,
          end_revision=15005,
          median_before_anomaly=100,
          median_after_anomaly=200).put()

  def testGet_ShowsForm(self):
    self._AddSampleData()
    response = self.testapp.get('/change_internal_only')
    self.assertEqual(1, len(response.html('form')))

  def testPost_SetInternalOnlyToTrue(self):
    self._AddSampleData()

    stored_object.Set(add_point_queue.BOT_WHITELIST_KEY, ['win7'])

    self.testapp.post('/change_internal_only', [
        ('internal_only', 'true'),
        ('bots', 'ChromiumPerf/win7'),
        ('bots', 'ChromiumGPU/mac'),
    ])
    self.ExecuteTaskQueueTasks(
        '/change_internal_only', change_internal_only._QUEUE_NAME)

    bot_whitelist = stored_object.Get(add_point_queue.BOT_WHITELIST_KEY)
    self.assertEqual(bot_whitelist, [])

    # Verify that Bot entities were changed.
    bots = graph_data.Bot.query().fetch()
    for bot in bots:
      path = bot.key.parent().string_id() + '/' + bot.key.string_id()
      if path == 'ChromiumPerf/win7' or path == 'ChromiumGPU/mac':
        self.assertTrue(bot.internal_only)
      else:
        self.assertFalse(bot.internal_only)

    # Verify that TestMetadata entities were changed.
    tests = graph_data.TestMetadata.query().fetch()
    for test in tests:
      if (test.test_path.startswith('ChromiumPerf/win7') or
          test.test_path.startswith('ChromiumGPU/mac')):
        self.assertTrue(test.internal_only)
      else:
        self.assertFalse(test.internal_only)

    # Verify that Anomaly entities were changed.
    anomalies = anomaly.Anomaly.query().fetch()
    for a in anomalies:
      test_path = utils.TestPath(a.test)
      if (test_path.startswith('ChromiumPerf/win7') or
          test_path.startswith('ChromiumGPU/mac')):
        self.assertTrue(a.internal_only)
      else:
        self.assertFalse(a.internal_only)

  def testPost_SetInternalOnlyToFalse(self):
    # First change to internal only.
    self._AddSampleData()

    stored_object.Set(add_point_queue.BOT_WHITELIST_KEY, ['win7'])

    self.testapp.post('/change_internal_only', [
        ('internal_only', 'true'),
        ('bots', 'ChromiumPerf/win7'),
        ('bots', 'ChromiumGPU/mac'),
    ])
    self.ExecuteTaskQueueTasks(
        '/change_internal_only', change_internal_only._QUEUE_NAME)

    bot_whitelist = stored_object.Get(add_point_queue.BOT_WHITELIST_KEY)
    self.assertEqual(bot_whitelist, [])

    # Then change back.
    self._AddSampleData()
    self.testapp.post('/change_internal_only', [
        ('internal_only', 'false'),
        ('bots', 'ChromiumPerf/win7'),
        ('bots', 'ChromiumGPU/mac'),
    ])
    self.ExecuteTaskQueueTasks(
        '/change_internal_only', change_internal_only._QUEUE_NAME)

    bot_whitelist = stored_object.Get(add_point_queue.BOT_WHITELIST_KEY)
    self.assertTrue('win7' in bot_whitelist)
    self.assertTrue('mac' in bot_whitelist)

    # No entities should be marked as internal only.
    bots = graph_data.Bot.query().fetch()
    for bot in bots:
      self.assertFalse(bot.internal_only)
    tests = graph_data.TestMetadata.query().fetch()
    for test in tests:
      self.assertFalse(test.internal_only)


if __name__ == '__main__':
  unittest.main()
