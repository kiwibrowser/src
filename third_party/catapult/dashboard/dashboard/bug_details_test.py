# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import unittest

import mock
import webapp2
import webtest

# pylint: disable=unused-import
from dashboard import mock_oauth2_decorator
# pylint: enable=unused-import

from dashboard import bug_details
from dashboard.common import testing_common
from dashboard.models import try_job
from dashboard.services import issue_tracker_service


GET_ISSUE_DATA = {
    'owner': {'name': 'sullivan@chromium.org'},
    'state': 'open',
    'status': 'Untriaged',
    'summary': 'Regression in sunspider',
    'published': '2017-02-17T23:08:44',
}

GET_COMMENTS_DATA = [
    {
        'author': 'foo@chromium.org',
        'content': 'This is the first comment',
        'published': '2017-02-17T09:59:55',
    }, {
        'author': 'bar@chromium.org',
        'content': 'This is the second comment',
        'published': '2017-02-17T10:00:0',
    }, {
        'author': 'bugdroid1@chromium.org',
        'content': 'The following revision refers to this bug:\n'
                   '  https://chromium.googlesource.com/chromium/src.git/+/'
                   '9ac6e6466cc0df7e1a3ad4488c5c8bdc2db4da36\n\n'
                   'Review-Url: https://codereview.chromium.org/2707483002\n\n',
        'published': '2017-02-17T23:08:44',
    }
]


class BugDetailsHandlerTest(testing_common.TestCase):

  def setUp(self):
    super(BugDetailsHandlerTest, self).setUp()
    app = webapp2.WSGIApplication([(
        '/bug_details', bug_details.BugDetailsHandler)])
    self.testapp = webtest.TestApp(app)

  # Mocks fetching bugs from issue tracker.
  @unittest.skipIf(sys.platform.startswith('linux'), 'oauth2 mock error')
  @mock.patch('services.issue_tracker_service.discovery.build',
              mock.MagicMock())
  @mock.patch.object(
      issue_tracker_service.IssueTrackerService, 'GetIssue',
      mock.MagicMock(return_value=GET_ISSUE_DATA))
  @mock.patch.object(
      issue_tracker_service.IssueTrackerService, 'GetIssueComments',
      mock.MagicMock(return_value=GET_COMMENTS_DATA))
  def testPost(self):
    try_job.TryJob(
        bug_id=12345, status='started', bot='win_perf',
        results_data={}).put()
    try_job.TryJob(
        bug_id=12345, status='failed', bot='android_bisect',
        results_data={'metric': 'foo'}).put()
    try_job.TryJob(
        bug_id=99999, status='failed', bot='win_perf',
        results_data={'metric': 'foo'}).put()
    response = self.testapp.post('/bug_details', {'bug_id': '12345'})
    self.assertEqual(
        'Regression in sunspider',
        self.GetJsonValue(response, 'summary'))
    self.assertEqual(
        'sullivan@chromium.org',
        self.GetJsonValue(response, 'owner'))
    self.assertEqual(
        '2017-02-17T23:08:44',
        self.GetJsonValue(response, 'published'))
    self.assertEqual(
        'open',
        self.GetJsonValue(response, 'state'))
    self.assertEqual(
        'Untriaged',
        self.GetJsonValue(response, 'status'))
    comments = self.GetJsonValue(response, 'comments')
    self.assertEqual(3, len(comments))
    self.assertEqual('This is the second comment', comments[1]['content'])
    self.assertItemsEqual(
        ['https://codereview.chromium.org/2707483002'],
        self.GetJsonValue(response, 'review_urls'))
    bisects = self.GetJsonValue(response, 'bisects')
    self.assertEqual(2, len(bisects))
    self.assertEqual('started', bisects[0]['status'])
    self.assertIsNone(bisects[0]['metric'])
    self.assertEqual('failed', bisects[1]['status'])
    self.assertEqual('foo', bisects[1]['metric'])
    self.assertEqual('android_bisect', bisects[1]['bot'])


if __name__ == '__main__':
  unittest.main()
