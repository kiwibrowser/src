# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib2
import json
import unittest

import mock
import webapp2
import webtest

from google.appengine.ext import ndb

from dashboard import can_bisect
from dashboard import start_try_job
from dashboard.common import testing_common
from dashboard.common import utils
from dashboard.common import namespaced_stored_object
from dashboard.models import anomaly
from dashboard.models import bug_data
from dashboard.models import graph_data
from dashboard.models import try_job
from dashboard.services import issue_tracker_service

# pylint: disable=too-many-lines

# Below is a series of test strings which may contain long lines.
# pylint: disable=line-too-long
_EXPECTED_BISECT_CONFIG_DIFF = """config = {
-  'command': '',
-  'good_revision': '',
-  'bad_revision': '',
-  'metric': '',
-  'repeat_count':'',
-  'max_time_minutes': '',
+  "bad_revision": "215828",
+  "bisect_mode": "mean",
+  "bug_id": "12345",
+  "builder_type": "",
+  "command": "src/tools/perf/run_benchmark -v --browser=release --output-format=chartjson --upload-results --pageset-repeat=1 --also-run-disabled-tests dromaeo.jslibstylejquery",
+  "good_revision": "215806",
+  "max_time_minutes": "20",
+  "metric": "jslib/jslib",
+  "repeat_count": "20",
+  "target_arch": "ia32",
+  "try_job_id": 1
 }
"""

_EXPECTED_PERF_CONFIG_DIFF = """config = {
-  'command': '',
-  'metric': '',
-  'repeat_count': '',
-  'max_time_minutes': '',
+  "bad_revision": "215828",
+  "command": "src/tools/perf/run_benchmark -v --browser=release --output-format=chartjson --upload-results --pageset-repeat=1 --also-run-disabled-tests dromaeo.jslibstylejquery",
+  "good_revision": "215806",
+  "max_time_minutes": "60",
+  "repeat_count": "1",
+  "try_job_id": 1
 }
"""

_EXPECTED_PERF_CONFIG_TRACING_DIFF = """config = {
-  'command': '',
-  'metric': '',
-  'repeat_count': '',
-  'max_time_minutes': '',
+  "bad_revision": "215828",
+  "command": "src/tools/perf/run_benchmark -v --browser=release --output-format=chartjson --upload-results --pageset-repeat=1 --also-run-disabled-tests --extra-chrome-categories=toplevel --extra-atrace-categories=battor dromaeo.jslibstylejquery",
+  "good_revision": "215806",
+  "max_time_minutes": "60",
+  "repeat_count": "1",
+  "try_job_id": 1
 }
"""

_FAKE_XSRF_TOKEN = '1234567890'

_ISSUE_CREATED_RESPONSE = """Issue created. https://test-rietveld.appspot.com/33001
1
1001 filename
"""

_BISECT_CONFIG_CONTENTS = """# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

\"\"\"Config file for Run Performance Test Bisect Tool

This script is intended for use by anyone that wants to run a remote bisection
on a range of revisions to look for a performance regression. Modify the config
below and add the revision range, performance command, and metric. You can then
run a git try <bot>.

Changes to this file should never be submitted.

Args:
  'command': This is the full command line to pass to the
      bisect-perf-regression.py script in order to execute the test.
  'good_revision': An svn or git revision where the metric hadn't regressed yet.
  'bad_revision': An svn or git revision sometime after the metric had
      regressed.
  'metric': The name of the metric to parse out from the results of the
      performance test. You can retrieve the metric by looking at the stdio of
      the performance test. Look for lines of the format:

      RESULT <graph>: <trace>= <value> <units>

      The metric name is "<graph>/<trace>".
  'repeat_count': The number of times to repeat the performance test.
  'max_time_minutes': The script will attempt to run the performance test
      "repeat_count" times, unless it exceeds "max_time_minutes".

Sample config:

config = {
  'command': './out/Release/performance_ui_tests' +
      ' --gtest_filter=PageCyclerTest.Intl1File',
  'good_revision': '179755',
  'bad_revision': '179782',
  'metric': 'times/t',
  'repeat_count': '20',
  'max_time_minutes': '20',
}

On Windows:

config = {
  'command': 'tools/perf/run_benchmark -v --browser=release kraken',
  'good_revision': '185319',
  'bad_revision': '185364',
  'metric': 'Total/Total',
  'repeat_count': '20',
  'max_time_minutes': '20',
}


On ChromeOS:
  - Script accepts either ChromeOS versions, or unix timestamps as revisions.
  - You don't need to specify --identity and --remote, they will be added to
    the command using the bot's BISECT_CROS_IP and BISECT_CROS_BOARD values.

config = {
  'command': './tools/perf/run_benchmark -v '\
      '--browser=cros-chrome-guest '\
      'dromaeo tools/perf/page_sets/dromaeo/jslibstylejquery.json',
  'good_revision': '4086.0.0',
  'bad_revision': '4087.0.0',
  'metric': 'jslib/jslib',
  'repeat_count': '20',
  'max_time_minutes': '20',
}

\"\"\"

config = {
  'command': '',
  'good_revision': '',
  'bad_revision': '',
  'metric': '',
  'repeat_count':'',
  'max_time_minutes': '',
}

# Workaround git try issue, see crbug.com/257689"""

