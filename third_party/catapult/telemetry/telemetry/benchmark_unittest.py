# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import unittest

from telemetry import android
from telemetry import benchmark
from telemetry.testing import options_for_unittests
from telemetry.timeline import chrome_trace_category_filter
from telemetry.internal import story_runner
from telemetry import page
from telemetry.page import legacy_page_test
from telemetry.page import shared_page_state
from telemetry import story as story_module
from telemetry.web_perf import timeline_based_measurement

from tracing.value.diagnostics import generic_set


class DummyPageTest(legacy_page_test.LegacyPageTest):
  def ValidateAndMeasurePage(self, *_):
    pass


class TestBenchmark(benchmark.Benchmark):
  def __init__(self, story):
    super(TestBenchmark, self).__init__()
    self._story_set = story_module.StorySet()
    self._story_set.AddStory(story)

  def CreatePageTest(self, _):
    return DummyPageTest()

  def CreateStorySet(self, _):
    return self._story_set


class BenchmarkTest(unittest.TestCase):

  def testPageTestWithIncompatibleStory(self):
    b = TestBenchmark(story_module.Story(
        name='test story',
        shared_state_class=shared_page_state.SharedPageState))
    with self.assertRaisesRegexp(
        Exception, 'containing only telemetry.page.Page stories'):
      b.Run(options_for_unittests.GetCopy())

    state_class = story_module.SharedState
    b = TestBenchmark(story_module.Story(
        name='test benchmark',
        shared_state_class=state_class))
    with self.assertRaisesRegexp(
        Exception, 'containing only telemetry.page.Page stories'):
      b.Run(options_for_unittests.GetCopy())

    b = TestBenchmark(android.AndroidStory(
        name='test benchmark', start_intent=None))
    with self.assertRaisesRegexp(
        Exception, 'containing only telemetry.page.Page stories'):
      b.Run(options_for_unittests.GetCopy())

  def testPageTestWithCompatibleStory(self):
    original_run_fn = story_runner.Run
    was_run = [False]
    def RunStub(*arg, **kwargs):
      del arg, kwargs
      was_run[0] = True
    story_runner.Run = RunStub

    try:
      options = options_for_unittests.GetCopy()
      options.output_formats = ['none']
      options.suppress_gtest_report = True
      parser = optparse.OptionParser()
      benchmark.AddCommandLineArgs(parser)
      options.MergeDefaultValues(parser.get_default_values())

      b = TestBenchmark(page.Page(url='about:blank', name='about:blank'))
      b.Run(options)
    finally:
      story_runner.Run = original_run_fn

    self.assertTrue(was_run[0])

  def testBenchmarkMakesTbmTestByDefault(self):
    class DefaultTbmBenchmark(benchmark.Benchmark):
      pass

    self.assertIsInstance(
        DefaultTbmBenchmark().CreatePageTest(options=None),
        timeline_based_measurement.TimelineBasedMeasurement)

  def testUnknownTestTypeRaises(self):
    class UnknownTestType(object):
      pass
    class UnknownTestTypeBenchmark(benchmark.Benchmark):
      test = UnknownTestType

    type_error_regex = (
        '"UnknownTestType" is not a PageTest or a TimelineBasedMeasurement')
    with self.assertRaisesRegexp(TypeError, type_error_regex):
      UnknownTestTypeBenchmark().CreatePageTest(options=None)

  def testBenchmarkWithOverridenShouldAddValue(self):
    class ShouldNotAddValueBenchmark(TestBenchmark):

      @classmethod
      def ShouldAddValue(cls, unused_value, unused_is_first_result):
        return False

    original_run_fn = story_runner.Run
    valid_should_add_value = [False]

    def RunStub(test, story_set_module, finder_options, results,
                *args, **kwargs): # pylint: disable=unused-argument
      should_add_value = results._should_add_value
      valid = should_add_value == ShouldNotAddValueBenchmark.ShouldAddValue
      valid_should_add_value[0] = valid

    story_runner.Run = RunStub

    try:
      options = options_for_unittests.GetCopy()
      options.output_formats = ['none']
      options.suppress_gtest_report = True
      parser = optparse.OptionParser()
      benchmark.AddCommandLineArgs(parser)
      options.MergeDefaultValues(parser.get_default_values())

      b = ShouldNotAddValueBenchmark(
          page.Page(url='about:blank', name='about:blank'))
      b.Run(options)
    finally:
      story_runner.Run = original_run_fn

    self.assertTrue(valid_should_add_value[0])

  def testBenchmarkExpectationsEmpty(self):
    b = TestBenchmark(story_module.Story(
        name='test name',
        shared_state_class=shared_page_state.SharedPageState))
    self.assertIsInstance(
        b.expectations, story_module.expectations.StoryExpectations)

  def testGetOwners(self):
    @benchmark.Owner(emails=['alice@chromium.org'])
    class FooBenchmark(benchmark.Benchmark):
      @classmethod
      def Name(cls):
        return "foo"

    @benchmark.Owner(emails=['bob@chromium.org', 'ben@chromium.org'],
                     component='xyzzyx')
    class BarBenchmark(benchmark.Benchmark):
      @classmethod
      def Name(cls):
        return "bar"

    @benchmark.Owner(component='xyzzyx')
    class BazBenchmark(benchmark.Benchmark):
      @classmethod
      def Name(cls):
        return "baz"

    foo_owners_diagnostic = FooBenchmark(None).GetOwners()
    bar_owners_diagnostic = BarBenchmark(None).GetOwners()
    baz_owners_diagnostic = BazBenchmark(None).GetOwners()

    self.assertIsInstance(foo_owners_diagnostic, generic_set.GenericSet)
    self.assertIsInstance(bar_owners_diagnostic, generic_set.GenericSet)
    self.assertIsInstance(baz_owners_diagnostic, generic_set.GenericSet)

    self.assertEqual(foo_owners_diagnostic.AsDict()['values'],
                     ['alice@chromium.org'])
    self.assertEqual(bar_owners_diagnostic.AsDict()['values'],
                     ['bob@chromium.org', 'ben@chromium.org'])
    self.assertEqual(baz_owners_diagnostic.AsDict()['values'], [])

  def testGetBugComponents(self):
    @benchmark.Owner(emails=['alice@chromium.org'])
    class FooBenchmark(benchmark.Benchmark):
      @classmethod
      def Name(cls):
        return "foo"

    @benchmark.Owner(emails=['bob@chromium.org'], component='xyzzyx')
    class BarBenchmark(benchmark.Benchmark):
      @classmethod
      def Name(cls):
        return "bar"

    foo_bug_components_diagnostic = FooBenchmark(None).GetBugComponents()
    bar_bug_components_diagnostic = BarBenchmark(None).GetBugComponents()

    self.assertIsInstance(foo_bug_components_diagnostic, generic_set.GenericSet)
    self.assertIsInstance(bar_bug_components_diagnostic, generic_set.GenericSet)

    self.assertEqual(list(foo_bug_components_diagnostic), [])
    self.assertEqual(list(bar_bug_components_diagnostic), ['xyzzyx'])

  def testGetTBMOptionsSupportsLegacyName(self):
    class TbmBenchmark(benchmark.Benchmark):
      def CreateTimelineBasedMeasurementOptions(self):
        return 'Legacy'

    options = TbmBenchmark(None)._GetTimelineBasedMeasurementOptions(None)
    self.assertEqual(options, 'Legacy')

  def testGetTBMOptionsSupportsNewName(self):
    class TbmBenchmark(benchmark.Benchmark):
      def CreateCoreTimelineBasedMeasurementOptions(self):
        return 'New'

    options = TbmBenchmark(None)._GetTimelineBasedMeasurementOptions(None)
    self.assertEqual(options, 'New')

  def testGetTBMOptionsBothAsserts(self):
    # TODO(sullivan): remove this test after fully removing
    # CreateCoreTimelineBasedMeasurementOptions.
    class TbmBenchmark(benchmark.Benchmark):
      def CreateTimelineBasedMeasurementOptions(self):
        return 'Legacy'
      def CreateCoreTimelineBasedMeasurementOptions(self):
        return 'New'


    with self.assertRaisesRegexp(
        AssertionError, 'Benchmarks should override'):
      TbmBenchmark(None)._GetTimelineBasedMeasurementOptions(None)

  def testChromeTraceOptionsUpdateFilterString(self):
    class TbmBenchmark(benchmark.Benchmark):
      def CreateCoreTimelineBasedMeasurementOptions(self):
        tbm_options = timeline_based_measurement.Options(
            chrome_trace_category_filter.ChromeTraceCategoryFilter(
                filter_string='rail,toplevel'))
        tbm_options.config.enable_chrome_trace = True
        return tbm_options

    options = options_for_unittests.GetCopy()
    options.extra_chrome_categories = 'toplevel,net'
    parser = optparse.OptionParser()
    benchmark.AddCommandLineArgs(parser)
    options.MergeDefaultValues(parser.get_default_values())

    b = TbmBenchmark(None)
    tbm = b.CreatePageTest(options)
    self.assertEqual(
        'net,rail,toplevel',
        tbm.tbm_options.category_filter.stable_filter_string)

  def testAtraceOptionsTurnsOnAtrace(self):
    class TbmBenchmark(benchmark.Benchmark):
      def CreateCoreTimelineBasedMeasurementOptions(self):
        tbm_options = timeline_based_measurement.Options()
        tbm_options.config.atrace_config.categories = []
        return tbm_options

    options = options_for_unittests.GetCopy()
    options.extra_atrace_categories = 'foo,bar'
    parser = optparse.OptionParser()
    benchmark.AddCommandLineArgs(parser)
    options.MergeDefaultValues(parser.get_default_values())

    b = TbmBenchmark(None)
    tbm = b.CreatePageTest(options)
    self.assertTrue(tbm.tbm_options.config.enable_atrace_trace)
    self.assertEqual(
        ['foo', 'bar'],
        tbm.tbm_options.config.atrace_config.categories)

  def testAdditionalAtraceCategories(self):
    class TbmBenchmark(benchmark.Benchmark):
      def CreateCoreTimelineBasedMeasurementOptions(self):
        tbm_options = timeline_based_measurement.Options()
        tbm_options.config.enable_atrace_trace = True
        tbm_options.config.atrace_config.categories = 'string,foo,stuff'
        return tbm_options

    options = options_for_unittests.GetCopy()
    options.extra_atrace_categories = 'foo,bar'
    parser = optparse.OptionParser()
    benchmark.AddCommandLineArgs(parser)
    options.MergeDefaultValues(parser.get_default_values())

    b = TbmBenchmark(None)
    tbm = b.CreatePageTest(options)
    self.assertTrue(tbm.tbm_options.config.enable_atrace_trace)
    self.assertEqual(
        ['string', 'foo', 'stuff', 'bar'],
        tbm.tbm_options.config.atrace_config.categories)

  def testEnableSystrace(self):
    class TbmBenchmark(benchmark.Benchmark):
      def CreateCoreTimelineBasedMeasurementOptions(self):
        return timeline_based_measurement.Options()

    options = options_for_unittests.GetCopy()
    options.enable_systrace = True
    parser = optparse.OptionParser()
    benchmark.AddCommandLineArgs(parser)
    options.MergeDefaultValues(parser.get_default_values())

    b = TbmBenchmark(None)
    tbm = b.CreatePageTest(options)
    self.assertTrue(
        tbm.tbm_options.config.chrome_trace_config.enable_systrace)

  def testCanRunOnPlatformReturnTrue(self):
    b = TestBenchmark(story_module.Story(
        name='test name',
        shared_state_class=shared_page_state.SharedPageState))
    # We can pass None for both arguments because it defaults to ALL for
    # supported platforms, which always returns true.
    self.assertTrue(b._CanRunOnPlatform(None, None))

  def testCanRunOnPlatformReturnFalse(self):
    b = TestBenchmark(story_module.Story(
        name='test name',
        shared_state_class=shared_page_state.SharedPageState))
    b.SUPPORTED_PLATFORMS = [] # pylint: disable=invalid-name
    # We can pass None for both arguments because we select no platforms as
    # supported, which always returns false.
    self.assertFalse(b._CanRunOnPlatform(None, None))

  def testAugmentExpectationsWithParserNoData(self):
    b = TestBenchmark(story_module.Story(
        name='test_name',
        shared_state_class=shared_page_state.SharedPageState))
    b.AugmentExpectationsWithParser('')
    expectations = b.expectations.AsDict()
    self.assertFalse(expectations.get('test_name'))

  def testAugmentExpectationsWithParserData(self):
    b = TestBenchmark(story_module.Story(
        name='test_name',
        shared_state_class=shared_page_state.SharedPageState))
    data = 'crbug.com/123 benchmark_unittest.TestBenchmark/test_name [ Skip ]'
    b.AugmentExpectationsWithParser(data)
    expectations = b.expectations.AsDict()
    self.assertTrue(expectations['stories'].get('test_name'))
