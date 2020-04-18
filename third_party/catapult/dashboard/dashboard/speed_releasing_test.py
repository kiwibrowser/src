# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import webapp2
import webtest

from google.appengine.ext import ndb

from dashboard import speed_releasing
from dashboard.common import datastore_hooks
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import graph_data
from dashboard.models import sheriff
from dashboard.models import table_config

_SAMPLE_BOTS = ['ChromiumPerf/win', 'ChromiumPerf/linux']
_DOWNSTREAM_BOTS = ['ClankInternal/win', 'ClankInternal/linux']
_SAMPLE_TESTS = ['my_test_suite/my_test', 'my_test_suite/my_other_test']
_SAMPLE_LAYOUT = ('{ "my_test_suite/my_test": ["Foreground", '
                  '"Pretty Name 1"],"my_test_suite/my_other_test": '
                  ' ["Foreground", "Pretty Name 2"]}')


RECENT_REV = speed_releasing.CHROMIUM_MILESTONES[
    speed_releasing.CURRENT_MILESTONE][0] + 42


class SpeedReleasingTest(testing_common.TestCase):

  def setUp(self):
    super(SpeedReleasingTest, self).setUp()
    app = webapp2.WSGIApplication([(
        r'/speed_releasing/(.*)',
        speed_releasing.SpeedReleasingHandler)])
    self.testapp = webtest.TestApp(app)
    testing_common.SetSheriffDomains(['chromium.org'])
    testing_common.SetIsInternalUser('internal@chromium.org', True)
    self.SetCurrentUser('internal@chromium.org', is_admin=True)

  def tearDown(self):
    super(SpeedReleasingTest, self).tearDown()
    self.UnsetCurrentUser()

  def _AddInternalBotsToDataStore(self):
    """Adds sample bot/master pairs."""
    master_key = ndb.Key('Master', 'ChromiumPerf')
    graph_data.Bot(
        id='win', parent=master_key, internal_only=True).put()
    graph_data.Bot(
        id='linux', parent=master_key, internal_only=True).put()

  def _AddPublicBotsToDataStore(self):
    """Adds sample bot/master pairs."""
    master_key = ndb.Key('Master', 'ChromiumPerf')
    graph_data.Bot(
        id='win', parent=master_key, internal_only=False).put()
    graph_data.Bot(
        id='linux', parent=master_key, internal_only=False).put()

  def _AddTableConfigDataStore(self, name, is_internal, is_downstream=False):
    """Add sample internal only tableConfig."""
    keys = self._AddTests(is_downstream)
    if is_internal:
      self._AddInternalBotsToDataStore()
    else:
      self._AddPublicBotsToDataStore()
    table_config.CreateTableConfig(
        name=name, bots=_SAMPLE_BOTS if not is_downstream else _DOWNSTREAM_BOTS,
        tests=_SAMPLE_TESTS,
        layout=_SAMPLE_LAYOUT,
        username='internal@chromium.org',
        override=0)
    return keys

  def _AddSheriffToDatastore(self):
    sheriff_key = sheriff.Sheriff(
        id='Chromium Perf Sheriff', email='internal@chromium.org')
    sheriff_key.patterns = ['*/*/my_test_suite/*']
    sheriff_key.put()

  def _AddTests(self, is_downstream):
    master = 'ClankInternal' if is_downstream else 'ChromiumPerf'
    testing_common.AddTests([master], ['win', 'linux'], {
        'my_test_suite': {
            'my_test': {},
            'my_other_test': {},
        },
    })
    keys = [
        utils.TestKey(master + '/win/my_test_suite/my_test'),
        utils.TestKey(master + '/win/my_test_suite/my_other_test'),
        utils.TestKey(master + '/linux/my_test_suite/my_test'),
        utils.TestKey(master + '/linux/my_test_suite/my_other_test'),
    ]
    for test_key in keys:
      test = test_key.get()
      test.units = 'timeDurationInMs'
      test.put()
    return keys

  def _AddAlertsWithDifferentMasterAndBenchmark(self):
    """Adds 10 alerts with different benchmark/master."""
    sheriff_key = sheriff.Sheriff(
        id='Fake Sheriff', email='internal@chromium.org')
    sheriff_key.patterns = ['*/*/my_fake_suite/*']
    sheriff_key.put()
    master = 'FakeMaster'
    testing_common.AddTests([master], ['win'], {
        'my_fake_suite': {
            'my_fake_test': {},
        },
    })
    keys = [
        utils.TestKey(master + '/win/my_fake_suite/my_fake_test'),
    ]
    self._AddRows(keys)
    self._AddAlertsToDataStore(keys)

  def _AddAlertsToDataStore(self, test_keys):
    """Adds sample data, including triaged and non-triaged alerts."""
    key_map = {}
    sheriff_key = ndb.Key('Sheriff', 'Chromium Perf Sheriff')
    for test_key in test_keys:
      test = test_key.get()
      test.improvement_direction = anomaly.DOWN
      test.put()

    # Add some (10 * len(keys)) non-triaged alerts.
    for end_rev in xrange(420500, 421500, 100):
      for test_key in test_keys:
        ref_test_key = utils.TestKey('%s_ref' % utils.TestPath(test_key))
        anomaly_entity = anomaly.Anomaly(
            start_revision=end_rev - 5, end_revision=end_rev, test=test_key,
            median_before_anomaly=100, median_after_anomaly=200,
            ref_test=ref_test_key, sheriff=sheriff_key)
        anomaly_entity.SetIsImprovement()
        anomaly_key = anomaly_entity.put()
        key_map[end_rev] = anomaly_key

    return key_map


  def _AddRows(self, keys):
    for key in keys:
      testing_common.AddRows(utils.TestPath(key), [1, 2, 3, RECENT_REV])

  def _AddDownstreamRows(self, keys):
    revisions = [1, 2, 1485025126, 1485099999]
    for key in keys:
      testing_common.AddRows(
          utils.TestPath(key), revisions)
    for key in keys:
      for rev in revisions:
        row_key = utils.GetRowKey(key, rev)
        row = row_key.get()
        row.r_commit_pos = str(rev / 10000)
        row.a_default_rev = 'r_foo'
        row.r_foo = 'abcdefghijk'
        row.put()

  def testGet_ShowPage(self):
    response = self.testapp.get('/speed_releasing/')
    self.assertIn('speed-releasing-page', response)

  def testPost_InternalListPage(self):
    self._AddTableConfigDataStore('BestTable', True)
    self._AddTableConfigDataStore('SecondBestTable', True)
    self._AddTableConfigDataStore('ThirdBestTable', False)
    response = self.testapp.post('/speed_releasing/')
    self.assertIn('"show_list": true', response)
    self.assertIn('"list": ["BestTable", "SecondBestTable", '
                  '"ThirdBestTable"]', response)

  def testPost_ShowInternalTable(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?revA=1&revB=2')
    self.assertIn('"name": "BestTable"', response)
    self.assertIn('"table_bots": ["ChromiumPerf/win", '
                  '"ChromiumPerf/linux"]', response)
    self.assertIn('"table_tests": ["my_test_suite/my_test",'
                  ' "my_test_suite/my_other_test"]', response)
    self.assertIn('"table_layout"', response)
    self.assertIn('"revisions": [2, 1]', response)
    self.assertIn('"display_revisions": [2, 1]', response)
    self.assertIn('"units": {"my_test_suite/my_test": "timeDurationInMs", '
                  '"my_test_suite/my_other_test": "timeDurationInMs"',
                  response)
    self.assertIn('"categories": {"Foreground": 2}', response)
    self.assertIn('"values": {"1": {"ChromiumPerf/linux": '
                  '{"my_test_suite/my_test": 1.0, '
                  '"my_test_suite/my_other_test": 1.0}, '
                  '"ChromiumPerf/win": {"my_test_suite/my_test": 1.0, '
                  '"my_test_suite/my_other_test": 1.0}}, '
                  '"2": {"ChromiumPerf/linux": {"my_test_suite/my_test": '
                  '2.0, "my_test_suite/my_other_test": 2.0}, '
                  '"ChromiumPerf/win": {"my_test_suite/my_test": 2.0, '
                  '"my_test_suite/my_other_test": 2.0}}}', response)
    self.assertIn('"urls": {"ChromiumPerf/linux/my_test_suite/my_other_test": '
                  '"?masters=ChromiumPerf&start_rev=1&checked=my_other_test&'
                  'tests=my_test_suite%2Fmy_other_test&end_rev=2&bots=linux", '
                  '"ChromiumPerf/win/my_test_suite/my_other_test": '
                  '"?masters=ChromiumPerf&start_rev=1&checked=my_other_test&'
                  'tests=my_test_suite%2Fmy_other_test&end_rev=2&bots=win", '
                  '"ChromiumPerf/linux/my_test_suite/my_test": "?masters'
                  '=ChromiumPerf&start_rev=1&checked=my_test&tests='
                  'my_test_suite%2Fmy_test&end_rev=2&bots=linux", '
                  '"ChromiumPerf/win/my_test_suite/my_test": "?masters='
                  'ChromiumPerf&start_rev=1&checked=my_test&tests=my_test_suite'
                  '%2Fmy_test&end_rev=2&bots=win"}',
                  response)

  def testPost_InternalListPageToExternalUser(self):
    self._AddTableConfigDataStore('BestTable', True)
    self._AddTableConfigDataStore('SecondBestTable', True)
    self._AddTableConfigDataStore('ThirdBestTable', False)
    self.UnsetCurrentUser()
    datastore_hooks.InstallHooks()
    response = self.testapp.post('/speed_releasing/')
    self.assertIn('"show_list": true', response)
    self.assertIn('"list": ["ThirdBestTable"]', response)

  def testPost_ShowInternalTableToExternalUser(self):
    self._AddTableConfigDataStore('BestTable', True)
    self.UnsetCurrentUser()
    self.testapp.post('/speed_releasing/BestTable?revA=1&revB=2', {
    }, status=500) # 500 means user can't see data.

  def testPost_TableWithTableNameThatDoesntExist(self):
    response = self.testapp.post('/speed_releasing/BestTable')
    self.assertIn('Invalid table name.', response)

  def testPost_TableWithNoRevParamsOnlyDownStream(self):
    keys = self._AddTableConfigDataStore('BestTable', True, True)
    self._AddDownstreamRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?revA=1485099999&'
                                 'revB=1485025126')
    self.assertIn('"revisions": [1485099999, 1485025126]', response)
    self.assertIn('"display_revisions": ["148509-abc", "148502-abc"]', response)

  def testPost_TableWithMilestoneParam(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?m=56')
    self.assertIn('"revisions": [445288, 433400]', response)
    self.assertIn('"display_milestones": [56, 56]', response)
    self.assertIn('"navigation_milestones": [55, 57]', response)

  def testPost_TableWithNewestMilestoneParam(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    current_milestone = speed_releasing.CURRENT_MILESTONE
    response = self.testapp.post('/speed_releasing/BestTable?m=%s' %
                                 current_milestone)
    current_milestone_start_rev = speed_releasing.CHROMIUM_MILESTONES[
        current_milestone][0]
    self.assertIn(
        '"revisions": [%s, %s]' % (
            RECENT_REV, current_milestone_start_rev), response)
    self.assertIn(
        '"display_milestones": [%s, %s]' % (
            current_milestone, current_milestone), response)
    self.assertIn(
        '"navigation_milestones": [%s, null]' % (
            current_milestone - 1), response)

  def testPost_TableWithHighMilestoneParam(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?m=71')
    self.assertIn('"error": "No data for that milestone."', response)

  def testPost_TableWithLowMilestoneParam(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?m=7')
    self.assertIn('"error": "No data for that milestone."', response)

  def testPost_TableWithNoRevParams(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable')
    current_milestone_start_rev = speed_releasing.CHROMIUM_MILESTONES[
        speed_releasing.CURRENT_MILESTONE][0]
    self.assertIn(
        '"revisions": [%s, %s]' % (
            RECENT_REV, current_milestone_start_rev), response)

  def testPost_TableWithRevParamEndRevAlsoStartRev(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?revA=433400')
    self.assertIn('"revisions": [445288, 433400]', response)

  def testPost_TableWithOneRevParamUniqueEndRev(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?revA=423768')
    self.assertIn('"revisions": [423768, 416640]', response)
    self.assertIn('"display_milestones": [54, 54]', response)
    self.assertIn('"navigation_milestones": [null, 55]', response)

  def testPost_TableWithOneRevParamBetweenMilestones(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?revB=455000')
    self.assertIn('"revisions": [463842, 455000]', response)
    self.assertIn('"display_milestones": [58, 58]', response)

  def testPost_TableWithRevParamMiddleRev(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?revB=444000')
    self.assertIn('"revisions": [445288, 444000]', response)
    self.assertIn('"display_milestones": [56, 56]', response)

  def testPost_TableWithRevParamHighRev(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?revB=50000000')
    self.assertIn('"revisions": [50000000, %s]' % RECENT_REV, response)
    self.assertIn('"display_milestones": [%s, %s]' % ((
        speed_releasing.CURRENT_MILESTONE,)*2), response)

  def testPost_TableWithRevParamLowRev(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?revB=1')
    self.assertIn('"revisions": [%s, 1]' % RECENT_REV, response)
    self.assertIn('"display_milestones": [%s, %s]' % ((
        speed_releasing.CURRENT_MILESTONE,)*2), response)

  def testPost_TableWithRevsParamTwoMilestones(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?'
                                 'revA=417000&revB=440000')
    self.assertIn('"revisions": [440000, 417000]', response)
    self.assertIn('"display_milestones": [54, 56]', response)
    self.assertIn('"navigation_milestones": [null, 56]', response)

  def testPost_TableWithRevsParamHigh(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?'
                                 'revA=50000000&revB=60000000')
    self.assertIn('"revisions": [60000000, 50000000]', response)
    self.assertIn('"display_milestones": [%s, %s]' % ((
        speed_releasing.CURRENT_MILESTONE,)*2), response)

  def testPost_TableWithRevsParamSelfContained(self):
    keys = self._AddTableConfigDataStore('BestTable', True)
    self._AddRows(keys)
    response = self.testapp.post('/speed_releasing/BestTable?'
                                 'revB=420000&revA=421000')
    self.assertIn('"revisions": [421000, 420000]', response)
    self.assertIn('"display_milestones": [54, 54]', response)

  def testPost_ReleaseNotes(self):
    keys = self._AddTableConfigDataStore('BestTable', True, False)
    self._AddSheriffToDatastore()
    self._AddRows(keys)
    self._AddAlertsToDataStore(keys)
    self._AddAlertsWithDifferentMasterAndBenchmark()
    response = self.testapp.post('/speed_releasing/BestTable?'
                                 'revB=420000&revA=421000&anomalies=true')
    self.assertIn('"revisions": [421000, 420000]', response)
    # Make sure we aren't getting a table here instead of Release Notes.
    self.assertNotIn('"display_revisions"', response)

    # There are 50 anomalies total (5 tests on 10 revisions). 1 test does not
    # have the correct master/benchmark, so 4 valid tests. Further, the
    # revisions are [420500:421500:100] meaning that there are 6 revisions in
    # the url param's range. 6*4 = 24 anomalies that should be returned.
    anomaly_list = self.GetJsonValue(response, 'anomalies')
    self.assertEqual(len(anomaly.Anomaly.query().fetch()), 50)
    self.assertEqual(len(anomaly_list), 24)
    for alert in anomaly_list:
      self.assertEqual(alert['master'], 'ChromiumPerf')
      self.assertIn(alert['test'], ['my_test', 'my_other_test'])
      self.assertGreaterEqual(alert['end_revision'], 420000)
      self.assertLessEqual(alert['end_revision'], 421000)
