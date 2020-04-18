# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=too-many-lines

import copy
import json
import unittest

from dashboard import bisect_report
from dashboard.common import namespaced_stored_object
from dashboard.common import testing_common
from dashboard.models import try_job


_SAMPLE_BISECT_RESULTS_JSON = json.loads("""
  {
    "issue_url": "https://test-rietveld.appspot.com/200039",
    "aborted_reason": null,
    "bad_revision": "",
    "bisect_bot": "staging_android_nexus5X_perf_bisect",
    "bug_id": 12345,
    "buildbot_log_url": "http://build.chromium.org/513",
    "change": "7.35%",
    "command": "src/tools/perf/run_benchmark foo",
    "culprit_data": null,
    "good_revision": "",
    "metric": "Total/Score",
    "culprit_data": null,
    "revision_data": [],
    "secondary_regressions": [],
    "status": "completed",
    "test_type": "perf",
    "try_job_id": 123456,
    "warnings": []
  }
""")

_SAMPLE_BISECT_REVISION_JSON = json.loads("""
  {
    "build_id": null,
    "commit_hash": "",
    "depot_name": "chromium",
    "failed": false,
    "failure_reason": null,
    "n_observations": 0,
    "result": "unknown",
    "revision_string": ""
  }
""")

_SAMPLE_BISECT_CULPRIT_JSON = json.loads("""
  {
    "author": "author",
    "cl": "cl",
    "cl_date": "Thu Dec 08 01:25:35 2016",
    "commit_info": "commit_info",
    "email": "email",
    "revisions_links": [],
    "subject": "subject"
  }
""")

_ABORTED_NO_VALUES = ('Bisect cannot identify a culprit: No values were found '\
    'while testing the reference range.')
_ABORTED_NO_OUTPUT = ('Bisect cannot identify a culprit: Testing the \"good\" '\
    'revision failed: Test runs failed to produce output.')

class BisectReportTest(testing_common.TestCase):

  def setUp(self):
    super(BisectReportTest, self).setUp()

    namespaced_stored_object.Set('repositories', {
        'chromium': {
            'repository_url': 'https://chromium.googlesource.com/chromium/src'
        },
    })

  def _AddTryJob(self, results_data, **kwargs):
    job = try_job.TryJob(results_data=results_data, **kwargs)
    job.put()
    return job

  def _Revisions(self, revisions):
    revision_data = []
    for r in revisions:
      data = copy.deepcopy(_SAMPLE_BISECT_REVISION_JSON)
      data['commit_hash'] = r['commit']
      data['failed'] = r.get('failed', False)
      data['failure_reason'] = r.get('failure_reason', None)
      data['n_observations'] = r.get('num', 0)
      data['revision_string'] = r['commit']
      data['result'] = r.get('result', 'unknown')
      if 'mean' in r:
        data['mean_value'] = r.get('mean', 0)
        data['std_dev'] = r.get('std_dev', 0)
      data['depot_name'] = r.get('depot_name', 'chromium')
      revision_data.append(data)
    return revision_data

  def _Culprit(self, **kwargs):
    culprit = copy.deepcopy(_SAMPLE_BISECT_CULPRIT_JSON)
    culprit.update(kwargs)
    return culprit

  def _BisectResults(self, **kwargs):
    results = copy.deepcopy(_SAMPLE_BISECT_RESULTS_JSON)
    results.update(kwargs)
    return results

  def testGetReport_CompletedWithCulprit(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 102, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 103, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        culprit_data=self._Culprit(cl=102),
        good_revision=100, bad_revision=103)
    job = self._AddTryJob(results_data)

    log_with_culprit = r"""
=== BISECT JOB RESULTS ===
<b>Perf regression found with culprit</b>

Suspected Commit
  Author : author
  Commit : 102
  Date   : Thu Dec 08 01:25:35 2016
  Subject: subject

Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Revision      Result        N
100           100 +- 0      10      good
101           100 +- 0      10      good
102           200 +- 0      10      bad       <--
103           200 +- 0      10      bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""
    self.assertEqual(log_with_culprit, bisect_report.GetReport(job))

  def testGetReport_ConvertsUnicode(self):
    author = u'Steve Mart\xc3n'
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 102, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 103, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        culprit_data=self._Culprit(cl=102, author=author),
        good_revision=100, bad_revision=103)
    job = self._AddTryJob(results_data)

    self.assertIsInstance(bisect_report.GetReport(job), str)

  def testGetReport_CompletedWithCulprit_Memory(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 102, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 103, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        command='src/tools/perf/run_benchmark system_health.memory_foo',
        culprit_data=self._Culprit(cl=102),
        good_revision=100, bad_revision=103)
    job = self._AddTryJob(results_data)

    log_with_culprit = r"""
