# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import StringIO
import unittest

from telemetry import story
from telemetry import benchmark
from telemetry.internal.results import html_output_formatter
from telemetry.internal.results import page_test_results
from telemetry import page as page_module
from telemetry.value import improvement_direction
from telemetry.value import scalar
from tracing.value import histogram_set
from tracing_build import render_histograms_viewer


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


class HtmlOutputFormatterTest(unittest.TestCase):

  def setUp(self):
    self._output = StringIO.StringIO()
    self._story_set = _MakeStorySet()
    self._benchmark_metadata = benchmark.BenchmarkMetadata(
        'benchmark_name', 'benchmark_description')

  def testBasic(self):
    formatter = html_output_formatter.HtmlOutputFormatter(
        self._output, self._benchmark_metadata, False)
    results = page_test_results.PageTestResults(
        benchmark_metadata=self._benchmark_metadata)
    results.telemetry_info.benchmark_start_epoch = 1501773200

    results.WillRunPage(self._story_set[0])
    v0 = scalar.ScalarValue(results.current_page, 'foo', 'seconds', 3,
                            improvement_direction=improvement_direction.DOWN)
    results.AddValue(v0)
    results.DidRunPage(self._story_set[0])
    results.PopulateHistogramSet()

    formatter.Format(results)
    html = self._output.getvalue()
    dicts = render_histograms_viewer.ReadExistingResults(html)
    histograms = histogram_set.HistogramSet()
    histograms.ImportDicts(dicts)

    self.assertEqual(len(histograms), 1)
    self.assertEqual(histograms.GetFirstHistogram().name, 'foo')

