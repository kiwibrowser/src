# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json

import mock
import webapp2
import webtest

from dashboard.api import api_auth
from dashboard.common import namespaced_stored_object
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.services import gitiles_service
from dashboard.pinpoint.handlers import new
from dashboard.pinpoint.models import job as job_module


_BASE_REQUEST = {
    'target': 'telemetry_perf_tests',
    'configuration': 'chromium-rel-mac11-pro',
    'benchmark': 'speedometer',
    'bug_id': '12345',
    'start_git_hash': '1',
    'end_git_hash': '3',
}


# TODO: Make this agnostic to the parameters the Quests take.
_CONFIGURATION_ARGUMENTS = {
    'browser': 'release',
    'builder': 'Mac Builder',
    'dimensions': '{"key": "value"}',
    'repository': 'src',
    'swarming_server': 'https://chromium-swarm.appspot.com',
}


class _NewTest(testing_common.TestCase):

  def setUp(self):
    super(_NewTest, self).setUp()

    app = webapp2.WSGIApplication([
        webapp2.Route(r'/api/new', new.New),
    ])
    self.testapp = webtest.TestApp(app)

    self.SetCurrentUser('internal@chromium.org', is_admin=True)

    namespaced_stored_object.Set('bot_configurations', {
        'chromium-rel-mac11-pro': _CONFIGURATION_ARGUMENTS
    })
    namespaced_stored_object.Set('repositories', {
        'src': {'repository_url': 'http://src'},
    })


@mock.patch.object(gitiles_service, 'CommitInfo',
                   mock.MagicMock(return_value={'commit': 'abc'}))
class NewAuthTest(_NewTest):

  @mock.patch.object(api_auth, 'Authorize',
                     mock.MagicMock(side_effect=api_auth.NotLoggedInError()))
  def testPost_NotLoggedIn(self):
    response = self.testapp.post('/api/new', _BASE_REQUEST, status=401)
    result = json.loads(response.body)
    self.assertEqual(result, {'error': 'User not authenticated'})

  @mock.patch.object(api_auth, 'Authorize',
                     mock.MagicMock(side_effect=api_auth.OAuthError()))
  def testFailsOauth(self):
    response = self.testapp.post('/api/new', _BASE_REQUEST, status=403)
    result = json.loads(response.body)
    self.assertEqual(result, {'error': 'User authentication error'})


@mock.patch.object(gitiles_service, 'CommitInfo',
                   mock.MagicMock(return_value={'commit': 'abc'}))
@mock.patch('dashboard.services.issue_tracker_service.IssueTrackerService',
            mock.MagicMock())
