# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import tempfile
import StringIO
import unittest

from telemetry import benchmark
from telemetry import benchmark_runner
from telemetry import story as story_module
from telemetry import page as page_module
import mock


class BenchmarkFoo(benchmark.Benchmark):
  """Benchmark foo for testing."""

  def page_set(self):
    page = page_module.Page('http://example.com', name='dummy_page',
                            tags=['foo', 'bar'])
    story_set = story_module.StorySet()
    story_set.AddStory(page)
    return story_set

  @classmethod
  def Name(cls):
    return 'BenchmarkFoo'


class BenchmarkBar(benchmark.Benchmark):
  """Benchmark bar for testing."""

  def page_set(self):
    return story_module.StorySet()

  @classmethod
  def Name(cls):
    return 'BenchmarkBar'


class BenchmarkRunnerUnittest(unittest.TestCase):

  def setUp(self):
    self._stream = StringIO.StringIO()
    self._json_stream = StringIO.StringIO()
    self._mock_possible_browser = mock.MagicMock()
    self._mock_possible_browser.browser_type = 'TestBrowser'

  def testPrintBenchmarkListWithNoDisabledBenchmark(self):
    expected_printed_stream = (
        'Available benchmarks for TestBrowser are:\n'
        '  BenchmarkBar Benchmark bar for testing.\n'
        '  BenchmarkFoo Benchmark foo for testing.\n'
        'Pass --browser to list benchmarks for another browser.\n\n')
    benchmark_runner.PrintBenchmarkList([BenchmarkBar, BenchmarkFoo],
                                        self._mock_possible_browser, None,
                                        self._stream)
    self.assertEquals(expected_printed_stream, self._stream.getvalue())

  def testPrintBenchmarkListWithOneDisabledBenchmark(self):
    expected_printed_stream = (
        'Available benchmarks for TestBrowser are:\n'
        '  BenchmarkFoo Benchmark foo for testing.\n'
        '\n'
        'Disabled benchmarks for TestBrowser are (force run with -d):\n'
        '  BenchmarkBar Benchmark bar for testing.\n'
        'Pass --browser to list benchmarks for another browser.\n\n')

    expectations_file_contents = (
        '# tags: All\n'
        'crbug.com/123 [ All ] BenchmarkBar/* [ Skip ]\n'
    )

    expectations_file = tempfile.NamedTemporaryFile(bufsize=0, delete=False)
    try:
      expectations_file.write(expectations_file_contents)
      expectations_file.close()
      benchmark_runner.PrintBenchmarkList([BenchmarkFoo, BenchmarkBar],
                                          self._mock_possible_browser,
                                          expectations_file.name,
                                          self._stream)

      self.assertEquals(expected_printed_stream, self._stream.getvalue())

    finally:
      os.remove(expectations_file.name)

  def testPrintBenchmarkListInJSON(self):
    expected_json_stream = json.dumps(
        sorted([
            {'name': BenchmarkFoo.Name(),
             'description': BenchmarkFoo.Description(),
             'enabled': True,
             'story_tags': ['bar', 'foo']},
            {'name': BenchmarkBar.Name(),
             'description': BenchmarkBar.Description(),
             'enabled': False,
             'story_tags': []}], key=lambda b: b['name']),
        indent=4, sort_keys=True, separators=(',', ': '))

    expectations_file_contents = (
        '# tags: All\n'
        'crbug.com/123 [ All ] BenchmarkBar/* [ Skip ]\n'
    )

    expectations_file = tempfile.NamedTemporaryFile(bufsize=0, delete=False)
    try:
      expectations_file.write(expectations_file_contents)
      expectations_file.close()
      benchmark_runner.PrintBenchmarkList([BenchmarkFoo, BenchmarkBar],
                                          self._mock_possible_browser,
                                          expectations_file.name,
                                          self._stream, self._json_stream)

      self.assertEquals(expected_json_stream, self._json_stream.getvalue())

    finally:
      os.remove(expectations_file.name)