_PERF_CONFIG_CONTENTS = """# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

\"\"\"Config file for Run Performance Test Bot

This script is intended for use by anyone that wants to run a remote performance
test. Modify the config below and add the command to run the performance test,
the metric you're interested in, and repeat/discard parameters. You can then
run a git try <bot>.

Changes to this file should never be submitted.

Args:
  'command': This is the full command line to pass to the
      bisect-perf-regression.py script in order to execute the test.
  'metric': The name of the metric to parse out from the results of the
      performance test. You can retrieve the metric by looking at the stdio of
      the performance test. Look for lines of the format:

      RESULT <graph>: <trace>= <value> <units>

      The metric name is "<graph>/<trace>".
  'repeat_count': The number of times to repeat the performance test.
  'max_time_minutes': The script will attempt to run the performance test
      "repeat_count" times, unless it exceeds "max_time_minutes".

Sample config:

config = {
  'command': './tools/perf/run_benchmark --browser=release smoothness.key_mobile_sites',
  'metric': 'mean_frame_time/mean_frame_time',
  'repeat_count': '20',
  'max_time_minutes': '20',
}

On Windows:

config = {
  'command': 'tools/perf/run_benchmark -v --browser=release \
      smoothness.key_mobile_sites',
  'metric': 'mean_frame_time/mean_frame_time',
  'repeat_count': '20',
  'max_time_minutes': '20',
}


On ChromeOS:
  - Script accepts either ChromeOS versions, or unix timestamps as revisions.
  - You don't need to specify --identity and --remote, they will be added to
    the command using the bot's BISECT_CROS_IP and BISECT_CROS_BOARD values.

config = {
  'command': './tools/perf/run_benchmark -v '\
      '--browser=cros-chrome-guest '\
      'smoothness.key_mobile_sites',
  'metric': 'mean_frame_time/mean_frame_time',
  'repeat_count': '20',
  'max_time_minutes': '20',
}

\"\"\"

config = {
  'command': '',
  'metric': '',
  'repeat_count': '',
  'max_time_minutes': '',
}

# Workaround git try issue, see crbug.com/257689"""
# pylint: enable=line-too-long

# These globals are set in tests and checked in _MockMakeRequest.
_EXPECTED_CONFIG_DIFF = None
_TEST_EXPECTED_BOT = None
_TEST_EXPECTED_CONFIG_CONTENTS = None


def _MockMakeRequest(path, *args, **kwargs):  # pylint: disable=unused-argument
  """Mocks out a request, returning a canned response."""
  if path.endswith('xsrf_token'):
    assert kwargs['headers']['X-Requesting-XSRF-Token'] == 1
    return testing_common.FakeResponseObject(200, _FAKE_XSRF_TOKEN)
  if path == 'upload':
    assert kwargs['method'] == 'POST'
    assert _EXPECTED_CONFIG_DIFF in kwargs['body'], (
        '%s\nnot in\n%s\n' % (_EXPECTED_CONFIG_DIFF, kwargs['body']))
    return testing_common.FakeResponseObject(200, _ISSUE_CREATED_RESPONSE)
  if path == '33001/upload_content/1/1001':
    assert kwargs['method'] == 'POST'
    assert _TEST_EXPECTED_CONFIG_CONTENTS in kwargs['body']
    return testing_common.FakeResponseObject(200, 'Dummy content')
  if path == '33001/upload_complete/1':
    assert kwargs['method'] == 'POST'
    return testing_common.FakeResponseObject(200, 'Dummy content')
  if path == '33001/try/1':
    assert _TEST_EXPECTED_BOT in kwargs['body']
    return testing_common.FakeResponseObject(200, 'Dummy content')
  assert False, 'Invalid url %s requested!' % path


