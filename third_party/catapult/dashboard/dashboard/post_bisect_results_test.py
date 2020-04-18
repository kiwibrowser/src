# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import json

import webapp2
import webtest

from dashboard import post_bisect_results
from dashboard.common import testing_common
from dashboard.models import try_job

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
    'score': 99.9,
    'good_revision': '306475',
    'bad_revision': '306478',
    'warnings': None,
    'aborted_reason': None,
    'culprit_data': {
        'subject': 'subject',
        'author': 'author',
        'email': 'author@email.com',
        'cl_date': '1/2/2015',
        'commit_info': 'commit_info',
        'revisions_links': [],
        'cl': '1235'
    },
    'revision_data': [
        {
            'revision_string': 'chromium@1234',
            'commit_hash': '1234123412341234123412341234123412341234',
            'depot_name': 'chromium',
            'mean_value': 70,
            'std_dev': 0,
            'values': [70, 70, 70],
            'result': 'good'
        }, {
            'revision_string': 'chromium@1235',
            'depot_name': 'chromium',
            'commit_hash': '1235123512351235123512351235123512351235',
            'mean_value': 80,
            'std_dev': 0,
            'values': [80, 80, 80],
            'result': 'bad'
        }
    ]
}

# Sample IP addresses to use in the tests below.
_WHITELISTED_IP = '123.45.67.89'


class PostBisectResultsTest(testing_common.TestCase):

  def setUp(self):
    super(PostBisectResultsTest, self).setUp()
    app = webapp2.WSGIApplication([
        ('/post_bisect_results',
         post_bisect_results.PostBisectResultsHandler)])
    self.testapp = webtest.TestApp(app)
    testing_common.SetIpWhitelist([_WHITELISTED_IP])

  def testPost(self):
    job_key = try_job.TryJob(id=6789, rietveld_issue_id=200034).put()
    data_param = json.dumps(_SAMPLE_BISECT_RESULTS_JSON)
    self.testapp.post(
        '/post_bisect_results', {'data': data_param},
        extra_environ={'REMOTE_ADDR': _WHITELISTED_IP})

    job = job_key.get()
    self.assertEqual(6789, job.results_data['try_job_id'])
    self.assertEqual('completed', job.results_data['status'])

  def testPost_InProgress(self):
    job_key = try_job.TryJob(id=6790, rietveld_issue_id=200035, bug_id=10).put()
    data = copy.deepcopy(_SAMPLE_BISECT_RESULTS_JSON)
    data['status'] = 'started'
    data['try_job_id'] = 6790
    data_param = json.dumps(data)
    self.testapp.post(
        '/post_bisect_results', {'data': data_param},
        extra_environ={'REMOTE_ADDR': _WHITELISTED_IP})

    job = job_key.get()
    self.assertEqual(6790, job.results_data['try_job_id'])
    self.assertEqual('started', job.results_data['status'])
