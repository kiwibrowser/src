# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time
import sys

from tracing.mre import progress_reporter


class GTestRunReporter(progress_reporter.RunReporter):

  def __init__(self, canonical_url, output_stream, timestamp):
    super(GTestRunReporter, self).__init__(canonical_url)
    self._output_stream = output_stream
    self._timestamp = timestamp

  def _GetMs(self):
    assert self._timestamp is not None, 'Did not call WillRun.'
    return (time.time() - self._timestamp) * 1000

  def DidAddFailure(self, failure):
    super(GTestRunReporter, self).DidAddFailure(failure)
    print >> self._output_stream, failure.stack.encode('utf-8')
    self._output_stream.flush()

  def DidRun(self, run_failed):
    super(GTestRunReporter, self).DidRun(run_failed)
    if run_failed:
      print >> self._output_stream, '[  FAILED  ] %s (%0.f ms)' % (
          self.canonical_url, self._GetMs())
    else:
      print >> self._output_stream, '[       OK ] %s (%0.f ms)' % (
          self.canonical_url, self._GetMs())
    self._output_stream.flush()


class GTestProgressReporter(progress_reporter.ProgressReporter):
  """A progress reporter that outputs the progress report in gtest style.

  Be careful each print should only handle one string. Otherwise, the output
  might be interrupted by Chrome logging, and the output interpretation might
  be incorrect. For example:
      print >> self._output_stream, "[ OK ]", testname
  should be written as
      print >> self._output_stream, "[ OK ] %s" % testname
  """

  def __init__(self, output_stream=sys.stdout):
    super(GTestProgressReporter, self).__init__()
    self._output_stream = output_stream

  def WillRun(self, canonical_url):
    super(GTestProgressReporter, self).WillRun(canonical_url)
    print >> self._output_stream, '[ RUN      ] %s' % (
        canonical_url.encode('utf-8'))
    self._output_stream.flush()
    return GTestRunReporter(canonical_url, self._output_stream, time.time())

  def DidFinishAllRuns(self, result_list):
    super(GTestProgressReporter, self).DidFinishAllRuns(result_list)
    successful_runs = 0
    failed_canonical_urls = []
    failed_runs = 0
    for run in result_list:
      if len(run.failures) != 0:
        failed_runs += 1
        for f in run.failures:
          failed_canonical_urls.append(f.trace_canonical_url)
      else:
        successful_runs += 1

    unit = 'test' if successful_runs == 1 else 'tests'
    print >> self._output_stream, '[  PASSED  ] %d %s.' % (
        (successful_runs, unit))
    if len(failed_canonical_urls) > 0:
      unit = 'test' if len(failed_canonical_urls) == 1 else 'tests'
      print >> self._output_stream, '[  FAILED  ] %d %s, listed below:' % (
          (failed_runs, unit))
      for failed_canonical_url in failed_canonical_urls:
        print >> self._output_stream, '[  FAILED  ]  %s' % (
            failed_canonical_url.encode('utf-8'))
      print >> self._output_stream
      count = len(failed_canonical_urls)
      unit = 'TEST' if count == 1 else 'TESTS'
      print >> self._output_stream, '%d FAILED %s' % (count, unit)
    print >> self._output_stream

    self._output_stream.flush()