@mock.patch('apiclient.discovery.build', mock.MagicMock())
@mock.patch.object(utils, 'ServiceAccountHttp', mock.MagicMock())
class StartBisectTest(testing_common.TestCase):

  def setUp(self):
    super(StartBisectTest, self).setUp()
    app = webapp2.WSGIApplication(
        [('/start_try_job', start_try_job.StartBisectHandler)])
    self.testapp = webtest.TestApp(app)
    namespaced_stored_object.Set(
        can_bisect.BISECT_BOT_MAP_KEY,
        {
            'ChromiumPerf': [
                ('nexus4', 'android_nexus4_perf_bisect'),
                ('nexus7', 'android_nexus7_perf_bisect'),
                ('win8', 'win_8_perf_bisect'),
                ('xp', 'win_xp_perf_bisect'),
                ('android', 'android_nexus7_perf_bisect'),
                ('mac', 'mac_perf_bisect'),
                ('win', 'win_perf_bisect'),
                ('linux', 'linux_perf_bisect'),
                ('', 'linux_perf_bisect'),
            ],
        })
    namespaced_stored_object.Set(
        start_try_job._BUILDER_TYPES_KEY,
        {'ChromiumPerf': 'perf', 'OtherMaster': 'foo'})
    namespaced_stored_object.Set(
        start_try_job._BOT_BROWSER_MAP_KEY,
        [
            ['android', 'android-chromium'],
            ['winx64', 'release_x64'],
            ['win_x64', 'release_x64'],
            ['', 'release'],
        ])
    namespaced_stored_object.Set(
        start_try_job._MASTER_BUILDBUCKET_MAP_KEY,
        {
            'ChromiumPerf': 'master.tryserver.chromium.perf'
        })
    testing_common.SetSheriffDomains(['chromium.org'])

  @mock.patch.object(utils, 'IsGroupMember', mock.MagicMock(return_value=False))
  def testPost_InvalidUser_ShowsErrorMessage(self):
    self.SetCurrentUser('foo@yahoo.com')
    response = self.testapp.post('/start_try_job', {
        'test_path': 'ChromiumPerf/win7/morejs/times/page_load_time',
        'step': 'prefill-info',
    })
    self.assertEqual(
        {'error': 'User "foo@yahoo.com" not authorized.'},
        json.loads(response.body))

  def testPost_PrefillInfoStep(self):
    self.SetCurrentUser('foo@chromium.org')
    testing_common.AddTests(
        ['ChromiumPerf'],
        [
            'win7',
            'android-nexus7',
            'chromium-rel-win8-dual',
            'chromium-rel-xp-single'
        ],
        {
            'page_cycler.morejs': {
                'benchmark_duration': {},
                'times': {
                    'page_load_time': {},
                    'page_load_time_ref': {},
                    'blog.chromium.org': {},
                    'dev.chromium.org': {},
                    'test.blogspot.com': {},
                    'http___test.com_': {},
                    'Wikipedia_(1_tab)': {}
                },
                'vm_final_size_renderer': {
                    'ref': {},
                    'vm_final_size_renderer_extcs1': {}
                },
            },
            'blink_perf': {
                'Animation_balls': {}
            },
            'octane': {
                'Total': {
                    'Score': {},
                }
            },
            'media.tough_video_cases_extra': {
                'seek': {
                    'garden2_10s.webm_seek_warm': {},
                    'video.html?src_garden2_10s.webm': {}
                }
            },
            'memory.top_10_mobile': {
                'foreground': {
                    'http_en_m_wikipedia_org': {}
                },
                'background': {
                    'after_http_en_m_wikipedia_org': {}
                }
            }
        })
    tests = graph_data.TestMetadata.query().fetch()
    for test in tests:
      name = test.test_name
      if name in ('times', 'page_cycler.morejs', 'blink_perf'):
        continue
      test.has_rows = True
    ndb.put_multi(tests)

    response = self.testapp.post('/start_try_job', {
        'test_path': ('ChromiumPerf/win7/page_cycler.morejs/'
                      'times/Wikipedia_(1_tab)'),
        'step': 'prefill-info',
    })
    info = json.loads(response.body)
    self.assertEqual('win_perf_bisect', info['bisect_bot'])
    self.assertEqual('foo@chromium.org', info['email'])
    self.assertEqual('page_cycler.morejs', info['suite'])
    self.assertEqual('times/Wikipedia_(1_tab)', info['default_metric'])
    self.assertEqual('ChromiumPerf', info['master'])
    self.assertFalse(info['internal_only'])
    self.assertFalse(info['is_admin'])
    self.assertEqual(
        [
            'android_nexus4_perf_bisect',
            'android_nexus7_perf_bisect',
            'linux_perf_bisect',
            'mac_perf_bisect',
            'win_8_perf_bisect',
            'win_perf_bisect',
            'win_xp_perf_bisect',
        ], info['all_bots'])
    self.assertEqual(
        [
            'times/Wikipedia_(1_tab)',
            'times/blog.chromium.org',
            'times/dev.chromium.org',
            'times/http___test.com_',
            'times/page_load_time',
            'times/test.blogspot.com'
        ],
        info['all_metrics'])
    self.assertEqual(info['story_filter'], 'Wikipedia..1.tab.')

    response = self.testapp.post('/start_try_job', {
        'test_path': ('ChromiumPerf/win7/page_cycler.morejs/'
                      'vm_final_size_renderer'),
        'step': 'prefill-info',
    })
    info = json.loads(response.body)
    self.assertEqual(
        ['vm_final_size_renderer/vm_final_size_renderer',
         'vm_final_size_renderer/vm_final_size_renderer_extcs1'],
        info['all_metrics'])
    # vm_final_size_renderer is not a story, so no filter.
    self.assertEqual(info['story_filter'], '')

    response = self.testapp.post('/start_try_job', {
        'test_path': 'ChromiumPerf/win7/blink_perf/Animation_balls',
        'step': 'prefill-info',
    })
    info = json.loads(response.body)
    self.assertEqual('Animation_balls/Animation_balls', info['default_metric'])
    self.assertEqual(info['story_filter'], 'Animation.balls')

    response = self.testapp.post('/start_try_job', {
        'test_path': 'ChromiumPerf/android-nexus7/blink_perf/Animation_balls',
        'step': 'prefill-info',
    })
    info = json.loads(response.body)
    self.assertEqual('android_nexus7_perf_bisect', info['bisect_bot'])
    self.assertEqual(info['story_filter'], 'Animation.balls')

    response = self.testapp.post('/start_try_job', {
        'test_path': 'ChromiumPerf/android-nexus7/octane/Total/Score',
        'step': 'prefill-info',
    })
    info = json.loads(response.body)
    self.assertEqual(info['story_filter'], '')

    response = self.testapp.post('/start_try_job', {
        'test_path': 'ChromiumPerf/win7/page_cycler.morejs/benchmark_duration',
        'step': 'prefill-info',
    })
    info = json.loads(response.body)
    self.assertEqual(info['story_filter'], '')

    response = self.testapp.post('/start_try_job', {
        'test_path': (
            'ChromiumPerf/android-nexus7/media.tough_video_cases_extra'
            '/seek/seek/video.html?src_garden2_10s.webm'),
        'step': 'prefill-info',
    })
    info = json.loads(response.body)
    # Story filter used for pages.
    self.assertEqual(info['story_filter'], 'video.html.src.garden2.10s.webm')

    response = self.testapp.post('/start_try_job', {
        'test_path': (
            'ChromiumPerf/android-nexus7/media.tough_video_cases_extra'
            '/seek/seek/garden2_10s.webm_seek_warm'),
        'step': 'prefill-info',
    })
    info = json.loads(response.body)
    # No story filter used for non-page leaf metrics.
    self.assertEqual(info['story_filter'], '')

    response = self.testapp.post('/start_try_job', {
        'test_path': (
            'ChromiumPerf/android-nexus7/memory.top_10_mobile'
            '/background/after_http_en_m_wikipedia_org'),
        'step': 'prefill-info',
    })
    info = json.loads(response.body)
    # Special story filter for memory.top_10_mobile.
    self.assertEqual(info['story_filter'], 'http.en.m.wikipedia.org')

    response = self.testapp.post('/start_try_job', {
        'test_path': ('ChromiumPerf/chromium-rel-win8-dual/'
                      'blink_perf/Animation_balls'),
        'step': 'prefill-info',
    })
    info = json.loads(response.body)
    self.assertEqual('win_8_perf_bisect', info['bisect_bot'])

    response = self.testapp.post('/start_try_job', {
        'test_path': ('ChromiumPerf/chromium-rel-xp-single/'
                      'blink_perf/Animation_balls'),
        'step': 'prefill-info',
    })
    info = json.loads(response.body)
    self.assertEqual('win_xp_perf_bisect', info['bisect_bot'])

  def _TestGetBisectConfig(self, parameters, expected_config_dict):
    """Helper method to test get-config requests."""
    response = start_try_job.GetBisectConfig(**parameters)
    self.assertEqual(expected_config_dict, response)

  def testGetConfig_UseStaging_GivesStagingRecipeTester(self):
    self._TestGetBisectConfig(
        {
            'bisect_bot': 'linux_perf_bisect',
            'master_name': 'ChromiumPerf',
            'suite': 'page_cycler.moz',
            'metric': 'times/page_load_time',
            'good_revision': '265549',
            'bad_revision': '265556',
            'repeat_count': '15',
            'max_time_minutes': '8',
            'bug_id': '-1',
            'use_staging_bot': 'true',
        },
        {
            'command': ('src/tools/perf/run_benchmark -v '
                        '--browser=release --output-format=chartjson '
                        '--upload-results '
                        '--pageset-repeat=1 '
                        '--also-run-disabled-tests '
                        'page_cycler.moz'),
            'good_revision': '265549',
            'bad_revision': '265556',
            'metric': 'times/page_load_time',
            'recipe_tester_name': 'staging_linux_perf_bisect',
            'repeat_count': '15',
            'max_time_minutes': '8',
            'bug_id': '-1',
            'builder_type': 'perf',
            'target_arch': 'ia32',
            'bisect_mode': 'mean',
        })

  def testGetConfig_EmptyUseArchiveParameter_GivesEmptyBuilderType(self):
    self._TestGetBisectConfig(
        {
            'bisect_bot': 'linux_perf_bisect',
            'master_name': 'ChromiumPerf',
            'suite': 'page_cycler.moz',
            'metric': 'times/page_load_time',
            'good_revision': '265549',
            'bad_revision': '265556',
            'repeat_count': '15',
            'max_time_minutes': '8',
            'bug_id': '-1',
        },
        {
            'command': ('src/tools/perf/run_benchmark -v '
                        '--browser=release --output-format=chartjson '
                        '--upload-results '
                        '--pageset-repeat=1 '
                        '--also-run-disabled-tests '
                        'page_cycler.moz'),
            'good_revision': '265549',
            'bad_revision': '265556',
            'metric': 'times/page_load_time',
            'recipe_tester_name': 'linux_perf_bisect',
            'repeat_count': '15',
            'max_time_minutes': '8',
            'bug_id': '-1',
            'builder_type': 'perf',
            'target_arch': 'ia32',
            'bisect_mode': 'mean',
        })

  def testGetConfig_TelemetryTest(self):
    self._TestGetBisectConfig(
        {
            'bisect_bot': 'win_perf_bisect',
            'master_name': 'ChromiumPerf',
            'suite': 'page_cycler.morejs',
            'metric': 'times/http___test.com_',
            'good_revision': '12345',
            'bad_revision': '23456',
            'repeat_count': '15',
            'max_time_minutes': '8',
            'bug_id': '-1',
            'story_filter': 'http...test\\.com.',
        },
        {
            'command': ('src/tools/perf/run_benchmark -v '
                        '--browser=release --output-format=chartjson '
                        '--upload-results '
                        '--pageset-repeat=1 '
                        '--also-run-disabled-tests '
                        "--story-filter='http...test\\.com.' "
                        'page_cycler.morejs'),
            'good_revision': '12345',
            'bad_revision': '23456',
            'metric': 'times/http___test.com_',
            'recipe_tester_name': 'win_perf_bisect',
            'repeat_count': '15',
            'max_time_minutes': '8',
            'bug_id': '-1',
            'builder_type': 'perf',
            'target_arch': 'ia32',
            'bisect_mode': 'mean',
        })

  def testGetConfig_BisectModeSetToReturnCode(self):
    self._TestGetBisectConfig(
        {
            'bisect_bot': 'linux_perf_bisect',
            'master_name': 'ChromiumPerf',
            'suite': 'page_cycler.moz',
            'metric': 'times/page_load_time',
            'good_revision': '265549',
            'bad_revision': '265556',
            'repeat_count': '15',
            'max_time_minutes': '8',
            'bug_id': '-1',
            'bisect_mode': 'return_code',
        },
        {
            'command': ('src/tools/perf/run_benchmark -v '
                        '--browser=release --output-format=chartjson '
                        '--upload-results '
                        '--pageset-repeat=1 '
                        '--also-run-disabled-tests '
                        'page_cycler.moz'),
            'good_revision': '265549',
            'bad_revision': '265556',
            'metric': 'times/page_load_time',
            'recipe_tester_name': 'linux_perf_bisect',
            'repeat_count': '15',
            'max_time_minutes': '8',
            'bug_id': '-1',
            'builder_type': 'perf',
            'target_arch': 'ia32',
            'bisect_mode': 'return_code',
        })

  def _TestGetConfigCommand(self, expected_command, **params_to_override):
    """Helper method to test the command returned for a get-config request."""
    parameters = dict(
        {
            'bisect_bot': 'linux_perf_bisect',
            'suite': 'page_cycler.moz',
            'master_name': 'ChromiumPerf',
            'metric': 'times/page_load_time',
            'good_revision': '265549',
            'bad_revision': '265556',
            'repeat_count': '15',
            'max_time_minutes': '8',
            'bug_id': '-1',
        }, **params_to_override)
    response = start_try_job.GetBisectConfig(**parameters)
    self.assertEqual(expected_command, response.get('command'))

  def testGuessBisectBot_FetchesNameFromBisectBotMap(self):
    namespaced_stored_object.Set(
        can_bisect.BISECT_BOT_MAP_KEY,
        {'OtherMaster': [('foo', 'super_foo_bisect_bot')]})
    self.assertEqual(
        'super_foo_bisect_bot',
        start_try_job.GuessBisectBot('OtherMaster', 'foo'))

  def testGuessBisectBot_PlatformNotFound_UsesAvailableFallback(self):
    namespaced_stored_object.Set(
        can_bisect.BISECT_BOT_MAP_KEY,
        {'OtherMaster': [('foo', 'super_foo_bisect_bot')]})
    self.assertEqual(
        'super_foo_bisect_bot',
        start_try_job.GuessBisectBot('OtherMaster', 'bar'))

  def testGuessBisectBot_PlatformNotFound_UsesLinuxFallback(self):
    namespaced_stored_object.Set(
        can_bisect.BISECT_BOT_MAP_KEY,
        {'OtherMaster': [
            ('foo', 'super_foo_bisect_bot'),
            ('linux', 'linux_perf_bisect')]})
    self.assertEqual(
        'linux_perf_bisect',
        start_try_job.GuessBisectBot('OtherMaster', 'bar'))

  def testGuessBisectBot_TreatsMasterNameAsPrefix(self):
    namespaced_stored_object.Set(
        can_bisect.BISECT_BOT_MAP_KEY,
        {'OtherMaster': [('foo', 'super_foo_bisect_bot')]})
    self.assertEqual(
        'super_foo_bisect_bot',
        start_try_job.GuessBisectBot('OtherMasterFyi', 'foo'))

  @mock.patch.object(start_try_job.buildbucket_service, 'PutJob',
                     mock.MagicMock(return_value='1234567'))
  @mock.patch.object(
      issue_tracker_service.IssueTrackerService, 'AddBugComment')
  def testPerformBuildbucketBisect(self, add_bug_comment_mock):
    self.SetCurrentUser('foo@chromium.org')

    bug_data.Bug(id=12345).put()

    query_parameters = {
        'bisect_bot': 'linux_perf_tester',
        'suite': 'dromaeo.jslibstylejquery',
        'metric': 'jslib/jslib',
        'good_revision': '215806',
        'bad_revision': '215828',
        'repeat_count': '20',
        'max_time_minutes': '20',
        'bug_id': 12345,
        'step': 'perform-bisect',
    }
    response = self.testapp.post('/start_try_job', query_parameters)
    response_dict = json.loads(response.body)
    self.assertEqual(response_dict['issue_id'], '1234567')
    self.assertIn('1234567', response_dict['issue_url'])
    job_entities = try_job.TryJob.query(
        try_job.TryJob.buildbucket_job_id == '1234567').fetch()
    self.assertEqual(1, len(job_entities))
    add_bug_comment_mock.assert_called_once_with(
        12345, 'Started bisect job https://None/buildbucket_job_status/1234567',
        send_email=False)

  def testPerformBisect_InvalidConfig_ReturnsError(self):
    bisect_job = try_job.TryJob(
        bot='foo',
        config='config = {}',
        master_name='ChromiumPerf',
        internal_only=False,
        job_type='bisect')
    self.assertEqual(
        {'error': 'No "recipe_tester_name" given.'},
        start_try_job.PerformBisect(bisect_job))

  @mock.patch.object(issue_tracker_service.IssueTrackerService, 'AddBugComment')
  @mock.patch(
      'google.appengine.api.app_identity.get_default_version_hostname',
      mock.MagicMock(return_value='my-dashboard.appspot.com'))
  @mock.patch.object(start_try_job.buildbucket_service, 'PutJob',
                     mock.MagicMock(return_value='33001'))
  def testPerformBisect(self, _):
    self.SetCurrentUser('foo@chromium.org')

    # Create bug.
    bug_data.Bug(id=12345).put()

    query_parameters = {
        'bisect_bot': 'win_perf_bisect',
        'suite': 'dromaeo.jslibstylejquery',
        'metric': 'jslib/jslib',
        'good_revision': '215806',
        'bad_revision': '215828',
        'repeat_count': '20',
        'max_time_minutes': '20',
        'bug_id': 12345,
        'step': 'perform-bisect',
    }
    global _EXPECTED_CONFIG_DIFF
    global _TEST_EXPECTED_BOT
    global _TEST_EXPECTED_CONFIG_CONTENTS
    _EXPECTED_CONFIG_DIFF = _EXPECTED_BISECT_CONFIG_DIFF
    _TEST_EXPECTED_BOT = 'win_perf_bisect'
    _TEST_EXPECTED_CONFIG_CONTENTS = _BISECT_CONFIG_CONTENTS
    response = self.testapp.post('/start_try_job', query_parameters)
    issue_url = 'https://my-dashboard.appspot.com/buildbucket_job_status/33001'
    self.assertEqual(
        json.dumps({'issue_id': '33001',
                    'issue_url': issue_url}),
        response.body)

    try_jobs = try_job.TryJob.query().fetch(use_cache=False)
    self.assertEqual(1, len(try_jobs))
    self.assertEqual(issue_url, try_jobs[0].results_data['issue_url'])
    self.assertEqual('33001', try_jobs[0].results_data['issue_id'])

  @mock.patch.object(issue_tracker_service.IssueTrackerService, 'AddBugComment')
  @mock.patch(
      'google.appengine.api.app_identity.get_default_version_hostname',
      mock.MagicMock(return_value='my-dashboard.appspot.com'))
  @mock.patch.object(start_try_job.buildbucket_service, 'PutJob',
                     mock.MagicMock(return_value='33001'))
  def testPerformBisect_AddsToAlert(self, _):
    self.SetCurrentUser('foo@chromium.org')

    # Create bug.
    bug_data.Bug(id=12345).put()

    test_key = utils.TestKey('M/B/S/foo')
    anomaly_entity = anomaly.Anomaly(
        start_revision=1, end_revision=2, test=test_key)
    anomaly_entity.put()

    query_parameters = {
        'bisect_bot': 'win_perf_bisect',
        'suite': 'dromaeo.jslibstylejquery',
        'metric': 'jslib/jslib',
        'good_revision': '215806',
        'bad_revision': '215828',
        'repeat_count': '20',
        'max_time_minutes': '20',
        'bug_id': 12345,
        'step': 'perform-bisect',
        'alerts': json.dumps([anomaly_entity.key.urlsafe()])
    }
    self.testapp.post('/start_try_job', query_parameters)

    try_jobs = try_job.TryJob.query().fetch(use_cache=False)
    self.assertEqual([try_jobs[0].key], anomaly_entity.recipe_bisects)

  @mock.patch.object(issue_tracker_service.IssueTrackerService, 'AddBugComment')
  @mock.patch.object(start_try_job.buildbucket_service, 'PutJob',
                     mock.MagicMock(side_effect=httplib2.HttpLib2Error))
  def testPerformBisectStep_DeleteJobOnFailedBisect(self, _):
    self.SetCurrentUser('foo@chromium.org')
    query_parameters = {
        'bisect_bot': 'linux_perf_bisect',
        'suite': 'dromaeo.jslibstylejquery',
        'metric': 'jslib/jslib',
        'good_revision': '215806',
        'bad_revision': '215828',
    }
    global _EXPECTED_CONFIG_DIFF
    global _TEST_EXPECTED_CONFIG_CONTENTS
    global _TEST_EXPECTED_BOT
    _EXPECTED_CONFIG_DIFF = _EXPECTED_PERF_CONFIG_DIFF
    _TEST_EXPECTED_CONFIG_CONTENTS = _PERF_CONFIG_CONTENTS
    _TEST_EXPECTED_BOT = 'linux_perf_bisect'

    query_parameters['step'] = 'perform-bisect'
    self.testapp.post('/start_try_job', query_parameters)
    try_jobs = try_job.TryJob.query().fetch()
    self.assertEqual(0, len(try_jobs))

  @mock.patch(
      'google.appengine.api.app_identity.get_default_version_hostname',
      mock.MagicMock(return_value='my-dashboard.appspot.com'))
  @mock.patch.object(start_try_job.buildbucket_service, 'PutJob',
                     mock.MagicMock(return_value='1234567'))
  @mock.patch.object(
      issue_tracker_service.IssueTrackerService, 'AddBugComment')
  def testPerformBisectWithArchive(self, _):
    self.SetCurrentUser('foo@chromium.org')

    # Create bug.
    bug_data.Bug(id=12345).put()

    query_parameters = {
        'bisect_bot': 'linux_perf_tester',
        'suite': 'dromaeo.jslibstylejquery',
        'metric': 'jslib/jslib',
        'good_revision': '215806',
        'bad_revision': '215828',
        'repeat_count': '20',
        'max_time_minutes': '20',
        'bug_id': 12345,
        'bisect_mode': 'mean',
        'step': 'perform-bisect',
    }
    response = self.testapp.post('/start_try_job', query_parameters)
    self.assertEqual(
        json.dumps({'issue_id': '1234567',
                    'issue_url': ('https://my-dashboard.appspot.com'
                                  '/buildbucket_job_status/1234567')}),
        response.body)

  def testGetBisectConfig_WithTargetArch(self):
    self._TestGetBisectConfig(
        {
            'bisect_bot': 'win_x64_perf_bisect',
            'master_name': 'ChromiumPerf',
            'suite': 'page_cycler.moz',
            'metric': 'times/page_load_time',
            'good_revision': '265549',
            'bad_revision': '265556',
            'repeat_count': '15',
            'max_time_minutes': '8',
            'bug_id': '-1',
        },
        {
            'command': ('src/tools/perf/run_benchmark -v '
                        '--browser=release_x64 --output-format=chartjson '
                        '--upload-results '
                        '--pageset-repeat=1 '
                        '--also-run-disabled-tests '
                        'page_cycler.moz'),
            'good_revision': '265549',
            'bad_revision': '265556',
            'metric': 'times/page_load_time',
            'recipe_tester_name': 'win_x64_perf_bisect',
            'repeat_count': '15',
            'max_time_minutes': '8',
            'bug_id': '-1',
            'builder_type': 'perf',
            'target_arch': 'x64',
            'bisect_mode': 'mean',
        })

  def testGetConfig_AndroidTelemetryTest(self):
    self._TestGetConfigCommand(
        ('src/tools/perf/run_benchmark -v '
         '--browser=android-chromium --output-format=chartjson '
         '--upload-results '
         '--pageset-repeat=1 '
         '--also-run-disabled-tests '
         'page_cycler.morejs'),
        bisect_bot='android_nexus7_perf_bisect',
        suite='page_cycler.morejs')

  def testGetConfig_ResourceSizestests(self):
    self._TestGetConfigCommand(
        ('src/build/android/resource_sizes.py '
         '--chromium-output-directory {CHROMIUM_OUTPUT_DIR} '
         '--chartjson {CHROMIUM_OUTPUT_DIR}/apks/MonochromePublic.apk'),
        bisect_bot='linux_perf_bisect',
        suite='resource_sizes (MonochromePublic.apk)')

  def testGetConfig_CCPerftests(self):
    self._TestGetConfigCommand(
        ('./src/out/Release/cc_perftests '
         '--test-launcher-print-test-stdio=always --verbose'),
        bisect_bot='linux_perf_bisect',
        suite='cc_perftests')

  def testGetConfig_AndroidCCPerftests(self):
    self._TestGetConfigCommand(
        'src/build/android/test_runner.py '
        'gtest --release -s cc_perftests --verbose',
        bisect_bot='android_nexus7_perf_bisect',
        suite='cc_perftests')

  def testGetConfig_TracingPerftests(self):
    self._TestGetConfigCommand(
        ('./src/out/Release/tracing_perftests '
         '--test-launcher-print-test-stdio=always --verbose'),
        bisect_bot='linux_perf_bisect',
        suite='tracing_perftests')

  def testGetConfig_AndroidTracingPerftests(self):
    self._TestGetConfigCommand(
        'src/build/android/test_runner.py '
        'gtest --release -s tracing_perftests --verbose',
        bisect_bot='android_nexus7_perf_bisect',
        suite='tracing_perftests')

  def testGetConfig_IdbPerf(self):
    self._TestGetConfigCommand(
        ('.\\src\\out\\Release\\performance_ui_tests.exe '
         '--gtest_filter=IndexedDBTest.Perf'),
        bisect_bot='win_perf_bisect',
        suite='idb_perf')

  def testGetConfig_PerformanceBrowserTests(self):
    self._TestGetConfigCommand(
        ('./src/out/Release/performance_browser_tests '
         '--test-launcher-print-test-stdio=always '
         '--enable-gpu'),
        bisect_bot='linux_perf_bisect',
        suite='performance_browser_tests')

  def testGetConfig_X64Bot_UsesX64ReleaseDirectory(self):
    self._TestGetConfigCommand(
        ('.\\src\\out\\Release_x64\\performance_browser_tests.exe '
         '--test-launcher-print-test-stdio=always '
         '--enable-gpu'),
        bisect_bot='winx64nvidia_perf_bisect',
        suite='performance_browser_tests')

  def testGuessMetric_SummaryMetricWithNoTIRLabel(self):
    testing_common.AddTests(
        ['M'], ['b'],
        {'benchmark': {'chart': {'page': {}}}})
    self.assertEqual(
        'chart/chart',
        start_try_job.GuessMetric('M/b/benchmark/chart'))


if __name__ == '__main__':
  unittest.main()
