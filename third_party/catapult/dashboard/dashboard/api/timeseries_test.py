# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import mock
import unittest

import webapp2
import webtest

from google.appengine.api import users

from dashboard.api import api_auth
from dashboard.api import timeseries
from dashboard.common import datastore_hooks
from dashboard.common import testing_common
from dashboard.common import utils


GOOGLER_USER = users.User(email='sullivan@chromium.org',
                          _auth_domain='google.com')
NON_GOOGLE_USER = users.User(email='foo@bar.com', _auth_domain='bar.com')


class TimeseriesTest(testing_common.TestCase):

  def setUp(self):
    super(TimeseriesTest, self).setUp()
    app = webapp2.WSGIApplication(
        [(r'/api/timeseries/(.*)', timeseries.TimeseriesHandler)])
    self.testapp = webtest.TestApp(app)

  def _AddData(self):
    """Adds sample TestMetadata entities and returns their keys."""
    testing_common.AddTests(['ChromiumPerf'], ['linux'], {
        'page_cycler': {'warm': {'cnn': {},}}
    })

    now = datetime.datetime.now()
    last_week = now - datetime.timedelta(days=7)
    rows = dict([(i * 100, {
        'value': i * 1000,
        'a_whatever': 'blah',
        'r_v8': '1234a',
        'timestamp': now if i > 5 else last_week,
        'error': 3.3232
    }) for i in range(1, 10)])
    rows[100]['r_not_every_row'] = 12345
    testing_common.AddRows('ChromiumPerf/linux/page_cycler/warm/cnn', rows)

  @mock.patch.object(utils, 'IsGroupMember')
  @mock.patch.object(api_auth, 'oauth')
  def testPost_TestPath_ReturnsInternalData(self, mock_oauth, mock_utils):
    mock_oauth.get_current_user.return_value = GOOGLER_USER
    mock_oauth.get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])
    mock_utils.return_value = True
    self._AddData()
    datastore_hooks.InstallHooks()
    test = utils.TestKey('ChromiumPerf/linux/page_cycler/warm/cnn').get()
    test.internal_only = True
    test.put()

    response = self.testapp.post(
        '/api/timeseries/ChromiumPerf/linux/page_cycler/warm/cnn')
    data = self.GetJsonValue(response, 'timeseries')
    self.assertEquals(10, len(data))
    self.assertEquals(
        ['revision', 'value', 'timestamp', 'r_not_every_row', 'r_v8'], data[0])
    self.assertEquals(100, data[1][0])
    self.assertEquals(900, data[9][0])
    self.assertEquals('1234a', data[1][4])

  @mock.patch.object(api_auth, 'oauth')
  def testPost_NumDays_ChecksTimestamp(self, mock_oauth):
    mock_oauth.get_current_user.return_value = GOOGLER_USER
    mock_oauth.get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])
    self._AddData()

    response = self.testapp.post(
        '/api/timeseries/ChromiumPerf/linux/page_cycler/warm/cnn',
        {'num_days': 1})
    data = self.GetJsonValue(response, 'timeseries')
    self.assertEquals(5, len(data))
    self.assertEquals(['revision', 'value', 'timestamp', 'r_v8'], data[0])
    self.assertEquals(600, data[1][0])
    self.assertEquals(900, data[4][0])
    self.assertEquals('1234a', data[1][3])

  @mock.patch.object(api_auth, 'oauth')
  def testPost_NumDaysNotNumber_400Response(self, mock_oauth):
    mock_oauth.get_current_user.return_value = GOOGLER_USER
    mock_oauth.get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])

    response = self.testapp.post(
        '/api/timeseries/ChromiumPerf/linux/page_cycler/warm/cnn',
        {'num_days': 'foo'}, status=400)
    self.assertIn('Invalid num_days parameter', response.body)

  @mock.patch.object(api_auth, 'oauth')
  def testPost_NegativeNumDays_400Response(self, mock_oauth):
    mock_oauth.get_current_user.return_value = GOOGLER_USER
    mock_oauth.get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])

    response = self.testapp.post(
        '/api/timeseries/ChromiumPerf/linux/page_cycler/warm/cnn',
        {'num_days': -1}, status=400)
    self.assertIn('num_days cannot be negative', response.body)

  @mock.patch.object(api_auth, 'oauth')
  def testPost_ExternalUserInternalData_500Error(self, mock_oauth):
    mock_oauth.get_current_user.return_value = NON_GOOGLE_USER
    mock_oauth.get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])
    self._AddData()
    datastore_hooks.InstallHooks()
    test = utils.TestKey('ChromiumPerf/linux/page_cycler/warm/cnn').get()
    test.internal_only = True
    test.put()

    self.testapp.post(
        '/api/timeseries/ChromiumPerf/linux/page_cycler/warm/cnn', status=500)


if __name__ == '__main__':
  unittest.main()
