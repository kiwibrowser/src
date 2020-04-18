# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import datetime
import json
import time
import unittest

import mock
import webapp2
import webtest

from dashboard import layered_cache
from dashboard import update_bug_with_results
from dashboard.common import namespaced_stored_object
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import bug_data
from dashboard.models import try_job
from dashboard.services import buildbucket_service

_SAMPLE_BISECT_RESULTS_JSON = {
    'try_job_id': 6789,
    'bug_id': 4567,
    'status': 'completed',
    'bisect_bot': 'linux',
    'buildbot_log_url': '',
    'command': ('tools/perf/run_benchmark -v '
                '--browser=release page_cycler.intl_ar_fa_he'),
    'metric': 'warm_times/page_load_time',
    'change': '',
    'good_revision': '306475',
    'bad_revision': '306478',
    'warnings': None,
    'abort_reason': None,
    'issue_url': 'https://issue_url/123456',
    'culprit_data': {
        'subject': 'subject',
        'author': u'author \u2265 unicode',
        'email': 'author@email.com',
        'cl_date': '1/2/2015',
        'commit_info': 'commit_info',
        'revisions_links': ['http://src.chromium.org/viewvc/chrome?view='
                            'revision&revision=20798'],
        'cl': '2a1781d64d'
    },
    'revision_data': [
        {
            'depot_name': 'chromium',
            'deps_revision': 1234,
            'commit_hash': '1234abcdf',
            'mean_value': 70,
            'std_dev': 0,
            'values': [70, 70, 70],
            'result': 'good'
        }, {
            'depot_name': 'chromium',
            'deps_revision': 1235,
            'commit_hash': '1235abdcf',
            'mean_value': 80,
            'std_dev': 0,
            'values': [80, 80, 80],
            'result': 'bad'
        }
    ]
}

_SAMPLE_BISECT_FAILED_JSON = {
    'try_job_id': 12345,
    'bug_id': 1234,
    'status': 'failed',
    'buildbot_log_url': 'http://master/builder/builds/138',
    'failure_reason': 'BUILD_FAILURE',
    'extra_result_code': [
        'B4T_MISSING_METRIC',
        'B4T_TEST_FAILURE',
    ],
}

_REVISION_RESPONSE = """
<html xmlns=....>
<head><title>[chrome] Revision 207985</title></head><body><table>....
<tr align="left">
<th>Log Message:</th>
<td> Message....</td>
&gt; &gt; Review URL: <a href="https://codereview.chromium.org/81533002">\
https://codereview.chromium.org/81533002</a>
&gt;
&gt; Review URL: <a href="https://codereview.chromium.org/96073002">\
https://codereview.chromium.org/96073002</a>

Review URL: <a href="https://codereview.chromium.org/17504006">\
https://codereview.chromium.org/96363002</a></pre></td></tr></table>....</body>
</html>
"""

_PERF_TEST_CONFIG = """config = {
  'command': 'tools/perf/run_benchmark -v --browser=release\
dromaeo.jslibstylejquery --profiler=trace',
  'good_revision': '215806',
  'bad_revision': '215828',
  'repeat_count': '1',
  'max_time_minutes': '120'
}"""

_ISSUE_RESPONSE = """
    {
      "description": "Issue Description.",
      "cc": [
              "chromium-reviews@chromium.org",
              "cc-bugs@chromium.org",
              "sullivan@google.com"
            ],
      "reviewers": [
                      "prasadv@google.com"
                   ],
      "owner_email": "sullivan@google.com",
      "private": false,
      "base_url": "svn://chrome-svn/chrome/trunk/src/",
      "owner":"sullivan",
      "subject":"Issue Subject",
      "created":"2013-06-20 22:23:27.227150",
      "patchsets":[1,21001,29001],
      "modified":"2013-06-22 00:59:38.530190",
      "closed":true,
      "commit":false,
      "issue":17504006
    }
"""


