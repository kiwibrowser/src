# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import webapp2
import webtest

from google.appengine.ext import ndb
from google.appengine.api import users

from dashboard import create_health_report
from dashboard.common import testing_common
from dashboard.common import xsrf
from dashboard.models import table_config
from dashboard.models import graph_data


# Sample table config that contains all the required fields.
_SAMPLE_TABLE_CONFIG = {
    'tableName': 'my_sample_config',
    'tableBots': 'ChromiumPerf/win\nChromiumPerf/linux',
    'tableTests': 'my_test_suite/my_test\nmy_test_suite/my_other_test',
    'tableLayout': '{"my_test_suite/my_test": ["Foreground", '
                   '"Pretty Name 1"], "my_test_suite/my_other_test": '
                   '["Foreground", "Pretty Name 2"]}',
    'override': '0',
}

_ALT_SAMPLE_TABLE_CONFIG = {
    'tableName': 'my_other_config',
    'tableBots': 'ChromiumPerf/win\nChromiumPerf/linux',
    'tableTests': 'my_test_suite/my_test\nmy_test_suite/my_other_test',
    'tableLayout': '{"my_test_suite/my_test": ["Foreground", '
                   '"Pretty Name 1"], "my_test_suite/my_other_test": '
                   '["Foreground", "Pretty Name 2"]}',
    'override': '0',
}