@mock.patch.object(utils, 'ServiceAccountHttp', mock.MagicMock())
@mock.patch.object(api_auth, 'Authorize', mock.MagicMock())
class NewTest(_NewTest):

  def testPost(self):
    response = self.testapp.post('/api/new', _BASE_REQUEST, status=200)
    result = json.loads(response.body)
    self.assertIn('jobId', result)
    self.assertEqual(
        result['jobUrl'],
        'https://testbed.example.com/job/%s' % result['jobId'])

  def testNoConfiguration(self):
    request = dict(_BASE_REQUEST)
    request.update(_CONFIGURATION_ARGUMENTS)
    del request['configuration']
    response = self.testapp.post('/api/new', request, status=200)
    result = json.loads(response.body)
    self.assertIn('jobId', result)
    self.assertEqual(
        result['jobUrl'],
        'https://testbed.example.com/job/%s' % result['jobId'])

  def testComparisonModeFunctional(self):
    request = dict(_BASE_REQUEST)
    request['comparison_mode'] = 'functional'
    response = self.testapp.post('/api/new', request, status=200)
    result = json.loads(response.body)
    self.assertIn('jobId', result)
    job = job_module.JobFromId(result['jobId'])
    self.assertEqual(job.state.comparison_mode, 'functional')

  def testComparisonModePerformance(self):
    request = dict(_BASE_REQUEST)
    request['comparison_mode'] = 'performance'
    response = self.testapp.post('/api/new', request, status=200)
    result = json.loads(response.body)
    self.assertIn('jobId', result)
    job = job_module.JobFromId(result['jobId'])
    self.assertEqual(job.state.comparison_mode, 'performance')

  def testComparisonModeUnknown(self):
    request = dict(_BASE_REQUEST)
    request['comparison_mode'] = 'invalid comparison mode'
    response = self.testapp.post('/api/new', request, status=400)
    self.assertIn('error', json.loads(response.body))

  def testWithChanges(self):
    base_request = {}
    base_request.update(_BASE_REQUEST)
    del base_request['start_git_hash']
    del base_request['end_git_hash']
    base_request['changes'] = json.dumps([
        {'commits': [{'repository': 'src', 'git_hash': '1'}]},
        {'commits': [{'repository': 'src', 'git_hash': '3'}]}])

    response = self.testapp.post('/api/new', base_request, status=200)
    result = json.loads(response.body)
    self.assertIn('jobId', result)
    self.assertEqual(
        result['jobUrl'],
        'https://testbed.example.com/job/%s' % result['jobId'])

  @mock.patch('dashboard.pinpoint.models.change.patch.FromDict')
  def testWithPatch(self, mock_patch):
    mock_patch.return_value = None
    request = dict(_BASE_REQUEST)
    request['patch'] = 'https://lalala/c/foo/bar/+/123'

    response = self.testapp.post('/api/new', request, status=200)
    result = json.loads(response.body)
    self.assertIn('jobId', result)
    self.assertEqual(
        result['jobUrl'],
        'https://testbed.example.com/job/%s' % result['jobId'])
    mock_patch.assert_called_with(request['patch'])

  def testMissingTarget(self):
    request = dict(_BASE_REQUEST)
    del request['target']
    response = self.testapp.post('/api/new', request, status=400)
    self.assertIn('error', json.loads(response.body))

  def testInvalidTestConfig(self):
    request = dict(_BASE_REQUEST)
    del request['configuration']
    response = self.testapp.post('/api/new', request, status=400)
    self.assertIn('error', json.loads(response.body))

  def testInvalidBug(self):
    request = dict(_BASE_REQUEST)
    request['bug_id'] = 'not_an_int'
    response = self.testapp.post('/api/new', request, status=400)
    self.assertEqual({'error': new._ERROR_BUG_ID},
                     json.loads(response.body))

  def testEmptyBug(self):
    request = dict(_BASE_REQUEST)
    request['bug_id'] = ''
    response = self.testapp.post('/api/new', request, status=200)
    result = json.loads(response.body)
    self.assertIn('jobId', result)
    self.assertEqual(
        result['jobUrl'],
        'https://testbed.example.com/job/%s' % result['jobId'])
    job = job_module.JobFromId(result['jobId'])
    self.assertIsNone(job.bug_id)

  @mock.patch('dashboard.pinpoint.models.change.patch.FromDict')
  def testPin(self, mock_patch):
    mock_patch.return_value = 'patch'
    request = dict(_BASE_REQUEST)
    request['pin'] = 'https://lalala/c/foo/bar/+/123'

    response = self.testapp.post('/api/new', request, status=200)
    result = json.loads(response.body)
    self.assertIn('jobId', result)
    self.assertEqual(
        result['jobUrl'],
        'https://testbed.example.com/job/%s' % result['jobId'])
    mock_patch.assert_called_with(request['pin'])

  def testValidTags(self):
    request = dict(_BASE_REQUEST)
    request['tags'] = json.dumps({'key': 'value'})
    response = self.testapp.post('/api/new', request, status=200)
    result = json.loads(response.body)
    self.assertIn('jobId', result)

  def testInvalidTags(self):
    request = dict(_BASE_REQUEST)
    request['tags'] = json.dumps(['abc'])
    response = self.testapp.post('/api/new', request, status=400)
    self.assertIn('error', json.loads(response.body))

  def testInvalidTagType(self):
    request = dict(_BASE_REQUEST)
    request['tags'] = json.dumps({'abc': 123})
    response = self.testapp.post('/api/new', request, status=400)
    self.assertIn('error', json.loads(response.body))

  def testUserFromParams(self):
    request = dict(_BASE_REQUEST)
    request['user'] = 'foo@example.org'
    response = self.testapp.post('/api/new', request, status=200)
    result = json.loads(response.body)
    job = job_module.JobFromId(result['jobId'])
    self.assertEqual(job.user, 'foo@example.org')

  def testUserFromAuth(self):
    response = self.testapp.post('/api/new', _BASE_REQUEST, status=200)
    result = json.loads(response.body)
    job = job_module.JobFromId(result['jobId'])
    self.assertEqual(job.user, 'example@example.com')
