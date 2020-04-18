# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import webapp2
import webtest

from google.appengine.api import users

from dashboard import edit_anomaly_configs
from dashboard import edit_config_handler
from dashboard import list_tests
from dashboard import put_entities_task
from dashboard.common import testing_common
from dashboard.common import xsrf
from dashboard.models import anomaly_config
from dashboard.models import graph_data


class EditAnomalyConfigsTest(testing_common.TestCase):

  # This test case tests post requests to /edit_anomaly_configs.
  # Each post request is either a request to add an entity or to edit one.

  def setUp(self):
    super(EditAnomalyConfigsTest, self).setUp()
    app = webapp2.WSGIApplication(
        [
            ('/edit_anomaly_configs',
             edit_anomaly_configs.EditAnomalyConfigsHandler),
            ('/put_entities_task', put_entities_task.PutEntitiesTaskHandler),
        ])
    self.testapp = webtest.TestApp(app)

  def tearDown(self):
    super(EditAnomalyConfigsTest, self).tearDown()
    self.UnsetCurrentUser()

  def testAdd(self):
    """Tests changing the config property of an existing AnomalyConfig."""
    self.SetCurrentUser('qyearsley@chromium.org', is_admin=True)

    self.testapp.post('/edit_anomaly_configs', {
        'add-edit': 'add',
        'add-name': 'New Config',
        'config': '{"foo": 10}',
        'patterns': 'M/b/ts/*\nM/b/ts/*/*\n',
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
    })

    anomaly_configs = anomaly_config.AnomalyConfig.query().fetch()
    self.assertEqual(len(anomaly_configs), 1)
    config = anomaly_configs[0]
    self.assertEqual('New Config', config.key.string_id())
    self.assertEqual({'foo': 10}, config.config)
    self.assertEqual(['M/b/ts/*', 'M/b/ts/*/*'], config.patterns)

  def testEdit(self):
    """Tests changing the config property of an existing AnomalyConfig."""
    self.SetCurrentUser('sullivan@chromium.org', is_admin=True)
    anomaly_config.AnomalyConfig(
        id='Existing Config', config={'old': 11},
        patterns=['MyMaster/*/*/*']).put()

    self.testapp.post('/edit_anomaly_configs', {
        'add-edit': 'edit',
        'edit-name': 'Existing Config',
        'config': '{"new": 10}',
        'patterns': 'MyMaster/*/*/*',
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
    })

    anomaly_configs = anomaly_config.AnomalyConfig.query().fetch()
    self.assertEqual(len(anomaly_configs), 1)
    self.assertEqual('Existing Config', anomaly_configs[0].key.string_id())
    self.assertEqual({'new': 10}, anomaly_configs[0].config)
    self.assertEqual(['MyMaster/*/*/*'], anomaly_configs[0].patterns)

  def testEdit_AddPattern(self):
    """Tests changing the patterns list of an existing AnomalyConfig."""
    self.SetCurrentUser('sullivan@chromium.org', is_admin=True)
    master = graph_data.Master(id='TheMaster').put()
    graph_data.Bot(id='TheBot', parent=master).put()
    suite1 = graph_data.TestMetadata(id='TheMaster/TheBot/Suite1').put()
    suite2 = graph_data.TestMetadata(id='TheMaster/TheBot/Suite2').put()
    test_aaa = graph_data.TestMetadata(
        id='TheMaster/TheBot/Suite1/aaa', has_rows=True).put()
    test_bbb = graph_data.TestMetadata(
        id='TheMaster/TheBot/Suite1/bbb', has_rows=True).put()
    test_ccc = graph_data.TestMetadata(
        id='TheMaster/TheBot/Suite1/ccc', has_rows=True).put()
    test_ddd = graph_data.TestMetadata(
        id='TheMaster/TheBot/Suite2/ddd', has_rows=True).put()
    anomaly_config.AnomalyConfig(id='1-Suite1-specific', config={'a': 10}).put()
    anomaly_config.AnomalyConfig(id='2-Suite1-general', config={'b': 20}).put()

    self.testapp.post('/edit_anomaly_configs', {
        'add-edit': 'edit',
        'edit-name': '1-Suite1-specific',
        'config': '{"a": 10}',
        'patterns': '*/*/Suite1/aaa',
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
    })
    self.ExecuteTaskQueueTasks(
        '/put_entities_task', edit_config_handler._TASK_QUEUE_NAME)
    self.testapp.post('/edit_anomaly_configs', {
        'add-edit': 'edit',
        'edit-name': '2-Suite1-general',
        'config': '{"b": 20}',
        'patterns': '*/*/Suite1/*',
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
    })
    self.ExecuteDeferredTasks('default')
    self.ExecuteTaskQueueTasks(
        '/put_entities_task', edit_config_handler._TASK_QUEUE_NAME)

    # The lists of test patterns in the AnomalyConfig entities in the datastore
    # should be set based on what was added in the two requests above.
    self.assertEqual(
        ['*/*/Suite1/*'],
        anomaly_config.AnomalyConfig.get_by_id('2-Suite1-general').patterns)
    self.assertEqual(
        ['*/*/Suite1/aaa'],
        anomaly_config.AnomalyConfig.get_by_id('1-Suite1-specific').patterns)

    # The 1-Suite1-specific config applies instead of the other config
    # because its name comes first according to sort order.
    self.assertEqual(
        '1-Suite1-specific',
        test_aaa.get().overridden_anomaly_config.string_id())
    # The 2-Suite1-specific config applies to the other tests under Suite1.
    self.assertEqual(
        '2-Suite1-general',
        test_bbb.get().overridden_anomaly_config.string_id())
    self.assertEqual(
        '2-Suite1-general',
        test_ccc.get().overridden_anomaly_config.string_id())

    # Note that Suite2/ddd has no config, and nor do the parent tests.
    self.assertIsNone(test_ddd.get().overridden_anomaly_config)
    self.assertIsNone(suite1.get().overridden_anomaly_config)
    self.assertIsNone(suite2.get().overridden_anomaly_config)

  def testEdit_RemovePattern(self):
    """Tests removing a pattern from an AnomalyConfig."""
    self.SetCurrentUser('sullivan@chromium.org', is_admin=True)
    anomaly_config_key = anomaly_config.AnomalyConfig(
        id='Test Config', config={'a': 10},
        patterns=['*/*/one', '*/*/two']).put()
    master = graph_data.Master(id='TheMaster').put()
    graph_data.Bot(id='TheBot', parent=master).put()
    test_one = graph_data.TestMetadata(
        id='TheMaster/TheBot/one', overridden_anomaly_config=anomaly_config_key,
        has_rows=True).put()
    test_two = graph_data.TestMetadata(
        id='TheMaster/TheBot/two', overridden_anomaly_config=anomaly_config_key,
        has_rows=True).put()

    # Verify the state of the data before making the request.
    self.assertEqual(['*/*/one', '*/*/two'], anomaly_config_key.get().patterns)
    self.assertEqual(
        ['TheMaster/TheBot/one'],
        list_tests.GetTestsMatchingPattern('*/*/one'))

    self.testapp.post('/edit_anomaly_configs', {
        'add-edit': 'edit',
        'edit-name': 'Test Config',
        'config': '{"a": 10}',
        'patterns': ['*/*/two'],
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
    })
    self.ExecuteDeferredTasks('default')
    self.ExecuteTaskQueueTasks(
        '/put_entities_task', edit_config_handler._TASK_QUEUE_NAME)

    self.assertEqual(['*/*/two'], anomaly_config_key.get().patterns)
    self.assertIsNone(test_one.get().overridden_anomaly_config)
    self.assertEqual(
        'Test Config', test_two.get().overridden_anomaly_config.string_id())


if __name__ == '__main__':
  unittest.main()