def _MockFetch(url=None):
  url_to_response_map = {
      'http://src.chromium.org/viewvc/chrome?view=revision&revision=20798': [
          200, _REVISION_RESPONSE
      ],
      'http://src.chromium.org/viewvc/chrome?view=revision&revision=20799': [
          200, 'REVISION REQUEST FAILED!'
      ],
      'https://codereview.chromium.org/api/17504006': [
          200, json.dumps(json.loads(_ISSUE_RESPONSE))
      ],
  }

  if url not in url_to_response_map:
    assert False, 'Bad url %s' % url

  response_code = url_to_response_map[url][0]
  response = url_to_response_map[url][1]
  return testing_common.FakeResponseObject(response_code, response)


def _MockBuildBucketResponse(result, updated_ts, status='COMPLETED'):
  return {
      "build": {
          "status": status,
          "created_ts": "1468500053889710",
          "url": "http://build.chromium.org/p/tryserver.chromium.perf/builders/winx64_10_perf_bisect/builds/594",  # pylint: disable=line-too-long
          "bucket": "master.tryserver.chromium.perf",
          "result_details_json": "{\"properties\": {\"got_nacl_revision\": \"0949e1bef9d6b25ee44eb69a54e0cc6f8a677375\", \"got_swarming_client_revision\": \"df6e95e7669883c8fe9ef956c69a544154701a49\", \"got_revision\": \"c061dd11ada8a97335b2ef9b13757cdb780f84e8\", \"recipe\": \"bisection/desktop_bisect\", \"got_webrtc_revision_cp\": \"refs/heads/master@{#13407}\", \"build_revision\": \"a16e208eee39697b78877c2482b8c4c8d67ac866\", \"buildnumber\": 594, \"slavename\": \"build230-m4\", \"got_revision_cp\": \"refs/heads/master@{#404161}\", \"blamelist\": [], \"branch\": \"\", \"revision\": \"\", \"workdir\": \"C://build/slave/winx64_10_perf_bisect\", \"repository\": \"\", \"buildername\": \"winx64_10_perf_bisect\", \"got_webrtc_revision\": \"05929e21d437bc5f80309455f168a9e4bb2bc94b\", \"mastername\": \"tryserver.chromium.perf\", \"build_scm\": \"git\", \"got_angle_revision\": \"5695fc990fae1897f31bd418f9278e931776abdf\", \"got_v8_revision\": \"b70ce97a8692ddc60102e481a502de32cd4b305e\", \"got_v8_revision_cp\": \"refs/heads/5.4.53@{#1}\", \"requester\": \"425761728072-pa1bs18esuhp2cp2qfa1u9vb6p1v6kfu@developer.gserviceaccount.com\", \"buildbotURL\": \"http://build.chromium.org/p/tryserver.chromium.perf/\", \"bisect_config\": {\"good_revision\": \"404161\", \"builder_host\": null, \"recipe_tester_name\": \"winx64_10_perf_bisect\", \"metric\": \"load_tools-memory:chrome:all_processes:reported_by_chrome:dom_storage:effective_size_avg/load_tools_dropbox\", \"max_time_minutes\": \"20\", \"builder_port\": null, \"bug_id\": 627867, \"command\": \"src/tools/perf/run_benchmark -v --browser=release_x64 --output-format=chartjson --upload-results --also-run-disabled-tests system_health.memory_desktop\", \"repeat_count\": \"20\", \"try_job_id\": 5774140045262848, \"test_type\": \"perf\", \"gs_bucket\": \"chrome-perf\", \"bad_revision\": \"404190\"}, \"project\": \"\", \"requestedAt\": 1468500060, \"got_buildtools_revision\": \"aa47d9773d8f4d6254a587a1240b3dc023d54f06\"}}",  # pylint: disable=line-too-long
          "status_changed_ts": "1468501519275050",
          "failure_reason": "INFRA_FAILURE",
          "result": result,
          "utcnow_ts": "1469127526866210",
          "id": "2222",
          "completed_ts": "1468501519275080",
          "updated_ts": str(updated_ts)
          },
      "kind": "buildbucket#resourcesItem",
      "etag": "H2Wqa6BpE2P1s-eqgn97T3TaxBw/X08XU5XK_4l3O610jUcjy3KlfwU"
  }


