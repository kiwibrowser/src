# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import StringIO
import unittest

import mock

from telemetry import story
from telemetry import benchmark
from telemetry.internal.results import csv_output_formatter
from telemetry.internal.results import page_test_results
from telemetry import page as page_module
from telemetry.value import improvement_direction
from telemetry.value import scalar
from telemetry.value import trace
from tracing.trace_data import trace_data


def _MakeStorySet():
  story_set = story.StorySet(base_dir=os.path.dirname(__file__))
  story_set.AddStory(page_module.Page(
      'http://www.foo.com/', story_set, story_set.base_dir,
      name='http://www.foo.com/'))
  story_set.AddStory(page_module.Page(
      'http://www.bar.com/', story_set, story_set.base_dir,
      name='http://www.bar.com/'))
  return story_set


class CsvOutputFormatterTest(unittest.TestCase):

  def setUp(self):
    self._output = StringIO.StringIO()
    self._story_set = _MakeStorySet()
    self._results = page_test_results.PageTestResults(
        benchmark_metadata=benchmark.BenchmarkMetadata('benchmark'))
    self._formatter = None
    self.MakeFormatter()

  def MakeFormatter(self):
    self._formatter = csv_output_formatter.CsvOutputFormatter(self._output)

  def SimulateBenchmarkRun(self, list_of_page_and_values):
    """Simulate one run of a benchmark, using the supplied values.

    Args:
      list_of_pages_and_values: a list of tuple (page, list of values)
    """
    for page, values in list_of_page_and_values:
      self._results.WillRunPage(page)
      for v in values:
        v.page = page
        self._results.AddValue(v)
      self._results.DidRunPage(page)

  def Format(self):
    self._results.telemetry_info.benchmark_start_epoch = 15e8
    self._results.PopulateHistogramSet()
    self._formatter.Format(self._results)
    return self._output.getvalue()

  def testSimple(self):
    # Test a simple benchmark with only one value:
    self.SimulateBenchmarkRun([
        (self._story_set[0], [scalar.ScalarValue(
            None, 'foo', 'seconds', 3,
            improvement_direction=improvement_direction.DOWN)])])
    expected = '\r\n'.join([
        'name,unit,avg,count,max,min,std,sum,architectures,benchmarks,' +
        'benchmarkStart,bots,builds,deviceIds,displayLabel,masters,' +
        'memoryAmounts,osNames,osVersions,productVersions,stories,' +
        'storysetRepeats,traceStart,traceUrls',
        'foo,ms,3000,1,3000,3000,0,3000,,benchmark,2017-07-14 02:40:00,,,,' +
        'benchmark 2017-07-14 02:40:00,,,,,,http://www.foo.com/,,,',
        ''])

    self.assertEqual(expected, self.Format())

  @mock.patch('py_utils.cloud_storage.Insert')
  def testMultiplePagesAndValues(self, cs_insert_mock):
    cs_insert_mock.return_value = 'https://cloud_storage_url/foo'
    trace_value = trace.TraceValue(
        None, trace_data.CreateTraceDataFromRawData('{"traceEvents": []}'),
        remote_path='rp', upload_bucket='foo', cloud_url='http://google.com')
    trace_value.UploadToCloud()
    self.SimulateBenchmarkRun([
        (self._story_set[0], [
            scalar.ScalarValue(
                None, 'foo', 'seconds', 4,
                improvement_direction=improvement_direction.DOWN)]),
        (self._story_set[1], [
            scalar.ScalarValue(
                None, 'foo', 'seconds', 3.4,
                improvement_direction=improvement_direction.DOWN),
            trace_value,
            scalar.ScalarValue(
                None, 'bar', 'km', 10,
                improvement_direction=improvement_direction.DOWN),
            scalar.ScalarValue(
                None, 'baz', 'count', 5,
                improvement_direction=improvement_direction.DOWN)])])

    # Parse CSV output into list of lists.
    csv_string = self.Format()
    lines = csv_string.split('\r\n')
    values = [s.split(',') for s in lines[1:-1]]
    values.sort()

    self.assertEquals(len(values), 4)
    self.assertEquals(len(set((v[1] for v in values))), 2)  # 2 pages.
    self.assertEquals(len(set((v[2] for v in values))), 4)  # 4 value names.
    self.assertEquals(values[2], [
        'foo', 'ms', '3400', '1', '3400', '3400', '0', '3400', '', 'benchmark',
        '2017-07-14 02:40:00', '', '', '', 'benchmark 2017-07-14 02:40:00', '',
        '', '', '', '', 'http://www.bar.com/', '', '', 'http://google.com'])
