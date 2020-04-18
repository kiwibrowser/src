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
from dashboard.api import bugs
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import try_job


GOOGLER_USER = users.User(email='sullivan@chromium.org',
                          _auth_domain='google.com')
NON_GOOGLE_USER = users.User(email='foo@bar.com', _auth_domain='bar.com')


class MockIssueTrackerService(object):
  """A fake version of IssueTrackerService that returns expected data."""
  def __init__(self, http=None):
    pass

  @classmethod
  def GetIssue(cls, _):
    return {
        'cc': [
            {
                'kind': 'monorail#issuePerson',
                'htmlLink': 'https://bugs.chromium.org/u/1253971105',
                'name': 'user@chromium.org',
            }, {
                'kind': 'monorail#issuePerson',
                'name': 'hello@world.org',
            }
        ],
        'labels': [
            'Type-Bug',
            'Pri-3',
            'M-61',
        ],
        'owner': {
            'kind': 'monorail#issuePerson',
            'htmlLink': 'https://bugs.chromium.org/u/49586776',
            'name': 'owner@chromium.org',
        },
        'id': 737355,
        'author': {
            'kind': 'monorail#issuePerson',
            'htmlLink': 'https://bugs.chromium.org/u/49586776',
            'name': 'author@chromium.org',
        },
        'state': 'closed',
        'status': 'Fixed',
        'summary': 'The bug title',
        'components': [
            'Blink>ServiceWorker',
            'Foo>Bar',
        ],
        'published': '2017-06-28T01:26:53',
        'updated': '2018-03-01T16:16:22',
    }

  @classmethod
  def GetIssueComments(cls, _):
    return [{
        'content': 'Comment one',
        'published': '2017-06-28T04:42:55',
        'author': 'comment-one-author@company.com',
    }, {
        'content': 'Comment two',
        'published': '2017-06-28T10:16:14',
        'author': 'author-two@chromium.org'
    }]


class BugsTest(testing_common.TestCase):

  def setUp(self):
    super(BugsTest, self).setUp()
    app = webapp2.WSGIApplication(
        [(r'/api/bugs/(.*)', bugs.BugsHandler)])
    self.testapp = webtest.TestApp(app)
    # Add a fake issue tracker service that we can get call values from.
    self.original_service = bugs.issue_tracker_service.IssueTrackerService
    bugs.issue_tracker_service = mock.MagicMock()
    self.service = MockIssueTrackerService
    bugs.issue_tracker_service.IssueTrackerService = self.service

  def tearDown(self):
    super(BugsTest, self).tearDown()
    bugs.issue_tracker_service.IssueTrackerService = self.original_service

  def _SetGooglerOAuth(self, mock_oauth):
    mock_oauth.get_current_user.return_value = GOOGLER_USER
    mock_oauth.get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])

  @mock.patch.object(utils, 'ServiceAccountHttp', mock.MagicMock())
  @mock.patch.object(utils, 'IsGroupMember')
  @mock.patch.object(api_auth, 'oauth')
  def testPost_WithValidBug_ShowsData(self, mock_oauth, mock_utils):
    self._SetGooglerOAuth(mock_oauth)
    mock_utils.return_value = True

    try_job.TryJob(
        bug_id=123456, status='started', bot='win_perf',
        results_data={}, config='config = {"command": "cmd"}',
        last_ran_timestamp=datetime.datetime(2017, 01, 01)).put()
    try_job.TryJob(
        bug_id=123456, status='failed', bot='android_bisect',
        results_data={'metric': 'foo'},
        config='config = {"command": "cmd"}',
        last_ran_timestamp=datetime.datetime(2017, 01, 01)).put()
    try_job.TryJob(
        bug_id=99999, status='failed', bot='win_perf',
        results_data={'metric': 'foo'},
        config='config = {"command": "cmd"}',
        last_ran_timestamp=datetime.datetime(2017, 01, 01)).put()
    response = self.testapp.post('/api/bugs/123456')
    bug = self.GetJsonValue(response, 'bug')
    self.assertEqual('The bug title', bug.get('summary'))
    self.assertEqual(2, len(bug.get('cc')))
    self.assertEqual('hello@world.org', bug.get('cc')[1])
    self.assertEqual('Fixed', bug.get('status'))
    self.assertEqual('closed', bug.get('state'))
    self.assertEqual('author@chromium.org', bug.get('author'))
    self.assertEqual('owner@chromium.org', bug.get('owner'))
    self.assertEqual('2017-06-28T01:26:53', bug.get('published'))
    self.assertEqual('2018-03-01T16:16:22', bug.get('updated'))
    self.assertEqual(2, len(bug.get('comments')))
    self.assertEqual('Comment two', bug.get('comments')[1].get('content'))
    self.assertEqual(
        'author-two@chromium.org', bug.get('comments')[1].get('author'))
    self.assertEqual(2, len(bug.get('legacy_bisects')))
    self.assertEqual('started', bug.get('legacy_bisects')[0].get('status'))
    self.assertEqual('cmd', bug.get('legacy_bisects')[0].get('command'))
    self.assertEqual('2017-01-01T00:00:00', bug.get('legacy_bisects')[0].get(
        'started_timestamp'))

  @mock.patch.object(utils, 'ServiceAccountHttp', mock.MagicMock())
  @mock.patch.object(utils, 'IsGroupMember')
  @mock.patch.object(api_auth, 'oauth')
  def testPost_WithInvalidBugIdParameter_ShowsError(
      self, mock_oauth, mock_utils):
    mock_utils.return_value = True
    self._SetGooglerOAuth(mock_oauth)
    response = self.testapp.post('/api/bugs/foo', status=400)
    self.assertIn('Invalid bug ID \\"foo\\".', response.body)

  @mock.patch.object(utils, 'ServiceAccountHttp', mock.MagicMock())
  @mock.patch.object(utils, 'IsGroupMember')
  @mock.patch.object(api_auth, 'oauth')
  def testPost_NoAccess_ShowsError(
      self, mock_oauth, mock_utils):
    mock_utils.return_value = False
    mock_oauth.get_current_user.return_value = NON_GOOGLE_USER
    mock_oauth.get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])
    response = self.testapp.post('/api/bugs/foo', status=400)
    self.assertIn('No access', response.body)

  @mock.patch.object(api_auth, 'oauth')
  def testPost_NoOauthUser(self, mock_oauth):
    mock_oauth.get_current_user.return_value = None
    mock_oauth.get_client_id.return_value = (
        api_auth.OAUTH_CLIENT_ID_WHITELIST[0])
    self.testapp.post('/api/bugs/12345', status=401)

  @mock.patch.object(api_auth, 'oauth')
  def testPost_BadOauthClientId(self, mock_oauth):
    mock_oauth.get_current_user.return_value = GOOGLER_USER
    mock_oauth.get_client_id.return_value = 'invalid'
    self.testapp.post('/api/bugs/12345', status=403)


if __name__ == '__main__':
  unittest.main()