=== BISECT JOB RESULTS ===
<b>Perf regression found with culprit</b>

Suspected Commit
  Author : author
  Commit : 102
  Date   : Thu Dec 08 01:25:35 2016
  Subject: subject

Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : system_health.memory_foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Revision      Result        N
100           100 +- 0      10      good
101           100 +- 0      10      good
102           200 +- 0      10      bad       <--
103           200 +- 0      10      bad

Please refer to the following doc on diagnosing memory regressions:
  https://chromium.googlesource.com/chromium/src/+/master/docs/memory-infra/memory_benchmarks.md

To Run This Test
  src/tools/perf/run_benchmark system_health.memory_foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""
    self.assertEqual(log_with_culprit, bisect_report.GetReport(job))

  def testGetReport_CompletedWithCulprit_WebRTC(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 102, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 103, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        command='src/tools/perf/run_benchmark webrtc.foo',
        culprit_data=self._Culprit(cl=102),
        good_revision=100, bad_revision=103)
    job = self._AddTryJob(results_data)

    webrtc_doc_link = """
Please refer to the following doc on diagnosing webrtc regressions:
  https://chromium.googlesource.com/chromium/src/+/master/docs/speed/benchmark_harnesses/webrtc_perf.md"""

    self.assertIn(webrtc_doc_link, bisect_report.GetReport(job))

  def testGetReport_CompletedWithCulprit_BlinkPerf(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 102, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 103, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        command='src/tools/perf/run_benchmark blink_perf.foo',
        culprit_data=self._Culprit(cl=102),
        good_revision=100, bad_revision=103)
    job = self._AddTryJob(results_data)

    webrtc_doc_link = """
Please refer to the following doc on diagnosing blink_perf regressions:
  https://chromium.googlesource.com/chromium/src/+/master/docs/speed/benchmark_harnesses/blink_perf.md"""

    self.assertIn(webrtc_doc_link, bisect_report.GetReport(job))

  def testGetReport_CompletedWithCulpritReturnCode(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 0, 'num': 10, 'result': 'good'},
                {'commit': 101, 'mean': 0, 'num': 10, 'result': 'good'},
                {'commit': 102, 'mean': 1, 'num': 10, 'result': 'bad'},
                {'commit': 103, 'mean': 1, 'num': 10, 'result': 'bad'},
            ]),
        culprit_data=self._Culprit(cl=102),
        good_revision=100, bad_revision=103, test_type='return_code')
    job = self._AddTryJob(results_data)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Test failure found with culprit</b>

Suspected Commit
  Author : author
  Commit : 102
  Date   : Thu Dec 08 01:25:35 2016
  Subject: subject

Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score

Revision      Exit Code      N
100           0 +- 0         10      good
101           0 +- 0         10      good
102           1 +- 0         10      bad       <--
103           1 +- 0         10      bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

  def testGetReport_CompletedWithoutCulprit(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101},
                {'commit': 102},
                {'commit': 103, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        culprit_data=None,
        good_revision=100, bad_revision=103)
    job = self._AddTryJob(results_data)

    log_without_culprit = r"""
=== BISECT JOB RESULTS ===
<b>NO Perf regression found</b>

Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Revision      Result        N
100           100 +- 0      10      good
103           200 +- 0      10      bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(log_without_culprit, bisect_report.GetReport(job))

  def testGetReport_CompletedWithoutCulpritBuildFailuresAfterReference(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 102, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 103, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        culprit_data=None,
        good_revision=100, bad_revision=103)
    job = self._AddTryJob(results_data)

    log_without_culprit = r"""
=== BISECT JOB RESULTS ===
<b>Perf regression found but unable to narrow commit range</b>

Build failures prevented the bisect from narrowing the range further.


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Suspected Commit Range
  2 commits in range
  https://chromium.googlesource.com/chromium/src/+log/100..103


Revision      Result        N
100           100 +- 0      10       good
102           ---           ---      build failure
103           200 +- 0      10       bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(log_without_culprit, bisect_report.GetReport(job))

  def testGetReport_CompletedWithoutCulpritUnknownDepot(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 1, 'num': 10,
                 'depot_name': 'a', 'result': 'good'},
                {'commit': 101, 'mean': 1, 'num': 10,
                 'depot_name': 'a', 'result': 'good'},
                {'commit': 102, 'mean': 2, 'num': 10,
                 'depot_name': 'a', 'result': 'bad'},
                {'commit': 103, 'mean': 2, 'num': 10,
                 'depot_name': 'a', 'result': 'bad'},
            ]),
        culprit_data=None,
        good_revision=100, bad_revision=103)
    job = self._AddTryJob(results_data)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Perf regression found but unable to narrow commit range</b>