# In this class, we patch apiclient.discovery.build so as to not make network
# requests, which are normally made when the IssueTrackerService is initialized.
@mock.patch('apiclient.discovery.build', mock.MagicMock())
@mock.patch.object(utils, 'ServiceAccountHttp', mock.MagicMock())
@mock.patch.object(utils, 'TickMonitoringCustomMetric', mock.MagicMock())
class UpdateBugWithResultsTest(testing_common.TestCase):

  def setUp(self):
    super(UpdateBugWithResultsTest, self).setUp()
    app = webapp2.WSGIApplication([(
        '/update_bug_with_results',
        update_bug_with_results.UpdateBugWithResultsHandler)])
    self.testapp = webtest.TestApp(app)

    self.SetCurrentUser('internal@chromium.org', is_admin=True)

    namespaced_stored_object.Set('repositories', {
        'chromium': {
            'repository_url': 'https://chromium.googlesource.com/chromium/src'
        },
    })

  def _AddTryJob(self, bug_id, status, bot, **kwargs):
    job = try_job.TryJob(bug_id=bug_id, status=status, bot=bot, **kwargs)
    job.put()
    bug_data.Bug(id=bug_id).put()
    return job

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service, 'IssueTrackerService',
      mock.MagicMock())
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet(self):
    # Put succeeded, failed, staled, and not yet finished jobs in the
    # datastore.
    self._AddTryJob(11111, 'started', 'win_perf',
                    results_data=_SAMPLE_BISECT_RESULTS_JSON)
    staled_timestamp = (datetime.datetime.now() -
                        update_bug_with_results._STALE_TRYJOB_DELTA)
    self._AddTryJob(22222, 'started', 'win_perf',
                    last_ran_timestamp=staled_timestamp)
    self._AddTryJob(33333, 'failed', 'win_perf')
    self._AddTryJob(44444, 'started', 'win_perf')

    self.testapp.get('/update_bug_with_results')
    pending_jobs = try_job.TryJob.query().fetch()
    # Expects no jobs to be deleted.
    self.assertEqual(4, len(pending_jobs))
    self.assertEqual(11111, pending_jobs[0].bug_id)
    self.assertEqual('completed', pending_jobs[0].status)
    self.assertEqual(22222, pending_jobs[1].bug_id)
    self.assertEqual('staled', pending_jobs[1].status)
    self.assertEqual(33333, pending_jobs[2].bug_id)
    self.assertEqual('failed', pending_jobs[2].status)
    self.assertEqual(44444, pending_jobs[3].bug_id)
    self.assertEqual('started', pending_jobs[3].status)

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service, 'IssueTrackerService',
      mock.MagicMock())
  @mock.patch.object(
      buildbucket_service, 'GetJobStatus',
      mock.MagicMock(
          return_value=_MockBuildBucketResponse(
              result='SUCCESS',
              updated_ts=int(round(time.time() * 1000000)))))
  def testCreateTryJob_WithoutExistingBug(self):
    # Put succeeded job in the datastore.
    try_job.TryJob(
        bug_id=12345, status='started', bot='win_perf',
        results_data=_SAMPLE_BISECT_RESULTS_JSON).put()

    self.testapp.get('/update_bug_with_results')
    pending_jobs = try_job.TryJob.query().fetch()

    # Expects job to finish.
    self.assertEqual(1, len(pending_jobs))
    self.assertEqual(12345, pending_jobs[0].bug_id)
    self.assertEqual('completed', pending_jobs[0].status)

  @mock.patch.object(utils, 'ServiceAccountHttp', mock.MagicMock())
  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'AddBugComment', mock.MagicMock(return_value=False))
  @mock.patch('logging.error')
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_FailsToUpdateBug_LogsErrorAndMovesOn(self, mock_logging_error):
    # Put a successful job and a failed job with partial results.
    # Note that AddBugComment is mocked to always returns false, which
    # simulates failing to post results to the issue tracker for all bugs.
    self._AddTryJob(12345, 'started', 'win_perf',
                    results_data=_SAMPLE_BISECT_RESULTS_JSON)
    self._AddTryJob(54321, 'started', 'win_perf',
                    results_data=_SAMPLE_BISECT_RESULTS_JSON)
    self.testapp.get('/update_bug_with_results')

    # Two errors should be logged.
    self.assertEqual(2, mock_logging_error.call_count)

    # The pending jobs should still be there.
    pending_jobs = try_job.TryJob.query().fetch()
    self.assertEqual(2, len(pending_jobs))
    self.assertEqual('started', pending_jobs[0].status)
    self.assertEqual('started', pending_jobs[1].status)

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'AddBugComment')
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_BisectCulpritHasAuthor_AssignsAuthor(self, mock_update_bug):
    # When a bisect has a culprit for a perf regression,
    # author and reviewer of the CL should be cc'ed on issue update.
    self._AddTryJob(12345, 'started', 'win_perf',
                    results_data=_SAMPLE_BISECT_RESULTS_JSON)

    self.testapp.get('/update_bug_with_results')
    mock_update_bug.assert_called_once_with(
        mock.ANY,
        '\n=== Auto-CCing suspected CL author author@email.com ===\n\nHi '
        'author@email.com, the bisect results pointed to your CL, please take '
        'a look at the\nresults.\n\n\n=== BISECT JOB RESULTS ===\n<b>Perf '
        'regression found with culprit</b>\n\nSuspected Commit\n  Author : '
        'author \xe2\x89\xa5 unicode\n  Commit : 2a1781d64d\n  Date   : '
        '1/2/2015\n  Subject: subject\n\nBisect Details\n  Configuration: '
        'linux\n  Benchmark    : page_cycler.intl_ar_fa_he\n  Metric       : '
        'warm_times/page_load_time\n\nRevision              Result       '
        'N\nchromium@unknown      70 +- 0      3      '
        'good\nchromium@unknown      80 +- 0      3      bad\n\nTo Run This '
        'Test\n  tools/perf/run_benchmark -v --browser=release page_cycler.'
        'intl_ar_fa_he\n\nMore information on addressing performance '
        'regressions:\n  http://g.co/ChromePerformanceRegressions\n\n'
        'Debug information about this bisect:\n  https://issue_url/123456\n\n\n'
        'For feedback, file a bug with component Speed>Bisection',
        cc_list=[u'author@email.com', 'prasadv@google.com'],
        merge_issue=None, labels=None, owner=u'author@email.com',
        status='Assigned')

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results,
      '_MapAnomaliesToMergeIntoBug')
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'AddBugComment')
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'GetIssue',
      mock.MagicMock(return_value={'id': 111222}))
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_BisectCulpritHasAuthor_MergesBugWithExisting(
      self, mock_update_bug, mock_merge_anomalies):
    layered_cache.SetExternal('commit_hash_2a1781d64d', 111222)
    self._AddTryJob(12345, 'started', 'win_perf',
                    results_data=_SAMPLE_BISECT_RESULTS_JSON)

    self.testapp.get('/update_bug_with_results')
    mock_update_bug.assert_called_once_with(
        mock.ANY, mock.ANY,
        cc_list=[], merge_issue='111222', labels=None, owner=None,
        status=None)
    # Should have skipped updating cache.
    self.assertEqual(
        layered_cache.GetExternal('commit_hash_2a1781d64d'), 111222)
    mock_merge_anomalies.assert_called_once_with('111222', 12345)

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results,
      '_MapAnomaliesToMergeIntoBug')
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'AddBugComment')
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'GetIssue',
      mock.MagicMock(
          return_value={
              'id': 111222,
              'status': 'Duplicate'
          }))
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_BisectCulpritHasAuthor_DoesNotMergeDuplicate(
      self, mock_update_bug, mock_merge_anomalies):
    layered_cache.SetExternal('commit_hash_2a1781d64d', 111222)
    self._AddTryJob(12345, 'started', 'win_perf',
                    results_data=_SAMPLE_BISECT_RESULTS_JSON)

    self.testapp.get('/update_bug_with_results')
    mock_update_bug.assert_called_once_with(
        mock.ANY, mock.ANY,
        cc_list=['author@email.com', 'prasadv@google.com'],
        merge_issue=None, labels=None, owner='author@email.com',
        status='Assigned')
    # Should have skipped updating cache.
    self.assertEqual(
        layered_cache.GetExternal('commit_hash_2a1781d64d'), 111222)
    # Should have skipped mapping anomalies.
    self.assertEqual(0, mock_merge_anomalies.call_count)

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'AddBugComment')
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'GetIssue',
      mock.MagicMock(
          return_value={
              'id': 123,
              'status': 'Assigned'
          }))
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_BisectCulpritHasAuthor_DoesNotMergeIntoBugWithMultipleCulprits(
      self, mock_update_bug):
    data = copy.deepcopy(_SAMPLE_BISECT_RESULTS_JSON)
    self._AddTryJob(123, 'completed', 'win_perf', results_data=data)
    layered_cache.SetExternal('commit_hash_2a1781d64d', '123')

    data = copy.deepcopy(_SAMPLE_BISECT_RESULTS_JSON)
    data['culprit_data']['email'] = 'some-person-2@foo.bar'
    data['culprit_data']['cl'] = 'BBBBBBBB'
    self._AddTryJob(123, 'completed', 'linux_perf', results_data=data)
    layered_cache.SetExternal('commit_hash_BBBBBBBB', '123')

    self._AddTryJob(456, 'started', 'win_perf',
                    results_data=_SAMPLE_BISECT_RESULTS_JSON)

    self.testapp.get('/update_bug_with_results')
    mock_update_bug.assert_called_once_with(
        mock.ANY, mock.ANY,
        cc_list=['author@email.com', 'prasadv@google.com'],
        merge_issue=None, labels=None, owner='author@email.com',
        status='Assigned')

    # Should have skipped updating cache.
    self.assertEqual(
        layered_cache.GetExternal('commit_hash_2a1781d64d'), '456')

  @mock.patch('logging.error')
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'GetIssue',
      mock.MagicMock(
          return_value={
              'id': 123,
              'status': 'Assigned'
          }))
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_BisectNoResultsData_NoException(self, mock_logging_error):
    data = copy.deepcopy(_SAMPLE_BISECT_RESULTS_JSON)
    data['culprit_data'] = None
    self._AddTryJob(123, 'completed', 'win_perf', results_data=data)
    layered_cache.SetExternal('commit_hash_2a1781d64d', '123')

    data = copy.deepcopy(_SAMPLE_BISECT_RESULTS_JSON)
    data['culprit_data'] = None
    self._AddTryJob(123, 'completed', 'linux_perf', results_data=data)
    layered_cache.SetExternal('commit_hash_BBBBBBBB', '123')

    self._AddTryJob(456, 'started', 'win_perf',
                    results_data=_SAMPLE_BISECT_RESULTS_JSON)

    self.testapp.get('/update_bug_with_results')
    self.assertEqual(0, mock_logging_error.call_count)

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'AddBugComment')
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_FailedRevisionResponse(self, mock_add_bug):
    # When a Rietveld CL link fails to respond, only update CL owner in CC
    # list.
    sample_bisect_results = copy.deepcopy(_SAMPLE_BISECT_RESULTS_JSON)
    sample_bisect_results['revisions_links'] = [
        'http://src.chromium.org/viewvc/chrome?view=revision&revision=20799']
    self._AddTryJob(12345, 'started', 'win_perf',
                    results_data=sample_bisect_results)

    self.testapp.get('/update_bug_with_results')
    mock_add_bug.assert_called_once_with(mock.ANY,
                                         mock.ANY,
                                         cc_list=['author@email.com',
                                                  'prasadv@google.com'],
                                         merge_issue=None,
                                         labels=None,
                                         owner='author@email.com',
                                         status='Assigned')

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'AddBugComment', mock.MagicMock())
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_PositiveResult_StoresCommitHash(self):
    self._AddTryJob(12345, 'started', 'win_perf',
                    results_data=_SAMPLE_BISECT_RESULTS_JSON)

    self.testapp.get('/update_bug_with_results')
    self.assertEqual('12345',
                     layered_cache.GetExternal('commit_hash_2a1781d64d'))

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'AddBugComment', mock.MagicMock())
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_NegativeResult_DoesNotStoreCommitHash(self):
    sample_bisect_results = copy.deepcopy(_SAMPLE_BISECT_RESULTS_JSON)
    sample_bisect_results['culprit_data'] = None
    self._AddTryJob(12345, 'started', 'win_perf',
                    results_data=sample_bisect_results)
    self.testapp.get('/update_bug_with_results')

    caches = layered_cache.CachedPickledString.query().fetch()
    self.assertEqual(0, len(caches))

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  @mock.patch.object(
      buildbucket_service, 'GetJobStatus',
      mock.MagicMock(
          return_value=_MockBuildBucketResponse(
              result='FAILURE',
              updated_ts=int(round(time.time() * 1000000)))))
  def testGet_InProgressResult_BuildbucketFailure(self):
    sample_bisect_results = copy.deepcopy(_SAMPLE_BISECT_RESULTS_JSON)
    sample_bisect_results['status'] = 'started'

    job_key = try_job.TryJob(bug_id=12345, status='started', bot='win_perf',
                             results_data=sample_bisect_results).put()
    self.testapp.get('/update_bug_with_results')

    job = job_key.get()
    self.assertEqual(job.results_data['status'], 'started')

  def testMapAnomaliesToMergeIntoBug(self):
    # Add anomalies.
    test_keys = map(utils.TestKey, [
        'ChromiumGPU/linux-release/scrolling-benchmark/first_paint',
        'ChromiumGPU/linux-release/scrolling-benchmark/mean_frame_time'])
    anomaly.Anomaly(
        start_revision=9990, end_revision=9997, test=test_keys[0],
        median_before_anomaly=100, median_after_anomaly=200,
        sheriff=None, bug_id=12345).put()
    anomaly.Anomaly(
        start_revision=9990, end_revision=9996, test=test_keys[0],
        median_before_anomaly=100, median_after_anomaly=200,
        sheriff=None, bug_id=54321).put()
    # Map anomalies to base(dest_bug_id) bug.
    update_bug_with_results._MapAnomaliesToMergeIntoBug(
        dest_bug_id=12345, source_bug_id=54321)
    anomalies = anomaly.Anomaly.query(
        anomaly.Anomaly.bug_id == int(54321)).fetch()
    self.assertEqual(0, len(anomalies))

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.email_template,
      'GetPerfTryJobEmailReport', mock.MagicMock(return_value=None))
  def testSendPerfTryJobEmail_EmptyEmailReport_DontSendEmail(self):
    self._AddTryJob(12345, 'started', 'win_perf', job_type='perf-try',
                    results_data=_SAMPLE_BISECT_RESULTS_JSON)
    self.testapp.get('/update_bug_with_results')
    messages = self.mail_stub.get_sent_messages()
    self.assertEqual(0, len(messages))

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'AddBugComment')
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_InternalOnlyTryJob_AddsInternalOnlyBugLabel(
      self, mock_update_bug):
    self._AddTryJob(12345, 'started', 'win_perf',
                    results_data=_SAMPLE_BISECT_RESULTS_JSON,
                    internal_only=True)

    self.testapp.get('/update_bug_with_results')
    mock_update_bug.assert_called_once_with(
        mock.ANY, mock.ANY,
        cc_list=mock.ANY,
        merge_issue=None, labels=['Restrict-View-Google'], owner=mock.ANY,
        status='Assigned')

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'AddBugComment')
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_FailedTryJob_UpdatesBug(
      self, mock_update_bug):
    self._AddTryJob(12345, 'started', 'win_perf',
                    results_data=_SAMPLE_BISECT_FAILED_JSON,
                    internal_only=True)

    self.testapp.get('/update_bug_with_results')
    mock_update_bug.assert_called_once_with(
        mock.ANY,
        'Bisect failed: http://master/builder/builds/138\n'
        'Failure reason: the build has failed.\n'
        'Additional errors:\n'
        'The metric was not found in the test output.\n'
        'The test failed to produce parseable results.\n')

  @mock.patch(
      'google.appengine.api.urlfetch.fetch',
      mock.MagicMock(side_effect=_MockFetch))
  @mock.patch.object(
      update_bug_with_results.issue_tracker_service.IssueTrackerService,
      'AddBugComment')
  @mock.patch.object(
      update_bug_with_results, '_IsJobCompleted',
      mock.MagicMock(return_value=True))
  def testGet_PostResult_WithoutBugEntity(
      self, mock_update_bug):
    job = try_job.TryJob(bug_id=12345, status='started', bot='win_perf',
                         results_data=_SAMPLE_BISECT_RESULTS_JSON)
    job.put()
    self.testapp.get('/update_bug_with_results')
    mock_update_bug.assert_called_once_with(
        12345, mock.ANY, cc_list=mock.ANY, merge_issue=mock.ANY,
        labels=mock.ANY, owner=mock.ANY, status='Assigned')

  def testValidateBuildbucketResponse_Scheduled(self):
    job = try_job.TryJob(bug_id=12345, status='started', bot='win_perf')
    job.put()
    buildbucket_response_scheduled = r"""{
     "build": {
       "status": "SCHEDULED",
       "id": "9043191319901995952"
     }
    }"""
    self.assertFalse(update_bug_with_results._ValidateBuildbucketResponse(
        json.loads(buildbucket_response_scheduled)))

  def testValidateBuildbucketResponse_Started(self):
    job = try_job.TryJob(bug_id=12345, status='started', bot='win_perf')
    job.put()
    buildbucket_response_started = r"""{
     "build": {
       "status": "STARTED",
       "id": "9043191319901995952"
     }
    }"""
    self.assertFalse(update_bug_with_results._ValidateBuildbucketResponse(
        json.loads(buildbucket_response_started)))

  def testValidateBuildbucketResponse_Success(self):
    buildbucket_response_success = r"""{
     "build": {
       "status": "COMPLETED",
       "url": "http://build.chromium.org/linux_perf_bisector/builds/47",
       "id": "9043278384371361584",
       "result": "SUCCESS"
     }
    }"""
    job = try_job.TryJob(bug_id=12345, status='started', bot='win_perf')
    job.put()
    self.assertTrue(update_bug_with_results._ValidateBuildbucketResponse(
        json.loads(buildbucket_response_success)))

  def testValidateBuildbucketResponse_Failed(self):
    buildbucket_response_failed = r"""{
     "build": {
       "status": "COMPLETED",
       "url": "http://build.chromium.org/linux_perf_bisector/builds/41",
       "failure_reason": "BUILD_FAILURE",
       "result": "FAILURE",
       "failure_reason": "BUILD_FAILURE",
       "id": "9043547105089652704"
     }
    }"""
    job = try_job.TryJob(bug_id=12345, status='started', bot='win_perf')
    job.put()
    with self.assertRaisesRegexp(
        update_bug_with_results.BisectJobFailure,
        update_bug_with_results._BUILD_FAILURE_REASON['BUILD_FAILURE']):
      update_bug_with_results._ValidateBuildbucketResponse(
          json.loads(buildbucket_response_failed))

  def testValidateBuildbucketResponse_Canceled(self):
    buildbucket_response_canceled = r"""{
     "build": {
       "status": "COMPLETED",
       "id": "9043278384371361584",
       "result": "CANCELED",
       "cancelation_reason": "CANCELED_EXPLICITLY"
     }
    }"""
    job = try_job.TryJob(bug_id=12345, status='started', bot='win_perf')
    job.put()
    with self.assertRaisesRegexp(
        update_bug_with_results.BisectJobFailure,
        update_bug_with_results._BUILD_FAILURE_REASON['CANCELED_EXPLICITLY']):
      update_bug_with_results._ValidateBuildbucketResponse(
          json.loads(buildbucket_response_canceled))

  def testValidateBuildbucketResponse_Timeout(self):
    buildbucket_response_canceled = r"""{
     "build": {
       "status": "COMPLETED",
       "cancelation_reason": "TIMEOUT",
       "id": "9043278384371361584",
       "result": "CANCELED"
     }
    }"""
    job = try_job.TryJob(bug_id=12345, status='started', bot='win_perf')
    job.put()
    with self.assertRaisesRegexp(
        update_bug_with_results.BisectJobFailure,
        update_bug_with_results._BUILD_FAILURE_REASON['TIMEOUT']):
      update_bug_with_results._ValidateBuildbucketResponse(
          json.loads(buildbucket_response_canceled))

  def testValidateBuildbucketResponse_InvalidConfig(self):
    buildbucket_response_failed = r"""{
     "build": {
       "status": "COMPLETED",
       "url": "http://build.chromium.org/linux_perf_bisector/builds/41",
       "failure_reason": "INVALID_BUILD_DEFINITION",
       "id": "9043278384371361584",
       "result": "FAILURE"
     }
    }"""
    job = try_job.TryJob(bug_id=12345, status='started', bot='win_perf')
    job.put()
    with self.assertRaisesRegexp(
        update_bug_with_results.BisectJobFailure,
        update_bug_with_results._BUILD_FAILURE_REASON[
            'INVALID_BUILD_DEFINITION']):
      update_bug_with_results._ValidateBuildbucketResponse(
          json.loads(buildbucket_response_failed))

  @mock.patch.object(
      buildbucket_service, 'GetJobStatus',
      mock.MagicMock(
          return_value=_MockBuildBucketResponse(
              result='FAILURE',
              updated_ts=int(round(time.time() * 1000000)))))
  def testCheckFailureBuildBucket_IsFailed(self):
    self._AddTryJob(
        bug_id=1111, status='started', bot='win_perf',
        buildbucket_job_id='12345')
    self.testapp.get('/update_bug_with_results')
    pending_jobs = try_job.TryJob.query().fetch()
    self.assertTrue(pending_jobs[0].status == 'failed')

  @mock.patch.object(
      buildbucket_service, 'GetJobStatus',
      mock.MagicMock(
          return_value=_MockBuildBucketResponse(
              result='STARTED',
              updated_ts=int(round(time.time() * 1000000)),
              status='STARTED')))
  def testCheckFailureBuildBucket_IsStarted(self):
    self._AddTryJob(
        bug_id=1111, status='started', bot='win_perf',
        buildbucket_job_id='12345')
    self.testapp.get('/update_bug_with_results')
    pending_jobs = try_job.TryJob.query().fetch()
    self.assertTrue(pending_jobs[0].status == 'started')

if __name__ == '__main__':
  unittest.main()