class CreateHealthReportTest(testing_common.TestCase):

  def setUp(self):
    super(CreateHealthReportTest, self).setUp()
    app = webapp2.WSGIApplication([(
        '/create_health_report',
        create_health_report.CreateHealthReportHandler)])
    self.testapp = webtest.TestApp(app)
    testing_common.SetSheriffDomains(['chromium.org'])
    testing_common.SetIsInternalUser('internal@chromium.org', True)
    self.SetCurrentUser('internal@chromium.org', is_admin=True)

  def tearDown(self):
    super(CreateHealthReportTest, self).tearDown()
    self.UnsetCurrentUser()

  def _AddInternalBotsToDataStore(self):
    """Adds sample bot/master pairs."""
    self._AddTests()
    bots = graph_data.Bot.query().fetch()
    for bot in bots:
      bot.internal_only = True
      bot.put()

  def _AddMixedBotsToDataStore(self):
    """Adds sample bot/master pairs."""
    self._AddTests()
    bots = graph_data.Bot.query().fetch()
    bots[1].internal_only = True
    bots[1].put()

  def _AddPublicBotsToDataStore(self):
    """Adds sample bot/master pairs."""
    self._AddTests()

  def _AddTests(self):
    testing_common.AddTests(['ChromiumPerf'], ['win', 'linux'], {
        'my_test_suite': {
            'my_test': {},
            'my_other_test': {},
        }
    })

  def testPost_NoXSRFToken_Returns403Error(self):
    self.testapp.post('/create_health_report', {
    }, status=403)
    query = table_config.TableConfig.query()
    table_values = query.fetch()
    self.assertEqual(len(table_values), 0)

  def testPost_GetXsrfToken(self):
    response = self.testapp.post('/create_health_report', {
        'getToken': 'true',
        })
    self.assertIn(xsrf.GenerateToken(users.get_current_user()), response)

  def testGet_ShowPage(self):
    response = self.testapp.get('/create_health_report')
    self.assertIn('create-health-report-page', response)

  def testPost_ExternalUserReturnsNotLoggedIn(self):
    self.UnsetCurrentUser()
    response = self.testapp.post('/create_health_report', {})
    self.assertIn('User not logged in.', response)

  def testPost_ValidData(self):
    self._AddInternalBotsToDataStore()
    config = copy.deepcopy(_SAMPLE_TABLE_CONFIG)
    config['xsrf_token'] = xsrf.GenerateToken(users.get_current_user())

    response = self.testapp.post('/create_health_report', config)
    self.assertIn('my_sample_config', response)
    table_entity = ndb.Key('TableConfig', 'my_sample_config').get()
    self.assertTrue(table_entity.internal_only)
    self.assertEqual('internal@chromium.org', table_entity.username)
    self.assertEqual(
        ['my_test_suite/my_test', 'my_test_suite/my_other_test'],
        table_entity.tests)
    master_key = ndb.Key('Master', 'ChromiumPerf')
    win_bot = graph_data.Bot(
        id='win', parent=master_key, internal_only=False).key
    linux_bot = graph_data.Bot(
        id='linux', parent=master_key, internal_only=False).key
    bots = [win_bot, linux_bot]
    self.assertEqual(bots, table_entity.bots)
    self.assertEqual(
        '{"my_test_suite/my_test": ["Foreground", "Pretty Name 1"], '
        '"my_test_suite/my_other_test": ["Foreground", "Pretty Name 2"]}',
        table_entity.table_layout)

  def testPost_EmptyForm(self):
    response = self.testapp.post('/create_health_report', {
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
        'override': 0,
        })
    self.assertIn('Please fill out the form entirely.', response)
    query = table_config.TableConfig.query()
    table_values = query.fetch()
    self.assertEqual(len(table_values), 0)

  def testPost_TwoPostsSameNameReturnsError(self):
    self._AddInternalBotsToDataStore()
    config = copy.deepcopy(_SAMPLE_TABLE_CONFIG)
    config['xsrf_token'] = xsrf.GenerateToken(users.get_current_user())

    self.testapp.post('/create_health_report', config)
    response = self.testapp.post('/create_health_report', config)
    self.assertIn('my_sample_config already exists.', response)

  def testPost_InvalidBots(self):
    self._AddInternalBotsToDataStore()
    response = self.testapp.post('/create_health_report', {
        'tableName': 'myName',
        'tableBots': 'garbage/moarGarbage',
        'tableTests': 'my_test_suite/my_test',
        'tableLayout': '{"Alayout":"isHere"}',
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
        'override': 0,
        })
    self.assertIn('Invalid Master/Bot: garbage/moarGarbage', response)
    query = table_config.TableConfig.query()
    table_values = query.fetch()
    self.assertEqual(len(table_values), 0)

  def testPost_InternalOnlyAndPublicBots(self):
    self._AddMixedBotsToDataStore()
    config = copy.deepcopy(_SAMPLE_TABLE_CONFIG)
    config['xsrf_token'] = xsrf.GenerateToken(users.get_current_user())

    self.testapp.post('/create_health_report', config)
    table_entity = ndb.Key('TableConfig', 'my_sample_config').get()
    self.assertTrue(table_entity.internal_only)

  def testPost_PublicOnlyBots(self):
    self._AddPublicBotsToDataStore()
    config = copy.deepcopy(_SAMPLE_TABLE_CONFIG)
    config['xsrf_token'] = xsrf.GenerateToken(users.get_current_user())

    self.testapp.post('/create_health_report', config)
    table_entity = ndb.Key('TableConfig', 'my_sample_config').get()
    self.assertFalse(table_entity.internal_only)

  def testPost_ValidBotsBadLayout(self):
    self._AddPublicBotsToDataStore()
    response = self.testapp.post('/create_health_report', {
        'tableName': 'myName',
        'tableBots': 'ChromiumPerf/linux',
        'tableTests': 'my_test_suite/my_test',
        'tableLayout': 'garbage',
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
        'override': 0,
        })
    self.assertIn('Invalid JSON for table layout', response)
    query = table_config.TableConfig.query()
    table_values = query.fetch()
    self.assertEqual(len(table_values), 0)

  def testPost_InvalidTests(self):
    self._AddInternalBotsToDataStore()
    response = self.testapp.post('/create_health_report', {
        'tableName': 'myName',
        'tableBots': 'ChromiumPerf/linux',
        'tableTests': 'someTests',
        'tableLayout': '{"Alayout":"isHere"}',
        'xsrf_token': xsrf.GenerateToken(users.get_current_user()),
        'override': 0,
        })
    self.assertIn('someTests is not a valid test.', response)
    query = table_config.TableConfig.query()
    table_values = query.fetch()
    self.assertEqual(len(table_values), 0)

  def testPost_GetTableConfigList(self):
    self._AddInternalBotsToDataStore()
    config = copy.deepcopy(_SAMPLE_TABLE_CONFIG)
    config['xsrf_token'] = xsrf.GenerateToken(users.get_current_user())

    alt_config = copy.deepcopy(_ALT_SAMPLE_TABLE_CONFIG)
    alt_config['xsrf_token'] = xsrf.GenerateToken(users.get_current_user())

    self.testapp.post('/create_health_report', config)
    self.testapp.post('/create_health_report', alt_config)

    response = self.testapp.post('/create_health_report', {
        'getTableConfigList': True,
        })
    return_list = self.GetJsonValue(response, 'table_config_list')
    query = table_config.TableConfig.query()
    all_configs = query.fetch(keys_only=True)
    for config in all_configs:
      self.assertIn(config.id(), return_list)

  def testPost_GetTableConfigDetailsForEdit(self):
    self._AddInternalBotsToDataStore()
    config = copy.deepcopy(_SAMPLE_TABLE_CONFIG)
    config['xsrf_token'] = xsrf.GenerateToken(users.get_current_user())

    self.testapp.post('/create_health_report', config)

    response = self.testapp.post('/create_health_report', {
        'getTableConfigDetails': 'my_sample_config',
        })
    # Similar to the valid data test, ensure everything is correct.
    self.assertIn('my_sample_config', response)
    table_entity = ndb.Key('TableConfig', 'my_sample_config').get()
    self.assertTrue(table_entity.internal_only)
    self.assertEqual('internal@chromium.org', table_entity.username)
    self.assertEqual(
        ['my_test_suite/my_test', 'my_test_suite/my_other_test'],
        table_entity.tests)
    master_key = ndb.Key('Master', 'ChromiumPerf')
    win_bot = graph_data.Bot(
        id='win', parent=master_key, internal_only=False).key
    linux_bot = graph_data.Bot(
        id='linux', parent=master_key, internal_only=False).key
    bots = [win_bot, linux_bot]
    self.assertEqual(bots, table_entity.bots)
    self.assertEqual(
        '{"my_test_suite/my_test": ["Foreground", "Pretty Name 1"], '
        '"my_test_suite/my_other_test": ["Foreground", "Pretty Name 2"]}',
        table_entity.table_layout)

  def testPost_TwoPostsSameNameAsEdit(self):
    self._AddInternalBotsToDataStore()
    config = copy.deepcopy(_SAMPLE_TABLE_CONFIG)
    config['xsrf_token'] = xsrf.GenerateToken(users.get_current_user())
    self.testapp.post('/create_health_report', config)

    config['override'] = 1
    config['tableLayout'] = (
        '{"my_test_suite/my_test": ["Foreground", "New Name 1"], '
        '"my_test_suite/my_other_test": ["Foreground", "New Name 2"]}')
    response = self.testapp.post('/create_health_report', config)

    self.assertIn('my_sample_config', response)
    self.assertNotIn('already exists.', response)
    table_entity = ndb.Key('TableConfig', 'my_sample_config').get()
    self.assertIn('my_test_suite/my_test": ["Foreground", "New Name 1"]',
                  table_entity.table_layout)
    self.assertIn('my_test_suite/my_other_test": ["Foreground", "New Name 2"]',
                  table_entity.table_layout)