Build failures prevented the bisect from narrowing the range further.


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 1 -> 2

Suspected Commit Range
  1 commits in range
  Unknown depot, please contact team to have this added.


Revision      Result      N
100           1 +- 0      10      good
101           1 +- 0      10      good
102           2 +- 0      10      bad
103           2 +- 0      10      bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

  def testGetReport_CompletedWithBuildFailures(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 102, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 103, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 104, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 105, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        culprit_data=self._Culprit(cl=104),
        good_revision=100, bad_revision=105)
    job = self._AddTryJob(results_data)

    log_without_culprit = r"""
=== BISECT JOB RESULTS ===
<b>Perf regression found with culprit</b>

Suspected Commit
  Author : author
  Commit : 104
  Date   : Thu Dec 08 01:25:35 2016
  Subject: subject

Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Revision      Result        N
100           100 +- 0      10      good
102           100 +- 0      10      good
103           100 +- 0      10      good
104           200 +- 0      10      bad       <--
105           200 +- 0      10      bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(log_without_culprit, bisect_report.GetReport(job))

  def testGetReport_Completed_AbortedWithNoValues(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100},
                {'commit': 105},
            ]),
        aborted=True, aborted_reason=_ABORTED_NO_VALUES,
        good_revision=100, bad_revision=105)
    job = self._AddTryJob(results_data)

    log_without_culprit = r"""
=== BISECT JOB RESULTS ===
<b>NO Perf regression found, tests failed to produce values</b>

Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score


