# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import json
import os
import shutil
import StringIO
import time
import unittest

from telemetry import benchmark
from telemetry import page as page_module
from telemetry import story
from telemetry.internal.results import json_3_output_formatter
from telemetry.internal.results import page_test_results
from telemetry.internal.results import results_options
from telemetry.testing import options_for_unittests
from telemetry.value import improvement_direction
from telemetry.value import scalar


def _MakeStorySet():
  story_set = story.StorySet(base_dir=os.path.dirname(__file__))
  story_set.AddStory(
      page_module.Page('http://www.foo.com/', story_set, story_set.base_dir,
                       name='Foo'))
  story_set.AddStory(
      page_module.Page('http://www.bar.com/', story_set, story_set.base_dir,
                       name='Bar'))
  story_set.AddStory(
      page_module.Page('http://www.baz.com/', story_set, story_set.base_dir,
                       name='Baz',
                       grouping_keys={'case': 'test', 'type': 'key'}))
  return story_set


def _HasBenchmark(tests_dict, benchmark_name):
  return tests_dict.get(benchmark_name, None) != None


def _HasStory(benchmark_dict, story_name):
  return benchmark_dict.get(story_name) != None


class Json3OutputFormatterTest(unittest.TestCase):
  def setUp(self):
    self._output = StringIO.StringIO()
    self._story_set = _MakeStorySet()
    self._formatter = json_3_output_formatter.JsonOutputFormatter(
        self._output)

  def testOutputAndParse(self):
    results = page_test_results.PageTestResults()
    results.telemetry_info.benchmark_start_epoch = 1501773200
    self._output.truncate(0)

    results.WillRunPage(self._story_set[0])
    v0 = scalar.ScalarValue(results.current_page, 'foo', 'seconds', 3,
                            improvement_direction=improvement_direction.DOWN)
    results.AddValue(v0)
    results.DidRunPage(self._story_set[0])

    self._formatter.Format(results)
    json.loads(self._output.getvalue())

  def testAsDictBaseKeys(self):
    results = page_test_results.PageTestResults()
    results.telemetry_info.benchmark_start_epoch = 1501773200
    d = json_3_output_formatter.ResultsAsDict(results)

    self.assertEquals(d['interrupted'], False)
    self.assertEquals(d['num_failures_by_type'], {})
    self.assertEquals(d['path_delimiter'], '/')
    self.assertEquals(d['seconds_since_epoch'], 1501773200)
    self.assertEquals(d['tests'], {})
    self.assertEquals(d['version'], 3)

  def testAsDictWithOnePage(self):
    results = page_test_results.PageTestResults()
    results.telemetry_info.benchmark_start_epoch = 1501773200
    results.telemetry_info.benchmark_name = 'benchmark_name'
    results.WillRunPage(self._story_set[0])
    v0 = scalar.ScalarValue(results.current_page, 'foo', 'seconds', 3,
                            improvement_direction=improvement_direction.DOWN)
    results.AddValue(v0)
    results.DidRunPage(self._story_set[0])

    d = json_3_output_formatter.ResultsAsDict(results)

    self.assertTrue(_HasBenchmark(d['tests'], 'benchmark_name'))
    self.assertTrue(_HasStory(d['tests']['benchmark_name'], 'Foo'))
    story_result = d['tests']['benchmark_name']['Foo']
    self.assertEquals(story_result['actual'], 'PASS')
    self.assertEquals(story_result['expected'], 'PASS')
    self.assertEquals(d['num_failures_by_type'], {'PASS': 1})

  def testAsDictWithTwoPages(self):
    results = page_test_results.PageTestResults()
    results.telemetry_info.benchmark_start_epoc = 1501773200
    results.telemetry_info.benchmark_name = 'benchmark_name'
    results.WillRunPage(self._story_set[0])
    v0 = scalar.ScalarValue(results.current_page, 'foo', 'seconds', 3,
                            improvement_direction=improvement_direction.DOWN)
    results.AddValue(v0)
    results.DidRunPage(self._story_set[0])

    results.WillRunPage(self._story_set[1])
    v1 = scalar.ScalarValue(results.current_page, 'bar', 'seconds', 4,
                            improvement_direction=improvement_direction.DOWN)
    results.AddValue(v1)
    results.DidRunPage(self._story_set[1])

    d = json_3_output_formatter.ResultsAsDict(results)

    self.assertTrue(_HasBenchmark(d['tests'], 'benchmark_name'))
    self.assertTrue(_HasStory(d['tests']['benchmark_name'], 'Foo'))
    story_result = d['tests']['benchmark_name']['Foo']
    self.assertEquals(story_result['actual'], 'PASS')
    self.assertEquals(story_result['expected'], 'PASS')

    self.assertTrue(_HasBenchmark(d['tests'], 'benchmark_name'))
    self.assertTrue(_HasStory(d['tests']['benchmark_name'], 'Bar'))
    story_result = d['tests']['benchmark_name']['Bar']
    self.assertEquals(story_result['actual'], 'PASS')
    self.assertEquals(story_result['expected'], 'PASS')

    self.assertEquals(d['num_failures_by_type'], {'PASS': 2})

  def testAsDictWithRepeatedTests(self):
    results = page_test_results.PageTestResults()
    results.telemetry_info.benchmark_start_epoch = 1501773200
    results.telemetry_info.benchmark_name = 'benchmark_name'

    results.WillRunPage(self._story_set[0])
    v0 = scalar.ScalarValue(results.current_page, 'foo', 'seconds', 3,
                            improvement_direction=improvement_direction.DOWN)
    results.AddValue(v0)
    results.DidRunPage(self._story_set[0])

    results.WillRunPage(self._story_set[1])
    results.Skip('fake_skip')
    results.DidRunPage(self._story_set[1])

    results.WillRunPage(self._story_set[0])
    v0 = scalar.ScalarValue(results.current_page, 'foo', 'seconds', 3,
                            improvement_direction=improvement_direction.DOWN)
    results.AddValue(v0)
    results.DidRunPage(self._story_set[0])

    results.WillRunPage(self._story_set[1])
    results.Skip('fake_skip')
    results.DidRunPage(self._story_set[1])

    d = json_3_output_formatter.ResultsAsDict(results)
    foo_story_result = d['tests']['benchmark_name']['Foo']
    self.assertEquals(foo_story_result['actual'], 'PASS')
    self.assertEquals(foo_story_result['expected'], 'PASS')

    bar_story_result = d['tests']['benchmark_name']['Bar']
    self.assertEquals(bar_story_result['actual'], 'SKIP')
    self.assertEquals(bar_story_result['expected'], 'SKIP')

    self.assertEquals(d['num_failures_by_type'], {'SKIP': 2, 'PASS': 2})


  def testAsDictWithSkippedAndFailedTests(self):
    results = page_test_results.PageTestResults()
    results.telemetry_info.benchmark_start_epoch = 1501773200
    results.telemetry_info.benchmark_name = 'benchmark_name'

    results.WillRunPage(self._story_set[0])
    v0 = scalar.ScalarValue(results.current_page, 'foo', 'seconds', 3,
                            improvement_direction=improvement_direction.DOWN)
    results.AddValue(v0)
    results.DidRunPage(self._story_set[0])

    results.WillRunPage(self._story_set[1])
    v1 = scalar.ScalarValue(results.current_page, 'bar', 'seconds', 4,
                            improvement_direction=improvement_direction.DOWN)
    results.AddValue(v1)
    results.DidRunPage(self._story_set[1])

    results.WillRunPage(self._story_set[0])
    results.Skip('fake_skip')
    results.DidRunPage(self._story_set[0])

    results.WillRunPage(self._story_set[1])
    results.Fail('fake_failure')
    results.DidRunPage(self._story_set[1])

    d = json_3_output_formatter.ResultsAsDict(results)

    foo_story_result = d['tests']['benchmark_name']['Foo']
    self.assertEquals(foo_story_result['actual'], 'PASS SKIP')
    self.assertEquals(foo_story_result['expected'], 'PASS SKIP')
    self.assertFalse(foo_story_result['is_unexpected'])

    bar_story_result = d['tests']['benchmark_name']['Bar']
    self.assertEquals(bar_story_result['actual'], 'PASS FAIL')
    self.assertEquals(bar_story_result['expected'], 'PASS')
    self.assertTrue(bar_story_result['is_unexpected'])

    self.assertEquals(
        d['num_failures_by_type'], {'PASS': 2, 'FAIL': 1, 'SKIP': 1})

  def testIntegrationCreateJsonTestResultsWithDisabledBenchmark(self):
    benchmark_metadata = benchmark.BenchmarkMetadata('test_benchmark')
    options = options_for_unittests.GetCopy()
    options.output_formats = ['json-test-results']
    options.upload_results = False
    tempfile_dir = 'unittest_results'
    options.output_dir = tempfile_dir
    options.suppress_gtest_report = False
    options.results_label = None
    parser = options.CreateParser()
    results_options.ProcessCommandLineArgs(parser, options)
    results = results_options.CreateResults(
        benchmark_metadata, options, benchmark_enabled=False)
    results.PrintSummary()
    results.CloseOutputFormatters()

    tempfile_name = os.path.join(tempfile_dir, 'test-results.json')
    with open(tempfile_name) as f:
      json_test_results = json.load(f)
    shutil.rmtree(tempfile_dir)

    self.assertEquals(json_test_results['interrupted'], False)
    self.assertEquals(json_test_results['num_failures_by_type'], {})
    self.assertEquals(json_test_results['path_delimiter'], '/')
    self.assertAlmostEqual(json_test_results['seconds_since_epoch'],
                           time.time(), 1)
    self.assertEquals(json_test_results['tests'], {})
    self.assertEquals(json_test_results['version'], 3)

  def testIntegrationCreateJsonTestResults(self):
    benchmark_metadata = benchmark.BenchmarkMetadata('test_benchmark')
    options = options_for_unittests.GetCopy()
    options.output_formats = ['json-test-results']
    options.upload_results = False
    tempfile_dir = 'unittest_results'
    options.output_dir = tempfile_dir
    options.suppress_gtest_report = False
    options.results_label = None
    parser = options.CreateParser()
    results_options.ProcessCommandLineArgs(parser, options)
    results = results_options.CreateResults(benchmark_metadata, options)

    story_set = story.StorySet(base_dir=os.path.dirname(__file__))
    test_page = page_module.Page(
        'http://www.foo.com/', story_set, story_set.base_dir, name='Foo')
    results.WillRunPage(test_page)
    v0 = scalar.ScalarValue(
        results.current_page,
        'foo',
        'seconds',
        3,
        improvement_direction=improvement_direction.DOWN)
    results.AddValue(v0)
    results.current_page_run.SetDuration(5.0123)
    results.DidRunPage(test_page)
    results.PrintSummary()
    results.CloseOutputFormatters()

    tempfile_name = os.path.join(tempfile_dir, 'test-results.json')
    with open(tempfile_name) as f:
      json_test_results = json.load(f)
    shutil.rmtree(tempfile_dir)

    self.assertEquals(json_test_results['interrupted'], False)
    self.assertEquals(json_test_results['num_failures_by_type'], {'PASS': 1})
    self.assertEquals(json_test_results['path_delimiter'], '/')
    self.assertAlmostEqual(json_test_results['seconds_since_epoch'],
                           time.time(), delta=1)
    testBenchmarkFoo = json_test_results['tests']['test_benchmark']['Foo']
    self.assertEquals(testBenchmarkFoo['actual'], 'PASS')
    self.assertEquals(testBenchmarkFoo['expected'], 'PASS')
    self.assertFalse(testBenchmarkFoo['is_unexpected'])
    self.assertEquals(testBenchmarkFoo['time'], 5.0123)
    self.assertEquals(testBenchmarkFoo['times'][0], 5.0123)
    self.assertEquals(json_test_results['version'], 3)