To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(log_without_culprit, bisect_report.GetReport(job))

  def testGetReport_Completed_AbortedWithNoOutput(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100},
                {'commit': 105},
            ]),
        aborted=True, aborted_reason=_ABORTED_NO_OUTPUT,
        good_revision=100, bad_revision=105)
    job = self._AddTryJob(results_data)

    log_without_culprit = r"""
=== BISECT JOB RESULTS ===
<b>NO Perf regression found, tests failed to produce values</b>

Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score


To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(log_without_culprit, bisect_report.GetReport(job))

  def testGetReport_CompletedCouldntNarrowCulprit(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 102, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 103, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 104, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 105, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 106, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        culprit_data=None,
        good_revision=100, bad_revision=106)
    job = self._AddTryJob(results_data)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Perf regression found but unable to narrow commit range</b>

Build failures prevented the bisect from narrowing the range further.


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Suspected Commit Range
  3 commits in range
  https://chromium.googlesource.com/chromium/src/+log/102..105


Revision      Result        N
100           100 +- 0      10       good
102           100 +- 0      10       good
103           ---           ---      build failure
104           ---           ---      build failure
105           200 +- 0      10       bad
106           200 +- 0      10       bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

  def testGetReport_CompletedMoreThan10BuildFailures(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 102, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 103, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 104, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 105, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 106, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 107, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 108, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 109, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 110, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 111, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 112, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 113, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 114, 'failed': True, 'failure_reason': 'reason'},
                {'commit': 115, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 116, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        culprit_data=None,
        good_revision=100, bad_revision=116)
    job = self._AddTryJob(results_data)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Perf regression found but unable to narrow commit range</b>

Build failures prevented the bisect from narrowing the range further.


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Suspected Commit Range
  13 commits in range
  https://chromium.googlesource.com/chromium/src/+log/102..115


Revision      Result        N
100           100 +- 0      10       good
102           100 +- 0      10       good
103           ---           ---      build failure
---           ---           ---      too many build failures to list
114           ---           ---      build failure
115           200 +- 0      10       bad
116           200 +- 0      10       bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

  def testGetReport_FailedBisect(self):
    results_data = self._BisectResults(
        good_revision=100, bad_revision=110, status='failed')
    job = self._AddTryJob(results_data)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Bisect failed for unknown reasons</b>

Please contact the team (see below) and report the error.


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score


To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

  def testGetReport_BisectWithWarnings(self):
    results_data = self._BisectResults(
        status='failed', good_revision=100, bad_revision=103,
        warnings=['A warning.', 'Another warning.'])
    job = self._AddTryJob(results_data)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Bisect failed for unknown reasons</b>

Please contact the team (see below) and report the error.

The following warnings were raised by the bisect job:
 * A warning.
 * Another warning.


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score


To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

  def testGetReport_BisectWithAbortedReason(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 102, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 103, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        good_revision=100, bad_revision=103,
        status='aborted', aborted_reason='Something terrible happened.')
    job = self._AddTryJob(results_data)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Bisect failed unexpectedly</b>

Bisect was aborted with the following:
  Something terrible happened.


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Revision      Result        N
100           100 +- 0      10      good
101           100 +- 0      10      good
102           200 +- 0      10      bad
103           200 +- 0      10      bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

  def testGetReport_StatusStarted(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 105, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 106, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        good_revision=100, bad_revision=106,
        status='started')
    job = self._AddTryJob(results_data)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Bisect was unable to run to completion</b>

The bisect was able to narrow the range, you can try running with:
  good_revision: 101
  bad_revision : 105

If failures persist contact the team (see below) and report the error.


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Revision      Result        N
100           100 +- 0      10      good
101           100 +- 0      10      good
105           200 +- 0      10      bad
106           200 +- 0      10      bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

  def testGetReport_StatusStarted_FailureReason(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 105, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 106, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        good_revision=100, bad_revision=106,
        failure_reason='INFRA_FAILURE',
        status='started')
    job = self._AddTryJob(results_data)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Bisect was unable to run to completion</b>

Error: INFRA_FAILURE

The bisect was able to narrow the range, you can try running with:
  good_revision: 101
  bad_revision : 105

If failures persist contact the team (see below) and report the error.


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Revision      Result        N
100           100 +- 0      10      good
101           100 +- 0      10      good
105           200 +- 0      10      bad
106           200 +- 0      10      bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

  def testGetReport_StatusInProgress(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 105, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 106, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        good_revision=100, bad_revision=106,
        status='in_progress')
    job = self._AddTryJob(results_data)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Bisect is still in progress, results below are incomplete</b>

The bisect was able to narrow the range, you can try running with:
  good_revision: 101
  bad_revision : 105


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Revision      Result        N
100           100 +- 0      10      good
101           100 +- 0      10      good
105           200 +- 0      10      bad
106           200 +- 0      10      bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

  def testGetReport_StatusStartedDepotMismatch(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 1, 'num': 10,
                 'depot_name': 'a', 'result': 'good'},
                {'commit': 101, 'mean': 1, 'num': 10,
                 'depot_name': 'a', 'result': 'good'},
                {'commit': 102, 'mean': 2, 'num': 10,
                 'depot_name': 'b', 'result': 'bad'},
                {'commit': 103, 'mean': 2, 'num': 10,
                 'depot_name': 'b', 'result': 'bad'},
            ]),
        good_revision=100, bad_revision=103,
        status='started')
    job = self._AddTryJob(results_data)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Bisect was unable to run to completion</b>

Please try rerunning the bisect.


If failures persist contact the team (see below) and report the error.


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 1 -> 2

Revision      Result      N
100           1 +- 0      10      good
101           1 +- 0      10      good
102           2 +- 0      10      bad
103           2 +- 0      10      bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

  def testGetReport_WithBugIdBadBisectFeedback(self):
    results_data = self._BisectResults(
        revision_data=self._Revisions(
            [
                {'commit': 100, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 101, 'mean': 100, 'num': 10, 'result': 'good'},
                {'commit': 102, 'mean': 200, 'num': 10, 'result': 'bad'},
                {'commit': 103, 'mean': 200, 'num': 10, 'result': 'bad'},
            ]),
        good_revision=100, bad_revision=103, bug_id=6789)
    job = self._AddTryJob(results_data, bug_id=6789)

    expected_output = r"""
=== BISECT JOB RESULTS ===
<b>Perf regression found but unable to narrow commit range</b>

Build failures prevented the bisect from narrowing the range further.


Bisect Details
  Configuration: staging_android_nexus5X_perf_bisect
  Benchmark    : foo
  Metric       : Total/Score
  Change       : 7.35% | 100 -> 200

Suspected Commit Range
  1 commits in range
  https://chromium.googlesource.com/chromium/src/+log/101..102


Revision      Result        N
100           100 +- 0      10      good
101           100 +- 0      10      good
102           200 +- 0      10      bad
103           200 +- 0      10      bad

To Run This Test
  src/tools/perf/run_benchmark foo

More information on addressing performance regressions:
  http://g.co/ChromePerformanceRegressions

Debug information about this bisect:
  https://test-rietveld.appspot.com/200039


For feedback, file a bug with component Speed>Bisection"""

    self.assertEqual(expected_output, bisect_report.GetReport(job))

if __name__ == '__main__':
  unittest.main()
